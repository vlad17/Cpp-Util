/*
 * Vladimir Feinberg
 * queues/hazard_queue.hpp
 * 2015-01-30
 *
 * Lock free FIFO queue, satisfies the MPMC problem.
 * Implemented with hazard pointers.
 *
 * This queue was my own invention, but it was influenced by
 * the advice in The Art of Multiprocessor Programming.
 */

#ifndef QUEUES_HAZARD_QUEUE_HPP_
#define QUEUES_HAZARD_QUEUE_HPP_

#include <atomic>
#include <memory>
#include <ostream>

#include "queues/queue.hpp"
#include "util/atomic_optional.hpp"
#include "util/optional.hpp"

namespace queues {

template<typename T>
class hazard_queue;

template<typename T>
std::ostream& operator<<(std::ostream&, const hazard_queue<T>&);

// T does not have to be synchronized; this class will provide the needed
// memory bariers for it to be always accessed in a linearized manner.
//
// The queue will never block on enqueue operations. The queue
// will block dequeing threads if empty. "try" methods never block.
template<typename T>
class hazard_queue : public queue<T> {
 private:
  struct node {
    node() : next_(nullptr) {}
    node(T&& val) : next_(nullptr), val_(std::forward<T>(val)) {}

    std::atomic<node*> next_;
    util::atomic_optional<T> val_;
  };

  // We will rely on hazard pointers to avoid ABA problem.
  std::atomic<node*> head_;
  // TODO: potential optimization, store in different cache lines
  std::atomic<node*> tail_;

  // Below versioning needed for empty()
  std::atomic<size_t> insert_version_; // number enqueued
  std::atomic<size_t> remove_version_; // number dequeued

  void enqueue(node* n) noexcept;

 public:
  hazard_queue() :
    insert_version_(0), remove_version_(0) {
    node* n = new node;
    std::atomic_store_explicit(&tail_, n, std::memory_order_relaxed);
    std::atomic_store_explicit(&head_, n, std::memory_order_relaxed);
  }
  virtual ~hazard_queue();
  // Oberservers - full() and empty() may not be very meaningful.
  bool is_lock_free() const;
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
  friend std::ostream& operator<< <>(std::ostream&, const hazard_queue&);
};

} // namespace queues

#include "queues/hazard_queue.tpp"

#endif /* QUEUES_HAZARD_QUEUE_HPP_ */
