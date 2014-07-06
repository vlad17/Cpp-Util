/*
 * Vladimir Feinberg
 * ptw-test.cpp
 * 2014-06-22
 *
 * Defines tests for pthreads wrappers
 */

#include "pthread_wrappers/rwlock.hpp"

#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

void rwtest();

int main()
{
  cout << "locks test...." << endl;
  rwtest();
  return 0;
}

void rwtest()
{
  cout << "\nrw lock test" << endl;
  const int SLEEP = 30;
  const int NUM = 3;
  int shared = 0;
  locks::rw rwlk;

  auto readerf = [&rwlk, &shared, SLEEP, NUM]()
    {
      this_thread::sleep_for(chrono::milliseconds(SLEEP));
      stringstream ss;
      ss << "\tThread " << this_thread::get_id() << " read: ";
      for(int i = 0; i < NUM; ++i)
        {
          rwlk.lock_shared();
          int reading = shared;
          string s = ss.str() + to_string(reading) + '\n';
          cout << s;
          rwlk.unlock_shared();
          this_thread::sleep_for(chrono::milliseconds(SLEEP));
        }
    };
  auto writerf = [&rwlk, &shared, SLEEP](int x)
    {
      this_thread::sleep_for(chrono::milliseconds(SLEEP/2));
      stringstream ss;
      ss << "\tThread " << this_thread::get_id() << " wrote: ";
      rwlk.lock();
      int reading = shared;
      shared = x;
      int writing = shared;
      rwlk.unlock();
      string s = ss.str() + to_string(reading) + "->" + to_string(writing) + '\n';
      cout << s;
      this_thread::sleep_for(chrono::milliseconds(SLEEP));
    };

  vector<thread> threads;
  for(int i = 0; i < 3; ++i)
    threads.push_back(thread(readerf));
  // Test write
  for(int i = 0; i < NUM; ++i)
    writerf(i);

  for(auto& t : threads)
    t.join();
}
