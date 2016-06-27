#include "ValueEncoder.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "SapLogHelper.h"
#include <vector>
#include <string>
#include <stdio.h>
using std::vector;
using std::string;
namespace sdo{
namespace commonsdk{
string CValueEncoder::normal_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);
	
	map<string,string>::const_iterator itr;
	
	for( itr=mapParam.begin();itr!=mapParam.end();itr++)
	{
		const string &strSrc=itr->first;
		const string &strDest=itr->second;
		boost::replace_all(strValue,strSrc,strDest);
	}
	return strValue;
}
string CValueEncoder::html_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);

	boost::replace_all(strValue,"&","&amp;");
	boost::replace_all(strValue,"<","&lt;");
	boost::replace_all(strValue,">","&gt;");
	boost::replace_all(strValue,"\"","&quot;");
	boost::replace_all(strValue,"'","&#x27;");
	boost::replace_all(strValue,"/","&#x2f;");
	return strValue;
}
string CValueEncoder::html_filter(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);

	boost::replace_all(strValue,"&","");
	boost::replace_all(strValue,"<","");
	boost::replace_all(strValue,">","");
	boost::replace_all(strValue,"\"","");
	boost::replace_all(strValue,"'","");
	boost::replace_all(strValue,"/","");
	return strValue;
}
string CValueEncoder::nocase_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);
	
	map<string,string>::const_iterator itr;
	
	for( itr=mapParam.begin();itr!=mapParam.end();itr++)
	{
		const string &strSrc=itr->first;
		const string &strDest=itr->second;
		boost::ireplace_all(strValue,strSrc,strDest);
	}
	return strValue;
}
string CValueEncoder::attack_filter(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);

	boost::replace_all(strValue,"&","");
	boost::replace_all(strValue,"<","");
	boost::replace_all(strValue,">","");
	boost::replace_all(strValue,"\"","");
	boost::replace_all(strValue,"'","");
	boost::replace_all(strValue,"/","");

	boost::ireplace_all(strValue,"script","");
	boost::ireplace_all(strValue,"exec","");
	boost::ireplace_all(strValue,"select","");
	boost::ireplace_all(strValue,"update","");
	boost::ireplace_all(strValue,"delete","");
	boost::ireplace_all(strValue,"insert","");
	boost::ireplace_all(strValue,"create","");
	boost::ireplace_all(strValue,"alter","");
	boost::ireplace_all(strValue,"drop","");
	boost::ireplace_all(strValue,"truncate","");
	return strValue;
}
string CValueEncoder::escape_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);
	return strValue;
}
string CValueEncoder::sbc2dbc_utf8(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);
	
	string temp; 
    for (size_t i = 0; i < strValue.size(); i++) { 
        if (((strValue[i] & 0xF0) ^ 0xE0) == 0) { //utf-8 1110xxxx 10xxxxxxxx 10xxxxxxxx 
            int old_char = (strValue[i] & 0xF) << 12 | ((strValue[i + 1] & 0x3F) << 6 | (strValue[i + 2] & 0x3F)); 
            if (old_char == 0x3000) { // blank 
                char new_char = 0x20; 
                temp += new_char; 
            } else if (old_char >= 0xFF01 && old_char <= 0xFF5E) { // full char 
                char new_char = old_char - 0xFEE0; 
                temp += new_char; 
            } else { // other 3 bytes char 
                temp += strValue[i]; 
                temp += strValue[i + 1]; 
                temp += strValue[i + 2]; 
            } 
            i = i + 2; 
        } else { 
            temp += strValue[i]; 
        } 
    } 
    strValue = temp; 
	
	return strValue;
}
string CValueEncoder::sbc2dbc_gbk(const void *pBuffer, int nLen, const map<string,string> &mapParam)
{
	if(pBuffer==NULL || nLen==0) return "";
	string strValue=string((const char *)pBuffer,nLen);
	
	string temp; 
    for (size_t i = 0; i < strValue.size(); i++) {
		if((strValue[i] & 0xFF) >= 0xA1){
			int old_char = (strValue[i] & 0xFF) << 8 | (strValue[i + 1] & 0xFF);
			if (old_char == 0xA1A1) { // blank 
                char new_char = 0x20; 
                temp += new_char; 
            } else if (old_char >= 0xA3C1 && old_char <= 0xA3FE) { // full char 
                char new_char = old_char - 0xA380; 
                temp += new_char; 
            } else { // other 2 bytes char 
                temp += strValue[i]; 
                temp += strValue[i + 1]; 
            } 
            i = i + 1; 
		} else {
			temp += strValue[i]; 
		}
	}
	strValue = temp; 
	
	return strValue;
}
}
}
