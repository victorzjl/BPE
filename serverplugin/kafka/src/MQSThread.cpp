#include <SapMessage.h>
#include <Cipher.h>


#include "MQSThread.h"
#include "MQSThreadGroup.h"
#include "ErrorMsg.h"
#include "AsyncVirtualServiceLog.h"
#include "Common.h"

#ifndef _WIN32
#include <netinet/in.h>
#endif

#define TIMEOUT_INTERVAL  3 
#define CONDITION_TIMES  10
#define TIMEOUT_ALARM_FREQUENCY 10

using namespace sdo::sap;

int CMQSReConnHelperImpl::TryReConnect(void *pContext, std::string& outMsg, unsigned int timeout)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSReConnHelperImpl::%s\n", __FUNCTION__);
	CKafkaProducer *pProducer = (CKafkaProducer *)pContext;
	CKafkaProducer producer(this);
	producer.Initialize(pProducer->GetKafkaBTP());
	int ret = -1;
	if (producer.Connect())
	{
		ret = 0;
	}
	return ret;
}


CMQSThread::CMQSThread( CMsgQueuePrio & oPublicQueue, CMQSThreadGroup *pMQSThreadGroup ):
	CChildMsgThread(oPublicQueue), m_isAlive(true),m_dwTimeoutNum(0),m_pMQSThreadGroup(pMQSThreadGroup),
	m_timerSelfCheck(this, 3, boost::bind(&CMQSThread::DoSelfCheck,this),CThreadTimer::TIMER_CIRCLE),
	m_bIsSelfCheckTimerStopped(true)
{
	m_MQSFunc[MQS_PRODUCE] = &CMQSThread::DoProduce;
	m_MQSFunc[MQS_STOP] = &CMQSThread::DoStop;
	m_MQSFunc[MQS_RECONN] = &CMQSThread::DoReConn;
}

CMQSThread::~CMQSThread()
{
	Stop();
}


void CMQSThread::DoSelfCheck()
{
	m_isAlive = true;
}

bool CMQSThread::GetSelfCheck() 
{
	bool flag = m_isAlive;
	m_isAlive = false;
	return flag;
}

void CMQSThread::StartInThread()
{
	m_timerSelfCheck.Start();
	m_bIsSelfCheckTimerStopped = false;
}

int CMQSThread::Start(const vector<PairServiceInfo>& vecServiceInfo)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	gettimeofday_a(&m_tmLastTimeoutWarn, 0);
	
	for (vector<PairServiceInfo>::const_iterator iter=vecServiceInfo.begin(); 
	     iter!=vecServiceInfo.end(); ++iter)
	{
		CKafkaProducer *pProducer = new CKafkaProducer(this);
		pProducer->Initialize(iter->first);
		for (vector<unsigned int>::const_iterator idIter=iter->second.begin(); 
		     idIter!=iter->second.end(); ++idIter)
		{
			m_mapServiceAndProducer[*idIter] = pProducer;
		}
		
		m_mapBTPAndProducer[iter->first] = pProducer;
		
		if (!pProducer->Connect())
		{
			CReConnThread::GetReConnThread()->OnReConn(this, &m_MQSReConnHelper, pProducer);
		}
	}
	
	CChildMsgThread::Start();
	return 0;
}

void CMQSThread::Stop()
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	SMQSStopMsg *pMQSStopMsg = new SMQSStopMsg;
	gettimeofday_a(&(pMQSStopMsg->tStart),0);
	PutQ(pMQSStopMsg);
	
	do 
	{
		sleep(1);
	}while(!m_bIsSelfCheckTimerStopped);
	
	CChildMsgThread::Stop();
	
	for (map<unsigned int, CKafkaProducer*>::iterator iter = m_mapServiceAndProducer.begin();
	     iter != m_mapServiceAndProducer.end(); ++iter)
	{
		iter->second->Close();
		delete iter->second;
		iter->second = NULL;
	}
	m_mapServiceAndProducer.clear();
}

