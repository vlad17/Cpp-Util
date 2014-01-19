/*
* Vladimir Feinberg
* impl_util.h
* 2014-01-18
*
* Contains assorted utility classes for helping implement data structures,
* under namespace implementation_utility.
*/

#ifndef IMPL_UTIL_H_
#define IMPL_UTIL_H_

// INLINE will ensure a function is always inlined
#define INLINE inline __attribute__ ((always_inline))

namespace implementation_utility
{
	/*
	 * easily_copyable class has a public type member type which is
	 * const T& if T is not integral and T if it is.
	 */
	template<typename T, typename Condition = std::is_integral<T>>
	class easily_copyable
	{
	private:
		template<bool B, class E>
		struct aux { typedef const E& type; };
		template<class E>
		struct aux<true, E> { typedef E type; };
	public:
		typedef typename aux<Condition::value, T>::type type;
	};

	/**
	 * optional class is inspired by boost::optional<T>, but is simpler and
	 * does not use factories (which copy arguments for construction).
	 *
	 * Allows for explicit construction and destruction of objects.
	 *
	 * Keeps track of whether or not the object is initialized.
	 */
	template<typename T>
	class optional
	{
	private:
		bool initialized; // boolean tracks current state (necessary for moving)
		char store[sizeof(T)]; // store object as a sequence of bytes explicitly
	public:
		// Constructors
		optional();
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
		// Getter for initialized
		INLINE bool valid() const;
		~optional();
	};
}

template<typename T>
implementation_utility::optional<T>::optional() :
	initialized(false) {}

template<typename T>
implementation_utility::optional<T>::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto implementation_utility::optional<T>::operator=(optional&& other) -> optional&
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
void implementation_utility::optional<T>::construct(Args&&... args)
{
	assert(!initialized);
	new (get()) T(std::forward<Args>(args)...);
	initialized = true;
}

template<typename T>
void implementation_utility::optional<T>::destruct()
{
	assert(initialized);
	(*get()).~T();
	initialized = false;
}

// need not be initialized
template<typename T>
T *implementation_utility::optional<T>::get() const
{
	return static_cast<T*>(static_cast<void*>(const_cast<char*>(store)));
}

template<typename T>
bool implementation_utility::optional<T>::valid() const
{
	return initialized;
}

template<typename T>
implementation_utility::optional<T>::~optional()
{
	if(initialized)
		get()->~T();
}

#endif /* IMPL_UTIL_H_ */


