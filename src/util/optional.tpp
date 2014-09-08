/*
* Vladimir Feinberg
* util/optional.tpp
* 2014-09-08
*
* Implements optional.
*/

#include <cassert>

namespace util {

template<typename T>
optional<T>::optional() :
    initialized(false) {}

template<typename T>
optional<T>::optional(T&& rval) :
    optional() {
  construct(std::forward<T>(rval));
}

template<typename T>
optional<T>::optional(optional&& other) :
    optional() {
  *this = std::forward<optional>(other);
}

template<typename T>
auto optional<T>::operator=(optional&& other) -> optional& {
  if (other.initialized) {
    if (initialized)
      access() = std::move(other.access());
    else construct(std::move(other.access()));
    // Note other.access() is still alive, just in a "moved" temporary
    // state.
  } else if (initialized)
    destruct();
  return *this;
}

template<typename T>
auto optional<T>::operator=(const optional& other) -> optional& {
  if (other.initialized) {
    if (initialized)
      access() = other.access();
    else
      construct(other.access());
  } else if (initialized)
      destruct();
  return *this;
}

template<typename T>
template<typename... Args>
void optional<T>::construct(Args&&... args) {
  assert(!initialized);
  new (get()) T(std::forward<Args>(args)...);
  initialized = true;
}

template<typename T>
void optional<T>::destruct() {
  assert(initialized);
  get()->~T();
  initialized = false;
}

template<typename T>
const T* optional<T>::get() const {
  return reinterpret_cast<const T*>(store);
}

template<typename T>
T* optional<T>::get() {
  return reinterpret_cast<T*>(store);
}

template<typename T>
T& optional<T>::access() {
  return *get();
}

template<typename T>
const T& optional<T>::access() const {
  return *get();
}

template<typename T>
bool optional<T>::valid() const {
  return initialized;
}

template<typename T>
optional<T>::~optional() {
  if(initialized)
    get()->~T();
}

} // namespace util
