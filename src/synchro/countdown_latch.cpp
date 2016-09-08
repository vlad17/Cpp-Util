/*
  Vladimir Feinberg
  synchro/countdown_latch.cpp
  2014-09-14

  Implements the countdown latch class.
*/

#include "synchro/countdown_latch.hpp"

#include <condition_variable>
#include <mutex>

#include <util/uassert.hpp>

using namespace std;
using namespace synchro;

countdown_latch::countdown_latch(int wait) :
    unready_count_(wait) {
  UASSERT(wait > 0);
}

countdown_latch::~countdown_latch() {
  // no need to acquire the lock - it's the user's fault if they
  // are giving access across threads to a destructing object.
  ready_.notify_all();
}

void countdown_latch::down() {
  // notice the two fetches here - this is why we need a lock
  bool broadcast = false;
  {
    lock_guard<mutex> lk(lock_);
    if (unready_count_ == 0) return;
    UASSERT(unready_count_ > 0);
    broadcast = !--unready_count_;
  }
  // Broadcast-after-unlock is OK here since it's the user's responsibility to
  // reset the countdown latch in a synchronized manner, so we can expect there
  // will be no "intercepting" threads who call wait() at this moment.
  if (broadcast)
    ready_.notify_all();
}

void countdown_latch::wait() {
  unique_lock<mutex> lk(lock_);
  ready_.wait(lk, [this] { return unready_count_ == 0; });
}

void countdown_latch::reset(int wait) {
  UASSERT(wait > 0);
  lock_guard<mutex> lk(lock_);
  unready_count_ = wait;
}