int CMQSThread::OnAddService(const vector<PairServiceInfo>& vecServiceInfo)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	for (vector<PairServiceInfo>::const_iterator iter=vecServiceInfo.begin(); 
	     iter!=vecServiceInfo.begin(); ++iter)
	{
		CKafkaProducer *pProducer = NULL;
		map<SKafkaBTP, CKafkaProducer*>::iterator iterConnection = m_mapBTPAndProducer.find(iter->first);
		if (iterConnection==m_mapBTPAndProducer.end())
		{
			pProducer = new CKafkaProducer(this);
			pProducer->Initialize(iter->first);
			CReConnThread::GetReConnThread()->OnReConn(this, &m_MQSReConnHelper, pProducer);
		}
		else
		{
			pProducer = iterConnection->second;
		}
		
		m_mapBTPAndProducer[iter->first]=pProducer;
		
		for (vector<unsigned int>::const_iterator idIter=iter->second.begin(); 
		     idIter!=iter->second.end(); ++idIter)
		{
			m_mapServiceAndProducer[*idIter] = pProducer;
		}
	}
	
	return 0;
}

int CMQSThread::OnKafkaError(IKafka *pSrc, const std::string& msg)
{
	/* MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s, pSrc[%p] error[%s]\n", __FUNCTION__, pSrc, msg.c_str());
	
	SMQSReConnMsg *pMQSReConnMsg = new SMQSReConnMsg;
	pMQSReConnMsg->pKafka = pSrc;
	gettimeofday_a(&(pMQSReConnMsg->tStart),0);
	PutQ(pMQSReConnMsg); */
	return 0;
}

int CMQSThread::OnReConn(void *pContext)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	
	SMQSReConnMsg *pMQSReConnMsg = new SMQSReConnMsg;
	pMQSReConnMsg->pKafka = (IKafka*)pContext;
	gettimeofday_a(&(pMQSReConnMsg->tStart),0);
	PutQ(pMQSReConnMsg);
	return 0;
}

void CMQSThread::Deal( void *pData )
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	SMQSMsg *pMQSMsg = (SMQSMsg *)pData;

	//如果超时
	float dwTimeInterval = GetTimeInterval(pMQSMsg->tStart)/1000;

	if(dwTimeInterval > TIMEOUT_INTERVAL)	
	{
		MQS_XLOG(XLOG_ERROR, "CMQSThread::%s, MQSender thread timeout %f seconds\n", __FUNCTION__, dwTimeInterval);

		m_dwTimeoutNum++;
		
		//连续超时告警
		if(m_dwTimeoutNum >= CONDITION_TIMES)
		{
			m_dwTimeoutNum = 0;
			if(GetTimeInterval(m_tmLastTimeoutWarn)/1000 > TIMEOUT_ALARM_FREQUENCY)
			{
				char szWarnInfo[256] = {0};
				snprintf(szWarnInfo, sizeof(szWarnInfo)-1, "MQSender thread timeout %d times over %d seconds", CONDITION_TIMES, TIMEOUT_INTERVAL);
				m_pMQSThreadGroup->Warn(szWarnInfo);
				gettimeofday_a(&m_tmLastTimeoutWarn, 0);
			}
		}

		delete pMQSMsg;
		return;
	}
	else
	{
		m_dwTimeoutNum = 0;
	}

	(this->*(m_MQSFunc[pMQSMsg->m_enMQSOperator]))(pMQSMsg);
	delete pMQSMsg;
}

