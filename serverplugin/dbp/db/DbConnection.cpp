#include "DbConnection.h"
#include "dbconnloghelper.h"
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <boost/algorithm/string.hpp>
#include "dbexec.h"
#include "DbAngent.h"
#include "DbOracleConnection.h"
#include "DbMySQLConnection.h"
#include "DbOdbcConnection.h"
#include <boost/algorithm/string.hpp>

using namespace sdo::commonsdk;
using namespace boost;
using namespace soci;

int CDbConnection::Procedure(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,map<string,SBindDataDesc>& mapResParams,
		map<string,SProcedureResBindData>& ResBinding,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s\n",__FUNCTION__);
	
	try{	
		string strSQL = vecSQL[0];
		details::prepare_temp_type ptt = (dbsession.prepare<< strSQL);

		SSqlDyncData& dync = vecDyncData[0];
		map<string,SDyncData>::iterator it_dy = dync.begin();
		
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s SQL[%s]\n",
				__FUNCTION__,
				strSQL.c_str());
		
		//Request Parameter Binding
		for(;it_dy!=dync.end();it_dy++)
		{
			
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s namek[%s] \n",__FUNCTION__,it_dy->first.c_str());
			if ( (it_dy->second).m_eDataType == BDT_INT)
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] value[%d]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_integers[0]);
				ptt,use((it_dy->second).m_integers[0],it_dy->first.substr(1));
			}
			else if ( (it_dy->second).m_eDataType == BDT_STRING)
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] value[%s]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_strings[0].c_str());
				ptt,use((it_dy->second).m_strings[0],it_dy->first.substr(1));
			}
		}
		
		//Response Binding

		map<string,SBindDataDesc>::iterator bindIter = mapResParams.begin();
		for (;bindIter != mapResParams.end(); bindIter++)
		{
			if (bindIter->second.eDataType == BDT_STRING)
			{
				ResBinding[bindIter->first].m_eDataType = BDT_STRING;
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s ResBind STRING name[%s]\n",__FUNCTION__,bindIter->first.c_str());

				ptt,use(ResBinding[bindIter->first].m_string, bindIter->first.substr(1));
			}
			else if (bindIter->second.eDataType == BDT_INT)
			{
				ResBinding[bindIter->first].m_eDataType = BDT_INT;
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s ResBind INT name[%s]\n",__FUNCTION__,bindIter->first.c_str());
				
				ptt,use(ResBinding[bindIter->first].m_integer, bindIter->first.substr(1));
			}
			else
			{
				//

				SV_XLOG(XLOG_ERROR,"CDbConnection::%s name[%s] binder must be int or string\n",__FUNCTION__,bindIter->first.c_str());
			}
		}
		
		procedure proc = ptt;
		proc.execute(true);

	}
	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle Procedure Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL Procedure Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"Procedure Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		return -1;
	}
	return 0;

}

int CDbConnection::Query(const string& strSQL,SSqlDyncData& rowData,vector<DBRow>& fetchResult,SLastErrorDesc& errDesc)
{
	
	try{

		statement st(dbsession);

		SSqlDyncData& dync = rowData;
		map<string,SDyncData>::iterator it_dy = dync.begin();
		//SV_XLOG(XLOG_DEBUG,"CDbConnection::%s SQL[%s]\n",
		//		__FUNCTION__,
		//		strSQL.c_str());
		
		printf("query-strSql: %s\n",strSQL.c_str());
		
		for(;it_dy!=dync.end();it_dy++)
		{
			
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s namek[%s] \n",__FUNCTION__,it_dy->first.c_str());
			if ( (it_dy->second).m_eDataType == BDT_INT)
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] value[%d]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_integers[0]);
				st.exchange(use((it_dy->second).m_integers,it_dy->first.substr(1)));
			}
			else if ( (it_dy->second).m_eDataType == BDT_STRING)
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] value[%s]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_strings[0].c_str());
				st.exchange(use((it_dy->second).m_strings,it_dy->first.substr(1)));
			}
		}
	  row r;
	  st.exchange(into(r));
	  st.alloc();
	  st.prepare(strSQL);
	  st.define_and_bind();
	  bool bResult = st.execute(true);

	    SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Run: %d\n",__FUNCTION__, (int)bResult);
	  	
	//rowset<row> rs = dbsession.prepare << strSQL.c_str() ;
	//for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
	if (bResult)
	{
	do
	{
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s st.fetch\n",__FUNCTION__);
		row const& rowGet = r;
		DBRow dbRow;
		for(std::size_t i = 0; i != rowGet.size(); ++i)
		{

			const column_properties& props = rowGet.get_properties(i);
			//rowSet.SetField(i,props.get_name());
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,FieldName=[%s]\n",__FUNCTION__,props.get_name().c_str());
			switch(props.get_data_type())
			{
				case dt_string:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING \n",__FUNCTION__);
					const indicator& ind=rowGet.get_indicator(i);
					if (i_null==ind)
					{
						SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING, i_null\n",__FUNCTION__);
						dbRow.push_back("");
					}
					else
					{
						SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING,Value=[%s]\n",__FUNCTION__,(rowGet.get<std::string>(i)).c_str());
						DBValue vTmp(rowGet.get<std::string>(i));
						dbRow.push_back(vTmp);
					}
					}
					break;

				case dt_date:
					{
					
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DATE\n",__FUNCTION__);
					 const indicator& ind=rowGet.get_indicator(i);
                                        if (i_null==ind)
                                        {
                                                SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DATE, i_null\n",__FUNCTION__);
                                                dbRow.push_back("");
                                        }
					else
					{
					char szDate[64] = {0};
					std::tm oTM = rowGet.get<std::tm>(i);
					snprintf(szDate,63,"%d-%02d-%02d %02d:%02d:%02d",
						oTM.tm_year+1900,
						oTM.tm_mon + 1,
						oTM.tm_mday,
						oTM.tm_hour,
						oTM.tm_min,
						oTM.tm_sec);
					dbRow.push_back(DBValue(string(szDate)));
					}
					}
					break;

				case dt_double:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DOUBLE\n",__FUNCTION__);
					char szDouble[64] = {0};
					snprintf(szDouble,63,"%.20g",rowGet.get<double>(i));
					dbRow.push_back(DBValue(string(szDouble)));
					}
					break;

				case dt_integer:
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_INT\n",__FUNCTION__);
					dbRow.push_back(DBValue(rowGet.get<int>(i)));
					break;
/*
				case dt_unsigned_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_UL\n",__FUNCTION__);
					char szUL[64] = {0};
					snprintf(szUL,63,"%lu",rowGet.get<unsigned long>(i));
					dbRow.push_back(DBValue(string(szUL)));}
					break;
*/
				case dt_long_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_LL\n",__FUNCTION__);
					char szLL[64] = {0};
					snprintf(szLL,63,"%lld",rowGet.get<long long>(i));
					dbRow.push_back(DBValue(string(szLL)));}
					break;

				case dt_unsigned_long_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_ULL\n",__FUNCTION__);
					char szULL[64] = {0};
					snprintf(szULL,63,"%llu",rowGet.get<unsigned long long>(i));
					dbRow.push_back(DBValue(string(szULL)));}
					break;
					
				default:
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,OTHER\n",__FUNCTION__);
					break;
			}
		}

		fetchResult.push_back(dbRow);
	}while(st.fetch());
	}
	
	}
	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle QuerySQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL QuerySQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"QuerySQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		return -1;
	}


	return 0;
}


