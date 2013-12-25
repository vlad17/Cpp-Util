/*
* Vladimir Feinberg
* block_ptr.h
* 2013-12-24
*
* Defines block_ptr class, which allows for faster use of memory of
* fixed-size blocks by maintaining a contiguous block of memory.
*
* Also defines
*/

#ifndef BLOCK_PTR_H_
#define BLOCK_PTR_H_

#include <queue>
#include <vector>
#include <cassert>

// TODO consistency ifndef NDEBUG...

// TODO add parallel_fixed_allocator
// (uses R/W lock, writing for construction if need to reallocate, read otherwise).
// pfa should inherit privately fixed allocator (need access to vector member,
// but should not allow client calls on non-parallel.

namespace mempool
{
	// Indexing type
	typedef unsigned index_t;

	/**
	 * block_ptr maintains a contiguous memory chunk referenced by
	 * fixed_allocator. In terms of ownership, it is basically a unique pointer.
	 */
	template<typename T>
	class block_ptr
	{
	private:
		class fixed_allocator
		{
		private:
			static const size_t DEF_PAGE = 4096;
			std::vector<T> store;
			std::queue<index_t> freelist;
			static fixed_allocator DEFAULT;
			fixed_allocator():
				store(DEF_PAGE/sizeof(T)), freelist() {};
			// TODO finish methods
			template<typename... Args>
			index_t construct(Args&&... args);
			void destruct(index_t i);
			T& operator[](index_t i);
			const T& operator[](index_t i) const;
			friend class block_ptr<T>;
		public:
			fixed_allocator(const fixed_allocator&) = delete;
			fixed_allocator(fixed_allocator&&) = default;
		};
		// TODO add parallel allocator class here
		fixed_allocator& allocator; // TODO just use default	?
		index_t index;
		const index_t NULLVAL = -1;
	private:
		block_ptr(fixed_allocator& alloc, index_t index) :
			allocator(alloc), index(index) {}
	public:
		typedef fixed_allocator basic_allocator;
		//typedef parallel_fixed_allocator parallel_allocator;
		template<typename... Args>
		static block_ptr create(Args&&... args);
		template<typename... Args>
		static block_ptr create(basic_allocator& alloc, Args&&... args);
		//template<typename... Args>
		//static block_ptr create_parallel(parallel_allocator& alloc, Args&&... args);
		//template<typename... Args>
		//static block_ptr create_parallel(parallel_allocator& alloc, Args&&... args);
		basic_allocator generate_allocator();
		// parallel_allocator generate_parallel_allocator();
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

	// data races: no dereferencing allowed during vector expansion
	// in construction.


}

// TODO make explicit constructor/destructor calls via pointer.

// data races: no dereferencing allowed during construction
// may invalidate parameter pointers, not indices.
template<typename T>
template<typename... Args>
auto mempool::block_ptr<T>::fixed_allocator::construct(Args&&... args) -> index_t
{
	index_t i;
	if(freelist.empty())
	{
		i = store.size();
		store.emplace_back(std::forward<Args>(args)...);
		return i;
	}
	i = freelist.front();
	new (&store[i]) T(std::forward<Args>(args)...);
	freelist.pop();
	return i;
}


template<typename T>
typename mempool::block_ptr<T>::fixed_allocator
mempool::block_ptr<T>::fixed_allocator::fixed_allocator::DEFAULT {};

template<typename T>
void mempool::block_ptr<T>::fixed_allocator::destruct(index_t i)
{
	store[i].~T();
	freelist.push(i);
}

// data races: no dereferencing allowed during construction
template<typename T>
T& mempool::block_ptr<T>::fixed_allocator::operator[](index_t i)
{
	return store[i];
}
template<typename T>
const T& mempool::block_ptr<T>::fixed_allocator::operator[](index_t i) const
{
	return store[i];
}

template<typename T>
template<typename... Args>
auto mempool::block_ptr<T>::create(Args&&... args) -> block_ptr<T>
{
	return create(fixed_allocator::DEFAULT, std::forward<Args>(args)...);
}

template<typename T>
template<typename... Args>
auto mempool::block_ptr<T>::create(basic_allocator& alloc,
		Args&&... args) -> block_ptr<T>
{
	return block_ptr<T>(alloc, alloc.construct(std::forward<Args>(args)...));
}

template<typename T>
auto mempool::block_ptr<T>::generate_allocator() -> basic_allocator
{
	return basic_allocator();
}

#endif /* BLOCK_PTR_H_ */
