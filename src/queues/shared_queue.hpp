/*
 * Vladimir Feinberg
 * queues/shared_queue.hpp
 * 2014-09-08
 *
 * Lock free FIFO queue, satisfies the MPMC problem.
 * Implemented using a shared pointer at the tail side.
 * This queue was my own invention, but it was influenced by
 * the advice in The Art of Multiprocessor Programming.
 *
 * For the most part the queue's lock free, but it relies
 * on an atomic shared_ptr, which may not be (the condition
 * variable for empty queues and its mutex don't count).
 */

#ifndef QUEUES_SHARED_QUEUE_HPP_
#define QUEUES_SHARED_QUEUE_HPP_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <ostream>

#include "synchro/atomic_shared.hpp"
#include "queues/queue.hpp"
#include "util/atomic_optional.hpp"
#include "util/optional.hpp"

namespace queues {

template<typename T>
class shared_queue;

template<typename T>
std::ostream& operator<<(std::ostream&, const shared_queue<T>&);

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
  synchro::atomic_shared_ptr<node> head_;
  std::atomic<node*> tail_;

  // Mutex and condition variable for blocked dequeuers waiting on
  // the "empty" condition.
  std::condition_variable empty_condition_;
  std::mutex empty_mutex_;

  // Below versioning needed for empty()
  std::atomic<size_t> insert_version_; // number enqueued
  std::atomic<size_t> remove_version_; // number dequeued

  void enqueue(node* n) noexcept;

 public:
  shared_queue() :
    insert_version_(0), remove_version_(0) {
    auto sptr = std::make_shared<node>();
    tail_.store(sptr.get(), std::memory_order_relaxed);
    std::atomic_store_explicit(&head_, sptr, std::memory_order_relaxed);
  }
  virtual ~shared_queue();
  // Oberservers - full() and empty() may not be very meaningful.
  bool is_lock_free() const; // ignores the std::condition_variable
  virtual bool full() const { return false; }
  virtual bool empty() const;
  // Strong guarantee for enqueues
  // Only construction of T or new node will throw.
  virtual void enqueue(T t) { enqueue(new node(std::move(t))); }
  virtual void try_enqueue(T t) { enqueue(std::move(t)); }
  // Strong guarantee for dequeue - only move can throw
  // In addition, empty_error may be thrown for try_dequeue()
  virtual T dequeue();
  virtual util::optional<T> try_dequeue();
  // For debugging. Prints [head ... tail]. Requires external locking
  // so that no operations occur on the queue.
  friend std::ostream& operator<< <>(std::ostream&, const shared_queue&);
};

} // namespace queues

#include "queues/shared_queue.tpp"

#endif /* QUEUES_SHARED_QUEUE_HPP_ */
