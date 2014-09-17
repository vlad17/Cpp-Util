/*
  Vladimir Feinberg
  caches/cache-test.cpp
  2014-09-08

  Defines tests for caches.
*/

#include <iostream>
#include <random>
#include <utility>

#include "caches/cache.hpp"
#include "caches/lfu_cache.hpp"

using namespace caches;
using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

// runs basic unit tests, single-threaded
template<typename T>
void cache_test(T t);

int main() {
  cout << "LFU test" << endl;
  cout << "\nheap_cache test" << endl;
  lfu::heap_cache<int, int> lhc;
  cache_test(lhc);
  return 0;
}

template<typename T>
void cache_test(T lhc) {
  cout << "Adding (0,1), (0,3) - replace - , (1,2), (2,4)" << endl;
  lhc.insert(make_pair(0,1));
  lhc.insert(make_pair(1,2));
  lhc.insert(make_pair(0,3));
  lhc.insert(make_pair(2,4));
  cout << lhc ; cout << endl;
  cout << "Empty: " << lhc.empty() ; cout << endl;
  cout << "Looking up key 3 (should be null): ptr=";
  cout << lhc.lookup(3) ; cout << endl;
  cout << "Looking up key 0: ";
  cout << *lhc.lookup(0) ; cout << endl;
  cout << "Looking up key 1 twice: ";
  cout << *lhc.lookup(1) << ", ";
  cout << *lhc.lookup(1) ; cout << endl;
  cout << lhc ; cout << endl;
  cout << "Set to max size 1" ; cout << endl;
  lhc.set_max_size(1);
  cout << lhc ; cout << endl;
  cout << "Clear:" ; cout << endl;
  lhc.clear();
  cout << lhc ; cout << endl;
  cout << "Empty: " << lhc.empty() ; cout << endl;
  cout << "set max. size 128, fill" ; cout << endl;
  lhc.set_max_size(128);
  for(int i = 0; i < 128; ++i)
    lhc.insert(make_pair(i,i));
  cout << lhc ; cout << endl;
  cout << "Lookup 500 times, randomly." ; cout << endl;
  for(int i = 0; i < 500; ++i)
    lhc.lookup(gen()&127);
  cout << lhc ; cout << endl;
  cout << "Try to insert new item (128,128)" ; cout << endl;
  lhc.insert(make_pair(128,128));
  cout << lhc ; cout << endl;
  cout << "Generate smaller cache" ; cout << endl;
  T lhc2;
  cout << lhc2 ; cout << endl;
  cout << "Add (i,i) up to 10, lookup randomly." ; cout << endl;
  for(int i = 0; i < 10; ++i)
    lhc2.insert(make_pair(i,i));
  for(int i = 0; i < 20; ++i)
    lhc2.lookup(gen()%10);
  cout << lhc2 ; cout << endl;
  cout << "copy this new cache to old one. Old one:" ; cout << endl;
  lhc = lhc2;
  cout << lhc ; cout << endl;
  cout << "second cache that was copied from after copy:" ; cout << endl;
  cout << lhc2 ; cout << endl;
  cout << "move newly copied old one to previous new one. After move (to assigned):" ; cout << endl;
  lhc2 = move(lhc);
  cout << lhc2 ; cout << endl;
  cout << "after move (from assignment value) - size: " << lhc.size() ; cout << endl;
  lhc2.clear();

#if (defined(NDEBUG) && !USE_UASSERT)
  cout << "stress tests for heap cache...." << endl;
  int constexpr STRESS_REPS = 1e7;
  int constexpr MAX_KEYSIZE = STRESS_REPS/100;
  for(int i = 0; i < STRESS_REPS; ++i)
  {
    int test = gen()%MAX_KEYSIZE;
    auto ptrtest = lhc2.lookup(test);
    if(ptrtest != nullptr && *ptrtest != test)
      cout << "\tError: value " << test << " not matched with key" << endl;
    if(ptrtest == nullptr)
      lhc2.insert(make_pair(test,test));
  }
  cout << "...Completed" << endl;
#endif /* NDEBUG */
}
