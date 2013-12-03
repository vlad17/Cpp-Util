/*
* Vladimir Feinberg
* lfu_cache.h
* 2013-12-02
*
* Defines heap_cache, exact_heap_cache, and linked_cache classes,
* which are implementations of the abstract cache class that eliminate
* the least frequently used member when a new element is inserted
* (and space is needed).
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
	 * Key - key type - should be quick to copy (several are made)
	 * Value - value type - type key maps to. Copied/moved once.
	 * Pred - equal-to predicate for keys
	 * Hash - hash for keys.
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
		// Public typedefs
		typedef cache<Key, Value, Pred> base_type;
		typedef typename base_type::key_type key_type;
		typedef typename base_type::value_type value_type;
		typedef typename base_type::key_equal key_equal;
		typedef typename base_type::kv_type kv_type;
		typedef Hash hasher;

		// Constructors/Destructor
		/*
		 * INPUT:
		 * size_t max = -1 - maximum initial size
		 * PRECONDITION:
		 * max > 0
		 * BEHAVIOR:
		 * Generates an empty heap_cache with specified max size.
		 */
		heap_cache(size_t max = -1):
			 keymap(), heap(), max_size(max) {heap.push_back(nullptr);}
		/*
		 * INPUT:
		 * const heap_cache& other - heap cache to copy from
		 * BEHAVIOR:
		 * Makes a deep copy of other.
		 */
		heap_cache(const heap_cache& other) :
			heap_cache(1) {*this = other;}
		/*
		 * INPUT:
		 * heap_cache&& other - heap cache to move from
		 * BEHAVIOR:
		 * Moves (deletes and shallow-copies) other.
		 */
		heap_cache(heap_cache&& other) :
			heap_cache(1) {*this = std::forward(other);}
		/*
		 * INPUT:
		 * BEHAVIOR:
		 * Destroys cache, deallocating all owned memory.
		 */
		virtual ~heap_cache();

		// Methods
		/*
		 * INPUT:
		 * const heap_cache& other - heap cache to copy from
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Deep copy of other is made, each key-value pair and associated
		 * lookup count is copied over
		 * RETURN:
		 * Reference to this.
		 */
		heap_cache& operator=(const heap_cache& other);
		/*
		 * INPUT:
		 * heap_cache&& other - heap cache to move from
		 * PRECONDITION:
		 * this != &other
		 * BEHAVIOR:
		 * Clears current members and moves over information from other
		 * to this. Other is put into an invalid state.
		 * RETURN:
		 * Reference to this.
		 */
		heap_cache& operator=(heap_cache&& other);
		/*
		 * INPUT:
		 * PRECONDITION:
		 * BEHAVIOR:
		 * RETURN:
		 * Whether the cache is empty
		 */
		virtual bool empty() const;
		/*
		 * INPUT:
		 * PRECONDITION:
		 * BEHAVIOR:
		 * RETURN:
		 * Current (not maximum) size of cache
		 */
		virtual size_t size() const;
		/*
		 * INPUT:
		 * PRECONDITION:
		 * BEHAVIOR:
		 * RETURN:
		 * Maximum size of cache
		 */
		virtual size_t get_max_size() const;
		/*
		 * INPUT:
		 * const kv_type& kv - key-value pair
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Inserts (key, value) into hash. May remove older, less frequently used
		 * members from cache. If another object is present with the same key,
		 * it is replaced.
		 * RETURN:
		 * True if an object of the same key is present (replacement occurred), or
		 * false otherwise.
		 */
		virtual bool insert(const kv_type& kv);
		/*
		 * INPUT:
		 * kv_type&& kv - key-value pair
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Inserts (key, value) into hash. May remove older, less frequently used
		 * members from cache. If another object is present with the same key,
		 * it is replaced.
		 * RETURN:
		 * False if an object of the same key is present (replacement occurred), or
		 * true otherwise.
		 */
		virtual bool insert(kv_type&& kv);
		/*
		 * INPUT:
		 * const key_type& key - key to look up
		 * PRECONDITION:
		 * BEHAVIOR:
		 * RETURN:
		 * Whether cache contains a certain key
		 */
		virtual bool contains(const key_type& key) const;
		/*
		 * INPUT:
		 * const key_type& key
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Increases the lookup count for the key and changes associated
		 * internal structure to prioritize retention of the key.
		 * RETURN:
		 * Pointer to associated value for key or nullptr if key is not
		 * mapped to a value currently in the cache.
		 */
		virtual value_type *lookup(const key_type& key) const;
		/*
		 * INPUT:
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Clears cache completely, removing all items and setting size to
		 * 0. Maximum size is kept the same.
		 * RETURN:
		 */
		virtual void clear();
		/*
		 * INPUT:
		 * size_t size - new max size
		 * PRECONDITION:
		 * size > 0
		 * BEHAVIOR:
		 * Sets maximum size. May result in deallocations and deletions if
		 * new max size is smaller than current size.
		 */
		void set_max_size(size_t size);
		/*
		 * INPUT:
		 * PRECONDITION:
		 * BEHAVIOR:
		 * RETURN:
		 * Hashing function
		 */
		static hasher hash_function() {return hashf;}
	};

	// TODO exact_heap_cache (min ordered at top, always removes exactly
	// LFU)

	// TODO linked_cache (exact, best amortized, nonlocal)
}

#include "heap_cache.tpp"

#endif /* LFU_CACHE_H_ */
