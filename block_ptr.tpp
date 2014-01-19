/*
* Vladimir Feinberg
* block_ptr.tpp
* 2014-01-18
*
* Contains implementation of some of the methods and nested class methods
* for weak_block_ptr
*/

template<typename T>
template<typename... Args>
mempool::weak_block_ptr<T>::optional::optional(Args&&... args) :
	initialized(false)
{
	this->construct(std::forward<Args>(args)...);
}

template<typename T>
mempool::weak_block_ptr<T>::optional::optional(optional&& other) :
	initialized(other.initialized)
{
	if(initialized)
		new (get()) T(std::move(*other.get()));
}

template<typename T>
auto mempool::weak_block_ptr<T>::optional::operator=(optional&& other) -> optional&
{
	if(other.initialized)
	{
		if(initialized) destruct();
		new (get()) T(std::move(*other.get()));
		initialized = true;
	}
	return *this;
}

template<typename T>
template<typename... Args>
void mempool::weak_block_ptr<T>::optional::construct(Args&&... args)
{
	assert(!initialized);
	new (get()) T(std::forward<Args>(args)...);
	initialized = true;
}

template<typename T>
void mempool::weak_block_ptr<T>::optional::destruct()
{
	assert(initialized);
	(*get()).~T();
	initialized = false;
}

// need not be initialized
template<typename T>
T *mempool::weak_block_ptr<T>::optional::get()
{
	return static_cast<T*>(static_cast<void*>(store));
}

// need not be initialized
template<typename T>
const T *mempool::weak_block_ptr<T>::optional::get() const
{
	return static_cast<const T*>(static_cast<const void*>(store));
}

template<typename T>
bool mempool::weak_block_ptr<T>::optional::valid() const
{
	return initialized;
}

template<typename T>
mempool::weak_block_ptr<T>::optional::~optional()
{
	if(initialized)
		get()->~T();
}

template<typename T>
template<typename... Args>
auto mempool::weak_block_ptr<T>::fixed_allocator::construct(Args&&... args) -> index_t
{
	index_t i;
	if(freelist.empty())
	{
		i = store.size();
		store.emplace_back(std::forward<Args>(args)...);
		return i;
	}
	i = freelist.front();
	store[i].construct(std::forward<Args>(args)...);
	freelist.pop();
	return i;
}

template<typename T>
void mempool::weak_block_ptr<T>::fixed_allocator::destruct(index_t i)
{
	store[i].destruct();
	freelist.push(i);
}

template<typename T>
T& mempool::weak_block_ptr<T>::fixed_allocator::operator[](index_t i)
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
const T& mempool::weak_block_ptr<T>::fixed_allocator::operator[](index_t i) const
{
	assert(i < store.size());
	assert(store[i].valid());
	return *store[i].get();
}

template<typename T>
std::vector<typename mempool::weak_block_ptr<T>::fixed_allocator>
mempool::weak_block_ptr<T>::fallocators {};

template<typename T>
auto mempool::weak_block_ptr<T>::generate_allocator() -> _allocator
{
	fallocators.emplace_back();
	return fallocators.size()-1;
}

template<typename T>
auto mempool::weak_block_ptr<T>::default_allocator() -> _allocator
{
	if(fallocators.empty()) generate_allocator();
	return 0;
}