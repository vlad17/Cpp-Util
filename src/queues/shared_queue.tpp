/*
   Vladimir Feinberg
   queues/shared_queue.tpp
   2014-09-08

   shared_queue implementation.
 */

// Implementation details:
//
// The queue is a concurrent singly-linked list with a persistent sentry.
// Enqueue:
//           0. [tail] <- x <- x <- [head]
//           1. x <- [tail] <- x <- x <- [head]
//           2. [tail] <- x <- x <- x <- [head]
//   1. tail->next atomically set to new value
//   2. tail updated to nonnull tail->next (may occur on other threads)
//      insertion is only considered complete if this has occured.
// Dequeue:
//           0. [tail] <- x <- x <- [head]
//           1. [tail] <- x <- [head]
//   1. If not empty, i.e., tail == head and value there is "non-value" (which
//      means that it is an optional instance that is not initialized), then
//      get head->next
//   2. If head is in same place, then swap atomically to head->next.
//
// In the case of exactly 1 item (head == tail but the optional value
// indicates that there still is a value in the node), then set the
// optional flag to "non-value" and return the value itself. Then, to
// enqueue in the empty case, since multiple enqueuers might attempt
// to write into the same node, we don't re-use the node but move tail
// over to a new node that's linked in (as in the regular case). This
// introduces the possibility that a dequeuer finds tail != head but
// the value at head is "non-value", in which case the dequeuer moves
// the head forward, essentially doing an extra dequeue beforehand without
// returning the value.
//
// We need not worry about the memory order for next pointer (relaxed suffices
// because the per-object atomic_optional takes care of the appropriate memory
// fences if we
// Legend:
//   E-x = thread x in enqueue()
//   D-x = thread x in dequeue().
//   (A) >> (B) = A sequenced-before B (must be same thread)
//   (A) -> (B) = A synchronizes-with then happens before B (differnt threads)
// Dependency chain:
// (E-1 constructs T) >> (E-1 release tail->next) ->
// ... time passes, node that was tail now at end ...
// (D-2 acquire [node that was tail]->next) >>
// (D-2 release head = [node that was tail]->next) ->
// (D-3 acquire head) >> (D-3 reads T)
//
// Shared queue invariants:
//   The queue is empty iff head == tail and !head->val.valid()
//     (though we can't use this to check for empty() since this would
//      require to atomic word-sized loads)
//   head and tail always move forward
//   tail is never ahead of head
//   In order for a thread to have exited the enqueue operation,
//     the head must have moved forward at least once, and the enqueued
//     value was at least for some point inside the queue
//   Flip of the above invariant for tail.
//   Head is at most one node behind the node with next = nullptr
//   node->next is written to only once
//   optional value is constructed once
//   Typical FIFO queue invariants (except correctness is less strict,
//     there only needs to be some linearization of all concurrent
//     method calls which has correct FIFO queue behavior for the
//     concurrent queue to be correct)
//   A value can only be invalid in the head.

