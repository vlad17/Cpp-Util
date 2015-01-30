/*
 * Vladimir Feinberg
 * queues/mpmc-test.cpp
 * 2014-09-08
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
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#ifdef HAVE_BOOST
#include <boost/lockfree/queue.hpp>
#endif /* HAVE_BOOST */

#include "util/timer.hpp"
#include "util/uassert.hpp"
#include "queues/queue.hpp"
#include "queues/shared_queue.hpp"
#include "queues/hazard_queue.hpp"

using namespace std;
using namespace queues;

template<template<typename> class T>
void unit_test();

template<template<typename> class T>
void test_multithreaded();

template<template<typename> class T>
void bench_queue();

#ifdef HAVE_BOOST
template<typename T>
class boost_queue {
 public:
  boost_queue() {}
  void enqueue(T t) {
    impl.push(t);
  }
  void dequeue() {
    int i;
    // Boost only provides the equivalent of a 'try_dequeue'.
    // The following loop mimics the same thing that 'dequeue' does.
    while (!impl.pop(i))
      std::this_thread::yield();
  }
  bool empty() { return impl.empty(); }
 private:
  boost::lockfree::queue<T> impl;
};
#endif /* HAVE_BOOST */

template<class T> string prvec(const T&);
vector<int> read_strvec(string);

int main(int argc, char** argv) {
  // Bench if argument passed
  bool bench = argc > 1 && strcmp(argv[1], "bench") == 0;
#ifdef HAVE_BOOST
  bool boost = argc > 2 && strcmp(argv[2], "boost") == 0;
#endif /* HAVE_BOOST */
  if (!bench) {
    cout << "MPMC Queue Unit Test..." << endl;
    cout << "\nShared Queue" << endl;
    cout << "\tUnit testing:" << endl;
    unit_test<shared_queue>();
    cout << "\tMultithreaded test:" << endl;
    test_multithreaded<shared_queue>();
  } else {
    cout << "MPMC Queue Benchmark" << endl;
#ifdef HAVE_BOOST
    if (boost) {
      cout << "\nBoost Queue" << endl;
      bench_queue<boost_queue>();
    }
#endif /* HAVE_BOOST */
    cout << "\nShared Queue" << endl;
    bench_queue<shared_queue>();
  }
  return 0;
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

template<typename C>
string prvec(const C& vec) {
  stringstream vecstream;
  vecstream << "[";
  if (!vec.empty()) {
    auto it = vec.begin();
    vecstream << *it++;
    for (; it != vec.end(); ++it)
      vecstream << ", " << *it;
  }
  vecstream << "]";
  return vecstream.str();
}

template<class X>
string as_string(const X& x) {
  stringstream sstr;
  sstr << x;
  return sstr.str();
}

// X should print [...]
template<class X>
bool same(const X& x, const vector<int>& vec) {
  return as_string(x) == prvec(vec);
}

template<template<typename> class T>
void unit_test() {
  T<int> t;
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  UASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}))
      << ("\n\t[t = " + as_string(t));
  UASSERT(!t.empty());
  complete();
  start("Remove 0..9");
  for(int i = 0; i < 10; ++i)
    UASSERT(t.dequeue() == i) << "queue not FIFO";
  UASSERT(same(t, {})) << ("\n\t[t = " + as_string(t));
  UASSERT(t.empty());
  complete();
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  UASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}))
      << ("\n\t[t = " + as_string(t));
  UASSERT(!t.empty());
  complete();
  start("Remove 0..4");
  for(int i = 0; i < 5; ++i)
    UASSERT(t.dequeue() == i) << "queue not FIFO (partial remove)";
  UASSERT(same(t, {5, 6, 7, 8, 9}))
      << ("\n\t(t = " + as_string(t) + ")");
  UASSERT(!t.empty());
  complete();
  start("Insert 10..14");
  for(int i = 10; i < 15; ++i)
    t.enqueue(i);
  UASSERT(same(t, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}))
      << ("\n\t[t = " + as_string(t));
  complete();
  start("Remove 5..14");
  for(int i = 5; i < 15; ++i)
    UASSERT(t.dequeue() == i) << "queue not FIFO (remove after being empty)";
  UASSERT(same(t, {}))
      << ("\n\t[t = " + as_string(t));
  UASSERT(t.empty());
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
  if (s.size() == 1) return out;
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

