/*
 * Vladimir Feinberg
 * shared_queue.h
 * 2014-08-15
 *
 * Lock free FIFO queue, satisfies the MPMC problem.
 * Implemented using a shared pointer at the tail side.
 * This queue was my own invention, but it was influenced by
 * the advice in The Art of Multiprocessor Programming.
 *
 * For the most part the queue's lock free, but it relies
 * on an atomic shared_ptr, which may not be.
 */

#ifndef QUEUES_SHARED_QUEUE_H_
#define QUEUES_SHARED_QUEUE_H_

#include <atomic>
#include <memory>
#include <ostream>
#include <stdexcept>

#include "sync/atomic_shared.h"
#include "queues/queue.h"
#include "utilities/atomic_optional.h"

template<typename T>
class shared_queue;

template<typename T>
std::ostream& operator<<(std::ostream&, const shared_queue<T>&);

// TODO is_lock_free

// T does not have to be synchronized; this class will the needed
// memory bariers for it to be always accessed in a linearized manner.
//
// The queue will never block on enqueue operations. The queue
// will block dequeing threads if empty. "try" methods never block.
template<typename T>
class shared_queue : public queue<T> {
 private:
  struct node {
    node() : next(nullptr), val() {}
    node(T&& val) : next(nullptr), val(std::forward<T>(val)) {}

    std::atomic<node*> next;
    util::atomic_optional<T> val;
  };

  // Shared pointer at the head to avoid ABA problem -
  // each dequeuer take a local copy which ensures that the node
  // is not deleted by other dequeuers (so no new node will
  // have the same address as)
  atomic_shared_ptr<node> head;
  std::atomic<node*> tail;

  void enqueue(node* n) noexcept;

 public:
  shared_queue() {
    auto sptr = std::make_shared<node>();
    tail.store(sptr.get(), std::memory_order_relaxed);
    std::atomic_store_explicit(&head, sptr, std::memory_order_relaxed);
  }
  virtual ~shared_queue() {}
  // Oberservers
  // TODO is_lock_free method
  virtual bool full() { return false; }
  virtual bool empty();
  // Strong guarantee for enqueues
  // Only construction of T or new node will throw.
  virtual void enqueue(T t) { enqueue(new node(std::move(t))); }
  virtual void try_enqueue(T) {throw std::runtime_error("Unsupported");}
  // Strong guarantee for dequeue - only move can throw
  // In addition, empty_error may be thrown for try_dequeue()
  virtual T dequeue();
  virtual T try_dequeue() {throw std::runtime_error("Unsupported");}
  // For debugging. Prints [head ... tail]. Requires external locking
  // so that no operations occur on the queue.
  friend std::ostream& operator<< <>(std::ostream&, const shared_queue&);
};

// TODO move to tpp

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
bool shared_queue<T>::empty() {
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

template<typename T>
T shared_queue<T>::dequeue()
{
  // Specialized destructor shared pointer for local shared pointers
  // which toggles on the actual destruction (no need for atomics
  // here becuase of other memory barriers.
  struct ToggleDeleter {
    void operator()(node* n) {
      if (enabled) delete n;
    }
    bool enabled;
  };
  ToggleDeleter del;
  del.enabled = true;

  // Require acquire semantics on tail->next - see documentation above
  // ABA is avoided by using shared pointers - if a local
  // pointer is the same as one in the atomic tail register,
  // then the value pointed to by the local pointer is the same as
  // it was when the local pointer was last written to.
  auto oldhead = std::atomic_load_explicit(&head, std::memory_order_acquire);

  // Cycle until we can "claim" a node for the dequeuer, with the oldhead
  // variable pointing to it.
  while (true) {
    auto oldtail = tail.load(std::memory_order_relaxed);
    while (oldhead.get() == oldtail)
      if (oldhead->val.valid()) {
        if (oldhead->val.invalidate())
          goto claimed;
      } // TODO optimization: else condition wait on tail != head

    auto newhead = oldhead->next.load(std::memory_order_acquire);
    std::shared_ptr<node> newhead_shared(newhead, del);
    if (std::atomic_compare_exchange_weak_explicit(
            &head, &oldhead, newhead_shared, std::memory_order_release,
            std::memory_order_relaxed)) {
      // We have claimed oldhead successfully, but it could be invalid
      // (if the queue was empty and one insert occured)
      if (oldhead->val.valid())
        goto claimed;
    }
    // If we failed, we need to make sure to toggle off deletion as
    // newhead_shared will go out of scope.
    ToggleDeleter* actual_del = std::get_deleter<ToggleDeleter>(newhead_shared);
    actual_del->enabled = false;
  }

claimed:
  return std::move(*oldhead->val.get());
}

// Requires no operations are active on the queue
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

#endif /* QUEUES_SHARED_QUEUE_H_ */
