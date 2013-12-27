/*
* Vladimir Feinberg
* block_ptr.h
* 2013-12-26
*
* Defines block_ptr class, which allows for faster use of memory of
* fixed-size blocks by maintaining a contiguous block of memory.
*
* Also defines weak_block_ptr, which allows for dereferencing an object
* pointed to by a block_ptr but does not claim ownership of it, unlike
* the block_ptr.
*/

#ifndef BLOCK_PTR_H_
#define BLOCK_PTR_H_

#ifndef NDEBUG
#define BPTR_CHECK 1
#endif

#include <queue>
#include <vector>
#include <cassert>

// TODO consistency ifndef NDEBUG...

// TODO create an index of allocators (?)

// TODO add parallel_fixed_allocator
// (uses R/W lock, writing for construction if need to reallocate, read otherwise).
// pfa should inherit privately fixed allocator (need access to vector member,
// but should not allow client calls on non-parallel.

namespace mempool
{
	/**
	 * Indexing type index_t is an unsigned type which is used throughout
	 * the implementation.
	 *
	 * std::numeric_limits<index_t>::max() gives the maximum number of objects
	 * that theoretically can be indexed by a block_ptr at a given instance
	 * in time.
	 */
	typedef unsigned index_t;

	/**
	 * block_ptr refers to an index which is associated with an allocated object
	 * in a fixed_allocator. The fixed allocator maintains a vector that takes up
	 * about one PAGE_SIZE bytes initially.
	 *
	 * Because the fixed allocator contains a contiguous memory chunk, cache misses
	 * are minimized by dereferencing the pointer.
	 *
	 * Use block_ptr just like a regular pointer. A block pointer owns the object
	 * it points to, and has move semantics designed in such a way that it behaves
	 * like a unique_ptr.
	 *
	 * The templated type T needs to be move constructible.
	 */
	template<typename T>
	class block_ptr
	{
	private:
		/**
		 * optional class is inspired by boost::optional<T>, but is simpler and
		 * does not use factories (which copy arguments for construction).
		 *
		 * Allows for explicit construction and destruction of objects.
		 */
		class optional
		{
		private:
			bool initialized; // boolean tracks current state (necessary for moving)
			char store[sizeof(T)]; // store object as a sequence of bytes explicitly
		public:
			// Constructors
			template<typename... Args>
			optional(Args&&... args);
			optional(optional&& other);
			optional(const optional&) = delete;
			// Assignment operators
			optional& operator=(const optional&) = delete;
			optional& operator=(optional&& other);
			// Explicit construction, destruction
			template<typename... Args>
			void construct(Args&&... args);
			void destruct();
			// Get pointer to location of stored optional object.
			// Value of T may be undefined if not valid.
			T *get();
			const T *get() const;
			// Getter for initialized
			bool valid() const;
			~optional();
		};
		/**
		 * fixed_allocator maintains a contiguous chunk of memory
		 * and allows for construction of new objects of the same fixed
		 * size within it, as well as on-call destruction and in-place
		 * reconstruction.
		 */
		class fixed_allocator
		{
		protected:
			std::vector<optional> store; // chunk of memory
			std::queue<index_t> freelist; // list of free entries
			static fixed_allocator DEFAULT; // default allocator of this type
			// Default constructor makes empty everything.
			fixed_allocator():
				store(), freelist() {};
			// Construct, destruct, access object at index
			// Data races: if construction triggers resize (store.size() == store.capacity()),
			// then during resize no dereferencing allowed and no destruction is allowed (at all).
			// Otherwise construction may procede, but at any given time a certain object can
			// only be dereferenced or destructed.
			template<typename... Args>
			index_t construct(Args&&... args);
			void destruct(index_t i);
			T& operator[](index_t i);
			const T& operator[](index_t i) const;
			friend class block_ptr<T>; // let block_ptr use internals
		public:
			// User can only move the allocator
			fixed_allocator(const fixed_allocator&) = delete;
			fixed_allocator(fixed_allocator&&) = default;
		};
		// TODO add parallel allocator class here
		fixed_allocator& allocator; // TODO just use default	?
		index_t index;
		const index_t NULLVAL = -1;
		block_ptr(fixed_allocator& alloc, index_t index) :
			allocator(alloc), index(index) {}
		// template magic for easily copyable types (
		template<bool B, class E>
		struct easily_copyable { typedef const E& type; };
		template<class E>
		struct easily_copyable<true, E> { typedef E type; };
		template<class E>
		using cref = easily_copyable<std::is_integral<E>::value, E>;
	public:
		typedef T type;
		typedef T& reference;
		typedef typename cref<T>::type const_reference;
		typedef T* pointer;
		typedef const T* const_pointer;
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
		static basic_allocator generate_allocator();
		// parallel_allocator generate_parallel_allocator();
		block_ptr(const block_ptr&) = delete;
		block_ptr(block_ptr&& bp) :
			allocator(bp.allocator), index(std::move(bp.index))
		{ bp.index = NULLVAL;}
		// refrences, pointers only valid during call
		bool valid() const;
		reference operator*();
		const_reference operator*() const;
		pointer operator->();
		const_pointer operator->() const;
		~block_ptr();
	};

