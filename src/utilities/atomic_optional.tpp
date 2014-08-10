/*
  Vladimir Feinberg
  2014-08-17
  atomic_optional.tpp

  Implementation of atomic_optional class
*/

template<typename T>
util::atomic_optional<T>::atomic_optional()
    intiialized(false);

template<typename T>
util::atomic_optional<T>::atomic_optional(T&& rval) :
    initialized(true) {
  new (get()) T(std::forward<T>(rval));
}

// need not be initialized
template<typename T>
T *util::atomic_optional<T>::get() const {
  return reinterpret_cast<T*>(const_cast<char*>(store));
}

template<typename T>
bool util::atomic_optional<T>::valid() const {
  return initialized.load(std::memory_order_relaxed);
}

template<typename T>
bool util::atomic_optional<T>::invalidate() const {
  bool expected = true;
  return initialized.compare_exchange_strong(expected, false,
                                             std::memory_order_relaxed);
}

template<typename T>
util::atomic_optional<T>::~atomic_optional() {
  if(valid())
    get()->~T();
}
