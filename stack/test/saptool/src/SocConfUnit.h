#ifndef _SOC_CONF_UNITS_H_
#define _SOC_CONF_UNITS_H_
#include <vector>
#include <string>
#include "SapCommon.h"
using std::string;
using std::vector;
using namespace sdo::sap;
typedef enum
{
	E_EXECUTE_ONCE=1,
	E_EXECUTE_CIRCLE=2,
	E_EXECUTE_CONNECTION_CIRCLE=3
}EExecuteType;

typedef enum
{
	E_ATTRI_VALUE_INT=1,
	E_ATTRI_VALUE_STR=2,
	E_ATTRI_VALUE_STRUCT=3
}EAttriValueType;

typedef struct stSapAttributeTest
{
	int m_nAttriId;
	EAttriValueType m_nType;
	int m_nValue;
	string m_strValue;
	vector<string> vecStruct;
	stSapAttributeTest():m_nAttriId(1),m_nType(E_ATTRI_VALUE_INT),m_nValue(0){}
}SSapAttributeTest;

typedef struct stSapCommandTest
{
	unsigned int m_nServiceId;
	unsigned int m_nMsgId;
	int m_nInterval;
	int m_nTimes;
	vector<SSapAttributeTest> m_vecAttri;
	stSapCommandTest():m_nServiceId(1),m_nMsgId(1),m_nInterval(0),m_nTimes(1){}
}SSapCommandTest;

typedef struct stSocConfUnit
{
	int m_nTimes;
	int m_nPort;
	EExecuteType m_type;
	string m_strPass;
	string m_strKey;
	string m_strSocName;
	int m_nAppId;
	int m_nAreaId;
	vector<SSapCommandTest> m_vecCommands;
	stSocConfUnit():m_nTimes(0),m_nPort(-1),m_type(E_EXECUTE_ONCE){}
}SSocConfUnit;
typedef struct stSapCommandResponseTest
{
	unsigned int m_nServiceId;
	unsigned int m_nMsgId;
	unsigned int m_byCode;
	vector<SSapAttributeTest> m_vecAttri;
	stSapCommandResponseTest():m_byCode(0){}
}SSapCommandResponseTest;

class CSapInterpreter
{
public:
	static int GetCommandId(const string &strCommand);
	static int GetAttriId(const string &strCommand);
	static EAttriValueType GetAttriType(const string &strAttri);
	static int GetTypeValue(const string &strType);

	static string GetCommandName(int index);

	
private:
	const static string m_astrCommand[SAP_MSG_ALL];
	const static string m_astrAttri[SAP_Attri_ALL];
	const static EAttriValueType m_astrAttriType[SAP_Attri_ALL];
};
#endif

