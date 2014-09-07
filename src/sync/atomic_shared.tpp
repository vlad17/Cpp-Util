/*
  Vladimir Feinberg
  2014-08-24
  atomic_shared.tpp

  Implementation of atomic_shared.h's promises for atomic ops on a
  std::shared_ptr.
*/

#include <atomic>
#include <mutex>
#include <type_traits>

namespace _sync_atomic_shared_internal {

// Overload in this namespace while waiting for C++14...
template<class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

typedef std::mutex lock_type;
typedef std::lock_guard<lock_type> lock_guard;

template<class T>
class locked_shared_ptr : public std::shared_ptr<T> {
 public:
  typedef std::shared_ptr<T> base;
  // TODO eventually add shared_ptr constructors.
  lock_type& lock() const { return *lock_; }
  // No locked_shared_ptr-to-locked_shared_ptr copies
  // for now. Assignment, etc. may be extended in the future
  // with a composition-base implementation (TODO).
 private:
  // maintain move-ability between locked_shared_ptrs.
  std::unique_ptr<lock_type> lock_ = make_unique<lock_type>();
};

// Here's where the magic happens: we use SFINAE with a function
// that returns a decltype that has an overloaded fallback function.
// Technique was originally learned on StackOverflow, like everything.
// If the function does not exist, then the decltype below will cause a
// substitution failure.
template<class T>
constexpr auto has_atomic_overloads_f(T* t) -> decltype(
    std::atomic_compare_exchange_weak(t, t, *t)) {
    return true;
}

// Fallback in case of the decltype failing for SFINAE, indicates there is
// no atomic overload for shared_ptr. Fallback relies on va_args that we
// don't care about. Since it's va_args we're guaranteed that the
// function overload above with one parameter will be first in the compiler's
// substitution attempts.
constexpr bool has_atomic_overloads_f(...) { return false; }

// Struct which lets us conveniently extract the constant (as you can see from
// the massaging we need to do with the casts, we can't use this function
// directly when instantiating templates like we could with this class
// by just using has_atomic_overloads<T>::value.
template<class T>
struct has_atomic_overloads {
    static const T t;
    static constexpr bool value = has_atomic_overloads_f(const_cast<T*>(&t));
};

// Selects the shared pointer if library supports atomic shared pointer
// methods, otherwise defaults to the locked one defined above.
template<class T, bool ATOMIC_SUPPORTED>
using selector = typename std::conditional<ATOMIC_SUPPORTED, std::shared_ptr<T>,
                                           locked_shared_ptr<T> >::type;

// The final class that's meant to be used externally
template<class T>
using selected = selector<T, has_atomic_overloads<std::shared_ptr<T> >::value>;

} // namespace _sync_atomic_shared_internal

namespace std {
  template<class T>
  bool atomic_is_lock_free(
      const _sync_atomic_shared_internal::locked_shared_ptr<T>* p) {
    return false;
  }

  template<class T>
  shared_ptr<T> atomic_load_explicit(
      const _sync_atomic_shared_internal::locked_shared_ptr<T>* p,
      memory_order) {
    _sync_atomic_shared_internal::lock_guard lk(p->lock());
    return *p;
  }

  template<class T>
  void atomic_store_explicit(
      _sync_atomic_shared_internal::locked_shared_ptr<T>* p,
      shared_ptr<T> r, memory_order) {
    _sync_atomic_shared_internal::lock_guard lk(p->lock());
    // (**) Careful - *p = move(r) translates as
    // *p = locked_shared_ptr(move(r)), which gives
    // p a new mutex!
    p->swap(r);
  }

  template<class T>
  shared_ptr<T> atomic_exchange_explicit(
      _sync_atomic_shared_internal::locked_shared_ptr<T>* p,
      shared_ptr<T> r, memory_order) {
    p->lock().lock();
    p->swap(r); // see (**) above
    p->lock().unlock();
    return r;
  }

  template<class T>
  bool atomic_compare_exchange_strong_explicit(
      _sync_atomic_shared_internal::locked_shared_ptr<T>* p,
      shared_ptr<T>* expected, shared_ptr<T> desired,
      memory_order, memory_order) {
    _sync_atomic_shared_internal::lock_guard lk(p->lock());
    if (*p == *expected) {
      p->swap(desired); // see (**) above
      return true;
    }
    *expected = *p;
    return false;
  }

  template<class T>
  bool atomic_compare_exchange_weak_explicit(
      _sync_atomic_shared_internal::locked_shared_ptr<T>* p,
      shared_ptr<T>* expected, shared_ptr<T> desired,
      memory_order success, memory_order failure) {
    return atomic_compare_exchange_strong_explicit(
      p, expected, move(desired), success, failure);
  }

} // namespace std
