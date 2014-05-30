/*
 * Vladimir Feinberg
 * test.h
 * 2014-05-29
 *
 * Defines test() for queues
 */

#include "lock_free_q.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <cassert>

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

template<template<typename> class T>
void unit_test(void);

void test()
{
	cout << "Queue Test..." << endl;
	// performance comparison here TODO
	cout << "\nLock Free Queue" << endl;
	unit_test<lfqueue>();
}

void start(std::string s)
{
	cout << '\t' << left << setw(20);
	cout << s;
	cout.flush();
}

void complete()
{
	cout << right << "...Completed!\n";
}

template<template<typename> class T>
void unit_test()
{
	T<int> t;
	start("Insert 0..9");
	for(int i = 0; i < 10; ++i)
		t.enqueue(i);
	complete();
	start("Remove 0..9");
	for(int i = 0; i < 10; ++i)
		assert(t.dequeue([i](int x) {assert(i == x);}));
	complete();
}

