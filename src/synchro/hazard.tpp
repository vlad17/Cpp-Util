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
  hazard_record();

  // Returns a fresh non-null pointer to an activated record.
  static hazard_record* activated_record();
  // Marks pointer for deletion. Frees memory occasionally by scanning for
  // safe pointers to delete (not necessarily this one).
  typedef void (*deleter_t)(void*);
  static void schedule_deletion(void* ptr, deleter_t deleter);
  // Nullifies the protected pointer, then deactivates. No deletion scheduled.
  void deactivate(); // null, then deactivate.
  void publish(void* p) { protected_ptr_.store(p, std::memory_order_relaxed); }
  void* protected_ptr();
  hazard_record* next() const;

 private:
  bool active() const;
  bool capture();

  std::atomic<bool> active_;
  std::atomic<void*> protected_ptr_;
  hazard_record* next_;
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
  return *this;
}

template<typename T>
void hazard_ptr<T>::acquire(T* ptr) {
  record_->publish(ptr);
  ptr_ = ptr;
}

template<typename T>
void hazard_ptr<T>::acquire(const std::atomic<T*>& ptr) {
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
  _synchro_hazard_internal::hazard_record::schedule_deletion(ptr, ptr_deleter);
}

} // namespace synchro

#endif /* SYNCHRO_HAZARD_TPP_ */