template<template<typename> class T>
void test_multithreaded() {
  static const int kNumEnqueuers = 10;
  static const int kItemsPerEnqueuer = 2000;
  cout << "\tTesting concurrent enqueues and validity, "
       << kNumEnqueuers << " enqueuers x " << kItemsPerEnqueuer << " items"
       << endl;

  // Have each enqueuer fill with a unique # of items, then check that
  // they're all present (and queue's not empty). Relies on the fact
  // that the queue's safe to traverse so long as no dequeue operation
  // is going on.

  // Gives unique interval for concurrent inserters.

  start("concurrent inserts");

  atomic<int> cdl(kNumEnqueuers); // todo replace with a countdown latch
  T<int> testq;
  UASSERT(testq.empty());
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
      UASSERT(!in_range(j, start, end))
          << "Found " << j << " before [" << start << ", " << end << ")"
          << " dequeuer started.";


    int current = start;
    for (int j : half)
      if (in_range(j, start, end)) {
        UASSERT(j == current) << "Expected fifo order ("
                              << j << " != " << current << ")";
        current++;
      }
    UASSERT(current == mid) << "exactly half should be present ["
           << start << ", " << mid << ")";

    current = start;
    for (int j : after)
      if (in_range(j, start, end)) {
        UASSERT(j == current) << "Expected fifo order ("
                              << j << " != " << current << ")";
        current++;
      }
    UASSERT(current == end) << "all should be present ["
           << start << ", " << end << ")";
  }

  complete("...complete");

  // Keep the same constants so we have exactly as many items removed
  // as were put in (don't make dequeuers dequeue empty)
  static const int kNumDequeuers = kNumEnqueuers;
  static const int kItemsPerDequeuer = kItemsPerEnqueuer;
  cout << "\tTesting concurrent dequeues and validity, "
       << kNumDequeuers << " dequeuers x " << kItemsPerDequeuer << " items"
       << endl;
  start("concurrent pops");

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
            UASSERT(val >= 0 && enqueuer < kNumEnqueuers) << "Value " << val
                   << " could not have been enqueued (" << kNumEnqueuers
                   << " enqueuers)";
            UASSERT(latest[enqueuer] < val) << "Expected fifo order, saw "
                   << latest[enqueuer] << " before " << val;
            latest[enqueuer] = val;
          }
        }));

  for (auto& fut : dfuts)
    fut.get();

  complete("...complete");

  // Dequeued all, should be mpty
  start("Testing empty");

  UASSERT(testq.empty());
  complete("...complete");

  static const int kRandomizedNumEnqueuers = 3;
  static const int kRandomizedNumDequeuers = 4;
  static const int kInterval = 10;
  static const int kDesiredRandomizedNum = 1000;
  static const int kRandomizedNum =
    (kDesiredRandomizedNum / kRandomizedNumEnqueuers) * kRandomizedNumEnqueuers;
  cout << "\tEnqueue/Dequeue testing (pause after enqueuing " << kInterval
       << " to encourage dequeuers),\n"
       << "\t\tenqueue: " << kRandomizedNumEnqueuers << "\n"
       << "\t\tdequeue: " << kRandomizedNumDequeuers << " (test empty edge case)\n"
       << "\t\tNumber of items: " << kRandomizedNum << endl;
  start("Enqueue/dequeue");

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
            UASSERT(val >= 0 && enqueuer < kRandomizedNumEnqueuers)
                << "Value " << val
                << " could not have been enqueued (" << kRandomizedNumEnqueuers
                << " enqueuers)";
            UASSERT(latest[enqueuer] < val) << "Expected fifo order, saw "
                   << latest[enqueuer] << " before " << val;
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

  complete("...complete");

  start("Test empty");
  UASSERT(testq.empty());
  complete("...complete");

  start("");
  complete("...........Success!");
}

// returns 8 by default.
int nthreads() {
  int viastd = thread::hardware_concurrency();
  if (!viastd) viastd = 8;
  return viastd;
}


template<template<typename> class T>
void bench_mpmc(int nitmes, int nenq, int ndeq);

