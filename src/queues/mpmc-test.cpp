/*
 * Vladimir Feinberg
 * mpmc-test.cpp
 * 2014-08-15
 *
 * Defines tests for mpmc synchronoized queues.
 */

#define QUEUES_SHARED_QUEUE_H_PRINT_DEBUG_STRING

#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "utilities/test_util.h"
#include "queues/queue.h"
#include "queues/shared_queue.h"

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

template<template<typename> class T>
void unit_test();

void test_main() {
  cout << "Queue Test..." << endl;
  // performance comparison here TODO
  cout << "\nLock Free Queue" << endl;
  unit_test<lfqueue>();
  // cout << "\nAnother Queue" << endl; TODO
  test_multithreaded();
}

void start(const string& s) {
  cout << '\t' << left << setw(25);
  cout << s;
  cout.flush();
}

void complete(const string& s = "...complete") {
  cout << right << s << endl;
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
  return vecstream.str() == xstream.str();
}

template<template<typename> class T>
void unit_test() {
  T<int> t;
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  ASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
  ASSERT(!t.empty());
  complete();
  start("Remove 0..9");
  for(int i = 0; i < 10; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO");
  ASSERT(same(t, {}));
  ASSERT(t.empty());
  complete();
  start("Insert 0..9");
  for(int i = 0; i < 10; ++i)
    t.enqueue(i);
  ASSERT(same(t, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
  ASSERT(!t.empty());
  complete();
  start("Remove 0..4");
  for(int i = 0; i < 5; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO (partial remove)");
  ASSERT(same(t, {9, 8, 7, 6, 5}));
  ASSERT(!t.empty());
  complete();
  start("Insert 10..14");
  for(int i = 10; i < 15; ++i)
    t.enqueue(i);
  ASSERT(same(t, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}));
  complete();
  start("Remove 5..14");
  for(int i = 5; i < 15; ++i)
    ASSERT(t.dequeue() == i, "queue not FIFO (remove after being empty)");
  ASSERT(same(t, {}));
  ASSERT(t.empty());
  complete();
  start("");
  complete("...........Success!");
}

// TODO tempalte template
void test_multithreaded() {
  lfqueue<int> x;
  vector<int> v;
}
