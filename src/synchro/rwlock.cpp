/*
 * Vladimir Feinberg
 * synchro/rwlock.cpp
 * 2014-09-08
 *
 * r/w lock implementation
 */

#include "synchro/rwlock.hpp"

#include <system_error>
#include <pthread.h>

using namespace std;
using namespace synchro;
using namespace locks;

// RW class --------------------------------------------------------------

namespace {
  void ThrowOnError(int err) {
    if (err)
      throw system_error {error_code{err, generic_category()}};
  }
  void DieOnError(pthread_rwlock_t* rw, int err) {
    if (err) {
      ThrowOnError(pthread_rwlock_destroy(rw));
      throw system_error {error_code{err, generic_category()}};
    }
  }
}

rw::rw(bool pshared) :
  lock_(PTHREAD_RWLOCK_INITIALIZER)
{
  // private to process by default
  if(pshared)
    {
      pthread_rwlockattr_t attr;
      DieOnError(&lock_, pthread_rwlockattr_init(&attr));
      DieOnError(&lock_, pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED));

      DieOnError(&lock_, pthread_rwlock_destroy(&lock_));
      ThrowOnError(pthread_rwlock_init(&lock_, &attr));

      DieOnError(&lock_, pthread_rwlockattr_destroy(&attr)); // does not affect lock
    }
}

rw::~rw() { pthread_rwlock_destroy(&lock_); }

// below methods assume valid state.

// read
void rw::lock_shared() { ThrowOnError(pthread_rwlock_rdlock(&lock_)); }
bool rw::try_lock_shared() noexcept { return !pthread_rwlock_tryrdlock(&lock_); }
void rw::unlock_shared() { ThrowOnError(pthread_rwlock_unlock(&lock_)); }
// write
void rw::lock() { ThrowOnError(pthread_rwlock_wrlock(&lock_)); }
bool rw::try_lock() noexcept {return !pthread_rwlock_trywrlock(&lock_); }
void rw::unlock() { ThrowOnError(pthread_rwlock_unlock(&lock_)); }
