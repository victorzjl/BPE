#ifndef _COMPOSE_SERVICE_Obj_H_
#define _COMPOSE_SERVICE_Obj_H_
#include "ComposeService.h"
#include "../common/detail/_time.h"
#include "SapRecord.h"
#include "SapCommon.h"
#include "Utility.h"
#include "LogTrigger.h"
#include "SapTLVBody.h"
#include "ExtendHead.h"
#include "MetaData.h"
#include "SapLogHelper.h"
#include "AvenueTlvGroup.h"
namespace sdo{
namespace commonsdk{
    class CAvenueTlvGroupEncoder;	
	class CAvenueServiceConfigs;
	class CAvenueMsgHandler;
}
}
class CSessionManager;
class IComposeBaseObj;
class inst_value;

using sdo::commonsdk::CAvenueServiceConfigs;
using sdo::commonsdk::CAvenueTlvGroupEncoder;
using sdo::commonsdk::CAvenueMsgHandler;
using sdo::commonsdk::EValueType;
using sdo::commonsdk::SStructValue;

struct SReturnMsgData
{
	int nReturnMsgType;
	string strReturnMsg;
	SReturnMsgData():nReturnMsgType(0){}
};

struct SResponseRet
{
    int nCode;
    CAvenueMsgHandler *pHandler;

    SResponseRet(int nCode_=0, CAvenueMsgHandler *pHandler_=NULL):nCode(nCode_),pHandler(pHandler_){}
};

typedef struct stServiceLocation
{
	int nNode;
	int nSeq;	
	int nCode;
    string nodeName;
    string seqName;
    string srvAddr;
    string srvName;
    string reqParas;
    string rspParas;
    unsigned srvId;
    unsigned msgId;
}SServiceLocation;

typedef struct stSeqRet
{
	int nSeq;
	int nCode;
	stSeqRet(int nSeq_=0, int nCode_=0):nSeq(nSeq_),nCode(nCode_){}
}SSeqRet;

typedef struct stIPAddr
{
    string ip;
    unsigned port;
    stIPAddr(){}
    stIPAddr(const string& ip_, unsigned port_):ip(ip_),port(port_){}
    string toString()const
    {
        char szBuf[128] = {0};
        snprintf(szBuf, sizeof(szBuf), "%s,  %u", 
        ip.c_str(), port);
        return string(szBuf);
    }
}SIPAddr;


typedef struct stRequestMsg
{
	const void* pExtHead;
	int nExLen;
    SIPAddr remoteAddr;
	stRequestMsg(const void* pExHead_, int nExLen_, const SIPAddr& sIp)
		:pExtHead(pExHead_),nExLen(nExLen_),remoteAddr(sIp) {}
}SSRequestMsg;

typedef struct stNodeSeq
{
	int nNode;
	int nSeq;
	stNodeSeq(int nNode_=0,int nSeq_=0)
		:nNode(nNode_),nSeq(nSeq_)
		{}
	bool operator<(const stNodeSeq &rhs)const
	{
        return (nNode<rhs.nNode || ( !(rhs.nNode<nNode) && nSeq<rhs.nSeq));
	}
}SNodeSeq;

typedef enum
{
    E_VIRTUAL_IN = 0,
    E_VIRTUAL_OUT,
    E_COMPOSE_IN ,
    E_COMPOSE_OUT,
    E_AVS_IN,
    E_ALL
}EVirtualDirectType;

