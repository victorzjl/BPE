#ifndef _DB_EXEC_H_
#define _DB_EXEC_H_

#include "DbConnection.h"
#include <vector>
#include <set>

using std::vector;
using std::set;

#include "SapTLVBody.h"


	
class CDBExec
{

typedef int (CDBExec::* FunResponeEncode)(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType);

public:
	CDBExec(CDbConnection* pConn);
	CDBExec(vector<CDbConnection*> vecConn);
	
	~CDBExec();
	
public:
	int ExecuteCmd(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& avEnc,string& sSQLLog,SLastErrorDesc& errDesc);

private:
	string FormatPartSql(const string & sql, SMsgAttri* pMsgAttri, map<string,SBindDataDesc>& mapParams);

	int Execute(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
		map<string,SBindDataDesc>& mapBindDataType,vector<ptree>& vecParamValues,string& sSQLlog,SLastErrorDesc& errDesc);

	int SubExecuteDyncUDSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
		vector<ptree>& vecParamValues,string& sSQLLog,SLastErrorDesc& errDesc);

	void GenerateDyncProcedureSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,vector<string>& vecOutSQLs,vector<string>& vecLogSQLs,vector<SSqlDyncData> &vecOutDyncs);
	
	void GenerateDyncSelectSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& strSQL, string&strLogSql,SSqlDyncData& rowData);
	
	void GenerateDyncUDSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
		vector<ptree>& vecParamValues,vector<string>& vecOutSQLs,vector<string>& vecLogSQLs,vector<SSqlDyncData>& vecOutDyncs
		);
	
	
	void GenerateUDSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,vector<string>& vecOutSQLs);

	void GenerateSelectSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& strSQL);

	void GenerateUDSQLLog(vector<string>& vecSQLs,map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog);

	int SubExecuteDyncSimpleSelectSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string strSQL,string& sSQLLog,
	SLastErrorDesc& errDesc);
	
	int SubExecuteSimpleSelectSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string strSQL,string& sSQLLog);

	int SubExecuteUDSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, 
		map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string& sSQLlog);

	int SubExecuteSP(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, 
		map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string& sSQLlog,SLastErrorDesc& errDesc);
	
	int BindValueToRequestParams(CAvenueSmoothDecoder& decoder,
									int nServiceId,
									int nMsgId,
									vector<ptree>& vecParamValues,
									map<string,SBindDataDesc>& mapBindDataType,
									SLastErrorDesc& errDesc
									);

	void BindValueToSPResponseParams(CAvenueSmoothDecoder& decoder,int nServiceId,int nMsgId,
									map<string,SBindDataDesc>& mapBindDataType);

	int BindValueToProcedureResponeParams(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
									map<string,SProcedureResBindData> &ResBinding,
									int nAffectedRows,SLastErrorDesc& errDesc);
	
	int BindValueToResponeParams(CAvenueSmoothEncoder& encoder,
								int nServiceId,
								int nMsgId,
								vector<DBRow>& fetchResult,
								int nAffectedRows,SLastErrorDesc& errDesc);

	void BindValueToPTree(SServiceType* pType,SParam* pParam,SAvenueValueNode& sNode,
				ptree& pt,map<string,SBindDataDesc>& mapBindType,bool isArrayElement=false);
	
	void BindValueToExecuteSQL(SBindDataDesc* pDesc,string& bindName,ptree& pt,string& sOutSQL);

	int BindErrorToResponeParams(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,SLastErrorDesc& errDesc);
	
	void GenerateExecuteSQL(int nServiceId,
							int nMsgId, 
							map<string,SBindDataDesc>& mapParams,
							vector<ptree>& vecParamValues,
							vector<string>& vecOutSQL);

	void GenerateComposeSQL(SComposeSQL& composeSQL,ptree& pt,string& sOutSQL);

	int ResponeProcessRowCount(CAvenueSmoothEncoder& encoder,int nAffectedRows,SServiceType* pParamType);

	int ResponeProcessResult(CAvenueSmoothEncoder& encoder,int nServiceId,
								vector<DBRow>& fetchResult,SServiceType* pParamType);


	int ResponeProcessResultField(CAvenueSmoothEncoder& encoder,int nServiceId,
								vector<DBRow>& fetchResult,SServiceType* pParamType,string sBindName);


	int ResponeProcessResultCell(CAvenueSmoothEncoder& encoder,int nServiceId,
								vector<DBRow>& fetchResult,SServiceType* pParamType,string sBindName);

	int ResponeEncodeTlv(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType);
	
	int ResponeEncodeTlvArrayElement(sdo::sap::CSapTLVBodyEncoder &retEnc,DBRow& row,SServiceType* pParamType);
		
	int ResponeEncodeInt(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType);
	int ResponeEncodeString(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType);
	int ResponeEncodeStruct(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType);

	void GetStructValues(SServiceType* pServiceType,void* pLoc,unsigned int nLen,
					vector<DBStructElement>& vecStructValues);

	void GetStructValues(SServiceType* pServiceType,SAvenueValueNode& sNode,
					vector<DBStructElement>& vecStructValues);

	int EncodeSingleIntValue(CAvenueSmoothEncoder& encoder,int nValue,SServiceType* pParamType);
	int EncodeSingleStrValue(CAvenueSmoothEncoder& encoder,string sValue,SServiceType* pParamType);

	void DumBindDataDesc(map<string,SBindDataDesc>& mapDesc);
	void DumpExeUDSQLs(vector<string>& vecSQLs);
	void DumpExeUDSQLs(vector<string>& vecSQLs,vector<SSqlDyncData>& vecDyncs);
	void GenerateUDSQLSimpleReplace(vector<string>& vecSQLs,map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog,vector<string>& vecOutSQLs);

	void MergeSelectParamValue(map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,ptree& pt);

	void RmVectorDuplicates(vector<string>& vec);

	int GetIntValueFromVariant(DBValue& vValue,int& nValue);


	int ExtendSQL(string &strSQL, string &strLogSql, 
		          map<string,SBindDataDesc> &mapParams, ptree &paramValue);

	

private:
	//CDbConnection* m_pConn;
	CDBServiceConfig* m_pServiceConfig;
	map<int,FunResponeEncode> m_mapFunResEncode;
	vector<CDbConnection*> m_vecConn;

	bool m_bIsMutiConn;
	EDBType m_eDBType;
};




#endif


