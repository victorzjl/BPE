
#ifndef _DB_SERVICE_CONFIG_H_
#define _DB_SERVICE_CONFIG_H_

#include "dbconfigcommon.h"
#include "dbserviceconfigloghelper.h"
#include "tinyxml/ex_tinyxml.h"

#include "DbMsg.h"
using namespace sdo::util;

class CDBServiceConfig
{
public:
	static CDBServiceConfig* GetInstance();
	void LoadServiceConfig(const string& sPath); // load *.xml all

	void LoadDBConnConfig(const string& sPath);

	void GetAllServiceIds(vector<unsigned int>& vecServiceIds);
	vector<SConnDesc>* GetAllDBConnConfigs();
	int GetDBBigQueueNum();
	unsigned int GetDBDelayQueueNum();
	int GetDBThreadsNum();
	int GetBPEThreadsNum();
	SServiceDesc* GetServiceDesc(int nServiceId);
	map<string,SServiceType>* GetServiceTypeById(int nServiceId);
	SServiceType* GetSingleServiceType(int nServiceId,string& sTypeName);
	SMsgAttri* GetMsgAttriById(int nServiceId,int nMsgId);
	int GetDivdeDesc(int nServiceId,int nMsgId,SDivideDesc& divideDesc);

	SServiceType* GetParamTypeByBindName(int nServiceId,int nMsgId,string& sBindName);
	SServiceType* GetParamTypeByElementCode(int nServiceId,int nMsgId,int nElementCode);

	vector<SParam>* GetRequestParamsById(int nServiceId,int nMsgId);
	vector<SParam>* GetResponeParamsById(int nServiceId,int nMsgId);
	
	void Dump(); //dump all config

private:
	CDBServiceConfig();
	~CDBServiceConfig();

private:
	void ForSpeedUp();
	void SubLoadServiceConfig(const string& sPath,int& nLoadCount);
	int LoadXmlByFileName(const string& sFileName);
	int LoadServiceTypes(TiXmlElement* pType, SServiceType& sServiceType);
	int LoadServiceMessage(TiXmlElement* pMessage, SMsgAttri& sMsgAtrri);
	int LoadTlvServiceTypes(TiXmlElement* pType, SServiceType& sServiceType);
	
	int LoadRequestParam(TiXmlElement* pField, SParam& sParam);
	int LoadResponeParam(TiXmlElement* pField, SParam& sParam);
	int LoadSQLs(TiXmlElement* pSQL,SMsgAttri& sMsgAtrri);
	int LoadStructServiceTypes(TiXmlElement* pType, SServiceType& sServiceType);

	bool DecodePassword(const string&enc_text,string& out);
	bool EncodePassword(const string&clear_text,string& out);
	
	int PostProcessMsgAttri(map<int,SMsgAttri>& mapAttri,map<string,SServiceType>& mapType);
	void PostProcessServiceTypes(map<string,SServiceType>& mapType);
	void ProcessXMLCDATA(string& strData);

	/* db conn */
	int LoadXMLDBConnConfig(const string& sFileName);
	int LoadDBConnDesc(TiXmlElement* pSosList,SConnDesc& connDesc);
	int SubLoadDBConnDesc(TiXmlElement* pDBDesc,SConnItem& connItem);

	void DumpTypeInfo(SServiceType& typeInfo);
	void DumpMsgAttri(SMsgAttri& msgAttri);
	
private:
	map<int,SServiceDesc> mapServiceConfig;
	vector<SConnDesc> vecDBConnConfig;
	int m_nBPEThreadNum;
	int m_nDBThreadNum;
	int m_nDBBigQueueNum;
	unsigned int m_nDBDelayQueueNum;
	static CDBServiceConfig* sm_pInstance;


};


#endif
