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
	/*
	 * A heap_cache is a type which acts as an approximate least frequently used
	 * (lfu) container. The cache, when set to have a maximum size, will fill up
	 * and clear the least frequently used members in "chunks" - the last
	 * max_size/2 members are removed when capacity is overfilled.
	 *
	 * In this manner, the insertion (and cache auto-clearing) is amortized O(1)
	 * time - since it will take as many constant-time insertions to require
	 * a linear-time removal. Worst-case is O(n).
	 *
	 * Lookup needs to maintain the heap property, so it may be O(log n).
	 *
	 * heap_cache<Key, Value, Pred, Hash>
	 * Key - key type -
	 */
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
		void _del_back_full();
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

#include "heap_cache.tpp"

#endif /* LFU_CACHE_H_ */
