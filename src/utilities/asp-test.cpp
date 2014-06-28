/*
  Vladimir Feinberg
  asp-test.cpp
  2014-06-28

  Unit tests for atomic shared pointer.
*/

#include <iostream>
#include <cassert>

#include "utilities/atomic_shared_ptr.h"

using namespace std;

void test_one_ptr_one_thread(),
  test_mul_ptr_one_thread(),
  test_one_ptr_mul_thread(),
  test_mul_ptr_mul_thread();

int main() {
  test_one_ptr_one_thread();
  test_mul_ptr_one_thread();
  test_one_ptr_mul_thread();
  test_mul_ptr_mul_thread();
  return 0;
}

void test_one_ptr_one_thread() {
  atomic_shared_ptr<int> iptr;
  assert(iptr.get() == nullptr);
  assert(iptr.use_count() == 0);
  iptr.reset(5);
  assert(*iptr.get() == 5);
  assert(iptr.use_count() == 1);
}


void test_mul_ptr_one_thread() {}
void test_one_ptr_mul_thread() {}
void test_mul_ptr_mul_thread() {}
