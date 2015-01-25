/*
  Vladimir Feinberg
  synchro/countdown_latch.hpp
  2014-09-14

  Declares the countdown latch class.
*/

#ifndef SYNCHRO_COUNTDOWN_LATCH_HPP_
#define SYNCHRO_COUNTDOWN_LATCH_HPP_

#include <condition_variable>
#include <mutex>

namespace synchro {

// Allows synchronization accorss more than 2 threads.
// Calling down() counts down the unready count. While the
// unready count is nonzero, all threads inside wait() are blocked.
// As soon as the last thread calls down(), all blocked threads are released.
//
// This class is thread safe.
class countdown_latch {
 public:
  countdown_latch(int wait); // requires wait > 0
  ~countdown_latch(); // releases all waiting threads.
  void down(); // does nothing if count is at 0
  void wait();
  // Reset presumes there are no wait()-ing threads.
  void reset(int wait);
 private:
  std::mutex lock_;
  int unready_count_;
  std::condition_variable ready_;
};

} // namespace synchro

#endif /* SYNCHRO_COUNTDOWN_LATCH_HPP_ */
