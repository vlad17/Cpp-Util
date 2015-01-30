/*
  Vladimir Feinberg
  synchro/hazard-test.cpp
  2015-01-29

  Hazard pointer test.
*/

#include <iostream>
#include <mutex>
#include <unordered_map>

#include "synchro/hazard.hpp"
#include "util/uassert.hpp"

using namespace std;
using namespace synchro;

namespace {

class hazard_tester {
 public:
  void add(int val) {
    hazard_ptr<int> hazard;
    hazard.acquire(new int(val));
    lock_guard<mutex> lk(mtx_);
    hazards_.emplace(val, std::move(hazard));
  }
  void reset(int val) {
    lock_guard<mutex> lk(mtx_);
    auto& hptr = get(val);
    hptr.reset();
  }
  void check_valid(int val) {
    lock_guard<mutex> lk(mtx_);
    auto& hptr = get(val);
    UASSERT(hptr.get())
        << "value " << val << " ptr " << (uintptr_t) hptr.get() << " invalid!";
    UASSERT(hptr.get() == hptr.operator->());
    UASSERT(hptr.get() == &*hptr);
    UASSERT(*hptr == val) << "*hptr (" << *hptr << ") != " << val;
  }
  void check_reset(int val) {
    lock_guard<mutex> lk(mtx_);
    auto& hptr = get(val);
    UASSERT(!hptr) << "hptr (" << (uintptr_t) hptr.get() << ") not reset";
  }
  int* ptr_at(int x) {
    lock_guard<mutex> lk(mtx_);
    return get(x).get();
  }

 private:
  // Should be called unlocked.
  hazard_ptr<int>& get(int x) {
    auto it = hazards_.find(x);
    UASSERT(it != hazards_.end()) << "value " << x << " missing!";
    return it->second;
  }

  unordered_map<int, hazard_ptr<int>> hazards_;
  mutex mtx_; // protects 'hazards_'
};

} // anonymous namespace

int main(int, char**) {
  cout << "Hazard pointer testing." << endl;

  // We run this test with valgrind to check that the deletions actually occur.

  cout << "=====> Testing basic sequential execution" << endl;
  {
    hazard_tester test;
    for (int i = 0; i < 100; ++i) test.add(i);
    for (int i = 0; i < 100; ++i) test.check_valid(i);
    for (int i = 0; i < 100; ++i)
      hazard_ptr<int>::schedule_deletion(test.ptr_at(i));
    for (int i = 0; i < 100; ++i) test.check_valid(i);
    for (int i = 0; i < 50; ++i) test.reset(i);
    for (int i = 0; i < 50; ++i) test.check_reset(i);
    for (int i = 50; i < 100; ++i) test.check_valid(i);
    // Intentionally check that the destructor for the remaining 50
    // automatically releases the hazard pointer's protection.
  }
  cout << "...... Complete!" << endl;

  // TODO randomized hazard ptr testing.
}
