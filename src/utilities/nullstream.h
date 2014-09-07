/*
  Vladimir Feinberg
  nullstream.h
  2014-09-06

  Non-printing stream.
*/

#ifndef NULLSTREAM_H_
#define NULLSTREAM_H_

#include <iostream>

class nullstream_class : public std::ostream {
 public:
  template <class T>
  nullstream_class& operator<<(const T& t) {
    return *this;
  }
} nullstream;

#endif /* Non-printing stream. */
