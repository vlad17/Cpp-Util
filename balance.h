/*
 * Vladimir Feinberg
 * balance.h
 * 2014-01-22
 *
 * Declares and defines the bset class - standing for balanced set - which is an
 * ordered set implemented as a "balance tree." A balance tree is a balanced binary
 * search tree which satisfies the minimal height property and binary tree property.
 * The balance tree was an original concept.
 *
 * Because of its strict structure, it can be maintained in a resizeable array,
 * using heap indices, since the overhead is not too great.
 *
 * This allows for locality of reference and the memory.
 *
 * One drawback of the balance tree is the frequent moves: in order to maintain
 * binary tree property through swaps, the algorithms need
 * to update swapped nodes, which may take O(log n) swaps. Then, in general,
 * insert/delete have O((log n)^2) worst case time, for O(log n) swapped nodes.
 *
 * Note that the stable_bset cannot swap values (allowing for the stability)
 */

#ifndef BALANCE_H_
#define BALANCE_H_

#include <vector>
#include <iostream>

#include <unordered_set> // checking invariants
#include <queue> // checking invariants via bfs && printing

#include "impl_util.h"

// TODO non-recursive implementation

// TODO document
// TODO make like set (make an iterator)
	// for iterator just hold a pointer to current node
	// (make it a union of a pointer to node and pointer to list (differentiate by
 	// accessing identity member - -1 for lists, size for nodes)
// TODO add sorted elements constructor (method?)

// TODO faster algorithm?

/*
 * Set implementation using a balance tree, uses pointers to nodes
 * instead of an array with heap indices. In this implementation T need not
 * be move constructible, but the tree has no locality of reference.
 */
template<typename T, typename Compare = std::less<T> >
class stable_bset
{
public:
	// Public Typedefs
	typedef T value_type;
	typedef typename implementation_utility::easily_copyable<T>::type cref_type;
	typedef std::size_t size_type;
	typedef Compare key_comp;
private:
	struct node;
	/* Important structural note: the first member in meta, node,
	* and stable_bset must be a size_type, and in a bset it must have the value -1.
	* This allows for identification between the structures (which allows for lighter
	* iterator types).
	*/
	// Meta info for node
	struct meta
	{
		typename stable_bset<T,Compare>::size_type size;
		node *left, *right, *parent;
		meta() :
			size(1), left(nullptr), right(nullptr), parent(nullptr) {}
	};
	// Node struct
	struct node
	{
	private:
		meta m;
		T val;
	public:
		// Constructor/Destructor
		node(cref_type val) :
			val(val), m() {}
		~node() {delete m.left; delete m.right;}
		// Tree ops
		bool null() const; // disconnected?
		// Setters
		INLINE void set_meta(const meta& newm) {m = newm;}
		INLINE node *set_parent(node* n) {return m.parent = n;}
		INLINE node *set_left(node* n) {return m.left = n;}
		INLINE node *set_right(node* n) {return m.right = n;}
		INLINE size_type set_size(size_type s) {return m.size = s;}
		// Getters
		INLINE const meta& metadata() const {return m;}
		INLINE node *parent() const {return m.parent;}
		INLINE node *left() const {return m.left;}
		INLINE node *right() const {return m.right;}
		INLINE size_type size() const {return m.size;}
		INLINE cref_type value() const {return val;}
	};
	// Private members
	node *root;
	static key_comp compare;
	// Methods all have precondition that pointers are not null unless otherwise stated.
	// Node operations
	void node_link_immediate(node *n);
	void node_swap(node *n1, node *n2);
	INLINE bool node_equal(cref_type val, node *n) const;
	void node_swim(node *n);
	// Consistency and printing
	void _consistency_check() const;
	void _print_tree(std::ostream& o) const;
	template<typename T2, typename C2>
	friend std::ostream& operator<<(std::ostream& o, const stable_bset<T2,C2>& bset);
	// Tree algorithms
	node *find_helper(cref_type val, node *n) const; // accepts nullptr
	void insert_helper(node *free, node *n);
		// Descend tree decrementing size and replacing
	void erase_helper(node *erase, node *current, node *replacement);
	void push_out(node *n);
	node *predecessor(node *n);
	node *successor(node *n);
public:
	// Constructor
	stable_bset() :
		root(nullptr) {}
	~stable_bset() {delete root;}
	// Public methods
	INLINE size_type size() const;
	INLINE bool empty() const;
	void clear();
	bool find(cref_type val) const; // true if and only if there
	bool insert(cref_type val); // true if inserted
	size_type erase(cref_type val);
};

