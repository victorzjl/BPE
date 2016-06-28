#include "dbexec.h"
#include "hash.h"
#include "dbconnloghelper.h"
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <boost/algorithm/string.hpp>
#include "util.h"
#include "string.h"


using namespace sdo::commonsdk;
using namespace boost;
using namespace soci;
/*
bool StringContains(const string& content, const string& sub)
{
	int nSize = content.size();
	int nIndex = 0;
	const char* szContent = content.c_str();
	const char* szSub = sub.c_str();
	int nSubSize = sub.size();
	if (nSize<=0 || nSubSize<=0)
		return false;
	
	for(;nIndex<=nSize-nSubSize; nIndex++)
	{
		if (strncasecmp(szContent+nIndex,szSub,nSubSize)==0)
			return true;
	}

	return false;
}
*/


void SplitString(vector<string>&vecValues,const string& str)
{
	
	
	const char*ptr=(const char*)str.c_str();
	unsigned int left = 0;
	unsigned int right = 0;
	unsigned int index = 0;
	unsigned int beg = 0;
	while(ptr[index])
	{
		left = 0;
		right= 0;
		
		while (ptr[index] && ptr[index]==' ')  
		{
			index++;
		}
		if (index == str.size())
			break;
		
		if (ptr[index]=='(')	left=1;	
		if (ptr[index]==')')	right=1;
		
		beg = index;

		index++;
		int length = 1;
		while(true)
		{
			while(ptr[index] && ptr[index]!=' ')
			{
				if (ptr[index]=='(')	left++;	
				if (ptr[index]==')')	right++;
				
				index++;
				length++;
			}
			if (left == right)
			{
				vecValues.push_back(str.substr(beg,length));
				break;
			}
			if (ptr[index]==0)
				break;
			index++;
			length++;
		}

	}
		
	
}

	

bool stringContains(string&str,map<string,string>&strUsed)
{
	map<string,string>::iterator it = strUsed.begin();
	for(;it!=strUsed.end();it++)
	{
		if (boost::icontains(str,it->first))
			return true;
	}
	return false;
}

size_t FindNextRightParenthesis(string & str, size_t nextPos)
{
	size_t dwSize = str.size();
	size_t dwIndex = nextPos;
	const char* ptr=(const char*)str.c_str();
	int dwLeft = 1;
	while(dwIndex<dwSize)
	{
		if (ptr[dwIndex]==')')
		{
			dwLeft--;
		}
		else if (ptr[dwIndex]=='(')
		{
			dwLeft++;
		}
		if (dwLeft==0)
		{
			return dwIndex;
		}
		dwIndex++;
	}
	return string::npos;
}

void RemoveNULLInsertParameters(string& str,map<string,string>&strUsed)
{
	size_t left_1 = str.find_first_of('(');
	if (left_1 == string::npos)
		return;
	size_t right_1 = str.find_first_of(')');
	if (right_1 == string::npos)
		return;
	string rt="";
	string prefix;
    prefix=str.substr(0,left_1);	

	string keys=str.substr(left_1+1,right_1-left_1-1);
	size_t left_2 = str.find_first_of('(',right_1);
	if (left_2 == string::npos)
		return;
	size_t right_2=FindNextRightParenthesis(str,left_2+1);
	//size_t right_2 = str.find_first_of(')',left_2);
	if (right_2 == string::npos)
		return;
		
	string aftfix = str.substr(right_2+1);
	
	string values=str.substr(left_2+1,right_2-left_2-1);
	vector<string> vecSubKeys;
	boost::split(vecSubKeys,keys,is_any_of(","),token_compress_on);
	vector<string> vecSubValues;
	boost::split(vecSubValues,values,is_any_of(","),token_compress_on);
	
	vector<string> vecKeys;
	vector<string> vecValues;
	if (vecSubKeys.size()!=vecSubValues.size())
		return;
		
	unsigned int index = 0;
	for(index = 0;index<vecSubKeys.size();index++)
	{
		if(!boost::icontains(vecSubValues[index],":"))
		{
			vecKeys.push_back(vecSubKeys[index]);
			vecValues.push_back(vecSubValues[index]);
		}
		else if (stringContains(vecSubValues[index],strUsed))
		{
			vecKeys.push_back(vecSubKeys[index]);
			vecValues.push_back(vecSubValues[index]);
		}
	}
	
	string composedKey="";
    for(index=0;index<vecKeys.size();index++)
    {
		composedKey+=vecKeys[index];
		if (index!=vecKeys.size()-1)
			composedKey+=",";
    }
	
	string composedValue="";
    for(index=0;index<vecValues.size();index++)
    {
		composedValue+=vecValues[index];
		if (index!=vecValues.size()-1)
			composedValue+=",";
    }
	
	str = prefix+"("+composedKey+") values("+composedValue+") "+aftfix;
}



bool IsVariable(const string&str)
{
	unsigned int index = 0;
	const char*pStr=(const char*)str.c_str();
	int dwCount = 0;
	for(index=0;index<str.size();index++)
	{
		if (pStr[index]==':')
		{
			dwCount++;
		}
	}
	
	if (dwCount==0)
		return false;
	else if (dwCount==1)
		return true;
	else 
	{	
		if(boost::icontains(str,"to_date"))
		{
			SV_XLOG(XLOG_DEBUG,"to_date checked %d\n",dwCount);
			dwCount-=2;
		}
		
		if (dwCount==1)
			return true;
		else 
			return false;
	}
}

int ContainLastComma(const string &str)
{
	for(int i=str.size()-1;i>=0;i--)
	{
		if (str.at(i) ==' ')
		{}
		else if (str.at(i) ==',')
		{
			return i;
		}
		else 
			return -1;
	}
	return -1;
}
void RemoveNULLUpdateParameters(string& str,map<string,string>&strUsed)
{
	SV_XLOG(XLOG_DEBUG,"RemoveNULLUpdateParameters:Before %s\n",str.c_str());
	vector<string> vecSubParams;
	//boost::split(vecSubParams,str,is_any_of(" "),token_compress_on);
	
	SplitString(vecSubParams,str);
	
	unsigned int index=0;

	unsigned int index_set = 0;
	unsigned int index_where = 0;
	
	SV_XLOG(XLOG_DEBUG,"-----\n");
		
	string rt="";
	for(;index<vecSubParams.size();index++)
	{
		if(boost::iequals(vecSubParams[index],"set"))
		{
			index_set = index;
		}
		else if(boost::iequals(vecSubParams[index],"where"))
		{
			if(index_where==0)
			{
				index_where = index;
			}
		}
	}
	
	if (index_set == 0)
		return;
		
	string strLog;
	map<string,string>::iterator it = strUsed.begin();
	for(;it!=strUsed.end();it++)
	{
		strLog+=" ";
		strLog+=it->first;
	}
	
	SV_XLOG(XLOG_DEBUG,"USED: %s\n",strLog.c_str());
	
	string strPrefix="";
	string strCore="";
	string strSuffix="";
	for(index = 0; index<=index_set;index++)
	{
		strPrefix+=vecSubParams[index];	
		if (index !=index_set)
			strPrefix+=" ";
	}
	SV_XLOG(XLOG_DEBUG,"strPrefix: %s\n",strPrefix.c_str());
	
	for(index = index_set+1; index<index_where;index++)
	{
		SV_XLOG(XLOG_DEBUG,"strcheck: %s\n",vecSubParams[index].c_str());
		
		if(!IsVariable(vecSubParams[index]))
		{	
			strCore +=vecSubParams[index];	
			SV_XLOG(XLOG_DEBUG,"not variable\n",vecSubParams[index].c_str());
		}
		else if (stringContains(vecSubParams[index],strUsed))
		{
			strCore +=vecSubParams[index];	
			SV_XLOG(XLOG_DEBUG,"used variable\n",vecSubParams[index].c_str());
		}

		strCore+=" ";
	}
	
	for(index = index_where; index<vecSubParams.size();index++)
	{
		strSuffix+=vecSubParams[index];	
		if (index+1 !=vecSubParams.size())
			strSuffix+=" ";
	}
	
	if (strCore.size())
	{
		int dwLastComma = ContainLastComma(strCore);
		SV_XLOG(XLOG_DEBUG,"coresize=%d dwLastComma[%d]\n",strSuffix.size(),dwLastComma);
		if (dwLastComma!=-1)
			strCore=strCore.substr(0,dwLastComma);
	}
	
	SV_XLOG(XLOG_DEBUG,"strSuffix: %s\n",strSuffix.c_str());
	str=strPrefix+" "+strCore+" "+strSuffix;
	SV_XLOG(XLOG_DEBUG,"RemoveNULLUpdateParameters:After %s\n",str.c_str());
}


void RemoveNULLSelectParameters(string& str,map<string,string>&strUsed)
{
	SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:Before %s\n",str.c_str());
	vector<string> vecSubParams;
	boost::split(vecSubParams,str,is_any_of(" "),token_compress_on);
	unsigned int index = 0;
	unsigned int index_where = 0;
	unsigned int index_order_group = 0;

	for(;index<vecSubParams.size();index++)
	{
		if(boost::iequals(vecSubParams[index],"where"))
		{
			if (index_where!=0)
			{
				//多个where 不支持
				SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F1After %s\n",str.c_str());
				return;
			}
			index_where = index;
		}
		else if (boost::iequals(vecSubParams[index],"group")||
			boost::iequals(vecSubParams[index],"order"))
		{
			if (index_order_group!=0)
			{
				//group order 不支持
				SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F2After %s\n",str.c_str());
				return;
			}
			
			index_order_group = index;
		}

	}
	
	
	if (index_where == 0)
	{
		SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F3After %s\n",str.c_str());
		return;
	}


	
	if (index_order_group == 0)
	{
		index_order_group = vecSubParams.size();
	}
		
	SV_XLOG(XLOG_DEBUG,"index_where=%d index_order_group=%d size=%d\n",
		index_where,
		index_order_group,
		vecSubParams.size());
		
	if (index_order_group <= index_where+1)
	{
		SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F4After %s\n",str.c_str());
		return;
	}

	unsigned int total = index_order_group - (index_where+1);
	if (total%2!=1)
	{
		SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F5After %s\n",str.c_str());
		return;
	}
	
	for(index = index_where+1;index<index_order_group;index++)
	{
		if(boost::icontains(vecSubParams[index],"("))
		{
			if(!boost::icontains(vecSubParams[index],")"))
			{
				SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:F6After %s\n",str.c_str());
				return;
			}
		}
	}

	string strCore ="";
	string strPrefix ="";
	string strSuffix ="";
	
	int dwCoreCount = 0;
	bool bContainLastCore = false;
	vector<string> strConditions;
	
	for(index=0;index<total;index+=2)
	{		
		SV_XLOG(XLOG_DEBUG,"%s\n",vecSubParams[index].c_str());
		
		if(!boost::icontains(vecSubParams[index_where+1+index],":"))
		{
			SV_XLOG(XLOG_DEBUG,"contain not :\n");
			
			strConditions.push_back(vecSubParams[index_where+1+index]);
			
			if (index!=total-1)
			{
				
				strConditions.push_back(vecSubParams[index_where+1+index+1]);
			}
			else 
			{
				bContainLastCore = true;
			}
			dwCoreCount++;
		}
		else if (stringContains(vecSubParams[index_where+1+index],strUsed))
		{
			SV_XLOG(XLOG_DEBUG,"contain strUsed\n");
			strConditions.push_back(vecSubParams[index_where+1+index]);
			
			if (index!=total-1)
			{
				strConditions.push_back(vecSubParams[index_where+1+index+1]);
			}
			else 
			{
				bContainLastCore = true;
			}
			dwCoreCount++;
		}

	}
	
	if (dwCoreCount>=1)
	{
		unsigned int dwRange = strConditions.size();
		if (!bContainLastCore)
		{
			dwRange = strConditions.size() - 1;
		}
		
		for(index=0;index<dwRange;index++)
		{
			strCore+=strConditions[index];
			if (index!=dwRange-1)
			{
				strCore+=" ";
			}
		}
	}
	
	for(index=0;index<index_where;index++)
	{
		strPrefix+=vecSubParams[index];
		
		if (index!=index_where-1)
			strPrefix+=" ";	
	}
	
	if (dwCoreCount != 0)
	{
		strPrefix+=" where";
	}
	
	for(index=index_order_group;index<vecSubParams.size();index++)
	{
		strSuffix+=vecSubParams[index];
		
		if (index!=vecSubParams.size()-1)
			strSuffix+=" ";	
	}
	
	
	str=strPrefix+" "+strCore+" "+strSuffix;

	SV_XLOG(XLOG_DEBUG,"RemoveNULLSelectParameters:After %s\n",str.c_str());
}

void RemoveNULLParameters(string& str,map<string,string>&strUsed)
{
	SV_XLOG(XLOG_DEBUG,"RemoveNULLParameters:Before %s\n",str.c_str());
	
	if(boost::algorithm::istarts_with(str,"update"))
		RemoveNULLUpdateParameters(str,strUsed);
	else if (boost::algorithm::istarts_with(str,"insert"))
		RemoveNULLInsertParameters(str,strUsed);
	else if (boost::algorithm::istarts_with(str,"select"))
		RemoveNULLSelectParameters(str,strUsed);
		
	SV_XLOG(XLOG_DEBUG,"RemoveNULLParameters:After %s\n",str.c_str());
}

