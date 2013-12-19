/*
* Vladimir Feinberg
* cache.h
* 2013-12-18
*
* Declares block_ptr class, which allows for faster use of memory of
* fixed-size blocks by maintaining a contiguous block of memory.
*/

#ifndef BLOCK_PTR_H_
#define BLOCK_PTR_H_

#include <queue>
#include <vector>

// TODO add locks

// Type must be movable
template<typename T>
class block_ptr
{
private:
	typedef unsigned index_type;
	// fixed_allocator for each type T
	class fixed_allocator
	{
	private:
		typedef block_ptr<T>::index_type index_type;
		std::vector<T> store;
		std::queue<index_type> freelist;
	public:
		fixed_allocator() :
			store(4096/sizeof(T)), freelist() {};
		// data races: no dereferencing allowed during construction
		template<typename... Args>
		index_type construct(Args&& args...);
		void destruct(index_type i);
		// data races: no dereferencing allowed during construction
		T& operator[](index_type i);
		const T& operator[](index_type i) const;
	};
	static fixed_allocator allocator {};
	index_type index;
public:
	template<typename... Args>
	block_ptr(Args&& args...) :
		index(allocator.construct(std::forward(args))) {}
	// refrences, pointers only valid during call
	T& operator*() { return allocator[index];}
	const T& operator*() const { return allocator[index];}
	T *operator->() {return &**this;}
	const T *operator->() const {return &**this;}
	~block_ptr()
	{
		allocator.destruct(index);
	}
};

// data races: no dereferencing allowed during construction
// may invalidate pointers, not indices.
template<typename T>
template<typename... Args>
auto block_ptr<T>::fixed_allocator::construct(Args&& args...) -> index_type
{
	index_type i;
	if(freelist.empty())
	{
		i = store.size();
		store.emplace_back(args);
		return i;
	}
	i = freelist.front();
	store[i] = T(args);
	freelist.pop();
	return i;
}

template<typename T>
void block_ptr<T>::fixed_allocator::destruct(index_type i)
{
	freelist.push(i);
}

// data races: no dereferencing allowed during construction
template<typename T>
T& block_ptr<T>::fixed_allocator::operator[](index_type i)
{
	return store[i];
}
template<typename T>
const T& block_ptr<T>::fixed_allocator::operator[](index_type i) const
{
	return store[i];
}

#endif /* BLOCK_PTR_H_ */
