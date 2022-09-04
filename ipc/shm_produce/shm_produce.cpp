#include <iostream>
#include <time.h>
#include <string>

#include "shm_deque.h"

int main(int argc, char** argv)
{
	shm_deque<int> q(L"in_traffic", 65536,10);
	std::string inp;
	long max_msg = 1L << 20;
	long counter = 0;
	int tr = 0;
	do
	{
		int wait_milisec = rand() % 2;
		boost::this_thread::sleep(boost::posix_time::milliseconds(wait_milisec));

		++tr;
		if (tr < 0)
			tr = 0;
		//
		q.push_back(tr,true);
		std::cout << counter++ << ":" << tr << std::endl;
	} while (inp != "exit");
	return 0;
}