typedef struct stDefValue
{
    EComposeValueType enSourceType;
    int               nLastCode;
    string            strRemoteIp;
    EValueType        enType;
    vector<sdo::commonsdk::SStructValue> vecValue;
	vector<int> vecLoopRetCode;
	SMetaData      sMetaData;
    stDefValue() : enSourceType(VTYPE_UNKNOW), nLastCode(0), strRemoteIp("0.0.0.0"), enType(sdo::commonsdk::MSG_FIELD_STRING){}
	void LoadValueFromMetaField(stDefValue& def_value, const string& strField) 
	{		
		SValueData & value = def_value.sMetaData.GetField(strField);
		this->enType = value.enType;
		this->vecValue = value.vecValue;
	}
	void LoadValueFromMetaField(stDefValue& def_value, const string& strField, const string& strTargertField) 
	{		
		SValueData & value = def_value.sMetaData.GetField(strField);
		this->sMetaData.SetField(strTargertField,value);
		this->enType = sdo::commonsdk::MSG_FIELD_META;
	}
	void SaveValueToMetaField(stDefValue& def_value, const string& strField) 
	{		
		vector<sdo::commonsdk::SStructValue> vecValue = def_value.vecValue;
		if (def_value.enType == sdo::commonsdk::MSG_FIELD_INT)
		{
			if (def_value.enSourceType == VTYPE_LASTCODE || def_value.vecValue.size()==0)
			{
				sdo::commonsdk::SStructValue  ssv;
				ssv.pValue = (void*)&def_value.nLastCode;
				ssv.nLen = 4;
				vecValue.push_back(ssv);
			}
		}
		SValueData value(def_value.enType,vecValue);
		SS_XLOG(XLOG_DEBUG,"stDefValue::%s, line[%d] %d\n",__FUNCTION__,__LINE__, def_value.enSourceType);
		this->sMetaData.SetField(strField,value);
		SS_XLOG(XLOG_DEBUG,"stDefValue::%s, line[%d]\n",__FUNCTION__,__LINE__);
		this->enType = sdo::commonsdk::MSG_FIELD_META;
	}
}SDefValue;


typedef map<SNodeSeq, SResponseRet> RspContainer;
typedef RspContainer::iterator RCIterator;

class CComposeServiceObj
{
typedef map<unsigned int, SServiceLocation> SrvLocContainer;
typedef map<NodeIndex, SComposeNode>::const_iterator NodeIterator;
typedef map<SequeceIndex, SComposeSequence>::const_iterator SeqIterator;

public:	
	explicit CComposeServiceObj(IComposeBaseObj *pCBObj,const SComposeServiceUnit &rCSUnit,
		CAvenueServiceConfigs *pConfig,CSessionManager* pManager,unsigned dwSeq);
	
	explicit CComposeServiceObj(const SComposeServiceUnit &rCSUnit,unsigned dwSeq,CComposeServiceObj *pParentNode);
	virtual ~CComposeServiceObj();

public:
	void OnReceiveResponse(const void* pBuffer, int nLen, bool bToParent, const timeval_a& timeStart);
	virtual int SaveRequestMsg(const void* pBuffer, int nLen, SReturnMsgData& returnMsg);
	void SaveExHead(const SSRequestMsg &sReqMsg, const void* pBuffer);
	void NotifyOffLine() {m_pBase=NULL;}
	int Init(unsigned dwIndex, int level, const SSRequestMsg &sReqMsg,const void * pBuffer, int nLen, void* handle,
	SReturnMsgData& returnMsg);

	
	void SetErrorInfo(const string& strInfo){m_subErrInfo = strInfo;}
	
