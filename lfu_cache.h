/*
* Vladimir Feinberg
* lfu_cache.h
* 2013-11-03
*
* Defines heap_cache and linked_cache classes, which are implementations of
* the abstract cache class that eliminate the least frequently used member
* when a new element is inserted
*/

#ifndef LFU_CACHE_H_
#define LFU_CACHE_H_

#include <unordered_map>
#include <vector>
#include <algorithm>

#include "cache.h"

namespace lfu
{
	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Hash = std::hash<Key> >
	class heap_cache : public cache<Key, Value, Pred>
	{
	private:
		std::unordered_map<Key, size_t, Hash, Pred> keymap;
		std::vector<Value> valheap;
		Hash hashf;
	public:
		typedef Hash hasher;
		heap_cache() :
			keymap(), valheap(), hashf() {}
		// TODO inputiterator + efficient
		virtual ~heap_cache() {};
		virtual bool empty() const;
		virtual size_type size() const; // not max size, current size
		virtual void insert(const kv_type& kv);
		virtual void insert(kv_type&& kv);
		template<typename... Args>
		virtual void emplace(Args&& args);
		virtual value_type *lookup(const key_type& key) const;
		virtual void clear() = 0;
		//void trim_to_size(size_t size);
		//hasher hash_function() const {return hashf;}
	};
}

template<typename K, typename V, typename P>
bool lfu::heap_cache<K,V,P>::empty()
{
	return valheap.empty();
}




#endif /* LFU_CACHE_H_ */