unsigned long  mysql_real_escape_string(string& str)
{
	static char cSpecial[] = {'\x0','\n','\r','\\','\'','\"','\x1a'};
	//static char cReplace[] = {'0',  ' ', ' ', ' ', ' ' ,' ',  ' '};
	
	const char *from = str.c_str();
	unsigned long length = str.size();
	char *to = new char[length*2+1];
	unsigned long nToIndex = 0;
	for(unsigned long nSourceIndex = 0; nSourceIndex<length; nSourceIndex++)
	{
		bool bSpecial = false;
		unsigned long nIndex = 0;
		for(nIndex=0;nIndex<sizeof(cSpecial)/sizeof(char); nIndex++)
		{
			if (cSpecial[nIndex] ==  from[nSourceIndex])
			{
				bSpecial = true;
				break;
			}
		}
		
		if (bSpecial)
		{
			to[nToIndex++] = '\\';	
			//to[nToIndex++] = cReplace[nIndex];	
		}
		//else
		//{
			to[nToIndex++] = from[nSourceIndex];	
		//}
	}
	to[nToIndex] = 0;
	str = to;
	delete[]to;
	
	return nToIndex;
}
											   
CDBExec::CDBExec(CDbConnection*p)
{
	m_pServiceConfig = CDBServiceConfig::GetInstance();

	m_mapFunResEncode[DBP_DT_INT] = &CDBExec::ResponeEncodeInt;
	m_mapFunResEncode[DBP_DT_STRING] = &CDBExec::ResponeEncodeString;
	m_mapFunResEncode[DBP_DT_STRUCT] = &CDBExec::ResponeEncodeStruct;
	m_mapFunResEncode[DBP_DT_TLV] = &CDBExec::ResponeEncodeTlv;

	m_bIsMutiConn = false;
	m_vecConn.clear();
	m_vecConn.push_back(p);
}

CDBExec::CDBExec(vector<CDbConnection*> vecConn)
{
	m_pServiceConfig = CDBServiceConfig::GetInstance();

	m_mapFunResEncode[DBP_DT_INT] = &CDBExec::ResponeEncodeInt;
	m_mapFunResEncode[DBP_DT_STRING] = &CDBExec::ResponeEncodeString;
	m_mapFunResEncode[DBP_DT_STRUCT] = &CDBExec::ResponeEncodeStruct;
	m_mapFunResEncode[DBP_DT_TLV] = &CDBExec::ResponeEncodeTlv;

	m_bIsMutiConn = false;
	m_vecConn.clear();
	m_vecConn = vecConn;
}


CDBExec::~CDBExec()
{
	
}

int CDBExec::ExecuteCmd(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& avEnc,string& sSQLlog,SLastErrorDesc &errDesc)
{
	int nRet = 0;
	/* step 1 decode data packet */
	int nServiceId = avDec.GetServiceId();
	int nMsgId = avDec.GetMsgId();

	/* step 2 bind value to request params */
	map<string,SBindDataDesc> mapBindDataType;
	vector<ptree> vecReqParamTree;
	nRet = BindValueToRequestParams(avDec,nServiceId,nMsgId,vecReqParamTree,mapBindDataType,errDesc);
	
	if (nRet!=0)
		return nRet;

	DumBindDataDesc(mapBindDataType);

	/* step 3 actual execute */
	nRet = Execute(avDec,avEnc,nServiceId,nMsgId,mapBindDataType,vecReqParamTree,sSQLlog,errDesc);
	
	/* todo 4 */
	if(-1==nRet)
	{
		//DBERROR
		//设置数据库errorcode等,若有
		BindErrorToResponeParams(avEnc,nServiceId,nMsgId,errDesc);
	}
	
	return nRet;
}

void CDBExec::DumBindDataDesc(map<string,SBindDataDesc>& mapDesc)
{
	SV_XLOG(XLOG_DEBUG,"=============== Dump Bind Data Desc =============== \n");
	map<string,SBindDataDesc>::iterator iterMap;
	for(iterMap = mapDesc.begin(); iterMap != mapDesc.end();++iterMap)
	{
		SV_XLOG(XLOG_DEBUG,"ParamName=[%s],Type=[%d] ArrayElement[%d]\n",iterMap->first.c_str(),
			iterMap->second.eDataType,(int)iterMap->second.bIsArrayElement);
	}

	SV_XLOG(XLOG_DEBUG,"=============== End Dump =============== \n");
}

void CDBExec::DumpExeUDSQLs(vector<string>& vecSQLs)
{
	SV_XLOG(XLOG_DEBUG,"=============== Dump Exe UD SQLs =============== \n");
	vector<string>::iterator iterVec;
	for(iterVec = vecSQLs.begin(); iterVec != vecSQLs.end();++iterVec)
	{
		SV_XLOG(XLOG_DEBUG,"ExeSQL=[%s]\n",(*iterVec).c_str());
	}

	SV_XLOG(XLOG_DEBUG,"=============== End Dump =============== \n");
	
}


void CDBExec::DumpExeUDSQLs(vector<string>& vecSQLs,vector<SSqlDyncData>& vecDyncs)
{
	SV_XLOG(XLOG_DEBUG,"=============== Dump Exe UD SQLs =============== \n");
	vector<string>::iterator iterVec;
	vector<SSqlDyncData>::iterator iterVecDyncs = vecDyncs.begin();
	for(iterVec = vecSQLs.begin(); iterVec != vecSQLs.end();++iterVec,++iterVecDyncs)
	{
		SV_XLOG(XLOG_DEBUG,"ExeSQL=[%s]\n",(*iterVec).c_str());
		SSqlDyncData::iterator itPara = (*iterVecDyncs).begin();
		
		for (;itPara!=(*iterVecDyncs).end();++itPara)
		{
			string strContent = "ParamSettings: ";
			char szBuf[32]={0};
			strContent += itPara->first;  //parameter name
			strContent += "=>[";
			
			if (itPara->second.m_eDataType == BDT_INT)
			{
				vector<int>::iterator itInt = itPara->second.m_integers.begin();
				for(;itInt!=itPara->second.m_integers.end();++itInt)
				{
					if (itInt!=itPara->second.m_integers.begin())
						strContent += ",";
					snprintf(szBuf,sizeof(szBuf),"%d",*itInt);
					strContent +=szBuf;
				}
				
			}
			else if (itPara->second.m_eDataType == BDT_STRING)
			{
				vector<string>::iterator itString = itPara->second.m_strings.begin();
				for(;itString!=itPara->second.m_strings.end();++itString)
				{
					if (itString!=itPara->second.m_strings.begin())
						strContent += ",";
					
					strContent +=*itString;
				}
				
			}

			strContent += "]";
			SV_XLOG(XLOG_DEBUG,"%s\n",strContent.c_str());
		}

		
		
	}

	SV_XLOG(XLOG_DEBUG,"=============== End Dump =============== \n");
	
}


int CDBExec::Execute(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapBindDataType,vector<ptree>& vecParamValues,string& sSQLlog,SLastErrorDesc &errDesc)
{
	int nRet = 0;
	
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	if(pMsgAttri->exeSQLs.size() > 0)
	{	
		vector<string>& vecSimpleSQL = pMsgAttri->exeSQLs;
		if(vecSimpleSQL.size()==1 && boost::algorithm::istarts_with(vecSimpleSQL[0],"select"))
		{	
		// 动态绑定执行
			//nRet=SubExecuteSimpleSelectSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues, vecSimpleSQL[0],sSQLlog);
			nRet=SubExecuteDyncSimpleSelectSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues, vecSimpleSQL[0],sSQLlog,errDesc);
		}
		else
		{
		// 动态绑定执行
		//	nRet=SubExecuteUDSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues,sSQLlog);
			nRet=SubExecuteDyncUDSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues,sSQLlog,errDesc);
		}
	}
	else if(pMsgAttri->exeComposeSQLs.size() > 0)
	{
		// 动态绑定执行
		//	nRet=SubExecuteUDSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues,sSQLlog);
			nRet=SubExecuteDyncUDSQL(encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues,sSQLlog,errDesc);
	}
	else if(pMsgAttri->exeSPs.size() > 0)
	{
		nRet=SubExecuteSP(avDec,encoder,nServiceId,nMsgId,mapBindDataType,vecParamValues,sSQLlog,errDesc);
	}
	else
	{
		SV_XLOG(XLOG_ERROR,"CDBExec::%s,No Actual Execute SQL!\n",__FUNCTION__);
	}

	return nRet;
}

string CDBExec::FormatPartSql(const string & sql, SMsgAttri* pMsgAttri, map<string,SBindDataDesc>& mapParams)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::FormatPartSql\n");
	if (pMsgAttri->exePartSQLs[0].size()==0)
	{
		return sql;
	}
	string strFinal=sql;
	vector<SPartSql>::iterator it_part=pMsgAttri->exePartSQLs[0].begin();
	for(;it_part!=pMsgAttri->exePartSQLs[0].end();it_part++)
	{
		SPartSql& ss = *it_part;
		string strTmpSql="";
		if (ss.dwConditon!=OP_NOTHING)
		{
			bool bFind = mapParams.find(ss.strVariable)!=mapParams.end();
			SV_XLOG(XLOG_DEBUG,"CDBExec::FormatPartSql %s hasvalue[%d]\n",ss.strVariable.c_str(),(int)bFind);
			if ((bFind) && (ss.dwConditon==OP_NOTNULL))
			{
				strTmpSql= ss.strSql;
			}
			else if ((!bFind) && (ss.dwConditon==OP_NULL))
			{
				strTmpSql= ss.strSql;
			}
			else
			{
				strTmpSql="";
			}
		}
		else
		{
			strTmpSql= ss.strSql;
		}
		strFinal+=" "+strTmpSql;
	}
	return strFinal;
}

void CDBExec::GenerateDyncSelectSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& strSQL, string&strLogSql, SSqlDyncData& rowData)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::GenerateDyncSelectSQL\n");

	strLogSql = strSQL;
	if(vecParamValues.size() == 0 )
	{
		if(boost::icontains(strLogSql,":"))
		{
			//存在未解析参数 ，全部去掉
			map<string,string> strUsed;
			RemoveNULLParameters(strLogSql,strUsed);
			RemoveNULLParameters(strSQL,strUsed);
		}
		return;
	}
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	vector<int>::iterator iterParaCounts = pMsgAttri->paraCounts.begin();
	int paraCoutns = *iterParaCounts;
	int nParaCount = 0;
	
	strLogSql = strSQL = FormatPartSql(strSQL,pMsgAttri,mapParams);
	int nTBFactor = m_vecConn[0]->m_nTbFactor;

	map<string,SBindDataDesc>::iterator iterParam;
	ptree paramValue;
	MergeSelectParamValue(mapParams,vecParamValues,paramValue);

	ExtendSQL(strSQL, strLogSql, mapParams, paramValue);

	map<string,string> strUsed;
	for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
	{
		nParaCount++;
		SBindDataDesc& bindDesc = mapParams[iterParam->first];
		//string nameStr =  iterParam->first;
		//if(!boost::icontains(strSQL,nameStr))
		//{
			//这个sql 不用这个参数
		//	continue;
		//}

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s has Param[%s]\n",__FUNCTION__,iterParam->first.c_str());

		strUsed[iterParam->first]=iterParam->first;
			
		if(bindDesc.bSQL)
		{
			if(bindDesc.bDefault)
			{
				if(bindDesc.eDataType == BDT_STRING)
				{
					string sTmp = boost::get<string>(paramValue[iterParam->first]);	
					if (bindDesc.sDefaultVal==sTmp)
					{
						//等于默认值 脱掉引号
						string sTmpInSQL = sTmp ;
						boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
						boost::ireplace_all(strSQL,iterParam->first,sTmpInSQL);
						continue;
					}
				}
			}
			else
			{
				if(bindDesc.eDataType == BDT_STRING)
				{
		
					string sTmp = boost::get<string>(paramValue[iterParam->first]);		

					string sTmpInSQL = sTmp ;
					boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
					boost::ireplace_all(strSQL,iterParam->first,sTmpInSQL);
				}else if(bindDesc.eDataType == BDT_INT)
				{
					int dwTmp = boost::get<int>(paramValue[iterParam->first]);		
					char szTmp[32]={0};
					snprintf(szTmp,sizeof(szTmp),"%d",dwTmp);
					string sTmpInSQL = szTmp ;
					boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
					boost::ireplace_all(strSQL,iterParam->first,sTmpInSQL);
				}
				
				continue;
			}
		}
				
		if(bindDesc.eDataType == BDT_INT)
		{
			map<string,SDyncData>::iterator it_row = rowData.find(iterParam->first);
			if (it_row == rowData.end())
			{
				 SDyncData tmpSDyncData;
				
				 rowData[iterParam->first] = tmpSDyncData;
			}
			it_row = rowData.find(iterParam->first);
			SDyncData &dyData  = it_row->second;

			dyData.m_eDataType = BDT_INT;
		
			int nTmp = boost::get<int>(paramValue[iterParam->first]);

			SV_XLOG(XLOG_DEBUG,"CDBExec::GenerateDyncSelectSQL nTmp: %d\n",nTmp);
			dyData.m_integers.push_back(nTmp);

			char szTmp[64] = {0};
			snprintf(szTmp,63,"%u",nTmp);
			boost::ireplace_all(strLogSql,iterParam->first,szTmp);
			
			if(bindDesc.bIsHash)
			{

				unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
				int nTBIndex = lhashnum % nTBFactor;
				char szHash[64] = {0};
				snprintf(szHash,63,"%d",nTBIndex);
				boost::ireplace_all(strSQL,bindDesc.sHashName,szHash);
				boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
			}
			
		}
		else if(bindDesc.eDataType == BDT_STRING)
		{
			map<string,SDyncData>::iterator it_row = rowData.find(iterParam->first);
			if (it_row == rowData.end())
			{
				 SDyncData tmpSDyncData;
				
				 rowData[iterParam->first] = tmpSDyncData;
			}
			it_row = rowData.find(iterParam->first);
			SDyncData &dyData  = it_row->second;
					
			dyData.m_eDataType = BDT_STRING;					
			string sTmp = boost::get<string>(paramValue[iterParam->first]);

			mysql_real_escape_string(sTmp);
			
			SV_XLOG(XLOG_DEBUG,"CDBExec::GenerateDyncSelectSQL sTmp: %s\n",sTmp.c_str());
			
			dyData.m_strings.push_back(sTmp);

			string sTmpInSql = "'" + sTmp + "'";
			boost::ireplace_all(strLogSql,iterParam->first,sTmpInSql);
			
			if(bindDesc.bIsHash)
			{
				unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
				int nTBIndex = lhashnum % nTBFactor;
				char szHash[64] = {0};
				snprintf(szHash,63,"%d",nTBIndex);
				boost::ireplace_all(strSQL,bindDesc.sHashName,szHash);
				boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
			}
		}
		else if(bindDesc.eDataType == BDT_ARRAY)
		{
			string sTmp = boost::get<string>(paramValue[iterParam->first]);
			boost::ireplace_all(strSQL,iterParam->first,sTmp);
			boost::ireplace_all(strLogSql,iterParam->first,sTmp);
		}
	}
	
	if (nParaCount!=paraCoutns)
	{
		if(boost::icontains(strLogSql,":"))
		{
			//存在未解析参数 
			RemoveNULLParameters(strLogSql,strUsed);
			RemoveNULLParameters(strSQL,strUsed);
		}
	}
	
	return;
	
}


