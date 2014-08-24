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

template<template<typename> class T>
void test_multithreaded();

void test_main() {
  cout << "Queue Test..." << endl;
  // performance comparison here TODO
  cout << "\nShared Queue" << endl;
  cout << "\tUnit testing:" << endl;
  unit_test<shared_queue>();
  cout << "\n\tMultithreaded test:" << endl;
  test_multithreaded<shared_queue>();
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
  start("");
  complete("...........Success!");
}

template<template<typename> class T>
void test_multithreaded() {
  // TODO
  cout << "Test not yet made.\n";
}