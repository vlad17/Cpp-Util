/*
 * Vladimir Feinberg
 * avl.h
 * 2014-01-18
 *
 * Declares and defines the avl class, which is an avl tree that is implemented
 * with a resizable array. This allows for locality of reference and the memory
 * overhead is not too great because the avl tree is so strictly height-balanced.
 */

#ifndef AVL_H_
#define AVL_H_

#include <vector>

// TODO document
// TODO make like set
// TODO add sorted elements constructor (method?)

template<typename T, typename Compare = std::less<T> >
class avl
{
private:
	struct node // TODO use optional. TODO al
	{
		T val;
	};
	typedef std::vector<T> resizeable;


};

#endif /* AVL_H_ */





