#ifndef TIMERTHREAD_H_INCLUDED
#define TIMERTHREAD_H_INCLUDED
#include <boost/thread.hpp>
#include "TimerServiceConfig.h"

class CTimerThread
{
public:
    static CTimerThread* GetInstance();
    void Start();
    void Stop();

    void setRequests(const vector<SConfigRequest>& vecRequest){m_vecRequest=vecRequest;}
    bool isAlive();
private:
    void Run();

    void dump_sap_packet_info(const void *pBuffer);

    CTimerThread();
    ~CTimerThread();
    CTimerThread(const CTimerThread& rhd);
    CTimerThread operator=(const CTimerThread& rhd);
private:
    boost::thread m_thread;
    vector<SConfigRequest> m_vecRequest;
    static unsigned int m_seq;
    bool bRun;
};


#endif // TIMERTHREAD_H_INCLUDED
