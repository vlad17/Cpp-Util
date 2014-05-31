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
#include <memory>

template<typename T>
class lfqueue : public queue<T>
{
private:
	struct node
	{
		node(const T& val) : next(nullptr), val(val) {}
		node(T&& val) : next(nullptr), val(val) {}
		std::atomic<node*> next;
		T val;
	};
	std::atomic<node*> head;
	std::atomic<node*> tail;
	void enqueue(node *n) noexcept;
public:
	lfqueue() : head(nullptr), tail(nullptr) {}
	virtual ~lfqueue() {}
	// Strong guarentee for enqueues only construction can throw
	virtual void enqueue(const T& t) {enqueue(new node(t));}
	virtual void enqueue(T&& t) {enqueue(new node(std::forward<T>(t)));}
	virtual bool dequeue(const std::function<void(T&&)>& visit); // strong guarentee
};

template<typename T>
void lfqueue<T>::enqueue(node *n) noexcept
{
	node *oldhead, *newnext;
	bool first;
	while(
			// Check if empty, get head value
			!(first = head.compare_exchange_weak(oldhead = nullptr, n)) &&
			// Check if oldhead is still head (next is 0)
			!oldhead->next.compare_exchange_weak(newnext = nullptr, n));
	// No other enqueue() thread will change head past this point
	if(first) // no CAS here since dequeue will happily pretend it's empty
		tail = head.load();
	else // CAS needed here b/c in the tail == head case dequeue may change head
		head.compare_exchange_strong(oldhead, oldhead->next.load());
		// Strong CAS to guarantee head is updated by now to new one
}

// Function call throws b/c of function
template<typename T>
bool lfqueue<T>::dequeue(const std::function<void(T&&)>& visit)
{
	node *oldtail = tail.load(), *cpy;
	do {if(oldtail == nullptr) return false;}
	while(!tail.compare_exchange_weak(oldtail, oldtail->next.load()));
	// tail == head case, strong CAS to ensure condition below
	head.compare_exchange_strong(cpy = oldtail, oldtail->next.load());
	// No other enqueue() or dequeue() thread will access oldtail past this point
	std::unique_ptr<node> _guard(oldtail); // needed for strong
	visit(std::move(oldtail->val));
	return true;
}


#endif /* LOCK_FREE_Q_H_ */
