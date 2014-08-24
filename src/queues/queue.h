/*
   Vladimir Feinberg
   queue.h
   2014-08-17

   Contains FIFO queue interface.
 */

#ifndef QUEUES_QUEUE_H_
#define QUEUES_QUEUE_H_

#include "queues/size_except.h"

template<typename T>
class queue {
 public:
  virtual ~queue() {}
  // Observers
  virtual bool full() = 0;
  virtual bool empty() = 0;
  // Calling enqueue/dequeue when the queue is full/empty
  // will result in waiting for the queue to not be empty.
  // In the case of one thread, it makes one should check full()/empty().
  virtual void enqueue(T) = 0;
  virtual T dequeue() = 0;
  // "try" methods will throw in case queue is full/empty.
  virtual void try_enqueue(T) = 0;
  virtual T try_dequeue() = 0;
};

#endif /* QUEUES_QUEUE_H_ */
