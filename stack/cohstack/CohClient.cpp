#include "CohClient.h"
#include "CohConnection.h"
#include "CohStack.h"
#include "CohLogHelper.h"
namespace sdo{
namespace coh{
void ICohClient::SetTimeout(int nTimeout)
{
    CS_XLOG(XLOG_DEBUG,"ICohClient::SetTimeout,timeout[%d]\n",nTimeout);
    CCohConnection::SetTimeout(nTimeout);
}
void ICohClient::DoSendRequest(const string &strIp, unsigned int dwPort,const string & strRequest,int nTimeout)
{
    CS_XLOG(XLOG_DEBUG,"ICohClient::DoSendRequest,addr[%s:%d]\n",strIp.c_str(),dwPort);
    CohConnection_ptr new_connection(new CCohConnection(this,CCohStack::GetIoService()));
    new_connection->StartSendRequest(strIp,dwPort,strRequest,nTimeout);
}

}
}