int CDbConnection::Query(const string& strSQL,vector<DBRow>& fetchResult,SLastErrorDesc& errDesc)
{
	//SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, SQL=[%s]\n",__FUNCTION__,strSQL.c_str());
	printf("query-strSql: %s\n",strSQL.c_str());
	try{

	
	rowset<row> rs = dbsession.prepare << strSQL.c_str() ;
	for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
	{
		row const& rowGet = *it;
		DBRow dbRow;
		for(std::size_t i = 0; i != rowGet.size(); ++i)
		{

			const column_properties& props = rowGet.get_properties(i);
			//rowSet.SetField(i,props.get_name());
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,FieldName=[%s]\n",__FUNCTION__,props.get_name().c_str());
			switch(props.get_data_type())
			{
				case dt_string:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING,Value=[%s]\n",__FUNCTION__,(rowGet.get<std::string>(i)).c_str());
					DBValue vTmp(rowGet.get<std::string>(i));
					dbRow.push_back(vTmp);
					}
					break;

				case dt_date:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DATE\n",__FUNCTION__);
					char szDate[64] = {0};
					std::tm oTM = rowGet.get<std::tm>(i);
					snprintf(szDate,63,"%d-%02d-%02d %02d:%02d:%02d",
						oTM.tm_year+1900,
						oTM.tm_mon + 1,
						oTM.tm_mday,
						oTM.tm_hour,
						oTM.tm_min,
						oTM.tm_sec);
					dbRow.push_back(DBValue(string(szDate)));
					}
					break;

				case dt_double:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DOUBLE\n",__FUNCTION__);
					char szDouble[64] = {0};
					snprintf(szDouble,63,"%.20g",rowGet.get<double>(i));
					dbRow.push_back(DBValue(string(szDouble)));
					}
					break;

				case dt_integer:
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_INT\n",__FUNCTION__);
					dbRow.push_back(DBValue(rowGet.get<int>(i)));
					break;
/*
				case dt_unsigned_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_UL\n",__FUNCTION__);
					char szUL[64] = {0};
					snprintf(szUL,63,"%lu",rowGet.get<unsigned long>(i));
					dbRow.push_back(DBValue(string(szUL)));}
					break;
*/
				case dt_long_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_LL\n",__FUNCTION__);
					char szLL[64] = {0};
					snprintf(szLL,63,"%lld",rowGet.get<long long>(i));
					dbRow.push_back(DBValue(string(szLL)));}
					break;

				case dt_unsigned_long_long:
					{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_ULL\n",__FUNCTION__);
					char szULL[64] = {0};
					snprintf(szULL,63,"%llu",rowGet.get<unsigned long long>(i));
					dbRow.push_back(DBValue(string(szULL)));}
					break;
					
				default:
					break;
			}
		}

		fetchResult.push_back(dbRow);
	}
	}
	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle QuerySQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL QuerySQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"QuerySQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		return -1;
	}


	return 0;
}


void CDbConnection::GetBindParams(string strSQL,vector<string>& vecParams)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,SQL=[%s]\n",__FUNCTION__,strSQL.c_str());

	size_t found;
    while(found != string::npos)
    {
	    found=strSQL.find(":");
	    if(found != string::npos)
	    {
	        strSQL.erase(0,found);

	        found=strSQL.find_first_of(" ,)");
			string sTmp = strSQL.substr(0,found);
			boost::to_lower(sTmp);
	        vecParams.push_back(sTmp);
	        strSQL.erase(0,found);
	    }
    }

	vector<string>::iterator iterParam;
	for(iterParam=vecParams.begin();iterParam!=vecParams.end();++iterParam)
	{
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,Param=[%s]\n",__FUNCTION__,(*iterParam).c_str());
	}
}


void CDbConnection::DumpParamValues(vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType)
{
	vector<ptree>::iterator iterPV;
	for(iterPV = vecParamValues.begin(); iterPV != vecParamValues.end(); ++iterPV)
	{	
		ptree& tree = *iterPV;
		ptree::iterator iterTree;
		for(iterTree=tree.begin();iterTree!=tree.end();++iterTree)
		{
			string sParamName = iterTree->first;
			map<string,SBindDataDesc>::iterator iterParamDesc = mapBindDataType.find(sParamName);
			if(iterParamDesc != mapBindDataType.end())
			{
				if((iterParamDesc->second).eDataType == BDT_INT)
				{
					int nValue = boost::get<int>(iterTree->second);
					SV_XLOG(XLOG_DEBUG,"::::::::::::: Param :: Name=[%s],INT,Value=[%d]\n",
						sParamName.c_str(),nValue);
				}
				else if((iterParamDesc->second).eDataType == BDT_STRING)
				{
					string sValue = boost::get<string>(iterTree->second);
					SV_XLOG(XLOG_DEBUG,"::::::::::::: Param :: Name=[%s],STRING,Value=[%s]\n",
						sParamName.c_str(),sValue.c_str());
				}
			}
		}
	}
}

