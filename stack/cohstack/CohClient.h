#ifndef _SDO_COH_CLIENT_H_
#define _SDO_COH_CLIENT_H_
#include "SmallObject.h"
#include <string>
using std::string;
namespace sdo{
	namespace coh{
		class ICohClient:public sdo::common::CSmallObject
		{
			public:
				static void SetTimeout(int nTimeout);
				void DoSendRequest(const string &strIp, unsigned int dwPort,const string & strRequest,int nTimeout=0);
				virtual void OnReceiveResponse(const string &strResponse)=0;
				virtual ~ICohClient(){}
				
		};
	}
}
#endif

