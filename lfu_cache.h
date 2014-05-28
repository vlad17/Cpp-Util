/*
* Vladimir Feinberg
* lfu_cache.h
* 2014-05-27
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

#define CACHE_TYPEDEFS \
		typedef typename base_type::key_type key_type; \
		typedef typename base_type::value_type value_type; \
		typedef typename base_type::key_equal key_equal; \
		typedef typename base_type::kv_type kv_type; \
		typedef typename base_type::key_cref key_cref; \
		typedef typename base_type::size_type size_type;

namespace lfu
{
	struct heap_cache_traits
	{
		typedef size_t count_type;
	};

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
	 * Traits - counting type trait
	 *
	 * Two copies of the key will be kept, one in the heap and one in the hash.
	 */
	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Hash = std::hash<Key>, typename Traits = heap_cache_traits>
	class heap_cache : public cache<Key, Value, Pred>
	{
	public:
		// Public typedefs
		typedef cache<Key, Value, Pred> base_type;
		CACHE_TYPEDEFS
		typedef Hash hasher;
		typedef typename Traits::count_type count_type;
	protected:
		// An item is a key-value pair, location in heap, and count
		// Maintining all the information in one place allows two-way
		// access between the heap and lookup map.
		struct citem
		{
			citem(value_type&& val, size_t loc) :
				val(std::forward<value_type>(val)), loc(loc), count(0) {}
			citem(const value_type& val, size_t loc) :
				val(val), loc(loc), count(0) {}
			citem(citem&& c) = default;
			citem(const citem& c) = default;
			mutable value_type val;
			size_t loc;
			typename Traits::count_type count;
		};
		// Increase citem to restore heap property
		virtual void increase_key(key_cref k) const;
	private:
		// REFRESH_RATIO is ratio of cache that remains on lookup-triggered refresh.
		static constexpr double REFRESH_RATIO = 0.5;
		// Hash function
		static hasher hashf;
		// Maintains mapping from key to citem
		mutable std::unordered_map<key_type, citem, hasher, key_equal> keymap;
		// Heap keeps a priority-queue like structure
		mutable std::vector<key_type> heap;
		// Maximum size of cache.
		count_type max_size;
		// Pop back item from heap. Most likely to be recent, and infrequently
		// used.
		void _del_back();
		// Pop REFRESH_RATIO citems off
		void _del_back_full();
		// Check invariants
		void _consistency_check() const;
		// Prints debug info
		void _print_cache(std::ostream& o) const;
	public:
		// Constructors/Destructor
		/*
		 * INPUT:
		 * size_t max = -1 - maximum initial size
		 * BEHAVIOR:
		 * Generates an empty heap_cache with specified max size.
		 */
		heap_cache(size_t max = -1):
			 keymap(), heap(), max_size(max) {heap.push_back(Key());}
		// TODO input iterator range constructor
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
		 * Not noexcept. See move assignment.
		 */
		heap_cache(heap_cache&& other) :
			heap_cache(1) {*this = std::forward<heap_cache>(other);}
		virtual ~heap_cache() {}

		// Methods
		/*
		 * INPUT:
		 * const heap_cache& other - heap cache to copy from
		 * PRECONDITION:
		 * BEHAVIOR:
		 * Deep copy of other is made, each key-value pair and associated
		 * lookup count is copied over.
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
		 * Not noexcept because the class contains a member which has only
		 * a basic guarantee for exceptions (vector).
		 * RETURN:
		 * Reference to this.
		 */
		heap_cache& operator=(heap_cache&& other);
		// See cache.h for documentation of virtual functions
		virtual bool empty() const {return keymap.empty();}
		virtual size_type size() const {return keymap.size();}
		virtual size_type get_max_size() const {return max_size;}
		virtual bool insert(const kv_type& kv);
		virtual bool insert(kv_type&& kv);
		virtual bool contains(key_cref key) const {return keymap.find(key) != keymap.end();}
		virtual value_type *lookup(key_cref key) const;
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
