#include "SessionManager.h"
#include "tinyxml/tinyxml.h"
#include "TestLogHelper.h"
#include "SapCommon.h"
#include <fstream>
#include <boost/algorithm/string.hpp> 

using std::ifstream;
using namespace sdo::sap;
int CSessionManager::LoadClientSetting(const string & strFileName)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::LoadClientSetting,%s\n",strFileName.c_str());
    ifstream fout(strFileName.c_str());
    int nLine=0;
    string strContent; 
    while(!fout.eof()&&nLine<10000)
    {
        string strTmp;
        getline(fout,strTmp);
        while((*strTmp.begin())==' '||(*strTmp.begin())=='\t')
        {
            strTmp.erase(strTmp.begin());
        }
        while((*strTmp.rbegin())==' '||(*strTmp.rbegin())=='\t'
            ||(*strTmp.rbegin())=='\r'||(*strTmp.rbegin())=='\n')
        {
            strTmp.erase(strTmp.length()-1);
        }
        strContent+=strTmp;
    }
    TiXmlDocument xmlDoc;
    xmlDoc.Parse(strContent.c_str());
    if(xmlDoc.Error())
    {
        TEST_XLOG(XLOG_ERROR,"parse config error:%s\n",xmlDoc.ErrorDesc());
        return -1;
    }
    
    TiXmlElement *pCommand,*pText;
    TiXmlHandle docHandle( &xmlDoc );
    char attr[256]={0};
    TiXmlElement* pNode=NULL;
	pNode= docHandle.FirstChild( "parameters" ).FirstChild( "AppId" ).ToElement();
    if(pNode!=NULL){m_nAppId=atoi(pNode->GetText());}

	pNode= docHandle.FirstChild( "parameters" ).FirstChild( "AreaId" ).ToElement();
    if(pNode!=NULL){m_nAreaId=atoi(pNode->GetText());}
	
	
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "SapServerIp" ).ToElement();
    m_strServerIp=pNode->GetText();
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "SapServerPort" ).ToElement();
    m_nServerPort=atoi(pNode->GetText());
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "Soc" ).ToElement();
    for( pNode; pNode; pNode=pNode->NextSiblingElement() )
	{
	    SSocConfUnit *pConfUnit=new SSocConfUnit;
		pConfUnit->m_nAppId=m_nAppId;
		pConfUnit->m_nAreaId=m_nAreaId;
		
        if(pNode->QueryValueAttribute("concurrence",&attr)==TIXML_SUCCESS)
        {
            pConfUnit->m_nTimes=atoi(attr);
        }
        TiXmlElement* pDetail;
        pDetail=pNode->FirstChildElement("SapPort");
        if(pDetail!=NULL&&pDetail->GetText()!=NULL)
        {
	        pConfUnit->m_nPort=atoi(pDetail->GetText());
        }
        pDetail=pNode->FirstChildElement("Password");
        if(pDetail!=NULL&&pDetail->GetText()!=NULL)
        {
	        pConfUnit->m_strPass=pDetail->GetText();
        }
        pDetail=pNode->FirstChildElement("SocName");
        if(pDetail!=NULL&&pDetail->GetText()!=NULL)
        {
	        pConfUnit->m_strSocName=pDetail->GetText();
        }
        pDetail=pNode->FirstChildElement("Key");
        if(pDetail!=NULL&&pDetail->GetText()!=NULL)
        {
	        pConfUnit->m_strKey=pDetail->GetText();
        }
        pDetail=pNode->FirstChildElement("ExecuteOrder");
        if(pDetail!=NULL&&pDetail->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"circle")==0)
        {
        	TEST_XLOG(XLOG_ERROR,"CSessionManager::LoadClientSetting:%d,attr=%s\n",__LINE__,attr);
            pConfUnit->m_type=E_EXECUTE_CIRCLE;
        }
        else if(pDetail!=NULL&&pDetail->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"connection_circle")==0)
        {
        	TEST_XLOG(XLOG_ERROR,"CSessionManager::LoadClientSetting:%d,attr=%s\n",__LINE__,attr);
            pConfUnit->m_type=E_EXECUTE_CONNECTION_CIRCLE;
        }
        else
        {
        	TEST_XLOG(XLOG_ERROR,"CSessionManager::LoadClientSetting:%d,attr=%s\n",__LINE__,attr);
            pConfUnit->m_type=E_EXECUTE_ONCE;
        }

        TiXmlElement* pCmdNode=pDetail->FirstChildElement("Command");
        for( pCmdNode; pCmdNode; pCmdNode=pCmdNode->NextSiblingElement() )
        {
            SSapCommandTest command;
            if(pCmdNode->QueryValueAttribute("name",&attr)==TIXML_SUCCESS)
            {
                
                if(strcasecmp(attr,"Undefined")==0)
                {
                    if(pCmdNode->QueryValueAttribute("serviceId",&attr)==TIXML_SUCCESS)
                        command.m_nServiceId=atoi(attr);
                    if(pCmdNode->QueryValueAttribute("msgId",&attr)==TIXML_SUCCESS)
                        command.m_nMsgId=atoi(attr);
                }
                else
                {
                    command.m_nServiceId=SAP_PLATFORM_SERVICE_ID;
                    command.m_nMsgId=CSapInterpreter::GetCommandId(attr);
                }
                if(pCmdNode->QueryValueAttribute("interval",&attr)==TIXML_SUCCESS)
                        command.m_nInterval=atoi(attr);
                if(pCmdNode->QueryValueAttribute("concurrence",&attr)==TIXML_SUCCESS)
                        command.m_nTimes=atoi(attr);
                TiXmlElement* pAttriNode=pCmdNode->FirstChildElement("Attribute");
                for( pAttriNode; pAttriNode; pAttriNode=pAttriNode->NextSiblingElement() )
                {
                    SSapAttributeTest attribute;
                    if(pAttriNode->QueryValueAttribute("name",&attr)==TIXML_SUCCESS)
                    {
                        if(pAttriNode->GetText()!=NULL)
                        {
                            attribute.m_strValue=pAttriNode->GetText();
                            attribute.m_nValue=atoi(pAttriNode->GetText());
                            boost::algorithm::split( attribute.vecStruct, 
                                attribute.m_strValue,
                                boost::algorithm::is_any_of(","),
                                boost::algorithm::token_compress_on); 
                        }
                        if(strcasecmp(attr,"Undefined")==0)
                        {
                            if(pAttriNode->QueryValueAttribute("id",&attr)==TIXML_SUCCESS)
                                attribute.m_nAttriId=atoi(attr);
                            if(pAttriNode->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"string")==0)
                                attribute.m_nType=E_ATTRI_VALUE_STR;
                            if(pAttriNode->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"struct")==0)
                                attribute.m_nType=E_ATTRI_VALUE_STRUCT;
                            
                        }
                        else if(strcasecmp(attr,"type")==0)
                        {
                            attribute.m_nAttriId=CSapInterpreter::GetAttriId(attr);
                            attribute.m_nType=CSapInterpreter::GetAttriType(attr);
                            attribute.m_nValue=CSapInterpreter::GetTypeValue(attribute.m_strValue);
                        }
                        else
                        {   attribute.m_nAttriId=CSapInterpreter::GetAttriId(attr);
                            attribute.m_nType=CSapInterpreter::GetAttriType(attr);
                        }
                    }
                    command.m_vecAttri.push_back(attribute);
                }
            }
            pConfUnit->m_vecCommands.push_back(command);
        }
        m_vecUnits.push_back(pConfUnit);
	}
    return 0;
}
int CSessionManager::LoadServerSetting(const string & strFileName)
{
    TEST_XLOG(XLOG_DEBUG,"CSessionManager::LoadServerSetting,%s\n",strFileName.c_str());
    ifstream fout(strFileName.c_str());
    int nLine=0;
    string strContent; 
    while(!fout.eof()&&nLine<10000)
    {
        string strTmp;
        getline(fout,strTmp);
        while((*strTmp.begin())==' '||(*strTmp.begin())=='\t')
        {
            strTmp.erase(strTmp.begin());
        }
        while((*strTmp.rbegin())==' '||(*strTmp.rbegin())=='\t'
            ||(*strTmp.rbegin())=='\r'||(*strTmp.rbegin())=='\n')
        {
            strTmp.erase(strTmp.length()-1);
        }
        strContent+=strTmp;
    }
    TiXmlDocument xmlDoc;
    xmlDoc.Parse(strContent.c_str());
    if(xmlDoc.Error())
    {
        TEST_XLOG(XLOG_ERROR,"parse config error:%s\n",xmlDoc.ErrorDesc());
        return -1;
    }
    
    TiXmlElement *pCommand,*pText;
    TiXmlHandle docHandle( &xmlDoc );
    char attr[256]={0};
    TiXmlElement* pNode;
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "SapServerPort" ).ToElement();
    if(pNode!=NULL)
        m_nListenPort=atoi(pNode->GetText());
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "SocList" ).FirstChild( "soc" ).ToElement();
    for( pNode; pNode; pNode=pNode->NextSiblingElement() )
    {
        string strName;
        string StrPass;
        if(pNode->QueryValueAttribute("username",&attr)==TIXML_SUCCESS)
            strName=attr;
        if(pNode->QueryValueAttribute("password",&attr)==TIXML_SUCCESS)
            StrPass=attr;
        m_mapUsers[strName]=StrPass;
    }
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "Response" ).FirstChild( "Command" ).ToElement();
    for( pNode; pNode; pNode=pNode->NextSiblingElement() )
    {
        SSapCommandResponseTest *pCommand=new SSapCommandResponseTest;;
        if(pNode->QueryValueAttribute("name",&attr)==TIXML_SUCCESS)
        {
            
            if(strcasecmp(attr,"Undefined")==0)
            {
                if(pNode->QueryValueAttribute("serviceId",&attr)==TIXML_SUCCESS)
                    pCommand->m_nServiceId=atoi(attr);
                if(pNode->QueryValueAttribute("msgId",&attr)==TIXML_SUCCESS)
                    pCommand->m_nMsgId=atoi(attr);
            }
            else
            {
                pCommand->m_nServiceId=SAP_PLATFORM_SERVICE_ID;
                pCommand->m_nMsgId=CSapInterpreter::GetCommandId(attr);
            }
            if(pNode->QueryValueAttribute("code",&attr)==TIXML_SUCCESS)
                    pCommand->m_byCode=atoi(attr);
            TiXmlElement* pAttriNode=pNode->FirstChildElement("Attribute");
            for( pAttriNode; pAttriNode; pAttriNode=pAttriNode->NextSiblingElement() )
            {
                SSapAttributeTest attribute;
                if(pAttriNode->QueryValueAttribute("name",&attr)==TIXML_SUCCESS)
                {
                    if(pAttriNode->GetText()!=NULL)
                    {
                        attribute.m_strValue=pAttriNode->GetText();
                        attribute.m_nValue=atoi(pAttriNode->GetText());
                        boost::algorithm::split( attribute.vecStruct, 
                                attribute.m_strValue,
                                boost::algorithm::is_any_of(","),
                                boost::algorithm::token_compress_on); 
                    }
                    if(strcasecmp(attr,"Undefined")==0)
                    {
                        if(pAttriNode->QueryValueAttribute("id",&attr)==TIXML_SUCCESS)
                            attribute.m_nAttriId=atoi(attr);
                        if(pAttriNode->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"string")==0)
                            attribute.m_nType=E_ATTRI_VALUE_STR;
                        if(pAttriNode->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"struct")==0)
                                attribute.m_nType=E_ATTRI_VALUE_STRUCT;
                        
                    }
                    else if(strcasecmp(attr,"type")==0)
                    {
                        attribute.m_nAttriId=CSapInterpreter::GetAttriId(attr);
                        attribute.m_nType=CSapInterpreter::GetAttriType(attr);
                        attribute.m_nValue=CSapInterpreter::GetTypeValue(attribute.m_strValue);
                    }
                    else
                    {   attribute.m_nAttriId=CSapInterpreter::GetAttriId(attr);
                        attribute.m_nType=CSapInterpreter::GetAttriType(attr);
                    }
                }
                pCommand->m_vecAttri.push_back(attribute);
            }
        }
        int nId=(pCommand->m_nServiceId<<8)+pCommand->m_nMsgId;
        m_mapResponse[nId]=pCommand;
    }
    return 0;
}

