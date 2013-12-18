/*
* Vladimir Feinberg
* 2013-10-03
* fibheap.tpp
*
* Contains implementation of fibheap.h methods.
*/

template<typename T, typename C>
template<typename InputIterator>
fibheap<T,C>::fibheap(InputIterator first, InputIterator last,
		const C& comp) :
		min(nullptr), _size(0), comp(comp)
{
	for(InputIterator i = first; i != last; ++i)
		push(i);
}

template<typename T, typename C>
fibheap<T,C>::~fibheap()
{
	clear();
}

template<typename T, typename C>
fibheap<T,C>& fibheap<T,C>::operator=(const fibheap<T,C>& other)
{
	if(this == &other) return *this;
	clear();
	comp = other.comp;
	if(other.empty()) return *this;
	_size = other._size;
	min = new node(other.min->val);
	_copy_subtree(min, other.min);
	const node *traverse = other.min->right;
	node *n = min;
	while(traverse != other.min)
	{
		n->right = new node(traverse->val);
		n->right->left = n;
		n = n->right;
		_copy_subtree(n, traverse);
		traverse = traverse->right;
	}
	n->right = min;
	min->left = n;
	return *this;
}

template<typename T, typename C>
fibheap<T,C>& fibheap<T,C>::operator=(fibheap<T,C>&& other)
{
	clear();
	min = other.min;
	_size = other._size;
	comp = std::move(other.comp);
	other.min = nullptr;
	other._size = 0;
	return *this;
}

// size is nonzero
template<typename T, typename C>
constexpr size_t fibheap<T,C>::approx_childnum(size_t size)
{
	return size < 2? size : (size_t) log((double) size)/log(PHI);
}

template<typename T, typename C>
void fibheap<T,C>::_print_fibheap(std::ostream& o) const
{
	_consistency_check();
	o << "Fibheap @ " << this << ", size " << _size;
	if(!empty()) o << ", top " << top();
	o << '\n';
	if(empty()) return;
	std::queue<const node*> bfsqueue;
	const node *n = min;
	do
	{
		bfsqueue.push(n);
		n = n->right;
	} while(n != min);
	
	bfsqueue.push(nullptr); // signify newlines with nullptr
	
	while(!bfsqueue.empty())
	{
		n = bfsqueue.front();
		if(n == nullptr)
		{
			o << '\n';
			if(bfsqueue.size() > 1)
				bfsqueue.push(nullptr);
		}
		else
		{
			o << n->val << '(';
			if(n->up == nullptr) o << '-';
			else o << n->up->val;
			o << ')' << ' ';
			if(n->down != nullptr)
			{
				const node *child = n->down;
				do
				{
					bfsqueue.push(child);
					child = child->right;
				} while(child != n->down);
			}
		}
		bfsqueue.pop();
	}
	
	o << '\n';
}

template<typename T, typename C>
typename fibheap<T,C>::key_type fibheap<T,C>::push(const value_type& p)
{
	_consistency_check();
	node *added = new node(p);
	++_size;
	_insert_new_node(added);
	return static_cast<key_type>(added);
}

template<typename T, typename C>
template<typename... Args>
void fibheap<T,C>::emplace(Args&&... args)
{
	_consistency_check();
	node *added = new node(std::forward<Args>(args)...);
	++_size;
	_insert_new_node(added);
	return static_cast<key_type>(added);
}

// keeps key the same
template<typename T, typename C>
void fibheap<T,C>::decrease_key(key_type key, const value_type& val)
{
	assert(key != nullptr);
	_consistency_check();
	node* changed = static_cast<node*>(const_cast<void*>(key));
	assert(comp(val, changed->val));
	changed->val = val;
	changed->marked = false;
	if(changed->up != nullptr)
	{
		node *n = changed->up;
		_rlt_cut(changed);
		_rl_splice(min, changed);
		while(n->up != nullptr)
		{
			if(!n->marked)
			{
				n->marked = true;
				break;
			}
			n->marked = false;
			node *tmp = n->up;
			_rlt_cut(n);
			_rl_splice(min, n);
			n = tmp;
		}
	}
	if(comp(changed->val, min->val))
		min = changed;
}

template<typename T, typename C>
void fibheap<T,C>::decrease_key(key_type key, value_type&& val)
{
	assert(key != nullptr);
	_consistency_check();
	node* changed = static_cast<node*>(const_cast<void*>(key));
	changed->val = std::forward<value_type>(val);
	changed->marked = false;
	if(changed->up != nullptr)
	{
		node *n = changed->up;
		_rlt_cut(changed);
		_rl_splice(min, changed);
		while(n->up != nullptr)
		{
			if(!n->marked)
			{
				n->marked = true;
				break;
			}
			n->marked = false;
			node *tmp = n->up;
			_rlt_cut(n);
			_rl_splice(min, n);
			n = tmp;
		}
	}
	if(comp(changed->val, min->val))
		min = changed;
}

template<typename T, typename C>
void fibheap<T,C>::clear()
{
	_consistency_check();
	if(empty()) return;
	node *del = min;
	do
	{
		_delete_subtree(del);
		node *tmp = del;
		del = del->right;
		delete tmp;
	}
	while(del != min);
	_size = 0;
	min = nullptr;
}