void CDBExec::GenerateDyncProcedureSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,vector<string>& vecOutSQLs,vector<string>& vecLogSQLs,vector<SSqlDyncData> &vecOutDyncs)
{

	SV_XLOG(XLOG_DEBUG,"CDBExec::%s \n",__FUNCTION__);

	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	map<string,SBindDataDesc>::iterator iterParam;

   
	/* get tbfactor */
	int nTBFactor = m_vecConn[0]->m_nTbFactor;

	//process stored procedure
	vector<SProcedure>& vecSQLs = pMsgAttri->exeSPs;
	vector<SProcedure>::iterator iterSQL;
	for(iterSQL = vecSQLs.begin();iterSQL!=vecSQLs.end();++iterSQL)
	{
	
		vector<ptree>::iterator iterPTree;		
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{

			string sSimpleSQL = iterSQL->sProcName;
			string strLogSql = sSimpleSQL;
			
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,SimpleSQL=[%s]\n",__FUNCTION__,sSimpleSQL.c_str());
			SSqlDyncData rowData;	
		
			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				SBindDataDesc& bindDesc = mapParams[iterParam->first];

				SV_XLOG(XLOG_DEBUG,"CDBExec::%s Param[%s]\n",__FUNCTION__,iterParam->first.c_str());

				string nameStr =  iterParam->first;
				if(!boost::icontains(sSimpleSQL,nameStr))
				{
					//这个sql 不用这个参数
					continue;
				}

				SV_XLOG(XLOG_DEBUG,"CDBExec::%s has Param[%s]\n",__FUNCTION__,iterParam->first.c_str());
					
				map<string,SDyncData>::iterator it_row = rowData.find(iterParam->first);
				if (it_row == rowData.end())
				{
					 SDyncData tmpSDyncData;
					
rowData[iterParam->first] = tmpSDyncData;
				}
				it_row = rowData.find(iterParam->first);
				SDyncData &dyData  = it_row->second;
				
				
				if(bindDesc.eDataType == BDT_INT)
				{
					dyData.m_eDataType = BDT_INT;

					int nTmp = boost::get<int>(pt[iterParam->first]);

					dyData.m_integers.push_back(nTmp);

					char szTmp[64] = {0};
					snprintf(szTmp,63,"%u",nTmp);
					boost::ireplace_all(strLogSql,iterParam->first,szTmp);
					
					if(bindDesc.bIsHash )
					{
						
						unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}

					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					dyData.m_eDataType = BDT_STRING;
					
					string sTmp = boost::get<string>(pt[iterParam->first]);

					dyData.m_strings.push_back(sTmp);
					string sTmpInSQL = "'" + sTmp + "'";
					boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
					
					if(bindDesc.bIsHash )
					{

						unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
					SV_XLOG(XLOG_ERROR,"CDBExec::%s UnknownEType: %d\n",__FUNCTION__,bindDesc.eDataType);
				}
			}
			vecLogSQLs.push_back(strLogSql);	
			vecOutSQLs.push_back(sSimpleSQL);
			vecOutDyncs.push_back(rowData);
			
		}

		
		
	}
	
	/* dump sql */
	vector<string>::iterator iterOutSQL;
	for(iterOutSQL=vecOutSQLs.begin();iterOutSQL!=vecOutSQLs.end();++iterOutSQL)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,OutSQL=[%s]!\n",__FUNCTION__,(*iterOutSQL).c_str());
	}

}

void CDBExec::GenerateDyncUDSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,vector<string>& vecOutSQLs,vector<string>& vecLogSQLs,vector<SSqlDyncData> &vecOutDyncs)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s \n",__FUNCTION__);
	

	
	//动态SQL
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	map<string,SBindDataDesc>::iterator iterParam;
	bool hasArray = false;
	for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
	{
		if (iterParam->second.bIsArrayElement)
		{
			hasArray = true;
		}
	}
		
	/* get tbfactor */
	int nTBFactor = m_vecConn[0]->m_nTbFactor;

	//process simple sql (update/delete)
	vector<string>& vecSQLs = pMsgAttri->exeSQLs;
	
	if (vecParamValues.size()==0)
	{
		//无入参
		vecLogSQLs = vecSQLs;
		vecOutSQLs = vecSQLs;
		SSqlDyncData sdd;
		vecOutDyncs.push_back(sdd);
		vector<string>::iterator iterOutSQL;
		for(iterOutSQL=vecOutSQLs.begin();iterOutSQL!=vecOutSQLs.end();++iterOutSQL)
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,OutSQL=[%s]!\n",__FUNCTION__,(*iterOutSQL).c_str());
		}
		return;
	}
	
	vector<string>::iterator iterSQL;
	vector<int>::iterator iterParaCounts = pMsgAttri->paraCounts.begin();
	//SQLs
	for(iterSQL = vecSQLs.begin();iterSQL!=vecSQLs.end();++iterSQL)
	{
		int paraCoutns = *iterParaCounts;
		iterParaCounts++;
		
		//绑定变量数组s
		vector<ptree>::iterator iterPTree;		
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{

			string sSimpleSQL = *iterSQL;
			string strLogSql = sSimpleSQL;
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,SimpleSQL=[%s]\n",__FUNCTION__,sSimpleSQL.c_str());
			SSqlDyncData rowData;	
			int nParaCount = 0;
			bool isContainArrayElement = false;
			map<string,string> strUsed;
			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				SBindDataDesc& bindDesc = mapParams[iterParam->first];

				SV_XLOG(XLOG_DEBUG,"CDBExec::%s Param[%s]\n",__FUNCTION__,iterParam->first.c_str());

				string nameStr =  iterParam->first;
				//if (sSimpleSQL.find(iterParam->first,0)==string::npos)
				//if (StringContains(sSimpleSQL,":"+iterParam->first))
				if(!boost::icontains(sSimpleSQL,nameStr))
				{
					//这个sql 不用这个参数
					continue;
				}
				if (pt.find(nameStr)==pt.end())
				{
					continue;
				}
				nParaCount++;
				if (bindDesc.bIsArrayElement)
				{
					
					SV_XLOG(XLOG_DEBUG,"ArrayElement\n");
					isContainArrayElement = true;
				}
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s has Param[%s]\n",__FUNCTION__,iterParam->first.c_str());
					
				if(bindDesc.bSQL)
				{
					if(bindDesc.bDefault)
					{
						if(bindDesc.eDataType == BDT_STRING)
						{
							string sTmp = boost::get<string>(pt[iterParam->first]);
							if (bindDesc.sDefaultVal==sTmp)
							{
								//等于默认值 脱掉引号
								string sTmpInSQL = sTmp ;
								boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
								boost::ireplace_all(sSimpleSQL,iterParam->first,sTmpInSQL);
								continue;
							}
						}
					}
					else
					{
						if(bindDesc.eDataType == BDT_STRING)
						{
							
							string sTmp = boost::get<string>(pt[iterParam->first]);
							string sTmpInSQL = sTmp ;
							boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
							boost::ireplace_all(sSimpleSQL,iterParam->first,sTmpInSQL);
						}else if(bindDesc.eDataType == BDT_INT)
						{	
							int dwTmp = boost::get<int>(pt[iterParam->first]);
							char szTmp[32]={0};
							snprintf(szTmp,sizeof(szTmp),"%d",dwTmp);
							string sTmpInSQL = szTmp ;
							boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);
							boost::ireplace_all(sSimpleSQL,iterParam->first,sTmpInSQL);
						}
						continue;
					}
				}
				
				map<string,SDyncData>::iterator it_row = rowData.find(iterParam->first);
				if (it_row == rowData.end())
				{
					 SDyncData tmpSDyncData;
					
					rowData[iterParam->first] = tmpSDyncData;
				}
				it_row = rowData.find(iterParam->first);
				SDyncData &dyData  = it_row->second;
				
				strUsed[iterParam->first]=iterParam->first;
				
				if(bindDesc.eDataType == BDT_INT)
				{
					dyData.m_eDataType = BDT_INT;

					int nTmp = boost::get<int>(pt[iterParam->first]);

					dyData.m_integers.push_back(nTmp);
					char szTmp[64] = {0};
					snprintf(szTmp,63,"%u",nTmp);
					boost::ireplace_all(strLogSql,iterParam->first,szTmp);
					
					if(bindDesc.bIsHash )
					{
						unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}

					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					dyData.m_eDataType = BDT_STRING;
					
					string sTmp = boost::get<string>(pt[iterParam->first]);

					dyData.m_strings.push_back(sTmp);

					string sTmpInSQL = "'" + sTmp + "'";
					boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);	
					
					if(bindDesc.bIsHash )
					{
						unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
					SV_XLOG(XLOG_ERROR,"CDBExec::%s UnknownEType: %d\n",__FUNCTION__,bindDesc.eDataType);
				}
			}

			SV_XLOG(XLOG_DEBUG,"CDBExec::%s try strLogSql: %s paraCoutns[%d] nParaCount[%d]\n",__FUNCTION__,strLogSql.c_str(),paraCoutns,nParaCount);
			
			//if (nParaCount>0)
			{
				if( !boost::icontains(strLogSql,":")||(paraCoutns == nParaCount) )
				{

					vecOutSQLs.push_back(sSimpleSQL);
					vecOutDyncs.push_back(rowData);
					vecLogSQLs.push_back(strLogSql);

					if (!isContainArrayElement)
					{
						/*if (hasArray)
						{
							iterPTree = vecParamValues.insert(iterPTree+1,*iterPTree);
							SV_XLOG(XLOG_DEBUG,"ADD................\n");
						}
						*/
						break;
					}
				}
				else if(boost::istarts_with(strLogSql,"update")
				|| boost::istarts_with(strLogSql,"insert")
				|| boost::istarts_with(strLogSql,"select")
				|| boost::istarts_with(strLogSql,"merge"))
				{
	
					if (!hasArray)
					{
						RemoveNULLParameters(sSimpleSQL,strUsed);
						RemoveNULLParameters(strLogSql,strUsed);
					
						vecOutSQLs.push_back(sSimpleSQL);
						vecLogSQLs.push_back(strLogSql);
						vecOutDyncs.push_back(rowData);
					}
				}
			}
			
			
		}

		
		
	}

	vector<SComposeSQL>& vecComposeSQLs = pMsgAttri->exeComposeSQLs;
	vector<SComposeSQL>::iterator iterComposeSQL;
	for(iterComposeSQL = vecComposeSQLs.begin();iterComposeSQL!=vecComposeSQLs.end();++iterComposeSQL)
	{
		vector<ptree>::iterator iterPTree;
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{
			string sComposeSQL;
			SSqlDyncData rowData;	
			GenerateComposeSQL(*iterComposeSQL,*iterPTree,sComposeSQL);

			string strLogSql = sComposeSQL;
			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				
				SBindDataDesc& bindDesc = mapParams[iterParam->first];

				//if (sComposeSQL.find(iterParam->first,0)==string::npos)
				//if (StringContains(sComposeSQL,":"+iterParam->first))
				string nameStr = iterParam->first;
				if(!boost::icontains(sComposeSQL,nameStr))
				{
					//这个sql 不用这个参数
					continue;
				}
				
				map<string,SDyncData>::iterator it_row = rowData.find(iterParam->first);
				if (it_row == rowData.end())
				{
					 SDyncData tmpSDyncData;
					
rowData[iterParam->first] = tmpSDyncData;
				}
				it_row = rowData.find(iterParam->first);
				SDyncData &dyData  = it_row->second;
				
				if(bindDesc.eDataType == BDT_INT)
				{

					dyData.m_eDataType = BDT_INT;

					int nTmp = boost::get<int>(pt[iterParam->first]);

					dyData.m_integers.push_back(nTmp);

					char szTmp[64] = {0};
					snprintf(szTmp,63,"%u",nTmp);
					boost::ireplace_all(strLogSql,iterParam->first,szTmp);
					
					if(bindDesc.bIsHash)
					{
					
						unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}
					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					dyData.m_eDataType = BDT_STRING;
					
					string sTmp = boost::get<string>(pt[iterParam->first]);

					dyData.m_strings.push_back(sTmp);
					string sTmpInSQL = "'" + sTmp + "'";
					boost::ireplace_all(strLogSql,iterParam->first,sTmpInSQL);						
					if(bindDesc.bIsHash)
					{
						unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
						boost::ireplace_all(strLogSql,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
				}
				
			}
			vecLogSQLs.push_back(strLogSql);
			vecOutSQLs.push_back(sComposeSQL);
			vecOutDyncs.push_back(rowData);
			
		}
	}	

	/* dump sql */
	vector<string>::iterator iterOutSQL;
	for(iterOutSQL=vecOutSQLs.begin();iterOutSQL!=vecOutSQLs.end();++iterOutSQL)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,OutSQL=[%s]!\n",__FUNCTION__,(*iterOutSQL).c_str());
	}
	
}

