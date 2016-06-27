#ifndef _SDO_SAP_CONN_MANAGER_H_
#define _SDO_SAP_CONN_MANAGER_H_
#include "SapConnection.h"
#include <boost/thread/mutex.hpp>
#include "SapHandlerAlloc.h"
#include <boost/asio/deadline_timer.hpp>

#include <map>
using std::map;
namespace sdo{
namespace sap{
class CSapConnManager
{
public:
	static CSapConnManager * Instance();
	void Start();
	void Stop();
	
	void AddConnection(SapConnection_ptr ptrConn);
	SapConnection_ptr GetConnection(int nId);
	void DelConnection(SapConnection_ptr ptrConn);
	void DelConnection(int nId);

	void HandleTimeoutInterval();
private:
	CSapConnManager();
	~CSapConnManager();
	static CSapConnManager * sm_pInstance;
	static int sm_nId; 
	
	map<int,SapConnection_ptr> m_mapConns;
	boost::asio::deadline_timer m_tmTimeoOut;
	CSapHandlerAllocator m_allocTimeOut;


	boost::mutex m_mut;
	

	bool bRunning;
};
}
}
#endif
