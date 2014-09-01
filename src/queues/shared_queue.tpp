/*
   Vladimir Feinberg
   shared_queue.tpp
   2014-08-31

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

template<typename T>
bool shared_queue<T>::is_lock_free() const {
  return std::atomic_is_lock_free(&head)
    && tail.is_lock_free();
}

template<typename T>
bool shared_queue<T>::empty() const {
  auto oldhead = std::atomic_load_explicit(&head, std::memory_order_relaxed);
  auto oldtail = tail.load(std::memory_order_relaxed);
  return oldhead.get() == oldtail && !oldhead->val.valid();
}

template<typename T>
void shared_queue<T>::enqueue(node* n) noexcept {
  auto oldtail = tail.load(std::memory_order_relaxed);
  node* newnext = nullptr;

  // Require release semantics on node->next pointer - see documentation above
  // Set head->next to n if it is still null
  while (!oldtail->next.compare_exchange_weak(newnext, n,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
    // Validity of newnext in the commented CAS below is guaranteed
    // if the CAS succeeds b/c it meant that right before the CAS the tail
    // was still oldtail, so its next could not have been deleted.
    // In the case it does not succeed, we don't care since we wipe newnext.
    tail.compare_exchange_weak(oldtail, newnext, std::memory_order_relaxed,
                               std::memory_order_relaxed);
    newnext = nullptr;
  }

  // Make sure that by the time we are finished enqueuing, a dequeuer
  // is able to remove the head.
  tail.compare_exchange_strong(oldtail, n, std::memory_order_relaxed,
                               std::memory_order_relaxed);
}

// TODO remove with condition variable
#include <chrono>
#include <thread>

template<typename T>
T shared_queue<T>::dequeue()
{
  static std::atomic<int> deleted(0);
  // Specialized destructor shared pointer for local shared pointers
  // which toggles on the actual destruction (no need for atomics
  // here becuase of other memory barriers.
  struct ToggleDeleter {
    void operator()(node* n) {
      if (enabled)
        delete n;
    }
    bool enabled = true;
  };
  ToggleDeleter del;

  // Require acquire semantics on tail->next - see documentation above
  // ABA is avoided by using shared pointers - if a local
  // pointer is the same as one in the atomic tail register,
  // then the value pointed to by the local pointer is the same as
  // it was when the local pointer was last written to.
  auto oldhead = std::atomic_load_explicit(&head, std::memory_order_acquire);

  // Cycle until we can "claim" a node for the dequeuer, with the oldhead
  // variable pointing to it.
  while (true) {
    // We can't go ahead of tail, even if we see head->next != nullptr, because
    // enqueuers rely on that node's validity.
    auto oldtail = tail.load(std::memory_order_relaxed);
    while (oldhead.get() == oldtail) {
      if (oldhead->val.valid()) {
        if (oldhead->val.invalidate())
          goto claimed;
      } else {
        // TODO optimization: else condition wait on tail != head
        std::this_thread::sleep_for(std::chrono::nanoseconds(5));
      }
      // Some enqueues may have occured, so we re-check the tail
      // (and break out the loop if multiple enqueues occured).
      oldtail = tail.load(std::memory_order_relaxed);
    }

    auto newhead = oldhead->next.load(std::memory_order_acquire);
    std::shared_ptr<node> newhead_shared(newhead, del);
    if (std::atomic_compare_exchange_weak_explicit(
            &head, &oldhead, newhead_shared, std::memory_order_release,
            std::memory_order_relaxed)) {
      // We have claimed oldhead successfully, but it could be invalid
      // (if the queue was empty and one insert occured)
      if (oldhead->val.valid())
        goto claimed;
      // If we had a successful CAS but oldhead is invalid, then this
      // is the case where we were waiting on a head == tail case
      // but tail moved, so we moved head forward to a valid node
      // as well. Just restart dequeue() in this case.
    } else {
      // If we failed, we need to make sure to toggle off deletion as
      // newhead_shared will go out of scope.
      ToggleDeleter* actual_del = std::get_deleter<ToggleDeleter>(newhead_shared);
      actual_del->enabled = false;
    }
  }

claimed:
  return std::move(*oldhead->val.get());
}

// As is the normal assumption, the queue should not be in use
// by other threads.
template<typename T>
shared_queue<T>::~shared_queue() {
  auto oldhead = std::atomic_load_explicit(&head, std::memory_order_acquire);
  for (auto i = oldhead->next.load(std::memory_order_acquire);
       i != nullptr;) {
    auto prev = i;
    i = i->next.load(std::memory_order_acquire);
    delete prev;
  }
}

// Requires dequeuers are operating on the queue.
template<typename T>
std::ostream& operator<<(std::ostream& o, const shared_queue<T>& q) {
  auto tail = std::atomic_load_explicit(&q.tail, std::memory_order_relaxed);
  auto head = std::atomic_load_explicit(&q.head, std::memory_order_acquire);

  o << "[";
  if (head.get() == tail) {
    if (head->val.valid())
      o << *head->val.get();
    return o << "]";
  }

  for (auto i = head.get(); i != tail;
       i = i->next.load(std::memory_order_acquire))
    if (i->val.valid())
      o << *i->val.get() << ", ";
  return o << *tail->val.get() << "]";
}
