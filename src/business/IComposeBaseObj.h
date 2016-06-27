#ifndef __I_COMPOSE_BASE_OBJ__H_
#define __I_COMPOSE_BASE_OBJ__H_
#include <string>

using std::string;


class CSosSessionContainer;
class CComposeServiceObj;
class CSessionManager;
struct SComposeServiceUnit;
struct SReturnMsgData;

class IComposeBaseObj
{
public:
	IComposeBaseObj(const string & strModule,CSessionManager *pManager,int level);
	
	int ProcessComposeRequest(const void *pBuffer, int nLen, 
     const void *pExtHead, int nExt, 
     const string& sIp, int port, void* handle = NULL);	
    int ProcessDataComposeRequest(const void *pBuffer, int nLen, 
        const void *pExtHead, int nExt, 
        const string& sIp, int port);	
	virtual ~IComposeBaseObj();
public:		
	void OnDeleteComposeService(unsigned nComposeObjId);
	void OnAddComposeService(unsigned dwId, CComposeServiceObj* pCSObj);
	void Dump();
	virtual void WriteComposeData(void* handle, const void * pBuffer, int nLen)=0;
    void NotifyOffLine();

private:
	void FastResponse(const string &strAddr,const void *pBuffer, int nLen, int nCode, void* handle=NULL);
	void FastResponse(const string &strAddr,const void *pBuffer, int nLen, int nCode, void* handle, SReturnMsgData& returnMsg);
    void FastDataResponse(const string &strAddr, const SComposeKey& sComposeKey, const void *pBuffer, int nLen, int nCode);
    void RecordDataReq(const string& strIp, unsigned dwServiceId, unsigned dwMsgid, int nCode);
	
private:
	string m_strModule;
	CSessionManager *m_pManager;
	int m_level;
	unsigned int m_dwComposeIndex;	
	map<unsigned, CComposeServiceObj*> m_mapComposeObj;
};
#endif

