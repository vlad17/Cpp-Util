# Cpp-Util

This is a collection of some data structures I have been working on recently.

### Bragging

My MPMC queue is on par with Boost's lockfree::queue! Stay tuned as I optimize
mine further. See benchmark at the bottom of this document.

### Compilation

Requires cmake 2.6+ and a C++11 supporting compiler.

Requires boost installed and available in system headers.

To run tests:

    cmake . # specify -DCMAKE_CXX_COMPILER=... if it's not g++
    make -j`nproc`
    ctest -j`nproc`

Other cmake options are:

  * `-DCMAKE_BUILD_TYPE=(debug|release)` - specifies build setting
  * `-DASSERTIONS=(0|1|2)` - specifies whether to turn on assertions, on by default
  for debug and off by default for release. Default behavior is 0 (or unset flag)
  Manual enabling is possible by setting 1, disabling with 2.
        
For quick building and testing, use `chmod +x build-and-test.sh`
Then `./build-and-test.sh`.

### Overview

##### src/caches
`lfu_cache.hpp`: least frequently used cache

##### src/fibheap
`fibheap.hpp`: fibonacci min-heap

##### src/queues
`hazard_queue.hpp`: lock-free MPMC queue via hazard pointer implementation

`shared_queue.hpp`: MPMC queue via shared pointer implementation (relies on `shared_ptr` atomics)

##### src/sychro
`hazard.hpp`: hazard pointers

`atomic_shared.hpp`: wrapper for GCC's lack of atomic shared pointer support in 4.8

`countdown_latch.hpp`: countdown latch (from Java SE7)

--various pthreads wrappers for RAII--

##### src/util
`util.hpp`: misc. utils
`nullstream.hpp`: stream that eats tokens
`atomic_optional.hpp`: thread-safe optional container
`optional.hpp`: my version of what is currently `std::experimental::optional`
`timer.hpp`: convenience macro for timing a block
`radix.hpp`: radix sorting
`uassert.hpp`: poor man's gTest placeholder.

### TODO

  * For speedup, notice we can just always choose to create a new hazard_record. Investigate via valgrind why memory consumption for `hazard_queue` is so large.
  * Investigate if hazard pointer re-use before deletion creates a leak in the rlist (fix by using a set instead of vector for rlist)
  * `hazard_queue::is_lock_free` overload.
  * try using spinlocks instead of mutexes in the `atomic_shared_ptr` (the lock doesn't have to protect much, so it's probably better if you generate a lot of `atomic_shared_ptrs`).
  * Try to beat the `boost::lockfree::queue`
  * document throws for pthreads (look at the "too many readers" return val, maybe spin?)
  * cache and fibheap are terribly slow. speed them up.
  * implement SingularThreadpool (1t) (mpsc)
  * WorkStealingThreadpool # (using mpmc for each thread)
  * clean up TODOs in code
  * create test-testandset spinlock, and then queuelock (with everyhting, Art of mpp ch.7) 
  * Test against boost spinlock
	concurrent `heap_cache` (finish testing & debugging)
	`exact_heap_cache` (see `lfu_cache.h`) - make separate method in main for comparison/stress
	`linked_cache` (see `lfu_cache.h`)
	-> concurrent versions of `exact_heap_cache` and `linked_cache` (clean up the entire mess)
	fibheap (check file for TODOs)
	Use a lazily updated bool instead of `insert_version_` and `remove_version` to denote "empty" in `shared_queue.hpp` (also `hazard_queue.hpp`)
  * Optimize MSD sort
  * MSD 3-way quicksort (in place?)
  * More radix sorts (LSD, MSD with auxilary memory)
  * Future improvements to hazard pointers would be a global GC mechanism that cleans up the global_list_.
  * suffix tries + ukkonen construct

### MPMC (Multi-producer Multi-consumer) Queue Benchmark

Run `./release/mpmc-test.exe bench boost`. Results were from an i7 Intel Nehalem (64-bit, on Ubuntu 14.04).

    Boost Queue
	    Enqueues (8x100000): 129ms
	    Dequeues (8x100000): 83ms
	    Fair mpmc (~1000000 items, 4 enq, 4 deq): 227ms
	    Enqueue-weighted mpmc (~1000000 items, 6 enq, 2 deq): 146ms
	    Dequeue-weighted mpmc (~1000000 items, 2 enq, 6 deq): 190ms
    
    Shared Queue
	    Enqueues (8x100000): 654ms
	    Dequeues (8x100000): 810ms
	    Fair mpmc (~1000000 items, 4 enq, 4 deq): 1185ms
	    Enqueue-weighted mpmc (~1000000 items, 6 enq, 2 deq): 998ms
	    Dequeue-weighted mpmc (~1000000 items, 2 enq, 6 deq): 1209ms
    
    Hazard Queue
	    Enqueues (8x100000): 135ms
	    Dequeues (8x100000): 255ms
	    Fair mpmc (~1000000 items, 4 enq, 4 deq): 176ms
	    Enqueue-weighted mpmc (~1000000 items, 6 enq, 2 deq): 178ms
	    Dequeue-weighted mpmc (~1000000 items, 2 enq, 6 deq): 232ms
    
    
