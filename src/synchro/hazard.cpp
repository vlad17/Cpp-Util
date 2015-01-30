/*
  Vladimir Feinberg
  synchro/hazard.cpp
  2015-01-29

  This file houses the larger methods suitable for separate compilation that
  are separate from the type-safe hazard_ptr.

  Recall this is based off of:
  http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
*/

#include "synchro/hazard.hpp"

#include <atomic>
#include <unordered_set>
#include <utility>
#include <vector>

using std::pair;
using std::unordered_set;
using std::vector;
using namespace synchro;
using namespace _synchro_hazard_internal;

namespace {

// This file maintains the static
// executable-wide hazard pointer list and thread-local retired lists.
//
// There is some justification for the use of a static list like this.
// It's already thread-safe, especially since C++11 blesses multi-threaded
// static initialization. Furthermore, there are no issues with loading this
// as a dynamic library; the loaded file will just maintain its own list
// until unlink time.
class hazard_list {
 public:
  hazard_list() : head_(nullptr), len_(0), on_(true) {}
  ~hazard_list() {
    // Turn the list "off" so that deletions become no longer dependent
    // on the hazard record list, which we can then delete.
    on_.store(false, std::memory_order_relaxed);
    for (auto prev = head_.load(std::memory_order_acquire); prev;) {
      auto next = prev->next();
      delete prev;
      prev = next;
    }
  }

  static hazard_record* head() {
    if (!global_list_.on_.load(std::memory_order_relaxed)) return nullptr;
    // Use "acquire" semantics so we can read the 'next_' pointer.
    return global_list_.head_.load(std::memory_order_acquire);
  }
  static bool cas_head(hazard_record*& expect, hazard_record* desired) {
    // Use "release" semantics to publish the 'next_' pointer upon success.
    return global_list_.head_.compare_exchange_weak(
        expect, desired, std::memory_order_release, std::memory_order_relaxed);
  }
  static void incr_len() {
    global_list_.len_.fetch_add(1, std::memory_order_relaxed);
  }
  static size_t len() {
    return global_list_.len_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<hazard_record*> head_;
  std::atomic<size_t> len_;
  std::atomic<bool> on_;
  static hazard_list global_list_;
};
hazard_list hazard_list::global_list_;

// Thread-local "retired list" of pointers discarded by this thread.
struct retired_list {
  ~retired_list();
  vector<pair<void*, hazard_record::deleter_t> > rlist;
};

thread_local retired_list thread_retired;

// Thread iterates through the global hazard pointer list, searching
// for non-null protected pointers. It then takes the set difference
// (rlist) - (snapshot of protected pointers). This provides (according to
// the contract by schedule_deletion) a conservative, safe estimate of pointers
// viable for deletion.
void scan_delete() {
  // TODO: potential optimization: pre-alloc len() size.
  unordered_set<void*> snap;
  for (auto rec = hazard_list::head(); rec; rec = rec->next()) {
    auto ptr = rec->protected_ptr();
    if (ptr) snap.insert(ptr);
  }
  for (auto p : thread_retired.rlist) {
    auto ptr = p.first;
    auto del = p.second;
    if (snap.find(ptr) == snap.end())
      del(ptr);
  }
}

retired_list::~retired_list() { scan_delete(); }

} // anonymous namespace

hazard_record::hazard_record() :
    active_(1), protected_ptr_(nullptr), next_(nullptr) {}

void* hazard_record::protected_ptr() {
  return protected_ptr_.load(std::memory_order_relaxed);
}

hazard_record* hazard_record::next() const {
  return next_;
}

bool hazard_record::active() const {
  return active_.load(std::memory_order_relaxed);
}

bool hazard_record::capture() {
  bool expected = false;
  return active_.compare_exchange_strong(
      expected, true, std::memory_order_relaxed, std::memory_order_relaxed);
}

hazard_record* hazard_record::activated_record() {
  auto tentative = hazard_list::head();
  // Search through the list for an available hazard_record.
  for (; tentative; tentative = tentative->next_)
    if (tentative->active() || !tentative->capture()) continue;
    else return tentative;

  // Search failed. Make a new hazard record.
  // Note none of the below operations throw, so we're still providing
  // a strong exception-safety guarnatee.
  hazard_list::incr_len();
  auto new_hazard = new hazard_record;

  // TODO: potential optimization: the below read can use std::mo_relaxed.
  auto oldhead = hazard_list::head();
  do new_hazard->next_ = oldhead;
  while (!hazard_list::cas_head(oldhead, new_hazard));

  return new_hazard;
}

void hazard_record::schedule_deletion(void* ptr, deleter_t deleter) {
  thread_retired.rlist.emplace_back(ptr, deleter);
  // Use of magic constant 5/4 upon recommendation from source.
  // Anything >1 will allow for constant-time amortization.
  if (thread_retired.rlist.size() >= 5 * hazard_list::len() / 4)
    scan_delete();
}

void hazard_record::deactivate() {
  // Note that this order is important (and is inforced by std::atomic).
  // We deactivate only after nulling the pointer, thus maintaining the
  // invariant that all deactivated hazard records have their 'protected_ptr_'
  // set to nullptr. This enables scanning deleting threads to just check
  // the value of the protected ptr (instead of that and the active_ value),
  // saving on an atomic read.
  publish(nullptr);
  active_.store(false, std::memory_order_relaxed);
}
