#ifndef COMPOSE_SERVICE_H
#define COMPOSE_SERVICE_H
#include "AvenueMsgHandler.h"
#include "ConditionParse.h"
#include <string>
#include <vector>
#include <map>
#include "SapServiceInfo.h"

using namespace std;
using sdo::commonsdk::CAvenueServiceConfigs;
using sdo::commonsdk::EValueType;


typedef vector<int> ReturnValues;
typedef int SequeceIndex;
typedef int NodeIndex;
typedef string SequeceIndexStr;
typedef string NodeIndexStr;

typedef Parse CResultCondition;

struct SComposeIndex
{
    int     nIndex;
    string  strIndex;
    SComposeIndex() : nIndex(-1),
                      strIndex(""){}
};

enum EComposeValueType
{
	VTYPE_REQUEST,
	VTYPE_RESPONSE,
	VTYPE_XHEAD,            // the target is xhead
    VTYPE_DEF_VALUE,        // use for process parameter. the source is def value
    VTYPE_RETURN_CODE,      // the source is code
    VTYPE_NOCASE_VALUE,     // example $this.param=123. the source is no case value
    VTYPE_LASTCODE,         // the first error code which is wrong of the last node.
    VTYPE_REMOTEIP,         // the value is remote ip. 0.0.0.0
    VTYPE_DATA_REQUEST,     // request of data request
    VTYPE_DATA_RESPONSE,    // response of data request
    VTYPE_DATA_CODE,        // code of data request
	VTYPE_DATA_DEFVALUE,        // def value of data request
	VTYPE_FUNCTION,    //return result of function
	VTYPE_LUAFUNCTION, 
	VTYPE_LOOP_RETURN_CODE,        // the source is code of loop seq
	VTYPE_BUILTINFUNCTION,		   // built-in function
    VTYPE_UNKNOW,
};
struct SFunctionParam
{
	string              strSourceKey;
	EComposeValueType   enValueType;
	SComposeIndex       sNode;
	SComposeIndex       sSeq;

	SFunctionParam() : strSourceKey(""),enValueType(VTYPE_UNKNOW){}
};
struct SValueInCompose
{
    string              strTargetKey;
    string              strSourceKey;
    EComposeValueType   enValueType;
    EComposeValueType   enTargetType;
    SComposeIndex       sNode;
    SComposeIndex       sSeq;
	vector<SFunctionParam> sFunctionParams;
	string 				sFunctionRet;
	bool isLoop;

    SValueInCompose() : strTargetKey(""),
                        strSourceKey(""),
                        enValueType(VTYPE_UNKNOW),
                        enTargetType(VTYPE_REQUEST),
						isLoop(false){}
};

typedef struct stDefParameter
{
    EValueType  enType;
    bool        bHasDefaultValue;
    string      strDefaultValue;
    int         nDefaultValue;

    stDefParameter() : enType(sdo::commonsdk::MSG_FIELD_STRING),
                      bHasDefaultValue(false),
                      strDefaultValue(""),
                      nDefaultValue(0){}
}SDefParameter;

enum ESequenceType
{
	Sequence_Type_Request  = 0,
	Sequence_Type_Response = 1
};

enum EReturnType
{
	Return_Type_Sequence = 0,   // 返回指定Sequence错误号
	Return_Type_LastNode = 1,   // 返回上一节点错误号
	Return_Type_Code     = 2    // 返回指定错误号	
};

struct SReturnCode
{
	EReturnType     enReturnType;
	SComposeIndex   sNodeId;
	SComposeIndex   sSequenceId;
	int             nReturnCode;
    bool            bIsDefValue;
    string          strValueName;

    SReturnCode() : enReturnType(Return_Type_Code),
                    nReturnCode(-1),
                    bIsDefValue(false),
                    strValueName(""){}
};

