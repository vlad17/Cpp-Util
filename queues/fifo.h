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
	virtual ~queue() {}
	virtual void enqueue(T&& t) = 0;
	virtual bool dequeue(T& td) = 0; // moves popped to td, return true if moved, false if empty
};

#endif /* FIFO_H_ */


