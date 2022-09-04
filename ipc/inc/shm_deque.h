#pragma once

#include <memory>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <ct_string.h>

#include <atltrace.h>

template <typename QITEM>
class shm_deque
{
public:
	typedef QITEM type_data;

private:
	
	typedef boost::interprocess::allocator<shm_deque::type_data, boost::interprocess::managed_shared_memory::segment_manager> _type_shm_allocator;
	typedef boost::interprocess::deque<type_data, shm_deque::_type_shm_allocator> _type_shm_deque;

public:
	virtual ~shm_deque()
	{
		do {
			if (!m_b_ini)
				continue;
			if (!m_b_owner) {
				continue;
			}

			boost::interprocess::shared_memory_object::remove(m_s_shared_mem_name.c_str());

		} while (false);
		
	}

	shm_deque()
	{
		_ini_common();
	}

	shm_deque(const std::wstring& ws_shared_mem_name, size_t n_shared_mem, size_t n_deque_max_size)
	{
		std::string s_shared_mem_name(_ns_tools::ct_string::get_mcsc_from_unicode(ws_shared_mem_name));
		m_b_ini = _initialize(true, s_shared_mem_name, n_shared_mem, n_deque_max_size);
	}
	shm_deque(const std::wstring& ws_shared_mem_name)
	{
		std::string s_shared_mem_name(_ns_tools::ct_string::get_mcsc_from_unicode(ws_shared_mem_name));
		m_b_ini = _initialize(false, s_shared_mem_name,0,0);
	}


	bool is_ini() const
	{
		return m_b_ini;
	}

	bool push_back(const shm_deque::type_data& data,bool b_if_full_then_wait_until_poped = true)
	{
		bool b_result(false);

		do {
			if (!m_ptr_shm_mutex) {
				continue;
			}
			boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
			if (!m_p_shm_deque) {
				continue;
			}
			if (m_p_shm_deque->size() == m_n_deque_max_size) {
				if (!b_if_full_then_wait_until_poped)
					continue;
				if (m_ptr_shm_cv_full) {
					m_ptr_shm_cv_full->wait(lock);
				}
			}
			m_p_shm_deque->push_back(data);
			if (m_ptr_shm_cv_pushed)
				m_ptr_shm_cv_pushed->notify_one();
			//
			b_result = true;
		} while (false);
		return b_result;
	}

	bool empty() const
	{
		bool b_empty(true);
		do {
			if (m_ptr_shm_mutex) {
				boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
				if (!m_p_shm_deque)
					continue;
				b_empty = m_p_shm_deque->empty();
			}
		} while (false);
		return b_empty;
	}
	void wait_during_empty()
	{
		do {
			if (m_ptr_shm_mutex) {
				boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
				if (!m_p_shm_deque)
					continue;
				if (m_p_shm_deque->empty()) {
					if(m_ptr_shm_cv_pushed)
						m_ptr_shm_cv_pushed->wait(lock);
				}
			}
		} while (false);
	}

	size_t size() const
	{
		size_t n_size(0);
		do {
			if (m_ptr_shm_mutex) {
				boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
				if (!m_p_shm_deque)
					continue;
				n_size = m_p_shm_deque->size();
			}
		} while (false);
		return n_size;
	}

	void pop_front()
	{
		do {
			if (m_ptr_shm_mutex) {
				boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
				if (!m_p_shm_deque)
					continue;
				if (m_p_shm_deque->empty())
					continue;
				m_p_shm_deque->pop_front();
				if (m_ptr_shm_cv_full)
					m_ptr_shm_cv_full->notify_one();
			}
		} while (false);
	}

	std::pair<bool,shm_deque::type_data >front(bool b_remove = false)
	{
		std::pair<bool, shm_deque::type_data > result(false, shm_deque::type_data());

		do {
			if (m_ptr_shm_mutex) {
				boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_ptr_shm_mutex);
				if (!m_p_shm_deque)
					continue;

				result.first = true;
				result.second = m_p_shm_deque->front();
				if (b_remove) {
					m_p_shm_deque->pop_front();
					if (m_ptr_shm_cv_full)
						m_ptr_shm_cv_full->notify_one();
				}
			}
		} while (false);
		return result;
	}
	