int CDbConnection::Execute(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	if (0==m_dwSmart)
		return ExecuteNow(vecSQL,vecDyncData,nAffectedRows,errDesc);
	else 
	{
		return ExecuteSmart(vecSQL,vecDyncData,nAffectedRows,errDesc);
	}
}

void CDbConnection::SmartAlloc()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s \n",__FUNCTION__);
	if (NULL==m_pSmartTrans)
	{
		m_pSmartTrans = new transaction(dbsession);
	}
}

void CDbConnection::SmartRollBack()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s \n",__FUNCTION__);
	if (m_pSmartTrans!=NULL)
	{
		m_pSmartTrans->rollback();
		delete m_pSmartTrans;
		m_pSmartTrans =NULL;
	}
}

void CDbConnection::SmartCommitNow()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s \n",__FUNCTION__);
	if (m_pSmartTrans!=NULL)
	{
		if (m_dwPreparedSQLCount>0)
		{
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s  RealDoNow [%d]\n",__FUNCTION__,m_dwPreparedSQLCount);
			time_t now=time(NULL);
			m_pSmartTrans->commit();
			delete m_pSmartTrans;
			m_pSmartTrans =NULL;
			m_dwPreparedSQLCount=0;
			m_dwLastCommitTime = now;
		}
	}
}

void CDbConnection::SmartCommit()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s \n",__FUNCTION__);
	if (m_pSmartTrans!=NULL)
	{
		if (m_dwPreparedSQLCount>0)
		{
			time_t now=time(NULL);
			if (m_dwPreparedSQLCount>=1000|| now-m_dwLastCommitTime>30)
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s  RealDo [%d]\n",__FUNCTION__,m_dwPreparedSQLCount);
				m_pSmartTrans->commit();
				delete m_pSmartTrans;
				m_pSmartTrans =NULL;
				m_dwPreparedSQLCount=0;
				m_dwLastCommitTime = now;
			}
		}
	}
}

