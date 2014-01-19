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

	// TODO bare_optional - minimal


	// TODO optional - safe
}

#endif /* IMPL_UTIL_H_ */


