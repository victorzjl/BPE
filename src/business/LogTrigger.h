#ifndef __LOG_TRIGGER_H__
#define __LOG_TRIGGER_H__
#include "TriggerConfig.h"

namespace sdo
{
namespace commonsdk
{
class CAvenueMsgHandler;
class CAvenueServiceConfigs;
}
}

using sdo::commonsdk::CAvenueServiceConfigs;
using sdo::commonsdk::CAvenueMsgHandler;

struct stDefParameter;
struct stDefValue;

typedef struct stLogInfo
{	
	STriggerItem *pItem;
	string serviceName;	
	/*string key1[2]; // main-key of query log 
	string key2[2]; // sub-key 
	string key3[2];*/
    map<int, string> mapKey;
    map<int, string> mapKey2;
	string strReq[2];
	string logInfo;
    string logInfo2;
	string plusLogInfo;
	bool isBNB;
	stLogInfo(STriggerItem *pItem_=NULL, const string& serviceName_="")
		:pItem(pItem_),serviceName(serviceName_),isBNB(false)
	{}
}SLogInfo;

const int BNB_SERVICEID = 50003;

class CLogTrigger
{
public:
	CLogTrigger(CAvenueServiceConfigs *pConfig=NULL):m_pConfig(pConfig){}
	~CLogTrigger(){}
	void SetConfig(CAvenueServiceConfigs *pConfig) {m_pConfig = pConfig;}
	//forward message
	void BeginTrigger(const void *pBuffer, int nLen, SLogInfo &sLog);
	void EndTrigger(const void *pBuffer, int nLen, SLogInfo &sLog);
	
	static string GetDefaultLog(int nServId, int nMsgId);	
	//compose service
	static void BeginTrigger(CAvenueMsgHandler *pHandler, int nServId, int nMsgId, SLogInfo& sInfo);
    static void EndTrigger(const void *pBody, int nLen,CAvenueMsgHandler *pHandler, SLogInfo &sLog, const map<string, vector<string> >* mapDefValue = NULL);
    static void BeginDataTrigger(SLogInfo& sInfo, CAvenueMsgHandler *pReqHandler, CAvenueMsgHandler *pRspHandler, int nServId, int nMsgId, const map<string, vector<string> >* mapDefValue = NULL);
    static void EndDataTrigger(SLogInfo &sLog, const map<string, vector<string> >* mapDefValue = NULL);

private:
	CAvenueServiceConfigs* m_pConfig;		
	static const int TRIGGERTYPE  = 1;		
};
#endif

