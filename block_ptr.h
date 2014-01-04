/*
* Vladimir Feinberg
* block_ptr.h
* 2013-12-28
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

// TODO inline everything.

// defines default not equal operator for two types x and y, for a template
// type T
#define _DEFAULT_NEQ_OPERATOR_(x, y) \
	template<typename T> bool operator!=(x xarg, y yarg) \
	{return !(xarg == yarg);}

// defines default equal operator for two types x and y, for a template
// type T, assuming == is defined for y as lhs and x as rhs.
#define _SWAP_EQ_OPERATOR_(x, y) \
	template<typename T> bool operator==(x xarg, y yarg) \
	{return yarg == xarg;}

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
	 * in time for a given allocator.
	 */
	typedef unsigned index_t;

	/*
	 * easily_copyable class has a public type member type which is
	 * const T& if T is not integral Cond and T if it is.
	 */
	template<typename T>
	class easily_copyable
	{
	private:
		template<bool B, class E>
		struct aux { typedef const E& type; };
		template<class E>
		struct aux<true, E> { typedef E type; };
	public:
		typedef typename aux<std::is_integral<T>::value, T>::type type;
	};

	/**
	 * base_ptr refers to an index which is associated with an allocated object
	 * in an _allocator.
	 *
	 * The _allocator refers to a fixed allocator maintains a contiguous memory
	 * chunk, so cache misses are minimized by dereferencing the pointer.
	 *
	 * The templated type T needs to be move constructible.
	 */
	template<typename T>
	class base_ptr
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
		private:
			std::vector<optional> store; // chunk of memory
			std::queue<index_t> freelist; // list of free entries
			// Default constructor makes empty everything.
		public:
			static fixed_allocator DEFAULT; // default allocator of this type
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
			fixed_allocator(const fixed_allocator&) = delete;
			fixed_allocator(fixed_allocator&&) = default;
			virtual ~fixed_allocator() {}
		};
		// TODO parallel allocator
		// no ownership of allocator, always a valid one if index is valid
		fixed_allocator *allocator;
		// TODO just use index?
		// TODO just use default?
		index_t index;
		static const index_t NULLVAL = -1;
	protected:
		// Constructors
		template<typename... Args>
		base_ptr(fixed_allocator &alloc, Args&&... args) :
			allocator(&alloc), index(alloc.construct(std::forward<Args>(args)...)) {}
		base_ptr() :
			allocator(nullptr), index(NULLVAL) {}
		// Access to default allocators
		static fixed_allocator& default_allocator() {return fixed_allocator::DEFAULT;}
		// parallel TODO
		// Ability to destruct for derived classes
		void safe_destruct()
		{if(*this != nullptr) allocator->destruct(index);}
		void set_null()
		{index = NULLVAL;}
		void safe_destruct_set_null()
		{if(*this == nullptr) return; allocator->destruct(index); index = NULLVAL;}
		base_ptr& operator=(base_ptr other)
		{allocator = other.allocator; index = other.index; return *this;}
	public:
		// Typebase_ptrdefs
		typedef T type;
		typedef T& reference;
		typedef typename easily_copyable<T>::type const_reference;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef fixed_allocator _allocator;
		// Assignment & Copying (just trivially copies)
		base_ptr(const base_ptr& other) :
			allocator(other.allocator), index(other.index) {}
		base_ptr& operator=(std::nullptr_t nptr) { index = NULLVAL; return *this;}
		// Equality
		bool operator==(std::nullptr_t nptr) {return index == NULLVAL;}
		bool operator==(base_ptr other)
		{
			if(index != other.index) return false;
			return allocator == other.allocator || index == NULLVAL;
		}
		// Observers - refrences, pointers only valid during call
		reference operator*() const {assert(*this != nullptr); return (*allocator)[index];}
		const_reference get_cref() const {assert(*this != nullptr); return (*allocator)[index];}
		pointer operator->() const {assert(*this != nullptr); return &**this;}
		const_pointer get_cptr() const {assert(*this != nullptr); return &**this;}
		// Static factories
		static _allocator generate_allocator();
		//static parallel_allocator generate_parallel_allocator();
		// Destructor
		virtual ~base_ptr() {}
	};

	_DEFAULT_NEQ_OPERATOR_(base_ptr<T>, std::nullptr_t)
	_DEFAULT_NEQ_OPERATOR_(std::nullptr_t, base_ptr<T>)
	_DEFAULT_NEQ_OPERATOR_(base_ptr<T>, base_ptr<T>)
	_SWAP_EQ_OPERATOR_(std::nullptr_t, base_ptr<T>)

	// data races: no dereferencing allowed during vector expansion
	// in construction.

	/**
	 * Use block_ptr just like a regular pointer. A block pointer owns the object
	 * it points to, and has move semantics designed in such a way that it behaves
	 * like a unique_ptr.
	 */
	template<typename T>
	class block_ptr : public base_ptr<T>
	{
	public:
		// Constructors
		template<typename... Args>
		block_ptr(typename block_ptr<T>::_allocator &alloc, Args&&... args) :
			base_ptr<T>(alloc, std::forward<Args>(args)...) {}
		template<typename... Args>
		block_ptr(Args&&... args) :
			base_ptr<T>(block_ptr<T>::default_allocator(), std::forward<Args>(args)...) {}
		block_ptr() :
			base_ptr<T>() {}
		block_ptr(std::nullptr_t nptr) :
			base_ptr<T>() {}
		// No copy
		block_ptr(const block_ptr&) = delete;
		// Movement sets moved pointer to null.
		block_ptr(block_ptr&& bp) noexcept :
				base_ptr<T>(bp) {bp.set_null();}
		// Assignment operators
		block_ptr<T>& operator=(const block_ptr<T>&) = delete;
		block_ptr<T>& operator=(block_ptr&& other)
		{this->base_ptr<T>::operator=(other); other.set_null(); return *this;}
		block_ptr<T>& operator=(std::nullptr_t other)
		{this->safe_destruct_set_null(); return *this;}
		virtual ~block_ptr() {this->safe_destruct();}
	};

	// no ownership
	template<typename T>
	class weak_block_ptr : public base_ptr<T>
	{
	public:
		weak_block_ptr() :
			base_ptr<T>() {}
		weak_block_ptr(std::nullptr_t nptr) :
			base_ptr<T>() {}
		weak_block_ptr(base_ptr<T> bptr) :
			base_ptr<T>(bptr) {}
		weak_block_ptr<T>& operator=(base_ptr<T> other)
		{this->base_ptr<T>::operator=(other); return *this;}
		virtual ~weak_block_ptr() {};
	};
}

