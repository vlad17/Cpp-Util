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
auto mempool::weak_block_ptr<T>::fixed_allocator::construct(Args&&... args) -> index_t
{
	index_t i;
	if(freelist.empty())
	{
		i = store.size();
		store.emplace_back();
		store.back().construct(std::forward<Args>(args)...);
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