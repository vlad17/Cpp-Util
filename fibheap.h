/*
* Vladimir Feinberg
* fibheap.h
* 2013-12-28
*
* Declares all methods that a decrease-key priority queue must have,
* implements it as a Fibonacci heap. Fibonacci heaps have similar
* time performance compared to binary heaps, except that their
* relaxed structure allows for constant-time insertion, merge, and
* decrease-key.
*/

#ifndef FIBHEAP_H_
#define FIBHEAP_H_

// Define fib check if invariant checking in debug mode should be thorough.
// Thorough check traverses entire data structure.
#ifndef NDEBUG
#define FIB_CHECK 1
#else
#define FIB_CHECK 0
#endif

#include <list>
#include <ostream>
#include <vector>
#include <queue>
#include <cmath>
#include <cassert>
#include <cstddef>

#if FIB_CHECK
#include <unordered_set>
#endif

// TODO document

// TODO const value_type& get_value(const key_type& k)
// TODO decrease-key to take in std::function(void(value_type&))
// TODO decrease-key copy old value and check? ifdef ...?
// TODO merge
// TODO iterator, ordered_iterator (in vector key table version v2)
// TODO copying (v2)
// TODO delete any key, not min

/*
 * fibheap is a priority queue that allows for constant-time merge,
 * decrease-key, and insert operations (the delete operations are
 * logarithmic).
 *
 * The faster amortized time is accomplished by the relaxed data
 * structure which consists of a linked list of parent-aware nodes
 * comprising trees that observe the heap property and the a Fibonacci
 * degree property - where, "the e size of a subtree rooted in a node of
 * degree k is at least F_{k+2}, where F_k is the kth Fibonacci number.
 * This is achieved by the rule that we can cut at most one child of each
 * non-root node" (Wikipedia).
 *
 * The heap returns its own keys (which can be copied easily) that can
 * be used to communicate which node to decrease.
 *
 * Allows for duplicates
 *
 * fibheap<T, Compare>
 * T - type being contained in the queue
 * Compare - comparison functor
 */
template<typename T, typename Compare = std::less<T> >
class fibheap
{
private:
	// Use list container to ensure pointers
	// to nodes are always valid
	struct node
	{
		node(const T& val) :
			val(val), up(nullptr), down(nullptr), left(nullptr),
			right(nullptr),	marked(false), num_children(0)
		{
			left = right = this;
		}
		node(T&& val) :
			val(std::forward<T>(val)), up(nullptr), down(nullptr),
			left(nullptr), right(nullptr),	marked(false), num_children(0)
		{
			left = right = this;
		}
		template<typename... Args>
		node(Args&&... args) :
			val(std::forward<Args>(args)...), up(nullptr), down(nullptr),
			left(nullptr), right(nullptr),	marked(false), num_children(0)
		{
			left = right = this;
		}

		node(const node&) = delete;
		node(node&&) = delete;

		node& operator=(const node&) = delete;
		node& operator=(node&&) = delete;

		T val;
		node *up, *down, *left, *right;
		bool marked;
		size_t num_children;
	};

	// Min node, always on top level, acts as a root/head.
	node *min;
	// Current size
	size_t _size;
	// Comparison operator
	Compare comp;

	// Cycles through roots and joins nodes of equal degree
	std::pair<bool, node*> _join_nodes(std::vector<node*>& trees, node *n);
	// Cut out child but keep parent connected
	static void _rl_cut(node *n);
	// Completely cut out subtree
	static void _rlt_cut(node *n);
	// Splice into horizontal family but keep parent.
	static void _rl_splice(node *main, node *insert);
	// Splice completely under new tree
	static void _rlt_splice(node *parent, node *child);
	// Recursively delete subtree n
	static void _delete_subtree(node* n);
	// Deep copy of subtree from n into cpy (does not copy roots)
	static void _copy_subtree(node *cpy, const node *n);
	// Inserts new node at top level, checking min.
	inline void _insert_new_node(node *added);
	// Check invariants
	void _consistency_check() const;
#if FIB_CHECK
	// Recursive full traversal check
	static size_t _tree_check(const node *root,
			std::unordered_set<const node*>& s);
#endif /* FIB_CHECK */
	// Print debug info to ostream
	void _print_fibheap(std::ostream& o) const;

	static constexpr const double PHI = 1+sqrt((double) 5)/2;
	static constexpr size_t approx_childnum(size_t size);
public:
	// Public typedefs
	typedef Compare comparator_type;
	typedef T value_type;
	typedef const void* key_type;