template<typename T, typename C>
typename stable_bset<T,C>::key_comp stable_bset<T,C>::compare {};

// ----- Node operations

template<typename T, typename C>
auto stable_bset<T,C>::node::null() const -> bool
{
	return m.size == 1 &&
		m.parent == nullptr &&
		m.left == nullptr &&
		m.right == nullptr;
}

template<typename T, typename C>
auto stable_bset<T,C>::node_link_immediate(node *n) -> void
{
	assert(n != nullptr);
	if(n->parent())
	{
		if(compare(n->parent()->value(), n->value()))
			n->parent()->set_right(n);
		else
			n->parent()->set_left(n);
	}
	if(n->right())
		n->right()->set_parent(n);
	if(n->left())
		n->left()->set_parent(n);
}

template<typename T, typename C>
auto stable_bset<T,C>::node_swap(node *n1, node *n2) -> void
{
	assert(n1 != nullptr);
	assert(n2 != nullptr);
	if(n1 == n2) return;
	// If any node is not in tree, or the nodes are far enough apart
	if(n1->null() || n2->null() ||
			(n1->parent() != n2 && n1->right() != n2 && n1->left() != n2))
	{
		meta tmp = n1->metadata();
		n1->set_meta(n2->metadata());
		n2->set_meta(tmp);
	}
	else // Related case is harder
	{
		// Make n2 parent of n1
		if(n1->parent() != n2)
		{
			node *tmp = n1;
			n1 = n2;
			n2 = tmp;
		}
		assert(n1->parent() == n2);
		// Now swap so that n1 is parent of n2
		meta child = n1->metadata();
		child.parent = n1;
		meta parent = n2->metadata();
		if(parent.left == n1) parent.left = n2;
		else if(parent.right == n1) parent.right = n2;
		n1->set_meta(parent);
		n2->set_meta(child);
	}
	// Now that they are in the right place, make sure related notes have
	// appropriate reverse connections
	node_link_immediate(n1);
	node_link_immediate(n2);
	// Check root
	if(root == n1) root = n2;
	else if(root == n2) root = n1;
}

template<typename T, typename C>
auto stable_bset<T,C>::node_equal(cref_type val, node *n) const -> bool
{
	assert(n != nullptr);
	return !compare(val, n->value()) && !compare(n->value(), val);
}

// swims (swaps) until restored property
template<typename T, typename C>
auto stable_bset<T,C>::node_swim(node *n) -> void
{
	assert(n != nullptr);
	node *lchild = n->left();
	node *rchild = n->right();
	if(lchild != nullptr && compare(n->value(), lchild->value()))
		node_swap(lchild, n);
	// Note that the following ELSE is what makes this logarithmic in time
	else if(rchild != nullptr && compare(rchild->value(), n->value()))
		node_swap(rchild, n);
	else return;
	node_swim(n); // if did not return need to keep swimming
}

// removes them
template<typename T, typename C>
auto stable_bset<T,C>::predecessor(node *n) -> node*
{
	assert(n != nullptr);
	assert(n->left() != nullptr);
	n = n->left();
	while(n->right())
	{
		n->set_size(n->size()-1);
		n = n->right();
	}
	if(n->left()) // can only have 1 child
	{
		node *child = n->left();
		child->set_parent(nullptr);
		n->set_left(nullptr);
		node_swap(child, n);
	}
	return n;
}

template<typename T, typename C>
auto stable_bset<T,C>::successor(node *n) -> node*
{
	assert(n != nullptr);
	assert(n->right() != nullptr);
	n = n->right();
	while(n->left())
	{
		n->set_size(n->size()-1);
		n = n->left();
	}
	if(n->right()) // can only have 1 child
	{
		node *child = n->right();
		child->set_parent(nullptr);
		n->set_right(nullptr);
		node_swap(child, n);
	}
	return n;
}

// ----- Consistency and printing

template<typename T, typename C>
auto stable_bset<T,C>::_consistency_check() const -> void
{
#ifndef NDEBUG
	if(root == nullptr) return;
	std::unordered_set<node*> visited;
	std::queue<node*> bfsq;
	bfsq.push(root);
	while(!bfsq.empty())
	{
		node *top = bfsq.front();
		assert(visited.insert(top).second); // should not have been there before
		bfsq.pop();
		size_type sum = 1;
		if(top->left())
		{
			bfsq.push(top->left());
			// Check order, parenthood.
			assert(compare(top->left()->value(), top->value()));
			assert(top->left()->parent() == top);
			sum+=top->left()->size();
		}
		if(top->right())
		{
			bfsq.push(top->right());
			// Check order, parenthood.
			assert(compare(top->value(), top->right()->value()));
			assert(top->right()->parent() == top);
			sum+=top->right()->size();
		}
		// Check size
		assert(top->size() == sum);
	}
	assert(visited.size() == size());
#endif /* NDEBUG */
}

