#ifndef _SDO_SAP_RECORD_H_
#define _SDO_SAP_RECORD_H_
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
using std::vector;
using std::string;
using std::stringstream;
using std::multimap;
typedef struct stOperate
{	
	string strReqTime;
	unsigned int dwServiceId;
	unsigned int dwMsgId;
	string strDstIp;
	unsigned int dwInterval;
	unsigned int dwCode;
}SOperate;

class TServiceIdMessageId 
{
public:
	unsigned int dwServiceId;
	unsigned int dwMessageId;
	bool operator <(const TServiceIdMessageId &other) const 
	{
		if (this->dwServiceId < other.dwServiceId)
			return true;
		else if (this->dwServiceId == other.dwServiceId)
		{
			if (this->dwMessageId < other.dwMessageId)
				return true;
		}
		return false;
	}
};



typedef struct stSosStruct
{
	vector<unsigned int> vecServiceId;
	vector<string> vecAddr;
	vector<TServiceIdMessageId> vecServiceIdMessageId;
	static string FormatServiceId(const vector<unsigned int> &vecServiceId)
	{
		stringstream sstr;
		bool frist = true;
		vector<unsigned int> vtService = vecServiceId;
		for (vector<unsigned int>::iterator iter = vtService.begin();
			iter != vtService.end(); ++iter)
		{
			if (!frist)
				sstr << ",";
			else
				frist = false;
			sstr << (*iter);
		}
		return sstr.str();
	}
}SSosStruct;


typedef struct stSosStatusData
{
	int nIndex;
	vector<unsigned int> vecServiceId;
	string strAddr;
}SSosStatusData;


typedef struct stServiceStatisData
{
	unsigned int dwRequest;
	unsigned int dwSucc;
	unsigned int dwFail;
	unsigned int dwReqs10ms;//# of request [0,10)	
	unsigned int dwReqs50ms;	
	unsigned int dwReqs250ms;
	unsigned int dwReqs1s;
	unsigned int dwReqs3s;
	unsigned int dwReqsOthers;	

	stServiceStatisData():dwRequest(0),dwSucc(0),dwFail(0),dwReqs10ms(0),dwReqs50ms(0),
		dwReqs250ms(0),dwReqs1s(0),	dwReqs3s(0),dwReqsOthers(0){}	
	
	stServiceStatisData(unsigned dwReq_,unsigned dwSucc_,unsigned dwFail_,unsigned idwReqs10ms, 
		unsigned idwReqs50ms,unsigned idwReqs250ms,unsigned idwReqs1s,unsigned idwReqs5s,unsigned idwReqsOthers)
		:dwRequest(dwReq_),dwSucc(dwSucc_),dwFail(dwFail_),
		dwReqs10ms(idwReqs10ms),dwReqs50ms(idwReqs50ms),dwReqs250ms(idwReqs250ms),
		dwReqs1s(idwReqs1s),dwReqs3s(idwReqs5s),dwReqsOthers(idwReqsOthers)
		{}
		
	stServiceStatisData& operator+=(const stServiceStatisData& lhds)
	{
		dwRequest += lhds.dwRequest;
		dwSucc += lhds.dwSucc;
		dwFail += lhds.dwFail;
		dwReqs10ms += lhds.dwReqs10ms;
		dwReqs50ms += lhds.dwReqs50ms;
		dwReqs250ms += lhds.dwReqs250ms;
		dwReqs1s += lhds.dwReqs1s;
		dwReqs3s += lhds.dwReqs3s;
		dwReqsOthers += lhds.dwReqsOthers;
		return *this;
	}	
	friend const stServiceStatisData operator -(const stServiceStatisData& rhds,
		const stServiceStatisData& lhds )
    {
		return stServiceStatisData(
			rhds.dwRequest-lhds.dwRequest,rhds.dwSucc-lhds.dwSucc,rhds.dwFail-lhds.dwFail,
			rhds.dwReqs10ms-lhds.dwReqs10ms, rhds.dwReqs50ms-lhds.dwReqs50ms,rhds.dwReqs250ms-lhds.dwReqs250ms,
			rhds.dwReqs1s-lhds.dwReqs1s, rhds.dwReqs3s-lhds.dwReqs3s,
			rhds.dwReqsOthers-lhds.dwReqsOthers);
	}
}SServiceStatisData;


