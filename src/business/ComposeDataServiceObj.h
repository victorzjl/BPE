#ifndef __COMPOSE_DATA_SERVICE_OBJ_H__
#define __COMPOSE_DATA_SERVICE_OBJ_H__
#include "ComposeServiceObj.h"

typedef struct stDataMsgRequest
{
    SSapMsgHeader*  pMsgHead;
    int             nResultCode;
    SComposeKey     sComposeKey;
    SSapValueNode   sRequestData;
    SSapValueNode   sResponseData;
    vector<SSapValueNode> vecDefValue;
}SDataMsgRequest;

class CComposeDataServiceObj : public CComposeServiceObj
{
typedef map<unsigned int, SServiceLocation> SrvLocContainer;
typedef map<NodeIndex, SComposeNode>::const_iterator NodeIterator;
typedef map<SequeceIndex, SComposeSequence>::const_iterator SeqIterator;

public:	
    explicit CComposeDataServiceObj(IComposeBaseObj *pCBObj,const SComposeServiceUnit &rCSUnit,
        CAvenueServiceConfigs *pConfig,CSessionManager* pManager,unsigned dwSeq);
	~CComposeDataServiceObj();

public:
    int Init(unsigned dwIndex, int level, const SSRequestMsg &sReqMsg, const SDataMsgRequest& sDataMsgRequest);

protected:
    static int DecodeDataMsgBody(const void *pBuffer, int nLen,CAvenueMsgHandler* pMsgHandler);
    virtual void DoStartNodeService(int nNodeIndex, int nLastCode=0);
    virtual int SaveRequestMsg(const SDataMsgRequest& sDataMsgRequest);
    virtual int BuildPacketBody(const string& strName, const vector<SValueInCompose>& vecAttri,CAvenueMsgHandler& MsgEncode,bool bIsRequest, string &reqParams);	
    virtual int DecodeDefValueMsg(const vector<SSapValueNode>& vecDefValue);
    virtual void RecordComposeService(int nCode)const;
    virtual bool IsRuleMatch(const SComposeGoto& goto_node_)const;
    virtual void DoSaveResponseDef(const SComposeNode& rNode);
    virtual void DoSaveResponseXhead(const SValueInCompose& value_in_compose);

protected:
    CAvenueMsgHandler *m_pRspHandler;
    map<string, vector<string> > m_reqDefValue;
    int m_reqResultCode;
};


#endif


