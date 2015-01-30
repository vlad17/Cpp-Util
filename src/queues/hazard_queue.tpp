/*
   Vladimir Feinberg
   queues/hazard_queue.tpp
   2015-01-30

   hazard_queue implementation.
 */

// Implementation details:
//
// See notes in shared_queue.tpp. This queue works the same way, but relies
// on hazard pointers to avoid the ABA problem (a lockfree approach).

#include "synchro/hazard.hpp"

namespace queues {

template<typename T>
bool hazard_queue<T>::is_lock_free() const {
  return std::atomic_is_lock_free(&head_)
      && std::atomic_is_lock_free(&tail_)
      && std::atomic_is_lock_free(&insert_version_)
      && std::atomic_is_lock_free(&remove_version_);
  // TODO: include hazard pointers' is lock free value here.
}

template<typename T>
bool hazard_queue<T>::empty() const {
  // Give a "conservative" estimate, likely to say empty() is true,
  // to prevent eager wakeups and contention, by reading insert version first.
  auto enq = insert_version_.load(std::memory_order_relaxed);
  auto deq = remove_version_.load(std::memory_order_relaxed);
  // invariant: enq >= deq
  return enq == deq;
}

template<typename T>
void hazard_queue<T>::enqueue(node* n) noexcept {
  synchro::hazard_ptr<node> hazard_tail;
  hazard_tail.acquire(tail_);
  node* newnext = nullptr;

  while (!std::atomic_compare_exchange_weak_explicit(
             &hazard_tail->next_, &newnext, n, std::memory_order_release,
             std::memory_order_relaxed)) {
    auto oldtail = hazard_tail.get();
    // Important for progress - other threads help move the tail forward.
    std::atomic_compare_exchange_weak_explicit(&tail_, &oldtail, newnext,
                                               std::memory_order_relaxed,
                                               std::memory_order_relaxed);
    hazard_tail.acquire(tail_);
    newnext = nullptr;
  }

  // Make sure that by the time we are finished enqueuing, a dequeuer
  // is able to remove the head.
  auto oldtail = hazard_tail.get();
  std::atomic_compare_exchange_strong_explicit(
      &tail_, &oldtail, n, std::memory_order_relaxed, std::memory_order_relaxed);

  insert_version_.fetch_add(1, std::memory_order_relaxed);
}

template<typename T>
util::optional<T> hazard_queue<T>::try_dequeue()
{
  // TODO: optimization for dequeue() - only take one hazard_head,
  // spin on that one, instead of making a new one each time.
  synchro::hazard_ptr<node> hazard_head;

  // Cycle until we can "claim" a node for the dequeuer, with the oldhead
  // variable pointing to it.
  while (true) {
    hazard_head.acquire(head_);

    // We can't go ahead of tail, even if we see head->next != nullptr, because
    // enqueuers rely on that node's validity.
    auto oldtail = std::atomic_load_explicit(&tail_, std::memory_order_relaxed);
    if (hazard_head.get() == oldtail) {
      if (hazard_head->val_.valid() && hazard_head->val_.invalidate())
        break;
      // If head == tail and head is invalid, then we're empty.
      return {};
    }

    auto newhead = std::atomic_load_explicit(&hazard_head->next_,
                                             std::memory_order_acquire);
    // It's possible that newhead is null here if a bunch of dequeuers
    // got in right after the if (oldhead == oldtail) returned false
    // above. In order to avoid swapping in the null head, we need to
    // restart the dequeuing process from the top
    if (!newhead) return {};

    auto oldhead = hazard_head.get();
    if (std::atomic_compare_exchange_weak_explicit(
            &head_, &oldhead, newhead, std::memory_order_release,
            std::memory_order_relaxed)) {
      // We have claimed oldhead successfully.
      // Recall hazard_head == oldhead, so we can schedule deletion
      // now (we're safe to use the pointer until we get out of scope).
      synchro::hazard_ptr<node>::schedule_deletion(hazard_head.get());

      // The head could still be invalid.
      // If we had a successful CAS but oldhead is invalid, then this
      // is the case where we were waiting on a head == tail case
      // but tail moved, so we moved head forward to a valid node
      // as well. Just restart dequeue() loop in this case.
      if (oldhead->val_.valid())
        break;
    }
  }

  // NOTE: we do not schedule deletion here; only the thread that
  // successfully CAS-es the head off (guaranteed to just be one)
  // may schedule the deletion. Notice two break statements above
  // lead to two paths that end up here.

  remove_version_.fetch_add(1, std::memory_order_relaxed);

  return {std::move(*hazard_head->val_.get())};
}

// TODO optimization: do manual RVO on the optional by inlining?
template<typename T>
T hazard_queue<T>::dequeue() {
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
hazard_queue<T>::~hazard_queue() {
  for (auto prev = head_.load(std::memory_order_acquire); prev;) {
    auto next = prev->next_.load(std::memory_order_acquire);
    delete prev;
    prev = next;
  }
}

// Requires dequeuers are not operating on the queue (thus, no need for
// hazard pointers).
template<typename T>
std::ostream& operator<<(std::ostream& o, const hazard_queue<T>& q) {
  auto tail = std::atomic_load_explicit(&q.tail_, std::memory_order_relaxed);
  auto head = std::atomic_load_explicit(&q.head_, std::memory_order_acquire);

  o << "[";
  if (head == tail) {
    if (head->val_.valid())
      o << *head->val_.get();
    return o << "]";
  }

  for (auto i = head; i != tail;
       i = std::atomic_load_explicit(&i->next_, std::memory_order_acquire))
    if (i->val_.valid())
      o << *i->val_.get() << ", ";
  return o << *tail->val_.get() << "]";
}

} // namespace queues
