#ifndef _DB_ORACLE_CONNECTION_H_
#define _DB_ORACLE_CONNECTION_H_


#include "DbConnection.h"


class COracleDbConnection : public CDbConnection
{
public:
	COracleDbConnection();
	~COracleDbConnection();

public:
	virtual bool CheckDbConnect();
	virtual int ExecuteProcedure();

	
	
};

#endif

