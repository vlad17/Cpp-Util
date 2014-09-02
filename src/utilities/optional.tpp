/*
* Vladimir Feinberg
* impl_util.tpp
* 2014-01-18
*
* Implements impl_util type methods.
*/

#include <cassert>

template<typename T>
util::optional<T>::optional() :
  initialized(false) {}

template<typename T>
util::optional<T>::optional(T&& rval) :
  optional() {
  construct(std::forward<T>(rval));
}

template<typename T>
util::optional<T>::optional(optional&& other) :
  optional() {
  *this = std::forward<optional>(other);
}

template<typename T>
auto util::optional<T>::operator=(optional&& other) -> optional& {
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
auto util::optional<T>::operator=(const optional& other) -> optional& {
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
void util::optional<T>::construct(Args&&... args)
{
  assert(!initialized);
  new (get()) T(std::forward<Args>(args)...);
  initialized = true;
}

template<typename T>
void util::optional<T>::destruct()
{
  assert(initialized);
  get()->~T();
  initialized = false;
}

template<typename T>
const T* util::optional<T>::get() const {
  return reinterpret_cast<const T*>(store);
}

template<typename T>
T* util::optional<T>::get() {
  return reinterpret_cast<T*>(store);
}

template<typename T>
T& util::optional<T>::access() {
  return *get();
}

template<typename T>
const T& util::optional<T>::access() const {
  return *get();
}

template<typename T>
bool util::optional<T>::valid() const
{
  return initialized;
}

template<typename T>
util::optional<T>::~optional()
{
  if(initialized)
    get()->~T();
}
