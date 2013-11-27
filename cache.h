/*
* Vladimir Feinberg
* cache.h
* 2013-11-24
*
* Declares generic abstract cache class and its methods.
*/

#ifndef CACHE_H_
#define CACHE_H_

// TODO document what everything should do

template<typename Key, typename Value, typename Pred = std::equal_to<Key> >
class cache
{
protected:
	cache() {};
	virtual void _print_cache(std::ostream& o) const;
	static Pred key_comp;
public:
	typedef Key key_type;
	typedef Value value_type;
	typedef Pred key_equal;
	typedef std::pair<Key, Value> kv_type;
	typedef size_t size_type;
	virtual ~cache() {};
	virtual bool empty() const = 0;
	virtual size_type size() const = 0; // not max size, current size
	// returns false if key already in table
	virtual bool insert(const kv_type& kv) = 0;
	virtual bool insert(kv_type&& kv) = 0;
	virtual value_type *lookup(const key_type& key) const = 0;
	virtual void clear() = 0;
	key_equal key_eq() {return key_comp;}
	template<typename K, typename V, typename P>
	friend std::ostream& operator<<(std::ostream&, const cache<K,V,P>&);
};

template<typename K, typename V, typename P>
std::ostream& operator<<(std::ostream& o, const cache<K,V,P>& cache)
{
	cache._print_cache(o);
	return o;
}

template<typename K, typename V, typename P>
void cache<K,V,P>::_print_cache(std::ostream& o) const
{
	o << "cache @ " << this << ", size " << size();
}

#endif CACHE_H_
