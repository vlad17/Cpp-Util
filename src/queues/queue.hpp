/*
  Vladimir Feinberg
  queues/queue.hpp
  2014-09-08

  Contains FIFO queue interface.
*/

#ifndef QUEUES_QUEUE_HPP_
#define QUEUES_QUEUE_HPP_

#include "util/optional.hpp"

namespace queues {

template<typename T>
class queue {
 public:
  virtual ~queue() {}
  // Observers
  virtual bool full() const = 0;
  virtual bool empty() const = 0;
  // Calling enqueue/dequeue when the queue is full/empty
  // will result in waiting for the queue to not be empty.
  // In the case of one thread, it makes one should check full()/empty().
  virtual void enqueue(T) = 0;
  virtual T dequeue() = 0;
  // "try" methods don't block - enqueue returns false if full and
  // dequeue returns an invalid (unconstructed) optional if empty.
  virtual void try_enqueue(T) = 0;
  virtual util::optional<T> try_dequeue() = 0;
};

} // namespace queues

#endif /* QUEUES_QUEUE_HPP_ */
