#pragma once

#include <string>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include <atltrace.h>


class shm_cv_am
{
public:
	~shm_cv_am()
	{
		if (m_b_ini && m_b_owner)
			*m_pn_notify = 0;
	}

	shm_cv_am(boost::interprocess::create_only_t a, boost::interprocess::managed_shared_memory *p_mshm, const char* name, const boost::interprocess::permissions& perm = boost::interprocess::permissions())
	{
		_ini();

		if (name)
			m_s_name = name;
		//
		m_p_mshm = p_mshm;
		//
		m_b_ini = _create();
	}
	shm_cv_am(boost::interprocess::open_or_create_t a, boost::interprocess::managed_shared_memory* p_mshm, const char* name, const boost::interprocess::permissions& perm = boost::interprocess::permissions())
	{
		_ini();

		if (name)
			m_s_name = name;
		//
		m_p_mshm = p_mshm;

		if (_open()) {
			m_b_ini = true;
		}
		else {
			m_b_ini = _create();
		}

	}
	shm_cv_am(boost::interprocess::open_only_t a, boost::interprocess::managed_shared_memory* p_mshm, const char* name)
	{
		_ini();

		if (name)
			m_s_name = name;
		//
		m_p_mshm = p_mshm;
		//
		m_b_ini = _open();
	}

	//!If there is a thread waiting on *this, change that
	//!thread's state to ready. Otherwise there is no effect.*/
	void notify_one(bool fixed_status = false)
	{
		do {
			if (!m_b_ini) {
				continue;
			}

			{
				boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*m_p_mutex);

				if (fixed_status)
					*m_pn_notify = SIZE_MAX;
				else {
					if (*m_pn_notify == SIZE_MAX - 1)
						continue;//error over notify
					//
					++(*m_pn_notify);
				}
			}
			//
			m_p_cv->notify_one();
		} while (false);
	}

	//!Change the state of all threads waiting on *this to ready.
	//!If there are no waiting threads, notify_all() has no effect.
	void notify_all()
	{
		m_p_cv->notify_all();
	}

	// this function occur spurious wakeup =====================
	//!Releases the lock on the interprocess_mutex object associated with lock, blocks
	//!the current thread of execution until readied by a call to
	//!this->notify_one() or this->notify_all(), and then reacquires the lock.
	template <typename L>
	void wait(L& lock)
	{
		do {
			if (!m_b_ini)
				continue;

			while (true) {
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> _lock(*m_p_mutex);
					if (*m_pn_notify > 0) {
						if (*m_pn_notify != SIZE_MAX) {
							--(*m_pn_notify);
						}
						break;//exit while
					}
				}

				m_p_cv->wait<L>(lock);
			}//end while

		} while (false);
	}

	//!Same as `timed_wait`, but this function is modeled after the
	//!standard library interface and uses relative timeouts.
	template <typename L, class Duration>
	boost::interprocess::cv_status wait_for(L& lock, const Duration& dur)
	{
		boost::interprocess::cv_status cv_st(::interprocess::cv_status::no_timeout);

		do {
			if (!m_b_ini)
				continue;

			while (true) {
				{
					boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> _lock(*m_p_mutex);
					if (*m_pn_notify > 0) {
						if (*m_pn_notify != SIZE_MAX) {
							--(*m_pn_notify);
						}
						return std::cv_status::no_timeout;//exit while
					}
				}

				if (m_p_cv->wait_for<L, Duration>(lock, dur) == std::cv_status::timeout)
					return std::cv_status::timeout;
			}//end while

		} while (false);
		return cv_st;
	}


	bool is_ini() const
	{
		return m_b_ini;
	}
		
	bool is_always_notified()
	{
		bool b_result(false);
		do {
			if (!m_b_ini)
				continue;
			boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*m_p_mutex);
			if (*m_pn_notify != SIZE_MAX)
				continue;
			b_result = true;
		} while (false);
		return b_result;
	}

