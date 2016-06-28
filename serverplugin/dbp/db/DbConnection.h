#ifndef _DB_CONN_H_
#define _DB_CONN_H_

#include "soci.h"
#include "dbconncommon.h"
#include "dbserviceconfig.h"

#include "DbMsg.h"
#include <string>

#include "soci-oracle.h"
#include "soci-mysql.h"
#include "soci-odbc.h"
#include "AvenueSmoothMessage.h"

#include <boost/variant.hpp>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

using namespace soci;
using namespace std;
using namespace sdo::commonsdk;


using boost::variant;
using sdo::commonsdk::CAvenueSmoothDecoder;
using sdo::commonsdk::CAvenueSmoothEncoder;
using soci::details::session_backend;

typedef variant<int,string> DBValue;
typedef vector<DBValue> DBRow;

typedef variant<int,string> DBStructElement;

typedef map<string,variant<int,string> > ptree;

class CDbAgent;

typedef struct DyncData
{
	EBindDataType  m_eDataType;
	vector<string> m_strings;
	vector<int>    m_integers;
}SDyncData;

typedef map<string,SDyncData> SSqlDyncData; //bindname=>data

typedef struct ProcedureResBindData
{
	EBindDataType  m_eDataType;
	string 		   m_string;
	int 		   m_integer;
}SProcedureResBindData;

typedef struct
{
	string 		   m_type;	//string,int
	string 		   m_name;  //$rsp.s1.xx
	string 		   m_string;
	int 		   m_integer;
}SqlRespData;

class CDbConnection;

struct CConnectControl
{
	boost::mutex mut;
	boost::condition_variable  cond;
	CDbConnection* pConnection;
	bool bFirstConnect;
};

class CDbConnection
{

public:
	CDbConnection();
	virtual ~CDbConnection();
	friend void* Thread_ConnectDB(void *pConnection);
public:
	void  SetDbAngent(CDbAgent*p){m_pAngent = p;}	
	CDbAgent* GetDbAngent(){return m_pAngent;}
	void SetPreSql(const string& sql) {m_strPreSql=sql;}
	int Procedure(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,map<string,SBindDataDesc>& mapResParams,
				map<string,SProcedureResBindData>& ResBinding,int& nAffectedRows,SLastErrorDesc& errDesc);
	
	int Query(const string& strSQL,SSqlDyncData& rowData,vector<DBRow>& fetchResult,SLastErrorDesc& errDesc);
	int Query(const string& strSQL,vector<DBRow>& fetchResult,SLastErrorDesc& errDesc);

	virtual int ExecuteProcedure();
	int Execute(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc);
	int Execute(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc);
	int Execute(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc);
	
	void SmartCommitNow();
private:
	int ExecuteSmart(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc);
	int ExecuteNow(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc);
	int ExecuteNow(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc);
	int ExecuteNow(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc);
	int ExecuteSmart(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc);
	int ExecuteSmart(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc);
	void SmartAlloc();
	void SmartRollBack();
	void SmartCommit();
	bool IsHybridSql(vector<string>& vecSQL);
public:
	int DoConnectDb(int nIndex,int nSubIndex, bool bMaster,vector<string> &strConns);	
	
	int DoConnectDb(int nIndex, int nSubIndex, int nConnectIndex,bool bMaster, const string &strConn);
	int DoRealConnectDb(int nIndex, int nSubIndex, int nConnectIndex,bool bMaster, const string &strConn);
	int DoReConnectDb();	
	int DoRealReConnectDb();
	
	int DoQuery(SDbQueryMsg*);	
	int DoWrite(SDbQueryMsg*);
	void SetSmart(int dwSmart);
	void Dump();
	string GetAddr()
	{  //return m_strHost+"/"+m_strDb;
		return m_strConn;
	}



		
	virtual bool CheckDbConnect();
	
protected:
	virtual int  ExeculteCmd(SDbQueryMsg*);

private:
	string	GetActualBindName(string& strBind);
	
public:
	bool m_isMaster;
	bool m_isFree;
	int  m_nIndex;	//Angent标志
	int  m_nSubIndex;//子连接标志
	int  m_nConnectIndex;//分库标志
	int  m_nTbFactor;
	bool m_bIsConnected;
	EDBType m_eDBType;	
	string m_strConn;
	int    m_nConnectNum;
	CDbConnection* m_pParent;
	vector<CDbConnection*> m_pChilds;
	//性能优化相关
	int    m_dwSmart;
	int    m_dwPreparedSQLCount;
	transaction* m_pSmartTrans;
	int    m_dwLastCommitTime;
protected:
	session dbsession;
	CDbAgent* m_pAngent;
	string m_strPreSql;

private:
	void GetBindParams(string strSQL,vector<string>& vecParams);
	void DumpParamValues(vector<ptree>& vecParamValues,
			map<string,SBindDataDesc>& mapBindDataType);
	void DoPreSql();
private:
	int  m_PreviousConnected;
	boost::shared_ptr<CConnectControl> m_pConnectControl;
};

CDbConnection* MakeConnection();
CDbConnection* MakeConnection(const string & str);

#endif
