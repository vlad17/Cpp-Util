/*
* Vladimir Feinberg
* lfu_cache.h
* 2013-11-29
*
* Defines heap_cache and linked_cache classes, which are implementations of
* the abstract cache class that eliminate the least frequently used member
* when a new element is inserted (and space is needed).
*/

#ifndef LFU_CACHE_H_
#define LFU_CACHE_H_

#include <unordered_map>
#include <vector>
#include <cassert>
#include <algorithm>

#include "cache.h"

namespace lfu
{
	// TODO document lfu is approximate (max ordered)
	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Hash = std::hash<Key> >
	class heap_cache : public cache<Key, Value, Pred>
	{
	private:
		struct citem
		{
			typedef typename cache<Key, Value, Pred>::kv_type kv_type;
			citem(kv_type&& kv, size_t loc) :
				key(std::move(kv.first)), val(std::move(kv.second)),
				loc(loc), count(0) {}
			citem(const kv_type& kv, size_t loc) :
				key(kv.first), val(kv.second), loc(loc), count(0) {}
			Key key;
			Value val;
			size_t loc;
			size_t count;
			const kv_type& kvpair() const
			{return *reinterpret_cast<const kv_type*>(&key);}
		};
		mutable std::unordered_map<Key, citem*, Hash, Pred> keymap;
		mutable std::vector<citem*> heap;
		static Hash hashf;
		void _del_back();
		void increase_key(citem *c) const;
		void _consistency_check() const;
		void _print_cache(std::ostream& o) const;
		size_t max_size;
	public:
		typedef cache<Key, Value, Pred> base_type;
		typedef typename base_type::key_type key_type;
		typedef typename base_type::value_type value_type;
		typedef typename base_type::key_equal key_equal;
		typedef typename base_type::kv_type kv_type;
		typedef typename base_type::size_type size_type;
		typedef Hash hasher;
		heap_cache(size_t max = -1):
			 keymap(), heap(), max_size(max) {heap.push_back(nullptr);}
		heap_cache(const heap_cache& other) :
			heap_cache(0) {*this = other;}
		heap_cache(heap_cache&& other) :
			heap_cache(0) {*this = std::forward(other);}
		heap_cache& operator=(const heap_cache& other);
		heap_cache& operator=(heap_cache&& other);
		virtual ~heap_cache() {};
		virtual bool empty() const;
		virtual size_type size() const; // not max size, current size
		// Returns true if value added without any removals
		// returns false if lfu kicked out if no room or if
		// item replaced for having the same key.
		virtual bool insert(const kv_type& kv);
		virtual bool insert(kv_type&& kv);
		virtual bool contains(const key_type& key) const;
		virtual value_type *lookup(const key_type& key) const;
		virtual void clear();
		void set_max_size(size_t size);
		hasher hash_function() const {return hashf;}
	};

	// TODO exact_heap_cache (min ordered at top, always removes exactly
	// LFU)

	// TODO linked_cache (exact, best amortized, nonlocal)
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
			_del_back();
			--valp->loc;
			retval = false;
		}
		mapped = valp;
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
	citem *& mapped = keymap[valp->key];
	if(mapped == nullptr)
	{
		if(keymap.size() == max_size)
		{
			_del_back();
			--valp->loc;
		}
		mapped = valp;
		heap.push_back(valp);
		return true;
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
void lfu::heap_cache<K,V,P,H>::set_max_size(size_t max)
{
	_consistency_check();
	max_size = max;
	if(max < heap.size()-1)
		while(heap.size() != max) // note +1 index, stop at max.
			_del_back();
}

// ---- helper methods

// swaps increased key until heap property is restored, returns
template<typename K, typename V, typename P, typename H>
void lfu::heap_cache<K,V,P,H>::_del_back()
{
	assert(heap.size() > 1);
	keymap.erase(heap.back()->key);
	delete heap.back();
	heap.pop_back();
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
	for(size_t i = keymap.size(); i > 1; --i)
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
	for(size_t i = 0; i < (size_t) (log(keymap.size()-1)/log(2))+1; ++i)
	{
		for(size_t j = pow(2, i); j <= pow(2, i+1)-1; ++j)
		{
			if(j > keymap.size()) break;
			o << '(' << heap[j]->key << "->" << heap[j]->val << ',';
			o << heap[j]->count << ") ";
		}
		o << '\n';
	}
}

#endif /* LFU_CACHE_H_ */
