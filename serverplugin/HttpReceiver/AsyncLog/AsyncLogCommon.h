#ifndef _ASYNC_LOG_COMMON_H_
#define _ASYNC_LOG_COMMON_H_

//used for counting all connect msg of http
typedef struct stHPSStatisticData
{
	stHPSStatisticData():dwConnected(0),dwDisConnect(0),dwConnecting(0),dwRequest(0),dwResponseSucc(0),dwResponseFail(0){}
	unsigned int dwConnected;
	unsigned int dwDisConnect;
	unsigned int dwConnecting;
	
	unsigned int dwRequest;

	unsigned int dwResponseSucc;
	unsigned int dwResponseFail;
	friend stHPSStatisticData operator -(const stHPSStatisticData& a1,const stHPSStatisticData& a2)
	{
		stHPSStatisticData data;
		data.dwConnected=a1.dwConnected-a2.dwConnected;		
		data.dwDisConnect=a1.dwDisConnect-a2.dwDisConnect;
		data.dwConnecting=a1.dwConnecting;
		data.dwRequest=a1.dwRequest-a2.dwRequest;		
		data.dwResponseSucc=a1.dwResponseSucc-a2.dwResponseSucc;
		data.dwResponseFail=a1.dwResponseFail-a2.dwResponseFail;
		return data;
	}
	friend stHPSStatisticData operator +(const stHPSStatisticData& a1,const stHPSStatisticData& a2)
	{
		stHPSStatisticData data;
		data.dwConnected=a1.dwConnected + a2.dwConnected;		
		data.dwDisConnect=a1.dwDisConnect + a2.dwDisConnect;
		data.dwConnecting=a2.dwConnecting + a2.dwConnecting;
		data.dwRequest=a1.dwRequest + a2.dwRequest;		
		data.dwResponseSucc=a1.dwResponseSucc + a2.dwResponseSucc;
		data.dwResponseFail=a1.dwResponseFail + a2.dwResponseFail;
		return data;
	}
}SHPSStatisticData;


typedef struct stSosStatisticsData
{		
	unsigned int dwRequest;
	unsigned int dwResSucc;
	unsigned int dwResFail;
	
	unsigned int dwRes10ms;//# of request [0,10)	
	unsigned int dwRes50ms;	
	unsigned int dwRes250ms;
	unsigned int dwRes1s;
	unsigned int dwRes3s;
	unsigned int dwResOthers;	

	void UpdateData(int nCode, float fSpentTime)
	{
		if(nCode == 0)
		{
			dwResSucc++;
		}
		else
		{
			dwResFail++;
		}

		if(fSpentTime <= 10.0)
		{
			dwRes10ms++;
		}
		else if(fSpentTime <= 50.0)
		{
			dwRes50ms++;
		}
		else if(fSpentTime <= 250.0)
		{
			dwRes250ms++;
		}
		else if(fSpentTime <= 1000.0)
		{
			dwRes1s++;
		}
		else if(fSpentTime <= 3000.0)
		{
			dwRes3s++;
		}
		else
		{
			dwResOthers++;
		}
	}

	friend stSosStatisticsData operator -(const stSosStatisticsData& a1,const stSosStatisticsData& a2)
	{
		stSosStatisticsData data;
		data.dwRequest=a1.dwRequest-a2.dwRequest;	
		data.dwResSucc=a1.dwResSucc-a2.dwResSucc;
		data.dwResFail=a1.dwResFail-a2.dwResFail;
		data.dwRes10ms=a1.dwRes10ms-a2.dwRes10ms;
		data.dwRes50ms=a1.dwRes50ms-a2.dwRes50ms;		
		data.dwRes250ms=a1.dwRes250ms-a2.dwRes250ms;
		data.dwRes1s=a1.dwRes1s-a2.dwRes1s;
		data.dwRes3s=a1.dwRes3s-a2.dwRes3s;	
		data.dwResOthers=a1.dwResOthers-a2.dwResOthers;
		return data;
	}
	friend stSosStatisticsData operator +(const stSosStatisticsData& a1,const stSosStatisticsData& a2)
	{
		stSosStatisticsData data;
		data.dwRequest=a1.dwRequest+a2.dwRequest;	
		data.dwResSucc=a1.dwResSucc+a2.dwResSucc;
		data.dwResFail=a1.dwResFail+a2.dwResFail;
		data.dwRes10ms=a1.dwRes10ms+a2.dwRes10ms;
		data.dwRes50ms=a1.dwRes50ms+a2.dwRes50ms;		
		data.dwRes250ms=a1.dwRes250ms+a2.dwRes250ms;
		data.dwRes1s=a1.dwRes1s+a2.dwRes1s;
		data.dwRes3s=a1.dwRes3s+a2.dwRes3s;	
		data.dwResOthers=a1.dwResOthers+a2.dwResOthers;
		return data;
	}
	bool operator ==(int nValue)
	{
		return ((int)dwRequest == nValue) &&
			((int)dwResSucc == nValue) &&
			((int)dwResFail == nValue);
	}

	
	stSosStatisticsData():dwRequest(0),dwResSucc(0),dwResFail(0),
		dwRes10ms(0), dwRes50ms(0), dwRes250ms(0), dwRes1s(0),dwRes3s(0),dwResOthers(0) {}	
}SSosStatisticData;




