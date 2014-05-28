/*
 * Vladimir Feinberg
 * concurrent_cache.h
 * 2014-05-26
 *
 * Contains some template specializations for concurrent versions of caches
 */

#include <atomic>
#include <mutex>

#include "locks.h"
#include "cache.h"
#include "lfu_cache.h"

#define LOCK(x) lock_guard<decltype(x)>(x);

// generic concurrency skeleton with r/w lock
namespace concurrent
{
	template<typename Cache>
	class default_synchronization : public Cache
	{
	protected:
		locks::rw _lk;
		locks::rw::ronly _rlk;
		template<typename... Args>
		default_synchronization(Args&&... args) :
			Cache(std::forward<Args>(args)...), _lk(), _rlk(_lk.read_only()) {}
	public:
		typedef Cache base_type;
		CACHE_TYPEDEFS;
		virtual ~default_synchronization() {}
		virtual bool empty() const {LOCK(_rlk); return base_type::empty();}
		virtual size_type size() const {LOCK(_rlk); return base_type::size();}
		virtual size_type get_max_size() const {LOCK(_rlk); return base_type::max_size();}
		virtual bool insert(const kv_type& kv) {LOCK(_lk); return base_type::insert(kv);}
		virtual bool insert(kv_type&& kv) {LOCK(_lk); return base_type::insert(std::forward<kv_type>(kv));}
		virtual bool contains(key_cref key) const {LOCK(_rlk); return base_type::contains();}
		virtual value_type *lookup(key_cref key) const {LOCK(_rlk); return base_type::lookup(key);}
	};

	/**
	 * Mutex on keys in hash map, r/w lock on heap. Assumes concurrent traits inserted.
	 * Relies on methods increase_key().
	 * Assumes instance heap[int] holds keys to a keymap.at(key)
	 * Requires all removals from heap (at back) done through virtual method.
	 */
	template<typename Cache>
	class heap_cache : public default_synchronization<Cache>
	{
	public:
		typedef Cache base_type;
		CACHE_TYPEDEFS;
		template<typename... Args>
		heap_cache(Args&&... args) :
			default_synchronization(std::forward<Args>(args)...), inuse() {}
		heap_cache(const heap_cache&) = delete;
		heap_cache(heap_cache&&) = delete;
		heap_cache& operator=(const heap_cache&) = delete;
		heap_cache& operator=(heap_cache&&) = delete;
		virtual ~heap_cache() {}
		virtual bool insert(const kv_type& kv) {LOCK(_lk); inuse.emplace_back(); return base_type::insert(kv);}
		virtual bool insert(kv_type&& kv) {LOCK(_lk); inuse.emplace_back(); return base_type::insert(std::forward<kv_type>(kv));}
		virtual void set_max_size(size_t size) {LOCK(_lk); base_type::set_max_size();};
	protected:
		virtual void increase_key(key_cref k) const;
		virtual void _del_back() {inuse.pop_back(); base_type::_del_back();} // assumes write lock
	private:
		std::vector<std::mutex> inuse;
	};

	struct concurrent_heap_cache_traits
	{
		typedef std::atomic<size_t> count_type;
	};

	template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
			typename Hash = std::hash<Key> >
	class lfu_heap_cache : public heap_cache
		<lfu::heap_cache<Key, Value, Pred, Hash, concurrent_heap_cache_traits> >
	{	};
}

template<typename T>
void concurrent::heap_cache<T>::increase_key(key_cref k) const
{
	auto& c = keymap.at(k);
	assert(c.loc < heap.size());
	assert(c.loc > 0);
	while(c.loc > 1)
	{
		std::lock(inuse[c.loc/2 - 1], inuse[c.loc - 1]);
		auto& parent = keymap.at(heap[c.loc/2]);
		if(parent.count >= c.count) break;
		std::swap(heap[parent.loc], heap[c.loc]);
		std::swap(parent.loc, c.loc);
		inuse[c.loc/2 - 1].unlock();
		inuse[c.loc - 1].unlock();
	}
}


