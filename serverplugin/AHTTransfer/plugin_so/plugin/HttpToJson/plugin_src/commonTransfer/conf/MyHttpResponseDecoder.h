#ifndef _MY_HTTP_RESPONSE_Decoder_H_
#define _MY_HTTP_RESPONSE_Decoder_H_
#include <string>
#include <map>
#include "SoPluginLog.h"

using std::string;
using std::map;
using std::make_pair;

class MyCHttpResponseDecoder
{
public:
	MyCHttpResponseDecoder(){}
	int Decode(const char *pBuffer, int nLen);
	
	int GetHttpCode(){return m_nHttpCode;}	
	string GetBody(){return m_strBody;}
	string GetHeadValue(const string &strHead);
	void Dump();
	map<string,string> GetHeadValue();
	
	
private:
	int m_nHttpCode;
	string m_strBody;
	map<string,string> m_mapHeadValue;
	
};

#endif
