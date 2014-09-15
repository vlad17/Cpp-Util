/*
  Vladimir Feinberg
  util/nullstream.hpp
  2014-09-08

  Non-printing stream.
*/

#ifndef UTIL_NULLSTREAM_H_
#define UTIL_NULLSTREAM_H_

#include <ostream>

namespace util {

class nullstream_class : public std::ostream {
 public:
  template<typename T>
  nullstream_class& operator<<(const T&) {
    return *this;
  }
} nullstream;

} // namespace util

#endif /* UTIL_NULLSTREAM_H_ */
