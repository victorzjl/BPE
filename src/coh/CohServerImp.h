#ifndef _COH_SERVER_IMP_H_
#define _COH_SERVER_IMP_H_
#include "CohStack.h"
#include "CohServer.h"
#include "CohRequestMsg.h"
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <string>

using std::string;
namespace sdo{
namespace coh{

class CohServerImp:public ICohServer
{
	
public:
	virtual void OnReceiveRequest(void * identifer,const string &request,const string &remote_ip,unsigned int remote_port);

private:
	void OnNotifyChanged(void * identifer);
	void OnNotifyLoadData(void * identifer);
	void OnUninstallSO(void * identifer,const string& strName);
	void OnInstallSO(void * identifer,const string& strName);
	void OnQuerySelfCheck(void * identifer);
	void QueryPluginVersion(void *identifer);
    void QueryConfigInfo(void *identifer);
    void QueryHelp(void *identifer);
};
}
}
#endif

