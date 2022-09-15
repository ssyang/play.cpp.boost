#pragma once

#include <string>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <atltrace.h>

class shm_lock_guard_fl
{
public:
	shm_lock_guard_fl(boost::interprocess::file_lock& fl) : m_fl(fl)
	{
		m_fl.lock();
	}

	~shm_lock_guard_fl()
	{
		m_fl.unlock();
	}

private:
	boost::interprocess::file_lock& m_fl;

private://don't call these methods
	shm_lock_guard_fl();
	shm_lock_guard_fl(const shm_lock_guard_fl&);
	shm_lock_guard_fl& operator=(const shm_lock_guard_fl&);
};