template<template<typename> class T>
void bench_queue() {
  // Basically, repeat the multithreaded test but without
  // correctness.

  static const int kNumEnqueuers = nthreads();
  static const int kItemsPerEnqueuer = 100000;
  cout << "\tEnqueues (" << kNumEnqueuers << "x"
       << kItemsPerEnqueuer << "): ";
  cout.flush();

  atomic<int> cdl(kNumEnqueuers + 1); // todo replace with a countdown latch
  T<int> testq;
  UASSERT(testq.empty());
  vector<future<void> > futs;

  for (int i = 0; i < kNumEnqueuers; ++i)
    futs.push_back(async(launch::async, [&](int idx) {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));
          int start, end;
          tie(start, end) = interval(idx, kItemsPerEnqueuer);
          for (int i = start; i < end; ++i)
            testq.enqueue(i);
        }, i));

  TIME_BLOCK(chrono::milliseconds, "") {
    --cdl;
    for (auto& fut : futs)
      fut.get();
  }

  static const int kNumDequeuers = kNumEnqueuers;
  static const int kItemsPerDequeuer = kItemsPerEnqueuer;
  cout << "\tDequeues (" << kNumEnqueuers << "x"
       << kItemsPerEnqueuer << "): ";
  cout.flush();

  vector<future<void> > dfuts;
  cdl.store(kNumDequeuers + 1);
  for (int i = 0; i < kNumDequeuers; ++i)
    dfuts.push_back(async(launch::async, [&]() {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          for (int j = 0; j < kItemsPerDequeuer; ++j)
            testq.dequeue();
        }));

  TIME_BLOCK(chrono::milliseconds, "") {
    cdl--;
    for (auto& fut : dfuts)
      fut.get();
  }

  UASSERT(testq.empty());

  static const int kItems = 1000000;

  static const int kEqualWeightEnqueuers = nthreads() / 2;
  static const int kEqualWeightDequeuers = kEqualWeightEnqueuers;

  cout << "\tFair mpmc (~" << kItems << " items, "
       << kEqualWeightEnqueuers << " enq, "
       << kEqualWeightDequeuers << " deq): ";
  cout.flush();

  bench_mpmc<T>(kItems, kEqualWeightEnqueuers, kEqualWeightDequeuers);

  static const int kEWDequeuers =
      (nthreads() / 3 == 0) ? 1 : nthreads() / 3;
  static const int kEWEnqueuers =
      nthreads() - kEWDequeuers;

  cout << "\tEnqueue-weighted mpmc (~" << kItems << " items, "
       << kEWEnqueuers << " enq, " << kEWDequeuers << " deq): ";
  cout.flush();

  bench_mpmc<T>(kItems, kEWEnqueuers, kEWDequeuers);

  static const int kDWEnqueuers =
      (nthreads() / 3 == 0) ? 1 : nthreads() / 3;
  static const int kDWDequeuers =
      nthreads() - kDWEnqueuers;

  cout << "\tDequeue-weighted mpmc (~" << kItems << " items, "
       << kDWEnqueuers << " enq, " << kDWDequeuers << " deq): ";
  cout.flush();

  bench_mpmc<T>(kItems, kDWEnqueuers, kDWDequeuers);
}

template<template<typename> class T>
void bench_mpmc(int nitems, int nenq, int ndeq) {
  T<int> testq;

  atomic<int> cdl;
  cdl.store(nenq + ndeq + 1);

  atomic<int> unfinished_enqueuers;
  unfinished_enqueuers.store(nenq, std::memory_order_relaxed);

  vector<future<void> > efuts;
  const int kPerEnqueuer = nitems / nenq;
  for (int i = 0; i < nenq; ++i)
    efuts.push_back(async(launch::async, [&](int idx) {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          int start, end;
          tie(start, end) = interval(idx, kPerEnqueuer);
          for (int j = start; j < end; ++j)
            testq.enqueue(j);
          int last =
              unfinished_enqueuers.fetch_sub(1, std::memory_order_relaxed);

          if (last == 1) {
            // Extra items needed so dequeuers don't block on dequeue()
            // after last enqueue() (may divide unevenly)
            for (int j = 0; j < ndeq; ++j)
              testq.enqueue(j);
          }
        }, i));

  vector<future<void> > dfuts;
  for (int i = 0; i < ndeq; ++i)
    dfuts.push_back(async(launch::async, [&]() {
          --cdl;
          while (cdl.load())
            this_thread::sleep_for(chrono::milliseconds(5));

          while (unfinished_enqueuers.load(std::memory_order_relaxed))
            testq.dequeue();
        }));


  TIME_BLOCK(chrono::milliseconds, "") {
    --cdl;
    for (auto& fut : efuts)
      fut.get();
    for (auto& fut : dfuts)
      fut.get();
  }
}
