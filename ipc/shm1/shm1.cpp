// shm1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <ct_type.h>

static void _test1();

int main()
{
    std::wcout << L"Hello World!\n";

    _test1();

    std::wstring s_end;
    std::wcin >> s_end;
    return 0;
}

void _test1()
{
    std::wcout << L"[EN] " << __WFUNCTION__ << std::endl;

    do {


    } while (false);

    std::wcout << L"[EX] " << __WFUNCTION__ << std::endl;
}
