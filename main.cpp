#include <iostream>

#include "fibheap.h"

#include <vector>

using namespace std;

int main()
{
	fibheap_test();
	return 0;
}

void fibheap_test()
{
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
}