typedef struct stSubServiceStatisData
{
	unsigned int dwRequest;
	unsigned int dwSucc;
	unsigned int dwFail;
	unsigned int dwReqs10ms;//# of request [0,10)	
	unsigned int dwReqs50ms;	
	unsigned int dwReqs150ms;
	unsigned int dwReqs250ms;
	unsigned int dwReqs1s;
	unsigned int dwReqsOthers;
	unsigned int dwTimeOut;

	stSubServiceStatisData():dwRequest(0),dwSucc(0),dwFail(0),dwReqs10ms(0),dwReqs50ms(0),
		dwReqs150ms(0),dwReqs250ms(0),dwReqs1s(0),dwReqsOthers(0),dwTimeOut(0){}	
	
	stSubServiceStatisData(unsigned dwReq_,unsigned dwSucc_,unsigned dwFail_,unsigned idwReqs10ms, 
		unsigned idwReqs50ms,unsigned idwReqs150ms,unsigned idwReqs250ms,
		unsigned idwReqs1s,unsigned idwReqsOthers,unsigned dwTimeOut_)
		:dwRequest(dwReq_),dwSucc(dwSucc_),dwFail(dwFail_),
		dwReqs10ms(idwReqs10ms),dwReqs50ms(idwReqs50ms),dwReqs150ms(idwReqs150ms),dwReqs250ms(idwReqs250ms),
		dwReqs1s(idwReqs1s),dwReqsOthers(idwReqsOthers),dwTimeOut(dwTimeOut_)
		{}
		
	stSubServiceStatisData& operator+=(const stSubServiceStatisData& lhds)
	{
		dwRequest += lhds.dwRequest;
		dwSucc += lhds.dwSucc;
		dwFail += lhds.dwFail;
		dwReqs10ms += lhds.dwReqs10ms;
		dwReqs50ms += lhds.dwReqs50ms;
		dwReqs150ms += lhds.dwReqs150ms;
		dwReqs250ms += lhds.dwReqs250ms;
		dwReqs1s += lhds.dwReqs1s;		
		dwReqsOthers += lhds.dwReqsOthers;
		dwTimeOut += lhds.dwTimeOut;
		return *this;
	}	
	friend const stSubServiceStatisData operator -(const stSubServiceStatisData& rhds,
		const stSubServiceStatisData& lhds )
    {
		return stSubServiceStatisData(
			rhds.dwRequest-lhds.dwRequest,rhds.dwSucc-lhds.dwSucc,rhds.dwFail-lhds.dwFail,
			rhds.dwReqs10ms-lhds.dwReqs10ms, rhds.dwReqs50ms-lhds.dwReqs50ms, rhds.dwReqs150ms-lhds.dwReqs150ms,
			rhds.dwReqs250ms-lhds.dwReqs250ms,rhds.dwReqs1s-lhds.dwReqs1s,
			rhds.dwReqsOthers-lhds.dwReqsOthers,rhds.dwTimeOut-lhds.dwTimeOut);
	}
}SSubServiceStatisData;


typedef struct stStatisticData
{
	stStatisticData():dwReqs10ms(0),dwReqs50ms(0),dwReqs250ms(0),dwReqs1s(0),
		dwReqs5s(0),dwReqs30s(0),dwReqsOthers(0),dwSucc(0),dwFail(0){}	
	
	stStatisticData(unsigned int idwReqs10ms, unsigned int idwReqs50ms,unsigned int idwReqs250ms, 
		unsigned int idwReqs1s,unsigned int idwReqs5s,unsigned int idwReqs30s,unsigned int idwReqsOthers,
		unsigned int idwSucc,unsigned int idwFail)
		:dwReqs10ms(idwReqs10ms),dwReqs50ms(idwReqs50ms),dwReqs250ms(idwReqs250ms),dwReqs1s(idwReqs1s),
		dwReqs5s(idwReqs5s),dwReqs30s(idwReqs30s),dwReqsOthers(idwReqsOthers),dwSucc(idwSucc),dwFail(idwFail){}
		
	unsigned int dwReqs10ms;//# of request [0,10)	
	unsigned int dwReqs50ms;	
	unsigned int dwReqs250ms;
	unsigned int dwReqs1s;
	unsigned int dwReqs5s;
	unsigned int dwReqs30s;
	unsigned int dwReqsOthers;	
	unsigned int dwSucc;
	unsigned int dwFail;
	stStatisticData& operator+=(const stStatisticData& lhds)
	{
		dwReqs10ms += lhds.dwReqs10ms;
		dwReqs50ms += lhds.dwReqs50ms;
		dwReqs250ms += lhds.dwReqs250ms;
		dwReqs1s += lhds.dwReqs1s;
		dwReqs5s += lhds.dwReqs5s;
		dwReqs30s += lhds.dwReqs30s;
		dwReqsOthers += lhds.dwReqsOthers;
		dwSucc+=lhds.dwSucc;
		dwFail+=lhds.dwFail;
		return *this;
	}	
	friend const stStatisticData operator -(const stStatisticData& rhds, const stStatisticData& lhds )
    {
		return stStatisticData(rhds.dwReqs10ms-lhds.dwReqs10ms, rhds.dwReqs50ms-lhds.dwReqs50ms,
			rhds.dwReqs250ms-lhds.dwReqs250ms, rhds.dwReqs1s-lhds.dwReqs1s, rhds.dwReqs5s-lhds.dwReqs5s,
			rhds.dwReqs30s-lhds.dwReqs30s, rhds.dwReqsOthers-lhds.dwReqsOthers,
			rhds.dwSucc-lhds.dwSucc, rhds.dwFail-lhds.dwFail);
	}
}SStatisticData;

