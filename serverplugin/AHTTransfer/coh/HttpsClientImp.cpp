#include "CohClient.h"
#include "HttpsConnection.h"
#include "HttpsClientImp.h"
#include "CohHttpsStack.h"
#include "stdio.h"
#include "HTDealerServiceLog.h"

using namespace HT_COH;
using namespace boost::asio; 

void CHttpsClientImp::SetTimeout(int nTimeout)
{
    CHttpsConnection::SetTimeout(nTimeout);
}

void CHttpsClientImp::Init(int nId, string& strUrl,CMessageDealer* m_pMD)
{
    HT_XLOG(XLOG_DEBUG,"CHttpsClientImp::%s url[%s]\n", __FUNCTION__, strUrl.c_str());   
	m_nId = nId;    
    size_t npos = strUrl.find('/', sizeof("https://")+1);
	if(npos != string::npos)
	{
		strUrl.erase(npos);
	}
    
    char szHost[2048]={0};
    char szPath[2048]={'/','\0'};
    if (sscanf (strUrl.c_str(), "%*[HTTPShttps]://%1000[^:/]/%1000s", szHost, szPath)!=2)
    {
		sscanf (strUrl.c_str(), "%*[HTTPShttps]://%1000[^:/]", szHost);
    }
    m_oServerUrl.strIp   = szHost;
    m_oServerUrl.strPath = szPath;
	
	//设置用于回调的指针
	m_Dealer = m_pMD;
}

void CHttpsClientImp::SendRequest(const string &strContent)
{
	HT_XLOG(XLOG_DEBUG,"CHttpsClientImp::%s content[%s]\n", __FUNCTION__, strContent.c_str());   
    boost::asio::ssl::context ctx(CCohHttpsStack::GetIoService(), boost::asio::ssl::context::sslv23);
    ctx.set_verify_mode(boost::asio::ssl::context::verify_none);  
    HttpsConn_ptr new_connection(new CHttpsConnection(this, CCohHttpsStack::GetIoService(), ctx));
    new_connection->StartSendRequest(m_oServerUrl.strIp, strContent);
}


void CHttpsClientImp::OnReceiveResponse(const string &strResponse)
{
	HT_XLOG(XLOG_DEBUG,"CHttpsClientImp::%s\n",__FUNCTION__);
	char strPort[25] = {0};
	
	sprintf(strPort,"%d",m_oServerUrl.dwPort);
	m_Dealer->OnReceive(m_nId, strResponse, PROTOCOL_HTTPS, m_oServerUrl.strIp,strPort);

}

