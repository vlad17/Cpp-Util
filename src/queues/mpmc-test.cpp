/*
 * Vladimir Feinberg
 * mpmc-test.cpp
 * 2014-09-06
 *
 * Defines tests for mpmc synchronoized queues.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "utilities/nullstream.h"
#include "utilities/test_util.h"
#include "utilities/timer.h"
#include "queues/queue.h"
#include "queues/shared_queue.h"

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

template<template<typename> class T>
void unit_test();

template<template<typename> class T, bool BENCH>
void test_multithreaded();

vector<int> read_strvec(string);

void test_main(int argc, char** argv) {
  // Bench if argument passed
  bool bench = argc > 1 && strcmp(argv[1], "bench") == 0;
  cout << "Queue Test..." << endl;
  cout << "\nShared Queue" << endl;
  if (!bench) {
    cout << "\tUnit testing:" << endl;
    unit_test<shared_queue>();
    cout << "\tMultithreaded test:" << endl;
    test_multithreaded<shared_queue, false>();
  } else {
    cout << "\tBenching (multithreaded test x100): ";
    cout.flush();
    TIME_BLOCK(std::chrono::milliseconds, "") {
      test_multithreaded<shared_queue, true>();
    }
  }
}

// TODO wrap s if longer that 40 chars (and adjust below test names)
void start(const string& s, std::ostream& outs = cout) {
  outs << '\t' << left << setw(40);
  outs << s;
  outs.flush();
}

void complete(const string& s = "...complete", std::ostream& outs = cout) {
  outs << right << s << endl;
}

// X should print [...]
template<class X>
bool same(X& x, const vector<int>& vec) {
  stringstream vecstream;
  vecstream << "[";
  if (!vec.empty()) {
    vecstream << vec.front();
    for (int i = 1 ; i < vec.size(); ++i)
      vecstream << ", " << vec[i];
  }
  vecstream << "]";
  stringstream xstream;
  xstream << x;
  string xstr = xstream.str();
  string vstr = vecstream.str();
  return vecstream.str() == xstream.str();
}

template<class X>
string as_string(const X& x) {
  stringstream sstr;
  sstr << x;
  return sstr.str();
}

template<template<typename> class T>
void unit_test() {
  T<int> t;
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  ASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}), "\n\t[t = " + as_string(t));
  ASSERT(!t.empty());
  complete();
  start("Remove 0..9");
  for(int i = 0; i < 10; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO");
  ASSERT(same(t, {}), "\n\t[t = " + as_string(t));
  ASSERT(t.empty());
  complete();
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  ASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}), "\n\t[t = " + as_string(t));
  ASSERT(!t.empty());
  complete();
  start("Remove 0..4");
  for(int i = 0; i < 5; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO (partial remove)");
  ASSERT(same(t, {5, 6, 7, 8, 9}), "\n\t(t = " + as_string(t) + ")");
  ASSERT(!t.empty());
  complete();
  start("Insert 10..14");
  for(int i = 10; i < 15; ++i)
    t.enqueue(i);
  ASSERT(same(t, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}),
         "\n\t[t = " + as_string(t));
  complete();
  start("Remove 5..14");
  for(int i = 5; i < 15; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO (remove after being empty)");
  ASSERT(same(t, {}), "\n\t[t = " + as_string(t));
  ASSERT(t.empty());
  complete();
  start("Non-empty delete (valgrind)");
  {
    T<int> t;
    for (int i = 0; i < 10; ++i)
      t.enqueue(i);
  }
  complete();
  start("");
  complete("...........Success!");
}

// reads in [.., .., .., ..] format
vector<int> read_strvec(string s) {
  replace(s.begin(), s.end(), ',', ' ');
  s.pop_back();
  istringstream in(s);
  in.get();
  vector<int> out;
  while (!in.eof()) {
    int x = 0;
    in >> x;
    out.push_back(x);
  }
  return out;
}

bool in_range(int i, int lo, int hi) {
  return lo <= i && i < hi;
}

tuple<int, int, int> interval_mid(int idx, int per) {
  int start = per * idx;
  int mid = start + per / 2;
  int end = start + per;
  return make_tuple(start, mid, end);
}

tuple<int, int> interval(int idx, int per) {
  int start = per * idx;
  int end = start + per;
  return make_tuple(start, end);
}

template<template<typename> class T, bool BENCH>
void test_multithreaded() {
  // Use larger values and don't print to cout if BENCH
  static const int kBenchMultiplier = BENCH ? 100 : 1;
  auto& outs = BENCH ? nullstream : cout;

  static const int kNumEnqueuers = 10;
  static const int kItemsPerEnqueuer = 2000 * kBenchMultiplier;
  outs << "\tTesting concurrent enqueues and validity, "
       << kNumEnqueuers << " enqueuers x " << kItemsPerEnqueuer << " items"
       << endl;

  // Have each enqueuer fill with a unique # of items, then check that
  // they're all present (and queue's not empty). Relies on the fact
  // that the queue's safe to traverse so long as no dequeue operation
  // is going on.

  // Gives unique interval for concurrent inserters.
  start("concurrent inserts", outs);

  atomic<int> cdl(kNumEnqueuers); // todo replace with a countdown latch
  T<int> testq;
  ASSERT(testq.empty());
  typedef tuple<vector<int>, vector<int>, vector<int> > threevec;
  vector<future<threevec> > futs;
  for (int i = 0; i < kNumEnqueuers; ++i)
    futs.push_back(async(launch::async,
                         [&](int idx) -> threevec {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));
          int start, mid, end;
          tie(start, mid, end) = interval_mid(idx, kItemsPerEnqueuer);
          auto before = read_strvec(as_string(testq));

          for (int i = start; i < mid; ++i)
            testq.enqueue(i);
          auto half = read_strvec(as_string(testq));

          for (int i = mid; i < end; ++i)
            testq.enqueue(i);
          auto after = read_strvec(as_string(testq));

          return make_tuple(before, half, after);
        }, i));

  for (int i = 0; i < kNumEnqueuers; ++i) {
    vector<int> before, half, after;
    tie(before, half, after) = futs[i].get();
    int start, mid, end;
    tie(start, mid, end) = interval_mid(i, kItemsPerEnqueuer);

    for (int j : before)
      ASSERT(!in_range(j, start, end));

    int current = start;
    for (int j : half)
      if (in_range(j, start, end))
        ASSERT(j == current++, "Expected fifo order ("
               << j << " != " << (current - 1) << ")");
    ASSERT(current == mid, "exactly half should be present ["
           << start << ", " << mid << ")");

    current = start;
    for (int j : after)
      if (in_range(j, start, end))
        ASSERT(j == current++, "Expected fifo order ("
               << j << " != " << (current - 1) << ")");
    ASSERT(current == end, "all should be present ["
           << start << ", " << end << ")");
  }

  complete("...complete", outs);

  // Keep the same constants so we have exactly as many items removed
  // as were put in (don't make dequeuers dequeue empty)
  static const int kNumDequeuers = kNumEnqueuers;
  static const int kItemsPerDequeuer = kItemsPerEnqueuer;
  outs << "\tTesting concurrent dequeues and validity, "
       << kNumDequeuers << " dequeuers x " << kItemsPerDequeuer << " items"
       << endl;
  start("concurrent pops", outs);

  // Dequeuers just check that they read everything in ascending order
  // (not totally, but within the resolution of each enqueuer).
  vector<future<void> > dfuts;
  cdl.store(kNumDequeuers);
  for (int i = 0; i < kNumDequeuers; ++i)
    dfuts.push_back(async(launch::async, [&]() {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          int latest[kNumEnqueuers];
          fill(latest, latest + kNumEnqueuers, -1);
          vector<int> seen; // save results in vector to encourage contention.
          seen.reserve(kItemsPerDequeuer);

          for (int j = 0; j < kItemsPerDequeuer; ++j)
            seen.push_back(testq.dequeue());

          for (int val : seen) {
            int enqueuer = val / kItemsPerEnqueuer;
            ASSERT(val >= 0 && enqueuer < kNumEnqueuers, "Value " << val
                   << " could not have been enqueued (" << kNumEnqueuers
                   << " enqueuers)");
            ASSERT(latest[enqueuer] < val, "Expected fifo order, saw "
                   << latest[enqueuer] << " before " << val);
            latest[enqueuer] = val;
          }
        }));

  for (auto& fut : dfuts)
    fut.get();

  complete("...complete", outs);

  // Dequeued all, should be mpty
  start("Testing empty", outs);
  ASSERT(testq.empty());
  complete("...complete", outs);

  static const int kRandomizedNumEnqueuers = 3;
  static const int kRandomizedNumDequeuers = 4;
  static const int kInterval = 10 * kBenchMultiplier;
  static const int kDesiredRandomizedNum = 1000 * kBenchMultiplier;
  static const int kRandomizedNum =
    (kDesiredRandomizedNum / kRandomizedNumEnqueuers) * kRandomizedNumEnqueuers;
  outs << "\tEnqueue/Dequeue testing (pause after enqueuing " << kInterval
       << " to encourage dequeuers),\n"
       << "\t\tenqueue: " << kRandomizedNumEnqueuers << "\n"
       << "\t\tdequeue: " << kRandomizedNumDequeuers << " (test empty edge case)\n"
       << "\t\tNumber of items: " << kRandomizedNum << endl;
  start("Enqueue/dequeue", outs);

  cdl.store(kRandomizedNumEnqueuers + kRandomizedNumDequeuers);

  vector<future<void> > refuts;
  const static int kRandomizedPerEnqueuer = kRandomizedNum / kRandomizedNumEnqueuers;
  for (int i = 0; i < kRandomizedNumEnqueuers; ++i)
    refuts.push_back(async(launch::async, [&](int idx) {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          int start, end;
          tie(start, end) = interval(idx, kRandomizedPerEnqueuer);
          for (int j = start; j < end; ++j) {
            testq.enqueue(j);
            if (j % kInterval)
              this_thread::sleep_for(chrono::nanoseconds(10));
          }
        }, i));

  vector<future<void > > rdfuts;
  const static int kRandomizedPerDequeuer = kRandomizedNum / kRandomizedNumDequeuers;
  for (int i = 0; i < kRandomizedNumDequeuers; ++i)
    rdfuts.push_back(async(launch::async, [&]() {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          int latest[kRandomizedNumEnqueuers];
          fill(latest, latest + kRandomizedNumEnqueuers, -1);
          vector<int> seen; // save results in vector to encourage contention.
          seen.reserve(kRandomizedPerDequeuer);

          for (int j = 0; j < kRandomizedPerDequeuer; ++j)
            seen.push_back(testq.dequeue());

          for (int val : seen) {
            int enqueuer = val / kRandomizedPerEnqueuer;
            ASSERT(val >= 0 && enqueuer < kRandomizedNumEnqueuers, "Value " << val
                   << " could not have been enqueued (" << kRandomizedNumEnqueuers
                   << " enqueuers)");
            ASSERT(latest[enqueuer] < val, "Expected fifo order, saw "
                   << latest[enqueuer] << " before " << val);
            latest[enqueuer] = val;
          }
        }));

  // Leftovers
  int dequeued = kRandomizedPerDequeuer * kRandomizedNumDequeuers;
  for (int i = dequeued; i < kRandomizedNum; ++i)
    testq.dequeue();

  for (auto& fut : refuts)
    fut.get();
  for (auto& fut : rdfuts)
    fut.get();

  complete("...complete", outs);

  start("Test empty", outs);
  ASSERT(testq.empty());
  complete("...complete", outs);

  start("", outs);
  complete("...........Success!", outs);
}
