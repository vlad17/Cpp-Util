/*
* Vladimir Feinberg
* cache.h
* 2013-12-02
*
* Declares generic abstract cache class and its methods.
*/

#ifndef CACHE_H_
#define CACHE_H_

template<typename Key, typename Value, typename Pred = std::equal_to<Key> >
class cache
{
protected:
	cache() {};
	virtual void _print_cache(std::ostream& o) const;
	static Pred key_comp;
public:
	// Public typedefs
	typedef Key key_type;
	typedef Value value_type;
	typedef Pred key_equal;
	typedef std::pair<Key, Value> kv_type;

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
	virtual size_t size() const = 0;
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Maximum size of cache
	 */
	virtual size_t get_max_size() const = 0;
	/*
	 * INPUT:
	 * const kv_type& kv - key-value pair
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Inserts (key, value) into hash. May remove older, less frequently used
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
	 * Inserts (key, value) into hash. May remove older, less frequently used
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
	virtual value_type *lookup(const key_type& key) const = 0;
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
	static key_equal key_eq() {return key_comp;}
	/*
	 * INPUT:
	 * std::ostream& - ostream to print to
	 * const cache<K,V,P>& - cache whose contents to print.
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Prints cache's contents to parameter ostream.
	 * RETURN:
	 */
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
	o << "cache @ " << this << ", size " << size() << '\n';
}

#endif /* CACHE_H_ */
