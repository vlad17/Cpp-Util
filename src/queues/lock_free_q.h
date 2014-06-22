/*
 * Vladimir Feinberg
 * lock_free_q.h
 * 2014-06-15
 *
 * Lock free FIFO queue
 */

#ifndef LOCK_FREE_Q_H_
#define LOCK_FREE_Q_H_

#include "queues/fifo.h"
#include "utilities/atomic_shared_ptr.h"

#include <cassert>
#include <atomic>
#include <utility>
#include <memory>
#include <limits>

// Lock free if shared_ptr is
template<typename T>
class lfqueue : public queue<T>
{
 private:

  struct node
  {
  node(const T& val) : next(nullptr), val(val) {}
  node(T&& val) : next(nullptr), val(val) {}
    std::atomic<node*> next;
    T val;
  };
  std::atomic<node*> head;
  atomic_shared_ptr<node> tail; // avoid ABA problem by using atomic shared_ptr
  void enqueue(node *n) noexcept;
 public:
 lfqueue() : head(nullptr), tail(nullptr) {}
  virtual ~lfqueue() {}
  // Strong guarentee for enqueues only construction can throw
  virtual void enqueue(const T& t) {enqueue(new node(t));}
  virtual void enqueue(T&& t) {enqueue(new node(std::forward<T>(t)));}
  virtual bool dequeue(const std::function<void(T&&)>& visit); // strong guarentee
};

template<typename T>
void lfqueue<T>::enqueue(node *n) noexcept
{
  node *oldhead, *newnext;
  bool first;
  while(
        // Check if empty, get head value
        !(first = head.compare_exchange_weak(oldhead = nullptr, n)) &&
        // Check if oldhead is still head (next is 0)
        !oldhead->next.compare_exchange_weak(newnext = nullptr, n));
  // No other enqueue() thread will change head past this point
  if(first)
    std::atomic_store(&tail, std::shared_ptr<node>{n});
  else // CAS needed here b/c in the tail == head case dequeue may change head
    head.compare_exchange_strong(oldhead, oldhead->next.load());
  // Strong CAS to guarantee head is updated by now to new one
}

// Function call throws b/c of function
template<typename T>
bool lfqueue<T>::dequeue(const std::function<void(T&)>& visit)
{
  // ABA occurences (*) below prevented by using shared_ptr
  std::shared_ptr<node> oldtail = std::atomic_load(&tail);
  node *cpy;
  do {if(oldtail == nullptr) return false;}
  while(!atomic_compare_exchange_weak(&tail,
                                      &oldtail, // ABA below (*)
                                      std::shared_ptr<node>{oldtail->next.load()}));
  // tail == head case (implies tail->next == nullptr)
  head.compare_exchange_strong(cpy = oldtail.get(), nullptr); // ABA here (*)
  // No other enqueue() or dequeue() thread will access oldtail past this point
  // ABA impossible since new will never offer resource pointed to by oldtail
  // until this thread finishes method and destructor deletes.
  visit(oldtail->val);
  return true;
}


#endif /* LOCK_FREE_Q_H_ */