protected:
	bool _create()
	{
		bool b_result(false);

		m_b_owner = false;

		do {
			if (!m_p_mshm)
				continue;
			//
			try {
				m_p_mutex = m_p_mshm->construct<boost::interprocess::interprocess_mutex>(_get_name_of_mutex().c_str())();
				if (!m_p_mutex) {
					continue;
				}

				m_pn_notify = m_p_mshm->construct<size_t>(
					_get_name_of_n_notify().c_str()
					)(0);
				if (!m_pn_notify) {
					continue;
				}

				m_p_cv = m_p_mshm->construct<boost::interprocess::interprocess_condition>(_get_name_of_cv().c_str())();
				if (!m_p_cv) {
					continue;
				}

				//
				b_result = true;
				m_b_owner = true;
			}
			catch (boost::interprocess::interprocess_exception& ex) {
				b_result = false;
				std::string _s_name = "none";
				if (m_p_mutex) {
					_s_name = _get_name_of_mutex();
				}
				else if (m_pn_notify) {
					_s_name = _get_name_of_n_notify();
				}
				else if (m_p_cv) {
					_s_name = _get_name_of_cv();
				}

				ATLTRACE(" : %s : E : %s : %s.\n", __FUNCTION__, ex.what(), _s_name.c_str());
				continue;
			}
		} while (false);
		return b_result;
	}
	bool _open()
	{
		bool b_result(false);

		m_b_owner = false;

		do {
			if (!m_p_mshm)
				continue;
			//
			try {
				m_p_mutex = m_p_mshm->find<boost::interprocess::interprocess_mutex>(_get_name_of_mutex().c_str()).first;
				if (!m_p_mutex) {
					continue;
				}

				m_pn_notify = m_p_mshm->find<size_t>(
					_get_name_of_n_notify().c_str()
					).first;
				if (!m_pn_notify) {
					continue;
				}

				m_p_cv = m_p_mshm->find<boost::interprocess::interprocess_condition>(_get_name_of_cv().c_str()).first;
				if (!m_p_cv) {
					continue;
				}

				b_result = true;
			}
			catch (boost::interprocess::interprocess_exception& ex){
				b_result = false;
				std::string _s_name = "none";
				if (m_p_mutex) {
					_s_name = _get_name_of_mutex();
				}
				else if (m_pn_notify) {
					_s_name = _get_name_of_n_notify();
				}
				else if (m_p_cv) {
					_s_name = _get_name_of_cv();
				}

				ATLTRACE(" : %s : E : %s : %s.\n", __FUNCTION__, ex.what(), _s_name.c_str());
				continue;
			}
			continue;
		} while (false);
		return b_result;
	}

	std::string _get_name_of_n_notify()
	{
		std::string s_name;
		s_name = m_s_name + "_n_notif___";
		return s_name;
	}
	std::string _get_name_of_mutex()
	{
		std::string s_name;
		s_name = m_s_name + "_mutex___";
		return s_name;
	}
	std::string _get_name_of_cv()
	{
		std::string s_name;
		s_name = m_s_name + "_cv___";
		return s_name;
	}

	void _ini()
	{
		m_b_ini = false;
		m_b_owner = false;
		m_s_name.clear();

		m_p_mshm = nullptr;
		m_pn_notify = nullptr;
		m_p_mutex = nullptr;
		m_p_cv = nullptr;
	}
protected:
	//local member
	bool m_b_ini;
	bool m_b_owner;
	std::string m_s_name;
	boost::interprocess::managed_shared_memory *m_p_mshm;

	//shared mem member
	size_t *m_pn_notify;
	boost::interprocess::interprocess_mutex *m_p_mutex;
	boost::interprocess::interprocess_condition *m_p_cv;


private://don't call these method
	shm_cv_am();
	shm_cv_am(const shm_cv_am&);
	shm_cv_am& operator=(const shm_cv_am&);
};