#include "JsonAdapter.h"
#include <boost/algorithm/string.hpp>
#include "SoPluginLog.h"

using namespace boost::algorithm;

int CJsonAdapter::GetJsonData( CJsonDecoder &decoder, const string &strKey, string &strValue )
{
	SO_XLOG(XLOG_DEBUG, "CJsonAdapter::%s, Key[%s]\n", __FUNCTION__, strKey.c_str());
	CJsonDecoder objJsonDecoder = decoder;
	
	string strSubKey;
	vector<string> vecKeys;
	split(vecKeys, strKey, is_any_of("."), boost::algorithm::token_compress_on);

	CJsonDecoder objSubJsonDecoder;
	//first node
	vector<string>::const_iterator itr = vecKeys.begin();
	//last node
	vector<string>::const_iterator itrEnd = vecKeys.end();
	--itrEnd;

	while (itr != itrEnd)
	{
		if(objJsonDecoder.GetValue(*itr, objSubJsonDecoder) == 0 )
		{
			objJsonDecoder = objSubJsonDecoder;
			++itr;
		}
		else
		{
			return -1;
		}
	}
	
	return objJsonDecoder.GetValue(*itr, strValue);
}
