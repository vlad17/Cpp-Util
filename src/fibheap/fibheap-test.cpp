/*
 * Vladimir Feinberg
 * fibheap-test.cpp
 * 2014-09-08
 *
 * Defines tests for fibheap
 */

#include "fibheap/fibheap.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <random>

using namespace std;

const int SEED = 0;
minstd_rand0 gen(SEED);

int main() {
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
  cout << "Stress test for fibheap..." << endl;
#ifdef NDEBUG
  int constexpr STRESS_REPS = 1e7;
#else
  int constexpr STRESS_REPS = 1e3;
#endif
  int constexpr MAX_KEYS = STRESS_REPS/50;
  moved.clear();
  // will not have all keys, just some for tests
  // TODO add decrease-key randomization
  array<fibheap<int>::key_type, MAX_KEYS> keyarr = {nullptr};
  for(int i = 0; i < STRESS_REPS; ++i)
    if (gen()&1 || moved.empty()) {
        int rand = static_cast<int>(gen()) % (MAX_KEYS * 2);
        if (0 <= rand && rand < MAX_KEYS) {
          if (!keyarr[rand])
            keyarr[rand] = moved.push(rand);
        } else
          moved.push(rand);
    } else {
      auto val = moved.top();
      // Check correctness on about 25% of the runs
      if (0 <= val && val < MAX_KEYS) {
        fibheap<int>::key_type k = keyarr[val];
        assert(k != fibheap<int>::key_type());
        assert(k == moved.pop());
        keyarr[val] = nullptr;
      }
    }
  while (!moved.empty() && moved.top() < 0)
    moved.pop();
  for (int i = 0; i < MAX_KEYS; ++i)
    if (keyarr[i] != nullptr) {
      assert(moved.top() == i);
      fibheap<int>::key_type k = keyarr[moved.top()];
      assert(k != fibheap<int>::key_type() && k == moved.pop());
    }
  while(!moved.empty())
    moved.pop();
  cout << "...completed" << endl;
  return 0;
}
