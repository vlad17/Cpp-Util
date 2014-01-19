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

#define INLINE inline __attribute__ ((always_inline))

#ifndef NDEBUG
#define BPTR_CHECK 1
#endif

#include <queue>
#include <vector>
#include <cassert>


// defines default not equal operator for two types x and y, for a template
// type T
#define _DEFAULT_NEQ_OPERATOR_(x, y) \
	template<typename T> INLINE bool operator!=(x xarg, y yarg) \
	{return !(xarg == yarg);}

// defines default equal operator for two types x and y, for a template
// type T, assuming == is defined for y as lhs and x as rhs.
#define _SWAP_EQ_OPERATOR_(x, y) \
	template<typename T> INLINE bool operator==(x xarg, y yarg) \
	{return yarg == xarg;}

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

	template<typename T>
	class block_ptr;

	/**
	 * weak_block_ptr refers to an index which is associated with an allocated object
	 * in an _allocator.
	 *
	 * The _allocator refers to a fixed allocator maintains a contiguous memory
	 * chunk, so cache misses are minimized by dereferencing the pointer.
	 *
	 * The templated type T needs to be move constructible.
	 *
	 * The weak_block_pointer does not own the refered index.
	 */
	template<typename T>
	class weak_block_ptr
	{
	public:
		typedef index_t _allocator;
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
			INLINE void construct(Args&&... args);
			INLINE void destruct();
			// Get pointer to location of stored optional object.
			// Value of T may be undefined if not valid.
			INLINE T *get();
			INLINE const T *get() const;
			// Getter for initialized
			INLINE bool valid() const;
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
			fixed_allocator():
				store(), freelist() {};
			// Construct, destruct, access object at index
			// Data races: if construction triggers resize (store.size() == store.capacity()),
			// then during resize no dereferencing allowed and no destruction is allowed (at all).
			// Otherwise construction may procede, but at any given time a certain object can
			// only be dereferenced or destructed.
			template<typename... Args>
			index_t construct(Args&&... args);
			INLINE void destruct(index_t i);
			INLINE T& operator[](index_t i);
			INLINE const T& operator[](index_t i) const;
			fixed_allocator(const fixed_allocator&) = delete;
			fixed_allocator(fixed_allocator&&) = default;
			virtual ~fixed_allocator() {}
		};
		// If index is valid then allocator must be valid
		_allocator allocator; // first bit for parallel.
		index_t index;
		static const index_t NULLVAL = -1;
		static std::vector<fixed_allocator> fallocators;
		fixed_allocator& alloc() const {return fallocators[allocator];}
		// Object-generating Constructors
		template<typename... Args>
		weak_block_ptr(_allocator alloci, Args&&... args) :
			allocator(alloci), index(alloc().construct(std::forward<Args>(args)...)) {}
		// Access to default allocators
		static _allocator default_allocator();
		// Ability to destruct for derived classes
		INLINE void safe_destruct() {if(*this != nullptr) alloc().destruct(index);}
		INLINE void set_null() {index = NULLVAL;}
		INLINE void safe_destruct_set_null()
		{if(*this == nullptr) return; alloc().destruct(index); index = NULLVAL;}
		friend class block_ptr<T>;
	public:
		// Typeweak_block_ptrdefs
		typedef T type;
		typedef T& reference;
		typedef typename easily_copyable<T>::type const_reference;
		typedef T* pointer;
		typedef const T* const_pointer;
		// Construction
		weak_block_ptr(const weak_block_ptr& other) :
			allocator(other.allocator), index(other.index) {}
		weak_block_ptr(const block_ptr<T>& other) :
			allocator(other.weak().allocator), index(other.weak().index) {}
		weak_block_ptr() :
			allocator(NULLVAL), index(NULLVAL) {}
		weak_block_ptr(std::nullptr_t nptr) :
			allocator(NULLVAL), index(NULLVAL) {}
		// Assignment
		INLINE weak_block_ptr& operator=(std::nullptr_t nptr)
		{index = NULLVAL; return *this;}
		INLINE weak_block_ptr<T>& operator=(weak_block_ptr<T> other)
		{allocator = other.allocator; index = other.index; return *this;}
		INLINE weak_block_ptr<T>& operator=(const block_ptr<T>& other)
		{return *this = other.weak();}
		// Equality
		INLINE bool operator==(std::nullptr_t nptr) {return index == NULLVAL;}
		INLINE bool operator==(weak_block_ptr other)
		{
			if(index != other.index) return false;
			return allocator == other.allocator || index == NULLVAL;
		}
		INLINE bool operator==(const block_ptr<T>& other)
		{return *this == other.weak();}
		// Observers - refrences, pointers only valid during call
		INLINE reference operator*() const {assert(*this != nullptr); return alloc()[index];}
		INLINE const_reference get_cref() const {assert(*this != nullptr); return alloc()[index];}
		INLINE pointer operator->() const {assert(*this != nullptr); return &**this;}
		INLINE const_pointer get_cptr() const {assert(*this != nullptr); return &**this;}
		// Static factories
		static _allocator generate_allocator();
		//static parallel_allocator generate_parallel_allocator();
		// Destructor
		virtual ~weak_block_ptr() {}
	};

	_DEFAULT_NEQ_OPERATOR_(weak_block_ptr<T>, std::nullptr_t)
	_DEFAULT_NEQ_OPERATOR_(std::nullptr_t, weak_block_ptr<T>)
	_DEFAULT_NEQ_OPERATOR_(weak_block_ptr<T>, weak_block_ptr<T>)
	_SWAP_EQ_OPERATOR_(std::nullptr_t, weak_block_ptr<T>)
	_SWAP_EQ_OPERATOR_(const block_ptr<T>, weak_block_ptr<T>)

	// data races: no dereferencing allowed during vector expansion
	// in construction.

	/**
	 * Use block_ptr just like a regular pointer. A block pointer owns the object
	 * it points to, and has move semantics designed in such a way that it behaves
	 * like a unique_ptr.
	 */
	template<typename T>
	class block_ptr
	{
	private:
		weak_block_ptr<T> wptr;
	public:
		// Public Typedefs
		typedef typename weak_block_ptr<T>::_allocator _allocator;
		typedef typename weak_block_ptr<T>::type type;
		typedef typename weak_block_ptr<T>::reference reference;
		typedef typename weak_block_ptr<T>::const_reference const_reference;
		typedef typename weak_block_ptr<T>::pointer pointer;
		typedef typename weak_block_ptr<T>::const_pointer const_pointer;
		// Object-creating Constructors
		// Generates with supplied allocator
		template<typename... Args>
		block_ptr(_allocator alloc, Args&&... args) :
			wptr(alloc, std::forward<Args>(args)...) {}
		// Generates with default allocator
		template<typename... Args>
		INLINE static block_ptr<T> dfl_alloc(Args&&... args)
		{return block_ptr<T>(weak_block_ptr<T>::default_allocator(), std::forward<Args>(args)...);}
		// Movement constructors
		block_ptr() :
			wptr() {}
		block_ptr(std::nullptr_t nptr) :
			wptr() {}
		// No copy
		block_ptr(const block_ptr&) = delete;
		// Movement sets moved pointer to null.
		block_ptr(block_ptr&& bp) noexcept :
			wptr(bp) {bp.wptr.set_null();}
		// Assignment operators
		INLINE block_ptr<T>& operator=(const block_ptr<T>&) = delete;
		INLINE block_ptr<T>& operator=(block_ptr&& other)
		{wptr = other.weak(); other.wptr.set_null(); return *this;}
		INLINE block_ptr<T>& operator=(std::nullptr_t other)
		{wptr.safe_destruct_set_null(); return *this;}
		// Equality
		INLINE bool operator==(std::nullptr_t nptr) {return wptr == nptr;}
		INLINE bool operator==(const block_ptr<T>& other) {return wptr == other.wptr;}
		// Observers
		INLINE weak_block_ptr<T> weak() const {return wptr;}
		INLINE reference operator*() const {return *wptr;}
		INLINE const_reference get_cref() const {return *wptr;}
		INLINE pointer operator->() const {return &**this;}
		INLINE const_pointer get_cptr() const {return &**this;}
		// Static factories
		static _allocator generate_allocator() {return weak_block_ptr<T>::generate_allocator();}
		//static parallel_allocator generate_parallel_allocator();
		// Destructor
		virtual ~block_ptr() {wptr.safe_destruct();}
	};

	_DEFAULT_NEQ_OPERATOR_(std::nullptr_t, const block_ptr<T>&)
	_DEFAULT_NEQ_OPERATOR_(const block_ptr<T>&, const block_ptr<T>&)
}

