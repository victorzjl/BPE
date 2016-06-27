#ifndef _SESSION_H_
#define _SESSION_H_
#include "SocConfUnit.h"
#include "MacroFsm.h"
#include "ThreadTimer.h"
#include "ISession.h"
#include <map>
using namespace sdo::common;
using std::map;

typedef struct tagSapStartTime
{
	int seconds;
	int useconds;
}SSapStartTime;
typedef struct tagSequenceTime
{
	int nSequence;
	SSapStartTime timeSend;
}SSequenceTime;
class CSessionSoc:public ISession
{
public:
	CSessionSoc(int nId,SSocConfUnit * pConfUnit,const map<int,SSapCommandResponseTest *> &mapResponse);
	int Id()const{return m_nId;}
	SSocConfUnit *ConfUnit()const{return m_pConfUnit;}

	virtual void OnSessionTimeout();
	virtual void OnConnectSucc();
	virtual void OnReceiveMsg(void *pBuffer, int nLen);
	virtual void OnSendMsg(const SSapCommandTest * pCmd);
	
	virtual ~CSessionSoc(){}

    void DoTimeout();

/*event function*/
	void DoStartTimer(void *pData);
	void DoStopTimer(void *pData);
	void DoSendRequest(void *pData);
	void DoSaveResponse(void *pData);
	void DoDefault(void *pData);

	virtual void Dump()const;
private:
	void OnRequest_(void *pData, int nLen);
private:
	int m_nId;
	SSocConfUnit *m_pConfUnit;
	const map<int,SSapCommandResponseTest *> &m_mapResponse;
	CFSM<CSessionSoc> m_fsm;
	CThreadTimer m_timer;
	vector<SSapCommandTest>::iterator m_itr;
	vector<SSequenceTime> m_vecSequence;

	
	bool m_bAesTransfer;
	int m_nClientType;
	unsigned char m_szNonce[16];
	unsigned char m_szAesKey[16];
	
	/*
	map<int,SSapStartTime> m_mapStarttime;
	*/
};
#endif

