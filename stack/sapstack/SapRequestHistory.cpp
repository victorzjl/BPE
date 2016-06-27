#include "SapRequestHistory.h"
#include <boost/asio.hpp>
#include <time.h>
#include "SapLogHelper.h"
#include "SapAgent.h"
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;

namespace sdo{
namespace sap{
unsigned int CSapMsgTimerList::sm_nRequestTimeout=10;
void CSapMsgTimerList::SetRequestTimeout(unsigned int timeout)
{
    sm_nRequestTimeout=timeout;
}
CSapMsgTimerList::CSapMsgTimerList()
{
    m_stTimeoutResponse.byIdentifer=SAP_PACKET_RESPONSE;
    m_stTimeoutResponse.byHeadLen=sizeof(SSapMsgHeader);
    m_stTimeoutResponse.dwPacketLen=htonl(sizeof(SSapMsgHeader));
    m_stTimeoutResponse.dwCode=htonl(SAP_CODE_STACK_TIMEOUT);
}
CSapMsgTimerList::~CSapMsgTimerList()
{
    m_history.clear();
	m_timers.clear();
}

void CSapMsgTimerList::AddHistory(int nId,unsigned int dwSequence,unsigned int dwServiceId,unsigned int dwMsgId,unsigned int timeout)
{
    SSapRequestHistory oRequest;
    oRequest.nId=nId;
    oRequest.dwSequence=dwSequence;
    oRequest.dwServiceId=dwServiceId;
    oRequest.dwMsgId=dwMsgId;
    
	gettimeofday_a(&(oRequest.starttime),0);
    struct timeval_a interval;
    unsigned int nTimout=(timeout<=0?sm_nRequestTimeout:timeout);
	interval.tv_sec=nTimout;
	interval.tv_usec=0;
    timeradd(&(oRequest.starttime),&interval,&oRequest.m_stEndtime);

    m_timers.insert(MMHistoryTimers::value_type(oRequest.m_stEndtime,oRequest));
    m_history[dwSequence]=oRequest;
}

bool CSapMsgTimerList::RemoveHistoryBySequence(unsigned int dwSequence,unsigned int dwCode)
{
    SS_XLOG(XLOG_DEBUG,"CSapMsgTimerList::RemoveHistoryBySequence,sequence[%d]\n",htonl(dwSequence));
    MHistory::iterator itrHistory=m_history.find(dwSequence);
	
    if(itrHistory!=m_history.end())
    {
        SS_XLOG(XLOG_DEBUG,"CSapMsgTimerList::RemoveHistoryBySequence,execute remove!\n");
        SSapRequestHistory & oRequest=itrHistory->second;
        
        std::pair<MMHistoryTimers::iterator, MMHistoryTimers::iterator> itr_pair = m_timers.equal_range(oRequest.m_stEndtime);
        MMHistoryTimers::iterator itr;
        for(itr=itr_pair.first; itr!=itr_pair.second;itr++)
        {
            SSapRequestHistory & oQueryRequest=itr->second;
            if(oQueryRequest.Equal(oRequest))
            {
                m_timers.erase(itr);
                break;
            }
        }
		if(IsEnableLevel(SAPPER_STACK_MODULE,XLOG_INFO))
        {      
    		struct tm tmStart;
            struct tm tmEnd;
    		
    		struct timeval_a now;
    		gettimeofday_a(&now,0);
    		
    		/*localtime_r(&(oRequest.starttime.tv_sec),&tmStart);
    		localtime_r(&(now.tv_sec),&tmEnd);*/
			ptime pStart = from_time_t(oRequest.starttime.tv_sec);
			tmStart = to_tm(pStart);

			ptime pEnd = second_clock::local_time();
			tmEnd = to_tm(pEnd);
    		
    		char szTmp[1024]={0};
    		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour,tmStart.tm_min,tmStart.tm_sec,(unsigned int)(oRequest.starttime.tv_usec/1000));
    		string strStartTime = szTmp;

    		memset(szTmp,0,1024);
    		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmEnd.tm_year+1900,tmEnd.tm_mon+1,tmEnd.tm_mday,tmEnd.tm_hour,tmEnd.tm_min,tmEnd.tm_sec,(unsigned int)(now.tv_usec/1000));
    		string strEndTime = szTmp;

            struct timeval_a tmInterval;
            timersub(&now,&(oRequest.starttime),&tmInterval);
    		
    		//LOG:nid,serviceid,msgid,sequence,starttime,interval(ms),OUT
    		SP_XLOG(XLOG_NOTICE,"%u,%u,%u,%u,%s,%s,%d.%03d,%u,OUT\n",oRequest.nId,oRequest.dwServiceId,oRequest.dwMsgId,htonl(oRequest.dwSequence),strStartTime.c_str(),strEndTime.c_str(),tmInterval.tv_sec*1000+tmInterval.tv_usec/1000,tmInterval.tv_usec%1000,dwCode);
        }
		m_history.erase(itrHistory);
        
