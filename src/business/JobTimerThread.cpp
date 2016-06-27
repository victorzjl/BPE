#include "JobTimerThread.h"
#include "SapLogHelper.h"

void OnRegisterCallback(pFnCallback fn, unsigned interval)
{
	CJobTimerThread::GetInstance()->RegisterCallback(fn, interval);
}

CJobTimerThread* CJobTimerThread::pInstance = NULL;
CJobTimerThread* CJobTimerThread::GetInstance()
{
    if (!pInstance) pInstance=new CJobTimerThread();
    return pInstance;
}


void CJobTimerThread::Start()
{
    SS_XLOG(XLOG_DEBUG,"CJobTimerThread::%s...\n",__FUNCTION__);
    m_thread = boost::thread(boost::bind(&CJobTimerThread::Run,this));
}

void CJobTimerThread::Run()
{
    while(1)
    {
        {
            boost::unique_lock<boost::mutex> lock(m_mut);
            if (!cbAddCntr.empty())
            {
                cbCntr.insert(cbAddCntr.begin(), cbAddCntr.end());
                SS_XLOG(XLOG_DEBUG,"CJobTimerThread::%s, register[%u], orignal[%u]\n",__FUNCTION__, 
                    cbAddCntr.size(), cbCntr.size());
                cbAddCntr.clear();
            }
        }
        time_t tBegin = time(NULL);
        for(cbIterator it= cbCntr.begin(); it!=cbCntr.end(); ++it)
        {
            if (it->first <= tBegin)
            {
                SCbMsg_ptr pCb = it->second;
                pCb->pFn();
                cbCntr.insert(std::make_pair(it->first+pCb->interval, pCb));
                cbCntr.erase(it);
            }
            else
            {
                break;
            }
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}

void CJobTimerThread::RegisterCallback(pFnCallback fn, unsigned interval)
{
    SS_XLOG(XLOG_DEBUG,"CJobTimerThread::%s, interval[%u]\n",__FUNCTION__, interval);
    boost::unique_lock<boost::mutex> lock(m_mut);
    SCbMsg_ptr pCb(new SCallbackMsg(fn, interval));
    cbAddCntr.insert(std::make_pair(time(NULL)+interval, pCb));
}


