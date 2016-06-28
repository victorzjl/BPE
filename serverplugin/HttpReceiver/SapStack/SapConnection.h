#ifndef _SAP_CONNECTION_H_
#define _SAP_CONNECTION_H_

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <string>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread/mutex.hpp>
#include <deque>

#include "ThreadTimer.h"
#include "SapHandlerAlloc.h"
#include "SmallObject.h"
#include "SapMessage.h"
#include "SapBuffer.h"
#include "HpsCommonInner.h"
#include "ThreadTimer.h"
#include "ISapSession.h"

using namespace sdo::common;
using namespace sdo::hps;
using namespace sdo::sap;

using std::string;
using boost::asio::ip::tcp;

class CSosSession;
#define MAX_QUEUE_FULL 1024*64

typedef enum
{
    E_SAP_IN=1,
    E_SAP_OUT=2
}ESapDirectType;

typedef struct stSapLenMsg
{
	int nLen;
	const void *pBuffer;
}SSapLenMsg;

typedef std::deque<SSapLenMsg> SapLenMsgQueue;

class CSapConnection:public sdo::common::CSmallObject,
	public boost::enable_shared_from_this<CSapConnection>,	
	private boost::noncopyable
{
public:
	CSapConnection(boost::asio::io_service &io_service, CTimerManager *pManager,ISapSession *pSession);
	virtual ~CSapConnection();

	void SetOwner(ISapSession *pSession){m_pSession = pSession;}
	unsigned int Id() const {return m_nId;}
	tcp::socket& Socket(){return m_socket;}
	
	void AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort=0);
	void AsyncRead();
	void AsyncReadInner();
	void WriteData(const void *pBuffer,int nLen,const char* pSignatureKey=NULL,unsigned int dwTimeout=0);
	void WriteDataEx(const void *pBuffer,int nLen, const void* pExHead, int nExLen,const char* pSignatureKey=NULL,unsigned int dwTimeout=0);
	void Close();
	
	void DoDetectInterval();

public:	
	void SetRemoteAddr();	
	void SetLocalDigest(const string &strDigest);
	void SetPeerDigest(const string &strDigest);
	void SetEncKey(unsigned char arKey[16]);
	
private:
	void HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator,unsigned int dwLocalPort);
	void HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleConnectTimeout(const boost::system::error_code& err);
	void HandleRead(const boost::system::error_code& err,std::size_t dwTransferred);
	void ReadPacketCompleted(const void *pBuffer, unsigned int nLen);
	void HandleWrite(const boost::system::error_code& err);
	void HandleStop();
	
	void WriteDataInner(const void *pBuffer,int nLen,unsigned int dwTimeout=0);
	
	void ConnectFinish(int result);
	void ReadAvenueResponsePacketCompleted(const void *pBuffer, unsigned int nLen);
	void ReadAvenueRequestPacketCompleted(const void *pBuffer, unsigned int nLen);
	
	void dump_sap_packet_notice(const void *pBuffer,ESapDirectType type);
	void dump_sap_packet_info(const void *pBuffer,ESapDirectType type);
	
	void StopConnection();

private:
	unsigned int m_nId;
	static unsigned int sm_nMark;
	static boost::mutex sm_mutCount;

	tcp::socket m_socket;
	tcp::resolver m_resolver;

	bool m_isConnected;
	bool bRunning;

	unsigned char * m_pHeader;
	CSapBuffer buffer_;
	
	unsigned int m_heartSequence;
	CSapEncoder m_heartRequest;
	
	struct timeval_a m_HeartBeatPoint;
	struct timeval_a m_AlivePoint;
	
	CSapHandlerAllocator m_allocWrite;
	CSapHandlerAllocator m_allocReader;	

	SapLenMsgQueue m_queue;
	int m_nQueueLen;

	string m_strRemoteIp;
	unsigned int m_dwRemotePort;

	bool m_isLocalDigest;
	string m_strLocalKey;

	bool m_isPeerDigest;
	string m_strPeerKey;

	bool m_isEnc;
	unsigned char m_arEncKey[16];

	CTimerManager *m_pTimerManager;
	ISapSession *m_pSession;

	CThreadTimer m_tmHeartBeat;
};
typedef boost::shared_ptr<CSapConnection> SapConnection_ptr;

#endif


