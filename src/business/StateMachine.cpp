#include "StateMachine.h"
#include "DirReader.h"
#include "SapLogHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstring>
#include "fstream"
#include <vector>
using std::vector;

#ifdef WIN32
#define CCH_PATH_SEP    '\\'
#define CST_PATH_SEP    "\\"
#else
#define CCH_PATH_SEP    '/'
#define CST_PATH_SEP    "/"
#endif

using sdo::commonsdk::CDirReader;

CStateMachine * CStateMachine::m_pInstance = NULL;
CStateMachine * CStateMachine::Instance()
{
	if(m_pInstance==NULL)
		m_pInstance=new CStateMachine;
	return m_pInstance;
}

int CStateMachine::LoadFiles(const string &strAdditionalPath)
{
	SS_XLOG(XLOG_TRACE,"CStateMachine::%s\n",__FUNCTION__);
	CDirReader oDirReader("*.xml");
	char szFileName[MAX_PATH] = {0};
	if (oDirReader.OpenDir(strAdditionalPath.c_str(), true) &&oDirReader.GetFirstFilePath(szFileName)) 
	{
		do
		{        			
			if(LoadMachineFile_(szFileName) != 0)
			{
				SS_XLOG(XLOG_WARNING,"CStateMachine::LoadFiles, load config error. file[%s]\n", szFileName);
				continue;
			}
		}
		while(oDirReader.GetNextFilePath(szFileName));    
	}
	Dump();

	return 0;
}
int CStateMachine::LoadMachineFile_(const string &strFile)
{
	SS_XLOG(XLOG_TRACE,"CStateMachine::%s, [%s]\n",__FUNCTION__,strFile.c_str());
	if (strFile.empty())
	{        
		return -1;
	}

	sdo::util::TiXmlDocument xmlDoc;
	xmlDoc.LoadFile(strFile.c_str());
	if (xmlDoc.Error())
	{
       	 SS_XLOG(XLOG_WARNING,"CStateMachine::%s, Parse config error:%s. file name[%s]\n", __FUNCTION__, xmlDoc.ErrorDesc(), strFile.c_str());
		return -1;
	}
	sdo::util::TiXmlElement *pPartNode = xmlDoc.FirstChildElement("part");
	if (pPartNode==NULL)
	{
		SS_XLOG(XLOG_WARNING,"CStateMachine::%s, Parse config error:%s. file name[%s] no part node\n", __FUNCTION__, xmlDoc.ErrorDesc(), strFile.c_str());
		return -1;
	}
	map<string, int>  mapVales;
	 for (sdo::util::TiXmlElement *pDef = pPartNode->FirstChildElement("def");
        pDef != NULL; pDef = pDef->NextSiblingElement("def"))
	{
		string strName;
		int nValue=0;
		if(LoadVariant_(pDef,strName,nValue)==0)
		{
			mapVales[strName]=nValue;
			string strTypeName;
		}
	}

	if(IsEnableLevel(SAP_STACK_MODULE,XLOG_DEBUG)) 
	{
		map<string, int>::iterator itrTmpValue;
		for(itrTmpValue=mapVales.begin();itrTmpValue!=mapVales.end();itrTmpValue++)
		{
			string strName=itrTmpValue->first;
			int nValue=itrTmpValue->second;
			SS_XLOG(XLOG_DEBUG,"\t%s=%d\n",strName.c_str(),nValue);
		}
	}
		

	sdo::util::TiXmlElement *pMachine = pPartNode->FirstChildElement("statemachine");
	while (pMachine!=NULL)
	{
		string strMachine;
		MAPNStates oNStates;
		MAPStrStates oStrStates;
		if (pMachine == NULL ||(pMachine->QueryValueAttribute("id", &strMachine)) != sdo::util::TIXML_SUCCESS)
		{
			SS_XLOG(XLOG_ERROR,"CStateMachine::LoadMachineFile [%s], get 'StateMachine' failed or no id\n",strFile.c_str());
			return -1;
		}
		MAPStateEvents & oStateEvents=m_machine[strMachine];

		for (sdo::util::TiXmlElement *pState = pMachine->FirstChildElement("state");
			pState != NULL; pState = pState->NextSiblingElement("state"))
		{
			string strState;
			int nState=0;
			if((pState->QueryValueAttribute("id", &strState)) != sdo::util::TIXML_SUCCESS)
			{
				SS_XLOG(XLOG_ERROR, "CStateMachine::LoadMachineFile [%s], state no id\n",strFile.c_str());
					continue;
			}
			map<string, int>::iterator itrValue;
			if((itrValue=mapVales.find(strState))!=mapVales.end())
			{
				nState=itrValue->second;
			}
			oNStates[nState]=strState;
			oStrStates[strState]=nState;
			MAPEvents &oEvents=oStateEvents[nState];

			string strIncludeStates;
			if((pState->QueryValueAttribute("includestate", &strIncludeStates)) == sdo::util::TIXML_SUCCESS)
			{
				boost::erase_all(strIncludeStates," ");
				boost::erase_all(strIncludeStates,"\t");
				vector<string> vecStr;
				boost::split(vecStr, strIncludeStates, boost::is_any_of(","));
				for(vector<string>::iterator itrInclude=vecStr.begin();itrInclude!=vecStr.end();++itrInclude)
				{
					string& strIncludeState=*itrInclude;
					int nIncludeState=0;
					map<string, int>::iterator itrValue;
					if((itrValue=mapVales.find(strIncludeState))!=mapVales.end())
					{
						nIncludeState=itrValue->second;
						if(oStateEvents.find(nIncludeState)!=oStateEvents.end())
						{
							MAPEvents &oIncludeEvents=oStateEvents[nIncludeState];
							for(MAPEvents::iterator itrEvents=oIncludeEvents.begin();itrEvents!=oIncludeEvents.end();++itrEvents)
							{
								oEvents[itrEvents->first]=itrEvents->second;
							}
						}
					}
				}
			}
			
			for (sdo::util::TiXmlElement *pEvent = pState->FirstChildElement("stateevent");
					pEvent != NULL; pEvent = pEvent->NextSiblingElement("stateevent"))
			{
				string strEvent=pEvent->GetText();
				boost::erase_all(strEvent," ");
				boost::erase_all(strEvent,"\t");
				vector<string> vecStr;
				boost::split(vecStr, strEvent, boost::is_any_of(","));
				if(vecStr.size()<2) 
				{
					SS_XLOG(XLOG_ERROR, "CStateMachine::LoadMachineFile [%s], stateevent format not 'event, state'\n",strFile.c_str());
					continue;
				}

				int nEvent=0;
				int nNextState=0;
				map<string, int>::iterator itrValue;
				if((itrValue=mapVales.find(vecStr[0]))!=mapVales.end())
				{
					nEvent=itrValue->second;
				}
				if((itrValue=mapVales.find(vecStr[1]))!=mapVales.end())
				{
					nNextState=itrValue->second;
				}
				oEvents[nEvent]=nNextState;
			}
		}
		m_nStates[strMachine]=oNStates;
		m_strStates[strMachine]=oStrStates;
		pMachine = pMachine->NextSiblingElement("statemachine");
	}
	return 0;
}

