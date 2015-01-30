/*
  Vladimir Feinberg
  synchro/hazard-test.cpp
  2015-01-29

  Hazard pointer test.
*/

#include <atomic>
#include <iostream>
#include <future>
#include <thread>
#include <unordered_map>
#include <vector>

#include "synchro/hazard.hpp"
#include "util/uassert.hpp"

using namespace std;
using namespace synchro;

namespace {

// Writing ops are thread-unsafe, but reading is ok.
class hazard_tester {
 public:
  // THREAD UNSAFE -----
  void add(int val) {
    hazard_ptr<int> hazard;
    hazard.acquire(new int(val));
    hazards_.emplace(val, std::move(hazard));
  }
  // THREAD SAFE -----
  void overwrite(int val) {
    hazard_ptr<int> hazard;
    hazard_ptr<int>& old = get(val);
    old = std::move(hazard);
  }
  void reset(int val) {
    auto& hptr = get(val);
    hptr.reset();
  }
  void check_valid(int val) {
    auto& hptr = get(val);
    UASSERT(hptr.get())
        << "value " << val << " ptr " << (uintptr_t) hptr.get() << " invalid!";
    UASSERT(hptr.get() == hptr.operator->());
    UASSERT(hptr.get() == &*hptr);
    UASSERT(*hptr == val) << "*hptr (" << *hptr << ") != " << val;
  }
  void check_reset(int val) {
    auto& hptr = get(val);
    UASSERT(!hptr) << "hptr (" << (uintptr_t) hptr.get() << ") not reset";
  }
  int* ptr_at(int x) {
    return get(x).get();
  }

 private:
  hazard_ptr<int>& get(int x) {
    auto it = hazards_.find(x);
    UASSERT(it != hazards_.end()) << "value " << x << " missing!";
    return it->second;
  }

  unordered_map<int, hazard_ptr<int>> hazards_;
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

  cout << "=====> Testing multithreaded execution" << endl;
  {
    static const int kWriters = 5, kReaders = 12, kPtrs = 6000;
    hazard_tester test;
    for (int i = 0; i < kPtrs; ++i) test.add(i);
    for (int i = 0; i < kPtrs; ++i) test.check_valid(i);
    static const int numWrites = kPtrs / kWriters; // divides evenly, important!
    static const int numReads = kPtrs / kReaders;

    vector<future<void> > reader_futs;
    std::atomic<bool> kr_array[kReaders];
    for (auto& ab : kr_array) ab.store(true, std::memory_order_relaxed);
    for (int i = 0; i < kReaders; ++i) reader_futs.push_back(async(
             launch::async, [&](int idx) {
               auto keep_reading = [&]() {
                 return kr_array[idx].load(std::memory_order_relaxed);
               };
               int start = idx * numReads;
               while (keep_reading()) {
                 for (int i = start; i < start + numReads
                          && keep_reading(); ++i)
                   test.check_valid(i);
                 std::this_thread::yield();
               }
               for (int i = start; i < start + numReads; ++i) {
                 test.check_valid(i);
                 test.overwrite(i);
                 test.reset(i);
                 test.check_reset(i);
               }
             }, i));
    vector<future<void> > writer_futs;
    for (int i = 0; i < kWriters; ++i) writer_futs.push_back(async(
             launch::async, [&](int idx) {
               const int start = idx * numWrites;
               for (int i = start; i < start + numWrites; ++i)
                 hazard_ptr<int>::schedule_deletion(test.ptr_at(i));
             }, i));
    // Release half first, just to make sure it's still ok to read even
    // when gc is going on.
    for (auto& fut : writer_futs) fut.get();
    for (int i = 0; i < kReaders / 2; ++i)
      kr_array[i].store(false, std::memory_order_relaxed);
    for (int i = 0; i < kReaders / 2; ++i)
      reader_futs[i].get();

    // Reap remaining readers.
    for (int i = kReaders / 2; i < kReaders; ++i)
      kr_array[i].store(false, std::memory_order_relaxed);
    for (int i = kReaders / 2; i < kReaders; ++i)
      reader_futs[i].get();
  }
  cout << "...... Complete!" << endl;
}