void CMQSThread::DoProduce( SMQSMsg *pMQSMsg )
{

	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	SMQSProduceMsg *pMQSProduceMsg = (SMQSProduceMsg *)pMQSMsg;
	map<unsigned int, CKafkaProducer*>::iterator iter = m_mapServiceAndProducer.find(pMQSProduceMsg->dwServiceId);
	
	if (iter == m_mapServiceAndProducer.end())
	{
		MQS_XLOG(XLOG_WARNING, "CMQSThread::%s\n, can't find topic for serviceId[%u]\n", __FUNCTION__, pMQSProduceMsg->dwServiceId);
		m_pMQSThreadGroup->Response(pMQSProduceMsg->handler, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                            pMQSProduceMsg->dwSequenceId, MQS_NO_TOPIC);
		m_pMQSThreadGroup->Log(pMQSProduceMsg->strGuid, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                       pMQSProduceMsg->tStart, MQS_NO_TOPIC, "");
		return;
	}
	
	if (!iter->second->IsConnected())
	{
		MQS_XLOG(XLOG_WARNING, "CMQSThread::%s, brokerlist[%s], topic[%s] is not connected.\n", __FUNCTION__,
                        		iter->second->GetKafkaBTP().brokerlist.c_str(), iter->second->GetKafkaBTP().topic.c_str());
		m_pMQSThreadGroup->Response(pMQSProduceMsg->handler, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                            pMQSProduceMsg->dwSequenceId, MQS_CONNECT_ERROR);
		m_pMQSThreadGroup->Log(pMQSProduceMsg->strGuid, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                       pMQSProduceMsg->tStart, MQS_CONNECT_ERROR, "");
		return;
	}
	
	if (!iter->second->PutOneMessage(pMQSProduceMsg->pBuffer, pMQSProduceMsg->dwLen, 3000))
	{
		MQS_XLOG(XLOG_WARNING, "CMQSThread::%s, failed to send message: serviceId[%u], brokerlist[%s], topic[%s]\n", __FUNCTION__,
                        		pMQSProduceMsg->dwServiceId, iter->second->GetKafkaBTP().brokerlist.c_str(), 
								iter->second->GetKafkaBTP().topic.c_str());
		iter->second->Close();
		CReConnThread::GetReConnThread()->OnReConn(this, &m_MQSReConnHelper, iter->second);
		m_pMQSThreadGroup->Response(pMQSProduceMsg->handler, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                            pMQSProduceMsg->dwSequenceId, MQS_PRODUCE_ERROR);
		m_pMQSThreadGroup->Log(pMQSProduceMsg->strGuid, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                       pMQSProduceMsg->tStart, MQS_PRODUCE_ERROR, "");
	}
	else
	{
		m_pMQSThreadGroup->Response(pMQSProduceMsg->handler, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                            pMQSProduceMsg->dwSequenceId, MQS_SUCESS);
		m_pMQSThreadGroup->Log(pMQSProduceMsg->strGuid, pMQSProduceMsg->dwServiceId, pMQSProduceMsg->dwMsgId, 
		                       pMQSProduceMsg->tStart, MQS_SUCESS, "");
	}
}

void CMQSThread::DoStop(SMQSMsg *pMQSMsg)
{
	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	m_timerSelfCheck.Stop();
	m_bIsSelfCheckTimerStopped = true;
}

void CMQSThread::DoReConn(SMQSMsg *pMQSMsg)
{

	MQS_XLOG(XLOG_DEBUG, "CMQSThread::%s\n", __FUNCTION__);
	SMQSReConnMsg *pMQSReConnMsg = (SMQSReConnMsg *)pMQSMsg;
	
	IKafka *pKafka = pMQSReConnMsg->pKafka;
	
	if (NULL == pKafka)
	{
		return;
	}
	
	if (pKafka->IsConnected())
	{
		return;
	}
	
	if (!pKafka->Connect())
	{
		CReConnThread::GetReConnThread()->OnReConn(this, &m_MQSReConnHelper, pKafka);
	}
}

void CMQSThread::Dump()
{
	MQS_XLOG(XLOG_NOTICE, "CMQSThread::%s\n", __FUNCTION__);

	MQS_XLOG(XLOG_NOTICE, "CMQSThread::%s < TimeoutInterval[%d] >\n", __FUNCTION__, TIMEOUT_INTERVAL);
	MQS_XLOG(XLOG_NOTICE, "CMQSThread::%s < ConditionTimes[%d] >\n", __FUNCTION__, CONDITION_TIMES);
	MQS_XLOG(XLOG_NOTICE, "CMQSThread::%s < TimeoutAlarmFrequency[%d] >\n", __FUNCTION__, TIMEOUT_ALARM_FREQUENCY);
	
	string strServiceTopics;
	for (map<unsigned int, CKafkaProducer*>::iterator iter=m_mapServiceAndProducer.begin(); 
	     iter!=m_mapServiceAndProducer.end(); ++iter)
	{
		char buffer[128] = {0};
		snprintf(buffer, 128, "%d-%s-%s ", iter->first, iter->second->GetKafkaBTP().brokerlist.c_str(),
		                                   iter->second->GetKafkaBTP().topic.c_str());
		strServiceTopics += buffer;
	}
	MQS_XLOG(XLOG_NOTICE, "CMQSThread::%s < MapForServiceIdAndTopic[ %s] >\n", __FUNCTION__, strServiceTopics.c_str());
}