int CStateMachine::LoadVariant_(sdo::util::TiXmlElement * pElement, string & strName, int & nValue)
{
	SS_XLOG(XLOG_TRACE, "CStateMachine::LoadVariant_...\n");
	string strVariant=pElement->GetText();
	boost::erase_all(strVariant," ");
	boost::erase_all(strVariant,"\t");
	boost::erase_all(strVariant,"$");

	vector<string> vecStr;
	boost::split(vecStr, strVariant, boost::is_any_of("="));
	if(vecStr.size()<2) 
	{
		SS_XLOG(XLOG_ERROR, "CStateMachine::LoadVariant_ failed, ,<def> format not 'key=nvalue' [%s]\n", strVariant.c_str());
		return -1;
	}
	strName=vecStr[0];
	nValue=atoi(vecStr[1].c_str());
	return 0;
}
int CStateMachine::ExcuteStateMachine(const string &strMachineName, int nState, int nEvent)
{
	string strLowerName=strMachineName;
	transform (strLowerName.begin(), strLowerName.end(), strLowerName.begin(), (int(*)(int))tolower);
	MAPStateMachine::const_iterator itrMachine;
	if((itrMachine=m_machine.find(strLowerName))==m_machine.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::ExcuteStateMachine, no machine[%s]\n",strLowerName.c_str());
		return -1;
	}
	
	const MAPStateEvents & oStateEvents=itrMachine->second;
	MAPStateEvents::const_iterator itrStateEvent;
	if((itrStateEvent=oStateEvents.find(nState))==oStateEvents.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::ExcuteStateMachine, machine[%s], no state[%d]\n",strLowerName.c_str(),nState);
		return -1;
	}
	
	const MAPEvents &oEvents=itrStateEvent->second;
	MAPEvents::const_iterator itrEvent;
	if((itrEvent=oEvents.find(nEvent))==oEvents.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::ExcuteStateMachine, machine[%s], state[%d],no event[%d]\n",strLowerName.c_str(),nState,nEvent);
		return -1;
	}
	return itrEvent->second;
}
string CStateMachine::GetMachineStateName(const string &strMachineName, int nState)
{
	string strLowerName=strMachineName;
	transform (strLowerName.begin(), strLowerName.end(), strLowerName.begin(), (int(*)(int))tolower);
	MAPMachineNStates::const_iterator itrNStates;
	if((itrNStates=m_nStates.find(strLowerName))==m_nStates.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::GetMachineStateName, no machine[%s]\n",strLowerName.c_str());
		return "";
	}
	

	const MAPNStates &oNStates=itrNStates->second;
	MAPNStates::const_iterator itrState;
	if((itrState=oNStates.find(nState))==oNStates.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::GetMachineStateName, machine[%s], no state[%d]\n",strLowerName.c_str(),nState);
		return "";
	}
	return itrState->second;
}
int CStateMachine::GetMachineState(const string &strMachineName,const string & strState)
{
	string strLowerName=strMachineName;
	string strLowerState=strState;
	transform (strLowerName.begin(), strLowerName.end(), strLowerName.begin(), (int(*)(int))tolower);
	transform (strLowerState.begin(), strLowerState.end(), strLowerState.begin(), (int(*)(int))tolower);
	MAPMachineStrStates::const_iterator itrStrStates;
	if((itrStrStates=m_strStates.find(strLowerName))==m_strStates.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::GetMachineStateName, no machine[%s]\n",strLowerName.c_str());
		return -1;
	}
	

	const MAPStrStates &oStrStates=itrStrStates->second;
	MAPStrStates::const_iterator itrState;
	if((itrState=oStrStates.find(strLowerState))==oStrStates.end())
	{
		SS_XLOG(XLOG_WARNING, "CStateMachine::GetMachineStateName, machine[%s], no state[%s]\n",strLowerName.c_str(),strState.c_str());
		return -1;
	}
	return itrState->second;
}
void CStateMachine::Dump()
{	
	MAPStateMachine::const_iterator itrMachine;
	for(itrMachine=m_machine.begin();itrMachine!=m_machine.end();itrMachine++)
	{
		const string & strMachine=itrMachine->first;
		SS_XLOG(XLOG_DEBUG,"####machine[%s]###\n",strMachine.c_str());
		{
			const MAPStateEvents & oStateEvents=itrMachine->second;
			MAPStateEvents::const_iterator itrStateEvent;
			for(itrStateEvent=oStateEvents.begin();itrStateEvent!=oStateEvents.end();++itrStateEvent)
			{
				int nState=itrStateEvent->first;
				SS_XLOG(XLOG_DEBUG,"\t####state[%d]:\n",nState);
				const MAPEvents &oEvents=itrStateEvent->second;
				MAPEvents::const_iterator itrEvent;
				for(itrEvent=oEvents.begin();itrEvent!=oEvents.end();++itrEvent)
				{
					int nEvent=itrEvent->first;
					int nNextState=itrEvent->second;
					SS_XLOG(XLOG_DEBUG,"\t\t%d,%d\n",nEvent,nNextState);
				}
			}
		}
	}
	/*
	string strState=GetMachineStateName("businEss_machine",2);
	SS_XLOG(XLOG_DEBUG,"\t####state[2]=%s && state[machine_send_validecode_state]=%d\n",
		strState.c_str(),GetMachineState("businEss_machine","machine_send_validecode_state"));
		*/
}
