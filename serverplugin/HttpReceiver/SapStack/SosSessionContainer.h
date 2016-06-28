#ifndef _SOS_SESSION_CONTAINER_H_
#define _SOS_SESSION_CONTAINER_H_ 

#include "SosSession.h"
#include "SapConnection.h"
#include "SessionManager.h"
#include "HpsCommonInner.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

class CHpsSessionManager;
class CSosSession;


typedef struct stSosList
{
	int nPri;
	vector<CSosSession *> vecSessions;
	vector<CSosSession *>::iterator itrNow;

	stSosList():nPri(0){}
	stSosList(int nPri_, vector<CSosSession *> vecSessions_):nPri(nPri_),vecSessions(vecSessions_){itrNow=vecSessions.begin();}
}SSosList;



class CSosSessionContainer
{
public:
	CSosSessionContainer(unsigned int dwIndex, CHpsSessionManager *pManager, vector<unsigned int> &vecServiceId);
	~CSosSessionContainer();

	void LoadSosList(const vector<string> & vecAddr, CHpsSessionManager *pManager);

	void OnConnected(unsigned int dwSosSessionId, unsigned int dwConnId);
	
	int DoSendMessage(const void * pBuffer,int nLen,const void * exHeader,int nExLen, int nInSosPri, int *pOutSosPri, const char* pSignatureKey = NULL, unsigned int* pSosId = NULL);

	void OnReceiveAvenueRequest(unsigned int dwSosSessionId, unsigned int dwConnId,const void *pBuffer, int nLen,const string &strRemoteIp,unsigned int dwRemotePort);
	int DoSendAvenueResponse(unsigned int dwSosSessionId, unsigned int dwConnId,const void *pBuffer, int nLen,const void * exHeader=NULL,int nExLen=0);

	void GetSosStatusData(SSosStatusData &sData);
	unsigned int GetContainerId(){return m_dwContainerId;}

	void GetAddrs(vector<string> &vecAddr){vecAddr = m_vecSosAddr;}

	int GetMapPriSosList(map<int, string > &mapAddrsByPri);
	
	void Dump();
	
public:
	void SetUsableStatus(bool bStatus){m_bIsUsable = bStatus;}
	bool GetUsableStatus(){return m_bIsUsable;}
	void SetServiceIds(const vector<unsigned int> &vecId){m_vecServiceId = vecId;}
	void GetServiceIds(vector<unsigned int> &vecId){vecId = m_vecServiceId;}
	
	bool ContainSosAddr(const vector<string> &vecSosAddr);
private:	
	unsigned int m_dwContainerId;	
	unsigned int m_dwSosSessionCount;

	CHpsSessionManager *m_pSessionManager;

	vector<unsigned int> m_vecServiceId;
	vector<string> m_vecSosAddr;

	map<int, SSosList> m_mapPriSosList;
	map<unsigned int, CSosSession *> m_mapSosSessionBySessionId;

	unsigned int m_dwSlsRegeisterSeq;
	bool m_bIsUsable;
};

#endif

