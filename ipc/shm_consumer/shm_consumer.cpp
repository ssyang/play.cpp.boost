#include <iostream>
#include <time.h>
#include <string>
#include <utility>

#include "shm_deque.h"


int main(int argc, char** argv)
{
	shm_deque<int> q(L"in_traffic");
	long counter = 0;
	do
	{
		q.wait_during_empty();

		int wait_milisec = rand() % 500;
		boost::this_thread::sleep(boost::posix_time::milliseconds(wait_milisec));

		bool b_result(false);
		int n_value(0);
		std::tie(b_result, n_value) = q.front(true);
		if (b_result) {
			std::cout << counter++ << ":" << n_value << std::endl;
		}
		else {
			std::cout << counter++ << ": fail front()." << std::endl;
		}

	} while (1);
	return 0;
}