template<typename T, typename C>
auto fibheap<T,C>::pop() -> key_type
{
	_consistency_check();
	// 0 nodes
	if(empty()) return nullptr;
	key_type key = static_cast<key_type>(min);
	--_size;
	// Add children to new top layer
	node *top = min->right;
	if(top == min) top = min->down;
	else
	{
		_rl_cut(min);
		if(min->down != nullptr)
			_rl_splice(top, min->down);
	}
	// delete min, note min->down children up == min still
	delete min;
	min = top;
	if(top == nullptr) return nullptr; // now empty
	// Go through roots (which must costartntain new min). Also minimize # of roots.
	std::vector<node*> trees(approx_childnum(_size));
	while(true)
	{
		// consistency
		top->up = nullptr;
		top->marked = false;
		auto next = _join_nodes(trees, top);
		if(next.first) top = next.second;
		if(comp(top->val, min->val))
			min = top;
		else if(!comp(min->val, top->val)) min = top; // need to keep min on top
		top = top->right;
		if(trees.size() > top->num_children && trees[top->num_children] == top)
			break;
	}
	return key;
}

/*
 * legal state:
 * min->up == nullptr
 * min == nullptr && _size == 0 if head == nullptr
 * size is consistent.
 * no top-down cycles. strict grid relation (n1->left != any node left or
 * 	right of n1->up or n1->down).
 * all nodes are circularly linked
 */
template<typename T, typename C>
void fibheap<T,C>::_consistency_check() const
{
	if(_size == 0)
	{
		assert(min == nullptr);
		return;
	}
	assert(min != nullptr);
	assert(min->left != nullptr && min->right != nullptr);
	assert(min->up == nullptr);
#if FIB_CHECK
	std::unordered_set<const node*> set;
	_tree_check(min, set);
#endif /* FIB_CHECK */
}
// does a bfs for L/R nonnull and consistency, adding viewed nodes to
// the set so that later on lower levels are not linked to higher ones.
// also checks for parent consistency. Returns num children.
#if FIB_CHECK
template<typename T, typename C>
size_t fibheap<T,C>::_tree_check(const node *root,
		std::unordered_set<const node*>& s)
{
	if(root == nullptr) return 0;
	const node *n = root;
	do
	{
		assert(s.find(n) == s.end());
		assert(n->left != nullptr);
		assert(n->right != nullptr);
		assert(n->right->left == n);
		assert(n->right->up == n->up);
		s.insert(n);
		n = n->right;
	} while(n != root);
	size_t ctr = 0;
	do
	{
		++ctr;
		size_t actual_num_children = _tree_check(n->down, s);
		assert(actual_num_children == n->num_children);
		n = n->right;
	} while (n != root);
	return ctr;
}
#endif /* FIB_CHECK */

template<typename T, typename C>
void fibheap<T,C>::_rl_cut(node *n)
{
	assert(n != nullptr);
	n->right->left = n->left;
	n->left->right = n->right;
	n->left = n->right = n;
}

template<typename T, typename C>
void fibheap<T,C>::_rlt_cut(node *n)
{
	assert(n != nullptr);
	assert(n->up != nullptr);
	node *next = n->right;
	_rl_cut(n);
	if(n->up->down == n)
		n->up->down = n == next? nullptr : next;
	--n->up->num_children;
	n->up = nullptr;
}

template<typename T, typename C>
void fibheap<T,C>::_rl_splice(node *main, node *insert)
{
	assert(main != nullptr);
	assert(insert != nullptr);
	node *r_main = main->right, *r_ins = insert->left;
	assert(main != nullptr);
	assert(insert != nullptr);
	main->right = insert;
	insert->left = main;
	r_main->left = r_ins;
	r_ins->right = r_main;
}

// indep of min
// returns whether need to re-cycle, and the top-level processed node
template<typename T, typename C>
auto fibheap<T,C>::_join_nodes(std::vector<node*>& trees, node *n) -> std::pair<bool, node*>
{
	while(n->num_children >= trees.size())
		trees.push_back(nullptr);
	node *&same_deg = trees[n->num_children];
	if(same_deg == nullptr)
	{
		same_deg = n;
		return std::make_pair(false, n);
	}
	node *parent, *child;
	if(comp(same_deg->val, n->val))
	{
		parent = same_deg;
		child = n;
	}
	else
	{
		parent = n;
		child = same_deg;
	}
	_rl_cut(child); // suffices for rlt cut as all nodes in trees are top level.
	_rlt_splice(parent, child);
	same_deg = nullptr;
	return std::make_pair(true, _join_nodes(trees, parent).second);
}

// child should be _rlt_cut
template<typename T, typename C>
void fibheap<T,C>::_rlt_splice(node *parent, node *child)
{
	assert(parent != nullptr);
	assert(child != nullptr);
	++parent->num_children;
	if(parent->down == nullptr) parent->down = child;
	else _rl_splice(parent->down, child);
	child->up = parent;
}

template<typename T, typename C>
void fibheap<T,C>::_delete_subtree(node *n)
{
	assert(n != nullptr);
	if(n->down == nullptr) return;
	node *del = n->down;
	do
	{
		_delete_subtree(del);
		node *tmp = del;
		del = del->right;
		delete tmp;
	} while(del != n->down);
}

template<typename T, typename C>
void fibheap<T,C>::_copy_subtree(node *cpy, const node *n)
{
	assert(cpy != nullptr);
	assert(n != nullptr);
	if(n->down == nullptr) return;
	cpy->down = new node(n->down->val);
	cpy->down->up = cpy;
	++cpy->num_children;
	_copy_subtree(cpy->down, n->down);
	const node *traverse = n->down->right;
	node *childcpy = cpy->down;
	while(traverse != n->down)
	{
		childcpy->right = new node(traverse->val);
		++cpy->num_children;
		childcpy->right->left = childcpy;
		childcpy = childcpy->right;
		childcpy->up = cpy;
		_copy_subtree(childcpy, traverse);
		traverse = traverse->right;
	}
	childcpy->right = cpy->down;
	cpy->down->left = childcpy;
}

template<typename T, typename C>
void fibheap<T,C>::_insert_new_node(node *added)
{
	if(min == nullptr) min = added;
	else
	{
		_rl_splice(min, added);
		if(comp(added->val, min->val))
			min = added;
	}
}