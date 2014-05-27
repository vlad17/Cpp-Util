/*
* Vladimir Feinberg
* cache.h
* 2014-05-26
*
* Declares generic abstract cache class and its methods.
*/

#ifndef CACHE_H_
#define CACHE_H_

#include "util.h"

template<typename Key, typename Value, typename Pred = std::equal_to<Key> >
struct dfl_cache_traits
{
	typedef Key key_type;
	typedef typename util::scref<Key>::type key_cref;
	typedef Value value_type;
	typedef Pred key_equal;
	typedef std::pair<Key, Value> kv_type;
	typedef size_t size_type;
};

template<typename Key, typename Value, typename Pred = std::equal_to<Key>,
		typename Traits = dfl_cache_traits<Key, Value, Pred> >
class cache
{
public:
	// Public typedefs
	typedef typename Traits::key_type key_type;
	typedef typename Traits::value_type value_type;
	typedef typename Traits::key_equal key_equal;
	typedef typename Traits::kv_type kv_type;
	typedef typename Traits::key_cref key_cref;
	typedef typename Traits::size_type size_type;

	// Destructor (does nothing)
	virtual ~cache() {};

	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Whether the cache is empty
	 */
	virtual bool empty() const = 0;
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Current (not maximum) size of cache
	 */
	virtual size_type size() const = 0;
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Maximum size of cache
	 */
	virtual size_type get_max_size() const = 0;
	/*
	 * INPUT:
	 * const kv_type& kv - key-value pair
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Inserts (key, value) into cache. May remove older, less frequently used
	 * members from cache. If another object is present with the same key,
	 * it is replaced.
	 * RETURN:
	 * False if an object of the same key is present (replacement occurred), or
	 * true otherwise.
	 */
	virtual bool insert(const kv_type& kv) = 0;
	/*
	 * INPUT:
	 * kv_type&& kv - key-value pair
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Inserts (key, value) into cache. May remove older, less frequently used
	 * members from cache. If another object is present with the same key,
	 * it is replaced.
	 * RETURN:
	 * False if an object of the same key is present (replacement occurred), or
	 * true otherwise.
	 */
	virtual bool insert(kv_type&& kv) = 0;
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
	virtual value_type *lookup(key_cref key) const = 0;
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Clears cache completely, removing all items and setting size to
	 * 0. Maximum size is kept the same.
	 * RETURN:
	 */
	virtual void clear() = 0;
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Key comparison function.
	 */
	static key_equal key_eq() {return key_predicate;}
	/*
	 * INPUT:
	 * std::ostream& - ostream to print to
	 * const cache<K,V,P>& - cache whose contents to print.
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Prints cache's contents to parameter ostream.
	 * RETURN:
	 */
	template<typename K, typename V, typename P, typename T>
	friend std::ostream& operator<<(std::ostream&, const cache<K,V,P,T>&);
protected:
	cache() {};
	virtual void _print_cache(std::ostream& o) const;
	static key_equal key_predicate;
};

/*
 * INPUT:
 * std::ostream& o - ostream to print to
 * const cache<K,V,P>& cache - cache to print
 * PRECONDITION:
 * BEHAVIOR:
 * Prints cache to stream
 * RETURN:
 * Original ostream
 */
template<typename K, typename V, typename P, typename T>
std::ostream& operator<<(std::ostream& o, const cache<K,V,P,T>& cache)
{
	cache._print_cache(o);
	return o;
}

template<typename K, typename V, typename P, typename T>
typename cache<K,V,P,T>::key_equal cache<K,V,P,T>::key_predicate {};

template<typename K, typename V, typename P, typename T>
void cache<K,V,P,T>::_print_cache(std::ostream& o) const
{
	o << "cache @ " << this << ", size " << size() << '\n';
}

#endif /* CACHE_H_ */
