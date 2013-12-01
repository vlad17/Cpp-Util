/*
* Vladimir Feinberg
* 2013-12-01
* heap_cache.tpp
*
* Contains implementation of lfu_cache.h's heap_cache methods.
*/

template<typename K, typename V, typename P, typename H>
lfu::heap_cache<K,V,P,H>::~heap_cache()
{	
	for(auto i : heap) delete i;
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::operator=(const heap_cache& other) -> heap_cache&
{
	if(this == &other) return *this;
	clear();
	if(other.empty()) return *this;
	max_size = other.max_size;
	heap.reserve(other.heap.size());
	for(auto it = ++other.heap.begin(); it != other.heap.end(); ++it)
	{
		auto i = *it;
		heap.push_back(new citem(i->kvpair(), i->loc));
		heap.back()->count = i->count;
		keymap.insert(std::make_pair(heap.back()->key, heap.back()));
	}
	return *this;
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::operator=(heap_cache&& other) -> heap_cache&
{
	assert(this != &other);
	clear();
	max_size = other.max_size;
	heap = std::move(other.heap);
	keymap = std::move(other.keymap);
	return *this;
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::empty() const
{
	return keymap.empty();
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::size() const -> size_type
{
	return keymap.size();
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::insert(const kv_type& kv)
{
	_consistency_check();
	if(max_size == 0) return false;
	citem *valp = new citem(kv, heap.size());
	citem *& mapped = keymap[valp->key];
	if(mapped == nullptr)
	{
		bool retval = true;
		if(keymap.size() == max_size)
		{
			_del_back_full();
			valp->loc = heap.size();
			retval = false;
		}
		keymap[valp->key] = valp;
		heap.push_back(valp);
		return retval;
	}
	heap[mapped->loc] = valp;
	valp->loc = mapped->loc;
	delete mapped;
	mapped = valp;
	return false;
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::insert(kv_type&& kv)
{
	_consistency_check();
	if(max_size == 0) return false;
	citem *valp = new citem(std::forward<kv_type>(kv), heap.size());
	citem *&mapped = keymap[valp->key];
	if(mapped == nullptr)
	{
		bool retval = true;
		if(keymap.size() == max_size)
		{
			_del_back_full();
			valp->loc = heap.size();
			retval = false;
		}
		keymap[valp->key] = valp;
		heap.push_back(valp);
		return retval;
	}
	heap[mapped->loc] = valp;
	valp->loc = mapped->loc;
	delete mapped;
	mapped = valp;
	return false;
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::contains(const key_type& key) const
{
	return keymap.find(key) != keymap.end();
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::lookup(const key_type& key) const -> value_type*
{
	_consistency_check();
	auto access = keymap.find(key);
	if(access == keymap.end()) return nullptr;
	++access->second->count;
	increase_key(access->second);
	return &access->second->val;
}

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::clear()
{
	_consistency_check();
	for(auto i : heap) delete i;
	heap.clear();
	keymap.clear();
	heap.push_back(nullptr);
}

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::set_max_size(size_type max)
{
	_consistency_check();
	max_size = max;
	if(max < heap.size()-1)
		while(heap.size() != max) // note +1 index, stop at max.
			_del_back();
}

// ---- helper methods

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::_del_back()
{
	assert(heap.size() > 1);
	keymap.erase(heap.back()->key);
	delete heap.back();
	heap.pop_back();
}

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::_del_back_full()
{
	// below 4 it's not worth it
	if(max_size <= 4) _del_back();
	else
		for(size_type i = max_size>>1; i < max_size; ++i)
			_del_back();
}

// swaps increased key until heap property is restored, returns
template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::increase_key(citem *c) const
{
	assert(c != nullptr);
	assert(c->loc < heap.size());
	assert(c->loc > 0);
	if(c->loc == 1) return;
	citem *parent = heap[c->loc/2];
	assert(parent != nullptr);
	if(parent->count < c->count)
	{
		heap[parent->loc] = c;
		heap[c->loc] = parent;
		parent->loc = c->loc;
		c->loc/=2;
		increase_key(c);
	}
}

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::_consistency_check() const
{
	assert(heap.size() >= 1);
	assert(max_size >= heap.size()-1);
	assert(max_size >= keymap.size());
	assert(keymap.size() + 1 == heap.size());
	assert(heap[0] == nullptr);
#ifdef HCACHE_CHECK
	for(size_type i = keymap.size(); i > 1; --i)
	{
		assert(heap[i] != nullptr);
		assert(heap[i/2] != nullptr);
		assert(heap[i]->count <= heap[i/2]->count);
	}
#endif
}

template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::_print_cache(std::ostream& o) const
{
	_consistency_check();
	base_type::_print_cache(o);
	if(empty()) return;
	for(size_type i = 0; i < (size_type) (log(keymap.size()-1)/log(2))+1; ++i)
	{
		for(size_type j = pow(2, i); j <= pow(2, i+1)-1; ++j)
		{
			if(j > keymap.size()) break;
			o << '(' << heap[j]->key << "->" << heap[j]->val << ',';
			o << heap[j]->count << ") ";
		}
		o << '\n';
	}
}
