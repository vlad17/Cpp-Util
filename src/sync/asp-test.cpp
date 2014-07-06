/*
  Vladimir Feinberg
  asp-test.cpp
  2014-06-28

  Unit tests for atomic shared pointer.
*/

#include <iostream>
#include <cassert>
#include <vector>

#include "sync/atomic_shared_ptr.h"

using namespace std;

void test_one_thread(), test_scope();

int main() {
  test_one_thread();
  test_scope();
  return 0;
}

void test_basic(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  if (ptr) {
    assert(shared != nullptr);
    assert(ptr == &*shared);
    assert(shared.use_count() > 0);
    assert(shared);
  }
  else {
    assert(shared.use_count() == 0);
    assert(shared == nullptr);
    assert(shared == atomic_shared_ptr<int>());
    assert(!shared);
  }
  assert(ptr == shared.operator->());
  assert(shared.unique() == (shared.use_count() == 1));
}

void test_move(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  int uses = shared.use_count();
  if (ptr) assert(uses > 0);
  else assert(uses == 0);
  auto moved = move(shared);
  assert(shared == nullptr);
  assert(ptr == moved.get());
  assert(uses == moved.use_count());
  moved.swap(shared);
}


void test_copy(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  int uses = shared.use_count();
  if (ptr) assert(uses > 0);
  else assert(uses == 0);
  auto cpy = shared;
  assert(ptr == cpy.get());
  assert(ptr == shared.get());
  assert(shared.use_count() == cpy.use_count());
  assert(cpy == shared);
  if (ptr) assert(cpy.use_count() == uses + 1);
  else assert(cpy.use_count() == 0);
}

void test_scope() {
  atomic_shared_ptr<int> iptr;
  assert(iptr.get() == nullptr);
  assert(iptr.use_count() == 0);
  iptr.reset(5);
  assert(*iptr.get() == 5);
  assert(iptr.use_count() == 1);
  assert(iptr.unique());
  {
    atomic_shared_ptr<int> cpy = iptr;
    assert(iptr.use_count() == 2);
    assert(cpy.use_count() == 2);
    assert(iptr.get() == cpy.get());
    assert(!iptr.unique());
    assert(!cpy.unique());
    assert(*cpy.get() == 5);
    iptr = nullptr;
    assert(cpy.unique());
    assert(cpy.use_count() == 1);
    assert(iptr.get() == nullptr);
    assert(iptr.use_count() == 0);
    assert(*cpy.get() == 5);
    cpy.reset(4);
    assert(*cpy.get() == 4);
    assert(cpy.use_count() == 1);
    iptr = move(cpy);
    assert(cpy == nullptr);
    assert(cpy.use_count() == 0);
    assert(iptr.use_count() == 1);
    assert(iptr.get() != nullptr);
    assert(*iptr == 4);
    cpy = iptr;
    assert(iptr.use_count() == 2);
  }
  assert(iptr.use_count() == 1);
  assert(*iptr == 4);
}

void run_battery(atomic_shared_ptr<int>& ptr) {
  test_basic(ptr);
  test_copy(ptr);
  test_move(ptr);
}

template<class List>
void run_battery(List& list) {
  for (auto ptr : list)
    run_battery(*ptr);
}

void test_full(atomic_shared_ptr<int>& ptr) {
  run_battery(ptr);
  auto cpy = ptr;
  run_battery(cpy);
  auto mov = move(ptr);
  vector<atomic_shared_ptr<int>*> list = {&mov, &ptr, &cpy};
  run_battery(list);
  ptr = cpy;
  run_battery(list);
  ptr = nullptr;
  run_battery(list);
  atomic_shared_ptr<int> next(-1);
  list.push_back(&next);
  run_battery(list);
  ptr.swap(next);
  run_battery(list);
  cpy.reset(-1);
  run_battery(list);
  assert(cpy != next);
  for (auto ptr : list) *ptr = nullptr;
  run_battery(list);
}

void test_one_thread() {
  atomic_shared_ptr<int> nonnull(5);
  test_full(nonnull);
  atomic_shared_ptr<int> null;
  test_full(null);
}
