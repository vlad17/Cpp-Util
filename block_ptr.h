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
		template<typename... Args>
		index_type construct(Args&& args...);
		void destruct(index_type i);
	};
	static fixed_allocator allocator {};
	index_type index;
public:
	template<typename... Args>
	block_ptr(Args&& args...) :
		index(allocator.construct(std::forward(args))) {}
	~block_ptr()
	{
		allocator.destruct(index);
	}
};

#endif /* BLOCK_PTR_H_ */
