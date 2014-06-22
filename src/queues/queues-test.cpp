/*
 * Vladimir Feinberg
 * test.h
 * 2014-06-22
 *
 * Defines tests for queues
 */

//#include "queues/lock_free_q.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <cassert>

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

template<template<typename> class T>
void unit_test(void);

int main()
{
        cout << "Queue Test..." << endl;
        // performance comparison here TODO
        cout << "\nLock Free Queue" << endl;
        //        unit_test<lfqueue>();
        cout << "\nAnother Queue" << endl;
}

void start(std::string s)
{
        cout << '\t' << left << setw(25);
        cout << s;
        cout.flush();
}

void complete(std::string s = "...complete")
{
        cout << right << s << endl;
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
        start("Attempt empty dequeue");
        assert(!t.dequeue([](int x) {assert(0);}));
        complete();
        start("Insert 0..9");
        for(int i = 0; i < 10; ++i)
                t.enqueue(i);
        complete();
        start("Remove 0..4");
        for(int i = 0; i < 5; ++i)
                assert(t.dequeue([i](int x) {assert(i == x);}));
        complete();
        start("Insert 10..14");
        for(int i = 10; i < 15; ++i)
                t.enqueue(i);
        complete();
        start("Remove 5..14");
        for(int i = 5; i < 15; ++i)
                assert(t.dequeue([i](int x) {assert(i == x);}));
        complete();
        start("Attempt empty dequeue");
        assert(!t.dequeue([](int x) {assert(0);}));
        complete();
        start("");
        complete("...........Success!");

}
