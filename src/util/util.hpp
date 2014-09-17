/*
 * Vladimir Feinberg
 * util/util.h
 * 2014-09-08
 *
 * Header for basic utility classes. Partially influenced by:
 * http://programmers.stackexchange.com/questions/
 *   237339/is-iterable-like-behavior-in-c-attainable/237352#237352
 */

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

// TODO this is just for compatibility, remove all uses later.
#define INLINE

// TODO cleanup the below

namespace util {

template<typename Iterator>
class iterable;

template<typename T>
auto as_iterator(T& t) -> iterable<decltype(t.begin())>;

template<typename Iterator>
class iterable {
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
auto as_iterator(T& t) -> iterable<decltype(t.begin())> {
  return iterable<decltype(t.begin())>(t.begin(), t.end());
}

// Pointer comparison
template<typename T>
struct ptrcmp {
  bool operator()(const T* t1, const T* t2) {return *t1 < *t2;}
};

/*
 * scref gives a constant reference for non-scalar types, or the
 * type itself otherwise. Allows for faster value semantics when it's worth it.
 */
template<typename T>
class scref {
 private:
  template<bool B, class E>
  struct aux { typedef const E& type; };
  template<class E>
  struct aux<true, E> { typedef const E type; };
 public:
  typedef typename aux<std::is_scalar<T>::value, T>::type type;
};

}

#endif /* UTIL_UTIL_H_ */
