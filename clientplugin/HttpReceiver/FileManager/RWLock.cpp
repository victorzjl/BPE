#include "RWLock.h"
RWLock::RWLock():rd_wait_cnt(0), wr_wait_cnt(0),rd_cnt(0),wr_cnt(0)
{
    // pthread_mutex_init(&cnt_mutex, NULL);
    // pthread_cond_init(&rd_cond, NULL);
    // pthread_cond_init(&wr_cond, NULL);
}

void RWLock::rd_lock()
{
    // pthread_mutex_lock(&cnt_mutex);
    boost::unique_lock<boost::mutex> lock(m_mut);
    while (wr_cnt+wr_wait_cnt >0)
    {
        rd_wait_cnt++;
        // pthread_cond_wait(&rd_cond,&cnt_mutex);
        m_readCond.wait(lock);
        rd_wait_cnt--;
    }

    rd_cnt++;
    // pthread_mutex_unlock(&cnt_mutex);
}

 void RWLock::rd_unlock()
{
    // pthread_mutex_lock(&cnt_mutex);
    boost::unique_lock<boost::mutex> lock(m_mut);
    rd_cnt--;
    if(wr_wait_cnt>0)
    {
        // pthread_cond_signal(&wr_cond);
        m_writeCond.notify_one();
    }
    else if(rd_wait_cnt>0)
    {
    	// pthread_cond_signal(&rd_cond);
        m_readCond.notify_one();
    }
    // pthread_mutex_unlock(&cnt_mutex);
}

void RWLock::wr_lock()
{
    // pthread_mutex_lock(&cnt_mutex);
    boost::unique_lock<boost::mutex> lock(m_mut);
    while (rd_cnt+wr_cnt>0)
    {
        wr_wait_cnt++;
        // pthread_cond_wait(&wr_cond,&cnt_mutex);
        m_writeCond.wait(lock);
        wr_wait_cnt--;
    }
    wr_cnt++;
    // pthread_mutex_unlock(&cnt_mutex);
}

void RWLock::wr_unlock()
{
    // pthread_mutex_lock(&cnt_mutex);
    boost::unique_lock<boost::mutex> lock(m_mut);
    wr_cnt--;
    if(wr_wait_cnt>0)
    {
        // pthread_cond_signal(&wr_cond);
        m_writeCond.notify_one();
    }
    else if(rd_wait_cnt>0)
    {
		// pthread_cond_signal(&rd_cond);
        m_readCond.notify_one();
    }
    // pthread_mutex_unlock(&cnt_mutex);
}

RWLock::~RWLock()
{
    // pthread_mutex_destroy(&cnt_mutex);
    // pthread_cond_destroy(&rd_cond);
    // pthread_cond_destroy(&wr_cond);
}

