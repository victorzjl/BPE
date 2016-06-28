#ifndef _RW_LOCK_H_
#define _RW_LOCK_H_

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

class RWLock
{
public:
	RWLock();
	~RWLock();
	void rd_lock();
	void rd_unlock();
	void wr_lock();
	void wr_unlock();
private :
    // pthread_mutex_t cnt_mutex;
    // pthread_cond_t rd_cond;
    // pthread_cond_t wr_cond;

    boost::mutex m_mut;
    boost::condition_variable  m_readCond;
    boost::condition_variable  m_writeCond;

    int rd_wait_cnt,wr_wait_cnt;
    int rd_cnt, wr_cnt;

    RWLock(const RWLock&);
    RWLock& operator= (const RWLock&);
};
#endif
