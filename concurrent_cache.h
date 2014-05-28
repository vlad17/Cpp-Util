/*
 * Vladimir Feinberg
 * concurrent_cache.h
 * 2014-05-26
 *
 * Contains some template specializations for concurrent versions of caches
 */

#include <atomic>

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
		virtual value_type *lookup(key_cref key) const {LOCK(_lk); return base_type::lookup(key);}
	};

	struct heap_cache_traits
	{
		typedef std::atomic<size_t> count_type;
	};

	/**
	 * Mutex on keys in hash map, r/w lock on heap. Assumes concurrent traits inserted.
	 * Relies on methods increase_
	 */
	template<typename Cache>
	class heap_cache : public default_synchronization<Cache>
	{
	private:
		std::vector<std::mutex> inuse;
	protected:
		virtual
	public:
		typedef Cache base_type;
		typedef typename base_type::count_type count_type;
		CACHE_TYPEDEFS;
		template<typename... Args>
		heap_cache(Args&&... args) :
			default_synchronization(std::forward<Args>(args)...), inuse() {}
		virtual ~heap_cache() {}
		virtual bool insert(const kv_type& kv) {LOCK(_lk); return base_type::insert(kv);}
		virtual bool insert(kv_type&& kv) {LOCK(_lk); return base_type::insert(std::forward<kv_type>(kv));}
	};
}


