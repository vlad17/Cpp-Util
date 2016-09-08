/*
  Vladimir Feinberg
  2014-09-08
  synchro/atomic_shared.hpp

  Atomic operations on shared pointers themselves are unfortuantely not
  supported by libstdc++, even though they are by libc++. This header
  provides a temporary compatibility resolution for the problem, which
  can safely be used in a libstd=libc++ enviornment because it decays
  to the defualt behavior (using template magic, of course).

  After this file is included, the atomic_shared_ptr will support all
  standard shared_ptr operations, both the nonatomic member functions
  and the atomic overloads that are declared in this header.
*/

#ifndef SYNCHRO_ATOMIC_SHARED_HPP_
#define SYNCHRO_ATOMIC_SHARED_HPP_

#include <memory>

#include "synchro/atomic_shared.tpp"

namespace synchro {

// Use this class just like a shared_ptr<T>. Obviously, shared_ptr<T> doesn't
// have a virtual destructor, so don't rely on a polymorphic delete!
// Don't rely on the fact that an atomic_shared_ptr extends shared_ptr at all.
// TODO implement this with composition eventually...
template<class T>
using atomic_shared_ptr = _synchro_atomic_shared_internal::selected<T>;

} // namespace synchro

// Note there are no operations which work with two different
// atomic_shared_ptrs - this could easily cause deadlock if you're not
// careful anyway.
template<class T>
bool std::atomic_is_lock_free(const synchro::atomic_shared_ptr<T>* p);
template<class T>
std::shared_ptr<T> std::atomic_load_explicit(
    const synchro::atomic_shared_ptr<T>* p, std::memory_order mo);
template<class T>
void std::atomic_store_explicit(
    synchro::atomic_shared_ptr<T>* p, std::shared_ptr<T> r,
    std::memory_order mo);
template<class T>
std::shared_ptr<T> std::atomic_exchange_explicit(
    synchro::atomic_shared_ptr<T>* p, std::shared_ptr<T> r,
    std::memory_order mo);
template<class T>
bool std::atomic_compare_exchange_strong_explicit(
    synchro::atomic_shared_ptr<T>* p, std::shared_ptr<T>* expected,
    std::shared_ptr<T> desired, std::memory_order success,
    std::memory_order failure);
template<class T>
bool std::atomic_compare_exchange_weak_explicit(
    synchro::atomic_shared_ptr<T>* p, std::shared_ptr<T>* expected,
    std::shared_ptr<T> desired, std::memory_order success,
    std::memory_order failure);

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

#endif /* SYNCHRO_ATOMIC_SHARED_HPP_*/
