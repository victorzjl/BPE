#ifndef _CONFIG_PARSER_H_
#define _CONFIG_PARSER_H_
#include <string>
#include <vector>
using std::vector;
using std::string;
#include "tinyxml/ex_tinyxml.h"
namespace sdo{
namespace common{
class CXmlConfigParser
{
public:
	CXmlConfigParser();
	int ParseFile(const string &filepath);
	int ParseBuffer(const char *buffer);
	int ParseDetailBuffer(const char *buffer);
	string GetParameter(const string &path);
	int GetParameter(const string &path,int defaultValue);
	string GetParameter(const string &path,const string& defaultValue);
	vector<string> GetParameters(const string & path);
	const string& GetErrorMessage()const{return m_strError;}
	string GetString();
private:
	sdo::util::TiXmlDocument m_xmlDoc;
	sdo::util::TiXmlElement *m_pParameters;
	string m_strError;
};
}
}
#endif