template<typename T>
template<typename... Args>
mempool::weak_block_ptr<T>::optional::optional(Args&&... args) :
	initialized(false)
{
	this->construct(std::forward<Args>(args)...);
}

template<typename T>
mempool::weak_block_ptr<T>::optional::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto mempool::weak_block_ptr<T>::optional::operator=(optional&& other) -> optional&
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
void mempool::weak_block_ptr<T>::optional::construct(Args&&... args)
{
	assert(!initialized);
	new (get()) T(std::forward<Args>(args)...);
	initialized = true;
}

template<typename T>
void mempool::weak_block_ptr<T>::optional::destruct()
{
	assert(initialized);
	(*get()).~T();
	initialized = false;
}

// need not be initialized
template<typename T>
T *mempool::weak_block_ptr<T>::optional::get()
{
	return static_cast<T*>(static_cast<void*>(store));
}

// need not be initialized
template<typename T>
const T *mempool::weak_block_ptr<T>::optional::get() const
{
	return static_cast<const T*>(static_cast<const void*>(store));
}

template<typename T>
bool mempool::weak_block_ptr<T>::optional::valid() const
{
	return initialized;
}

template<typename T>
mempool::weak_block_ptr<T>::optional::~optional()
{
	if(initialized)
		get()->~T();
}

template<typename T>
template<typename... Args>
auto mempool::weak_block_ptr<T>::fixed_allocator::construct(Args&&... args) -> index_t
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
void mempool::weak_block_ptr<T>::fixed_allocator::destruct(index_t i)
{
	store[i].destruct();
	freelist.push(i);
}

template<typename T>
T& mempool::weak_block_ptr<T>::fixed_allocator::operator[](index_t i)
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
const T& mempool::weak_block_ptr<T>::fixed_allocator::operator[](index_t i) const
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
std::vector<typename mempool::weak_block_ptr<T>::fixed_allocator>
mempool::weak_block_ptr<T>::fallocators {};

template<typename T>
auto mempool::weak_block_ptr<T>::generate_allocator() -> _allocator
{
	fallocators.emplace_back();
	return fallocators.size()-1;
}

template<typename T>
auto mempool::weak_block_ptr<T>::default_allocator() -> _allocator
{
	if(fallocators.empty()) generate_allocator();
	return 0;
}

#endif /* BLOCK_PTR_H_ */
