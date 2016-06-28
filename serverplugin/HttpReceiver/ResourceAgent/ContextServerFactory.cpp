#include "ContextServerFactory.h"
#include "HttpRecConfig.h"
#include "ResourceAgentLogHelper.h"

CContextServerFactory *CContextServerFactory::s_pInstance = NULL;

CContextServerFactory *CContextServerFactory::GetInstance()
{
    if (NULL == s_pInstance)
    {
        s_pInstance = new CContextServerFactory;
    }

    return s_pInstance;
}


CContextServerFactory::CContextServerFactory()
    :m_pDefaultContextServer(new CContextServer)
{
}

void CContextServerFactory::Load()
{
    RA_XLOG(XLOG_DEBUG, "CContextServerFactory::%s\n", __FUNCTION__);
    HttpRecConfigPtr pConfig = CHttpRecConfig::GetInstance();

    const vector<SServerConfig> &vecServerConfig = pConfig->GetServerConfig();
    for (vector<SServerConfig>::const_iterator iter = vecServerConfig.begin();
        iter != vecServerConfig.end(); ++iter)
    {
        ContextServerPtr pContextServer(new CContextServer(*iter));

        for (vector<SServerContext>::const_iterator iterContext = iter->_vecContext.begin();
            iterContext != iter->_vecContext.end(); ++iterContext)
        {
            m_mapContextServer.insert(std::make_pair(iterContext->_strDomain, pContextServer));

            if (iterContext->_strDomain.empty())
            {
                m_pDefaultContextServer = pContextServer;
            }
        }
    }
}

ContextServerPtr CContextServerFactory::GetContextServer(const string &strDomain, const string &strPath)
{
    RA_XLOG(XLOG_DEBUG, "CContextServerFactory::%s, Domain[%s], Path[%s]\n", __FUNCTION__, strDomain.c_str(), strPath.c_str());
    typedef multimap<string, ContextServerPtr>::iterator IterContextServer;

    ContextServerPtr pFind = m_pDefaultContextServer;
    int nChoosenPathLen = 0;
    std::pair<IterContextServer, IterContextServer> irFind = m_mapContextServer.equal_range(strDomain);
    while (irFind.first != irFind.second)
    {
        int nMatchedPathLength = irFind.first->second->GetMatchedContextPathLength(strDomain, strPath);
        if (-1 != nMatchedPathLength
            && nChoosenPathLen <= nMatchedPathLength)
        {
            nChoosenPathLen = nMatchedPathLength;
            pFind = irFind.first->second;
        }
        ++irFind.first;
    }

    return pFind;
}

