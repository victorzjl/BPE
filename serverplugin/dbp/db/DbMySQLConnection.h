#ifndef _DB_MYSQL_CONNECTION_H_
#define _DB_MYSQL_CONNECTION_H_

#include "DbConnection.h"


class CMysqlDbConnection : public CDbConnection
{
public:
	CMysqlDbConnection();
	~CMysqlDbConnection();

public:
	virtual bool CheckDbConnect();
	virtual int ExecuteProcedure();

	
};

#endif

