/*
  Vladimir Feinberg
  synchro/cdl-test.cpp
  2014-09-14

  Countdown latch unit test.
*/

#include "synchro/countdown_latch.hpp"

#include <deque>
#include <future>
#include <vector>

#include "util/test_util.hpp"

using namespace std;
using namespace synchro;

void test_main(int, char**) {
  cout << "Randomized CDL testing" << endl;

  const int kTestSize = 100;
  countdown_latch editing_latch(kTestSize);
  deque<countdown_latch> cdls; // can't move cdls, need deque
  for (int i = 0; i < kTestSize; ++i)
    cdls.emplace_back(i + 1);
  bool started[kTestSize] = {false};
  vector<future<void> > futs;
  for (int i = 0; i < kTestSize; ++i)
    futs.push_back(async(launch::async, [&](int idx) {
          for (bool b : started)
            ASSERT(!b, "should not start before editing!");
          editing_latch.down();
          editing_latch.wait();
          started[idx] = true;
          for (int i = idx; i < kTestSize; ++i)
            cdls[i].down();
          for (int i = 0; i < kTestSize; ++i) { // TODO randomize this
            cdls[i].wait();
            ASSERT(started[i]);
          }
        }, i));

  for (auto& fut : futs)
    fut.get();

  cout << "Success!" << endl;
}
