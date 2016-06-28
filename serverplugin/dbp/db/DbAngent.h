#ifndef _DB_AGENT_H_
#define _DB_AGENT_H_
#include <map>
#include <vector>
#include <deque>
#include "DbConnection.h"
#include "DbMsg.h"
#include <boost/thread/mutex.hpp>
#include "dbserviceconfig.h"
#include "util.h"

using std::map;
using std::vector;
using std::deque;

enum
{
	QUERY_COMMON,//分库查询单个库，或者不分库
	QUERY_ALL    //分库查询所有
};

class CDbAgent 
{
	public:
		CDbAgent(int,SConnDesc&,int QueryType);
		
		void DoInit();
		
		~CDbAgent();
		void DoWriteRequest(SDbQueryMsg *pQuery);
		void DoReadRequest(SDbQueryMsg *pQuery);
		void DoSmartCommit();
		void DoConnectInfo(SDbConnectMsg* pConnMsg);
		void Dump();

		void DoPing();
		int GetQueueCount(int &nQuery, int&nWrite);
	private:
		int AddDbConn(bool bMaster,int nSubIndex,int nConnectIndex, int nTableFactor, const string &strConn, int dwSmart);
		int AddDbConn(bool bMaster,int nSubIndex, int nConnectIndex, int nTableFactor, vector<string>& strConns, int dwSmart);
		void RecycleDbConn(CDbConnection *pConn);
		
		void  PushQueryDelayMsg(int nConnectIndex,SDbMsg *pMsg);
		SDbMsg* PopQueryDelayMsg(int nConnectIndex);

		void  PushWriteDelayMsg(int nConnectIndex,SDbMsg *pMsg);
		SDbMsg* PopWriteDelayMsg(int nConnectIndex);
		
		void  PushQueryAllDelayMsg(SDbMsg *pMsg);
		SDbMsg* PopQueryAllDelayMsg();
		
		bool DetectMsgTimeout(SDbMsg *pMsg);
		
		void DoQuery(CDbConnection *pDbConn,SDbQueryMsg *pQuery);
		void DoWrite(CDbConnection *pDbConn,SDbQueryMsg *pQuery);
	private:
		boost::mutex m_ConnMutex;
		vector<CDbConnection*>  m_Conns;
		map<int,deque<SDbMsg *> > m_queryDelays;
		map<int,deque<SDbMsg *> >  m_writeDelays;
		deque<SDbMsg *> m_queryAllDelay;
		int m_nTolerance;
	private:
		SConnDesc m_dbConf;
		int  nIndex;
		int  nQueryType;
		int m_nDBFactor;
};

class CDbAgent2
{
	public:
		CDbAgent2(int,SConnDesc&);
		void DoInit();
		~CDbAgent2();		
		CDbAgent* GetAngent(SDbQueryMsg *pQuery);
		void DoPing();
		void HandleSmartCommit();
		CDbAgent* GetCommonAgent(){return m_pCommon;}
		string GetServiceIds(){return m_strServiceIds;}
		void SetServiceIds(const string& services){m_strServiceIds=services;}
	private:
		CDbAgent *m_pCommon;	//分库查询单个库，或者不分库
		CDbAgent *m_pQueryALL;	//分库查询所有
		SConnDesc m_dbConf; 
		int  m_nIndex;
		string m_strServiceIds;
};


#endif


