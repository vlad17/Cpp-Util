/*
  Vladimir Feinberg
  synchro/hazard.tpp
  2015-01-28

  Template implementation file for hazard pointers.
*/

#ifndef SYNCHRO_HAZARD_TPP_
#define SYNCHRO_HAZARD_TPP_

namespace synchro {

namespace _synchro_hazard_internal {

// Type-agnostic hazard record. This contains the flag that protects the
// record's pointer from deletion.
class hazard_record {
 public:

  // Everything but publish into cpp... TODO
  static hazard_record* activated_record(); // returns fresh activated nonnull
  static void schedule_deletion(void* ptr); // retires and scans.
  void deactivate(); // null, then deactivate.
  void publish(void* p) { protected_ptr_.store(p, std::memory_order_relaxed); }

 private:
  std::atomic<bool> active_;
  std::atomic<void*> protected_ptr_;
  std::atomic<hazard_record*> next_;

  // TODO move below to cpp fil
  //This file maintains the static
  // executable-wide hazard pointer list and thread-local retired lists.
  //
  // There is some justification for the use of a static list like this.
  // It's already thread-safe, especially since C++11 blesses multi-threaded
  // static initialization. Furthermore, there are no issues with loading this
  // as a dynamic library; the loaded file will just maintain its own list
  // until unlink time.
  struct hazard_list {
    ~hazard_list();
    hazard_record head_;
  };
  static hazard_list global_list_;
};

} // namespace _synchro_hazard_internal

template<typename T>
hazard_ptr<T>::hazard_ptr() :
    ptr_(nullptr),
    record_(_synchro_hazard_internal::hazard_record::activated_record()) {}

template<typename T>
hazard_ptr<T>::~hazard_ptr() { if (record_) record_->deactivate(); }

template<typename T>
hazard_ptr<T>::hazard_ptr(hazard_ptr&& other) :
    ptr_(other.ptr_), record_(other.record_) {
  other.ptr_ = nullptr;
  other.record_ = nullptr;
  // Only moved instances have record_ == nullptr. Note none of the
  // methods support this condition except the destructor. This is
  // because moved instances only need to support self-destruction
  // and assignment (other usage is an error).
}

template<typename T>
hazard_ptr<T>& hazard_ptr<T>::operator=(hazard_ptr&& other) {
  if (record_) record_->deactivate();
  ptr_ = other.ptr_;
  record_ = other.record_;
  other.ptr_ = nullptr;
  other.record_ = nullptr;
}

template<typename T>
void hazard_ptr<T>::acquire(T* ptr) {
  record_->publish(ptr);
  ptr_ = ptr;
}

template<typename T>
void hazard_ptr<T>::acquire(std::atomic<T*> ptr) {
  T* oldval, newval = ptr.load(std::memory_order_relaxed);
  do {
    oldval = newval;
    record_->publish(oldval);
    newval = ptr.load(std::memory_order_relaxed);
  } while (newval != oldval);
  ptr_ = newval;
}

template<typename T>
void hazard_ptr<T>::schedule_deletion(T* ptr) {
  _synchro_hazard_internal::hazard_ptr::schedule_deletion(ptr);
}

} // namespace synchro

#endif /* SYNCHRO_HAZARD_TPP_ */
