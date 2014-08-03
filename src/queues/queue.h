/*
   Vladimir Feinberg
   queue.h
   2014-08-02

   Contains FIFO queue interface, which is by default
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdexcept>

template<typename T>
class queue {
 public:
  virtual ~queue() {}
  // Full/empty exceptions
  class full_error : public std::runtime_error {
  public:
    using runtime_error::runtime_error;
  };
  class empty_error : public std::runtime_error {
  public:
    using runtime_error::runtime_error;
  };
  // Observers
  virtual bool full() = 0;
  virtual bool empty() = 0;
  // Calling enqueue/dequeue when the queue is full/empty
  // will result in waiting for the queue to not be empty.
  // In the case of one thread, it makes one should check full()/empty().
  virtual void enqueue(const T&) = 0;
  virtual void enqueue(T&&) = 0;
  virtual T dequeue() = 0;
  // "try" methods will throw in case queue is full/empty.
  virtual void try_enqueue(const T&) = 0;
  virtual void try_enqueue(T&&) = 0;
  virtual T try_dequeue() = 0;
};

#endif /* FIFO_H_ */
