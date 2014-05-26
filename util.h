/*
 * Vladimir Feinberg
 * util.h
 * 2014-04-29
 *
 * Header for basic utility classes. Partially influenced by:
 * http://programmers.stackexchange.com/questions/237339/is-iterable-like-behavior-in-c-attainable/237352#237352
 */

#ifndef UTIL_H_
#define UTIL_H_

namespace util
{
	template<typename Iterator>
	class iterable;

	// todo below needed?
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
}

#endif /* UTIL_H_ */