typedef struct stReqResStatisticData
{
	unsigned int dwReqNum;
	unsigned int dwResSuccNum;
	unsigned int dwResFailNum;

	void UpdateData(int nCode)
	{
		nCode == 0 ? dwResSuccNum++ : dwResFailNum++;
	}
	
	friend stReqResStatisticData operator -(const stReqResStatisticData& a1,const stReqResStatisticData& a2)
	{
		stReqResStatisticData data;
		data.dwReqNum = a1.dwReqNum - a2.dwReqNum;	
		data.dwResSuccNum = a1.dwResSuccNum - a2.dwResSuccNum;
		data.dwResFailNum = a1.dwResFailNum - a2.dwResFailNum;
		return data;
	}
	friend stReqResStatisticData operator +(const stReqResStatisticData& a1,const stReqResStatisticData& a2)
	{
		stReqResStatisticData data;
		data.dwReqNum = a1.dwReqNum + a2.dwReqNum;	
		data.dwResSuccNum = a1.dwResSuccNum + a2.dwResSuccNum;
		data.dwResFailNum = a1.dwResFailNum + a2.dwResFailNum;

		return data;
	}
	bool operator ==(int nValue)
	{
		return ((int)dwReqNum == nValue) && ((int)dwResSuccNum == nValue) && ((int)dwResFailNum == nValue);
	}

	stReqResStatisticData():dwReqNum(0),dwResSuccNum(0),dwResFailNum(0){}
}ReqResStatisticData;

typedef struct stTimeStatisticData
{
	unsigned int dwRes10ms;//# of request [0,10)	
	unsigned int dwRes50ms;	
	unsigned int dwRes250ms;
	unsigned int dwRes1s;
	unsigned int dwRes3s;
	unsigned int dwResOthers;	

	void UpdateData(float fSpentTime)
	{
		if(fSpentTime <= 10.0){
			dwRes10ms++;
		}
		else if(fSpentTime <= 50.0){
			dwRes50ms++;
		}
		else if(fSpentTime <= 250.0){
			dwRes250ms++;
		}
		else if(fSpentTime <= 1000.0){
			dwRes1s++;
		}
		else if(fSpentTime <= 3000.0){
			dwRes3s++;
		}
		else{
			dwResOthers++;
		}
	}

	friend stTimeStatisticData operator -(const stTimeStatisticData &a1,const stTimeStatisticData &a2)
	{
		stTimeStatisticData data;
		data.dwRes10ms=a1.dwRes10ms-a2.dwRes10ms;
		data.dwRes50ms=a1.dwRes50ms-a2.dwRes50ms;		
		data.dwRes250ms=a1.dwRes250ms-a2.dwRes250ms;
		data.dwRes1s=a1.dwRes1s-a2.dwRes1s;
		data.dwRes3s=a1.dwRes3s-a2.dwRes3s;	
		data.dwResOthers=a1.dwResOthers-a2.dwResOthers;
		return data;
	}
	friend stTimeStatisticData operator +(const stTimeStatisticData &a1,const stTimeStatisticData &a2)
	{
		stTimeStatisticData data;
		data.dwRes10ms=a1.dwRes10ms+a2.dwRes10ms;
		data.dwRes50ms=a1.dwRes50ms+a2.dwRes50ms;		
		data.dwRes250ms=a1.dwRes250ms+a2.dwRes250ms;
		data.dwRes1s=a1.dwRes1s+a2.dwRes1s;
		data.dwRes3s=a1.dwRes3s+a2.dwRes3s;	
		data.dwResOthers=a1.dwResOthers+a2.dwResOthers;
		return data;
	}

	stTimeStatisticData():dwRes10ms(0), dwRes50ms(0), dwRes250ms(0), dwRes1s(0),dwRes3s(0),dwResOthers(0){}
}STimeStatisticData;

typedef struct stAvenueReqStatisticData
{
	ReqResStatisticData sReqResStatisticData;
	ReqResStatisticData sHttpServerStatisticData;
	STimeStatisticData sAllTimeStatisticData;
	STimeStatisticData sHttpServerTimeStatisticData;

	friend stAvenueReqStatisticData operator -(const stAvenueReqStatisticData& a1,const stAvenueReqStatisticData& a2)
	{
		stAvenueReqStatisticData data;
		data.sReqResStatisticData = a1.sReqResStatisticData - a2.sReqResStatisticData;
		data.sAllTimeStatisticData = a1.sAllTimeStatisticData - a2.sAllTimeStatisticData;
		data.sHttpServerStatisticData = a1.sHttpServerStatisticData - a2.sHttpServerStatisticData;
		data.sHttpServerTimeStatisticData = a1.sHttpServerTimeStatisticData - a2.sHttpServerTimeStatisticData;

		return data;
	}
	friend stAvenueReqStatisticData operator +(const stAvenueReqStatisticData& a1,const stAvenueReqStatisticData& a2)
	{
		stAvenueReqStatisticData data;
		data.sReqResStatisticData = a1.sReqResStatisticData + a2.sReqResStatisticData;
		data.sAllTimeStatisticData = a1.sAllTimeStatisticData + a2.sAllTimeStatisticData;
		data.sHttpServerStatisticData = a1.sHttpServerStatisticData + a2.sHttpServerStatisticData;
		data.sHttpServerTimeStatisticData = a1.sHttpServerTimeStatisticData + a2.sHttpServerTimeStatisticData;

		return data;
	}
	bool operator ==(int nValue)
	{
		return sReqResStatisticData == nValue;
	}
	
	stAvenueReqStatisticData(){}	
}SAvenueReqStatisticData;




#endif
