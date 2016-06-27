#ifndef _MESSAGE_RECEIVE_H_
#define _MESSAGE_RECEIVE_H_

//#include "MessageDealer.h"
//#include "HTMsg.h"
#include <string>
#include <vector>
#include <_time.h>
#include "MessageDealer.h"
//#include <boost/thread/mutex.hpp>
using std::string;
using std::vector;

//using namespace HT;
//using namespace sdo::common;

//class CMessageDealer;
class CHTDealer;

class CMessageReceive
{
public:
	//static CMessageReceive* GetInstance( ){ return m_pInstance; }
	static CMessageReceive* GetInstance();//8_13_17
	int Start( unsigned int dwThreadNum );
	void Stop();
	void OnProcess(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, 
					timeval_a tStart, void *pHandler, const void *pBuffer, int nLen, CHTDealer *htd);
	void GetSelfCheck( unsigned int &dwAliveNum, unsigned int &dwAllNum );	
	//void _registerSend(string name, PLUGIN_FUNC_SEND funcAddr);
	//void _registerRecv(string name, PLUGIN_FUNC_RECV funcAddr);
private:
	CMessageReceive();
	 ~CMessageReceive();
private:
	vector<CMessageDealer *> m_vecChildThread;
	static CMessageReceive* m_pInstance;
	//static boost::mutex m_mut;//8_13_17
	
	class CGarbo
    {
    public:
	   // boost::mutex m_mut_forGarbo;
        ~CGarbo()
        {  
            if (CMessageReceive::m_pInstance)
			{
				//boost::unique_lock<boost::mutex> lock(m_mut_forGarbo);
				//if (CMessageReceive::m_pInstance)
				delete CMessageReceive::m_pInstance;
				
			}
                
        }
    };
    static CGarbo Garbo;
	static boost::mutex m_mut;
	unsigned int rand;
	
	unsigned int bpeThreadNum;
	unsigned int selfCheckCount;
};

#endif
