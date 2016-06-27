#ifndef __COH_HTTPS_STACK_H_
#define __COH_HTTPS_STACK_H_
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class CCohHttpsStack
{
	public:
		static void Start();
		static void Stop();
		static boost::asio::io_service & GetIoService(){return sm_oIoService;}
        static boost::asio::io_service & GetContext(){return sm_oIoService;}
	private:
		static boost::asio::io_service sm_oIoService;
		static boost::asio::io_service::work *sm_oWork;
		static boost::thread m_thread;
};

#endif