int CDbConnection::ExecuteSmart(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Middle\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	vector<string>::iterator iterSQL; 
	vector<SSqlDyncData>::iterator iterDync;
	
	try
	{
		SmartAlloc();

	try
	{	

		iterDync = vecDyncData.begin();
		for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL,iterDync++)
		{
#ifdef WIN32
			statement *new_statement = new statement(dbsession);
			statement &st=*new_statement;
#else
			statement st(dbsession);
#endif
			SSqlDyncData& dync = *iterDync;
			map<string,SDyncData>::iterator it_dy = dync.begin();
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s SQL[%s]\n",
					__FUNCTION__,
					(*iterSQL).c_str());
			
			for(;it_dy!=dync.end();it_dy++)
			{
				
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s nameq[%s] \n",__FUNCTION__,it_dy->first.c_str());
				if ( (it_dy->second).m_eDataType == BDT_INT)
				{
					//SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s]  INT value[%d]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_integers[0]);
					st.exchange(use((it_dy->second).m_integers,it_dy->first.substr(1)));
				}
				else 
				{
					//SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] STRING value[%s]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_strings[0].c_str());
					st.exchange(use((it_dy->second).m_strings,it_dy->first.substr(1)));
				}
			}
			st.alloc();
			st.prepare(*iterSQL);
			st.define_and_bind();
		
			st.execute(true);
			int nTmp = st.get_affected_rows();
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s nTmp[%d] \n",__FUNCTION__,nTmp);
			nAffectedRows += nTmp;
			
	
		

		}
		

	}
	catch (oracle_soci_error const & e)
	{	
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{	
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SV_XLOG(XLOG_ERROR,"Begin to RollBack\n");
		SmartRollBack();
		SV_XLOG(XLOG_ERROR,"End to RollBack\n");
		return -1;
	}
	catch(std::exception const &e)
	{		
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
		m_dwPreparedSQLCount+=vecSQL.size();
		SmartCommit();
	}
	catch(std::exception const &e)
	{		
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	
	return nRet;
}

bool CDbConnection::IsHybridSql(vector<string>& vecSQL)
{
	if (vecSQL.size()<=1)
	{
		return false;
	}
	for(vector<string>::iterator it = vecSQL.begin();it!=vecSQL.end();it++)
	{
		if(boost::istarts_with(*it,"select"))
		{
			return true;
		}
	}
	return false;
}


	
int CDbConnection::ExecuteNow(vector<string>& vecSQL,vector<SSqlDyncData>& vecDyncData,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Middle\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	bool isHybrid = IsHybridSql(vecSQL);	
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Middle %d\n",__FUNCTION__, (int)isHybrid);
	vector<string>::iterator iterSQL; 
	vector<SSqlDyncData>::iterator iterDync;
	int nIndexSql=1;
	
	try
	{
		transaction trans(dbsession);
	
	try
	{	

		iterDync = vecDyncData.begin();
		for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL,iterDync++)
		{
			map<string,SqlRespData> vecRespData;
			char szSqlIndex[20]={0};
			snprintf(szSqlIndex,sizeof(szSqlIndex),"s%d",nIndexSql);
			string strSqlIndex = szSqlIndex;
			nIndexSql++;
#ifdef WIN32
			statement *new_statement = new statement(dbsession);
			statement &st=*new_statement;
#else
			statement st(dbsession);
#endif
			SSqlDyncData& dync = *iterDync;
			map<string,SDyncData>::iterator it_dy = dync.begin();
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s SQL[%s]\n",__FUNCTION__, (*iterSQL).c_str());
			
			for(;it_dy!=dync.end();it_dy++)
			{
				
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s nameq[%s] \n",__FUNCTION__,it_dy->first.c_str());
				if ( (it_dy->second).m_eDataType == BDT_INT)
				{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s]  INT value[%d]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_integers[0]);
					st.exchange(use((it_dy->second).m_integers,it_dy->first.substr(1)));
				}
				else 
				{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s name[%s] STRING value[%s]\n",__FUNCTION__,it_dy->first.c_str(),(it_dy->second).m_strings[0].c_str());
					st.exchange(use((it_dy->second).m_strings,it_dy->first.substr(1)));
				}
			}
			row r;
			st.exchange(into(r));
			st.alloc();
			st.prepare(*iterSQL);
			st.define_and_bind();
		

			bool bResult = st.execute(true);
			int nTmp = st.get_affected_rows();
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s nTmp[%d] \n",__FUNCTION__,nTmp);
			nAffectedRows += nTmp;
			
			if (isHybrid)
			{
				if(boost::istarts_with(*iterSQL,"select") )
				{
					if (!bResult)
					{
						SV_XLOG(XLOG_DEBUG,"CDbConnection::%s NO select data \n",__FUNCTION__);
						trans.rollback();
						return -1;
					}					
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s st.fetch\n",__FUNCTION__);
					row const& rowGet = r;
					DBRow dbRow;
					for(std::size_t i = 0; i != rowGet.size(); ++i)
					{
						const column_properties& props = rowGet.get_properties(i);
						string fieldName = props.get_name();
						SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,FieldName=[%s]\n",__FUNCTION__,props.get_name().c_str());
						switch(props.get_data_type())
						{
							case dt_string:
								{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING \n",__FUNCTION__);
								const indicator& ind=rowGet.get_indicator(i);
								if (i_null==ind)
								{
									SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING, i_null\n",__FUNCTION__);
									SqlRespData respData;
									respData.m_type="string";
									string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
									respData.m_name=strFormatName;
									respData.m_string="";
									vecRespData[strFormatName]=respData;
								}
								else
								{
									SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_STRING,Value=[%s]\n",__FUNCTION__,(rowGet.get<std::string>(i)).c_str());
									SqlRespData respData;
									respData.m_type="string";
									string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
									respData.m_name=strFormatName;
									respData.m_string=rowGet.get<std::string>(i);
									vecRespData[strFormatName]=respData;
								}
								}
								break;

							case dt_date:
								{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DATE\n",__FUNCTION__);
								 const indicator& ind=rowGet.get_indicator(i);
								if (i_null==ind)
								{
										SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DATE, i_null\n",__FUNCTION__);
									SqlRespData respData;
									respData.m_type="string";
									string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
									respData.m_name=strFormatName;
									respData.m_string="";
									vecRespData[strFormatName]=respData;
								}
								else
								{
								char szDate[64] = {0};
								std::tm oTM = rowGet.get<std::tm>(i);
								snprintf(szDate,63,"%d-%02d-%02d %02d:%02d:%02d",
									oTM.tm_year+1900,
									oTM.tm_mon + 1,
									oTM.tm_mday,
									oTM.tm_hour,
									oTM.tm_min,
									oTM.tm_sec);
								SqlRespData respData;
								respData.m_type="string";
								string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
								respData.m_name=strFormatName;
								respData.m_string=szDate;
								vecRespData[strFormatName]=respData;
								}
								}
								break;

							case dt_double:
								{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_DOUBLE\n",__FUNCTION__);
								char szDouble[64] = {0};
								snprintf(szDouble,63,"%.20g",rowGet.get<double>(i));
								SqlRespData respData;
								respData.m_type="string";
								string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
								respData.m_name=strFormatName;
								respData.m_string=szDouble;
								vecRespData[strFormatName]=respData;
								}
								break;

							case dt_integer:
							{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_INT\n",__FUNCTION__);
								SqlRespData respData;
								respData.m_type="int";
								string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
								respData.m_name=strFormatName;
								respData.m_integer=rowGet.get<int>(i);
								vecRespData[strFormatName]=respData;
							}
								break;
		
							case dt_long_long:
								{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_LL\n",__FUNCTION__);
								char szLL[64] = {0};
								snprintf(szLL,63,"%lld",rowGet.get<long long>(i));
								SqlRespData respData;
								respData.m_type="string";
								string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
								respData.m_name=strFormatName;
								respData.m_string=szLL;
								vecRespData[strFormatName]=respData;
								}
								break;

							case dt_unsigned_long_long:
								{
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,DT_ULL\n",__FUNCTION__);
								char szULL[64] = {0};
								snprintf(szULL,63,"%llu",rowGet.get<unsigned long long>(i));
								SqlRespData respData;
								respData.m_type="string";
								string strFormatName="$rsp."+strSqlIndex+"."+fieldName;
								respData.m_name=strFormatName;
								respData.m_string=szULL;
								vecRespData[strFormatName]=respData;
								}
								break;
								
							default:
								SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,OTHER\n",__FUNCTION__);
								break;
						}
					}
					


				}
				map<string,SqlRespData>::iterator it_resp= vecRespData.begin();
				for(;it_resp!=vecRespData.end();it_resp++)
				{
					for(vector<string>::iterator it1=iterSQL;it1!=vecSQL.end();++it1)
					{
						if (it_resp->second.m_type=="int")
						{
							char szTmp[64] = {0};
							snprintf(szTmp,63,"%u",it_resp->second.m_integer);
							boost::ireplace_all(*it1,it_resp->first,szTmp);
						}
						else
						{
							string sTmpInSQL = "'" + it_resp->second.m_string + "'";
							boost::ireplace_all(*it1,it_resp->first,sTmpInSQL);
						}	
					}
				}
				
			}
	
		

		}
		

	}
	catch (oracle_soci_error const & e)
	{	
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{	
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SV_XLOG(XLOG_ERROR,"Begin to RollBack\n");
		trans.rollback();
		SV_XLOG(XLOG_ERROR,"End to RollBack\n");
		return -1;
	}
	catch(std::exception const &e)
	{		
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	
		trans.commit();
	}
	catch(std::exception const &e)
	{		
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		return -1;
	}
	
	return nRet;
}

