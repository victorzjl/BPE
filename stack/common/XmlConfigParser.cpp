#include "XmlConfigParser.h"
#include <algorithm>
namespace sdo{
namespace common{
CXmlConfigParser::CXmlConfigParser():m_pParameters(NULL)
{
}

int CXmlConfigParser::ParseFile(const string &filepath)
{
    if(!m_xmlDoc.LoadFile(filepath.c_str()))
    {
        m_strError=m_xmlDoc.ErrorDesc();
        return -1;
    }
    if((m_pParameters=m_xmlDoc.RootElement())==NULL)
    {
        m_strError="no root node!";
        return -1;
    }
    return 0;
}
int CXmlConfigParser::ParseBuffer(const char *buffer)
{
    m_xmlDoc.Parse(buffer);
    if (m_xmlDoc.Error())
    {
        m_strError=m_xmlDoc.ErrorDesc();
        return -1;
    }
    if((m_pParameters=m_xmlDoc.RootElement())==NULL)
    {
        m_strError="no root node!";
        return -1;
    }
    return 0;
}
int CXmlConfigParser::ParseDetailBuffer(const char *buffer)
{
	string strDetail=string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><parameters>")+buffer+"</parameters>";
	return ParseBuffer(strDetail.c_str());
}

string CXmlConfigParser::GetString()
{
    sdo::util::TiXmlPrinter print;
    m_xmlDoc.Accept(&print);
    return print.Str();
}
int CXmlConfigParser::GetParameter(const string &path,int defaultValue)
{
    string result=GetParameter(path);
    return result==""?defaultValue:atoi(result.c_str());
}
string CXmlConfigParser::GetParameter(const string &path,const string& defaultValue)
{
    string result=GetParameter(path);
    return result==""?defaultValue:result;
}
string CXmlConfigParser::GetParameter(const string &path)
{
	string elemet;
	size_t begin=0,end=0;
	sdo::util::TiXmlElement *before=m_pParameters;
	if(before==NULL)
       {
            return "";
       }
	while((end=path.find("/",begin))!=string::npos)
	{
		elemet=path.substr(begin,end-begin);
		begin=end+1;
		if(elemet==""||elemet=="/")
			continue;
        transform(elemet.begin(),elemet.end(),elemet.begin(),tolower);
		if((before=before->FirstChildElement(elemet))==NULL)
		{
			return ""; 
		}
	}
       
	elemet=path.substr(begin);
	if(elemet!="")
	{ 
        transform(elemet.begin(),elemet.end(),elemet.begin(),tolower);
		if((before=before->FirstChildElement(elemet))==NULL)
		{
			return ""; 
		}
	}
    sdo::util::TiXmlPrinter print;
    before->InnerAccept(&print);
    string result=print.Str();
    result=result.substr(0,result.find_last_not_of("   \n\r\t")+1);
    return result;
}
vector<string> CXmlConfigParser::GetParameters(const string &path)
{
	string elemet;
	vector<string> result;
	size_t begin=0,end=0;
	sdo::util::TiXmlElement *before=m_pParameters;
	if(before==NULL)
	{
		return result; 
	}
	while((end=path.find("/",begin))!=string::npos)
	{
		elemet=path.substr(begin,end-begin);
		begin=end+1;
		if(elemet==""||elemet=="/")
			continue;
        transform(elemet.begin(),elemet.end(),elemet.begin(),tolower);
		if((before=before->FirstChildElement(elemet))==NULL) 
		{
			return result; 
		}
	}
	elemet=path.substr(begin);
	if(elemet!="")
	{
        transform(elemet.begin(),elemet.end(),elemet.begin(),tolower);
		if((before=before->FirstChildElement(elemet))==NULL)
		{
			return result; 
		}
		else
		{
			sdo::util::TiXmlPrinter print;
            before->InnerAccept(&print);
            string strTmp=print.Str();
            strTmp=strTmp.substr(0,strTmp.find_last_not_of("   \n\r\t")+1);
			result.push_back(strTmp);
		}
		while((before=before->NextSiblingElement(elemet))!=NULL)
		{
			sdo::util::TiXmlPrinter print;
            before->InnerAccept(&print);
			string strTmp=print.Str();
            strTmp=strTmp.substr(0,strTmp.find_last_not_of("   \n\r\t")+1);
			result.push_back(strTmp);
		}
	}
	return result;

}
}
}

