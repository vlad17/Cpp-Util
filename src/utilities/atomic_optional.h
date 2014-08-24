/*
  Vladimir Feinberg
  2014-08-16
  atomic_optional.h

  Implements the atomic_optional class, which is based off the optional
  class (see my own implementation and boost's as well).
*/

#include <atomic>

namespace util {

// Atomic optional may or may not hold a value, atomically.
// It is meant to be used by only one construction/destruction
// iteration, where only one thread gets to destruct the object.
//
// No memory barriers are provided. That is up to the mechanism
// exchanging the atomic_optional references accross threads. In other
// words, the user must manually make sure that the constructed value
// of T is visible accross querying threads.
template<typename T>
class atomic_optional {
 public:
  // Constructors
  atomic_optional(); // does not default-initialize T
  atomic_optional(T&& rval);
  ~atomic_optional();

  // Assignment operators
  atomic_optional& operator=(const atomic_optional&) = delete;
  atomic_optional& operator=(atomic_optional&& other) = delete;

  // Get pointer to location of stored optional object.
  // Value of T may be undefined if not valid.
  T* get() const;

  // Getter for initialized
  bool valid() const;

  // Returns whether this thread invalidated this object.
  // Is false if other threads got to it first, or was not
  // valid to begin with.
  bool invalidate();

 private:
  std::atomic<bool> initialized;
  char store[sizeof(T)];
};

} // namespace util

#include "utilities/atomic_optional.tpp"
