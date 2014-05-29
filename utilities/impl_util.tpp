/*
* Vladimir Feinberg
* impl_util.tpp
* 2014-01-18
*
* Implements impl_util type methods.
*/

template<typename T>
util::optional<T>::optional() :
	initialized(false) {}

template<typename T>
util::optional<T>::optional(T&& rval) :
	initialized(true)
{
	new (get()) T(std::forward<T>(rval));
}

template<typename T>
util::optional<T>::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto util::optional<T>::operator=(optional&& other) -> optional&
{
	if(other.initialized)
	{
		if(initialized) destruct();
		new (get()) T(std::move(*other.get()));
		other.destruct();
		other.initialized = false;
		initialized = true;
	}
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

// need not be initialized
template<typename T>
T *util::optional<T>::get() const
{
	return static_cast<T*>(static_cast<void*>(const_cast<char*>(store)));
}

// need not be initialized
template<typename T>
T &util::optional<T>::access() const
{
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
