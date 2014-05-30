/*
 * Vladimir Feinberg
 * fifo.h
 *
 * Contains FIFO queue interface. The queue provides "sequence number"
 * information that lets the threads know the relative order of the queue.
 */

#ifndef FIFO_H_
#define FIFO_H_

template<typename T>
class queue
{
protected:
	queue() {}
public:
	typedef unsigned long size_type;
	virtual ~queue() {}
	virtual size_type enqueue(T&& t) = 0;
	virtual T dequeue() = 0;
};

#endif /* FIFO_H_ */


