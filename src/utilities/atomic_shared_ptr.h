/*
 * Vladimir Feinberg
 * atomic_shared_ptr.h
 * 2014-06-22
 *
 * Declares and implements an atomic shared pointer class
 */

#ifndef ATOMIC_SHARED_PTR_H_
#define ATOMIC_SHARED_PTR_H_

#include <atomic>
#include <utility>

// Atomic pointer implements a basic shared pointer, containing data
// and a reference count. However, its interface is slightly different
// from that of the standard in that it can only construct in-place,
// rather than accepting a pointer. This is done for two reasons.
// (1) safety (no dangling references)
// (2) allows for construction of the reference count next to the
// data itself, so only one allocation is necessary. This also
// reduces pointer size.
//
// Both the shared pointer and the reference count are atomic, meaning
// that multiple threads can access shared or individual atomic_shared_ptr
// instances that themselves manage the same object.
//
// The class' atomic invariant is maintained by its pointer - that is,
// even if the count is not yet 0 according to some thread with a local
// copy of the pointer, another thread may read null.
// In other words, if any thread exchanges a pointer belonging to a
// shared pointer with a different one, then that pointer is guaranteed
// to have at least one reference count.
//
// Implementation is lock free and synchronized.
template<typename T>
class atomic_shared_ptr
{
 public:
  // Construct
  constexpr atomic_shared_ptr() noexcept
    : data_(nullptr) {}
  atomic_shared_ptr(std::nullptr_t) noexcept
    : data_(nullptr) {}
  template<class... Args>
    explicit atomic_shared_ptr(Args&&... args)
    : data_(new ref_counted(std::forward<Args>(args)...)) {}
  template<typename Y>
    atomic_shared_ptr(const atomic_shared_ptr<Y>& y) noexcept
    : atomic_shared_ptr() { *this = y; }
  template<typename Y>
    atomic_shared_ptr(atomic_shared_ptr<Y>&& y) noexcept
    : atomic_shared_ptr() { *this = std::forward<atomic_shared_ptr<Y> >(y); }
  // Assign
  template<typename Y>
    atomic_shared_ptr<T>& operator=(const atomic_shared_ptr<Y>&) noexcept;
  template<typename Y>
    atomic_shared_ptr<T>& operator=(atomic_shared_ptr<Y>&&) noexcept;
  atomic_shared_ptr<T>& operator=(std::nullptr_t) noexcept;
  // Modify
  template<class... Args>
    void reset(Args&&... args) noexcept;
  template<typename Y>
    void swap(atomic_shared_ptr<Y>& y) noexcept;
  // Observe
  T* get() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  long use_count() const noexcept;
  bool unique() const noexcept;
  explicit operator bool() const noexcept;

 private:
  struct value_type
  {
    template<class... Args>
      explicit value_type(Args&&... args)
    : value(std::forward<Args>(args)...) {}
    T value;
  };
  class ref_count
  {
  public:
    long up0() { return count_.load() ? 0 : ++count_; }
    long up() { return ++count_; }
    long down() { return --count_; }
    long count() const { return count_.load(); }
  private:
    std::atomic<long> count_;
  };
  class ref_counted : public ref_count, public value_type
  {
   public:
    template<class... Args>
      explicit ref_counted(Args&&... args)
      : ref_count(), value_type(std::forward<Args>(args)...) { up(); }
  };

  template<typename Y>
    static atomic_shared_ptr<T>::ref_counted*
    cast(Y* y)
    { return static_cast<atomic_shared_ptr<T>::ref_counted*>(y); }

  // down delete, up0 trim
  static bool up(ref_counted* ptr) { return ptr ? ptr->up() : false; }
  static bool down(ref_counted* ptr) { return ptr ? !ptr->down() : false; }
  static long count(const ref_counted* ptr) { return ptr ? ptr->count() : 0; }
  // requires ownership already
  bool up() { return up(data_.load()); }
  bool down() { return down(data_.load()); }
  long count() const { return count(data_.load()); }

  std::atomic<ref_counted*> data_;
};

// Assign
template<typename T>
template<typename Y>
atomic_shared_ptr<T>&
atomic_shared_ptr<T>::operator=(const atomic_shared_ptr<Y>& y) noexcept {
  // Spin until we get a claim of y's value or it is null
  ref_counted* next;
  do {
    next = cast(y.data_.load());
  } while (next && !next->up0());
  // Spin on replacement
  auto prev = data_.load();
  while (!data_.compare_exchange_weak(&prev, next));
  if (!down(prev))
    delete prev;
  return *this;
}

template<typename T>
template<typename Y>
atomic_shared_ptr<T>&
atomic_shared_ptr<T>::operator=(atomic_shared_ptr<Y>&& y) noexcept {
  assert(this != &y);
  auto next = cast(y.data_.exchange(nullptr));
  assert(!next || next->count());
  auto prev = data_.exchange(next);
  assert(!prev || prev->count());
  if (!down(prev))
    delete prev;
  return *this;
}

