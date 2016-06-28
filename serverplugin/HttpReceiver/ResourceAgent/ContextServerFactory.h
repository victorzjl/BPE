#ifndef _CONTEXT_SERVRE_FACTORY_H_
#define _CONTEXT_SERVRE_FACTORY_H_

#include "ContextServer.h"

class CContextServerFactory
{
public:
    static CContextServerFactory *GetInstance();
    void Load();
    ContextServerPtr GetContextServer(const string &strDomain, const string &strPath);

private:
    CContextServerFactory();

private:
    static CContextServerFactory *s_pInstance;
    multimap<string, ContextServerPtr>m_mapContextServer;
    ContextServerPtr m_pDefaultContextServer;
};

#endif //_CONTEXT_SERVRE_FACTORY_H_