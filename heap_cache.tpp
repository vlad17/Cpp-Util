/*
* Vladimir Feinberg
* 2014-05-27
* heap_cache.tpp
*
* Contains implementation of lfu_cache.h's heap_cache methods.
*/

template<typename K, typename V, typename P, typename H, typename S>
auto lfu::heap_cache<K,V,P,H,S>::operator=(const heap_cache& other) -> heap_cache&
{
	if(this == &other) return *this;
	clear();
	if(other.empty()) return *this;
	max_size = other.max_size;
	heap = other.heap;
	keymap = other.keymap;
	return *this;
}

template<typename K, typename V, typename P, typename H, typename S>
auto lfu::heap_cache<K,V,P,H,S>::operator=(heap_cache&& other) -> heap_cache&
{
	assert(this != &other);
	clear();
	max_size = other.max_size;
	heap = std::move(other.heap);
	keymap = std::move(other.keymap);
	return *this;
}

template<typename K, typename V, typename P, typename H, typename S>
bool lfu::heap_cache<K,V,P,H,S>::insert(const kv_type& kv)
{
	_consistency_check();
	if(max_size == 0) return false;
	auto itpair = keymap.emplace(std::piecewise_construct,
      std::tuple<key_type>(kv.first),
      std::tuple<value_type, size_t>(kv.second, heap.size()));
	if(!itpair.second) return false;
	auto it = itpair.first;
	citem& item = it->second;
	if(keymap.size() == max_size)
	{
		_del_back_full();
		item.loc = heap.size();
	}
	heap.push_back(kv.first);
	return true;
}

template<typename K, typename V, typename P, typename H, typename S>
bool lfu::heap_cache<K,V,P,H,S>::insert(kv_type&& kv)
{
	_consistency_check();
	if(max_size == 0) return false;
	auto itpair = keymap.emplace(std::piecewise_construct,
      std::forward_as_tuple<key_type>(std::forward<key_type>(kv.first)),
      std::forward_as_tuple<value_type, size_t>(std::forward<value_type>(kv.second),
      		heap.size()));
	if(!itpair.second) return false;
	auto it = itpair.first;
	citem& item = it->second;
	if(keymap.size() == max_size)
	{
		_del_back_full();
		item.loc = heap.size();
	}
	heap.push_back(it->first);
	return true;
}

template<typename K, typename V, typename P, typename H, typename S>
auto lfu::heap_cache<K,V,P,H,S>::lookup(key_cref key) const -> value_type*
{
	_consistency_check();
	auto it = keymap.find(key);
	if(it == keymap.end()) return nullptr;
	++it->second.count;
	increase_key(it->first);
	return &it->second.val;
}

template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::clear()
{
	_consistency_check();
	heap.clear();
	keymap.clear();
	heap.push_back(key_type{});
}

template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::set_max_size(size_t max)
{
	_consistency_check();
	max_size = max;
	if(max < heap.size()-1)
		while(heap.size() != max) // note +1 index, stop at max.
			_del_back();
}

// ---- helper methods

template<typename K, typename V, typename P,
	typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::_del_back()
{
	assert(heap.size() > 1);
	keymap.erase(heap.back());
	heap.pop_back();
}

template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::_del_back_full()
{
	// below 4 it's not worth it
	if(max_size <= 4) _del_back();
	else
		for(size_t i = static_cast<size_t>(max_size*REFRESH_RATIO); i < max_size; ++i)
			_del_back();
}

// swaps increased key until heap property is restored, returns
template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::increase_key(key_cref k) const
{
	citem& c = keymap.at(k);
	assert(c.loc < heap.size());
	assert(c.loc > 0);
	while(c.loc > 1)
	{
		citem& parent = keymap.at(heap[c.loc/2]);
		if(parent.count >= c.count) break;
		std::swap(heap[parent.loc], heap[c.loc]);
		std::swap(parent.loc, c.loc);
	}
}

template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::_consistency_check() const
{
	assert(heap.size() >= 1);
	assert(max_size >= heap.size()-1);
	assert(max_size >= keymap.size());
	assert(keymap.size() + 1 == heap.size());
	assert(base_type::key_predicate(heap[0], key_type{}));
#ifdef HCACHE_CHECK
	for(size_t i = keymap.size(); i > 1; --i)
	{
		assert(heap[i] != nullptr);
		assert(heap[i/2] != nullptr);
		assert(heap[i]->count <= heap[i/2]->count);
	}
#endif
}

template<typename K, typename V, typename P, typename H, typename S>
void lfu::heap_cache<K,V,P,H,S>::_print_cache(std::ostream& o) const
{
	_consistency_check();
	base_type::_print_cache(o);
	if(empty()) return;
	for(size_t i = 0; i < (size_t) (log(keymap.size()-1)/log(2))+1; ++i)
	{
		for(size_t j = pow(2, i); j <= pow(2, i+1)-1; ++j)
		{
			if(j > keymap.size()) break;
			o << '(' << heap[j] << "->" << keymap.at(heap[j]).val << ',';
			o << keymap.at(heap[j]).count << ") ";
		}
		o << '\n';
	}
}


template<typename K, typename V, typename P, typename H, typename S>
typename lfu::heap_cache<K,V,P,H,S>::hasher lfu::heap_cache<K,V,P,H,S>::hashf {};
