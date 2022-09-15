#include <iostream>
#include <time.h>
#include <string>
#include <utility>
#include <ct_string.h>

#include <ns_boost/ct_shm_req_deque_rsp_map.h>
#include "test_item.h"

#include <windows.h>
#include <atltrace.h>

typedef	_ns_tools::ct_shm_req_deque_rsp_map<struct test_item, struct test_item>	_type_shm;
typedef	std::shared_ptr<_type_shm> _type_ptr_shm;
typedef	_ns_tools::ct_shm_req_deque_rsp_map<struct test_item, struct test_item>::type_ptr_ncv	_type_ptr_cv;

//static _type_shm q(L"in_traffic", 10);
static _type_ptr_shm ptr_q;

BOOL CtrlHandler1(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_BREAK_EVENT:
	default:
		//q.call_deleter();
		ptr_q.reset();
		std::cout << "terminated" << std::endl;
		ATLTRACE("terminated. by handler_1\n");
	}
	return FALSE;//call next handler
}

int main(int argc, char** argv)
{
	BOOL fSuccess = SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler1, TRUE);
	
	long counter = 0;
	unsigned long n_pid = _getpid();
	struct test_item item { 0, 0 };
	std::srand(time(NULL));
	int wait_milisec = std::rand() % 100;//range 0 to 99

	std::string s_info;

	ptr_q = _type_ptr_shm(new _type_shm(L"in_traffic", 10));

	do
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(wait_milisec));

		item.n_pid = n_pid;
		++item.n_data;
		if (item.n_data < 0)
			item.n_data = 0;
		//
		std::pair<bool, long> result = ptr_q->client_push_back_deque(item, true);

		_ns_tools::ct_string::format(s_info,"[%d][PID:%u]-[in:%d].\n", counter++,n_pid, item.n_data);
		std::cout << s_info;
		if (result.first) {
			std::cout << "waiting response.\n";
			if (ptr_q->client_wait_until_set_result(result.second)) {

				std::cout << "find result.\n";
				std::tuple<bool, _type_shm::type_response_status, _type_ptr_cv > result_process = ptr_q->client_find_map(result.second, item);
				if (std::get<0>(result_process)) {
					std::cout << "[ SVR - " << item.n_pid << "]" << item.n_data << std::endl;
					_ns_tools::ct_string::format(s_info, "[RSP][OK]-[PID:%u]-[out:%d].\n", item.n_pid, item.n_data);
				}
				else {
					_ns_tools::ct_string::format(s_info, "[RSP][ER]-find_map().\n");
				}
				std::cout << s_info;
				ptr_q->client_remove_result(result.second);
			}
			else {
				std::cout << "[ER ]waiting response.\n";
			}
		}
		else {
			_ns_tools::ct_string::format(s_info, "[RSP][ER]-client_push_back_deque().\n");
			std::cout << s_info;
		}

	} while (item.n_data<100);
	return 0;
}