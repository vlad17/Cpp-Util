/*
  Vladimir Feinberg
  test_util.h
  2014-08-02

  Defines the basic test macros and the main method for
  all the unit test files, which should just implement
  void test_main(void).
*/

#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

// Should throw a test_error if a test fails, explaining what the issue
// is. Do not define int main(). Use this to conduct all tests instead.
//
// It is called exactly once.
//
// Check all statements using ASSERT(expr, message) or just ASSERT(expr),
// where expr should evaluate to false if the test should fail and the message
// should display. Do not add additional commas within either argument.
//
// message can be either a string literal, or many types bound with the
// operator<<. For instance:
//
// ASSERT(x == 5, "x is " << x);
//
// The above will print nothing if x == 5. Otherwise, if x is, say, 4,
// it will stop the test (returning a nonzero value from main) and print
// the following, and then a newline:
//
// Expr `x == 5` failed: x is 4
void test_main(int argc, char** argv);

#include "utilities/test_util.include"

#endif /* TEST_UTIL_H_ */
