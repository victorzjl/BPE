#ifndef _SDO_HPS_RESPONSE_MSG_H_
#define _SDO_HPS_RESPONSE_MSG_H_
#include "SmallObject.h"
#include <string>
#include <vector>
#include "tinyxml/tinyxml.h"

using std::string;
using std::vector;
namespace sdo{
namespace hps{
class CHpsResponseMsg
{
	public:
		CHpsResponseMsg();
		~CHpsResponseMsg(){}
		void SetCode(int nCode);
		void SetKeepAlive(bool isKeepAlive);
		void SetBody(const string &strResponse);
		void SetHttpVersion(int nHttpVersion);
		
		const string & Encode(int nHttpVersion,bool isKeepAlive,int nCode);
                const string & EncodeBody(int nCode);
		
		const string & Encode(int nHttpVersion,bool isKeepAlive,const string &strCharSet,
			const string &strContentType, const string &strResponse,int nHttpCode,
			const vector<string>& vecCookie, const string& strLocationAddr,
			const string &strHeaders);
                
		
	private:
		int m_nHttpVersion;
		bool m_isKeepAlive;
		int m_nStatusCode;
		string m_strResponse;
		
		string m_strHttpResponseMsg;
		
};
}
}
#endif //_SDO_HPS_RESPONSE_MSG_H_

