#ifndef __URL_SINGATURE_H__
#define __URL_SINGATURE_H__
#include <string>
#include <map>
#ifdef WIN32
#define snprintf _snprintf
#endif

using std::string;
using std::multimap;
using std::make_pair;
class CUrlSingature
{
typedef multimap<string, string> keyvalue_cntr;
typedef keyvalue_cntr::iterator keyvalue_iter;
public:
	CUrlSingature(){}
	
	void setMachantInfo(const string& name, const string &key)
	{
		parmas_.insert(make_pair("merchant_name", name));
		secretKey_ = key;
	}
	
	void setDomainPath(const string& domain, const string& path)
	{
		url_ = domain;
		if (!path.empty() && path[0] != '/')
		{
			url_ += '/';
		}
		url_ += path;
	}
	
	void setDomainPath(const string& domainPath)
	{
		url_ = domainPath;
	}
	
	void setParam(const string& name, const string& value)
	{
		parmas_.insert(make_pair(name, value));
	}
	
	void setParam(const string& name, const int value)
	{
		char szBuff[32] = {0};
		snprintf(szBuff, sizeof(szBuff), "%d", value);
		parmas_.insert(make_pair(name, szBuff));
	}
	
	void reset()
	{
		url_.clear(); 
		parmas_.clear();
	}
    /*
     * get url with signature
     * @return url with signature and additional params
     */
	const string createUrl();
private:
	keyvalue_cntr parmas_;	
	string url_;
	string secretKey_;
};

#endif