	// Constructors/Destructor
	/*
	 * INPUT:
	 * const comparator_type& comp - comparison functor, uses default
	 * 		constructor for default value
	 * BEHAVIOR:
	 * Generates an empty fibonacci heap.
	 */
	explicit fibheap(const comparator_type& comp = comparator_type()) :
		min(nullptr), _size(0), comp(comp) {}
	/*
	 * INPUT:
	 * InputIterator first - first input iterator
	 * InputIterator last - last input iterator
	 * const comparator_type& comp - comparison functor, uses default
	 * 		constructor for default value
	 * BEHAVIOR:
	 * Generates a fibonacci heap filled with the input elements.
	 */
	template<typename InputIterator>
	fibheap(InputIterator first, InputIterator last,
		const comparator_type& comp = comparator_type());
	/*
	 * INPUT:
	 * const fibheap& other - fibheap to copy from
	 * BEHAVIOR:
	 * Generates a deep-copy of the fibheap.
	 */
	fibheap(const fibheap& other) :
		fibheap(comparator_type()) {*this = other;}
	/*
	 * INPUT:
	 * fibheap&& other - rvalue ref to fibheap
	 * BEHAVIOR:
	 * Moves data from other fibheap to this one.
	 */
	fibheap(fibheap&& other) noexcept :
		fibheap(comparator_type())
		{*this = std::forward<fibheap<T, Compare> >(other);}
	/*
	 * BEHAVIOR:
	 * Deallocates all used memory.
	 */
	~fibheap();

	// Methods
	/*
	 * INPUT:
	 * const fibheap& other - fibheap to copy
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Performs a deep copy.
	 * RETURN:
	 * *this
	 */
	fibheap& operator=(const fibheap& other);
	/*
	 * INPUT:
	 * fibheap&& other
	 * PRECONDITION:
	 * this != &other
	 * BEHAVIOR:
	 * Performs a move.
	 * RETURN:
	 * *this
	 */
	fibheap& operator=(fibheap&& other);
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Whether empty
	 */
	inline bool empty() const {return min == nullptr;}
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * RETURN:
	 * Size
	 */
	inline size_t size() const {return _size;}
	/*
	 * INPUT:
	 * PRECONDITION:
	 * !empty()
	 * BEHAVIOR:
	 * RETURN:
	 * const reference to minimum element
	 */
	inline const value_type& top() const {return min->val;}
	/*
	 * INPUT:
	 * const value_type& p - value pushed into heap
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Adds value to fibonacci heap.
	 * RETURN:
	 * Key associated with pushed item. Key remains valid until the
	 * value is popped.
	 */
	key_type push(const value_type& p);
	/*
	 * INPUT:
	 * Args&&... args - arguments for generating value_type
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Adds generated value to heap.
	 * RETURN:
	 * Key associated with pushed item. Key remains valid until the
	 * value is popped.
	 */
	template<typename... Args>
	key_type emplace(Args&&... args);
	/*
	 * INPUT:
	 * key_type k - key of value to decrease
	 * const value_type& v - new value for associated key
	 * PRECONDITION:
	 * v is smaller than or equal to  key's value.
	 * BEHAVIOR:
	 * Decreases key value and increases its priority.
	 * Key maintains validity to new value.
	 * RETURN:
	 */
	void decrease_key(key_type k, const value_type& v);
	/*
	 * INPUT:
	 * key_type k - key of value to decrease
	 * value_type&& v - rvalue ref to new value for associated key
	 * PRECONDITION:
	 * v is smaller than or equal to  key's value.
	 * BEHAVIOR:
	 * Decreases key value and increases its priority.
	 * Key maintains validity to new value.
	 * RETURN:
	 */
	void decrease_key(key_type k, value_type&& v);
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Clears heap. Invalidates all keys.
	 * RETURN:
	 */
	void clear();
	/*
	 * INPUT:
	 * PRECONDITION:
	 * BEHAVIOR:
	 * Pops minimum value off heap.
	 * RETURN:
	 * Invalidated key.
	 */
	key_type pop();

	template<typename T2, typename C2>
	friend std::ostream& operator<<(std::ostream&, const fibheap<T2, C2>&);
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
template<typename T, typename C>
std::ostream& operator<<(std::ostream& o, const fibheap<T, C>& f)
{
	f._print_fibheap(o);
	return o;
}

#include "fibheap.tpp"

#endif /* FIBHEAP_H_ */
