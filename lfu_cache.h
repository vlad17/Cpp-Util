/*
* Vladimir Feinberg
* lfu_cache.h
* 2013-11-28
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

#include "cache.h"


// Value should be moveable (or a pointer)

namespace lfu
{
	// TODO maxsize w/ force remove
	// TODO setsize, just use -1 default.
	// TODO replace
	// TODO consistency
	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Hash = std::hash<Key> >
	class heap_cache : public cache<Key, Value, Pred>
	{
	private:
		struct citem
		{
			citem(Value&& v, size_t loc) :
				val(std::forward<Value>(v)), loc(loc), count(0) {}
			citem(const Value& v, size_t loc) :
				val(v), loc(loc), count(0) {}
			Value val;
			size_t loc;
			size_t count;
		};
		mutable std::unordered_map<Key, citem*, Hash, Pred> keymap;
		mutable std::vector<citem*> heap;
		static Hash hashf;
		void increase_key(citem *c) const;
	public:
		typedef cache<Key, Value, Pred> base_type;
		typedef typename base_type::key_type key_type;
		typedef typename base_type::value_type value_type;
		typedef typename base_type::key_equal key_equal;
		typedef typename base_type::kv_type kv_type;
		typedef typename base_type::size_type size_type;
		typedef Hash hasher;
		// push back dummy value to allow easy heap access.
		heap_cache():
			keymap(), heap() {heap.push_back(nullptr);}
		// TODO inputiterator + efficient
		virtual ~heap_cache() {};
		virtual bool empty() const;
		virtual size_type size() const; // not max size, current size
		virtual bool insert(const kv_type& kv);
		virtual bool insert(kv_type&& kv);
		virtual value_type *lookup(const key_type& key) const;
		virtual void clear();
		//void trim_to_size(size_t size);
		hasher hash_function() const {return hashf;}
	};
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::empty() const
{
	return heap.empty();
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::size() const -> size_type
{
	return heap.size();
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::insert(const kv_type& kv)
{
	citem *valp = new citem(kv.second, heap.size());
	auto mappair = keymap.insert(std::make_pair(kv.first, valp));
	if(!mappair.second)
		return false;
	heap.push_back(mappair.first->second);
	return true;
}

template<typename K, typename V, typename P, typename H>
bool lfu::heap_cache<K,V,P,H>::insert(kv_type&& kv)
{
	citem *valp = new citem(std::move(kv.second), heap.size());
	auto mappair = keymap.insert(
			std::make_pair(std::move(kv.first), valp));
	if(!mappair.second)
		return false;
	heap.push_back(mappair.first->second);
	return true;
}

template<typename K, typename V, typename P, typename H>
auto lfu::heap_cache<K,V,P,H>::lookup(const key_type& key) const -> value_type*
{
	auto access = keymap.find(key);
	if(access == keymap.end()) return nullptr;
	++access->second->count;
	increase_key(access->second);
	return &access->second->val;
}

// ---- helper methods

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
void lfu::heap_cache<K,V,P,H>::clear()
{
	for(auto i : heap) delete i;
	heap.clear();
	keymap.clear();
}

#endif /* LFU_CACHE_H_ */
