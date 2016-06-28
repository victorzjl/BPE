#include "DbOracleConnection.h"


COracleDbConnection::COracleDbConnection()
{
	
}

COracleDbConnection::~COracleDbConnection()
{
	
}

bool COracleDbConnection::CheckDbConnect()
{
	try
	{
		statement st = (dbsession.prepare <<
                "select 1 from dual");
		st.execute();
	}
	catch (std::exception const & e)    
	{       
		SV_XLOG(XLOG_WARNING,"COracleDbConnection::%s Bad Conn,Msg=[%s]!\n",__FUNCTION__,e.what());
		m_bIsConnected = false;
		return false;
	}
	
	SV_XLOG(XLOG_DEBUG,"COracleDbConnection::%s,Check Success\n",__FUNCTION__);
	m_bIsConnected = true;
	return true;

}

typedef struct _tagSPParamItem
{
	string sParamName;
	EBindDataType eType;
	DBValue paramValue;
} SSPParamItem;


int COracleDbConnection::ExecuteProcedure()
{
	
	/*
	session sql("postgresql://dbname=mydb");

int id = 123;
string name;

statement st(sql);
st.exchange(into(name));
st.exchange(use(id));
st.alloc();
st.prepare("select name from persons where id = :id");
st.define_and_bind();
st.execute(true);*/

	return 0;
}

/*
int COracleDbConnection::ExecuteProcedure(string sProcedureName,vector<SSPParamItem>& vecParams)
{
	return 0;
}*/

