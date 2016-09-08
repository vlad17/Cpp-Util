/*
  Vladimir Feinberg
  synchro/cdl-test.cpp
  2014-09-14

  Countdown latch unit test.
*/

#include "synchro/countdown_latch.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <future>
#include <numeric>
#include <random>
#include <vector>

#include "util/uassert.hpp"

using namespace std;
using namespace synchro;

int main(int argc, char** argv) {
  unsigned seed;
  if (argc == 2) seed = atoi(argv[1]);
  else seed = time(nullptr);

  cout << "Randomized CDL testing. Seed: " << seed << endl;

  const int kTestSize = 100;
  countdown_latch editing_latch(kTestSize);
  deque<countdown_latch> cdls; // can't move cdls, need deque
  for (int i = 0; i < kTestSize; ++i)
    cdls.emplace_back(i + 1);
  bool started[kTestSize] = {false};
  vector<future<void> > futs;
  for (int i = 0; i < kTestSize; ++i)
    futs.push_back(async(launch::async, [&](int idx) {
          vector<int> wait_order(kTestSize);
          iota(wait_order.begin(), wait_order.end(), 0);
          default_random_engine gen;
          gen.seed(idx + seed);
          random_shuffle(wait_order.begin(), wait_order.end(),
                         [&gen](int max) { return gen() % max; });

          for (bool b : started)
            UASSERT(!b) << "should not start before editing!";
          editing_latch.down();
          editing_latch.wait();
          started[idx] = true;
          for (int i = idx; i < kTestSize; ++i)
            cdls[i].down();
          for (int i = 0; i < kTestSize; ++i) { // TODO randomize this
            cdls[i].wait();
            UASSERT(started[i]);
          }
        }, i));

  for (auto& fut : futs)
    fut.get();

  cout << "Success!" << endl;
}
