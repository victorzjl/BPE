/***************************************************************

Copyright (C) Snda Corporation, 2005. All Rights Reserved

Module Name:

    

Abstract:

    

Author:

    Squall 

Revision History:

******************************************************************/

#ifndef _COH_CLIENT_IMP_H_
#define _COH_CLIENT_IMP_H_
#include "CohClient.h"
#include "CohPub.h"

#include <string>
using std::string;

using namespace sdo::coh;

class CCohClientImp:public ICohClient
{
public:
	static CCohClientImp * GetInstance();
	void Init(const string &strAllUrl,const string &strDetailUrl,const string &strErrorUrl);

	void SendAllRequest(const string &strContent);
	void SendDetailRequest(const string &strContent);
	void SendOurRequeset(const string &strContent);
	void SendErrRequest(const string &strContent);
	virtual void OnReceiveResponse(const string &strResponse);
private:
	SHttpServerInfo m_oAllUrl;
	SHttpServerInfo m_oDetailUrl;
	SHttpServerInfo m_oOurUrl;
	SHttpServerInfo m_oErrUrl;
};

#endif