struct SComposeSequence
{
	string strSequenceName;
    ESequenceType   enSequenceType;
    bool            bCheckServiceExist;
    bool            bAckMsg;
    string          strServiceName;       // Sequence_Type_Request时有效
    int             nTimeout;             // Sequence_Type_Request时有效
    SReturnCode     sReturnCode;          // Sequence_Type_Response时有效
    vector<SValueInCompose> vecField;
	SValueInCompose			sLoopTimes;
	SValueInCompose		sSpSosAddr;
};

struct SComposeGoto
{
    SComposeIndex           sTargetNode;
    CResultCondition        cCondition;
    vector<SValueInCompose> vecConditionParam;
};

struct SComposeNode
{
	string strNodeName;
    map<SequeceIndex, SComposeSequence> mapSeqs;
    vector<SComposeGoto>                vecResult;
	unsigned                            dwSeqSize;

    map<string, SequeceIndex>           mapSequenceIndex;
    vector<SValueInCompose>             vecResultValues;
};

struct SComposeServiceUnit
{
	SComposeKey                 sKey;
	string                      strServiceName;
	int                         nEndNode; //for error end
    map<NodeIndex, SComposeNode> mapNodes;

    map<string, NodeIndex>      mapNodeIndex;
    map<string, SDefParameter>  mapDefValue;
	
    bool                        isDataCompose;
    vector<unsigned>            vecDataServerId;

};

/*follow tow function : just for log*/
const char* getNodeName(const SComposeServiceUnit &stComposeServiceUnit, NodeIndex node_index_);
const char* getSequenceName(const SComposeServiceUnit &stComposeServiceUnit, NodeIndex node_index_, SequeceIndex seq_index_);

//////////////////////////////////////////////////////////////////////////

class TiXmlElement;


class CComposeServiceContainer
{
public:
	int LoadComposeServiceConfigs(const string &strComposeConfigPath, const string &strDataComposeConfigPath, CAvenueServiceConfigs &serviceConfig, const map<SComposeKey, vector<unsigned> >& mapDataServerId);
	const SComposeServiceUnit *FindComposeServiceUnit(const SComposeKey &sKey) const;

    void Dump();

    string OnGetConfigInfo();

private:
	int LoadConfig(const string &strConfigFile, const map<SComposeKey, vector<unsigned> >& mapDataServerId, bool isDataCompose = false);
	int LoadIncludeConfig(TiXmlElement *pCompseNode, TiXmlElement *pInclude, const string &strConfigFile);
	int CheckConfig(SComposeServiceUnit &stComposeServiceUnit);
	int ReadComposeNode(TiXmlElement *pNode, SComposeServiceUnit &stComposeServiceUnit);
	int ReadSequence(TiXmlElement *pNode, SComposeNode &stComposeNode);
    int ReadGoto(TiXmlElement *pNode, SComposeNode &stComposeNode);
	int ReadRequest(TiXmlElement *pNode, SComposeSequence &stComposeSequence);
	int ReadResponse(TiXmlElement *pNode, SComposeSequence &stComposeSequence);
	int ReadValueInCompose(TiXmlElement *pNode, SValueInCompose &stValueInCompose);
    int ReadValueInResult(TiXmlElement *pNode, SValueInCompose &stValueInCompose);
    int ReadDefValue(TiXmlElement *pNode, SComposeServiceUnit &stComposeServiceUnit);
	int ReadValueInFunction(SValueInCompose &stValueInCompose);

    int GenerateNodeId(TiXmlElement *pNode,SComposeServiceUnit &stComposeServiceUnit);
    int GenerateSequenceId(TiXmlElement *pNode,SComposeNode &stComposeNode);

    int GenerateId(SComposeServiceUnit &stComposeServiceUnit);

    int GenerateConditionParam(SComposeServiceUnit& stComposeServiceUnit);

    bool CheckDefValue(const string& def_value_);

private:
    const SComposeServiceUnit* m_current_unit;
	CAvenueServiceConfigs *m_pServiceConfig;
	map<SComposeKey, SComposeServiceUnit> m_mapComposeUnit;
};

#endif // COMPOSE_SERVICE_H
