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

class nullstream : public std::ostream {
 public:
  template<typename T>
  inline nullstream& operator<<(const T&) { return *this; }
};

} // namespace util

#endif /* UTIL_NULLSTREAM_H_ */
