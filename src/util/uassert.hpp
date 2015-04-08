/*
  Vladimir Feinberg
  util/uassert.hpp
  2014-09-14

  Defines a slightly more convenient multi-threaded assert macro,
  inspired by gtest but not quite as powerful.
*/

#ifndef UTIL_UASSERT_HPP_
#define UTIL_UASSERT_HPP_

// Reserved macro name, don't use it.
#ifdef _UTIL_UASSERT_INTERNAL_MACRO_UNDEF
#error "macro _UTIL_UASSERT_INTERNAL_MACRO_UNDEF reserved for internal use"
#endif /* _UTIL_UASSERT_INTERNAL_MACRO_UNDEF */

// Assertions may be optionally activated/deactivated on top of NDEBUG
// by defining USE_UASSERT as 1 or 0. They default to the same
// behavior that <cassert>'s assert() has.
#ifndef USE_UASSERT
#define _UTIL_UASSERT_INTERNAL_MACRO_UNDEF
#define USE_UASSERT !defined(NDEBUG)
#endif /* USE_UASSERT */


// Check all statements using ASSERT(expr) << message or just ASSERT(expr),
// where expr should evaluate to false if the test should fail and the message
// should display.
//
// The expression is evaluated exactly once.
//
// message can be either a string literal, or many types bound with the
// operator<<. For instance:
//
// ASSERT(x == 5) << "x is " << x;
//
// The above will print nothing if x == 5. Otherwise, if x is, say, 4,
// it will stop the test (returning a nonzero value from main) and print
// the following, and then a newline:
//
// Expr `x == 5` failed: x is 4
//
// Errors are printed to cerr. As soon as an assertion triggers the program
// calls std::exit with a failure code (because of this, assertions shouldn't
// be called from detached threads).
//
// The error string will always be printed atomically, even if the
// message is broken up into several different types. A newline
// is automatically appended at the end of the message.
#if USE_UASSERT
#define UASSERT(expr) \
  util::_util_uassert_internal::streamer(!(expr), __FILE__, __LINE__, #expr)
#else
#include <util/nullstream.hpp>
#define UASSERT(expr) if ((expr) || true) {} else util::nullstream{}
#endif /* USE_UASSERT */

#if USE_UASSERT
#include <sstream>

namespace util {
namespace _util_uassert_internal {

class streamer {
 public:
  streamer(bool print, const char* file, long line, const char* expr);
  ~streamer();
  template<class T>
  streamer& operator<<(const T& t) {
    if (!print_) return *this;
    try { sstr_ << t; } catch (...) { unexpected(); }
    return *this;
  }
 private:
  void unexpected();
  std::stringstream sstr_;
  const bool print_;
  const char* file_;
  const long line_;
  const char* expr_;
};

} // namespace _util_uassert_internal
} // namespace util
#endif /* USE_UASSERT */

#ifdef _UTIL_UASSERT_INTERNAL_MACRO_UNDEF
#undef USE_UASSERT
#undef _UTIL_UASSERT_INTERNAL_MACRO_UNDEF
#endif /* _UTIL_UASSERT_INTERNAL_MACRO_UNDEF */

#endif /* UTIL_UASSERT_HPP_ */