typedef struct stReportData
{
	unsigned dwRequest;
	unsigned dwSucc;
	unsigned dwFail;
	unsigned dwServiceError;
	unsigned dwSAuthenKPI;//static authen
	unsigned dwDAuthenKPI;//dynamic authen-ekey /ecard
	unsigned dwSDAuthenKPI; //static & dynamic authen--50001/3
	stReportData(unsigned dwRequest_=0, unsigned dwSucc_=0,unsigned dwFail_=0, 
		unsigned dwServiceError_=0, unsigned dwSAuthenKPI_=0, 
		unsigned dwDAuthenKPI_=0, unsigned dwSDAuthenKPI_=0)
		:dwRequest(dwRequest_),dwSucc(dwSucc_),dwFail(dwFail_), dwServiceError(dwServiceError_),
		dwSAuthenKPI(dwSAuthenKPI_),dwDAuthenKPI(dwDAuthenKPI_),dwSDAuthenKPI(dwSDAuthenKPI_)
		{}
	friend const stReportData operator -(const stReportData& rhds, const stReportData& lhds )
    {
		return stReportData(rhds.dwRequest-lhds.dwRequest, 
			rhds.dwSucc-lhds.dwSucc,
			rhds.dwFail-lhds.dwFail, 
			rhds.dwServiceError - lhds.dwServiceError,
			rhds.dwSAuthenKPI-lhds.dwSAuthenKPI,
			rhds.dwDAuthenKPI-lhds.dwDAuthenKPI,
			rhds.dwSDAuthenKPI-lhds.dwSDAuthenKPI);
	}
	stReportData& operator+=(const stReportData& lhds)
	{
		dwRequest += lhds.dwRequest;
		dwSucc += lhds.dwSucc;
		dwFail += lhds.dwFail;
		dwServiceError += lhds.dwServiceError;
		dwSAuthenKPI += lhds.dwSAuthenKPI;
		dwDAuthenKPI += lhds.dwDAuthenKPI;
		dwSDAuthenKPI += lhds.dwSDAuthenKPI;
		return *this;
	}	
}SReportData;

typedef struct stConnectData
{
	unsigned dwConnect;
	unsigned dwDisConnect;
	unsigned dwClient;
	stConnectData(	unsigned dwConnect_=0,unsigned dwDisConnect_=0,unsigned dwClient_=0)
		:dwConnect(dwConnect_),dwDisConnect(dwDisConnect_),dwClient(dwClient_)
		{}
	friend const stConnectData operator -(const stConnectData& rhds, const stConnectData& lhds )
    {
		return stConnectData(rhds.dwConnect-lhds.dwConnect, 
			rhds.dwDisConnect-lhds.dwDisConnect,
			rhds.dwClient);
	}
	stConnectData& operator+=(const stConnectData& lhds)
	{
		dwConnect += lhds.dwConnect;
		dwDisConnect += lhds.dwDisConnect;
		dwClient += lhds.dwClient;
		return *this;
	}	
}SConnectData;

typedef struct stSosStatusInfo
{
	vector<SSosStatusData> vecDeadSos;
	int nDeadSos;
	int nAliveSos;
}SSosStatusInfo;

const int ASC_TIMEOUT_INTERVAL = 60;
const int ASC_TIMEOUT_CHECK = 30;
#endif