namespace queues {

template<typename T>
bool shared_queue<T>::is_lock_free() const {
  return std::atomic_is_lock_free(&head_)
      && std::atomic_is_lock_free(&tail_)
      && std::atomic_is_lock_free(&insert_version_)
      && std::atomic_is_lock_free(&remove_version_);
}

template<typename T>
bool shared_queue<T>::empty() const {
  // Give a "conservative" estimate, likely to say empty() is true,
  // to prevent eager wakeups and contention, by reading insert version first.
  auto enq = insert_version_.load(std::memory_order_relaxed);
  auto deq = remove_version_.load(std::memory_order_relaxed);
  // invariant: enq >= deq
  return enq == deq;
}

template<typename T>
void shared_queue<T>::enqueue(node* raw_n) noexcept {
  // oldtail ABA would occur here, but we take care to make sure it's a shared
  // pointer.
  auto oldtail = std::atomic_load_explicit(&tail_, std::memory_order_relaxed);
  std::shared_ptr<node> newnext, n(raw_n, node::deleter);

  // Require release semantics on node->next pointer - see documentation above
  // Set head->next to n if it is still null
  while (!std::atomic_compare_exchange_weak_explicit(
             &oldtail->next, &newnext, n, std::memory_order_release,
             std::memory_order_relaxed)) {
    // Validity of newnext in the commented CAS below is guaranteed
    // if the CAS succeeds b/c it meant that right before the CAS the tail
    // was still oldtail, so its next could not have been deleted.
    // In the case it does not succeed, we don't care since we wipe newnext.
    std::atomic_compare_exchange_weak_explicit(&tail_, &oldtail, newnext,
                                               std::memory_order_relaxed,
                                               std::memory_order_relaxed);
    newnext = nullptr;
  }

  // Make sure that by the time we are finished enqueuing, a dequeuer
  // is able to remove the head.
  std::atomic_compare_exchange_strong_explicit(
      &tail_, &oldtail, n, std::memory_order_relaxed, std::memory_order_relaxed);

  insert_version_.fetch_add(1, std::memory_order_relaxed);
}

template<typename T>
util::optional<T> shared_queue<T>::try_dequeue()
{
  // Require acquire semantics on tail->next - see documentation above
  // ABA is avoided by using shared pointers - if a local
  // pointer is the same as one in the atomic tail register,
  // then the value pointed to by the local pointer is the same as
  // it was when the local pointer was last written to.
  auto oldhead = std::atomic_load_explicit(&head_, std::memory_order_acquire);

  // Cycle until we can "claim" a node for the dequeuer, with the oldhead
  // variable pointing to it.
  while (true) {
    // We can't go ahead of tail, even if we see head->next != nullptr, because
    // enqueuers rely on that node's validity.
    auto oldtail = std::atomic_load_explicit(&tail_, std::memory_order_relaxed);
    if (oldhead.get() == oldtail.get()) {
      if (oldhead->val.valid() && oldhead->val.invalidate())
        break;
      // If head == tail and head is invalid, then we're empty.
      return {};
    }

    auto newhead = std::atomic_load_explicit(&oldhead->next,
                                             std::memory_order_acquire);
    // It's possible that newhead is null here if a bunch of dequeuers
    // got in right after the if (oldhead == oldtail) returned false
    // above. In order to avoid swapping in the null head, we need to
    // restart the dequeuing process from the top
    if (!newhead) return {};

    if (std::atomic_compare_exchange_weak_explicit(
            &head_, &oldhead, newhead, std::memory_order_release,
            std::memory_order_relaxed)) {
      // We have claimed oldhead successfully, but it could be invalid
      // (if the queue was empty and one insert occured)
      if (oldhead->val.valid())
        break;
      // If we had a successful CAS but oldhead is invalid, then this
      // is the case where we were waiting on a head == tail case
      // but tail moved, so we moved head forward to a valid node
      // as well. Just restart dequeue() loop in this case.
    }
  }

  remove_version_.fetch_add(1, std::memory_order_relaxed);

  return {std::move(*oldhead->val.get())};
}

// TODO optimization: do manual RVO on the optional by inlining?
template<typename T>
T shared_queue<T>::dequeue() {
  util::optional<T> opt;
  while (true) {
    opt = try_dequeue();
    if (opt.valid())
      break;
    std::this_thread::yield();
  }
  return std::move(opt.access());
}

// As is the normal assumption, the queue should not be in use
// by other threads.
template<typename T>
shared_queue<T>::~shared_queue() {
  // Manually run through the list to avoid blowing the stack
  // with destructor calls.
  //
  // To destroy manually, we can't invoke the regular shared pointer
  // destructor since that would recursively delete the rest of the
  // list, and thus we have to rely on a custom deletion function.
  //
  // Usually, the node deletes itself (i.e., during normal queue operation
  // in its lifetime). However, during destruction we disable for the reasons
  // above.
  //
  // We also take care to clear out shared pointers to nodes, so that we're
  // not left with the local variable 'i' being the second-to-last
  // shared owner of the node (we want it to be exactly the last one).

  auto oldhead = std::atomic_load_explicit(&head_, std::memory_order_acquire);
  auto start = std::atomic_load_explicit(&oldhead->next,
                                         std::memory_order_acquire);
  std::atomic_store_explicit(&oldhead->next, std::shared_ptr<node>(),
                             std::memory_order_relaxed);
  auto oldtail = std::atomic_load_explicit(&tail_, std::memory_order_relaxed);
  for (auto i = start; i != oldtail && i != nullptr;) {
    start.reset();
    auto to_delete = i.get();
    to_delete->delete_self = false;

    // Note deleter called below in assignement; (*i)::operator().
    i = std::atomic_load_explicit(&i->next, std::memory_order_acquire);

    std::atomic_store_explicit(&to_delete->next, std::shared_ptr<node>(),
                               std::memory_order_relaxed);
    delete to_delete;
  }

  // Let the queue's shared_ptrs take care of their own memory.
  oldhead->delete_self = true;
  oldtail->delete_self = true;
}

// Requires dequeuers are not operating on the queue.
template<typename T>
std::ostream& operator<<(std::ostream& o, const shared_queue<T>& q) {
  auto tail = std::atomic_load_explicit(&q.tail_, std::memory_order_relaxed);
  auto head = std::atomic_load_explicit(&q.head_, std::memory_order_acquire);

  o << "[";
  if (head.get() == tail.get()) {
    if (head->val.valid())
      o << *head->val.get();
    return o << "]";
  }

  for (auto i = head; i != tail;
       i = std::atomic_load_explicit(&i->next, std::memory_order_acquire))
    if (i->val.valid())
      o << *i->val.get() << ", ";
  return o << *tail->val.get() << "]";
}

} // namespace queues
