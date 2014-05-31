/*
 * Vladimir Feinberg
 * fifo.h
 *
 * Contains FIFO queue interface. The queue provides "sequence number"
 * information that lets the threads know the relative order of the queue.
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <functional>

template<typename T>
class queue
{
protected:
	queue() {}
public:
	virtual ~queue() {}
	virtual void enqueue(const T& t) = 0;
	virtual void enqueue(T&& t) = 0;
	virtual bool dequeue(const std::function<void(T&&)>& visit) = 0;
};

#endif /* FIFO_H_ */


