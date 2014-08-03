/*
 * Vladimir Feinberg
 * lock_free_q.h
 * 2014-08-02
 *
 * Lock free FIFO queue, satisfies the MPMC problem.
 */

#ifndef LOCK_FREE_Q_H_
#define LOCK_FREE_Q_H_

#ifndef NDEBUG
#define LOCK_FREE_Q_H_PRINT_DEBUG_STRING
#endif

#include "queues/queue.h"

#include <atomic>
#include <memory>
#ifdef LOCK_FREE_Q_H_PRINT_DEBUG_STRING
#include <ostream>
#endif
#include <stdexcept>
#include <utility>

// wait free? check perf of this against below.
// http://www.cs.technion.ac.il/~erez/Papers/wfquque-ppopp.pdf

// TODO: potential optimization - put test-and-CAS for every CAS
// (check this actually helps)

// Lock free if std::atomic<T*> and shared pointer is.
// T does not have to be synchronized; this class will the needed
// memory bariers for it to be always accessed in a linearized manner.
//
// The queue will never block on enqueue operations. The queue
// will block dequeing threads if empty. "try" methods never block.
template<typename T>
class lfqueue : public queue<T> {
 private:
  struct node {
    node() {
      std::atomic_store_explicit(&next, nullptr, std::memory_order_relaxed);
    }
    std::shared_ptr<node*> next;
  };
  struct valnode : public node {
  valnode(const T& val) :
    val(val) {}
  valnode(T&& val) :
    val(val) {}
    T val;
  };
  // Need to use shared pointers because once a local copy of the
  // head node is made, a fast sequence of dequeuers could come to
  // delete the value pointed to by that copy. On the tail side,
  // using shared pointers also helps avoid ABA.
  std::shared_ptr<node> sentinel;
  std::shared_ptr<node> head; // never null
  std::shared_ptr<valnode> tail; // shared pointer to avoid ABA
  void enqueue(const std::shared_ptr<node>& n) noexcept;
 public:
  // Tail always points to sentinel.next, so an empty queue
  // is represented by head == sentinel and tail == nullptr
  lfqueue() {
    std::atomic_store_explicit(&sentinel, std::make_shared<node>(),
                               std::memory_order_relaxed);
    std::atomic_store_explicit(&head, sentinel, std::memory_order_relaxed);
    std::atomic_store_explicit(&tail, nullptr, std::memory_order_relaxed);
  }
  virtual ~lfqueue() {}
  // Oberservers
  // TODO is_lock_free method
  virtual bool full() {return false;}
  virtual bool empty() {return !tail;}
  // Strong guarantee for enqueues
  // Only construction of T or new node will throw.
  virtual void enqueue(const T& t) {enqueue(std::make_shared<valnode>(t));}
  virtual void enqueue(T&& t) {
    enqueue(std::make_shared<valnode>(std::forward<T>(t)));
  }
  virtual void try_enqueue(const T&) {throw std::runtime_error("Unsupported");}
  virtual void try_enqueue(T&&) {throw std::runtime_error("Unsupported");}
  // Strong guarantee for dequeue - only move can throw
  // In addition, empty_error may be thrown for try_dequeue()
  virtual T dequeue();
  virtual T try_dequeue() {throw std::runtime_error("Unsupported");}
#ifdef LOCK_FREE_Q_H_PRINT_DEBUG_STRING
  // For debugging. Takes a snapshot of the queue and prints
  // [head ... tail]
  template<typename U>
  friend std::ostream& operator<<(std::ostream&, const lfqueue<U>&);
#endif /* LOCK_FREE_Q_H_PRINT_DEBUG_STRING */
};

template<typename T>
void lfqueue<T>::enqueue(const std::shared_ptr<node>& n) noexcept {
  auto oldhead = std::atomic_load_explicit(&head, std::memory_order_relaxed);
  std::shared_ptr<node> newnext = nullptr;

  // For the CAS and store ops below, we need release semantics because the
  // relaxed writes to n (for its T val member) must be made visible to other
  // threads. There's no need for global consistency (which may be necessary
  // when working with two atomics such as tail and head) since this same
  // method writes to tail with release semantics, so the dependency chain is
  // contiguous throughout the method.

  // "Claim" the next head position by swapping the current head's next
  // with our new node. Only the current head has next == nullptr, so
  // it's safe to operate on oldhead.
  while (!std::atomic_compare_exchange_weak(&oldhead->next, &newnext, n,
                                            std::memory_order_release)) {
    oldhead = newnext;
    newnext = nullptr;
    // TODO: once in a while, it might be good to set oldhead = load(head)
    // since some threads may really start lagging behind. However, this will
    // increase head contention.
  }

  // Now, we've acquired our stake to next. Because of the above loop,
  // only one thread can be below this comment at any given time.

  // If this was an insertion into an empty queue, update tail
  if (!atomic_load_explicit(&tail, std::memory_order_relaxed))
    std::atomic_store(&tail, n, std::memory_order_release);

  // There's no need to worry about dequeurs getting ahead of the head.
  // The global head (currently one behind this local one) is all that's
  // visible to the enqueuers, who fail on the CAS with an expected null
  // value. The dequeurs may at this point delete up to and past the local
  // head, setting tail back to null. Because of shared pointers, the local
  // head is still valid. Note that in this last case, this may cause an extra
  // value to temporarily be stored (at most one such value can exist, though).
  std::atomic_store(&head, n, std::memory_order_relaxed);
}

template<typename T>
T lfqueue<T>::dequeue()
{
  // Require acquire semantics on tail - see enqueue() for explanation.
  // ABA is avoided by using shared pointers - if a local
  // pointer is the same as one in the atomic tail register,
  // then the value pointed to by the local pointer is the same as
  // it was when the local pointer was last written to.
  auto oldtail = std::atomic_load_explicit(&tail, std::memory_order_acquire);
  while (true) {
    // TODO optimization: condition wait on tail != nullptr
    while (oldtail == nullptr)
      oldtail = std::atomic_load_explicit(&tail, std::memory_order_acquire);
    // Swap in the successor
    auto newtail = std::atomic_load_explicit(&oldtail->next,
                                             std::memory_order_relaxed);
    if (std::atomic_compare_exchange_weak(&tail, &oldtail, newtail,
                                          std::memory_order_acquire))
      break;
  }

  // No other enqueue() or dequeue() thread will access oldtail past this point
  // ABA impossible since new will never offer resource pointed to by oldtail
  // until this thread finishes method and destructor deletes.
  return std::move(oldtail->val);
}

#ifdef LOCK_FREE_Q_H_PRINT_DEBUG_STRING

template<typename T>
std::ostream& operator<<(std::ostream& o, const lfqueue<T>& q) {
  // Snapshot tail, then head. That way it is guaranteed
  // tail is at most one ahead of head (in the edge case where a recently
  // inserted local head is dequeued since it is linked in the list, but
  // the head has not been updated yet). We need to use acquire for tail
  // and next pointers to ensure the values are constructed.
  auto tail = std::atomic_load_explicit(&q.tail, std::memory_order_acquire);
  auto head = std::atomic_load_explicit(&q.head, std::memory_order_relaxed);

  if (head->next == tail)
    return o << "[]";
  o << "[";
  while (tail != head) {
    o << tail->val << ", ";
    tail = std::atomic_load_explicit(&tail->next, std::memory_order_acquire);
  }
  o << tail->val << "]";
}

#endif /* LOCK_FREE_Q_H_PRINT_DEBUG_STRING */

#undef LOCK_FREE_Q_H_PRINT_DEBUG_STRING

#endif /* LOCK_FREE_Q_H_ */
