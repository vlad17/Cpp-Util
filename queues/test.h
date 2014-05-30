/*
 * Vladimir Feinberg
 * test.h
 * 2014-05-29
 *
 * Defines test() for queues
 */

#include "lock_free_q.h"

#include <iostream>
#include <random>

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

template<template<typename> class T>
void unit_test()
{
	T<int> t;
	cout << "hi" << endl;
}

