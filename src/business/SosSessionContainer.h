#ifndef _SOS_SESSION_CONTAINER_H_
#define _SOS_SESSION_CONTAINER_H_
#include "SosSession.h"
#include <boost/unordered_map.hpp>

class CSessionManager;
class CSosSession;
class CSosSessionContainer
{
public:
	CSosSessionContainer(const vector<unsigned int> &vecServiceId);
	void ReloadSos(const vector<unsigned int> vecServiceId,const vector<string> & strAddr, CSessionManager *pManager,int & nIndex);
	void AddSos(const string &strAddr,CSosSession *pSos, int &nIndex);
	void GetAllSos(boost::unordered_map<string,CSosSession *> &mapSos);
	CSosSession * GetSos();
	void ReportSocOnline(const string &strName,const string &strAddr,unsigned int dwType,const string & strVersion, const string & strBuildTime);
	void ReportSocDownline(const string &strName,const string &strAddr,unsigned int dwType);
	void OnStopComposeService();
	void DoSelfCheck( vector<SSosStatusData> &vecDeadSos, int & nDeadSos,int &nAliveSos);
	void Dump();
	~CSosSessionContainer();
private:
	boost::unordered_map<string,CSosSession *> m_mapSession;
	boost::unordered_map<string,CSosSession *>::iterator m_itr;
	int m_nSos;
	vector<unsigned int> m_vecServiceId;
	const static unsigned MAX_ERRORS = 10;
};
#endif
