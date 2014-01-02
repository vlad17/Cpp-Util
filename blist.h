/*
* Vladimir Feinberg
* blist.h
* 2013-12-28
*
* Defines blist class, which uses the block_ptr in order to be a
* more cache-friendly linked list.
*/

// TODO cref everywhere

#ifndef BLIST_H_
#define BLIST_H_

#include "block_ptr.h"

// TODO move, copy, assign, fill inputiterator

template<typename T>
class blist
{
	class node;
	typedef mempool::block_ptr<node> unique_node_ptr;
	typedef mempool::weak_block_ptr<node> weak_node_ptr;
	typedef decltype(unique_node_ptr::generate_allocator()) node_alloc;
	class node // owns everything past it.
	{
	public:
		unique_node_ptr next;
		weak_node_ptr prev;
		T val;
		template<typename... Args>
		node(Args&&... args) :
			next(),	prev(), val(std::forward<Args>(args)...) {}
		node(node&& other) :
			next(std::move(other.next)), prev(other.prev),
			val(std::move(other.val)) {}
	};
	node_alloc alloc;
	unique_node_ptr head;
	weak_node_ptr tail;
	size_t _size;
	class _iterator
	{
	private:
		weak_node_ptr ptr;
		_iterator() :
			ptr() {}
		_iterator(weak_node_ptr ptr) :
			ptr(ptr) {}
		friend class blist<T>;
	public:
		_iterator(const _iterator& other) :
			ptr(other.ptr) {}
		_iterator& operator++() {ptr = ptr->next; return *this;}
		bool operator==(_iterator other) const {return ptr == other.ptr;}
		bool operator!=(_iterator other) const {return ptr != other.ptr;}
		T& operator*() const {return ptr->val;}
		T *operator->() const {return &ptr->val;}
	};
public:
	typedef _iterator iterator;
	blist() :
		alloc(unique_node_ptr::generate_allocator()),
		head(), tail(), _size(0) {}
	size_t size() const {return _size;}
	bool empty() const {return _size == 0;}
	template<typename... Args>
	void emplace_front(Args&&... args);
	template<typename... Args>
	void emplace_back(Args&&... args);
	void pop_front();
	void pop_back();
	void clear();
	iterator begin() {return iterator(head);}
	iterator end() {return iterator();}
};

// TODO const_iterator
template<typename T>
std::ostream& operator<<(std::ostream& o, blist<T>& list)
{
	o << "list@" << (size_t) &list << "\n[";
	for(auto& i : list)
		o << i << ", ";
	o << "]\n";
	return o;
}

template<typename T>
template<typename... Args>
void blist<T>::emplace_front(Args&&... args)
{
	unique_node_ptr newnode =
			unique_node_ptr::create_alloc(alloc, std::forward<Args>(args)...);
	if(empty())
		tail = head = std::move(newnode);
	else
	{
		head->prev = newnode;
		newnode->next = std::move(head);
		head = std::move(newnode);
	}
	++_size;
}

template<typename T>
template<typename... Args>
void blist<T>::emplace_back(Args&&... args)
{
	unique_node_ptr newnode =
			unique_node_ptr::create_alloc(alloc, std::forward<Args>(args)...);
	if(empty())
		tail = head = std::move(newnode);
	else
	{
		newnode->prev = tail;
		tail->next = std::move(newnode);
		tail = tail->next;
	}
	++_size;
}

template<typename T>
void blist<T>::pop_front()
{
	assert(!empty());
	if(head->next == nullptr)
		tail = head = nullptr;
	else
	{
		unique_node_ptr newhead = std::move(head->next);
		newhead->prev = nullptr;
		head = std::move(newhead);
	}
	--_size;
}

template<typename T>
void blist<T>::pop_back()
{
	assert(!empty());
	if(tail->prev == nullptr)
		tail = head = nullptr;
	else
	{
		weak_node_ptr newtail = tail->prev;
		newtail->next = nullptr;
		tail = newtail;
	}
	--_size;
}

template<typename T>
void blist<T>::clear()
{
	tail = head = nullptr;
	_size = 0;
}

#endif /* BLIST_H_ */
