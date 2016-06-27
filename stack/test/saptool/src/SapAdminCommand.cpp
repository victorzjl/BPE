#include "SapAdminCommand.h"
#include "tinyxml/tinyxml.h"
#include <fstream>
#include "TestLogHelper.h"
#include "SessionManager.h"

using std::ifstream;

CSapAdminCommand * CSapAdminCommand::sm_instance=NULL;
CSapAdminCommand * CSapAdminCommand::Instance()
{
    if(sm_instance==NULL)
    {
        sm_instance=new CSapAdminCommand;
    }
    return sm_instance;
}

CSapAdminCommand::CSapAdminCommand()
{
    m_mapFuncs["help"]=&CSapAdminCommand::DoHelp_;
    m_mapFuncs["show"]=&CSapAdminCommand::DoShowSessions_;
    m_mapFuncs["reload"]=&CSapAdminCommand::DoReloadConfig_;
    m_mapFuncs["send"]=&CSapAdminCommand::DoSendCommand_;
}

void CSapAdminCommand::Dump() const
{


}
string CSapAdminCommand::DoAdminCommand(const string &strCommand)
{
    string strIndex=strCommand.substr(0,strCommand.find(' '));
    boost::unordered_map<string,pfuncCommand>::iterator itr=m_mapFuncs.find(strIndex);
    if(itr!=m_mapFuncs.end())
    {
        pfuncCommand pFunc=itr->second;
        return (this->*pFunc)(strCommand);
    }
    else
    {
        return "    can't recognize command, use 'help' for detail!";
    }
        
}
string CSapAdminCommand::DoHelp_(const string &strVariant)
{
    string strResult="    help: help\n        description: for detail\n\n" \
            "    reload: reload\n        description: reload ./setting.xml\n\n" \
            "    show sessions: show sessions\n        description: show all sessions\n\n" \
            "    show commands: show commands\n        description: show all commands name\n\n";
    map<int,SSapCommandTest *>::iterator itr;
    for(itr=m_mapSapCommand.begin();itr!=m_mapSapCommand.end();++itr)
    {
        SSapCommandTest *pCommand=itr->second;
        if(pCommand->m_nServiceId==SAP_PLATFORM_SERVICE_ID)
        {
            string strCmdName=CSapInterpreter::GetCommandName(pCommand->m_nMsgId);
            strResult+="    send "+strCmdName+": send "+strCmdName+" sessionId\n        description: send"+
                strCmdName+" to this peer\n\n";
        }
        else
        {
            char szBuf[128]={0};
            sprintf(szBuf,"undefined_%d_%d",pCommand->m_nServiceId,pCommand->m_nMsgId);
            strResult+=string("    send ")+szBuf+": send "+szBuf+" sessionId\n        description: send"+
                szBuf+" to this peer\n\n";
        }
            
    }
    return strResult;
}
string CSapAdminCommand::DoShowSessions_(const string &strVariant)
{
    char szPre[128]={0};
    char szDetail[128]={0};
    sscanf(strVariant.c_str(),"%127s%127s",szPre,szDetail);
    if(strcasecmp(szDetail,"sessions")==0)
    {
        return CSessionManager::Instance()->GetAllSessions();
    }
    else if(strcasecmp(szDetail,"commands")==0)
    {
        string strResult="     name               service    msg    index\n";
        map<int,SSapCommandTest *>::iterator itr;
        for(itr=m_mapSapCommand.begin();itr!=m_mapSapCommand.end();++itr)
        {
            int index=itr->first;
            SSapCommandTest *pCommand=itr->second;
            string strCmdName;
            if(pCommand->m_nServiceId==SAP_PLATFORM_SERVICE_ID)
            {
                strCmdName=CSapInterpreter::GetCommandName(pCommand->m_nMsgId);
            }
            else
            {
                char szBuf[128]={0};
                sprintf(szBuf,"undefined_%d_%d",pCommand->m_nServiceId,pCommand->m_nMsgId);
                strCmdName=szBuf;
            }
            char szList[256]={0};
            int nId=(pCommand->m_nServiceId<<8)+pCommand->m_nMsgId;
            sprintf(szList,"    %-15s     %2d         %2d      %3d\n",
                strCmdName.c_str(),pCommand->m_nServiceId,pCommand->m_nMsgId,nId);
            strResult+=szList;
        }
        return strResult;
    }
    else
    {
        return "    can't recognize command, use 'help' for detail!";
    }
}
string CSapAdminCommand::DoSendCommand_(const string &strVariant)
{
    char szPre[128]={0};
    char szDetail[128]={0};
    int nSeessionid=0;
    sscanf(strVariant.c_str(),"%127s%127s %d",szPre,szDetail,&nSeessionid);
    int nServiceId=0;
    int nMsgId=CSapInterpreter::GetCommandId(szDetail);
    if(nMsgId==SAP_MSG_ALL)
        sscanf(szDetail,"undefined_%d_%d",&nServiceId,&nMsgId);
    else
        nServiceId=SAP_PLATFORM_SERVICE_ID;

    
    int nId=(nServiceId<<8)+nMsgId;

    map<int,SSapCommandTest *>::iterator itr=m_mapSapCommand.find(nId);
    if(itr!=m_mapSapCommand.end())
    {
        SSapCommandTest *pCmd=itr->second;
        CSessionManager::Instance()->OnSendSessionRequest(nSeessionid,pCmd);
        return "    sended!\n";
    }
    else
    {
        char szResult[128]={0};
        sprintf(szResult,"    fail! No this command,serviceId[%d],msgId[%d],index[%d]\n",
            nServiceId,nMsgId,nId);
        return szResult;
    }
}

string CSapAdminCommand::DoReloadConfig_(const string &strVariant)
{
    LoadManagerSetting("./manager.xml");
    return "    Success!";
}
int CSapAdminCommand::LoadManagerSetting(const string & strFileName)
{
    TEST_XLOG(XLOG_DEBUG,"CSapAdminCommand::LoadManagerSetting,%s\n",strFileName.c_str());
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
    pNode= docHandle.FirstChild( "parameters" ).FirstChild( "Request" ).FirstChild( "Command" ).ToElement();
    for( pNode; pNode; pNode=pNode->NextSiblingElement() )
    {
        stSapCommandTest *pCommand=new stSapCommandTest;;
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
                    }
                    if(strcasecmp(attr,"Undefined")==0)
                    {
                        if(pAttriNode->QueryValueAttribute("id",&attr)==TIXML_SUCCESS)
                            attribute.m_nAttriId=atoi(attr);
                        if(pAttriNode->QueryValueAttribute("type",&attr)==TIXML_SUCCESS&&strcasecmp(attr,"string")==0)
                            attribute.m_nType=E_ATTRI_VALUE_STR;
                        
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
        m_mapSapCommand[nId]=pCommand;
    }
    return 0;
}

