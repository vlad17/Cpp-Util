/*
  Vladimir Feinberg
  asp-test.cpp
  2014-06-28

  Unit tests for atomic shared pointer.
*/

#include <iostream>
#include <vector>

#include "sync/atomic_shared_ptr.h"
#include "utilities/test-util.h"

using namespace std;

void test_one_thread(), test_scope(), test_casts();

void test_main() {
  test_one_thread();
  test_scope();
  test_casts();
  // TODO add atomicops to full test
  // TODO test_full with locking
  // TODO randomized shared pointer test (compare to shared pointer)
  // TODO with atomic ops ^^^
}

void test_basic(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  if (ptr) {
    ASSERT(shared != nullptr);
    ASSERT(ptr == &*shared);
    ASSERT(shared.use_count() > 0);
    ASSERT(shared);
  }
  else {
    ASSERT(shared.use_count() == 0);
    ASSERT(shared == nullptr);
    ASSERT(shared == atomic_shared_ptr<int>());
    ASSERT(!shared);
  }
  ASSERT(ptr == shared.operator->());
  ASSERT(shared.unique() == (shared.use_count() == 1));
}

void test_move(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  int uses = shared.use_count();
  if (ptr) ASSERT(uses > 0);
  else ASSERT(uses == 0);
  auto moved = move(shared);
  ASSERT(shared == nullptr);
  ASSERT(ptr == moved.get());
  ASSERT(uses == moved.use_count());
  moved.swap(shared);
}


void test_copy(atomic_shared_ptr<int>& shared) {
  int* ptr = shared.get();
  int uses = shared.use_count();
  if (ptr) ASSERT(uses > 0);
  else ASSERT(uses == 0);
  auto cpy = shared;
  ASSERT(ptr == cpy.get());
  ASSERT(ptr == shared.get());
  ASSERT(shared.use_count() == cpy.use_count());
  ASSERT(cpy == shared);
  if (ptr) ASSERT(cpy.use_count() == uses + 1);
  else ASSERT(cpy.use_count() == 0);
}

void test_scope() {
  atomic_shared_ptr<int> iptr;
  ASSERT(iptr.get() == nullptr);
  ASSERT(iptr.use_count() == 0);
  iptr.reset(5);
  ASSERT(*iptr.get() == 5);
  ASSERT(iptr.use_count() == 1);
  ASSERT(iptr.unique());
  {
    atomic_shared_ptr<int> cpy = iptr;
    ASSERT(iptr.use_count() == 2);
    ASSERT(cpy.use_count() == 2);
    ASSERT(iptr.get() == cpy.get());
    ASSERT(!iptr.unique());
    ASSERT(!cpy.unique());
    ASSERT(*cpy.get() == 5);
    iptr = nullptr;
    ASSERT(cpy.unique());
    ASSERT(cpy.use_count() == 1);
    ASSERT(iptr.get() == nullptr);
    ASSERT(iptr.use_count() == 0);
    ASSERT(*cpy.get() == 5);
    cpy.reset(4);
    ASSERT(*cpy.get() == 4);
    ASSERT(cpy.use_count() == 1);
    iptr = move(cpy);
    ASSERT(cpy == nullptr);
    ASSERT(cpy.use_count() == 0);
    ASSERT(iptr.use_count() == 1);
    ASSERT(iptr.get() != nullptr);
    ASSERT(*iptr == 4);
    cpy = iptr;
    ASSERT(iptr.use_count() == 2);
  }
  ASSERT(iptr.use_count() == 1);
  ASSERT(*iptr == 4);
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
  ASSERT(cpy != next);
  for (auto ptr : list) *ptr = nullptr;
  run_battery(list);
}

void test_one_thread() {
  atomic_shared_ptr<int> nonnull(5);
  test_full(nonnull);
  atomic_shared_ptr<int> null;
  test_full(null);
}

class A {};
class B : public A {};
class C {};

template<class T, class U>
void test_null_isolated(T& t, U& u) {
  ASSERT(t == u);
  t = nullptr;
  ASSERT(t == nullptr);
  ASSERT(u != nullptr);
  ASSERT(t != u);
}

template<bool SetOriginToNull, class DownCast>
void safe_down_cast(const DownCast& dc) {
  atomic_shared_ptr<A> a_from_b(default_construct);
  ASSERT(a_from_b != nullptr);
  // TODO inject casted construction here
  atomic_shared_ptr<B> bcast = dc(a_from_b);
  ASSERT(a_from_b == bcast);
  ASSERT(a_from_b.get() == static_cast<B*>(bcast.get()));
  // Set origin to null, check casted untouched.
  if (SetOriginToNull) {
    test_null_isolated(a_from_b, bcast);
  } else {
    test_null_isolated(bcast, a_from_b);
  }
}

void test_casts() {
  // Derived construction test of casting
  safe_down_cast<true>([](const atomic_shared_ptr<A>& a) {
      return static_pointer_cast<B>(a);
    });
  safe_down_cast<false>([](const atomic_shared_ptr<A>& a) {
      return static_pointer_cast<B>(a);
    });
  // Derived construction test of constructing
  safe_down_cast<true>([](const atomic_shared_ptr<A>& a) {
      return atomic_shared_ptr<B>(a);
    });
  safe_down_cast<false>([](const atomic_shared_ptr<A>& a) {
      return atomic_shared_ptr<B>(a);
    });
  // Upcast test
  atomic_shared_ptr<B> bptr(default_construct);
  ASSERT(bptr != nullptr);
  atomic_shared_ptr<A> upcast = static_pointer_cast<A>(bptr);
  ASSERT(upcast == bptr);
}
