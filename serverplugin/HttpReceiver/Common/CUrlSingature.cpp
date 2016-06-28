#include "md5.h"
#include "CUrlSingature.h"
#include <time.h>
#include <set>
using std::set;

static string UrlEncode(const string& szToEncode)
{
	static const char reserved[] = "!'()*-._~";
    string dst;
	char tmp[8]={0};
    for (string::const_iterator it=szToEncode.begin(); it!=szToEncode.end(); ++it)
    {
        unsigned char cc = *it;	
		if (isalnum(cc) || 
			NULL != memchr(reserved, cc, sizeof(reserved)))  
        {  
			dst += cc;  
        }
		else if (cc == ' ')  
		{  
			dst += '+';  
		}
		else  
		{   			
			snprintf(tmp, sizeof(tmp),"%%%X",cc);
			dst += tmp;
		}  
    }
    return dst;
}

const string CUrlSingature::createUrl()
{
	if (!url_.empty()&&url_[url_.size()-1] != '?')
	{
		url_ += '?';
	}
	
    char szTime[64] = {0};
    snprintf(szTime, sizeof(szTime), "%u", time(NULL)); 	
	parmas_.insert(make_pair("signature_method", "MD5"));
	parmas_.insert(make_pair("timestamp", szTime));
		
    // sort & encode value
	set<string> sigs;
    char szValues[1024] = {0};
    for(keyvalue_iter it=parmas_.begin(); it!=parmas_.end(); ++it)
    {
        snprintf(szValues, sizeof(szValues), "%s=%s", 
			it->first.c_str(), it->second.c_str());        
		sigs.insert(szValues);
		
        snprintf(szValues, sizeof(szValues), "%s=%s&", 
			it->first.c_str(), UrlEncode(it->second).c_str());
        url_ += szValues;
    }
	
	// get signature string
	string sigStr;
    for(set<string>::iterator it=sigs.begin(); it!=sigs.end(); ++it)
    {        
        sigStr += *it;
    }
    sigStr += secretKey_;

    char szMd5Hex[32] = {0};
    calc_md5(sigStr.c_str(), szMd5Hex);
    url_ += "signature=";
    url_ += szMd5Hex;
    return url_;
}
