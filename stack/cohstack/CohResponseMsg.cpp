#include "CohResponseMsg.h"
#include "CohLogHelper.h"

namespace sdo{
namespace coh{
void CCohResponseMsg::SetStatus(int nCode, const string &strMsg)
{
    m_nStatusCode=nCode;
    m_strStatusMsg=strMsg;
}
void CCohResponseMsg::SetAttribute(const string &strPath, const string &strValue)
{
    m_strParameters.append("<"+strPath+">"+strValue+"</"+strPath+">");
}
void CCohResponseMsg::SetAttribute(const string &strPath, int nValue)
{
    char szBuf[16]={0};
    sprintf(szBuf,"%d",nValue);
    m_strParameters.append("<"+strPath+">"+szBuf+"</"+strPath+">");
}
void CCohResponseMsg::SetXmlAttribute(const string &strPath, const string &strValue)
{
    
    m_strParameters.append("<"+strPath+"><![CDATA["+strValue+"]]></"+strPath+">");
}

const string & CCohResponseMsg::Encode()
{
    char bufCode[10];
	char bufLength[36];

	sprintf(bufCode,"%d",m_nStatusCode);
	string body=string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
		"<response><result code=\"")+bufCode+"\">"+m_strStatusMsg+"</result>" \
		"<parameters>" +m_strParameters+"</parameters></response>";
	
	sprintf(bufLength,"Content-Length: %d\r\n",body.length());
	
	m_strRresponseMsg.append("HTTP/1.0 200 OK\r\n");
	m_strRresponseMsg.append("Content-Type: text/xml;charset=utf-8\r\n");
	m_strRresponseMsg.append(bufLength);
	m_strRresponseMsg.append("Server: COH Server/1.0\r\n\r\n");

	m_strRresponseMsg.append(body);
	return m_strRresponseMsg;
}
int CCohResponseMsg::Decode(const string &strResponse)
{
        double ver;
        int httpCode=0;
        sscanf(strResponse.c_str(),"%*[HTTPhttp]/%lf %d",&ver,&httpCode);
        if(httpCode<200||httpCode>=300)
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,httpCode[%d]\n",httpCode);
            return -3;
        }
    
        size_t end;
        end= strResponse.find("\r\n\r\n");
        if(end==string::npos)
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,can't find /r/n/r/n\n");
            return -2;
        }
    
        sdo::util::TiXmlElement *root,*resultNode;
        m_xmlDoc.Parse(strResponse.substr(end+4).c_str());    
        if (m_xmlDoc.Error())
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,error:%s\n",m_xmlDoc.ErrorDesc());
            return -1;
        }
        if((root=m_xmlDoc.RootElement())==NULL)
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,no root node\n");
            return -1;
        }
        if(strncmp(root->Value(),"response",8)!=0)
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,no response node\n");
            return -1;
        }
        if((resultNode=root->FirstChildElement("result"))==NULL) 
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,no result node\n");
            return -1;
        }
        
        string strCode=resultNode->Attribute("code");
        if(strCode.length()==0)
        {
            CS_XLOG(XLOG_WARNING,"CCohResponseMsg::Decode,code value NULL\n");
            return -1;
        }
        m_nStatusCode=atoi(strCode.c_str());
        const char *p=NULL;
        p=resultNode->GetText();
        if(p==NULL)
        {
            m_strStatusMsg="";
        }
        else
        {
            m_strStatusMsg.assign(p);
        }
        m_pxmlParameters=root->FirstChildElement("parameters");
        return 0;

}
int CCohResponseMsg::GetStatusCode() const
{
    return m_nStatusCode;
}
const string & CCohResponseMsg::GetStatusMsg() const
{
    return m_strStatusMsg;
}
string CCohResponseMsg::GetAttribute(const string &strPath)
{
    string elemet;
	size_t begin=0,end=0;
	sdo::util::TiXmlElement *before=m_pxmlParameters;
	if(before==NULL)
    {
            return "";
    }
	while((end=strPath.find("/",begin))!=string::npos)
	{
		elemet=strPath.substr(begin,end-begin);
		begin=end+1;
		if(elemet==""||elemet=="/")
			continue;
		if((before=before->FirstChildElement(elemet))==NULL)
		{
			return ""; 
		}
	}
       
	elemet=strPath.substr(begin);
	if(elemet!="")
	{
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
vector<string> CCohResponseMsg::GetAttributes(const string &strPath)
{
    string elemet;
    vector<string> result;
    size_t begin=0,end=0;
    sdo::util::TiXmlElement *before=m_pxmlParameters;
    if(before==NULL)
    {
        return result; 
    }
    while((end=strPath.find("/",begin))!=string::npos)
    {
        elemet=strPath.substr(begin,end-begin);
        begin=end+1;
        if(elemet==""||elemet=="/")
            continue;
        if((before=before->FirstChildElement(elemet))==NULL) 
        {
            return result; 
        }
    }
    elemet=strPath.substr(begin);
    
    if(elemet!="")
    {
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
