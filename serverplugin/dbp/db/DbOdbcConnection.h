#ifndef _DB_ODBC_CONNECTION_H_
#define _DB_ODBC_CONNECTION_H_


#include "DbConnection.h"


class COdbcDbConnection : public CDbConnection
{
public:
	COdbcDbConnection();
	~COdbcDbConnection();

public:
	virtual bool CheckDbConnect();
	virtual int ExecuteProcedure();

	
	
};

#endif

