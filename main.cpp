/*
 * Vladimir Feinberg
 * main.cpp
 * 2013-12-02
 *
 * Contains some tests for the data structures, prints to stdout.
 */

#include <iostream>

#include "fibheap.h"
#include "lfu_cache.h"
#include <random>
#include <array>

using namespace std;

int constexpr SEED = 0;

void cache_test(), fibheap_test();

int main()
{
	fibheap_test();
	return 0;
}

void cache_test()
{
	cout << "LFU test" << endl;
	cout << "\nheap_cache test" << endl;
	lfu::heap_cache<int, int> lhc;
	cout << "Adding (0,1), (0,3) - replace - , (1,2), (2,4)" << endl;
	lhc.insert(make_pair(0,1));
	lhc.insert(make_pair(1,2));
	lhc.insert(make_pair(0,3));
	lhc.insert(make_pair(2,4));
	cout << lhc << endl;
	cout << "Empty: " << lhc.empty() << endl;
	cout << "Looking up key 3 (should be null): ptr=";
	cout << lhc.lookup(3) << endl;
	cout << "Looking up key 0: ";
	cout << *lhc.lookup(0) << endl;
	cout << "Looking up key 1 twice: ";
	cout << *lhc.lookup(1) << ", ";
	cout << *lhc.lookup(1) << endl;
	cout << lhc << endl;
	cout << "Set to max size 1" << endl;
	lhc.set_max_size(1);
	cout << lhc << endl;
	cout << "Clear:" << endl;
	lhc.clear();
	cout << lhc << endl;
	cout << "Empty: " << lhc.empty() << endl;
	cout << "set max. size 128, fill" << endl;
	lhc.set_max_size(128);
	for(int i = 0; i < 128; ++i)
		lhc.insert(make_pair(i,i));
	cout << lhc << endl;
	cout << "Lookup 500 times, randomly." << endl;
	minstd_rand0 gen(SEED);
	for(int i = 0; i < 500; ++i)
		lhc.lookup(gen()&127);
	cout << lhc << endl;
	cout << "Try to insert new item (128,128)" << endl;
	lhc.insert(make_pair(128,128));
	cout << lhc << endl;
	cout << "Generate smaller cache" << endl;
	lfu::heap_cache<int,int> lhc2;
	cout << lhc2 << endl;
	cout << "Add (i,i) up to 10, lookup randomly." << endl;
	for(int i = 0; i < 10; ++i)
		lhc2.insert(make_pair(i,i));
	for(int i = 0; i < 20; ++i)
		lhc2.lookup(gen()%10);
	cout << lhc2 << endl;
	cout << "copy this new cache to old one. Old one:" << endl;
	lhc = lhc2;
	cout << lhc << endl;
	cout << "second cache that was copied from after copy:" << endl;
	cout << lhc2 << endl;
	cout << "move newly copied old one to previous new one. After move (to assigned):" << endl;
	lhc2 = move(lhc);
	cout << lhc2 << endl;
	cout << "after move (from assignment value) - size: " << lhc.size() << endl;
	lhc2.clear();
#ifdef NDEBUG
	cout << "stress tests for heap cache...." << endl;
	int constexpr STRESS_REPS = 1e7;
	int constexpr MAX_KEYSIZE = STRESS_REPS/100;
	for(int i = 0; i < STRESS_REPS; ++i)
	{
		int test = gen()%MAX_KEYSIZE;
		auto ptrtest = lhc2.lookup(test);
		if(ptrtest != nullptr && *ptrtest != test)
				cout << "\tError: value " << test << " not matched with key" << endl;
		lhc2.insert(make_pair(test,test));
	}
	cout << "...Completed" << endl;
#endif /* NDEBUG */
}

