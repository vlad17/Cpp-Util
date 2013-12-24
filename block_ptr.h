/*
* Vladimir Feinberg
* block_ptr.h
* 2013-12-24
*
* Declares block_ptr class, which allows for faster use of memory of
* fixed-size blocks by maintaining a contiguous block of memory.
*/

#ifndef BLOCK_PTR_H_
#define BLOCK_PTR_H_

#include <queue>
#include <vector>
#include <cassert>
#include <memory>

// Neat little trick I found on Stack Overflow:
// http://stackoverflow.com/questions/3058267/nested-class-member-function-cant-access-function-of-enclosing-class-why
// Allows
#define _BPTR_OUTERCLASS(className, memberName) \
    reinterpret_cast<className*>(reinterpret_cast<char*>(this)- \
		offsetof(className, memberName))


// TODO make fixed_allocator conform to specification of normal allocator

// TODO add parallel_fixed_allocator
// (uses R/W lock, writing for construction if need to reallocate, read otherwise).
// pfa should inherit privately fixed allocator (need access to vector member,
// but should not allow client calls on non-parallel.

namespace mempool
{
	// Indexing type (may be set larger if more objects expected).
	typedef unsigned index_type;

	// fixed_allocator for each type T
	template<typename T>
	class fixed_allocator
	{
	private:
		static const size_t DEF_PAGE = 4096;
		typedef index_type index_t;
		std::vector<T> store;
		std::queue<index_t> freelist;
		T& operator[](index_t i);
		const T& operator[](index_t i) const;
		/**
		 * block_ptr maintains a contiguous memory chunk referenced by
		 * fixed_allocator. In terms of ownership, it is basically a unique pointer.
		 */
		class block_ptr
		{
		private:
			typedef typename fixed_allocator<T>::index_t index_t;
			fixed_allocator<T>& allocator; // TODO just use default?
			index_t index;
			const index_t NULLVAL = -1;
		public:
			block_ptr(fixed_allocator<T>& alloc, index_t index) :
				allocator(alloc), index(index) {}
			block_ptr(const block_ptr&) = delete;
			block_ptr(block_ptr&& bp) :
				allocator(bp.allocator), index(std::move(bp.index))
			{ bp.index = NULLVAL;}
			// refrences, pointers only valid during call
			T& operator*() { assert(index != NULLVAL); return allocator[index];}
			const T& operator*() const { assert(index != NULLVAL); return allocator[index];}
			T *operator->() { assert(index != NULLVAL); return &**this;}
			const T *operator->() const { assert(index != NULLVAL); return &**this;}
			~block_ptr() { if(index != NULLVAL) allocator.destruct(index);}
		};
		friend class block_ptr;
	public:
		typedef T value_type;
		typedef block_ptr pointer;
		typedef T& reference;
		typedef const block_ptr const_pointer;
		typedef const T& const_reference;
		typedef index_t size_type;
		typedef long ptrdiff_t;
		// All propogation types are false_type by default in allocator_traits
		static fixed_allocator<T> DEFAULT;
		fixed_allocator():
			store(DEF_PAGE/sizeof(T)), freelist() {};
		// TODO finish methods
		template<typename... Args>
		index_t construct(Args&&... args);
		void destruct(index_t i);
	};
	// data races: no dereferencing allowed during vector expansion
	// in construction.


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