int CDbConnection::Execute(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	if (m_dwSmart==0)
	{
		return ExecuteNow(vecSQL,nAffectedRows,errDesc);
	}
	else
	{
		return ExecuteSmart(vecSQL,nAffectedRows,errDesc);
	}

}

int CDbConnection::ExecuteSmart(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Small\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	try{
	SmartAlloc();
	vector<string>::iterator iterSQL; 

	try
	{	
		for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL)
		{
			statement st(dbsession);
			st.alloc();
			st.prepare(*iterSQL);
			st.define_and_bind();
			st.execute(true);
			nAffectedRows += st.get_affected_rows();

		}
	}
	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	m_dwPreparedSQLCount+=vecSQL.size();
	SmartCommit();
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	return nRet;
	
}

int CDbConnection::ExecuteNow(vector<string>& vecSQL,int& nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Small\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	try{
	transaction trans(dbsession);
	vector<string>::iterator iterSQL; 

	try
	{	
		for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL)
		{
			statement st(dbsession);
			st.alloc();
			st.prepare(*iterSQL);
			st.define_and_bind();
			st.execute(true);
			nAffectedRows += st.get_affected_rows();

		}
	}
	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}

	trans.commit();
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		return -1;
	}
	return nRet;
	
}

int CDbConnection::Execute(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc)
{
	if (m_dwSmart==0)
	{	
		return ExecuteNow(vecSQL,nAffectedRows,vecParamValues,mapBindDataType,errDesc);
	}
	else
	{
		return ExecuteSmart(vecSQL,nAffectedRows,vecParamValues,mapBindDataType,errDesc);
	}
}

int CDbConnection::ExecuteSmart(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Big\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	try{
	SmartAlloc();
	vector<string>::iterator iterSQL; //naive sql

	DumpParamValues(vecParamValues,mapBindDataType);

	try
	{


	vector<ptree>::iterator iterParamValues = vecParamValues.begin();
	for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL,iterParamValues++)
	{
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,execute sql=[%s]\n",__FUNCTION__,(*iterSQL).c_str());

		statement st(dbsession);
		st.alloc();
		st.prepare(*iterSQL);

		vector<string> vecBindParams;
		GetBindParams(*iterSQL,vecBindParams);
		vector<string>::iterator iterParam;

		map<string,vector<int> > mapIntValues;
		map<string,vector<string> > mapStrValues;
		
		for(iterParam = vecBindParams.begin();iterParam != vecBindParams.end(); ++iterParam)
		{
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,FindParam[%s]\n",__FUNCTION__,(*iterParam).c_str());

			
			map<string,SBindDataDesc>::iterator iterParamDesc;

			iterParamDesc = mapBindDataType.find(*iterParam);

			if(iterParamDesc!=mapBindDataType.end())
			{
				SBindDataDesc& paramDesc = iterParamDesc->second;
				if(paramDesc.eDataType == BDT_INT)
				{
					vector<int> vecIntTmp;
					
				//	for(iterParamValues = vecParamValues.begin(); iterParamValues != vecParamValues.end(); ++iterParamValues)
					{
						ptree& pt = *iterParamValues;
						map<string,variant<int,string> >::iterator iterPV = pt.find(*iterParam);

						if(iterPV != pt.end())
						{
							vecIntTmp.push_back(boost::get<int>(iterPV->second));
							SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,IntValue=[%d]\n",__FUNCTION__,boost::get<int>(iterPV->second));
						}
					}
					mapIntValues[GetActualBindName(*iterParam)] = vecIntTmp;
					
				}
				else if(paramDesc.eDataType == BDT_STRING)
				{
					vector<string> vecStrTmp;

					//for(iterParamValues = vecParamValues.begin(); iterParamValues != vecParamValues.end(); ++iterParamValues)
					{
						ptree& pt = *iterParamValues;
						map<string,variant<int,string> >::iterator iterPV = pt.find(*iterParam);
						if(iterPV != pt.end())
						{
							string strTmpSpre = boost::get<string>(iterPV->second);
							HanleNIL(strTmpSpre);
							vecStrTmp.push_back(strTmpSpre);
							SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,StringValue=[%s]\n",__FUNCTION__,(boost::get<string>(iterPV->second)).c_str());
						}
					}

					mapStrValues[GetActualBindName(*iterParam)] = vecStrTmp;
				}
				else
				{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,Not Support Type,ParamName=[%s],Type=[%d]\n",
						__FUNCTION__,(*iterParam).c_str(),paramDesc.eDataType);
				}

			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,Can Not Find BindDataDesc,ParamName=[%s]\n",
						__FUNCTION__,(*iterParam).c_str());
			}

		}

		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,OK,Then We go Execute\n",__FUNCTION__);
		map<string,vector<int> >::iterator iterMapIntValue;
		for(iterMapIntValue=mapIntValues.begin();iterMapIntValue!=mapIntValues.end();++iterMapIntValue)
		{
			st.exchange(use(iterMapIntValue->second,iterMapIntValue->first));
		}

		map<string,vector<string> >::iterator iterMapStrValue;
		for(iterMapStrValue=mapStrValues.begin();iterMapStrValue!=mapStrValues.end();++iterMapStrValue)
		{
			st.exchange(use(iterMapStrValue->second,iterMapStrValue->first));
		}
	
		st.define_and_bind();

		//int nExecuteCount = vecParamValues.size();
		//for(int i = 0;i<nExecuteCount;i++)
		//{
			st.execute(true);
			nAffectedRows += st.get_affected_rows();	
		//}
		
	}

	}

	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	m_dwPreparedSQLCount+=vecSQL.size();
	SmartCommit();
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		SmartRollBack();
		return -1;
	}
	return nRet;
}

