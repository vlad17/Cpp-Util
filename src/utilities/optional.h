/*
  Vladimir Feinberg
  optional.h
  2014-09-01

  The optional type declarations.
*/

#ifndef UTILITIES_OPTIONAL_H
#define UTILITIES_OPTIONAL_H

namespace util {

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
  alignas(T) char store[sizeof(T)]; // store object as a sequence of bytes explicitly
  bool initialized; // boolean tracks current state (necessary for moving)
 public:
  // Constructors
  optional(); // does not default-initialize
  optional(const T&);
  optional(T&&);
  optional(const optional&);
  optional(optional&&);
  // Assignment operators
  // (call assignment operators for T if both lhs and rhs are initialized)
  optional& operator=(const optional&);
  optional& operator=(optional&&);
  // Explicit construction, destruction
  template<typename... Args>
  void construct(Args&&...);
  void destruct();
  // Get pointer to location of stored optional object.
  // Value of T may be undefined if not valid.
  T* get();
  T& access();
  const T* get() const;
  const T& access() const;
  // Getter for initialized
  bool valid() const;
  ~optional();
};

} // namespace util

#include "utilities/optional.tpp"

#endif /* UTILITIES_OPTIONAL_H */
