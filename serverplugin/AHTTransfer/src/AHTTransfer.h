#ifndef _SO_AHT_TRANSFER_H_
#define _SO_AHT_TRANSFER_H_

#include "CohPub.h"
#include "AvenueServiceConfigs.h"
#include "HTInfo.h"
#include <map>
#include <vector>

#define IN
#define OUT
#define INOUT
using std::vector;
using std::map;
using std::make_pair;
using namespace HT;
#define COMMON_AHT_VERSION 


class CAHTTransfer
{
public:
	CAHTTransfer();
	~CAHTTransfer();
	int Initialize(const char *szXmlConfigFilePath);
	int AvenueToHttp(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequenceId,IN const char *pAvenueBody, IN unsigned int nAvenueLen,
							OUT char *pHttpReqPacket, INOUT int &nHttpReqPacketLen, OUT string &serverUri, OUT string &serverAddr);

	int HttpToAvenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN const char *pHttpResPacket, IN int nHttpResLen, 
							OUT char *pAvenueBody, INOUT int &nAvenueLen, OUT int &nRetCode);

	//int GetServiceUri(IN unsigned int dwServiceId, IN unsigned int dwMsgId, OUT string &strUri);
	//int GetServiceInfo(OUT vector<sServiceIdMsgId> &vecSidMid);
private:
	//map<sServiceIdMsgId, sServerParam>  m_mapSidMidServerParam;  //存储了某一条服务对应的参数信息	
	//int m_sdpVerify;	
	CAvenueServiceConfigs *m_pAvenueServiceConfigs;
	vector<unsigned int> m_vecServiceIds;
	SID_HOST_MAP m_HostMap;
};

#endif 