int CDbConnection::ExecuteNow(vector<string>& vecSQL,int& nAffectedRows,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s Big\n",__FUNCTION__);

	int nRet = 0;
	nAffectedRows = 0;
	
	try{
	transaction trans(dbsession);
	vector<string>::iterator iterSQL; //naive sql

	DumpParamValues(vecParamValues,mapBindDataType);

	try
	{


	vector<ptree>::iterator iterParamValues = vecParamValues.begin();
	for(iterSQL=vecSQL.begin();iterSQL!=vecSQL.end();++iterSQL,iterParamValues++)
	{
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,execute sql=[%s]\n",__FUNCTION__,(*iterSQL).c_str());

		statement st(dbsession);
		st.alloc();
		st.prepare(*iterSQL);

		vector<string> vecBindParams;
		GetBindParams(*iterSQL,vecBindParams);
		vector<string>::iterator iterParam;

		map<string,vector<int> > mapIntValues;
		map<string,vector<string> > mapStrValues;
		
		for(iterParam = vecBindParams.begin();iterParam != vecBindParams.end(); ++iterParam)
		{
			SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,FindParam[%s]\n",__FUNCTION__,(*iterParam).c_str());

			
			map<string,SBindDataDesc>::iterator iterParamDesc;

			iterParamDesc = mapBindDataType.find(*iterParam);

			if(iterParamDesc!=mapBindDataType.end())
			{
				SBindDataDesc& paramDesc = iterParamDesc->second;
				if(paramDesc.eDataType == BDT_INT)
				{
					vector<int> vecIntTmp;
					
				//	for(iterParamValues = vecParamValues.begin(); iterParamValues != vecParamValues.end(); ++iterParamValues)
					{
						ptree& pt = *iterParamValues;
						map<string,variant<int,string> >::iterator iterPV = pt.find(*iterParam);

						if(iterPV != pt.end())
						{
							vecIntTmp.push_back(boost::get<int>(iterPV->second));
							SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,IntValue=[%d]\n",__FUNCTION__,boost::get<int>(iterPV->second));
						}
					}
					mapIntValues[GetActualBindName(*iterParam)] = vecIntTmp;
					
				}
				else if(paramDesc.eDataType == BDT_STRING)
				{
					vector<string> vecStrTmp;

					//for(iterParamValues = vecParamValues.begin(); iterParamValues != vecParamValues.end(); ++iterParamValues)
					{
						ptree& pt = *iterParamValues;
						map<string,variant<int,string> >::iterator iterPV = pt.find(*iterParam);
						if(iterPV != pt.end())
						{
							string strTmpSpre = boost::get<string>(iterPV->second);
							HanleNIL(strTmpSpre);
							vecStrTmp.push_back(strTmpSpre);
							SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,StringValue=[%s]\n",__FUNCTION__,(boost::get<string>(iterPV->second)).c_str());
						}
					}

					mapStrValues[GetActualBindName(*iterParam)] = vecStrTmp;
				}
				else
				{
					SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,Not Support Type,ParamName=[%s],Type=[%d]\n",
						__FUNCTION__,(*iterParam).c_str(),paramDesc.eDataType);
				}

			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,Can Not Find BindDataDesc,ParamName=[%s]\n",
						__FUNCTION__,(*iterParam).c_str());
			}

		}

		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,OK,Then We go Execute\n",__FUNCTION__);
		map<string,vector<int> >::iterator iterMapIntValue;
		for(iterMapIntValue=mapIntValues.begin();iterMapIntValue!=mapIntValues.end();++iterMapIntValue)
		{
			st.exchange(use(iterMapIntValue->second,iterMapIntValue->first));
		}

		map<string,vector<string> >::iterator iterMapStrValue;
		for(iterMapStrValue=mapStrValues.begin();iterMapStrValue!=mapStrValues.end();++iterMapStrValue)
		{
			st.exchange(use(iterMapStrValue->second,iterMapStrValue->first));
		}
	
		st.define_and_bind();

		//int nExecuteCount = vecParamValues.size();
		//for(int i = 0;i<nExecuteCount;i++)
		//{
			st.execute(true);
			nAffectedRows += st.get_affected_rows();	
		//}
		
	}

	}

	catch (oracle_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"Oracle ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	catch (mysql_soci_error const & e)
	{
		SV_XLOG(XLOG_ERROR,"MySQL ExeSQL Error,Msg=[%s],ErrorNo=[%d]\n",e.what(),e.err_num_);
		errDesc.nErrNo = e.err_num_;
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"ExeSQL Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();
		trans.rollback();
		return -1;
	}

	trans.commit();
	}
	catch(std::exception const &e)
	{	
		SV_XLOG(XLOG_ERROR,"transaction Error,Msg=[%s]\n",e.what());
		errDesc.sErrMsg = e.what();

		return -1;
	}
	return nRet;
}

int CDbConnection::ExecuteProcedure()
{
	return 0;
}



//-----------------------------------------------------
CDbConnection* MakeConnection()
{
	return new CDbConnection();
}

int GetDBType(const string& str)
{
	if(strncmp(str.c_str(),"oracle",6)==0)
		return DBT_ORACLE;
	else if(strncmp(str.c_str(),"mysql",5)==0)
		return DBT_MYSQL;
	else if(strncmp(str.c_str(),"odbc",4)==0)
		return DBT_ODBC;
	else
		return -10000;
}

CDbConnection* MakeConnection(const string & str)
{
	int dbType = GetDBType(str);
	if(dbType == DBT_ORACLE)
	{
		return (new COracleDbConnection());
	}
	else if(dbType == DBT_MYSQL)
	{
		return (new CMysqlDbConnection());
	}
	else if(dbType == DBT_ODBC)
	{
		return (new COdbcDbConnection());
	}
	else
	{
		return new CDbConnection();
	}
}