template<typename T, typename C>
auto stable_bset<T,C>::_print_tree(std::ostream& o) const -> void
{
	_consistency_check();
	o << "stable_bset@" << this << " size " << size() << '\n';
	if(empty()) return;
	// TODO print ordered version w/ iterator
	std::queue<node*> bfsq;
	bfsq.push(root);
	bfsq.push(nullptr);
	while(!bfsq.empty())
	{
		node *top = bfsq.front();
		bfsq.pop();
		if(top != nullptr)
		{
			if(top->left()) bfsq.push(top->left());
			if(top->right()) bfsq.push(top->right());
			o << top->value() << '(';
			if(top == root) o << "-";
			else o << top->parent()->value();
		 	o << ") ";
		}
		else if(!bfsq.empty())
			{
				o << '\n';
				bfsq.push(nullptr);
			}
	}
	o << '\n';
}

template<typename T, typename C>
std::ostream& operator<<(std::ostream& o, const stable_bset<T,C>& bset)
{
	bset._print_tree(o);
	return o;
}

// ----- Tree algorithms

template<typename T, typename C>
auto stable_bset<T,C>::find_helper(cref_type val, node *n) const -> node*
{
	if(n == nullptr)
		return nullptr;
	if(node_equal(val, n))
		return n;
	if(compare(val, n->value()))
		return find_helper(val, n->left());
	else
		return find_helper(val, n->right());
}

template<typename T, typename C>
auto stable_bset<T,C>::insert_helper(node *free, node *n) -> void
{
	assert(free != nullptr);
	assert(n != nullptr);
	assert(!node_equal(free->value(), n));
	n->set_size(n->size()+1);
	// free < n ?
	if(compare(free->value(), n->value()))
	{
		// Can always add to size 0 subtree
		if(n->left() == nullptr)
			n->set_left(free)->set_parent(n);
		// Immediate swap case - set n as right.
		else if(n->right() == nullptr)
		{
			node_swap(free, n);
			free->set_right(n)->set_parent(free);
			node_swim(free);
		}
		// Can always add to less than or equal subtree
		else if(n->left()->size() <= n->right()->size())
			insert_helper(free, n->left());
		// Swap case: left subtree is larger. Swap w/ n and send down right.
		else
		{
			node_swap(free, n);
			insert_helper(n, free->right());
			// Left child may be greater than free, swap nodes until tree property restored.
			node_swim(free);
		}
	}
	else
	{
		// Can always add to size 0 subtree
		if(n->right() == nullptr)
			n->set_right(free)->set_parent(n);
		// Immediate swap case - set n as left.
		else if(n->left() == nullptr)
		{
			node_swap(free, n);
			free->set_left(n)->set_parent(free);
			node_swim(free);
		}
		// Can always add to less than or equal subtree
		else if(n->right()->size() <= n->left()->size())
			insert_helper(free, n->right());
		// Swap case: right subtree is larger. Swap w/ n and send down left.
		else
		{
			node_swap(free, n);
			insert_helper(n, free->left());
			// Right child may be less than free, swap nodes until tree property restored.
			node_swim(free);
		}
	}
}

// pushes node n down to a leaf position and then deletes it
template<typename T, typename C>
auto stable_bset<T,C>::push_out(node *n) -> void
{
	node *left, *right;
	// push node down to leaf of largest subtree
	while((left = n->left()) || (right = n->right()))
	{
		n->set_size(n->size()-1);
		if(left && right)
			node_swap(n, left->size() > right->size()? left : right);
		else if(left)
			node_swap(n, left);
		else
			node_swap(n, right);
	}
	assert(n->left() == nullptr && n->right() == nullptr);
	node *parent = n->parent();
	if(parent->right() == n)
		parent->set_right(nullptr);
	else
		parent->set_left(nullptr);
	delete n;
}

