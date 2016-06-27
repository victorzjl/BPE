#ifndef _SAP_STACK_THREAD_H_
#define _SAP_STACK_THREAD_H_
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using std::map;
namespace sdo{
namespace sap{
class CSapStackThread
{
public:
	CSapStackThread();
	~CSapStackThread();
	void Start();
	void Stop();
	boost::asio::io_service & GetIoService(){return m_oIoService;}
	
private:
	boost::asio::io_service m_oIoService;
	boost::asio::io_service::work *m_oWork;
	boost::thread m_thread;
};
}
}
#endif
