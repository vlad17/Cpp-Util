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

#include <algorithm>

#include "cache.h"

namespace lfu
{
	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Hash = std::hash<Key> >
	class heap_cache : public cache<Key, Value, Pred>
	{
	public:
		typedef Hash hasher;
	};
}

#endif /* LFU_CACHE_H_ */
