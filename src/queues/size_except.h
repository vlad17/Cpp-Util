/*
  Vladimir Feinberg
  2014-08-17
  size_except.h

  Contains definitions of size-related exceptions
*/

#include <stdexcept>

// Full/empty exceptions
class full_error : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

class empty_error : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};
