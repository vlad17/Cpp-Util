/*
* Vladimir Feinberg
* cache.h
* 2013-11-03
*
* Declares generic abstract cache class and its methods.
* Declares and defines one cache implementation, the heap_cache,
* which maintains a priority queue in the form a binary heap, allowing
* for logarithmic-time lookup, insertion.
*/

#ifndef CACHE_H_
#define CACHE_H_

// TODO document what everything should do

template<typename Key, typename Value, typename Pred = std::equal_to<Key> >
class cache
{
protected:
	cache() {};
public:
	typedef Key key_type;
	typedef Value value_type;
	typedef Pred key_equal;
	typedef std::pair<Key, Value> kv_type;
	typedef size_t size_type;
	virtual ~cache() {};
	virtual bool empty() const = 0;
	virtual size_type size() const = 0; // not max size, current size
	virtual void insert(const kv_type& kv) = 0;
	virtual void insert(kv_type&& kv) = 0;
	template<typename... Args>
	virtual void emplace(Args&& args) = 0;
	virtual value_type *lookup(const key_type& key) const = 0;
	virtual void clear() = 0;
};

#endif CACHE_H_
