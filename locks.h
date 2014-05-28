/*
 * Vladimir Feinberg
 * locks.h
 * 2014-05-27
 *
 * Generates a wrapper around the pthreads rw lock
 */

#ifndef RWLOCK_H_
#define RWLOCK_H_

#include "pthread.h"

namespace locks
{
	class rw
	{
	private:
		void check(int err, bool kill);
		pthread_rwlock_t _lock;
	public:
		rw(const rw&) = delete;
		rw(rw&&) = delete;
		rw& operator=(const rw&) = delete;
		rw& operator=(rw&&) = delete;
		// bool pshared - allow sharing between processes?
		// throws system_error if cannot initialize
		explicit rw(bool pshared = false);
		~rw();
		// read, lock behavior undef if owned, unlock undef if not
		void lock_shared();
		bool try_lock_shared(); //
		void unlock_shared();
		// write, lock behavior undef if owned, unlock undef if not
		void lock();
		bool try_lock();
		void unlock();
	};
}

#endif /* RWLOCK_H_ */




