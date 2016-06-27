#ifndef _SDO_COH_REQUEST_MSG_H_
#define _SDO_COH_REQUEST_MSG_H_
#include "SmallObject.h"
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
using std::string;
using std::vector;
namespace sdo{
	namespace coh{
		const unsigned int DEFAULT_COH_PORT=80;
		class CCohRequestMsg:public sdo::common::CSmallObject
		{
			public:
				void SetUrl(const string &strUrl);
				void SetAttribute(const string &strItem, const string &strValue);
				void SetAttribute(const string &strItem,int nValue);
				const string &Encode();
				const string &GetIp()const{return m_strIp;}
				unsigned int GetPort()const{return m_dwPort;}
			public:
				int Decode(const string &strRequest);
				string GetCommand() const;
				string GetAttribute(const string &strItem) const;
				vector<string> GetAttributes(const string &strItem) const;
			private:
				string m_strBody;
				string m_strRequest;

				string m_strIp;
				unsigned int m_dwPort;
				string m_strPath;

				boost::unordered_multimap<string,string> m_mapAttri;
		};
	}
}
#endif