template<typename T, typename C>
auto stable_bset<T,C>::erase_helper(node *erase, node *current, node *replacement) -> void
{
	assert(erase != nullptr);
	assert(current != nullptr);
	current->set_size(current->size()-1);
	if(erase == current)
	{
		if(replacement == nullptr)
			push_out(erase);
		else
		{
			node_swap(erase, replacement);
			node_swim(replacement);
			delete erase;
		}
	}
	else
	{
		node *left = current->left(), *right = current->right(), *next;
		if(compare(erase->value(), current->value()))
		{
			// need to compensate removal?
			if(replacement == nullptr && left->size() < right->size())
			{
				node_swap(current, successor(current));
				replacement = current;
			}
			next = left;
		}
		else
		{
			// need to compensate removal?
			if(replacement == nullptr && right->size() < left->size())
			{
				node_swap(current, predecessor(current));
				replacement = current;
			}
			next = right;
		}
		node_swap(replacement, current);
		erase_helper(erase, next, current);
	}
}

// ----- Public methods

template<typename T, typename C>
auto stable_bset<T,C>::size() const -> size_type
{
	return root == nullptr? 0 : root->size();
}

template<typename T, typename C>
auto stable_bset<T,C>::empty() const -> bool
{
	return root == nullptr;
}

template<typename T, typename C>
auto stable_bset<T,C>::clear() -> void
{
	delete root;
	root = nullptr;
}

template<typename T, typename C>
auto stable_bset<T,C>::find(cref_type val) const -> bool
{
	_consistency_check();
	return find_helper(val, root);
}

template<typename T, typename C>
auto stable_bset<T,C>::insert(cref_type val) -> bool
{
	_consistency_check();
	// Trivial case
	if(root == nullptr)
	{
		root = new node(val);
		return true;
	}
	// Need to check if in tree due to nature of algorithm
	if(find_helper(val, root)) return false;
	// Proceed with insertion
	node *free = new node(val);
	insert_helper(free, root);
	return true;
}

template<typename T, typename C>
auto stable_bset<T,C>::erase(cref_type val) -> size_type
{
	_consistency_check();
	if(root == nullptr)
		return 0;
	node *n = find_helper(val, root);
	if(n == nullptr)
		return 0;
	erase_helper(n, root, erase);
	return 1;
}

// ----- Array/movable version

/* set implementation requires T is move constructible, uses an array for storing
 * nodes.
 */
template<typename T, typename Compare = std::less<T> >
class bset
{
private:
	struct node;
	typedef std::vector<node> resizeable; // needs to have reserve()
public:
	// Public typedefs
	typedef T value_type;
	typedef typename implementation_utility::easily_copyable<T>::type cref_type;
	typedef typename resizeable::size_type size_type;
private:
	struct node
	{
		typedef typename bset<T, Compare>::size_type size_type;
		typedef implementation_utility::optional<T> optional;
		size_type size;
		optional optional_val;
	};
	resizeable vec;
	// Getting nodes
	INLINE size_type root() const;
	INLINE size_type lchild(size_type parent) const;
	INLINE size_type rchild(size_type parent) const;
	INLINE size_type parent(size_type child) const;
	INLINE node &access(size_type loc); // adds to vector if out of range
	INLINE bool null(size_type node) const;
	// Consistency and printing
	bool _consistency_check() const;
	void _print_tree(std::ostream& o) const;
	template<typename T2, typename C2>
	friend std::ostream& operator<<(std::ostream& o, const bset<T2,C2>& bset);
public:
	// Constructors
	bset(size_type init = 1) :
		vec(init) {vec.emplace_back();} // first is empty
	void insert(value_type&& val);
};

template<typename T, typename C>
auto bset<T,C>::root() const -> size_type
{
	return 1;
}

template<typename T, typename C>
auto bset<T,C>::lchild(size_type parent) const -> size_type
{
	return parent << 1;
}

template<typename T, typename C>
auto bset<T,C>::rchild(size_type parent) const -> size_type
{
	return parent << 1;
}

template<typename T, typename C>
auto bset<T,C>::parent(size_type child) const -> size_type
{
	return child >> 1;
}

template<typename T, typename C>
auto bset<T,C>::access(size_type loc) -> node&
{
	if(loc > vec.size())
	{
		vec.reserve(loc+1);
		while(vec.size() <= loc)
			vec.emplace_back();
	}
	return vec[loc];
}
template<typename T, typename C>
auto bset<T,C>::null(size_type node) const -> bool
{
	return node > vec.size() || !vec[node].valid();
}

template<typename T, typename C>
std::ostream& operator<<(std::ostream& o, const bset<T,C>& bset)
{
	bset._print_tree(o);
	return 0;
}

#endif /* AVL_H_ */





