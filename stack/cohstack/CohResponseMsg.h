#ifndef _SDO_COH_RESPONSE_MSG_H_
#define _SDO_COH_RESPONSE_MSG_H_
#include "SmallObject.h"
#include <string>
#include <vector>
#include "tinyxml/ex_tinyxml.h"

using std::string;
using std::vector;
namespace sdo{
	namespace coh{
		class CCohResponseMsg:public sdo::common::CSmallObject
		{
			public:
				void SetStatus(int nCode, const string &strMsg);
				void SetAttribute(const string &strPath, const string &strValue);
				void SetAttribute(const string &strPath, int nValue);
				void SetXmlAttribute(const string &strPath, const string &strValue);
				const string & Encode();
			public:
				int Decode(const string &strResponse);
				int GetStatusCode() const;
				const string & GetStatusMsg() const;
				string GetAttribute(const string &strPath);
				vector<string> GetAttributes(const string &strPath);
			private:
				int m_nStatusCode;
				string m_strStatusMsg;
				string m_strParameters;
				string m_strRresponseMsg;

				sdo::util::TiXmlDocument m_xmlDoc;
				sdo::util::TiXmlElement *m_pxmlParameters;
		};
	}
}
#endif

