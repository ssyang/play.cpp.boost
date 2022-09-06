#include <iostream>
#include <time.h>
#include <string>

#include <ns_boost/ct_shm_req_deque_rsp_map.h>
#include "test_item.h"

int main(int argc, char** argv)
{
	typedef	_ns_tools::ct_shm_req_deque_rsp_map<struct test_item, struct test_item>	_type_shm;

	_type_shm q(L"in_traffic", 65536,10);
	std::string inp;
	long max_msg = 1L << 20;
	long counter = 0;
	int tr = 0;
	unsigned long n_pid = _getpid();
	struct test_item item { 0, 0 };
	long n_key(0);

	do {
		std::cout << " : wait request from client." << std::endl;
		q.owner_wait_during_empty_deque();

		bool b_result(false);
		std::tie(b_result, item, n_key) = q.owner_front_deque(true);
		if (b_result) {
			std::cout << "[ Clinet - " << item.n_pid << "]" << counter++ << ": data - " << item.n_data << std::endl;

			if (!q.owner_set_response(n_key, _type_shm::st_complete_success, item)) {
				std::cout << "[" << "-_-" << "]" << counter++ << ": fail set_response()." << std::endl;
			}
			else {
				std::cout << "[ RSP - " << item.n_pid << "] KEY - " << n_key << ": data - " << item.n_data << std::endl;
			}

		}
		else {
			std::cout << "[" << "-_-" << "]" << counter++ << ": fail front()." << std::endl;
		}
	} while (counter<100000);
	return 0;

}