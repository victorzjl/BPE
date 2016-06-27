//#include "CohLogHelper.h"
#include <stdio.h>
#include "BusinessCohClientImp.h"
#include "CohPub.h"
#include "HTDealerServiceLog.h"
#include "boost/thread.hpp"  
#include <boost/lexical_cast.hpp>//使用字符串转换功能 

using namespace boost::asio; 

/*初始化*/
void CBusinessCohClientImp::Init(int nId, string& strUrl, CMessageDealer* m_pMD)
{
    size_t npos = strUrl.find('/', sizeof("http://")+1);
	if(npos != string::npos)
	{
		strUrl.erase(npos);
	}
	/*设置id号*/
	m_nId = nId;
	//设置用于回调的指针
	m_Dealer = m_pMD;

	/*设置服务端地址*/
	ParseHttpUrl(strUrl, m_oServerUrl);
}


/*发送http包到http服务器端*/
void CBusinessCohClientImp::SendRequest(const string &strContent)
{
	HT_XLOG(XLOG_DEBUG,"CBusinessCohClientImp::%s\n", __FUNCTION__);
    DoSendRequest(m_oServerUrl.strIp, m_oServerUrl.dwPort, strContent);
	//m_Dealer->SetIPPORT(m_oServerUrl.strIp, m_oServerUrl.dwPort,m_nId);
}

/*接收数据*/
void CBusinessCohClientImp::OnReceiveResponse(const string &strResponse)
{
	HT_XLOG(XLOG_DEBUG,"CBusinessCohClientImp::%s [%s]\n",__FUNCTION__, strResponse.c_str());
	
	char strPort[25] = {0};
	sprintf(strPort,"%d",(int)m_oServerUrl.dwPort);
	
	m_Dealer->OnReceive(m_nId, strResponse,PROTOCOL_HTTP, m_oServerUrl.strIp, strPort);

}
