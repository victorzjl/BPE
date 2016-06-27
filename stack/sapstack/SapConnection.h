#ifndef _SDO_SAP_CONNECTION_H_
#define _SDO_SAP_CONNECTION_H_
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "SmallObject.h"
#include "SapMessage.h"
#include <string>
#include "SapHandlerAlloc.h"
#include <boost/asio/deadline_timer.hpp>
#include "../common/detail/_time.h"
#include <boost/thread/mutex.hpp>
#include <deque>
#include "SapRequestHistory.h"
#include "SapAgent.h"

#include "SapBuffer.h"

using std::string;
using boost::asio::ip::tcp;
namespace sdo{
namespace sap{
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
class CSapAgent;
class CSapConnection:public sdo::common::CSmallObject,
	public boost::enable_shared_from_this<CSapConnection>,
	private boost::noncopyable
{
	typedef map<unsigned int, SSapRequestHistory> MHistory;
public:
	CSapConnection(CSapAgent *pAgent,boost::asio::io_service &io_service);
	~CSapConnection();
	static void SetInterval(int nHeartIntervl,int nHeartDisconnectInterval)
	{
		sm_nHeartInterval=nHeartIntervl;
		sm_nTimeoutInterval=nHeartDisconnectInterval;
	}
	void SetId(int nId){m_nId=nId;}
	int Id() const {return m_nId;}
	tcp::socket& Socket(){return m_socket;}
	void AsyncConnect(const string &strIp,unsigned int dwPort,int nTimeout,unsigned int dwLocalPort=0);
	void AsyncRead();
	void AsyncWrite(const void *pBuffer,int nLen,unsigned int dwTimeout=0);
	void Close();

	void DetectInterval();

	void SetRemoteAddr();
	const string & GetRemoteIp()const {return m_strRemoteIp;}
	unsigned int GetRemotePort()const {return m_dwRemotePort;}

	void SetLocalDigest(const string &strDigest);
	void SetPeerDigest(const string &strDigest);
	void SetEncKey(unsigned char arKey[16]);
	
private:
	void HandleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator,unsigned int dwLocalPort);
	void HandleConnected(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void HandleConnected(const boost::system::error_code& err);
	void HandleConnectTimeout(const boost::system::error_code& err);
	void HandleRead(const boost::system::error_code& err,std::size_t dwTransferred);
	void ReadPacketCompleted(const void *pBuffer, unsigned int nLen);
	void HandleWrite(const boost::system::error_code& err);
	void HandleStop();

	void WriteData(const void *pBuffer,int nLen,unsigned int dwTimeout=0);
	void DoDetectInterval();


	void ConnectFinish(int result);
	void ReadBusinessPacketCompleted(const void *pBuffer, unsigned int nLen);

	void dump_sap_packet_notice(const void *pBuffer,ESapDirectType type);
	void dump_sap_packet_info(const void *pBuffer,ESapDirectType type);

	void StopConnection();

	void AddReceiveRequestHistory(const void * pBuffer);
	void RemoveReceiveRequestHistory(const void * pBuffer);
	void ResponseFail(unsigned int dwServiceId,unsigned int dwMsgId,unsigned int dwSequence,unsigned int dwCode);
private:
	CSapAgent *m_pAgent;
	static int sm_nHeartInterval;
	static int sm_nTimeoutInterval;
	int m_nId;
	tcp::socket m_socket;
	tcp::resolver m_resolver;
	boost::asio::deadline_timer m_timer;

	unsigned char * m_pHeader;
	CSapBuffer buffer_;

	unsigned int m_heartSequence;
	CSapEncoder m_heartRequest;
	CSapEncoder m_heartResponse;

	bool bClientType;
	struct timeval_a m_HeartBeatPoint;
	struct timeval_a m_AlivePoint;
	
	CSapHandlerAllocator m_allocWrite;
	CSapHandlerAllocator m_allocReader;

	bool bRunning;
	SapLenMsgQueue m_queue;
	CSapMsgTimerList m_oRequestList;
	MHistory m_mapReceive;

	bool m_isConnected;

	string m_strRemoteIp;
	unsigned int m_dwRemotePort;

	bool m_isLocalDigest;
	string m_strLocalKey;

	bool m_isPeerDigest;
	string m_strPeerKey;

	bool m_isEnc;
	unsigned char m_arEncKey[16];
};
typedef boost::shared_ptr<CSapConnection> SapConnection_ptr;
}
}
#endif
