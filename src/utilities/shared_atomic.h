/*
  Vladimir Feinberg
  2014-08-24
  shared_atomic.h

  Atomic operations on shared pointers themselves are unfortuantely not
  supported by libstdc++, even though they are by libc++. This header
  provides a temporary compatibility resolution for the problem, which
  decays into nop methods if HAS_SHARED_PTR_ATOMICS is defined.
*/

#ifndef UTILTIES_SHARED_ATOMIC_H_
#define UTILTIES_SHARED_ATOMIC_H_

// Overloads for unsupporting enviornments of atomic_... overloads
// on shared_ptr. Use just like the normal overloads, but have a
// top-level spinlock on the finest grain of multithreaded access
// defined with SHARED_PTR_LOCK(name). This is intended to decay into the
// regular standard-defined behavior when
//
// Usage:
// class myclass {
// public:
//   void method();
//   SHARED_COMPATIBLE_LOCK(mtx);
//   std::shared_ptr<int> ptr;
// }; // myclass will only be movable
// void myclass::method() {
//   auto mo = std::memory_order_relaxed;
//   auto local = std::atomic_load_explicit(&ptr, mo, SHARED_COMPATIBLE(mtx));
// }
// void nonmember(myclass& mc) {
//   auto mo = std::memory_order_relaxed;
//   auto local = std::atomic_load_explicit(&mc.ptr, mo,
//                                          SHARED_COMPATIBLE(mc.mtx));

#include <mutex>
#include <memory>
// #include TODO boost spinlock and use that instead of mutex
// TODO pull out all the interal implementation details into a tpp

namespace _sa_internal {

// Here's the key: turn off if libc++, which supports this already
#ifdef HAS_SHARED_PTR_ATOMICS // TODO autodetect
class dummy {};
typedef dummy lock;
#define SHARED_COMPATIBLE_LOCK(name) _sa_internal::lock name
#define SHARED_COMPATIBLE(name) (name)
#else
typedef std::mutex lock;
#define SHARED_COMPATIBLE_LOCK(name) std::unique_ptr<std::mutex> \
  name { new std::mutex }
#define SHARED_COMPATIBLE(name) (*name)
#endif

} // namespace _sa_internal

// This is a temporary, file-only macro which lets us include parameter
// names only if HAS_SHARED_PTR_ATOMICS is defined (in order to avoid
// "unused" warnings from compilers).
#ifdef HAS_SHARED_PTR_ATOMICS
#define _sa_internal_OPTIONAL_PARAM(type, name) type name
#else
#define _sa_internal_OPTIONAL_PARAM(type, name) type
#endif


namespace std {

  template<class T>
  bool atomic_is_lock_free( const shared_ptr<T>* p,
                            _sa_internal::lock&) {
#ifdef HAS_SHARED_PTR_ATOMICS
    return atomic_is_lock_free(p);
#else
    return false;
#endif
  }

  template<class T>
  shared_ptr<T> atomic_load_explicit( const shared_ptr<T>* p,
          _sa_internal_OPTIONAL_PARAM(memory_order, mo),
                                      _sa_internal::lock& lock) {
#ifdef HAS_SHARED_PTR_ATOMICS
    return atomic_load_explicit(p, mo);
#else
    lock_guard<_sa_internal::lock> lk(lock);
    return *p;
#endif
  }

  template<class T>
      void atomic_store_explicit( shared_ptr<T>* p,
                                  shared_ptr<T> r,
      _sa_internal_OPTIONAL_PARAM(memory_order, mo),
                                  _sa_internal::lock& lock) {
#ifdef HAS_SHARED_PTR_ATOMICS
    atomic_store_explicit(p, r, mo);
#else
    lock_guard<_sa_internal::lock> lk(lock);
    p->swap(r);
#endif
  }

  template<class T>
  shared_ptr<T> atomic_exchange_explicit( shared_ptr<T>* p,
                                          shared_ptr<T> r,
        _sa_internal_OPTIONAL_PARAM(memory_order, mo),
                                          _sa_internal::lock& lock) {
#ifdef HAS_SHARED_PTR_ATOMICS
    return atomic_exchange_explicit(p, r, mo);
#else
    lock.lock();
    p->swap(r);
    lock.unlock();
    return r;
#endif
  }


  template<class T>
  bool atomic_compare_exchange_strong_explicit( shared_ptr<T>* p,
                                                shared_ptr<T>* expected,
                                                shared_ptr<T> desired,
                    _sa_internal_OPTIONAL_PARAM(memory_order, success),
                    _sa_internal_OPTIONAL_PARAM(memory_order, failure),
                                                _sa_internal::lock& lock) {
#ifdef HAS_SHARED_PTR_ATOMICS
    return atomic_compare_exchange_strong_explicit(
        p, expected, desired, success, failure);
#else
    lock_guard<_sa_internal::lock> lk(lock);
    if (*p == *expected) {
      *p = move(desired);
      return true;
    }
    *expected = *p;
    return false;
#endif
  }


  template<class T>
  bool atomic_compare_exchange_weak_explicit( shared_ptr<T>* p,
                                              shared_ptr<T>* expected,
                                              shared_ptr<T> desired,
                                              memory_order success,
                                              memory_order failure,
                                              _sa_internal::lock& lock) {
#ifdef HAS_SHARED_PTR_ATOMICS
    return atomic_compare_exchange_weak_explicit(
        p, expected, desired, success, failure);
#else
    return atomic_compare_exchange_strong_explicit(
        p, expected, desired, success, failure, lock);
#endif
  }

/* TODO support defaults

template< class T >
std::shared_ptr<T> atomic_load( const std::shared_ptr<T>* p );



template< class T >
void atomic_store( std::shared_ptr<T>* p,
                   std::shared_ptr<T> r );

template< class T >
std::shared_ptr<T> atomic_exchange( std::shared_ptr<T>* p,
                                    std::shared_ptr<T> r);


template< class T >
bool atomic_compare_exchange_weak( std::shared_ptr<T>* p,
                                   std::shared_ptr<T>* expected,
                                   std::shared_ptr<T> desired);


template<class T>
bool atomic_compare_exchange_strong( std::shared_ptr<T>* p,
                                     std::shared_ptr<T>* expected,
                                     std::shared_ptr<T> desired);
*/

} // namespace std

#undef _sa_internal_OPTIONAL_PARAM

#endif /* UTILTIES_SHARED_ATOMIC_H_ */