void CDBExec::GenerateUDSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,vector<string>& vecOutSQLs)
{
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	map<string,SBindDataDesc>::iterator iterParam;

	/* get tbfactor */
	int nTBFactor = m_vecConn[0]->m_nTbFactor;
	
	//process simple sql (update/delete)
	vector<string>& vecSQLs = pMsgAttri->exeSQLs;
	vector<string>::iterator iterSQL;
	for(iterSQL = vecSQLs.begin();iterSQL!=vecSQLs.end();++iterSQL)
	{
		vector<ptree>::iterator iterPTree;
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{
			string sSimpleSQL = *iterSQL;
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,SimpleSQL=[%s]\n",__FUNCTION__,sSimpleSQL.c_str());
			
			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				SBindDataDesc& bindDesc = mapParams[iterParam->first];
				
				if(bindDesc.eDataType == BDT_INT)
				{
					int nTmp = boost::get<int>(pt[iterParam->first]);

					char szTmp[64] = {0};
					snprintf(szTmp,63,"%u",nTmp);
					boost::ireplace_all(sSimpleSQL,iterParam->first,szTmp);
					
					if(bindDesc.bIsHash)
					{
						unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
					}

					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					string sTmp = boost::get<string>(pt[iterParam->first]);

					string sTmpInSQL = "'" + sTmp + "'";
					boost::ireplace_all(sSimpleSQL,iterParam->first,sTmpInSQL);
					
					if(bindDesc.bIsHash)
					{
						unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
						int nTBIndex = lhashnum % nTBFactor;
						SV_XLOG(XLOG_DEBUG,"%s hashnum = %d tb: %d tbindex= %d\n",sTmp.c_str(),
							lhashnum,nTBFactor,nTBIndex);
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
				}
			}
				
			vecOutSQLs.push_back(sSimpleSQL);
		}
	}

	vector<SComposeSQL>& vecComposeSQLs = pMsgAttri->exeComposeSQLs;
	vector<SComposeSQL>::iterator iterComposeSQL;
	for(iterComposeSQL = vecComposeSQLs.begin();iterComposeSQL!=vecComposeSQLs.end();++iterComposeSQL)
	{
		vector<ptree>::iterator iterPTree;
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{
			string sComposeSQL;
			GenerateComposeSQL(*iterComposeSQL,*iterPTree,sComposeSQL);

			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				
				SBindDataDesc& bindDesc = mapParams[iterParam->first];
				
				if(bindDesc.eDataType == BDT_INT)
				{
					int nTmp = boost::get<int>(pt[iterParam->first]);

					char szTmp[64] = {0};
					snprintf(szTmp,63,"%u",nTmp);
					boost::ireplace_all(sComposeSQL,iterParam->first,szTmp);

					if(bindDesc.bIsHash)
					{
						unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
					}
					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					string sTmp = boost::get<string>(pt[iterParam->first]);

					string sTmpInSQL = "'" + sTmp + "'";
					boost::ireplace_all(sComposeSQL,iterParam->first,sTmpInSQL);
					
					if(bindDesc.bIsHash)
					{
						unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
						int nTBIndex = lhashnum % nTBFactor;
						char szHash[64] = {0};
						snprintf(szHash,63,"%d",nTBIndex);
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
				}
				
			}

			vecOutSQLs.push_back(sComposeSQL);
			
		}
	}	

	/* dump sql */
	vector<string>::iterator iterOutSQL;
	for(iterOutSQL=vecOutSQLs.begin();iterOutSQL!=vecOutSQLs.end();++iterOutSQL)
	{
		SV_XLOG(XLOG_ERROR,"CDBExec::%s,OutSQL=[%s]!\n",__FUNCTION__,(*iterOutSQL).c_str());
	}
	
	return;
}

void CDBExec::MergeSelectParamValue(map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,ptree& pt)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	if(vecParamValues.size() == 0) 
	{
		return;
	}
	else 
	{
		pt = vecParamValues[0];
		map<string,SBindDataDesc>::iterator iterParam;
		vector<ptree>::iterator iterPTree;
		for(iterParam=mapParams.begin();iterParam!=mapParams.end();++iterParam)
		{
			if(boost::istarts_with(iterParam->first,":_"))
			{
				string sMerge;
				for(iterPTree=vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
				{
					ptree& paramValue = *iterPTree;
					if(iterParam->second.eDataType == BDT_INT)
					{
						int nTmp = boost::get<int>(paramValue[iterParam->first]);
						char szTmp[64] = {0};
						snprintf(szTmp,63,"%d",nTmp);
						sMerge += string(szTmp) + ",";
					}
					else if (iterParam->second.eDataType == BDT_STRING)
					{
						string sTmp = boost::get<string>(paramValue[iterParam->first]);
						sTmp = "'" + sTmp + "',";
						sMerge += sTmp;
					}
				}

				if(boost::iends_with(sMerge,","))
				{
					sMerge = sMerge.substr(0,sMerge.length()-1);
				}

				SBindDataDesc desc;
				desc.eDataType = BDT_ARRAY;
				desc.bIsHash = false;
				mapParams[iterParam->first] = desc;

				pt[iterParam->first] = sMerge;

				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,MergeName=[%s],MergeValue=[%s]\n",
					__FUNCTION__,iterParam->first.c_str(),sMerge.c_str());
				
			}
		}
	}
}



void CDBExec::GenerateSelectSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& strSQL)
{
	//SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	if(vecParamValues.size() == 0 )
	{
		return;
	}

	int nTBFactor = m_vecConn[0]->m_nTbFactor;

	map<string,SBindDataDesc>::iterator iterParam;
	ptree paramValue;
	MergeSelectParamValue(mapParams,vecParamValues,paramValue);
	SSqlDyncData rowData;
	
	for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
	{
		SBindDataDesc& bindDesc = mapParams[iterParam->first];
		if(bindDesc.eDataType == BDT_INT)
		{
			int nTmp = boost::get<int>(paramValue[iterParam->first]);
			char szTmp[64] = {0};
			snprintf(szTmp,63,"%u",nTmp);
			boost::ireplace_all(strSQL,iterParam->first,szTmp);

			if(bindDesc.bIsHash)
			{
				unsigned long lhashnum = average_hash(szTmp, strlen(szTmp),0);
				int nTBIndex = lhashnum % nTBFactor;
				char szHash[64] = {0};
				snprintf(szHash,63,"%d",nTBIndex);
				boost::ireplace_all(strSQL,bindDesc.sHashName,szHash);
			}
			
		}
		else if(bindDesc.eDataType == BDT_STRING)
		{
			string sTmp = boost::get<string>(paramValue[iterParam->first]);

			//char buf[1000]={0};
			//mysql_real_escape_string(buf,sTmp.c_str(),sTmp.size());
			//sTmp = buf;
			
			string sTmpInSql = "'" + sTmp + "'";
			boost::ireplace_all(strSQL,iterParam->first,sTmpInSql);

			if(bindDesc.bIsHash)
			{
				unsigned long lhashnum = average_hash(sTmp.c_str(), sTmp.length(),0);
				int nTBIndex = lhashnum % nTBFactor;
				char szHash[64] = {0};
				snprintf(szHash,63,"%d",nTBIndex);
				boost::ireplace_all(strSQL,bindDesc.sHashName,szHash);
			}
		}
		else if(bindDesc.eDataType == BDT_ARRAY)
		{
			string sTmp = boost::get<string>(paramValue[iterParam->first]);
			boost::ireplace_all(strSQL,iterParam->first,sTmp);
		}
	}
	
	return;
	
}

int CDBExec::SubExecuteDyncSimpleSelectSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string strSQL,string& sSQLLog,SLastErrorDesc& errDesc)
{
	
		SSqlDyncData rowData;
		GenerateDyncSelectSQL(nServiceId,nMsgId,mapParams,vecParamValues,strSQL,sSQLLog,rowData);
		int nRet = 0;
	
		//sSQLLog = strSQL; //log
		vector<CDbConnection*>::iterator iterConn;
		for(iterConn=m_vecConn.begin();iterConn!=m_vecConn.end();++iterConn)
		{
			vector<DBRow> fetchResult;
			CDbConnection* pConn = (CDbConnection*)(*iterConn);
			
			if(pConn->Query(strSQL,rowData,fetchResult,errDesc) == 0)
			{
				if(BindValueToResponeParams(encoder,nServiceId,nMsgId,fetchResult,fetchResult.size(),errDesc)<0)
				{
					nRet = -2;
					break;
					
				}
			}
			else
			{
				nRet = -1;
				break;
			}
		}
	
		return nRet;
	}



int CDBExec::SubExecuteSimpleSelectSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,string strSQL,string& sSQLLog)
{

	
	GenerateSelectSQL(nServiceId,nMsgId,mapParams,vecParamValues,strSQL);
	//SSqlDyncData rowData;
	//GenerateDyncSelectSQL(nServiceId,nMsgId,mapParams,vecParamValues,strSQL,rowData);
	int nRet = 0;

	sSQLLog = strSQL; //log

	vector<CDbConnection*>::iterator iterConn;
	for(iterConn=m_vecConn.begin();iterConn!=m_vecConn.end();++iterConn)
	{
		vector<DBRow> fetchResult;
		CDbConnection* pConn = (CDbConnection*)(*iterConn);
		SLastErrorDesc errDesc;
		if(pConn->Query(strSQL,fetchResult,errDesc) == 0)
		//if(pConn->Query(strSQL,rowData,fetchResult,errDesc) == 0)
		{
			if(BindValueToResponeParams(encoder,nServiceId,nMsgId,fetchResult,fetchResult.size(),errDesc)<0)
			{
				nRet = -2;
			}

		}

		else
		{
			nRet = -1;
		}
	}

	return nRet;
}

void CDBExec::GenerateUDSQLSimpleReplace(vector<string>& vecSQLs,map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog,vector<string>& vecOutSQLs)
{
	vecOutSQLs.clear();
	vector<string>::iterator iterSQL;
	for(iterSQL = vecSQLs.begin(); iterSQL!=vecSQLs.end();++iterSQL)
	{
		vector<ptree>::iterator iterPV;
		string sSQL = *iterSQL;
		for(iterPV = vecParamValues.begin(); iterPV != vecParamValues.end(); ++iterPV)
		{	
			ptree& tree = *iterPV;
			ptree::iterator iterTree;
			for(iterTree=tree.begin();iterTree!=tree.end();++iterTree)
			{
				string sParamName = iterTree->first;
				map<string,SBindDataDesc>::iterator iterParamDesc = mapParams.find(sParamName);
				if(iterParamDesc != mapParams.end())
				{
					if((iterParamDesc->second).eDataType == BDT_INT)
					{
						int nTmp = boost::get<int>(tree[iterParamDesc->first]);
						char szTmp[64] = {0};
						snprintf(szTmp,63,"%d",nTmp);
						boost::ireplace_all(sSQL,iterParamDesc->first,szTmp);
					}
					else if((iterParamDesc->second).eDataType == BDT_STRING)
					{
						string sTmp = boost::get<string>(tree[iterParamDesc->first]);
						sTmp = "'" + sTmp + "'";
						boost::ireplace_all(sSQL,iterParamDesc->first,sTmp);
					}
				}
			}
		}

		sSQLLog += sSQL + "||";
		vecOutSQLs.push_back(sSQL);		
	}
}



void CDBExec::GenerateUDSQLLog(vector<string>& vecSQLs,map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog)
{
	vector<string>::iterator iterSQL;
	for(iterSQL = vecSQLs.begin(); iterSQL!=vecSQLs.end();++iterSQL)
	{
		vector<ptree>::iterator iterPV;
		for(iterPV = vecParamValues.begin(); iterPV != vecParamValues.end(); ++iterPV)
		{	
			string sSQL = *iterSQL;
			ptree& tree = *iterPV;
			ptree::iterator iterTree;
			for(iterTree=tree.begin();iterTree!=tree.end();++iterTree)
			{
				string sParamName = iterTree->first;
				map<string,SBindDataDesc>::iterator iterParamDesc = mapParams.find(sParamName);
				if(iterParamDesc != mapParams.end())
				{
					if((iterParamDesc->second).eDataType == BDT_INT)
					{
						int nTmp = boost::get<int>(tree[iterParamDesc->first]);
						char szTmp[64] = {0};
						snprintf(szTmp,63,"%d",nTmp);
						boost::ireplace_all(sSQL,iterParamDesc->first,szTmp);
					}
					else if((iterParamDesc->second).eDataType == BDT_STRING)
					{
						string sTmp = boost::get<string>(tree[iterParamDesc->first]);
						sTmp = "'" + sTmp + "'";
						boost::ireplace_all(sSQL,iterParamDesc->first,sTmp);
					}
				}
			}

			sSQLLog += sSQL + "||";
		}
	}
}


void CDBExec::RmVectorDuplicates(vector<string>& vec)
{
	set<string> setTmp;
	vector<string>::iterator iterVec;
	for(iterVec = vec.begin(); iterVec != vec.end(); ++iterVec)
	{
		setTmp.insert(*iterVec);
	}

	vec.clear();

	set<string>::iterator iterSet;
	for(iterSet = setTmp.begin(); iterSet != setTmp.end(); ++iterSet)
	{
		vec.push_back(*iterSet);
	}

	return ;
	
}