	// data races: no dereferencing allowed during vector expansion
	// in construction.

	// no ownership
	template<typename T>
	class weak_block_ptr
	{
	private:
		block_ptr<T>& bptr;
	public:
		typedef typename block_ptr<T>::type type;
		typedef typename block_ptr<T>::reference reference;
		typedef typename block_ptr<T>::const_reference const_reference;
		typedef typename block_ptr<T>::pointer pointer;
		typedef typename block_ptr<T>::const_pointer const_pointer;
		weak_block_ptr(block_ptr<T>& bptr) :
			bptr(bptr) {}
		weak_block_ptr(const weak_block_ptr& other) :
			bptr(other.bptr) {}
		reference operator*() {return *bptr;}
		const_reference operator*() const {return *bptr;}
		pointer operator->() {return bptr.operator->();}
		const_pointer operator->() const {return bptr.operator->();}
	};

}
template<typename T>
template<typename... Args>
mempool::block_ptr<T>::optional::optional(Args&&... args) :
	initialized(false)
{
	this->construct(std::forward<Args>(args)...);
}

template<typename T>
mempool::block_ptr<T>::optional::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto mempool::block_ptr<T>::optional::operator=(optional&& other) -> optional&
{
	if(other.initialized)
	{
		if(initialized) destruct();
		new (get()) T(std::move(*other.get()));
		initialized = true;
	}
	return *this;
}

template<typename T>
template<typename... Args>
void mempool::block_ptr<T>::optional::construct(Args&&... args)
{
	assert(!initialized);
	new (get()) T(std::forward<Args>(args)...);
	initialized = true;
}

template<typename T>
void mempool::block_ptr<T>::optional::destruct()
{
	assert(initialized);
	(*get()).~T();
	initialized = false;
}

// need not be initialized
template<typename T>
T *mempool::block_ptr<T>::optional::get()
{
	return static_cast<T*>(static_cast<void*>(store));
}

// need not be initialized
template<typename T>
const T *mempool::block_ptr<T>::optional::get() const
{
	return static_cast<const T*>(static_cast<const void*>(store));
}

template<typename T>
mempool::block_ptr<T>::optional::~optional()
{
	if(initialized)
		get()->~T();
}

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
	store[i].construct(std::forward<Args>(args)...);
	freelist.pop();
	return i;
}


template<typename T>
typename mempool::block_ptr<T>::fixed_allocator
mempool::block_ptr<T>::fixed_allocator::fixed_allocator::DEFAULT {};

template<typename T>
void mempool::block_ptr<T>::fixed_allocator::destruct(index_t i)
{
	store[i].destruct();
	freelist.push(i);
}

template<typename T>
T& mempool::block_ptr<T>::fixed_allocator::operator[](index_t i)
{
	assert(store[i].valid());
	return *store[i].get();
}
template<typename T>
const T& mempool::block_ptr<T>::fixed_allocator::operator[](index_t i) const
{
	assert(store[i].valid());
	return *store[i].get();
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

template<typename T>
bool mempool::block_ptr<T>::valid() const
{
	return index != NULLVAL;
}

template<typename T>
auto mempool::block_ptr<T>::operator*() -> reference
{
	assert(valid());
	return allocator[index];
}

template<typename T>
auto mempool::block_ptr<T>::operator*() const -> const_reference
{
	assert(valid());
	return allocator[index];
}

template<typename T>
auto mempool::block_ptr<T>::operator->() -> pointer
{
	assert(valid());
	return &**this;
}

template<typename T>
auto mempool::block_ptr<T>::operator->() const -> const_pointer
{
	assert(valid());
	return &**this;
}

template<typename T>
mempool::block_ptr<T>::~block_ptr()
{
	if(valid())
		allocator.destruct(index);
}

#endif /* BLOCK_PTR_H_ */
