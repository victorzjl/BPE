#include "HttpClientImp.h"
#include "CohConnection.h"

unsigned int CHttpClientImp::s_dwTimeout = 0;

void CHttpClientImp::SetTimeout(unsigned int timeout)
{
    CCohConnection::SetTimeout(timeout);
    s_dwTimeout = timeout;
}

void CHttpClientImp::OnReceiveResponse(const string &strResponse)
{
    m_pCallBack->OnRemoteResponse(m_dwId, strResponse);
}