int CDBExec::SubExecuteDyncUDSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog,SLastErrorDesc& errDesc)
{

	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);
	

	//Dynamic Sql
	vector<string> vecSQLs;
	vector<string> vecLogSQLs;
	vector<SSqlDyncData> vecDyncs;
	
	GenerateDyncUDSQL(nServiceId,nMsgId,mapParams,vecParamValues,vecSQLs,vecLogSQLs,vecDyncs);
	
	//RmVectorDuplicates(vecSQLs);

	sSQLLog = "";
	vector<string>::iterator iterExeSQL;
	for(iterExeSQL=vecLogSQLs.begin();iterExeSQL!=vecLogSQLs.end();++iterExeSQL)
	{
		sSQLLog += *iterExeSQL + "^_^_^";
	}

	
	DumpExeUDSQLs(vecSQLs,vecDyncs);
	

	int nRet = 0;
	int nAffectedRows = 0;

	vector<CDbConnection*>::iterator iterConn;
	for(iterConn=m_vecConn.begin();iterConn!=m_vecConn.end();++iterConn)
	{
		vector<DBRow> fetchResult;
		CDbConnection* pConn = (CDbConnection*)(*iterConn);

		
		if(pConn->Execute(vecSQLs,vecDyncs,nAffectedRows,errDesc) == 0)
		{
			if(BindValueToResponeParams(encoder,nServiceId,nMsgId,fetchResult,nAffectedRows,errDesc)<0)
			{
				nRet = -2;
				break;
			}
		}
		else
		{
			nRet = -1;
			break;
		}
	}

	return nRet;
}


int CDBExec::SubExecuteUDSQL(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,
	vector<ptree>& vecParamValues,string& sSQLLog)
{
	vector<string> vecSQLs;

	GenerateUDSQL(nServiceId,nMsgId,mapParams,vecParamValues,vecSQLs);
	//GenerateUDSQLLog(vecSQLs,mapParams,vecParamValues,sSQLLog);
	//vector<string> vecActualExeSQLs;
	//GenerateUDSQLSimpleReplace(vecSQLs,mapParams,vecParamValues,sSQLLog,vecActualExeSQLs);

	RmVectorDuplicates(vecSQLs);

	vector<string>::iterator iterExeSQL;
	for(iterExeSQL=vecSQLs.begin();iterExeSQL!=vecSQLs.end();++iterExeSQL)
	{
		sSQLLog += *iterExeSQL + "^_^_^";
	}

	DumpExeUDSQLs(vecSQLs);

	int nRet = 0;
	int nAffectedRows = 0;

	vector<CDbConnection*>::iterator iterConn;
	for(iterConn=m_vecConn.begin();iterConn!=m_vecConn.end();++iterConn)
	{
		vector<DBRow> fetchResult;
		CDbConnection* pConn = (CDbConnection*)(*iterConn);
		SLastErrorDesc errDesc;

		if(pConn->Execute(vecSQLs,nAffectedRows,errDesc) == 0)
		{
			if(BindValueToResponeParams(encoder,nServiceId,nMsgId,fetchResult,nAffectedRows,errDesc)<0)
			{
				nRet = -2;
			}
		}
		else
		{
			nRet = -1;
		}
	}

	return nRet;
}

int CDBExec::SubExecuteSP(CAvenueSmoothDecoder& avDec,CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapReqParams,
	vector<ptree>& vecParamValues,string& sSQLLog,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	vector<string> vecSQLs;
	vector<string> vecLogSQLs;
	vector<SSqlDyncData> vecDyncs;
	GenerateDyncProcedureSQL(nServiceId,nMsgId,mapReqParams,vecParamValues,vecSQLs,vecLogSQLs,vecDyncs);

	vector<string>::iterator iterExeSQL;
	for(iterExeSQL=vecLogSQLs.begin();iterExeSQL!=vecLogSQLs.end();++iterExeSQL)
	{
		sSQLLog += *iterExeSQL + "^_^_^";
	}
	
	map<string,SBindDataDesc> mapResParams;
	BindValueToSPResponseParams(avDec,nServiceId,nMsgId,mapResParams);

	
	int nRet = 0;
	int nAffectedRows = 0;
	
	vector<CDbConnection*>::iterator iterConn;
	for(iterConn=m_vecConn.begin();iterConn!=m_vecConn.end();++iterConn)
	{
		CDbConnection* pConn = (CDbConnection*)(*iterConn);
		map<string,SProcedureResBindData> ResBinding;
		if(pConn->Procedure(vecSQLs,vecDyncs,mapResParams,ResBinding,nAffectedRows,errDesc) == 0)
		{
			if(BindValueToProcedureResponeParams(encoder,nServiceId,nMsgId,ResBinding,nAffectedRows,errDesc)<0)
			{
				nRet = -2;
				break;
			}
		}
		else
		{
			nRet = -1;
			break;
		}
	}
	return 0;
}


int CDBExec::BindValueToProcedureResponeParams(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,
	map<string,SProcedureResBindData>& ResBinding,
	int nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s, AffectRows=[%d]\n",
					__FUNCTION__,nAffectedRows);

	int nRet = 0;


	//Process $ Binding
	vector<SParam>* pResParams = m_pServiceConfig->GetResponeParamsById(nServiceId,nMsgId);

	vector<SParam>::iterator iterResParam;
	for(iterResParam=pResParams->begin();iterResParam!=pResParams->end();++iterResParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterResParam->sTypeName);

		if(boost::istarts_with(iterResParam->sBindName,"$rowcount"))
		{
			if(EncodeSingleIntValue(encoder,nAffectedRows,pParamType)<0)
			{
				nRet = -2;
				break;
			}

			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name[$rowcount] value[%d] \n",
					__FUNCTION__,nAffectedRows);
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$errorno"))
		{
			if(EncodeSingleIntValue(encoder,errDesc.nErrNo,pParamType)<0)
			{
				nRet = -2;
				break;
			}
			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name[$errorno] value[%d] \n",
					__FUNCTION__,errDesc.nErrNo);
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$errormsg"))
		{
			if(EncodeSingleStrValue(encoder,errDesc.sErrMsg,pParamType)<0)
			{
				nRet = -2;
				break;
			}

			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name[$errormsg] value[%d] \n",
					__FUNCTION__,errDesc.sErrMsg.c_str());
		}
		
	}

	//Process Response Binding
	map<string,SProcedureResBindData>::iterator itBinder= ResBinding.begin();
	for(; itBinder != ResBinding.end(); itBinder++)
	{
		iterResParam = pResParams->begin();
		for(;iterResParam!=pResParams->end();iterResParam++)
		{
			if (iterResParam->sBindName == itBinder->first)
				break;
		}
		if (iterResParam == pResParams->end())
		{
			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name: %s not found\n",
					__FUNCTION__,itBinder->first.c_str());	
			break;
		}
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterResParam->sTypeName);
		if (pParamType == NULL)
		{
			SV_XLOG(XLOG_ERROR,"CDBExec::%s, typename: %s not found \n",
					__FUNCTION__,iterResParam->sTypeName.c_str());	
			break;
		}
		if (itBinder->second.m_eDataType == BDT_INT)
		{
			if(EncodeSingleIntValue(encoder,itBinder->second.m_integer,pParamType)<0)
			{
				nRet = -2;
				break;
			}

			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name[%s] value[%d] \n",
					__FUNCTION__,itBinder->first.c_str(),itBinder->second.m_integer);	
		}
		else if (itBinder->second.m_eDataType == BDT_STRING)
		{
			if(EncodeSingleStrValue(encoder,itBinder->second.m_string,pParamType)<0)
			{
				nRet = -2;
				break;
			}
			SV_XLOG(XLOG_ERROR,"CDBExec::%s, name[%s] value[%s] \n",
					__FUNCTION__,itBinder->first.c_str(),itBinder->second.m_string.c_str());	
		}
	}

	return nRet;
	
}


int CDBExec::BindErrorToResponeParams(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	int nRet = 0;

	vector<SParam>* pResParams = m_pServiceConfig->GetResponeParamsById(nServiceId,nMsgId);

	vector<SParam>::iterator iterResParam;
	for(iterResParam=pResParams->begin();iterResParam!=pResParams->end();++iterResParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterResParam->sTypeName);

		if(boost::istarts_with(iterResParam->sBindName,"$errorno"))
		{
			if(EncodeSingleIntValue(encoder,errDesc.nErrNo,pParamType)<0)
			{
				nRet = -2;
				SV_XLOG(XLOG_ERROR,"CDBExec::%s errorno\n",__FUNCTION__);
				break;
			}
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$errormsg"))
		{
			if(EncodeSingleStrValue(encoder,errDesc.sErrMsg,pParamType)<0)
			{
				nRet = -2;
				SV_XLOG(XLOG_ERROR,"CDBExec::%s sErrMsg\n",__FUNCTION__);
				break;
			}
		}
	}
	return nRet;
	
}

int CDBExec::BindValueToResponeParams(CAvenueSmoothEncoder& encoder,int nServiceId,int nMsgId,vector<DBRow>& fetchResult,
	int nAffectedRows,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s, OAffectRows=[%d]\n",
					__FUNCTION__,nAffectedRows);

	int nRet = 0;

	vector<SParam>* pResParams = m_pServiceConfig->GetResponeParamsById(nServiceId,nMsgId);
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s, %d\n",
						__FUNCTION__,pResParams->size());
						
	vector<SParam>::iterator iterResParam;
	for(iterResParam=pResParams->begin();iterResParam!=pResParams->end();++iterResParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterResParam->sTypeName);
		
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, %s\n",
						__FUNCTION__,iterResParam->sBindName.c_str());
					
		if(boost::istarts_with(iterResParam->sBindName,"$rowcount"))
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, AffectRows=[%d] rowcount1\n",
					__FUNCTION__,nAffectedRows);
			if(EncodeSingleIntValue(encoder,nAffectedRows,pParamType)<0)
			{
				nRet = -2;
				break;
			}
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, AffectRows=[%d] rowcount2\n",
					__FUNCTION__,nAffectedRows);
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$errorno"))
		{
			if(EncodeSingleIntValue(encoder,errDesc.nErrNo,pParamType)<0)
			{
				nRet = -2;
				break;
			}
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$errormsg"))
		{
			if(EncodeSingleStrValue(encoder,errDesc.sErrMsg,pParamType)<0)
			{
				nRet = -2;
				break;
			}
		}
		else if(boost::istarts_with(iterResParam->sBindName,"$result"))
		{
			if (fetchResult.size()==0)
				continue;

			if(boost::icontains(iterResParam->sBindName,"]["))
			{
				if(ResponeProcessResultCell(encoder,nServiceId,fetchResult,pParamType,iterResParam->sBindName)<0)
				{
					nRet = -2;
					break;
				}
			}
			else if(boost::icontains(iterResParam->sBindName,"["))
			{
				if(ResponeProcessResultField(encoder,nServiceId,fetchResult,pParamType,iterResParam->sBindName)<0)
				{
					nRet = -2;
					break;
				}
			}
			else 
			{
				if(ResponeProcessResult(encoder,nServiceId,fetchResult,pParamType)<0)
				{
					nRet = -2;
					break;
				}
			}
		}
		else
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Not Support Form,Form=[%d].\n",
					__FUNCTION__,(iterResParam->sBindName).c_str());
			nRet = -2;
			break;
		}
		
	}


	return nRet;
	
}

void CDBExec::BindValueToPTree(SServiceType* pType,SParam* pParam,SAvenueValueNode& sNode,
				ptree& pt,map<string,SBindDataDesc>& mapBindType,bool bIsArrayElement)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);


	if(pType->eType == DBP_DT_STRING)
	{
		pt[pParam->sBindName] = string((const char*)(sNode.pLoc),(sNode.nLen));	
		
		SBindDataDesc sDesc;
		sDesc.eDataType = BDT_STRING;
		sDesc.bIsHash = pParam->bIsDivide;
		sDesc.sHashName = pParam->sHashName;
		sDesc.bIsArrayElement = bIsArrayElement;
		sDesc.bSQL = pParam->bSQL;
		sDesc.bDefault = pParam->bDefault;
		sDesc.sDefaultVal = pParam->sDefaultVal;
		sDesc.bExtend = pParam->bExtend;
		mapBindType[pParam->sBindName] = sDesc;

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process String,ParamName=[%s]\n",
			__FUNCTION__,pParam->sBindName.c_str());
	}
	else if(pType->eType == DBP_DT_INT)
	{
		pt[pParam->sBindName] = ntohl(*((unsigned int*)sNode.pLoc));		

		SBindDataDesc sDesc;
		sDesc.eDataType = BDT_INT;
		sDesc.bIsHash = pParam->bIsDivide;
		sDesc.sHashName = pParam->sHashName;
		sDesc.bSQL = pParam->bSQL;
		sDesc.bIsArrayElement = bIsArrayElement;
		mapBindType[pParam->sBindName] = sDesc;

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process Int,ParamName=[%s]\n",
			__FUNCTION__,pParam->sBindName.c_str());
		
	}
	else if(pType->eType == DBP_DT_STRUCT)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process Struct\n",__FUNCTION__);

		vector<SServiceType>& vecSubType = pType->vecSubType;
		vector<SServiceType>::iterator iterSubType;

		vector<string> vecSubParams;
		//boost::erase_all(pParam->sBindName," ");
		boost::split(vecSubParams,pParam->sBindName,is_any_of(","),token_compress_on);

		vector<DBStructElement> vecValues;
		GetStructValues(pType,sNode,vecValues);

		unsigned int nSubParamIndex;
		for(nSubParamIndex = 0; nSubParamIndex < vecSubParams.size(); nSubParamIndex++)
		{	
			SServiceType& sSubParamType = vecSubType[nSubParamIndex];
			if(sSubParamType.eType == DBP_DT_STRING)
			{
				string str = boost::get<string>(vecValues[nSubParamIndex]);
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,BindParamName=[%s],Value=[%s]\n",
					__FUNCTION__,(vecSubParams[nSubParamIndex]).c_str(),str.c_str());
				
				if (m_eDBType == DBT_MYSQL)
				{
				
					mysql_real_escape_string
					(
					 str
					);
					
					pt[vecSubParams[nSubParamIndex]] = str;
				}
				else
				{
					pt[vecSubParams[nSubParamIndex]] = str;
				}
				
				SBindDataDesc sDesc;
				sDesc.eDataType = BDT_STRING;
				sDesc.bIsHash = false;
				sDesc.sHashName = "";
				sDesc.bIsArrayElement = bIsArrayElement;
				mapBindType[vecSubParams[nSubParamIndex]] = sDesc;
			}
			else if (sSubParamType.eType == DBP_DT_INT)
			{
				pt[vecSubParams[nSubParamIndex]] = ntohl((boost::get<int>(vecValues[nSubParamIndex])));

				SBindDataDesc sDesc;
				sDesc.eDataType = BDT_INT;
				sDesc.bIsHash = false;
				sDesc.sHashName = "";
				sDesc.bIsArrayElement = bIsArrayElement;
				mapBindType[vecSubParams[nSubParamIndex]] = sDesc;
			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Not Support Struct Element Type,Type=[%d].\n",
					__FUNCTION__,sSubParamType.eType);
			}
		}
	}
}


