/*
  Vladimir Feinberg
  condition_queue.h
  2014-08-02

  Condition queues satisfy the following behavior: their
  enqueue and dequeue methods always succeed, even if it means that
  the enqueueing or dequeueing thread has to wait for an item.
*/

#ifndef CONDITION_QUEUE_H_
#define CONDITION_QUEUE_H_

#include "queues/queue.h"

template<typename T>
class dequeue_blocking : public virtual bounded_queue<T> {
};

template<typename T>
class enqueue_blocking : public virtual bounded_queue<T> {
};

#endif
