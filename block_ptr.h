/*
* Vladimir Feinberg
* cache.h
* 2013-12-18
*
* Declares block_ptr class, which allows for faster use of memory of
* fixed-size blocks by maintaining a contiguous block of memory.
*/

#ifndef BLOCK_PTR_H_
#define BLOCK_PTR_H_

#include <queue>
#include <vector>
#include <cassert>


// TODO make fixed_allocator conform to specification of normal allocator

// TODO add parallel_fixed_allocator
// (uses R/W lock, writing for construction if need to reallocate, read otherwise).

namespace mempool
{
	// Indexing type (may be set larger if more objects expected).
	typedef unsigned index_t;

	// fixed_allocator for each type T
	template<typename T>
	class fixed_allocator
	{
	private:
		typedef index_t index_type;
		std::vector<T> store;
		std::queue<index_type> freelist;
		static const size_t DEF_PAGE = 4096;
	public:
		static fixed_allocator<T> DEFAULT;
		fixed_allocator() :
			store(DEF_PAGE/sizeof(T)), freelist() {};
		// data races: no dereferencing allowed during construction
		template<typename... Args>
		index_type construct(Args&&... args);
		void destruct(index_type i);
		// data races: no dereferencing allowed during construction
		T& operator[](index_type i);
		const T& operator[](index_type i) const;
	};

	/**
	 * block_ptr maintains a contiguous memory chunk referenced by
	 * fixed_allocator. In terms of ownership, it is basically a unique pointer.
	 */
	template<typename T,
			fixed_allocator<T>& allocator = fixed_allocator<T>::DEFAULT>
	class block_ptr
	{
	private:
		typedef index_t index_type;
		index_type index;
		const index_type NULLVAL = -1;
	public:
		template<typename... Args>
		block_ptr(Args&&... args) :
			index(allocator.construct(std::forward<Args>(args)...)) {}
		block_ptr(const block_ptr&) = delete;
		block_ptr(block_ptr&& bp) :
			index(std::move(bp.index))
		{
			bp.index = NULLVAL;
		}
		// refrences, pointers only valid during call
		T& operator*()
		{ assert(index != NULLVAL); return allocator[index];}
		const T& operator*() const
		{ assert(index != NULLVAL); return allocator[index];}
		T *operator->()
		{ assert(index != NULLVAL); return &**this;}
		const T *operator->() const
		{ assert(index != NULLVAL); return &**this;}
		~block_ptr()
		{
			if(index != NULLVAL)
				allocator.destruct(index);
		}
	};
}

// data races: no dereferencing allowed during construction
// may invalidate parameterpointers, not indices.
template<typename T>
template<typename... Args>
auto mempool::fixed_allocator<T>::construct(Args&&... args) -> index_type
{
	index_type i;
	if(freelist.empty())
	{
		i = store.size();
		store.emplace_back(std::forward<Args>(args)...);
		return i;
	}
	i = freelist.front();
	store[i] = T(std::forward<Args>(args)...);
	freelist.pop();
	return i;
}

template<typename T>
mempool::fixed_allocator<T> mempool::fixed_allocator<T>::DEFAULT {};

template<typename T>
void mempool::fixed_allocator<T>::destruct(index_type i)
{
	freelist.push(i);
}

// data races: no dereferencing allowed during construction
template<typename T>
T& mempool::fixed_allocator<T>::operator[](index_type i)
{
	return store[i];
}
template<typename T>
const T& mempool::fixed_allocator<T>::operator[](index_type i) const
{
	return store[i];
}

#endif /* BLOCK_PTR_H_ */
