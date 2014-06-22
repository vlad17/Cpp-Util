/*
 * Vladimir Feinberg
 * rwlock.hpp
 * 2014-05-27
 *
 * Generates a wrapper around the pthreads rw lock
 */

#ifndef RWLOCK_HPP_
#define RWLOCK_HPP_

#include <pthread.h>
#include <mutex>

namespace locks
{
  class rw
  {
  private:
    class sharable
    {
    private:
      locks::rw& lkref_;
    public:
      sharable(locks::rw& lk) : lkref_(lk) {}
      void lock() {lkref_.lock_shared();}
      void unlock() {lkref_.unlock_shared();}
    };
    pthread_rwlock_t lock_;
  public:
    typedef sharable ronly;
    rw(const rw&) = delete;
    rw(rw&&) = delete;
    rw& operator=(const rw&) = delete;
    rw& operator=(rw&&) = delete;
    // bool pshared - allow sharing between processes
    // throws system_error if cannot initialize
    explicit rw(bool pshared = false);
    ~rw();
    // read, lock behavior undef if owned, unock undef if not
    void lock_shared();
    bool try_lock_shared() noexcept;
    void unlock_shared();
    // write, lock behavior undef if owned, unlock undef if not
    void lock();
    bool try_lock() noexcept;
    void unlock();
    // read-only reference (lock() is a lock_shared). Only valid as long as this is.
    ronly read_only() {return ronly(*this);}
  };
}

#endif /* RWLOCK_HPP_ */
