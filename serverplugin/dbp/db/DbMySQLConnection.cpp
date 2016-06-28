
#include "DbMySQLConnection.h"


CMysqlDbConnection::CMysqlDbConnection()
{

}

CMysqlDbConnection::~CMysqlDbConnection()
{
	
}

bool CMysqlDbConnection::CheckDbConnect()
{
	session_backend* pBackEnd = dbsession.get_backend();
	if(!pBackEnd)
	{
		m_bIsConnected = false;
		SV_XLOG(XLOG_WARNING,"CMysqlDbConnection::%s,Check connect fail1\n",__FUNCTION__);
		return false;
	}
	mysql_session_backend* pMysql = static_cast<mysql_session_backend*>(pBackEnd);
	int nPintResult = 0;
	if( (nPintResult=mysql_ping(pMysql->conn_)) != 0)
	{
		m_bIsConnected = false;
		SV_XLOG(XLOG_WARNING,"CMysqlDbConnection::%s,Check connect fail2\n",__FUNCTION__);
		return false;
	}
	m_bIsConnected = true;
	SV_XLOG(XLOG_DEBUG,"CMysqlDbConnection::%s,Check Success\n",__FUNCTION__);
	return true;
}

int CMysqlDbConnection::ExecuteProcedure()
{
	return 0;
}


