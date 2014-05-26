/*
 * Vladimir Feinberg
 * util.h
 * 2014-05-26
 *
 * Header for basic utility classes. Partially influenced by:
 * http://programmers.stackexchange.com/questions/237339/is-iterable-like-behavior-in-c-attainable/237352#237352
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <cassert>

// INLINE will ensure a function is always inlined (gcc)
#define INLINE inline __attribute__ ((always_inline))

namespace util
{
	template<typename Iterator>
	class iterable;

	template<typename T>
	auto as_iterator(T& t) -> iterable<decltype(t.begin())>;

	template<typename Iterator>
	class iterable
	{
	private:
		Iterator _begin;
		Iterator _end;
		iterable(Iterator _begin, Iterator _end) :
			_begin(_begin), _end(_end) {}
		template<typename T>
		friend auto as_iterator(T& t) -> iterable<decltype(t.begin())>;
	public:
		Iterator begin() const {return _begin;}
		Iterator end() const {return _end;}
	};

	template<typename T>
	auto as_iterator(T& t) -> iterable<decltype(t.begin())>
	{
		return iterable(t.begin(), t.end());
	}

	// Pointer comparison
	template<typename T>
	struct ptrcmp
	{
		bool operator()(const T* t1, const T* t2) {return *t1 < *t2;}
	};

	/*
	 * scref gives a constant reference for non-scalar types, or the
	 * type itself otherwise. Allows for faster value semantics when it's worth it.
	 */
	template<typename T>
	class scref
	{
	private:
		template<bool B, class E>
		struct aux { typedef const E& type; };
		template<class E>
		struct aux<true, E> { typedef E type; };
	public:
		typedef typename aux<std::is_scalar<T>::value, T>::type type;
	};

	/**
	 * optional class is inspired by boost::optional<T>, but is simpler and
	 * does not use factories (which copy arguments for construction).
	 *
	 * Allows for explicit construction and destruction of objects.
	 *
	 * Keeps track of whether or not the object is initialized.
	 *
	 * Does not allow for copy capability b/c type may not have implemented copy.
	 * A workaround is creating a new optional instance with a copy being move-assigned.
	 */
	template<typename T>
	class optional
	{
	private:
		bool initialized; // boolean tracks current state (necessary for moving)
		char store[sizeof(T)]; // store object as a sequence of bytes explicitly
	public:
		// Constructors
		optional(); // does not default-initialize
		optional(T&& rval);
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
		INLINE T *get() const;
		INLINE T &access() const;
		// Getter for initialized
		INLINE bool valid() const;
		~optional();
	};
}

#include "impl_util.tpp"

#endif /* UTIL_H_ */