void fibheap_test()
{
	cout << "Fibheap test" << endl;
	fibheap<int> f;
	cout << "Add 0..9 in ascending order" << endl;
	for(int i = 0; i < 10; ++i)
		f.push(i);
	cout << f << endl;
	cout << "Delete min once" << endl;
	f.pop();
	cout << f << endl;
	cout << "Extract min until empty" << endl;
	while(!f.empty()) f.pop();
	cout << f << endl;
	cout << "Add 9..0 in descending order" << endl;
	for(int i = 9; i >=0; --i)
		f.push(i);
	cout << f << endl;
	cout << "Delete min once" << endl;
	f.pop();
	cout << f << endl;
	cout << "Extract min until empty" << endl;
	while(!f.empty()) f.pop();
	cout << f << endl;
	cout << "Clear empty" << endl;
	f.clear();
	cout << f << endl;
	cout << "Clear when filled (adds 10 items first)" << endl;
	for(int i = 0; i < 10; ++i) f.push(i);
	f.clear();
	cout << f << endl;
	cout << "Fill with 0,0,0" << endl;
	f.push(0);
	f.push(0);
	f.push(0);
	cout << f << endl;
	cout << "Extract min" << endl;
	f.pop();
	cout << f << endl;
	cout << "Fill with overlap -1,1 x2" << endl;
	for(int j = 0; j < 2; ++j)
	{
			f.push(-1);
			f.push(1);
	}
	cout << f << endl;
	cout << "Extract min" << endl;
	f.pop();
	cout << f << endl;
	cout << "Extract min" << endl;
	f.pop();
	cout << f << endl;
	cout << "Extract min" << endl;
	f.pop();
	cout << f << endl;
	cout << "Clear when full" << endl;
	f.clear();
	cout << f << endl;
	cout << "Add 1..3";
	auto one = f.push(1);
	f.push(2);
	auto three = f.push(3);
	cout << f << endl;
	cout << "Decrease-key 1 to 0" << endl;
	f.decrease_key(one, 0);
	cout << f << endl;
	cout << "Add 4 x4, then extract min" << endl;
	for(int i = 0; i < 3; ++i) f.push(4);
	auto four = f.push(4);
	f.pop();
	cout << f << endl;
	cout << "Decrease-key 3 to 0" << endl;
	f.decrease_key(three, 0);
	cout << f << endl;
	cout << "Decrease-key 4 to 3" << endl;
	f.decrease_key(four, 3);
	cout << f << endl;
	cout << "Extract min" << endl;
	f.pop();
	cout << f << endl;
	cout << "Copy construct" << endl;
	fibheap<int> fcpy(f);
	cout << fcpy << endl;
	cout << "heap value after copy:" << endl;
	cout << f << endl;
	cout << "Move construct" << endl;
	fibheap<int> moved(std::move(f));
	cout << moved << endl;
	cout << "heap value after move:" << endl;
	cout << f << endl;
#ifdef NDEBUG
	cout << "Stress test for fibheap..." << endl;
	int constexpr STRESS_REPS = 1e7;
	int constexpr MAX_KEYS = STRESS_REPS/100;
	// will not have all keys, just some for tests
	array<fibheap<int>::key_type, MAX_KEYS> keyarr;
	minstd_rand0 gen(0);
	for(int i = 0; i < STRESS_REPS; ++i)
		if(gen()&1)
			if(i < MAX_KEYS)
				keyarr[i] = moved.push(i);
			else
			{
				int rand = static_cast<int>(gen());
				rand = rand >= 0 && rand < MAX_KEYS? rand+MAX_KEYS : rand;
				moved.push(rand);
			}
		else
		{
			if(moved.top() < MAX_KEYS)
			{
				fibheap<int>::key_type k = keyarr[moved.top()];
				if(k != moved.pop());
					cout << "\tError: popped key does not match saved key" << endl;
			}
			moved.pop();
		}
	for(auto i : keyarr)
		if(i != nullptr) moved.decrease_key(i, 0);
	while(!moved.empty())
		moved.pop();
	cout << "...completed";
#endif /* NDEBUG */
}
