#include "DbOdbcConnection.h"


COdbcDbConnection::COdbcDbConnection()
{
	
}

COdbcDbConnection::~COdbcDbConnection()
{
	
}

bool COdbcDbConnection::CheckDbConnect()
{
	try
	{
		statement st = (dbsession.prepare <<
                "select 1");
		st.execute();
	}
	catch (std::exception const & e)    
	{       
		SV_XLOG(XLOG_WARNING,"COdbcDbConnection::%s Bad Conn,Msg=[%s]!\n",__FUNCTION__,e.what());
		m_bIsConnected = false;
		return false;
	}
	
	SV_XLOG(XLOG_DEBUG,"COdbcDbConnection::%s,Check Success\n",__FUNCTION__);
	m_bIsConnected = true;
	return true;

}

typedef struct _tagSPParamItem
{
	string sParamName;
	EBindDataType eType;
	DBValue paramValue;
} SSPParamItem;


int COdbcDbConnection::ExecuteProcedure()
{
	

	return 0;
}