template<typename T>
template<typename... Args>
mempool::base_ptr<T>::optional::optional(Args&&... args) :
	initialized(false)
{
	this->construct(std::forward<Args>(args)...);
}

template<typename T>
mempool::base_ptr<T>::optional::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto mempool::base_ptr<T>::optional::operator=(optional&& other) -> optional&
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
void mempool::base_ptr<T>::optional::construct(Args&&... args)
{
	assert(!initialized);
	new (get()) T(std::forward<Args>(args)...);
	initialized = true;
}

template<typename T>
void mempool::base_ptr<T>::optional::destruct()
{
	assert(initialized);
	(*get()).~T();
	initialized = false;
}

// need not be initialized
template<typename T>
T *mempool::base_ptr<T>::optional::get()
{
	return static_cast<T*>(static_cast<void*>(store));
}

// need not be initialized
template<typename T>
const T *mempool::base_ptr<T>::optional::get() const
{
	return static_cast<const T*>(static_cast<const void*>(store));
}

template<typename T>
bool mempool::base_ptr<T>::optional::valid() const
{
	return initialized;
}

template<typename T>
mempool::base_ptr<T>::optional::~optional()
{
	if(initialized)
		get()->~T();
}

template<typename T>
template<typename... Args>
auto mempool::base_ptr<T>::fixed_allocator::construct(Args&&... args) -> index_t
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
typename mempool::base_ptr<T>::fixed_allocator
mempool::base_ptr<T>::fixed_allocator::fixed_allocator::DEFAULT {};

template<typename T>
void mempool::base_ptr<T>::fixed_allocator::destruct(index_t i)
{
	store[i].destruct();
	freelist.push(i);
}

template<typename T>
T& mempool::base_ptr<T>::fixed_allocator::operator[](index_t i)
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
const T& mempool::base_ptr<T>::fixed_allocator::operator[](index_t i) const
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
auto mempool::base_ptr<T>::generate_allocator() -> _allocator
{
	return fixed_allocator();
}

#endif /* BLOCK_PTR_H_ */
