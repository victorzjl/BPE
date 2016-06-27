#ifndef _SDO_SAP_CONNECTION_H_
#define _SDO_SAP_CONNECTION_H_
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "SapMessage.h"
#include <string>
#include "SapHandlerAlloc.h"
#include <boost/asio/deadline_timer.hpp>
#include "../common/detail/_time.h"
#include <boost/thread/mutex.hpp>
#include <map>
#include "ISapSession.h"
#include "SapBuffer.h"
#include "ThreadTimer.h"

#define MAX_QUEUE_FULL 1024*64
using namespace sdo::common;

using std::map;
using std::string;
using boost::asio::ip::tcp;
typedef enum
{
    E_SAP_IN=1,
    E_SAP_OUT=2
}ESapDirectType;

class CSapConnection:
	public boost::enable_shared_from_this<CSapConnection>,
	private boost::noncopyable
{
public:
	CSapConnection(boost::asio::io_service &io_service,void *pManager);
	void * GetManager()const{return m_pManager;}
	
	void SetOwner(ISapSession * pSession){m_pOwner=pSession;}
	
	~CSapConnection();
	static void SetInterval(int nHeartIntervl,int nHeartDisconnectInterval)
	{
		sm_nHeartInterval=nHeartIntervl;
		sm_nTimeoutInterval=nHeartDisconnectInterval;
	}
	tcp::socket& Socket(){return m_socket;}
	void AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort=0);
	void AsyncRead();
	
	
	void Close();

	
	void WriteData(const void *pBuffer,int nLen, const void* pExHead, int nExLen,unsigned int dwTimeout=0);
	void WriteData(const void *pBuffer,int nLen,unsigned int dwTimeout=0);
	void OnTimer();

	void SetRemoteAddr();
	const string & GetRemoteIp()const {return m_strRemoteIp;}
	unsigned int GetRemotePort()const {return m_dwRemotePort;}

	void SetLocalDigest(const string &strDigest);
	void SetPeerDigest(const string &strDigest);
	void SetEncKey(unsigned char arKey[16]);

	void Dump();
	
private:
	void ASyncReadInner();
	void WriteDataInner(const void *pBuffer,int nLen,unsigned int dwTimeout=0);
	
	void CloseInner();
	
	void HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator,unsigned int dwLocalPort);
	void HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void ConnectFinish(int result);
	
	void HandleRead(const boost::system::error_code& err,std::size_t dwTransferred);
	void ReadPacketCompleted(const void *pBuffer, unsigned int nLen);
	void HandleWrite(const boost::system::error_code& err);
	void dump_sap_packet_notice(const void *pBuffer,ESapDirectType type);
	void dump_sap_packet_info(const void *pBuffer,ESapDirectType type);

	void StopConnection();


	void ResponseFail(unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwSequence,unsigned int dwCode);
private:
	static int sm_nHeartInterval;
	static int sm_nTimeoutInterval;

	tcp::socket m_socket;
	tcp::resolver m_resolver;

	unsigned char * m_pHeader;
	CSapBuffer buffer_;

	unsigned int m_heartSequence;
	SSapMsgHeader m_heartRequest;
	SSapMsgHeader m_heartResponse;

	bool bClientType;
	struct timeval_a m_AlivePoint;

	CThreadTimer m_tmHeartBeat;
	
	
	CSapHandlerAllocator m_allocWrite;
	CSapHandlerAllocator m_allocReader;

	bool bRunning;
	SapLenMsgQueue m_queue;
	int m_nQueueLen;

	string m_strRemoteIp;
	unsigned int m_dwRemotePort;

	void *m_pManager;

	ISapSession * m_pOwner;


	bool m_isLocalDigest;
	string m_strLocalKey;

	bool m_isPeerDigest;
	string m_strPeerKey;

	bool m_isEnc;
	unsigned char m_arEncKey[16];
};
typedef boost::shared_ptr<CSapConnection> SapConnection_ptr;


#endif