void CDbConnection::SetSmart(int dwSmart)
{
	m_dwSmart=dwSmart;
}
CDbConnection::CDbConnection()
{
	m_bIsConnected = false;
	m_nConnectIndex = 0;
	m_nSubIndex = 0;
	m_pParent = NULL;	
	m_PreviousConnected = 0;
	m_dwSmart = 0;//默认不开启性能优化
	m_dwPreparedSQLCount = 0;
	m_pSmartTrans = NULL;
	m_dwLastCommitTime = 0;
}

CDbConnection::~CDbConnection()
{
	vector<CDbConnection*>::iterator it = m_pChilds.begin();
	for(;it!=m_pChilds.end();it++)
	{
		delete *it;
	}
	m_pChilds.clear(); 
}

int CDbConnection::DoConnectDb(int nIndex,int nSubIndex,bool bMaster,vector<string> &strConns)
{

	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] strConn[%d]\n",__FUNCTION__, 
		nIndex,strConns.size()); 
	m_isFree = true;
	m_isMaster = bMaster;
	m_nIndex = nIndex;
	m_nSubIndex = nSubIndex;
	m_bIsConnected = true;
	m_nConnectNum = strConns.size();
	
	for(unsigned int i=0;i<strConns.size(); i++)
	{
		CDbConnection* p = MakeConnection(strConns[i]);
		p->SetPreSql(m_strPreSql);
		p->m_pParent = this;
		p->m_pAngent = this->m_pAngent;
		
		if (-1==p->DoConnectDb(nIndex,nSubIndex,i,bMaster,strConns[i]))
		{
			
			return -1;
		}
		m_pChilds.push_back(p);
	}	
	return 0;
}

void CDbConnection::DoPreSql()
{
	if (m_strPreSql.size()>0)
	{
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, m_strPreSql[%s]\n",__FUNCTION__, m_strPreSql.c_str()); 
		statement st(dbsession);
		st.alloc();
		st.prepare(m_strPreSql);
		st.define_and_bind();
		st.execute(true);
	}
}

void* Thread_ConnectDB(void *pvoid)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, ASYNCONNECT THREAD \n",__FUNCTION__); 
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CConnectControl* pControl= (CConnectControl*)pvoid;
	CDbConnection* pDbConnection= (CDbConnection*)pControl->pConnection;
	if (pControl->bFirstConnect)
	{
		pDbConnection->DoRealConnectDb(pDbConnection->m_nIndex,pDbConnection->m_nSubIndex,pDbConnection->m_nConnectIndex,
								  pDbConnection->m_isMaster,pDbConnection->m_strConn);
	}
	else
	{
		pDbConnection->DoRealReConnectDb();
	}
	pControl->cond.notify_one();
	return (void*)0;
}

int CDbConnection::DoConnectDb(int nIndex,int nSubIndex,int nConnectIndex,bool bMaster,const string & strConn)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] connindex[%d], strConn[%s]\n",__FUNCTION__, 
		nIndex,nConnectIndex,strConn.c_str()); 	

	this->m_pConnectControl.reset(new CConnectControl);
	this->m_pConnectControl->pConnection = this;
	this->m_pConnectControl->bFirstConnect = true;
	this->m_strConn = strConn;
	this->m_isMaster = bMaster;
	this->m_nIndex = nIndex;
	this->m_nSubIndex = nSubIndex;
	this->m_nConnectIndex = nConnectIndex;
		
	pthread_t ntid;
	pthread_create(&ntid,NULL,Thread_ConnectDB,(void*)(this->m_pConnectControl.get()));
	
	boost::unique_lock<boost::mutex> lock(this->m_pConnectControl->mut);
	if(this->m_pConnectControl->cond.timed_wait<boost::posix_time::seconds>(lock,boost::posix_time::seconds(3))==false) 
	{
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT WAIT_TIMEOUT\n",__FUNCTION__); 
		if (0==pthread_cancel(ntid))
		{
			SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CANCLE_OK\n",__FUNCTION__); 
		}
		else
		{
			SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CANCLE FAIL\n",__FUNCTION__); 
		}
	}
	void*status;
	pthread_join(ntid,&status);
	if (this->m_bIsConnected)
	{
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CONNECT OK\n",__FUNCTION__); 
		return 0;
	}
	SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CONNECT FAIL nIndex[%d] connindex[%d], strConn[%s]\n",__FUNCTION__, 
		nIndex,nConnectIndex,strConn.c_str()); 
	return -1;
}

int CDbConnection::DoRealConnectDb(int nIndex,int nSubIndex,int nConnectIndex,bool bMaster,const string & strConn)
{
	m_strConn = strConn;
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] m_nConnectIndex[%d]%s \n",__FUNCTION__, 
			m_nIndex,m_nConnectIndex,m_strConn.c_str()
			); 
			
	try{
		/* strConn should be endname://desc info */
		m_isFree = true;
		m_isMaster = bMaster;
		m_nIndex = nIndex;
		m_nSubIndex = nSubIndex;
		m_nConnectIndex = nConnectIndex;

		dbsession.open(m_strConn);
		
		m_PreviousConnected=1;
		m_bIsConnected = true;

		string sBackEndName = dbsession.get_backend_name();
		if(sBackEndName == "oracle")
		{
			m_eDBType = DBT_ORACLE;
		}
		else if(sBackEndName == "mysql")
		{
			m_eDBType = DBT_MYSQL;
		}
		else if(sBackEndName == "odbc")
		{
			m_eDBType = DBT_ODBC;
		}
		else
		{
			m_eDBType = DBT_SIMPLE;
		}
		DoPreSql();
	}	
	catch (std::exception const & e)    
	{       
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s,ErrorMsg=[%s]\n",__FUNCTION__,e.what());
		return -1;
	}
	return 0;
}