template<typename T>
atomic_shared_ptr<T>&
atomic_shared_ptr<T>::operator=(std::nullptr_t) noexcept {
  auto prev = data_.exchange(nullptr);
  assert(!prev || prev->count());
  if (!down(prev))
    delete prev;
  return *this;
}
// Modify
template<typename T>
template<class... Args>
void
atomic_shared_ptr<T>::reset(Args&&... args) noexcept {
  // TODO faster manually inlined implementation
  atomic_shared_ptr<T> tmp(std::forward<Args>(args)...);
  *this = std::move(tmp);
}

template<typename T>
template<class Y>
void
atomic_shared_ptr<T>::swap(atomic_shared_ptr<Y>& y) noexcept {
  // TODO faster manually inlined implementation
  std::swap(*this, y);
}

// Observe
template<typename T>
T*
atomic_shared_ptr<T>::get() const noexcept {
  auto ptr = data_.load();
  return ptr ? &ptr->value : nullptr;
}

template<typename T>
T&
atomic_shared_ptr<T>::operator*() const noexcept {
  return *get();
}

template<typename T>
T*
atomic_shared_ptr<T>::operator->() const noexcept {
  return get();
}

template<typename T>
long
atomic_shared_ptr<T>::use_count() const noexcept {
  return count(data_.load());
}

template<typename T>
bool
atomic_shared_ptr<T>::unique() const noexcept {
  return use_count() == 1;
}

template<typename T>
atomic_shared_ptr<T>::operator bool() const noexcept {
  return get();
}

// Non-member associated functions

template<class T, class U>
atomic_shared_ptr<T>
static_pointer_cast(const atomic_shared_ptr<U>& u) noexcept {
  return u;
}

template<class T, class U>
atomic_shared_ptr<T>
dynamic_pointer_cast(const atomic_shared_ptr<U>& u) noexcept {
  return atomic_shared_ptr<T>(dynamic_cast<T>(u.get()));
}

template<class T, class U>
atomic_shared_ptr<T>
const_pointer_cast(const atomic_shared_ptr<U>& u) noexcept {
  return atomic_shared_ptr<T>(const_cast<T>(u.get()));
}

template<class T>
std::ostream&
operator<<(std::ostream& o, const atomic_shared_ptr<T>& ptr) noexcept {
  o << ptr.get();
  return o;
}

namespace std {
  template<class T>
    struct hash<atomic_shared_ptr<T> > {
    size_t operator()(const atomic_shared_ptr<T>& ptr) {
      static const hash<T*> hashf;
      return hashf(ptr.get());
    }
  };
}

/*
template<class T>
bool atomic_is_lock_free(const atomic_shared_ptr<T>* p );
template<class T>
atomic_shared_ptr<T> atomic_load(const atomic_shared_ptr<T>* p );
template<class T>
atomic_shared_ptr<T> atomic_load_explicit(const shared_ptr<T>* p,
                                          std::memory_order mo );
template<class T>
void atomic_store(atomic_shared_ptr<T>* p,
                  atomic_shared_ptr<T> r );
template<class T>
void atomic_store_explicit(atomic_shared_ptr<T>* p,
                           shared_ptr<T> r,
                           std::memory_order mo);
template<class T>
atomic_shared_ptr<T> atomic_exchange(atomic_shared_ptr<T>* p,
                                     atomic_shared_ptr<T> r);
template<class T>
atomic_shared_ptr<T> atomic_exchange_explicit(atomic_shared_ptr<T>* p,
                                              atomic_shared_ptr<T> r,
                                              std::memory_order mo);
template<class T>
bool atomic_compare_exchange_weak(atomic_shared_ptr<T>* p,
                                  atomic_shared_ptr<T>* expected,
                                  atomic_shared_ptr<T> desired);
template<class T>
bool atomic_compare_exchange_strong(atomic_shared_ptr<T>* p,
                                    atomic_shared_ptr<T>* expected,
                                    atomic_shared_ptr<T> desired);
template<class T>
bool atomic_compare_exchange_strong_explicit(atomic_shared_ptr<T>* p,
                                             atomic_shared_ptr<T>* expected,
                                             atomic_shared_ptr<T> desired,
                                             std::memory_order success,
                                             std::memory_order failure);
template<class T>
bool atomic_compare_exchange_weak_explicit(atomic_shared_ptr<T>* p,
                                           atomic_shared_ptr<T>* expected,
                                           atomic_shared_ptr<T> desired,
                                           std::memory_order success,
                                           std::memory_order failure);
*/

#endif /* ATOMIC_SHARED_PTR_H_ */
