/*
* Vladimir Feinberg
* fibheap.h
* 2013-12-02
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
#endif

#include <list>
#include <vector>
#include <queue>
#include <cmath>
#include <cassert>

#if FIB_CHECK
#include <unordered_set>
#endif

// TODO document

// TODO efficient inputiterator?
// TODO decrease-key to take in std::function(void(value_type&))
// TODO decrease-key copy old value and check? ifdef ...?
// TODO merge
// TODO iterator, ordered_iterator (in vector key table version v2)
// TODO copying (v2)

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

	node *min; // min always on top level
	size_t _size;
	Compare comp;

	std::pair<bool, node*> _join_nodes(std::vector<node*>& trees, node *n);
	static void _rl_cut(node *n);
	static void _rlt_cut(node *n);
	static void _rl_splice(node *main, node *insert);
	static void _rlt_splice(node *parent, node *child);
	static void _delete_subtree(node* n);
	static void _copy_subtree(node *cpy, const node *n);
	inline void _insert_new_node(node *added);
	void _consistency_check() const;
#if FIB_CHECK
	static size_t _tree_check(const node *root,
			std::unordered_set<const node*>& s);
#endif /* FIB_CHECK */
	void _print_fibheap(std::ostream& o) const;

	static constexpr const double PHI = 1+sqrt((double) 5)/2;
	static constexpr size_t approx_childnum(size_t size);
public:
	// Types
	typedef Compare comparator_type;
	typedef T value_type;
	typedef const void* key_type;
	// Constructors
	explicit fibheap(const comparator_type& comp = comparator_type()) :
		min(nullptr), _size(0), comp(comp) {}
	template<typename InputIterator>
	fibheap(InputIterator first, InputIterator last,
		const Compare& comp = Compare());
	fibheap(const fibheap& other) :
		fibheap(comparator_type()) {*this = other;}
	fibheap(fibheap&& other) noexcept :
		fibheap(comparator_type())
		{*this = std::forward<fibheap<T, Compare> >(other);}
	~fibheap();
	// Assignment
	fibheap& operator=(const fibheap& other);
	fibheap& operator=(fibheap&& other);
	// Methods
	inline bool empty() const {return min == nullptr;}
	inline size_t size() const {return _size;}
	inline const value_type& top() const {return min->val;} // !empty()
	// Key is valid until the item is popped.
	key_type push(const value_type& p);
	template<typename... Args>
	void emplace(Args&&... args);
	// key needs to be vaild, remains valid.
	void decrease_key(key_type k, const value_type& v);
	void decrease_key(key_type k, value_type&& v); // NOT CHECKED FOR BEING SMALLER
	// Invalidates all keys
	void clear();
	// Returns invalidated key
	key_type pop();

	template<typename T2, typename C2>
	friend std::ostream& operator<<(std::ostream&, const fibheap<T2, C2>&);
};

template<typename T, typename C>
std::ostream& operator<<(std::ostream& o, const fibheap<T, C>& f)
{
	f._print_fibheap(o);
	return o;
}

#include "fibheap.tpp"

#endif /* FIBHEAP_H_ */