int CDbConnection::DoReConnectDb()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] m_nConnectIndex[%d]%s \n",__FUNCTION__, 
			m_nIndex,m_nConnectIndex,m_strConn.c_str()
			); 

	this->m_pConnectControl.reset(new CConnectControl);
	this->m_pConnectControl->pConnection = this;
	this->m_pConnectControl->bFirstConnect = false;
	
	pthread_t ntid;
	pthread_create(&ntid,NULL,Thread_ConnectDB,(void*)(this->m_pConnectControl.get()));
	
	boost::unique_lock<boost::mutex> lock(this->m_pConnectControl->mut);
	if(this->m_pConnectControl->cond.timed_wait<boost::posix_time::seconds>(lock,boost::posix_time::seconds(3))==false) 
	{
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT WAIT TIMEOUT\n",__FUNCTION__); 
		if (0==pthread_cancel(ntid))
		{
			SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CANCLE OK\n",__FUNCTION__); 
		}
		else
		{
			SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CANCLE FAIL\n",__FUNCTION__); 
		}
	}
	void*status;
	pthread_join(ntid,&status);
	if (this->m_bIsConnected)
	{
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CONNECT OK\n",__FUNCTION__); 
		return 0;
	}
	SV_XLOG(XLOG_FATAL,"CDbConnection::%s, ASYNCONNECT CONNECT FAIL nIndex[%d] m_nConnectIndex[%d]%s \n",__FUNCTION__, 
			m_nIndex,m_nConnectIndex,m_strConn.c_str()
			); 
	return -1;
}

int CDbConnection::DoRealReConnectDb()
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] m_nConnectIndex[%d]%s \n",__FUNCTION__, 
			m_nIndex,m_nConnectIndex,m_strConn.c_str()
			); 

	try{
		if (m_PreviousConnected == 0)
		{
			dbsession.open(m_strConn);
			
			string sBackEndName = dbsession.get_backend_name();
			if(sBackEndName == "oracle")
			{
				m_eDBType = DBT_ORACLE;
			}
			else if(sBackEndName == "mysql")
			{
				m_eDBType = DBT_MYSQL;
			}
			else if(sBackEndName == "odbc")
			{
				m_eDBType = DBT_ODBC;
			}
			else
			{
				m_eDBType = DBT_SIMPLE;
			}
			DoPreSql();
			m_PreviousConnected=1;
			SV_XLOG(XLOG_DEBUG,"First Connect OK\n");
		}
		else
		{
			dbsession.reconnect();
			SV_XLOG(XLOG_DEBUG,"Re Connect OK\n");
		}
		
		m_bIsConnected = true;
		
	}	
	catch (std::exception const & e)    
	{       
		m_bIsConnected = false;
		SV_XLOG(XLOG_FATAL,"CDbConnection::%s,ErrorMsg=[%s]\n",__FUNCTION__,e.what());
		return -1;
	}

	return 0;
}

int CDbConnection::DoQuery(SDbQueryMsg* pMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] \n",__FUNCTION__, m_nIndex); 
	gettimeofday_a(&pMsg->sResponse.m_tStartDB, 0);
 	int nRet = ExeculteCmd(pMsg);
	gettimeofday_a(&pMsg->sResponse.m_tEndDB, 0);
	pMsg->sResponse.isqueryed = true;
	pMsg->sResponse.nCode = nRet;

	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] ExecnRet[%d]\n",__FUNCTION__, m_nIndex,nRet); 

	//CheckDbConnect();
	if(nRet==-3)
	{
		pMsg->sResponse.nCode = ERROR_MISSPARA;
	}
	else if(nRet==-2)
	{
		pMsg->sResponse.nCode = ERROR_BINDRESPONSE;
	}
	else if(nRet!=0)
	{
		CheckDbConnect();
		pMsg->sResponse.nCode = ERROR_DB;
	}
	

	return 0;
}
int CDbConnection::DoWrite(SDbQueryMsg* pMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] \n",__FUNCTION__, m_nIndex); 
	gettimeofday_a(&pMsg->sResponse.m_tStartDB, 0);
 	int nRet = ExeculteCmd(pMsg);
	gettimeofday_a(&pMsg->sResponse.m_tEndDB, 0);
	pMsg->sResponse.isqueryed = true;
	pMsg->sResponse.nCode = nRet;
	
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s, nIndex[%d] ExecnRet[%d]\n",__FUNCTION__, m_nIndex,nRet); 
	
	if(nRet==-3)
	{
		pMsg->sResponse.nCode = ERROR_MISSPARA;
	}
	else if(nRet==-2)
	{
		pMsg->sResponse.nCode = ERROR_BINDRESPONSE;
	}
	else if(nRet!=0)
	{
		CheckDbConnect();
		pMsg->sResponse.nCode = ERROR_DB;
	}

	return 0;
}

void CDbConnection::Dump()
{
	
}


bool CDbConnection::CheckDbConnect()
{
	bool bConnect  = true;
	if(m_pChilds.size())
	{
		vector<CDbConnection*>::iterator it = m_pChilds.begin();
		for(;it!=m_pChilds.end();++it)
		{
			if (!(*it)->CheckDbConnect())
				bConnect = false;
		}
	}
	
	return bConnect;
}

string CDbConnection::GetActualBindName(string& strBind)
{
	if(boost::istarts_with(strBind,":"))
	{
		return strBind.substr(1);
	}
	else
	{
		return strBind;
	}
}


int  CDbConnection::ExeculteCmd(SDbQueryMsg* pMsg)
{
	SV_XLOG(XLOG_DEBUG,"CDbConnection::%s\n",__FUNCTION__);
	int nRet = 0;
	
	if(m_pChilds.size())
	{
		CDBExec exec(m_pChilds);
		nRet = exec.ExecuteCmd(pMsg->sapDec,pMsg->sResponse.sapEnc,pMsg->sResponse.strSql,pMsg->errDesc);
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,sql=[%s]\n",__FUNCTION__,pMsg->sResponse.strSql.c_str());
	}
	else
	{
		CDBExec exec(this);
		nRet = exec.ExecuteCmd(pMsg->sapDec,pMsg->sResponse.sapEnc,pMsg->sResponse.strSql,pMsg->errDesc);
		SV_XLOG(XLOG_DEBUG,"CDbConnection::%s,sql=[%s]\n",__FUNCTION__,pMsg->sResponse.strSql.c_str());
	}

	return nRet;
}