        return true;
    }
    else
    {
        SS_XLOG(XLOG_WARNING,"CSapMsgTimerList::RemoveHistoryBySequence,no sequence[%d]\n",htonl(dwSequence));
        return false;
    }
    
}
void CSapMsgTimerList::DetectTimerList()
{
    struct timeval_a now;
	gettimeofday_a(&now,0);
    while(!m_timers.empty())
    {
        struct timeval_a curTimerEnd=m_timers.begin()->first;
        if(curTimerEnd<now)
        {
            SSapRequestHistory & oRequest=m_timers.begin()->second;
            
            SS_XLOG(XLOG_WARNING,"CSapMsgTimerList::OnRequstTimeout,id[%d],sequence[%d]\n",oRequest.nId,htonl(oRequest.dwSequence));
            m_stTimeoutResponse.dwServiceId=oRequest.dwServiceId;
            m_stTimeoutResponse.dwMsgId=oRequest.dwMsgId;
            m_stTimeoutResponse.dwSequence=oRequest.dwSequence;

            CSapAgent::Instance()->OnReceiveMessage(oRequest.nId,&m_stTimeoutResponse,sizeof(m_stTimeoutResponse),"",0);

            if(IsEnableLevel(SAPPER_STACK_MODULE,XLOG_INFO))
            {      
        		struct tm tmStart;
        		struct tm tmEnd;
        		
        		/*localtime_r(&(oRequest.starttime.tv_sec),&tmStart);
        		localtime_r(&(now.tv_sec),&tmEnd);*/
        		ptime pStart = from_time_t(oRequest.starttime.tv_sec);
				tmStart = to_tm(pStart);

				ptime pEnd = second_clock::local_time();
				tmEnd = to_tm(pEnd);

        		char szTmp[1024]={0};
        		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmStart.tm_year+1900,tmStart.tm_mon+1,tmStart.tm_mday,tmStart.tm_hour,tmStart.tm_min,tmStart.tm_sec,(unsigned int)(oRequest.starttime.tv_usec/1000));
        		string strStartTime = szTmp;

        		memset(szTmp,0,1024);
        		sprintf(szTmp,"%04u-%02u-%02u %02u:%02u:%02u.%03u",tmEnd.tm_year+1900,tmEnd.tm_mon+1,tmEnd.tm_mday,tmEnd.tm_hour,tmEnd.tm_min,tmEnd.tm_sec,(unsigned int)(now.tv_usec/1000));
        		string strEndTime = szTmp;

                struct timeval_a tmInterval;
                timersub(&now,&(oRequest.starttime),&tmInterval);
        		
        		//LOG:nid,serviceid,msgid,sequence,starttime,interval(ms),code,OUT
        		SP_XLOG(XLOG_NOTICE,"%u,%u,%u,%u,%s,%s,%d.%03d,%d,OUT\n",oRequest.nId,oRequest.dwServiceId,oRequest.dwMsgId,htonl(oRequest.dwSequence),strStartTime.c_str(),strEndTime.c_str(),tmInterval.tv_sec*1000+tmInterval.tv_usec/1000,tmInterval.tv_usec%1000,SAP_CODE_STACK_TIMEOUT);
            }
	    m_history.erase(oRequest.dwSequence);               
            m_timers.erase(m_timers.begin());
        }
        else
        {
            break;
        }
    }
}
bool CSapMsgTimerList::IsExistSequence(unsigned int dwSequence)
{
    SS_XLOG(XLOG_DEBUG,"CSapMsgTimerList::IsExistSequence,sequence[%d]\n",htonl(dwSequence));
    MHistory::iterator itrHistory=m_history.find(dwSequence);
    if(itrHistory!=m_history.end())
    {
        return true;
    }
    else
    {
        SS_XLOG(XLOG_WARNING,"CSapMsgTimerList::IsExistSequence,no this sequence[%d]\n",htonl(dwSequence));
        return false;
    }
}
}
}

