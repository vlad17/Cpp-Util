/*
  Vladimir Feinberg
  synchro/hazard.hpp
  2015-01-28

  Hazard pointer interface. Allows implementation of lockfree data structures.
  In particular, hazard pointers enable their users to mark data as "in-use."
  Which in turn prevents any deletions scheduled by a proxy method in the class
  to occur.

  Implementation based on the following link:
  http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890

  It is important to note that hazard_ptrs are to be used for managing
  lifetimes. They don't protect concurrent access to data pointed-to by the
  pointers they protect. This INCLUDES the synchronization of previous writes
  made by a thread with a hazard_ptr. The hazard_ptr does NOT establish
  any happens-before relation to cross-thread writes and reads to the
  pointed-to data.
*/

#ifndef SYNCHRO_HAZARD_HPP_
#define SYNCHRO_HAZARD_HPP_

#include <atomic>

namespace synchro {

namespace _synchro_hazard_internal {
  class hazard_record;
} // namespace _synchro_hazard_internal

// hazard_ptr itself is NOT thread safe. However, it may be used to ensure that
// protected ("acquired") pointers are not deleted, so long as all deletions
// occur through a call to schedule_deletion().
//
// Note hazard_ptr provides a type-safe interface, even though it doesn't
// really need to (you can just protect void*). I haven't really had a need
// to offer multi-type hazard_ptrs yet, so the additional type safety
// shouldn't be an issue.
template<typename T>
class hazard_ptr {
 public:
  hazard_ptr();
  ~hazard_ptr();

  hazard_ptr(hazard_ptr&&);
  hazard_ptr& operator=(hazard_ptr&&);
  // Implicitly-deleted copy constructor and assignment operator.

  // After this call completes, it is safe to use ptr. HOWEVER,
  // ptr may have been already deleted before this call completed.
  // It must be checked that the acquired pointer is indeed still valid.
  // If attempting to acquire an atomic pointer, use the atomic method,
  // which checks for validity after the call.
  //
  // If 'ptr' was valid after the call to 'acquire' (or the second method
  // was used), then the 'ptr' is guaranteed valid for the duration of the
  // hazard_ptr's lifetime (or until a different pointer is acquired; a
  // hazard_ptr only protects one pointer at a time).
  void acquire(T* ptr);
  void acquire(const std::atomic<T*>& ptr);

  void reset() { acquire(nullptr); }

  // Pointer-getters.
  T* get() const { return ptr_; }
  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  explicit operator bool() const { return ptr_; }

  // Should only be called once. hazard_pointer users should
  // make sure that this is called once by only allowing a deletion
  // call to be scheduled by some writing thread to the data structure,
  // which is able to atomically unlink this pointer from it, ensuring
  // the exactly-once requirement.
  //
  // 'ptr' need not be owned by any hazard pointers, and can even be
  // used after scheduling deletion, so long as it's protected by a
  // hazard_ptr instance.
  //
  // The contract for usage of schedule_deletion() ALSO includes the
  // responsibility that after it is called, no readers subsequently
  // acquire new hazard pointers for the parameter pointer. If an
  // atomic 'unlinking' of the type above occured, this condition is
  // already met automatically.
  static void schedule_deletion(T* ptr);

 private:
  T* ptr_; // local non-atomic copy.
  _synchro_hazard_internal::hazard_record* record_;

  // A hack to allow for type-agnostic separate compilation.
  static void ptr_deleter(void* ptr) { delete static_cast<T*>(ptr); }
};

} // namespace synchro

#include "synchro/hazard.tpp"

#endif /* SYNCHRO_HAZARD_HPP_ */
