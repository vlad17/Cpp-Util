/*
  Vladimir Feinberg
  util/timer.h
  2016-09-08

  Smart pointer utilities
*/

#ifndef UTIL_MEMORY_HPP_
#define UTIL_MEMORY_HPP_

#include <memory>

namespace util {

// Overload in this namespace while waiting for C++14...
template<class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Overload in this namespace while waiting for C++14...
template<class T>
std::unique_ptr<T> uniquep(T t) {
  return std::unique_ptr<T>(new T(std::move(t)));
}

} // namespace util

#endif /* UTIL_MEMORY_HPP_ */