void CDBExec::BindValueToSPResponseParams(CAvenueSmoothDecoder& decoder,int nServiceId,int nMsgId,
	map<string,SBindDataDesc>& mapBindDataType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,ServiceId=[%d],MsgId=[%d]\n",__FUNCTION__,nServiceId,nMsgId);
	vector<SParam>* pResParams = m_pServiceConfig->GetResponeParamsById(nServiceId,nMsgId);
	vector<SParam>::iterator iterResParam;

	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,ParamSize=[%d]\n",__FUNCTION__,pResParams->size());
	for(iterResParam = pResParams->begin(); iterResParam!= pResParams->end(); ++iterResParam)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,@@Response Param = [%s]\n",__FUNCTION__,iterResParam->sBindName.c_str());
	}

	for(iterResParam=pResParams->begin();iterResParam!=pResParams->end();++iterResParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterResParam->sTypeName);

		if (iterResParam->sTypeName.size()>=1 && memcmp(iterResParam->sBindName.c_str(),"$",1)==0)
		{
			//$rowcount 之类的过滤
			continue;
		}

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,@@BindName=[%s],ParamTypeName=[%s]\n",
			__FUNCTION__,iterResParam->sBindName.c_str(),iterResParam->sTypeName.c_str());
			
		if(pParamType->eType == DBP_DT_STRING)
		{

			SBindDataDesc sDesc;
			sDesc.eDataType = BDT_STRING;
			sDesc.bIsHash = iterResParam->bIsDivide;
			sDesc.sHashName = iterResParam->sHashName;
	
			mapBindDataType[iterResParam->sBindName] = sDesc;

			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process String,ParamName=[%s]\n",
				__FUNCTION__,iterResParam->sBindName.c_str());
		}
		else if(pParamType->eType == DBP_DT_INT)
		{
			
			SBindDataDesc sDesc;
			sDesc.eDataType = BDT_INT;
			sDesc.bIsHash = iterResParam->bIsDivide;
			sDesc.sHashName = iterResParam->sHashName;
			mapBindDataType[iterResParam->sBindName] = sDesc;

			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process Int,ParamName=[%s]\n",
				__FUNCTION__,iterResParam->sBindName.c_str());
			
		}
			
		
	}
}


int CDBExec::BindValueToRequestParams(CAvenueSmoothDecoder& decoder,int nServiceId,int nMsgId,vector<ptree>& vecParamValues,
	map<string,SBindDataDesc>& mapBindDataType,SLastErrorDesc& errDesc)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,ServiceId=[%d],MsgId=[%d]\n",__FUNCTION__,nServiceId,nMsgId);
	vector<SParam>* pReqParams = m_pServiceConfig->GetRequestParamsById(nServiceId,nMsgId);
	vector<SParam>::iterator iterReqParam;

	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,ParamSize=[%d]\n",__FUNCTION__,pReqParams->size());
	for(iterReqParam = pReqParams->begin(); iterReqParam!= pReqParams->end(); ++iterReqParam)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,@@Request Param = [%s]\n",__FUNCTION__,iterReqParam->sBindName.c_str());
	}

	bool isUpdate = false;
	
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);
	if(pMsgAttri->exeSQLs.size() > 0)
	{	
		if(boost::algorithm::istarts_with(pMsgAttri->exeSQLs[0],"update"))
			isUpdate = true;
		else if (boost::algorithm::istarts_with(pMsgAttri->exeSQLs[0],"insert"))
			isUpdate = true;
		else if (boost::algorithm::istarts_with(pMsgAttri->exeSQLs[0],"select"))
			isUpdate = true;
		else if (boost::algorithm::istarts_with(pMsgAttri->exeSQLs[0],"merge"))
			isUpdate = true;
		SV_XLOG(XLOG_DEBUG,"***%s\n",pMsgAttri->exeSQLs[0].c_str());
	}
	
	SV_XLOG(XLOG_DEBUG,"***isUpdate = %d\n", (int)isUpdate);
	
	//round 1 : process array 
	bool bIsFirstArray = true;
	for(iterReqParam=pReqParams->begin();iterReqParam!=pReqParams->end();++iterReqParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterReqParam->sTypeName);

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,First Round,@@BindName=[%s],ParamTypeName=[%s]\n",
			__FUNCTION__,iterReqParam->sBindName.c_str(),iterReqParam->sTypeName.c_str());

		
		if(pParamType->eType == DBP_DT_ARRAY)
		{
			SServiceType* pElementType = m_pServiceConfig->GetSingleServiceType(nServiceId,pParamType->sElementName);

			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Process Array,ElementName=[%s]\n",
				__FUNCTION__,pParamType->sElementName.c_str());
			
			vector<SAvenueValueNode> vecValueNode;

			decoder.GetValues(pElementType->nTypeCode,vecValueNode);
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Node Values,ElementName=[%s],size=[%d]\n",
				__FUNCTION__,pElementType->sTypeName.c_str(),vecValueNode.size());

			
			if(vecValueNode.size()==0)
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Value Error,TypeCode=[%d]\n",__FUNCTION__,pParamType->nTypeCode);
			
				errDesc.sErrMsg = pElementType->sTypeName+" MISS";
				if (!isUpdate)
					return -3;
				else
					continue;
			}
			
			vector<SAvenueValueNode>::iterator iterValueNode;
			if(!bIsFirstArray && pMsgAttri->bMultArray)
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Not First Array,TypeName=[%s],ParamName=[%s]\n",
					__FUNCTION__,pElementType->sTypeName.c_str(),(&(*iterReqParam))->sBindName.c_str());

				if(vecValueNode.size() > vecParamValues.size())
				{
					continue;
				}
				vector<ptree>::iterator iterPTree;
				for(iterValueNode = vecValueNode.begin(),iterPTree = vecParamValues.begin(); 
					(iterValueNode != vecValueNode.end())&&(iterPTree!=vecParamValues.end()); 
					++iterValueNode,++iterPTree)
				{
					BindValueToPTree(pElementType,&(*iterReqParam),*iterValueNode,*iterPTree,mapBindDataType);	
				}
				
			}
			else
			
			{
				for(iterValueNode = vecValueNode.begin(); iterValueNode != vecValueNode.end(); ++iterValueNode)
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Array,TypeName=[%s],ParamName=[%s]\n",
					__FUNCTION__,pElementType->sTypeName.c_str(),(&(*iterReqParam))->sBindName.c_str());

					ptree ptreeParam;
					BindValueToPTree(pElementType,&(*iterReqParam),*iterValueNode,ptreeParam,mapBindDataType,true);
					bIsFirstArray = false;
					vecParamValues.push_back(ptreeParam);
				}
			}
		}
	}

	//round 2 : process other types	
	for(iterReqParam=pReqParams->begin();iterReqParam!=pReqParams->end();++iterReqParam)
	{
		SServiceType* pParamType = m_pServiceConfig->GetSingleServiceType(nServiceId,iterReqParam->sTypeName);

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Second Round,@@BindName=[%s],ParamTypeName=[%s]\n",
			__FUNCTION__,iterReqParam->sBindName.c_str(),iterReqParam->sTypeName.c_str());

		
		if(pParamType->eType == DBP_DT_ARRAY)
		{
			continue;
		}

		if(vecParamValues.size() == 0)
		{
			ptree ptreeParam;
			SAvenueValueNode sNode;

			if(decoder.GetValue(pParamType->nTypeCode,&(sNode.pLoc),&(sNode.nLen))<0)
			{
			
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Value Error,TypeCode=[%d]\n",__FUNCTION__,pParamType->nTypeCode);
				
				if (iterReqParam->bDefault)
				{
					int nDefault;
					if(pParamType->eType == DBP_DT_INT)
					{
						nDefault = atoi(iterReqParam->sDefaultVal.c_str());
						nDefault = htonl(nDefault);
						sNode.pLoc=&nDefault;
						sNode.nLen=4;
					}
					else
					{
						sNode.pLoc=(void*)iterReqParam->sDefaultVal.c_str();
						sNode.nLen=iterReqParam->sDefaultVal.size();
					}
					BindValueToPTree(pParamType,&(*iterReqParam),sNode,ptreeParam,mapBindDataType);	
				}
				else
				{
					errDesc.sErrMsg = iterReqParam->sTypeName+" MISS";
					if (!isUpdate)
						return -3;
					else
						continue;
				}
				
			}
			else
			{
				BindValueToPTree(pParamType,&(*iterReqParam),sNode,ptreeParam,mapBindDataType);	
			}
			
			vecParamValues.push_back(ptreeParam);
			
		}
		else
		{
			vector<ptree>::iterator iterPTree;
			for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
			{
				SAvenueValueNode sNode;
				if(decoder.GetValue(pParamType->nTypeCode,&(sNode.pLoc),&(sNode.nLen))<0)
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Value Error,TypeCode=[%d]\n",__FUNCTION__,pParamType->nTypeCode);
				
					if (iterReqParam->bDefault)
					{
						int nDefault;
						if(pParamType->eType == DBP_DT_INT)
						{
							nDefault = atoi(iterReqParam->sDefaultVal.c_str());
							nDefault = htonl(nDefault);
							sNode.pLoc=&nDefault;
							sNode.nLen=4;
						}
						else
						{
							sNode.pLoc=(void*)iterReqParam->sDefaultVal.c_str();
							sNode.nLen=iterReqParam->sDefaultVal.size();
						}
						BindValueToPTree(pParamType,&(*iterReqParam),sNode,*iterPTree,mapBindDataType);	
					}
					else
					{
						errDesc.sErrMsg = iterReqParam->sTypeName+" MISS";
						if (!isUpdate)
							return -3;
						else
							continue;
					}
				}
				else
				{
					BindValueToPTree(pParamType,&(*iterReqParam),sNode,*iterPTree,mapBindDataType);	
				}
			}
		}
	}

	return 0;
}


void CDBExec::GenerateExecuteSQL(int nServiceId,int nMsgId, map<string,SBindDataDesc>& mapParams,vector<ptree>& vecParamValues,vector<string>& vecOutSQL)
{
	SMsgAttri* pMsgAttri = m_pServiceConfig->GetMsgAttriById(nServiceId,nMsgId);

	map<string,SBindDataDesc>::iterator iterParam;

	vector<string>& vecSQLs = pMsgAttri->exeSQLs;
	vector<string>::iterator iterSQL;
	for(iterSQL = vecSQLs.begin();iterSQL!=vecSQLs.end();++iterSQL)
	{
		vector<ptree>::iterator iterPTree;
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{
			string sSimpleSQL = *iterSQL;
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,SimpleSQL=[%s]\n",__FUNCTION__,sSimpleSQL.c_str());
			
			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				SBindDataDesc& bindDesc = mapParams[iterParam->first];
				
				if(bindDesc.eDataType == BDT_INT)
				{
					int nTmp = boost::get<int>(pt[iterParam->first]);
					char szTmp[64] = {0};
					snprintf(szTmp,63,"%d",nTmp);
					boost::ireplace_all(sSimpleSQL,iterParam->first,szTmp);

					if(bindDesc.bIsHash)
					{
						//do process hash
						char szHash[64] = {0};
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
					}
					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					string sTmp = boost::get<string>(pt[iterParam->first]);
					sTmp = "'" + sTmp + "'";
					boost::ireplace_all(sSimpleSQL,iterParam->first,sTmp);

					if(bindDesc.bIsHash)
					{
						//do process hash
						char szHash[64] = {0};
						boost::ireplace_all(sSimpleSQL,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
				}
			}
				
			vecOutSQL.push_back(sSimpleSQL);
		}
	}

	vector<SComposeSQL>& vecComposeSQLs = pMsgAttri->exeComposeSQLs;
	vector<SComposeSQL>::iterator iterComposeSQL;
	for(iterComposeSQL = vecComposeSQLs.begin();iterComposeSQL!=vecComposeSQLs.end();++iterComposeSQL)
	{
		vector<ptree>::iterator iterPTree;
		for(iterPTree = vecParamValues.begin();iterPTree!=vecParamValues.end();++iterPTree)
		{
			string sComposeSQL;
			GenerateComposeSQL(*iterComposeSQL,*iterPTree,sComposeSQL);

			for(iterParam = mapParams.begin(); iterParam!=mapParams.end();++iterParam)
			{
				ptree& pt = *iterPTree;
				
				SBindDataDesc& bindDesc = mapParams[iterParam->first];
				
				if(bindDesc.eDataType == BDT_INT)
				{
					int nTmp = boost::get<int>(pt[iterParam->first]);
					char szTmp[64] = {0};
					snprintf(szTmp,63,"%d",nTmp);
					boost::ireplace_all(sComposeSQL,iterParam->first,szTmp);

					if(bindDesc.bIsHash)
					{
						//do process hash
						char szHash[64] = {0};
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
					}
					
				}
				else if(bindDesc.eDataType == BDT_STRING)
				{
					string sTmp = boost::get<string>(pt[iterParam->first]);
					sTmp = "'" + sTmp + "'";
					boost::ireplace_all(sComposeSQL,iterParam->first,sTmp);

					if(bindDesc.bIsHash)
					{
						//do process hash
						char szHash[64] = {0};
						boost::ireplace_all(sComposeSQL,bindDesc.sHashName,szHash);
					}
				}
				else
				{
					//error
				}

				
				
			}

			vecOutSQL.push_back(sComposeSQL);
			
		}
	}	

	/* dump */
	vector<string>::iterator iterDump;
	for(iterDump = vecOutSQL.begin(); iterDump!=vecOutSQL.end(); ++iterDump)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,ExeSQL=[%s]\n",__FUNCTION__,(*iterDump).c_str());
	}

	return;
	
}

