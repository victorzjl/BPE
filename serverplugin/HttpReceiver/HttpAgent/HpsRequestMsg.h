#ifndef _SDO_HPS_REQUEST_MSG_H_
#define _SDO_HPS_REQUEST_MSG_H_
#include "SmallObject.h"
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
namespace sdo{
	namespace hps{
		const unsigned int DEFAULT_HPS_PORT=10089;
		class CHpsRequestMsg
		{
		public:
			CHpsRequestMsg();
			int Decode(char *pBuff, int nLen);
			string GetCommand() const;
            const string &GetBody() const { return m_strBody; }
            const string &GetXForwardedFor()const{ return m_strXForwardedFor; }
            const string &GetCookie()const{ return m_strCookie; }
            const string &GetUserAgent() const { return m_strUserAgent; }
            const string &GetContentType() const { return m_strContentType; }
			bool IsKeepAlive(){return m_isKeepAlive;}
			bool IsXForwardedFor(){return m_isXForwardedFor;}
                            bool IsPost(){return m_isPost;}
			int GetHttpVersion(){return m_nHttpVersion;}
			const string &GetHost()const{return m_strHost;}
			int GetXFFPort(){return m_xffport;}
			char *FindLastString(char* pBegin,char*pEnd, const char* pSub);
			static string GetValueByKey(const string& strUrlArri, const string& strKey);
            const std::multimap<std::string, std::string>& GetHeaders() { return m_headers; }

        private:
            void ParseHeaders(const char *pHeaders, unsigned int dwLen);
                                
		private:
			string m_strHost;
			string m_strBody;
			string m_strRequest;
			string m_strXForwardedFor;
			string m_strCookie;
            string m_strUserAgent;
            string m_strContentType;
            unsigned int m_dwContentLen;

			string m_strPath;
			bool m_isKeepAlive;
			bool m_isXForwardedFor;
			int m_nHttpVersion;
			int m_xffport;
                                
            bool m_isPost;
            std::multimap<std::string, std::string> m_headers;
		};
	}
}
#endif //_SDO_HPS_REQUEST_MSG_H_