private:
	void _ini_common()
	{
		m_b_ini = false;
		m_b_owner = true;
		m_s_shared_mem_name.clear();
		m_p_shm_deque = nullptr;
		m_n_deque_max_size = 0;
	}

	bool _initialize(
		bool b_owner,
		const std::string& s_shared_mem_name, 
		size_t n_shared_mem,
		size_t n_deque_max_size
	)
	{
		bool b_result(false);
		do {
			_ini_common();
			m_b_owner = b_owner;

			if (s_shared_mem_name.empty()) {
				ATLTRACE(L" : %s : s_shared_mem_name.empty().\n", __WFUNCTION__);
				continue;
			}
			
			m_s_shared_mem_name = s_shared_mem_name;
			m_n_deque_max_size = n_deque_max_size;

			if (m_b_owner) {
				boost::interprocess::shared_memory_object::remove(s_shared_mem_name.c_str());
				boost::interprocess::named_condition::remove(_get_cond_value_empty_queue_name().c_str());
				boost::interprocess::named_condition::remove(_get_cond_value_full_queue_name().c_str());
				boost::interprocess::named_mutex::remove(_get_muetx_name().c_str());
				//
				m_ptr_shm_mutex = std::shared_ptr<boost::interprocess::named_mutex>( new boost::interprocess::named_mutex(
					boost::interprocess::create_only,
					_get_muetx_name().c_str()
				));
				if (!m_ptr_shm_mutex) {
					ATLTRACE(L" : %s : m_ptr_shm_mutex is empty.\n", __WFUNCTION__);
					continue;
				}

				//Condition to wait when the queue is empty
				m_ptr_shm_cv_pushed = std::shared_ptr< boost::interprocess::named_condition>( new boost::interprocess::named_condition(
					boost::interprocess::create_only,
					_get_cond_value_empty_queue_name().c_str()
				));
				if (!m_ptr_shm_cv_pushed) {
					ATLTRACE(L" : %s : m_ptr_shm_cv_empty is empty.\n", __WFUNCTION__);
					continue;
				}

				//Condition to wait when the queue is full
				m_ptr_shm_cv_full = std::shared_ptr < boost::interprocess::named_condition>( new boost::interprocess::named_condition(
					boost::interprocess::create_only,
					_get_cond_value_full_queue_name().c_str()
				));
				if (!m_ptr_shm_cv_full) {
					ATLTRACE(L" : %s : m_ptr_shm_cv_full is empty.\n", __WFUNCTION__);
					continue;
				}

				m_ptr_mshm = std::shared_ptr<boost::interprocess::managed_shared_memory>( new boost::interprocess::managed_shared_memory(
					boost::interprocess::create_only,
					m_s_shared_mem_name.c_str(),
					n_shared_mem
				));
				if (!m_ptr_mshm) {
					ATLTRACE(L" : %s : m_ptr_mshm is empty.\n", __WFUNCTION__);
					continue;
				}

				m_p_shm_deque = m_ptr_mshm->construct<shm_deque::_type_shm_deque>(
					_get_deque_name().c_str()
					)(m_ptr_mshm->get_segment_manager());
				if (!m_p_shm_deque) {
					ATLTRACE(L" : %s : m_p_shm_deque is empty.\n", __WFUNCTION__);
					continue;
				}

				b_result = true;
				continue;
			}

			//client
			m_ptr_shm_mutex = std::shared_ptr<boost::interprocess::named_mutex>(new boost::interprocess::named_mutex(
				boost::interprocess::open_only,
				_get_muetx_name().c_str()
			));
			if (!m_ptr_shm_mutex) {
				ATLTRACE(L" : %s : m_ptr_shm_mutex is empty.\n", __WFUNCTION__);
				continue;
			}

			//Condition to wait when the queue is empty
			m_ptr_shm_cv_pushed = std::shared_ptr< boost::interprocess::named_condition>(new boost::interprocess::named_condition(
				boost::interprocess::open_only,
				_get_cond_value_empty_queue_name().c_str()
			));
			if (!m_ptr_shm_cv_pushed) {
				ATLTRACE(L" : %s : m_ptr_shm_cv_empty is empty.\n", __WFUNCTION__);
				continue;
			}

			//Condition to wait when the queue is full
			m_ptr_shm_cv_full = std::shared_ptr < boost::interprocess::named_condition>(new boost::interprocess::named_condition(
				boost::interprocess::open_only,
				_get_cond_value_full_queue_name().c_str()
			));
			if (!m_ptr_shm_cv_full) {
				ATLTRACE(L" : %s : m_ptr_shm_cv_full is empty.\n", __WFUNCTION__);
				continue;
			}
			//
			m_ptr_mshm = std::shared_ptr<boost::interprocess::managed_shared_memory>(new boost::interprocess::managed_shared_memory(
				boost::interprocess::open_only,
				m_s_shared_mem_name.c_str()
			));
			if (!m_ptr_mshm) {
				ATLTRACE(L" : %s : m_ptr_mshm is empty.\n", __WFUNCTION__);
				continue;
			}

			m_p_shm_deque = m_ptr_mshm->find<shm_deque::_type_shm_deque>(
				_get_deque_name().c_str()
				).first;
			if (!m_p_shm_deque) {
				ATLTRACE(L" : %s : m_p_shm_deque is empty.\n", __WFUNCTION__);
				continue;
			}

			b_result = true;
		} while (false);

		if (m_b_owner && !b_result && !m_s_shared_mem_name.empty()) {
			boost::interprocess::shared_memory_object::remove(s_shared_mem_name.c_str());
			boost::interprocess::named_condition::remove(_get_cond_value_empty_queue_name().c_str());
			boost::interprocess::named_condition::remove(_get_cond_value_full_queue_name().c_str());
			boost::interprocess::named_mutex::remove(_get_muetx_name().c_str());

		}
		return b_result;
	}

	std::string _get_muetx_name()
	{
		std::string s_name;
		if (!m_s_shared_mem_name.empty()) {
			s_name = m_s_shared_mem_name + "_mutex__";
		}
		return s_name;
	}
	std::string _get_cond_value_empty_queue_name()
	{
		std::string s_name;
		if (!m_s_shared_mem_name.empty()) {
			s_name = m_s_shared_mem_name + "_condition_value_empty__";
		}
		return s_name;
	}
	std::string _get_cond_value_full_queue_name()
	{
		std::string s_name;
		if (!m_s_shared_mem_name.empty()) {
			s_name = m_s_shared_mem_name + "_condition_value_full__";
		}
		return s_name;
	}
	std::string _get_deque_name()
	{
		std::string s_name;
		if (!m_s_shared_mem_name.empty()) {
			s_name = m_s_shared_mem_name + "_deque__";
		}
		return s_name;
	}


private:
	bool m_b_ini;
	bool m_b_owner;
	std::string m_s_shared_mem_name;
	size_t m_n_deque_max_size;
	//
	std::shared_ptr<boost::interprocess::named_mutex> m_ptr_shm_mutex;
	std::shared_ptr< boost::interprocess::named_condition> m_ptr_shm_cv_pushed, m_ptr_shm_cv_full;

	std::shared_ptr<boost::interprocess::managed_shared_memory> m_ptr_mshm;
	shm_deque::_type_shm_deque *m_p_shm_deque;

};