void CDBExec::GenerateComposeSQL(SComposeSQL& composeSQL,ptree& pt,string& sOutSQL)
{	
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Prefix=[%s],Suffix=[%s]\n",__FUNCTION__,composeSQL.sPrefix.c_str(),
		composeSQL.sSuffix.c_str());

	vector<string> vecDetailActual;

	vector<string>& vecDetail = composeSQL.vecDetail;
	vector<string>::iterator iterDetail;
	for(iterDetail = vecDetail.begin(); iterDetail!=vecDetail.end();++iterDetail)
	{
		string sDetail = *iterDetail;
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Detail=[%s]\n",__FUNCTION__,(*iterDetail).c_str());
		
		boost::ireplace_all(sDetail," ","");
		/* xx=:1 */
		vector<string> vecParm;
		boost::split(vecParm,sDetail,is_any_of("="),token_compress_on);
		if(vecParm.size()>=2)
		{
			string sParamName = vecParm[1];
			boost::ireplace_all(sParamName,",","");
			ptree::iterator iterPT = pt.find(sParamName);
			if(iterPT!=pt.end())
			{
				vecDetailActual.push_back(*iterDetail);
			}
		}
	}

	if(vecDetailActual.size() == 0)
	{
		sOutSQL = "";
	}
	else
	{
		sOutSQL = composeSQL.sPrefix;
		vector<string>::iterator iterSQL;
		for(iterSQL=vecDetailActual.begin();iterSQL!=vecDetailActual.end();++iterSQL)
		{
			sOutSQL+= " " + (*iterSQL) + ",";
		}
		if(boost::iends_with(sOutSQL,","))
		{
			sOutSQL = sOutSQL.substr(0,sOutSQL.length()-1);
		}

		sOutSQL += " " + composeSQL.sSuffix;
	}

	return;
}

int CDBExec::EncodeSingleStrValue(CAvenueSmoothEncoder& encoder,string sValue,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	if(pParamType->eType == DBP_DT_STRING)
	{
		encoder.SetValue(pParamType->nTypeCode,sValue);
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,String Type Only, Type=[%d].\n",
			__FUNCTION__,pParamType->eType);
	}

	return 0;
}

int CDBExec::EncodeSingleIntValue(CAvenueSmoothEncoder& encoder,int nValue,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	if(pParamType->eType == DBP_DT_INT)
	{
		encoder.SetValue(pParamType->nTypeCode,nValue);
	}
	else if(pParamType->eType == DBP_DT_STRING)
	{
		char szValue[64] = {0};
		snprintf(szValue,63,"%d",nValue);
		encoder.SetValue(pParamType->nTypeCode,string(szValue));
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Int Just Support Int Or String, Type=[%d].\n",
			__FUNCTION__,pParamType->eType);
	}

	return 0;
}


int CDBExec::ResponeProcessResult(CAvenueSmoothEncoder& encoder,int nServiceId,vector<DBRow>& fetchResult,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	if(fetchResult.size() == 0)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, No Fetch Result.\n",__FUNCTION__);
		return 0;
	}

	if(pParamType->eType == DBP_DT_INT ||pParamType->eType == DBP_DT_STRING || pParamType->eType == DBP_DT_STRUCT || pParamType->eType == DBP_DT_TLV )
	{
		if ((this->*m_mapFunResEncode[pParamType->eType])(encoder,fetchResult[0],pParamType) < 0)
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Encode Respone Error.\n",__FUNCTION__);
		}
	}
	else if(pParamType->eType == DBP_DT_TLV_ARRAY)
	{
		SServiceType* pElementType = m_pServiceConfig->GetSingleServiceType(nServiceId,pParamType->sElementName);

		unsigned int nRowIndex;
		sdo::sap::CSapTLVBodyEncoder retEnc;
		
		for(nRowIndex = 0; nRowIndex<fetchResult.size(); nRowIndex++)
		{
			if(pElementType->eType == DBP_DT_TLV)
			{
				if(ResponeEncodeTlvArrayElement(retEnc,fetchResult[nRowIndex],pElementType)<0)
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Encode DBP_DT_TLV_ARRAY Respone Error.\n",__FUNCTION__);
					return -1;
				}
			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Tlv-Array Element Not Support Type,Type=[%d].\n",
					__FUNCTION__,pElementType->eType);
				return -1;
			}
		}
		
		encoder.SetValue(pParamType->nTypeCode,(const void*)retEnc.GetBuffer(),(unsigned int)retEnc.GetLen());
	}
	else if(pParamType->eType == DBP_DT_ARRAY)
	{
		SServiceType* pElementType = m_pServiceConfig->GetSingleServiceType(nServiceId,pParamType->sElementName);

		unsigned int nRowIndex;
		for(nRowIndex = 0; nRowIndex<fetchResult.size(); nRowIndex++)
		{
			if(pElementType->eType == DBP_DT_INT ||pElementType->eType == DBP_DT_STRING || pElementType->eType == DBP_DT_STRUCT || pElementType->eType == DBP_DT_TLV)
			{
				if((this->*m_mapFunResEncode[pElementType->eType])(encoder,fetchResult[nRowIndex],pElementType)<0)
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Encode Respone Error.\n",__FUNCTION__);
					return -1;
				}
			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Array Element Not Support Type,Type=[%d].\n",
					__FUNCTION__,pElementType->eType);
				return -1;
			}
		}
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Not Support Type,Type=[%d].\n",
					__FUNCTION__,pParamType->eType);
		return -1;
	}

	return 0;
}

int CDBExec::ResponeProcessResultField(CAvenueSmoothEncoder& encoder,int nServiceId,
								vector<DBRow>& fetchResult,SServiceType* pParamType,string sBindName)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);
	//process $result[i]
	boost::ireplace_all(sBindName,"$result","");
	boost::ireplace_all(sBindName,"[","");
	boost::ireplace_all(sBindName,"]","");

	unsigned int nRowIndex = atoi(sBindName.c_str());
	if (pParamType->eType == DBP_DT_ARRAY)
	{
		SServiceType*pSubType = CDBServiceConfig::GetInstance()->GetSingleServiceType(nServiceId,pParamType->sElementName);
		if (pSubType==NULL)
		{
			return -1;
		}
		
		for(unsigned int dwIndex=0; dwIndex<fetchResult.size(); dwIndex++)
		{
			DBRow& row=fetchResult[dwIndex];
			int nRet;
			if (pSubType->eType==DBP_DT_STRING)
			{
				try
				{
					string sTmp = boost::get<string>(row[nRowIndex]);
					encoder.SetValue(pSubType->nTypeCode,sTmp);
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String Value Error,Msg=[%s]\n",__FUNCTION__,e.what());
					return -1;
				}
			}
			else
			{
				int nTmp = 0;
				if( (nRet = GetIntValueFromVariant(row[nRowIndex],nTmp)) == 0)
				{
					encoder.SetValue(pSubType->nTypeCode,nTmp);
				}
				else
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Int Value Error\n",__FUNCTION__);
					return -1;
				}
			}
			 
		}
	}
	else
	{
		if(nRowIndex >= fetchResult.size())
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Invalid Row Index, RowIndex=[%d], ResultSize=[%d].\n",
						__FUNCTION__,nRowIndex,fetchResult.size());
			return -1;
		}

		if(pParamType->eType == DBP_DT_STRUCT||
			pParamType->eType == DBP_DT_TLV)
		{
			(this->*m_mapFunResEncode[pParamType->eType])(encoder,fetchResult[nRowIndex],pParamType);
		}
		else
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Form $result[i] Just Support Type_Struct, Type=[%d].\n",
				__FUNCTION__,pParamType->eType);
			return -1;
		}
	}

	return 0;
}



int CDBExec::ResponeProcessResultCell(CAvenueSmoothEncoder& encoder,int nServiceId,
								vector<DBRow>& fetchResult,SServiceType* pParamType,string sBindName)
{
	//process $result[i][j]
	int nRet = 0;
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s,BindName=[%s]\n",__FUNCTION__,sBindName.c_str());
	boost::ireplace_all(sBindName,"$result","");
	boost::ireplace_all(sBindName,"[","");

	vector<string> vecIndex;
	boost::split(vecIndex,sBindName,is_any_of("]"),token_compress_on);
	if(vecIndex.size() < 2)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,size=%d\n",__FUNCTION__,vecIndex.size());
		return -1;
	}

	unsigned int nRowIndex = atoi(vecIndex[0].c_str());
	unsigned int nFieldIndex = atoi(vecIndex[1].c_str());

	if(nRowIndex >= fetchResult.size())
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Invalid Row Index, RowIndex=[%d], ResultSize=[%d].\n",
					__FUNCTION__,nRowIndex,fetchResult.size());

		return -1;
	}

	DBRow& dbRow = fetchResult[nRowIndex];
	if(nFieldIndex >= dbRow.size())
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Invalid Field Index, FieldIndex=[%d], FieldSize=[%d].\n",
					__FUNCTION__,nFieldIndex,dbRow.size());

		return -1;
	}

	if(pParamType->eType == DBP_DT_INT)
	{
		int nTmp = 0;
		if( (nRet = GetIntValueFromVariant(dbRow[nFieldIndex],nTmp)) == 0)
		{
			encoder.SetValue(pParamType->nTypeCode,nTmp);
		}
		else
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Int Value Error\n",__FUNCTION__);
			return nRet;
		}
	}
	else if (pParamType->eType == DBP_DT_STRING)
	{
		try
		{
			string sTmp = boost::get<string>(dbRow[nFieldIndex]);
			encoder.SetValue(pParamType->nTypeCode,sTmp);

			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,AvenueLen=[%d]\n",__FUNCTION__,encoder.GetLen());
		}
		catch (std::exception const & e)    
		{       
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String Value Error,Msg=[%s]\n",__FUNCTION__,e.what());
			return -1;
		}
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Form $result[i][j] Just Support Type_Int,Type_String, Type=[%d].\n",
			__FUNCTION__,pParamType->eType);
		return -1;
	}

	return 0;

}

int CDBExec::ResponeEncodeInt(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);
	int nRet = 0;
	int nTmp = 0;
	if( (nRet = GetIntValueFromVariant(row[0],nTmp)) == 0)
	{
		encoder.SetValue(pParamType->nTypeCode,nTmp);
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Int Value Error\n",__FUNCTION__);
	}
	return nRet;
}

int CDBExec::ResponeEncodeString(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);
	int nRet = 0;
	try
	{
		string sTmp = boost::get<string>(row[0]);
		encoder.SetValue(pParamType->nTypeCode,sTmp);
	}
	catch (std::exception const & e)    
	{       
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String Value Error,Msg=[%s]\n",__FUNCTION__,e.what());
		nRet = -1;
	}

	return nRet;
}

int CDBExec::ResponeEncodeTlvArrayElement(sdo::sap::CSapTLVBodyEncoder &retEnc,DBRow& row,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	int nRet = 0;
	
	vector<SServiceType>& vecSubType = pParamType->vecSubType;
	sdo::sap::CSapTLVBodyEncoder enc;
	do
	{

		for(unsigned int nStructIndex=0;nStructIndex<vecSubType.size();nStructIndex++)
		{
			if(vecSubType[nStructIndex].eType==DBP_DT_STRING)
			{
				string sTmp = "";
				try
				{
					sTmp = boost::get<string>(row[nStructIndex]);	
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get String Value Error,nCode[%d], Msg=[%s]\n",__FUNCTION__,
					vecSubType[nStructIndex].nTypeCode,
					e.what());
					nRet = -1;
					break;
				}
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Set String nCode[%d] Value[%s]\n",__FUNCTION__,vecSubType[nStructIndex].nTypeCode,sTmp.c_str());
					
				enc.SetValue(vecSubType[nStructIndex].nTypeCode,sTmp);
			}
			else if(vecSubType[nStructIndex].eType==DBP_DT_INT)
			{
				int nTmp = 0;
				try
				{
					nTmp= boost::get<int>(row[nStructIndex]);
				}
				catch (boost::bad_get const & e)
				{
					try
					{
						string sTmp = boost::get<string>(row[nStructIndex]);
						nTmp = atoi(sTmp.c_str());
					}
					catch (std::exception const & e)    
					{       
						SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get String->Int Value Error,nCode[%d],Msg=[%s]\n",__FUNCTION__,
						vecSubType[nStructIndex].nTypeCode,
						e.what());
						nRet = -1;
					}
					
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get Int Value Error,nCode[%d],Msg=[%s]\n",__FUNCTION__,
					vecSubType[nStructIndex].nTypeCode,
					e.what());
					nRet = -1;
					break;
				}
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Set Int nCode[%d] Value[%d]\n",__FUNCTION__,vecSubType[nStructIndex].nTypeCode,nTmp);
				
				enc.SetValue(vecSubType[nStructIndex].nTypeCode,nTmp);
			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Compute Struct Size Error,"\
					"Struct Element Not Support Type,Type=[%d].\n",
					__FUNCTION__,vecSubType[nStructIndex].eType);
				nRet = -1;
				break;
			}
		}
		
		if(nRet==0)
		{
			retEnc.SetValue(pParamType->nTypeCode,(const void*)enc.GetBuffer(),(unsigned int)enc.GetLen());
		}

	} while(0);

	return nRet;
	
}

