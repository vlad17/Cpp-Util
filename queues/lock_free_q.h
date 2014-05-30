/*
 * Vladimir Feinberg
 * lock_free_q.h
 * 2014-05-30
 *
 * Lock free FIFO queue
 */

#ifndef LOCK_FREE_Q_H_
#define LOCK_FREE_Q_H_

#include "fifo.h"

#include <cassert>
#include <atomic>
#include <utility>

template<typename T>
class lfqueue : public queue<T>
{
private:
	struct node
	{
		node(T&& val) : next(nullptr), val(val) {}
		std::atomic<node*> next;
		T val;
	};
	std::atomic<node*> head;
	std::atomic<node*> tail;
public:
	lfqueue() : head(nullptr), tail(nullptr) {}
	virtual ~lfqueue() {}
	virtual void enqueue(T&& t); // strong guarentee
	virtual bool dequeue(T& td) noexcept;
};


template<typename T>
void lfqueue<T>::enqueue(T&& t)
{
	node *oldhead, *newnext;
	node *n = new node(std::forward<T>(t)); // no need for unique_ptr since only new throws here.
	bool first;
	while(
			// Check if empty, get head value
			!(first = head.compare_exchange_weak(oldhead = nullptr, n)) ||
			// Check if oldhead is still head (next is 0)
			!oldhead->next.compare_exchange_weak(newnext = nullptr, n));
	// No other enqueue() thread will change head past this point
	if(first) // no CAS here since dequeue will happily pretend it's empty
		tail = head.load();
	else // CAS needed here b/c in the tail == head case dequeue may change head
		head.compare_exchange_strong(oldhead, oldhead->next.load());
		// Strong CAS to guarantee head is updated by now to new one
}

template<typename T>
bool lfqueue<T>::dequeue(T& t) noexcept
{
	node *oldtail = tail.load();
	do {if(oldtail == nullptr) return false;}
	while(!tail.compare_exchange_weak(oldtail, oldtail->next.load()));
	// tail == head case, weak CAS (head will do the work if needed)
	head.compare_exchange_weak(oldtail, oldtail->next.load());
	// No other enqueue() or dequeue() thread will access oldtail past this point
	t = std::move_if_noexcept(oldtail->val);
	delete oldtail;
	return true;
}


#endif /* LOCK_FREE_Q_H_ */
