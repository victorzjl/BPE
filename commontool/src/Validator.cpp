#include "Validator.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time.hpp>
#include <boost/asio.hpp>
#include "SapLogHelper.h"
#include "SdkLogHelper.h"
#include <vector>
#include <string>
using std::vector;
using std::string;
namespace sdo{
namespace commonsdk{
bool CValidator::require_validate(const void *pBuffer, int nLen, const string &strParam)
{
	SDK_XLOG(XLOG_NOTICE, "CValidator::%s\n", __FUNCTION__);	
	return (pBuffer!=NULL && nLen!=0);
}
bool CValidator::number_validate(const void *pBuffer, int nLen, const string &strParam)
{
	SDK_XLOG(XLOG_NOTICE, "CValidator::%s\n", __FUNCTION__);
	if(pBuffer==NULL || nLen<4) return false;
	int nValue=*(int *)pBuffer;
	
	int nLeftRange=0x80000000;
	int nRightRange=0x7FFFFFFF;
	if(sscanf(strParam.c_str(),"%d,%d",&nLeftRange, &nRightRange)<2)
	{
		sscanf(strParam.c_str(),",%d",&nRightRange);
	}

	return (nValue>=nLeftRange && nValue<=nRightRange);
}
bool CValidator::regex_validate(const void *pBuffer, int nLen, const string &strParam)
{
	if(pBuffer==NULL || nLen==0) return false;
	string strValue=string((const char *)pBuffer,nLen);
	boost::regex reg( strParam );
	return boost::regex_match( strValue , reg);
}
bool CValidator::email_validate(const void *pBuffer, int nLen, const string &strParam)
{
	return CValidator::regex_validate(pBuffer, nLen,"([0-9A-Za-z\\-_\\.]+)@([a-zA-Z0-9_-])+(.[a-zA-Z0-9_-])+");
}
bool CValidator::url_validate(const void *pBuffer, int nLen, const string &strParam)
{
	string strRegex = "^((https|http|ftp|rtsp|mms|HTTP|HTTPS|FTP|RTSP|MMS)?://)?(([0-9a-z_!~*'().&=+$%-]+: )?[0-9a-z_!~*'().&=+$%-]+@)?(([0-9]{1,3}\\.){3}[0-9]{1,3}|([0-9a-z_!~*'()-]+.)*([0-9a-z][0-9a-z-]{0,61})?[0-9a-z].[a-z]{2,6})(:[0-9]{1,4})?((/?)|(/[0-9a-z_!~*'().;?:@&=+$,%#-]+)+/?)$";  
	return CValidator::regex_validate(pBuffer, nLen, strRegex);
}
bool CValidator::stringlength_validate(const void *pBuffer, int nLen, const string &strParam)
{
	int nLeftRange=0x80000000;
	int nRightRange=0x7FFFFFFF;
	if(sscanf(strParam.c_str(),"%d,%d",&nLeftRange, &nRightRange)<2)
	{
		sscanf(strParam.c_str(),",%d",&nRightRange);
	}
	return (nLen>=nLeftRange && nLen<=nRightRange);
}
bool CValidator::numberset_validate(const void *pBuffer, int nLen, const string &strParam)
{
	if(pBuffer==NULL || nLen<4) return false;
	int nValue=*(int *)pBuffer;

	char szValue[32]={0};
	snprintf(szValue,32,"%d",nValue);

	vector<string> vecParams;
	boost::algorithm::split( vecParams, strParam, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_on); 
	
	for(vector<string>::iterator itr=vecParams.begin();itr!=vecParams.end();itr++)
	{
		string strItr=*itr;
		if(strItr==szValue)
			return true;
	}
	return false;
}
bool CValidator::timerange_validate(const void *pBuffer, int nLen, const string &strParam)
{
	char pStart[64]="1970-1-1 00:00:00";
	char pEnd[64]="2099-1-1 00:00:00";
	if(sscanf(strParam.c_str(),"%s,%s",pStart, pEnd)<2)
	{
		sscanf(strParam.c_str(),",%s",pEnd);
	}
	string strValue=string((const char *)pBuffer,nLen);

	try
	{
		boost::posix_time::ptime ptStart(boost::posix_time::time_from_string(pStart));
		boost::posix_time::ptime ptEnd(boost::posix_time::time_from_string(pEnd));
		boost::posix_time::ptime ptValue(boost::posix_time::time_from_string(strValue));
		return ptValue>=ptStart && ptStart<=ptEnd;
	}
	catch(...)
	{
		return 0;
	}
}
}
}