int CDBExec::ResponeEncodeTlv(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	int nRet = 0;
	
	vector<SServiceType>& vecSubType = pParamType->vecSubType;
	sdo::sap::CSapTLVBodyEncoder enc;
	
	do
	{

		for(unsigned int nStructIndex=0;nStructIndex<vecSubType.size();nStructIndex++)
		{
			if(vecSubType[nStructIndex].eType==DBP_DT_STRING)
			{
				string sTmp = "";
				try
				{
					sTmp = boost::get<string>(row[nStructIndex]);	
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get String Value Error,nCode[%d], Msg=[%s]\n",__FUNCTION__,
					vecSubType[nStructIndex].nTypeCode,
					e.what());
					nRet = -1;
					break;
				}
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Set String nCode[%d] Value[%s]\n",__FUNCTION__,vecSubType[nStructIndex].nTypeCode,sTmp.c_str());
					
				enc.SetValue(vecSubType[nStructIndex].nTypeCode,sTmp);
			}
			else if(vecSubType[nStructIndex].eType==DBP_DT_INT)
			{
				int nTmp = 0;
				try
				{
					nTmp= boost::get<int>(row[nStructIndex]);
				}
				catch (boost::bad_get const & e)
				{
					try
					{
						string sTmp = boost::get<string>(row[nStructIndex]);
						nTmp = atoi(sTmp.c_str());
					}
					catch (std::exception const & e)    
					{       
						SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get String->Int Value Error,nCode[%d],Msg=[%s]\n",__FUNCTION__,
						vecSubType[nStructIndex].nTypeCode,
						e.what());
						nRet = -1;
					}
					
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_ERROR,"CDBExec::%s,Get Int Value Error,nCode[%d],Msg=[%s]\n",__FUNCTION__,
					vecSubType[nStructIndex].nTypeCode,
					e.what());
					nRet = -1;
					break;
				}
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Set Int nCode[%d] Value[%d]\n",__FUNCTION__,vecSubType[nStructIndex].nTypeCode,nTmp);
				
				enc.SetValue(vecSubType[nStructIndex].nTypeCode,nTmp);
			}
			else
			{
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Compute Struct Size Error,"\
					"Struct Element Not Support Type,Type=[%d].\n",
					__FUNCTION__,vecSubType[nStructIndex].eType);
				nRet = -1;
				break;
			}
		}
		
		if(nRet==0)
		{
			encoder.SetValue(pParamType->nTypeCode,(const void*)enc.GetBuffer(),(unsigned int)enc.GetLen());
		}

	} while(0);

	return nRet;
	
}


int CDBExec::ResponeEncodeStruct(CAvenueSmoothEncoder& encoder,DBRow& row,SServiceType* pParamType)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);

	int nRet = 0;
	
	vector<SServiceType>& vecSubType = pParamType->vecSubType;

	do
	{
		unsigned int nStructSize = 0;
		for(unsigned int nStructIndex=0;nStructIndex<vecSubType.size();nStructIndex++)
		{
			if(nStructIndex==(vecSubType.size()-1)&&vecSubType[nStructIndex].eType==DBP_DT_STRING)
			{
				try
				{
					string sTmp = boost::get<string>(row[row.size()-1]);
					HanleNIL(sTmp);
					nStructSize+= sTmp.length();
				}
				catch (std::exception const & e)    
				{       
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String Value Error*,Msg=[%s]\n",__FUNCTION__,e.what());
					nRet = -1;
					break;
				}
			}
			else if(vecSubType[nStructIndex].eType==DBP_DT_STRING)
			{
				nStructSize+=vecSubType[nStructIndex].nStrLen;
			}
			else if(vecSubType[nStructIndex].eType==DBP_DT_SYSTEMSTRING)
			{
				string sTmp="";
				try{
					sTmp = boost::get<string>(row[nStructIndex]);
				}
				catch (std::exception const & e)    
				{       
					int nTmp = boost::get<int>(row[nStructIndex]);	
					char szBuf[24]={0};
					snprintf(szBuf,sizeof(szBuf),"%d",nTmp);
					sTmp=szBuf;
				}
				
				nStructSize+=sizeof(int);
				nStructSize+=sTmp.size()%4==0?sTmp.size():sTmp.size()+(4-sTmp.size()%4);
			}
			else if(vecSubType[nStructIndex].eType==DBP_DT_INT)
			{
				nStructSize+=sizeof(int);
			}
			else
			{
				
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Compute Struct Size Error,"\
					"No Support Struct Element Type,Type=[%d].\n",
					__FUNCTION__,vecSubType[nStructIndex].eType);
				nRet = -1;
				break;
			}
		}

		SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Struct Size=[%d]\n",__FUNCTION__,nStructSize);

		char* szTmp = new char[nStructSize];
		memset(szTmp,0,nStructSize);
		do{
			unsigned int nCurPos = 0;
			for(unsigned int nStructIndex=0;nStructIndex<vecSubType.size();nStructIndex++)
			{
				if(vecSubType[nStructIndex].eType==DBP_DT_STRING)
				{
					try
					{
						

						if(nStructIndex == vecSubType.size()-1)
						{
							string sTmp = boost::get<string>(row[nStructIndex]);
							HanleNIL(sTmp);
							memcpy(szTmp+nCurPos,sTmp.c_str(),sTmp.length());
							nCurPos+=sTmp.length();
						}
						else
						{
							string sTmp = boost::get<string>(row[nStructIndex]);
							HanleNIL(sTmp);
							int nCopyLen = sTmp.length();
							if (sTmp.length()>(unsigned int)vecSubType[nStructIndex].nStrLen)
							{
								nCopyLen = vecSubType[nStructIndex].nStrLen;
							}
							memcpy(szTmp+nCurPos,sTmp.c_str(),nCopyLen);
							nCurPos+=vecSubType[nStructIndex].nStrLen;
						}
					}
					catch (std::exception const & e)    
					{       
						SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String Value Error,Msg=[%s] nStructIndex[%d]\n",__FUNCTION__,e.what(),nStructIndex);
						nRet = -1;
						break;
					}
				}
				else if(vecSubType[nStructIndex].eType==DBP_DT_SYSTEMSTRING)
				{
					string sTmp="";
					try
					{
						sTmp = boost::get<string>(row[nStructIndex]);

					}
					catch (std::exception const & e)    
					{       
						int nTmp = boost::get<int>(row[nStructIndex]);	
						char szBuf[24]={0};
						snprintf(szBuf,sizeof(szBuf),"%d",nTmp);
						sTmp=szBuf;
					}
					int nCopyLen = sTmp.size()%4==0?sTmp.size():sTmp.size()+(4-sTmp.size()%4);
					int nTmp = htonl(sTmp.size());
					memcpy(szTmp+nCurPos,&nTmp,sizeof(int));
					nCurPos+=sizeof(int);
					if (sTmp.size()!=0)
						memcpy(szTmp+nCurPos,sTmp.c_str(),sTmp.size());
					nCurPos+=nCopyLen;
				}
				else if(vecSubType[nStructIndex].eType==DBP_DT_INT)
				{
					int nTmp = 0;
					try
					{
						nTmp = boost::get<int>(row[nStructIndex]);
						nTmp = htonl(nTmp);
						memcpy(szTmp+nCurPos,&nTmp,sizeof(int));
						nCurPos+=sizeof(int);
					}
					catch (boost::bad_get const & e)
					{
						try
						{
							string sTmp = boost::get<string>(row[nStructIndex]);
							nTmp = atoi(sTmp.c_str());
							nTmp = htonl(nTmp);
							memcpy(szTmp+nCurPos,&nTmp,sizeof(int));
							nCurPos+=sizeof(int);
						}
						catch (std::exception const & e)    
						{       
							SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get String->Int Value Error,Msg=[%s]\n",__FUNCTION__,e.what());
							nRet = -1;
						}
						
					}
					catch (std::exception const & e)    
					{       
						SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Int Value Error,Msg=[%s]\n",__FUNCTION__,e.what());
						nRet = -1;
						break;
					}
				}
				else
				{
					SV_XLOG(XLOG_DEBUG,"CDBExec::%s, Compute Struct Size Error,"\
						"Struct Element Not Support Type,Type=[%d].\n",
						__FUNCTION__,vecSubType[nStructIndex].eType);
					nRet = -1;
					break;
				}
			}
			
			if(nRet==0)
			{
				encoder.SetValue(pParamType->nTypeCode,(const void*)szTmp,(unsigned int)nStructSize);
			}
		}while(0);

		delete szTmp;
	} while(0);

	return nRet;
	
}

void CDBExec::GetStructValues(SServiceType* pServiceType,void* pLoc,unsigned int nLen,
					vector<DBStructElement>& vecStructValues)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s\n",__FUNCTION__);
	char* pCurrLoc = (char*)pLoc;
	int nPos = 0;

	if(pServiceType->eType == DBP_DT_STRUCT)
	{
		vector<SServiceType>& vecSubType = pServiceType->vecSubType;

		for(unsigned int nIndex = 0; nIndex < vecSubType.size(); nIndex++)
		{
			if((nIndex==(vecSubType.size()-1))&&(vecSubType[nIndex].eType == DBP_DT_STRING))
			{
				string sTmp = string(pCurrLoc,nLen-nPos);
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Last String=[%s]\n",__FUNCTION__,sTmp.c_str());
				
				DBStructElement dbVar(sTmp);
				vecStructValues.push_back(dbVar);
			}
			else if(vecSubType[nIndex].eType == DBP_DT_STRING)
			{
				string sTmp = string(pCurrLoc,vecSubType[nIndex].nStrLen);
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,String=[%s]\n",__FUNCTION__,sTmp.c_str());

				DBStructElement dbVar(sTmp);
				vecStructValues.push_back(dbVar);
				nPos+=vecSubType[nIndex].nStrLen;
				pCurrLoc+=vecSubType[nIndex].nStrLen;
			}
			else if(vecSubType[nIndex].eType == DBP_DT_INT)
			{
				int nTmp = *((int*)(pCurrLoc));
				SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Int=[%d]\n",__FUNCTION__,nTmp);

				nPos += sizeof(int);
				pCurrLoc += sizeof(int);
				DBStructElement dbVar(nTmp);
				vecStructValues.push_back(dbVar);
			}
		}	
	}
	else
	{
		return;
	}
	
}

void CDBExec::GetStructValues(SServiceType* pServiceType,SAvenueValueNode& sNode,
					vector<DBStructElement>& vecStructValues)
{
	GetStructValues(pServiceType,sNode.pLoc,sNode.nLen,vecStructValues);
}


int CDBExec::GetIntValueFromVariant(DBValue& vValue,int& nValue)
{
	int nRet = 0;
	try
	{
		nValue = boost::get<int>(vValue);
	}
	catch (boost::bad_get const & e)
	{
		try
		{
			string sTmp = boost::get<string>(vValue);
			nValue = atoi(sTmp.c_str());
		}
		catch (std::exception const & e)
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s,String->Int Error,Msg=[%s]",__FUNCTION__,
				e.what());
			nRet = -2;
		}
	}
	catch (std::exception const & e)
	{
		SV_XLOG(XLOG_DEBUG,"CDBExec::%s,Get Int Error,Msg=[%s]",__FUNCTION__,
				e.what());
		nRet = -1;
	}

	return nRet;
}


int CDBExec::ExtendSQL(string &strSQL, string &strLogSql, map<string,SBindDataDesc> &mapParams, ptree &paramValue)
{
	SV_XLOG(XLOG_DEBUG,"CDBExec::%s, before SQL[%s]\n", __FUNCTION__, strSQL.c_str());
	map<string,SBindDataDesc> mapCopyParams = mapParams;
	mapParams.clear();
	for(map<string,SBindDataDesc>::iterator iterParam=mapCopyParams.begin(); iterParam!=mapCopyParams.end(); ++iterParam)
	{
		SBindDataDesc &bindDesc = iterParam->second;
		if (BDT_STRING == bindDesc.eDataType && bindDesc.bExtend)
		{
			SV_XLOG(XLOG_DEBUG,"CDBExec::%s, extend [%s]\n", __FUNCTION__, iterParam->first.c_str());
			ptree::iterator iterValue = paramValue.find(iterParam->first);
			if (iterValue != paramValue.end())
			{
				vector<string> vecSubValue;
				boost::split(vecSubValue, boost::get<string>(iterValue->second), is_any_of(","), token_compress_on);

				char szIndex[4] = {0};
				string strExtend;
				const string strSubPlaceHolderPrefix = iterParam->first + "_";
				for (unsigned int dwIndex = 0; dwIndex<vecSubValue.size(); ++dwIndex)
				{
					snprintf(szIndex, 4, "%03d", dwIndex);
					string strSubPlaceHolder = strSubPlaceHolderPrefix + szIndex;
					paramValue[strSubPlaceHolder] = vecSubValue[dwIndex];
					mapParams[strSubPlaceHolder] = bindDesc;
					if (strExtend.empty())
					{
						strExtend = strSubPlaceHolder;
					}
					else
					{
						strExtend += ',';
						strExtend += strSubPlaceHolder;
					}
				}
				boost::ireplace_all(strSQL, iterParam->first, strExtend);
				boost::ireplace_all(strLogSql, iterParam->first, strExtend);
				paramValue.erase(iterValue);
			}
		}
		else
		{
			mapParams[iterParam->first] = bindDesc;
		}
	}

	SV_XLOG(XLOG_DEBUG,"CDBExec::%s, after SQL[%s]\n", __FUNCTION__, strSQL.c_str());

	return 0;
}