	void Dump();
public:
	CComposeServiceObj* GetParentObejct(){return m_pParent;}
	string GetServiceMsgName(){return m_oCSUnit.strServiceName; }
	string GetLevelLog(bool isLeaf);
protected:
	static int DecodeMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler);
	static int DecodeMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler, SReturnMsgData& returnMsg);
	//this function is called recursively
	virtual void DoStartNodeService(int nNodeIndex, int nLastCode=0);
	int RequestSubCompose(const void * pBuffer, int nLen, const SServiceLocation& sLoc, SReturnMsgData& returnMsg);
	bool DoSaveResponseRet(int nNode, int nSeq,int nCode, const SComposeNode& rNode);
    virtual void DoSaveResponseDef(const SComposeNode& rNode);
    virtual void DoSaveResponseXhead(const SValueInCompose& value_in_compose);
	
	int DoGetCode(const SReturnCode& rCode, int nLastCode)const;
	virtual bool IsRuleMatch(const SComposeGoto& goto_node_)const;
	
	string GetFunctionFullSourcekey(int type, const string& strSourcekey);
	int FindGotoNode(const vector<SComposeGoto> &vecRet)const;
	void SendToClient(int nCode,const void *pBuffer, int nLen)const;
	void SendToParent(int nCode,const void *pBuffer, int nLen)const;
	bool DoIsDestroy(int nCurrNode)const;

	void DoSaveCode(unsigned int dwSeq,int nCode);
	virtual int BuildPacketBody(const string& strName, const vector<SValueInCompose>& vecAttri,CAvenueMsgHandler &oBodyEncode,bool bIsRequest, string &reqParams,int seqLoopIndex=-1);	
    int BuildDataPacketBody(CSapEncoder& sapEncoder, const void* rspBuffer, unsigned rspLen, int nResult, const map<string, vector<string> >& defValues);
	
	int InitSub(const void *pBuffer, int nLen, SReturnMsgData& returnMsg);	
	virtual void RecordComposeService(int nCode, const string& strErrInfo)const;
    void RecordSequence(const SServiceLocation& rLoc, const timeval_a& timeStart, bool isLeaf=true);
    void RecordSyncRequest(const SServiceLocation& sParas, int nCode);
    void BuildField(CAvenueMsgHandler *pGet, CAvenueMsgHandler &msgSet, const SValueInCompose& sValue,int seqLoopIndex=-1);
    void BuildDefField(const map<string, SDefParameter>& def_parameter, map<string, SDefValue>& def_values, CAvenueMsgHandler &msgSet, const SValueInCompose& sValue,int seqLoopIndex=-1);
    void AddOrUpdateMsg(RspContainer& mapResponse, const SNodeSeq& sNodeSeq, const SResponseRet& sRsp);

    void GetAllDefValue(map<string, vector<string> >& mapDefValue);
	const int* GetNodeCode(const vector<SSeqRet>& vecSeqReturn, int nSeq) const;
	void BuildSequenceLoopTimes(const map<SequeceIndex, SComposeSequence>& mapSeqs);
	void BuildSequenceSpSosAddr(const map<SequeceIndex, SComposeSequence>& mapSeqs);
	inst_value GetFunctionParamValue(const SFunctionParam& sFunctionParam);
	void BuildFunctionLogRequest(vector<inst_value> &stk, SServiceLocation &location);
	void BuildFunctionLogResponse(vector<inst_value> &stk, SServiceLocation &location);
	void BuildStateMachineFunctionLogRequest(const string &strMachineName, int nState, int nEvent, SServiceLocation &location);
	void BuildStateMachineFunctionLogResponse(int nResult, SServiceLocation &location);
	void BuildStateMachineFunctionLogResponse(string strName, SServiceLocation &location);
	void HandleAfterAssign(const string& strTargetKey, SDefValue& def_value); 
	bool IsMetaKey(const string &strTargetKey) const;

protected:
	enum{DEFAULT_RESPONSE_CODE=-1, 
		SYNC_VSERVICE_BEG = 40000, SYNC_VSERVICE_END = 41000,
		ASYNC_VSERVICE_BEG = 41001, ASYNC_VSERVICE_END = 50000,
		SEQUENCE_MAX = 256,
		OLDSERVICE_ERRORCODE_RANGE = 1000,Compose_Node_End = -1};
	
	CComposeServiceObj *m_pParent;	
	int m_nCsosLog;
	IComposeBaseObj *m_pBase;	
	const SComposeServiceUnit &m_oCSUnit;
	CAvenueServiceConfigs *m_pConfig;
	CSessionManager *m_pManager;

	unsigned int m_dwSrcSeq;//sequence of the request for compose service
	unsigned int m_dwServiceSeq;
	SrvLocContainer m_mapService;
	RspContainer m_mapResponse;
    
	CAvenueMsgHandler *m_pReqHandler;
    void* m_pReqBuffer;
    unsigned m_dwBufferLen;
	vector<SSeqRet> m_vecNodeRet;
	map<int,vector<int> > m_mapNodeLoopRet;
	map<SequeceIndex,int> m_mapSeqLoopTimes;   //loopTimes of the sequence
	unsigned int m_nTotalSeqSize;			//total loopTimes of the sequence in the node
	map<SequeceIndex,string> m_mapSeqSpSosAddr;   //SpSosAddr of the sequence
    int m_last_code;

    map<string, SDefValue> _def_values;

	unsigned int m_dwIndex;
	SServiceLocation m_serviceRet;// for record log
	//set<int> m_nodeTrail;
	
    string m_strRemoteIp;
	string m_subErrInfo;
	int m_level;
	int m_rootMsgId;
	int m_rootServId;
	unsigned m_rootSeq;
	SLogInfo m_logInfo;
	struct timeval_a m_tmStart;
    string m_errInfo;
	string m_strRunPath;
    EXHeadHandler_ptr m_pExHead;
	void* m_handle;
};


#endif


