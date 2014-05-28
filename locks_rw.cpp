/*
 * Vladimir Feinberg
 * locks_rw.cpp
 * 2014-05-27
 *
 * r/w lock implementation
 */

#include <system_error>
#include <pthread.h>

#include "locks.h"

using namespace std;
using namespace locks;

// RW LOCK class --------------------------------------------------------------

rw::rw(bool pshared) :
	_lock(PTHREAD_RWLOCK_INITIALIZER)
{
	// private by default
	if(pshared)
	{
		pthread_rwlockattr_t attr;
		check(pthread_rwlockattr_init(&attr), true);
		check(pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED), true);

		check(pthread_rwlock_destroy(&_lock), true);
		check(pthread_rwlock_init(&_lock, &attr), false);

		check(pthread_rwlockattr_destroy(&attr), true); // does not affect lock
	}
}

rw::~rw()
{
	// spin on delete while busy
	while(pthread_rwlock_destroy(&_lock) == EBUSY);
}

void rw::check(int err, bool kill)
{
	if(err)
	{
		if(kill) pthread_rwlock_destroy(&_lock); // attempt to destroy
		throw system_error {error_code{err, generic_category()}};
	}
}

// below methods assume valid state.

// read
void rw::lock_shared() {while(pthread_rwlock_rdlock(&_lock) == EAGAIN);}
bool rw::try_lock_shared() {return !pthread_rwlock_tryrdlock(&_lock);}
void rw::unlock_shared() {pthread_rwlock_unlock(&_lock);}
// write
void rw::lock() {pthread_rwlock_wrlock(&_lock);}
bool rw::try_lock() {return !pthread_rwlock_trywrlock(&_lock);}
void rw::unlock() {pthread_rwlock_unlock(&_lock);}

