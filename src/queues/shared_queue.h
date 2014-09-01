/*
 * Vladimir Feinberg
 * shared_queue.h
 * 2014-08-31
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
#include <queue>

#include "sync/atomic_shared.h"
#include "queues/queue.h"
#include "utilities/atomic_optional.h"

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
  atomic_shared_ptr<node> head;
  std::atomic<node*> tail;

  void enqueue(node* n) noexcept;

 public:
  shared_queue() {
    auto sptr = std::make_shared<node>();
    tail.store(sptr.get(), std::memory_order_relaxed);
    std::atomic_store_explicit(&head, sptr, std::memory_order_relaxed);
  }
  virtual ~shared_queue();
  // Oberservers
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
  virtual T try_dequeue() {throw std::runtime_error("Unsupported");}
  // For debugging. Prints [head ... tail]. Requires external locking
  // so that no operations occur on the queue.
  friend std::ostream& operator<< <>(std::ostream&, const shared_queue&);
};

#include "queues/shared_queue.tpp"

#endif /* QUEUES_SHARED_QUEUE_H_ */
