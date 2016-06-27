#include "ComposeService.h"
#include "DirReader.h"
#include "SapLogHelper.h"
#include "tinyxml/tinyxml.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstring>
#include "fstream"

#ifdef WIN32
#define CCH_PATH_SEP    '\\'
#define CST_PATH_SEP    "\\"
#else
#define CCH_PATH_SEP    '/'
#define CST_PATH_SEP    "/"
#endif

//using namespace sdo::util;
using sdo::commonsdk::CDirReader;
using sdo::commonsdk::SSeriveConfig;
using sdo::commonsdk::SAvenueMessageConfig;


const SComposeServiceUnit* CComposeServiceContainer::FindComposeServiceUnit(const SComposeKey &sKey)const
{
	map<SComposeKey, SComposeServiceUnit>::const_iterator itr=m_mapComposeUnit.find(sKey);
	if(itr!=m_mapComposeUnit.end())
	{
		return &(itr->second);
	}
	return NULL;    
}

int CComposeServiceContainer::LoadComposeServiceConfigs(const string &strComposeConfigPath, const string &strDataComposeConfigPath, CAvenueServiceConfigs &serviceConfig, const map<SComposeKey, vector<unsigned> >& mapDataServerId)
{
    SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s\n",__FUNCTION__);
	m_pServiceConfig = &serviceConfig;
	m_mapComposeUnit.clear();
	CDirReader oDirReader("*.xml");
    char szFileName[MAX_PATH] = {0};
    //char szServiceConfigFile[MAX_PATH] = {0};

	if (oDirReader.OpenDir(strComposeConfigPath.c_str(), true) &&
        oDirReader.GetFirstFilePath(szFileName)) 
	{
        do
        {        
            //sprintf(szServiceConfigFile, "%s%s%s", strComposeConfigPath.c_str(), CST_PATH_SEP, szFileName);				
            if(LoadConfig(szFileName, mapDataServerId) != 0)
            {
                SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadComposeServiceConfigs, load config error. file[%s]\n", szFileName);
                continue;
            }
        }
        while(oDirReader.GetNextFilePath(szFileName));    
	}

	CDirReader oDataComposeDirReader("*.xml");
    if(oDataComposeDirReader.OpenDir(strDataComposeConfigPath.c_str(), true) && 
        oDataComposeDirReader.GetFirstFilePath(szFileName))
    {
        do
        {
            //sprintf(szServiceConfigFile, "%s%s%s", strDataComposeConfigPath.c_str(), CST_PATH_SEP, szFileName);				
            if(LoadConfig(szFileName, mapDataServerId, true) != 0)
            {
                SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, load data compose config error. file[%s]\n", __FUNCTION__, szFileName);
                continue;
            }
        }
        while(oDataComposeDirReader.GetNextFilePath(szFileName));     
    }


    Dump();
    
	return 0;
}

int CComposeServiceContainer::LoadIncludeConfig(TiXmlElement *pCompseNode, TiXmlElement *pInclude, const string &strConfigFilePassIn)
{
	SS_XLOG(XLOG_TRACE,"CComposeServiceContainer::%s, [%s]\n",__FUNCTION__,strConfigFilePassIn.c_str());
	string strConfigFile = strConfigFilePassIn;
	TiXmlDocument xmlDoc;
	string strContent;
	int nPos = strConfigFile.find("(");
	if (nPos==string::npos)
	{
		string strConfigFileRead = "./compose_include/" +strConfigFile;
		xmlDoc.LoadFile(strConfigFileRead.c_str());
		if (xmlDoc.Error())
		{
			strConfigFileRead = "./state_conf/" +strConfigFile;
			xmlDoc.LoadFile(strConfigFileRead.c_str());
		}
	}
	else
	{
		int nPos2 = strConfigFile.find(")");
		string strParams = strConfigFile.substr(nPos+1,nPos2-nPos-1);
		strConfigFile = strConfigFile.substr(0,nPos);
		string strConfigFileRead = "./compose_include/" +strConfigFile;
		std::ifstream infile(strConfigFileRead.c_str(),ios::in|ios::binary);
		if(!infile)
		{
			 strConfigFileRead = "./state_conf/" +strConfigFile;
			 std::ifstream infile2(strConfigFileRead.c_str(),ios::in|ios::binary);
			 if(!infile2)
			 {
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, file missing error file name[%s]\n", __FUNCTION__,  strConfigFileRead.c_str());
				return -1;
			 }
			infile2.seekg(0, ios::end);
			int dwLen = infile2.tellg();
			infile2.seekg(0, ios::beg);
			strContent.resize(dwLen+1,0);
			infile2.read((char*)strContent.c_str(),dwLen);
			infile2.close();
		}
		else
		{
			infile.seekg(0, ios::end);
			int dwLen = infile.tellg();
			infile.seekg(0, ios::beg);
			strContent.resize(dwLen+1,0);
			infile.read((char*)strContent.c_str(),dwLen);
			infile.close();
		}
		
		vector<string> vecParams;
		boost::split(vecParams,strParams,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
		for(unsigned int dwIndex = 0; dwIndex < vecParams.size(); dwIndex++)
		{
			char szTmp[32]={0};
			snprintf(szTmp,sizeof(szTmp),":%03d",dwIndex+1);
			boost::ireplace_all(strContent,szTmp,vecParams[dwIndex]);
		}
		
		xmlDoc.Parse(strContent.c_str());
	}
	
	if (xmlDoc.Error())
	{
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Parse config error:%s. file name[%s]\n", __FUNCTION__, xmlDoc.ErrorDesc(), strConfigFile.c_str());
		return -1;
	}
	TiXmlHandle docHandle(&xmlDoc);
	TiXmlElement *pPartNode = docHandle.FirstChild("part").ToElement();
	if (pPartNode == NULL)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadIncludeConfig [%s], get 'part' failed\n",strConfigFile.c_str());
		return -1;
	}
	for (TiXmlElement *pNode = pPartNode->FirstChildElement();
        pNode != NULL; pNode = pNode->NextSiblingElement())
    {
		pCompseNode->InsertBeforeChild(pInclude,*pNode);
    }
	pCompseNode->RemoveChild(pInclude);
	return 0;
}

int CComposeServiceContainer::LoadConfig(const string &strConfigFile, const map<SComposeKey, vector<unsigned> >& mapDataServerId, bool isDataCompose)
{
	SS_XLOG(XLOG_TRACE,"CComposeServiceContainer::%s, [%s]\n",__FUNCTION__,strConfigFile.c_str());
	if (strConfigFile.empty())
	{        
		//SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig, strConfigFile is empty.\n");
		return -1;
	}

	TiXmlDocument xmlDoc;
	xmlDoc.LoadFile(strConfigFile.c_str());
	//xmlDoc.Parse(strContent.c_str());
	if (xmlDoc.Error())
	{
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Parse config error:%s. file name[%s]\n", __FUNCTION__, xmlDoc.ErrorDesc(), strConfigFile.c_str());
		return -1;
	}

	// Get Compose Elements
	TiXmlHandle docHandle(&xmlDoc);
	char szAttr[256] = {0};

	TiXmlElement *pCompseNode = docHandle.FirstChild("compose").ToElement();
	if (pCompseNode == NULL)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig [%s], get 'compose' failed\n",strConfigFile.c_str());
		return -1;
	}

	SComposeServiceUnit stComposeServiceUnit;
    m_current_unit = &stComposeServiceUnit;
    stComposeServiceUnit.isDataCompose = isDataCompose;

	// Get service name
	if (pCompseNode->QueryValueAttribute("service", &szAttr) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig [%s], Get 'service' failed\n",strConfigFile.c_str());
		return -1;
	}	
    stComposeServiceUnit.strServiceName = szAttr;

    stComposeServiceUnit.mapNodeIndex["-1"] = -1;
	
	// Get includes
	 for (TiXmlElement *pNode = pCompseNode->FirstChildElement("include");
        pNode != NULL; pNode = pNode->NextSiblingElement("include"))
    {
		if (pNode->GetText()!=NULL)
		{
			string strIncludeFileName = pNode->GetText();
			if (LoadIncludeConfig(pCompseNode, pNode, strIncludeFileName) != 0)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GetIncludeFile %s failed\n",strIncludeFileName.c_str());
				return -1;
			}
		}
    }
	
	string strConfigFileRunning = strConfigFile;
	boost::ireplace_all(strConfigFileRunning,"compose_conf","compose_running");
	xmlDoc.SaveFile(strConfigFileRunning);
	
	// Get nodes
    for (TiXmlElement *pNode = pCompseNode->FirstChildElement("node");
        pNode != NULL; pNode = pNode->NextSiblingElement("node"))
    {
        if (GenerateNodeId(pNode, stComposeServiceUnit) != 0)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateNodeId failed\n");
            return -1;
        }
    }

    char encode_str[256];
    if (pCompseNode->QueryValueAttribute("endnode", &encode_str) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig [%s], Get 'endnode' failed\n", strConfigFile.c_str());
		return -1;
	}
    stComposeServiceUnit.nEndNode = -1;
    map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(encode_str);
    if (itr_node_index == stComposeServiceUnit.mapNodeIndex.end())
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig [%s], get end code index failed. end_code[%s]\n", strConfigFile.c_str(), encode_str);
        return -1;
    }
    stComposeServiceUnit.nEndNode = itr_node_index->second;

    //get def value
    for (TiXmlElement *pNode = pCompseNode->FirstChildElement("def");
        pNode != NULL; pNode = pNode->NextSiblingElement("def"))
    {
        if (ReadDefValue(pNode, stComposeServiceUnit) != 0)
        {
            SS_XLOG(XLOG_NOTICE,"CComposeServiceContainer::LoadConfig, Get 'def' failed\n");
        }
    }

	for (TiXmlElement *pNode = pCompseNode->FirstChildElement("node");
		pNode != NULL; pNode = pNode->NextSiblingElement("node"))
	{
		if (ReadComposeNode(pNode, stComposeServiceUnit) != 0)
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig [%s], Get 'node' failed\n", strConfigFile.c_str());
			return -1;
		}
	}

	if (stComposeServiceUnit.mapNodes.empty())
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig, no correct 'node' configure in service[%s]\n", stComposeServiceUnit.strServiceName.c_str());
		return -1;
	}
	if (CheckConfig(stComposeServiceUnit) != 0 ||
        GenerateId(stComposeServiceUnit) != 0 ||
        GenerateConditionParam(stComposeServiceUnit) != 0)
	{
		return -1;
	}

	unsigned serviceId, msgId;
	if (m_pServiceConfig->GetServiceIdByName(stComposeServiceUnit.strServiceName,&serviceId,&msgId) != 0)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::LoadConfig, get[%s] serviceid,msgid failed\n",stComposeServiceUnit.strServiceName.c_str());
		return -1;
	}
    stComposeServiceUnit.sKey.dwMsgId = msgId;
    stComposeServiceUnit.sKey.dwServiceId = serviceId;
	SComposeKey sKey(serviceId,msgId);

    map<SComposeKey, vector<unsigned> >::const_iterator itrDataServerId = mapDataServerId.find(sKey);
    if (itrDataServerId != mapDataServerId.end())
    {
        stComposeServiceUnit.vecDataServerId = itrDataServerId->second;
    }
	m_mapComposeUnit.insert(make_pair(sKey, stComposeServiceUnit));

	return 0;
}

int CComposeServiceContainer::CheckConfig(SComposeServiceUnit &stComposeServiceUnit)
{
	map<NodeIndex, SComposeNode>&          mapNodes = stComposeServiceUnit.mapNodes;
    map<NodeIndex, SComposeNode>::iterator iterNode = mapNodes.find(stComposeServiceUnit.nEndNode);
    if(iterNode == mapNodes.end())
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::CheckConfig, service[%s] end node[%s][%d] not found\n",
					stComposeServiceUnit.strServiceName.c_str(),getNodeName(stComposeServiceUnit, stComposeServiceUnit.nEndNode), stComposeServiceUnit.nEndNode);
        return -1;
    }
    
	for ( iterNode= mapNodes.begin(); iterNode != mapNodes.end(); ++iterNode)
	{
		vector<SComposeGoto> &results = iterNode->second.vecResult;
		if (results.empty())
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::CheckConfig,service[%s] node[%s][%d] find goto node failed.\n", 
                stComposeServiceUnit.strServiceName.c_str(),getNodeName(stComposeServiceUnit, iterNode->first), iterNode->first);
		}
		for (vector<SComposeGoto>::iterator iterRet = results.begin();
			iterRet != results.end(); ++iterRet)
		{
			if (iterRet->sTargetNode.nIndex != -1)
			{
                map<NodeIndex, SComposeNode>::iterator iterNodeCheck = mapNodes.find(iterRet->sTargetNode.nIndex);
				if (iterNodeCheck == mapNodes.end())
				{
					SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::CheckConfig, faild in service[%s] node[%s][%d]\n",
					stComposeServiceUnit.strServiceName.c_str(), iterRet->sTargetNode.strIndex.c_str(), iterRet->sTargetNode.nIndex);
					return -1; // 如果goto中的node指向的节点不存在，则认为非法
				}
			}
		}
	}

	return 0;
}

int CComposeServiceContainer::ReadComposeNode(TiXmlElement *pNode, SComposeServiceUnit &stComposeServiceUnit)
{
	SComposeNode stComposeNode;
    char node_str[128];

	// Get Node Index
	if (pNode->QueryValueAttribute("index", &node_str) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadComposeNode, get 'index' failed\n");
		return -1;
	}

    for (TiXmlElement *pSequence = pNode->FirstChildElement("sequence");
        pSequence != NULL; pSequence = pSequence->NextSiblingElement("sequence"))
    {
        if (GenerateSequenceId(pSequence, stComposeNode) != 0)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateSequenceId failed\n");
            return -1;
        }
    }

	for (TiXmlElement *pSequence = pNode->FirstChildElement("sequence");
		pSequence != NULL; pSequence = pSequence->NextSiblingElement("sequence"))
	{
		if (ReadSequence(pSequence, stComposeNode) != 0)
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadComposeNode, Get 'sequence' failed, node[%s]\n", node_str);
			return -1;
		}
	}

	TiXmlElement *pResult = pNode->FirstChildElement("result");
    if (!pResult)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadComposeNode, Get 'result' failed, node[%s]\n", node_str);
        return -1;
    }
	
    for (TiXmlElement *pAssign = pResult->FirstChildElement("assign");
        pAssign != NULL; pAssign = pAssign->NextSiblingElement("assign"))
    {
        SValueInCompose valueInCompose;
        if (ReadValueInResult(pAssign, valueInCompose) != 0)
        {
            SS_XLOG(XLOG_NOTICE,"CComposeServiceContainer::ReadComposeNode, Get 'assign' in result failed, node[%s]\n", node_str);
            return -1;
        }
        stComposeNode.vecResultValues.push_back(valueInCompose);
    }

	for (TiXmlElement *pGoto = pResult->FirstChildElement("goto");
		pGoto != NULL; pGoto = pGoto->NextSiblingElement("goto"))
	{
		if (ReadGoto(pGoto, stComposeNode) != 0)
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadComposeNode, Get 'goto' failed, node[%s]\n", node_str);
			return -1;
		}
	}

    map<string, NodeIndex>::const_iterator itr_node_index;
    for (vector<SComposeGoto>::iterator itr_goto = stComposeNode.vecResult.begin();
            itr_goto != stComposeNode.vecResult.end(); ++ itr_goto)
    {
        itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_goto->sTargetNode.strIndex);
        itr_node_index != stComposeServiceUnit.mapNodeIndex.end() ? 
            itr_goto->sTargetNode.nIndex = itr_node_index->second : itr_goto->sTargetNode.nIndex = -1;
    }

    //stComposeNode.dwSeqSize = stComposeNode.mapSeqs.size();
	//SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::ReadComposeNode, seqSize[%d], node[%s]\n", stComposeNode.dwSeqSize,node_str);

    itr_node_index = stComposeServiceUnit.mapNodeIndex.find(node_str);
    if (itr_node_index != stComposeServiceUnit.mapNodeIndex.end())
    {
    	stComposeNode.strNodeName=itr_node_index->first;
        stComposeServiceUnit.mapNodes.insert(make_pair(itr_node_index->second, stComposeNode));
    }
    else
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadComposeNode, Can't find node index, node[%s]\n", node_str);
        return -1;
    }

    return 0;
}

int CComposeServiceContainer::ReadDefValue(TiXmlElement *pNode, SComposeServiceUnit &stComposeServiceUnit)
{
    // Get source key
    const char* pValue = pNode->GetText();
    if (!pValue)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Get source key failed\n",__FUNCTION__);
        return -1;
    }

    if (*pValue != '$')
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, There is no '$' before the def value name. value name[%s]\n",
            __FUNCTION__, pValue);
        return -1;
    }

    char szType[256] = {0};
    SDefParameter def_param;
    if (pNode->QueryValueAttribute("class", &szType) == TIXML_SUCCESS)
    {
        if (memcmp(szType, "int", 3) == 0)
        {
            def_param.enType = sdo::commonsdk::MSG_FIELD_INT;
        }
        else if (memcmp(szType, "string", 6) == 0)
        {
            def_param.enType = sdo::commonsdk::MSG_FIELD_STRING;
        }
		else if (memcmp(szType, "meta", 4) == 0)
        {
            def_param.enType = sdo::commonsdk::MSG_FIELD_META;
        }
        else
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, unknow def value type. type[%s]\n", __FUNCTION__, szType);
            return -1;
        }
    }

    string value_name(pValue + 1);
    string::size_type index = value_name.find("=");
    if (index != string::npos)
    {
        def_param.strDefaultValue = value_name.c_str() + index + 1;
        value_name.erase(index);
        if (def_param.enType == sdo::commonsdk::MSG_FIELD_INT)
        {
            def_param.nDefaultValue = atoi(def_param.strDefaultValue.c_str());
        }
        def_param.bHasDefaultValue = true;
    }

    stComposeServiceUnit.mapDefValue[value_name] = def_param;

    return 0;
}

int CComposeServiceContainer::ReadSequence(TiXmlElement *pNode, SComposeNode &stComposeNode)
{
	SComposeSequence stComposeSequence;
	stComposeSequence.bCheckServiceExist = false;
	stComposeSequence.bAckMsg = false;
	//stComposeSequence.enState = Compose_Service_NoExecute;

    char index_str[128] = {0};
	if (pNode->QueryValueAttribute("index", &index_str) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadSequence, get 'index' failed.\n");
		return -1;
	}
	TiXmlElement *pResult = pNode->FirstChildElement("request");
	if (pResult != NULL)
	{
		stComposeSequence.enSequenceType = Sequence_Type_Request;
		if (ReadRequest(pResult, stComposeSequence) != 0)
		{			
			return -1;
		}
	}
	else if ((pResult = pNode->FirstChildElement("response")) != NULL)
	{
		stComposeSequence.enSequenceType = Sequence_Type_Response;
		if (ReadResponse(pResult, stComposeSequence) != 0)
		{			
			return -1;
		}
	}
	else
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadSequence, get request/response failed,seq[%s]\n",index_str);
		return -1;
	}

    map<string, SequeceIndex>::const_iterator itr_seq_index = stComposeNode.mapSequenceIndex.find(index_str);
    if (itr_seq_index != stComposeNode.mapSequenceIndex.end())
    {
    	stComposeSequence.strSequenceName=index_str;
        stComposeNode.mapSeqs.insert(make_pair(itr_seq_index->second, stComposeSequence));
    }
    else
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadSequence, Can't find sequence index, seq index[%s]\n", index_str);
        return -1;
    }

	return 0;
}

int CComposeServiceContainer::ReadGoto(TiXmlElement *pNode, SComposeNode &stComposeNode)
{
	SComposeGoto stComposeGoto;

    char target_node_str[128] = {0};
	if (pNode->QueryValueAttribute("node", &target_node_str) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadGoto, get node failed.\n");
		return -1;
	}
    stComposeGoto.sTargetNode.strIndex = target_node_str;

    const char *szValue = pNode->GetText();
    if (szValue != NULL)
    {
        if (stComposeGoto.cCondition.setExpression(szValue) != 0)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadGoto, condition setExpression error. szValue[%s]\n", szValue);
            return -1;
        }
    }

	stComposeNode.vecResult.push_back(stComposeGoto);

	return 0;
}

int CComposeServiceContainer::ReadRequest(TiXmlElement *pNode, SComposeSequence &stComposeSequence)
{
	char szAttr[256] = {0};
	if (pNode->QueryValueAttribute("service", &szAttr) != TIXML_SUCCESS)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadRequest, get 'service' failed.\n");
		return -1;
	}
	stComposeSequence.strServiceName = string(szAttr);

	if (pNode->QueryValueAttribute("loop", &szAttr) != TIXML_SUCCESS)
	{
		stComposeSequence.sLoopTimes.strSourceKey = "1";  // default nLoopTimes 1
		stComposeSequence.sLoopTimes.enValueType=VTYPE_NOCASE_VALUE;
	}
	else
	{
		SValueInCompose& sValue=stComposeSequence.sLoopTimes;
		sValue.strSourceKey=string(szAttr);
		string::size_type index=string::npos;
		if (strncmp(sValue.strSourceKey.c_str(), "$rsp.", 5) == 0)
		{
			sValue.enValueType = VTYPE_RESPONSE;
			sValue.strSourceKey.erase(0, 5);

			index = sValue.strSourceKey.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, szAttr);
				return -1;
			}

			sValue.sNode.strIndex.assign(sValue.strSourceKey.c_str(), index);
			sValue.sSeq.strIndex.assign(sValue.strSourceKey.c_str() + index + 1);

			index = sValue.sSeq.strIndex.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, szAttr);
				return -1;
			}
			sValue.strSourceKey.assign(sValue.sSeq.strIndex.c_str() + index + 1);
			sValue.sSeq.strIndex.erase(index);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "$req.", 5) == 0)
		{
			sValue.enValueType = VTYPE_REQUEST;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "$", 1) == 0 && sValue.strSourceKey.size() > 1)
		{
			sValue.enValueType = VTYPE_DEF_VALUE;
			sValue.strSourceKey.erase(0, 1);
			if (!CheckDefValue(sValue.strSourceKey))
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, def value doesn't define. def value[%s]\n",__FUNCTION__, sValue.strSourceKey.c_str());
				return -1;
			}
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%req.", 5) == 0)
		{
			sValue.enValueType = VTYPE_DATA_REQUEST;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%rsp.", 5) == 0)
		{
			sValue.enValueType = VTYPE_DATA_RESPONSE;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%", 1) == 0 && sValue.strSourceKey.size() > 1)
		{
			sValue.enValueType = VTYPE_DATA_DEFVALUE;
			sValue.strSourceKey.erase(0, 1);
		}
		else
		{
			sValue.enValueType = VTYPE_NOCASE_VALUE;
		}
	}
	if (pNode->QueryValueAttribute("target", &szAttr) == TIXML_SUCCESS)
	{
		SValueInCompose& sValue=stComposeSequence.sSpSosAddr;
		sValue.strSourceKey=string(szAttr);
		string::size_type index=string::npos;
		if (strncmp(sValue.strSourceKey.c_str(), "$rsp.", 5) == 0)
		{
			sValue.enValueType = VTYPE_RESPONSE;
			sValue.strSourceKey.erase(0, 5);

			index = sValue.strSourceKey.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s,set request target Text failed. Text[%s]\n",__FUNCTION__, szAttr);
				return -1;
			}

			sValue.sNode.strIndex.assign(sValue.strSourceKey.c_str(), index);
			sValue.sSeq.strIndex.assign(sValue.strSourceKey.c_str() + index + 1);

			index = sValue.sSeq.strIndex.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, set request target Text failed. Text[%s]\n",__FUNCTION__, szAttr);
				return -1;
			}
			sValue.strSourceKey.assign(sValue.sSeq.strIndex.c_str() + index + 1);
			sValue.sSeq.strIndex.erase(index);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "$req.", 5) == 0)
		{
			sValue.enValueType = VTYPE_REQUEST;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "$", 1) == 0 && sValue.strSourceKey.size() > 1)
		{
			sValue.enValueType = VTYPE_DEF_VALUE;
			sValue.strSourceKey.erase(0, 1);
			if (!CheckDefValue(sValue.strSourceKey))
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, def value doesn't define. def value[%s]\n",
					__FUNCTION__, sValue.strSourceKey.c_str());
				return -1;
			}
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%req.", 5) == 0)
		{
			sValue.enValueType = VTYPE_DATA_REQUEST;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%rsp.", 5) == 0)
		{
			sValue.enValueType = VTYPE_DATA_RESPONSE;
			sValue.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sValue.strSourceKey.c_str(), "%", 1) == 0 && sValue.strSourceKey.size() > 1)
		{
			sValue.enValueType = VTYPE_DATA_DEFVALUE;
			sValue.strSourceKey.erase(0, 1);
		}
		else
		{
			sValue.enValueType = VTYPE_NOCASE_VALUE;
		}
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::ReadRequest, set request target sourceKey[%s].\n",szAttr);
	}

	if (pNode->QueryValueAttribute("timeout", &stComposeSequence.nTimeout) != TIXML_SUCCESS)
	{
		stComposeSequence.nTimeout = 3 * 1000;  // default timeout 3 second
	}

	if (pNode->QueryValueAttribute("checkexist", &szAttr) == TIXML_SUCCESS)
	{
		if (string("true") == szAttr)
		{
			stComposeSequence.bCheckServiceExist = true;
		}
	}

	if (pNode->QueryValueAttribute("isack", &szAttr) == TIXML_SUCCESS)
	{
		if (string("true") == szAttr)
		{
			stComposeSequence.bAckMsg = true;
		}
	}
	if (!stComposeSequence.bAckMsg)
	{
		SSeriveConfig *pSeriveConfig = NULL;
		SAvenueMessageConfig *pMessage = NULL;
		char szServiceName[128] = {0};
		char szMsgName[128] = {0};
		string strLowName(stComposeSequence.strServiceName); 
		transform(strLowName.begin(), strLowName.end(), strLowName.begin(), (int(*)(int))tolower);
		sscanf(strLowName.c_str(), "%[^.].%s", szServiceName, szMsgName);
		m_pServiceConfig->GetServiceByName(szServiceName, &pSeriveConfig);
		if (pSeriveConfig != NULL)
		{
			pSeriveConfig->oConfig.GetMessageTypeByName(szMsgName, &pMessage);
			if (pMessage != NULL)
			{
				stComposeSequence.bAckMsg = pMessage->bAckMsg;
			}
		}
	}

	for (TiXmlElement *pAssign = pNode->FirstChildElement("assign");
		pAssign != NULL; pAssign = pAssign->NextSiblingElement("assign"))
	{
		SValueInCompose valueInCompose;
		if (ReadValueInCompose(pAssign, valueInCompose) != 0)
		{	
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadRequest, get 'assign' failed\n");
			return -1;
		}
        if (valueInCompose.enValueType == VTYPE_RETURN_CODE)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadRequest, can't use code as the request parameter. Please use def value\n");
            return -1;
		}
		if (valueInCompose.enValueType == VTYPE_LOOP_RETURN_CODE)
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadRequest, can't use loopcode as the request parameter. Please use def value\n");
			return -1;
		}
		stComposeSequence.vecField.push_back(valueInCompose);
	}

	return 0;
}

int CComposeServiceContainer::ReadResponse(TiXmlElement *pNode, SComposeSequence &stComposeSequence)
{
	// <code node="1" sequence="0"></code>
	SReturnCode &sReturnCode = stComposeSequence.sReturnCode;

    TiXmlElement* pAssign = NULL;
    for (pAssign = pNode->FirstChildElement("assign");
        pAssign != NULL; pAssign = pAssign->NextSiblingElement("assign"))
    {
        const char *szValue = pAssign->GetText();
        if (szValue != NULL && strncmp(szValue, "$code=", 6) == 0)
            break;
    }

	if (pAssign)
	{
        string code_str = pAssign->GetText() + 6;

        string::size_type index = code_str.find("$req.");
        if (index != string::npos)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadResponse, can't assign req value to $code. text[%s]\n", code_str.c_str());
            return -1;
        }

        index = code_str.find("$rsp.");
        if (index != string::npos)
        {
            code_str.erase(0, index + 5);
            index = code_str.find(".");
            if (index != string::npos)
            {
                sReturnCode.enReturnType = Return_Type_Sequence;
                sReturnCode.sNodeId.strIndex.assign(code_str.c_str(), index);
                sReturnCode.sSequenceId.strIndex.assign(code_str.c_str() + index + 1);
            }
            else
            {
                SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadResponse, get node and sequence faild. text[%s]\n", code_str.c_str());
                return -1;
            }
        }
        else
        {
            if (strncmp("$lastcode", code_str.c_str(), 9) == 0)
            {
                sReturnCode.enReturnType = Return_Type_LastNode;
            }
            else if (strncmp("$", code_str.c_str(), 1) == 0)
            {
                sReturnCode.enReturnType = Return_Type_Code;
                sReturnCode.bIsDefValue  = true;
                sReturnCode.strValueName = code_str.c_str() + 1;
            }
            else
            {
                sReturnCode.enReturnType = Return_Type_Code;
		        sReturnCode.nReturnCode = atoi(code_str.c_str());
            }
		}
	}
	else
	{
		sReturnCode.enReturnType = Return_Type_LastNode;
	}

	stComposeSequence.nTimeout = 0;
	for (pAssign = pNode->FirstChildElement("assign");
		pAssign != NULL; pAssign = pAssign->NextSiblingElement("assign"))
	{
        const char *szValue = pAssign->GetText();
        if (szValue != NULL && strncmp(szValue, "$code", 5) == 0)
        {
            continue;
        }
		SValueInCompose valueInCompose;
		if (ReadValueInCompose(pAssign, valueInCompose) != 0)
		{	
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::ReadResponse failed\n");
			return -1;
		}
		stComposeSequence.vecField.push_back(valueInCompose);
	}
	return 0;
}

int CComposeServiceContainer::ReadValueInCompose(TiXmlElement *pNode, SValueInCompose &stValueInCompose)
{
    const char* node_text = pNode->GetText();
	if (!node_text)
	{
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node text.\n",__FUNCTION__);
        return -1;
	}

    stValueInCompose.enValueType = VTYPE_REQUEST;
    if (strncmp(node_text, "$this.", 6) == 0)
    {
        stValueInCompose.strTargetKey = node_text + 6;
    }
    else if (strncmp(node_text, "$xhead.", 7) == 0)
    {
        stValueInCompose.strTargetKey = node_text + 7;
        stValueInCompose.enTargetType = VTYPE_XHEAD;
	}
    else
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text is not use '$this' or '$xhead'. node_text[%s]\n",__FUNCTION__, node_text);
        return -1;
    }

    string::size_type index = stValueInCompose.strTargetKey.find("=");
    if (index == string::npos)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't find '=' in text. Text[%s]\n",
            __FUNCTION__, stValueInCompose.strTargetKey.c_str());
        return -1;
    }

    stValueInCompose.strSourceKey = stValueInCompose.strTargetKey.c_str() + index + 1;
    stValueInCompose.strTargetKey.erase(index);

	string::size_type loopPos= stValueInCompose.strSourceKey.find("[loopindex]");
	if(loopPos!=string::npos&&loopPos>0)
	{
		stValueInCompose.strSourceKey=stValueInCompose.strSourceKey.erase(loopPos);
		stValueInCompose.isLoop=true;
	}

    if (strncmp(stValueInCompose.strSourceKey.c_str(), "$rsp.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_RESPONSE;
        stValueInCompose.strSourceKey.erase(0, 5);

        index = stValueInCompose.strSourceKey.find(".");
        if (index == string::npos)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, node_text);
            return -1;
        }

        stValueInCompose.sNode.strIndex.assign(stValueInCompose.strSourceKey.c_str(), index);
        stValueInCompose.sSeq.strIndex.assign(stValueInCompose.strSourceKey.c_str() + index + 1);

        index = stValueInCompose.sSeq.strIndex.find(".");
        if (index == string::npos)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, node_text);
            return -1;
        }
        stValueInCompose.strSourceKey.assign(stValueInCompose.sSeq.strIndex.c_str() + index + 1);
        stValueInCompose.sSeq.strIndex.erase(index);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$req.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_REQUEST;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$lastcode", 9) == 0)
    {
        stValueInCompose.enValueType = VTYPE_LASTCODE;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$_remoteip", 10) == 0)
    {
        stValueInCompose.enValueType = VTYPE_REMOTEIP;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$code.", 6) == 0)
    {
        stValueInCompose.enValueType    = VTYPE_RETURN_CODE;
        stValueInCompose.sSeq.strIndex  = stValueInCompose.strSourceKey.c_str() + 6;
        stValueInCompose.sNode.strIndex = "this";
	}
	else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$loopcode.", 10) == 0)
	{
		stValueInCompose.enValueType    = VTYPE_LOOP_RETURN_CODE;
		stValueInCompose.sSeq.strIndex  = stValueInCompose.strSourceKey.c_str() + 10;
		stValueInCompose.sNode.strIndex = "this";
	}
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$", 1) == 0 && stValueInCompose.strSourceKey.size() > 1)
    {
        stValueInCompose.enValueType = VTYPE_DEF_VALUE;
        stValueInCompose.strSourceKey.erase(0, 1);
        if (!CheckDefValue(stValueInCompose.strSourceKey))
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, def value doesn't define. def value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
            return -1;
        }
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%req.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_REQUEST;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%rsp.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_RESPONSE;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%code", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_CODE;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%", 1) == 0 && stValueInCompose.strSourceKey.size() > 1)
    {
        stValueInCompose.enValueType = VTYPE_DATA_DEFVALUE;
        stValueInCompose.strSourceKey.erase(0, 1);
	}
	else if (stValueInCompose.strSourceKey.find("cpp_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_FUNCTION;
		stValueInCompose.strSourceKey.erase(0, 13);
		ReadValueInFunction(stValueInCompose);
	}
	else if (stValueInCompose.strSourceKey.find("lua_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read lua_function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_LUAFUNCTION;
		stValueInCompose.strSourceKey.erase(0, 13);
		ReadValueInFunction(stValueInCompose);
	}
	else if (stValueInCompose.strSourceKey.find("builtin_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read builtin_function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_BUILTINFUNCTION;
		stValueInCompose.strSourceKey.erase(0, 17);
		ReadValueInFunction(stValueInCompose);
	}
    else
    {
        stValueInCompose.enValueType = VTYPE_NOCASE_VALUE;
    }

    return 0;
}

int CComposeServiceContainer::ReadValueInResult(TiXmlElement *pNode, SValueInCompose &stValueInCompose)
{
    const char* node_text = pNode->GetText();
    if (!node_text)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed\n",__FUNCTION__);
        return -1;
    }
    if (strncmp(node_text, "$", 1) != 0)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text is not use '$'.node_text[%s]\n",__FUNCTION__, node_text);
        return -1;
    }

    stValueInCompose.enTargetType = VTYPE_DEF_VALUE;
    if (strncmp(node_text, "$xhead.", 7) == 0)
    {
        stValueInCompose.strTargetKey = node_text + 7;
        stValueInCompose.enTargetType = VTYPE_XHEAD;
    }
    else
    {
        stValueInCompose.strTargetKey = node_text + 1;
    }

    string::size_type index = stValueInCompose.strTargetKey.find("=");
    if (index == string::npos)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't find '=' in the text. Text[%s]\n",
            __FUNCTION__, node_text);
        return -1;
    }

    stValueInCompose.strSourceKey = stValueInCompose.strTargetKey.c_str() + index + 1;
    stValueInCompose.strTargetKey.erase(index);

    if (strncmp(stValueInCompose.strSourceKey.c_str(), "$rsp.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_RESPONSE;
        stValueInCompose.strSourceKey.erase(0, 5);

        index = stValueInCompose.strSourceKey.find(".");
        if (index == string::npos)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, node_text);
            return -1;
        }

        stValueInCompose.sNode.strIndex.assign(stValueInCompose.strSourceKey.c_str(), index);
        stValueInCompose.sSeq.strIndex.assign(stValueInCompose.strSourceKey.c_str() + index + 1);

        index = stValueInCompose.sSeq.strIndex.find(".");
        if (index == string::npos)
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text failed. Text[%s]\n",__FUNCTION__, node_text);
            return -1;
        }
        stValueInCompose.strSourceKey.assign(stValueInCompose.sSeq.strIndex.c_str() + index + 1);
		stValueInCompose.sSeq.strIndex.erase(index);
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s,execCppPlus read from response. Text[%s]\n",__FUNCTION__, node_text);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$req.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_REQUEST;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$lastcode", 9) == 0)
    {
        stValueInCompose.enValueType = VTYPE_LASTCODE;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$_remoteip", 10) == 0)
    {
        stValueInCompose.enValueType = VTYPE_REMOTEIP;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$code.", 6) == 0)
    {
        stValueInCompose.enValueType    = VTYPE_RETURN_CODE;
        stValueInCompose.sSeq.strIndex  = stValueInCompose.strSourceKey.c_str() + 6;
        stValueInCompose.sNode.strIndex = "this";
	}
	else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$loopcode.", 10) == 0)
	{
		stValueInCompose.enValueType    = VTYPE_LOOP_RETURN_CODE;
		stValueInCompose.sSeq.strIndex  = stValueInCompose.strSourceKey.c_str() + 10;
		stValueInCompose.sNode.strIndex = "this";
	}
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "$", 1) == 0 && stValueInCompose.strSourceKey.size() > 1)
    {
        stValueInCompose.enValueType = VTYPE_DEF_VALUE;
        stValueInCompose.strSourceKey.erase(0, 1);
        if (!CheckDefValue(stValueInCompose.strSourceKey))
        {
            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, def value doesn't define. def value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
            return -1;
        }
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%req.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_REQUEST;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%rsp.", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_RESPONSE;
        stValueInCompose.strSourceKey.erase(0, 5);
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%code", 5) == 0)
    {
        stValueInCompose.enValueType = VTYPE_DATA_CODE;
    }
    else if (strncmp(stValueInCompose.strSourceKey.c_str(), "%", 1) == 0 && stValueInCompose.strSourceKey.size() > 1)
    {
        stValueInCompose.enValueType = VTYPE_DATA_DEFVALUE;
        stValueInCompose.strSourceKey.erase(0, 1);
	}
	else if (stValueInCompose.strSourceKey.find("cpp_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_FUNCTION;
		stValueInCompose.strSourceKey.erase(0, 13);
		ReadValueInFunction(stValueInCompose);
	}
	else if (stValueInCompose.strSourceKey.find("lua_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read lua_function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_LUAFUNCTION;
		stValueInCompose.strSourceKey.erase(0, 13);
		ReadValueInFunction(stValueInCompose);
	}
	else if (stValueInCompose.strSourceKey.find("builtin_function.") == 0)
	{
		SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read builtin_function value[%s]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		stValueInCompose.enValueType = VTYPE_BUILTINFUNCTION;
		stValueInCompose.strSourceKey.erase(0, 17);
		ReadValueInFunction(stValueInCompose);
	}
    else
    {
        stValueInCompose.enValueType = VTYPE_NOCASE_VALUE;
    }

    return 0;
}

int CComposeServiceContainer::ReadValueInFunction(SValueInCompose &stValueInCompose)
{
	//if (stValueInCompose.enValueType!=VTYPE_FUNCTION)
	//{
	//	SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, ValueType is not VTYPE_FUNCTION\n",__FUNCTION__);
	//	return -1;
	//}
	if (stValueInCompose.strSourceKey.empty())
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text empty\n",__FUNCTION__);
		return -1;
	}
	string::size_type index_begin = stValueInCompose.strSourceKey.find_first_of('(');
	string::size_type index_end = stValueInCompose.strSourceKey.find_last_of(')');
	if (index_begin==string::npos
		||index_end==string::npos
		||index_begin>=index_end)
	{
		SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, Text is not a function [%s]\n",
			__FUNCTION__, stValueInCompose.strSourceKey.c_str());
		return -1;
	}
	string strParams;
	strParams.assign( stValueInCompose.strSourceKey.c_str() + index_begin+1, index_end - index_begin - 1);
	stValueInCompose.strSourceKey.erase(index_begin);
	vector<string> vecParams;
	boost::algorithm::split( vecParams, strParams, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
	for (vector<string>::iterator itr = vecParams.begin();itr != vecParams.end(); ++itr)
	{
		boost::trim(*itr);
		if (itr->empty())
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get function param[%s].\n",__FUNCTION__,strParams.c_str());
			return -1;
		}
		SFunctionParam sFunctionParam;

		sFunctionParam.enValueType = VTYPE_REQUEST;
		sFunctionParam.strSourceKey=*itr;
		if (strncmp(sFunctionParam.strSourceKey.c_str(), "$rsp.", 5) == 0)
		{
			sFunctionParam.enValueType = VTYPE_RESPONSE;
			sFunctionParam.strSourceKey.erase(0, 5);

			string::size_type index = sFunctionParam.strSourceKey.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, function param failed. Text[%s]\n",__FUNCTION__, strParams.c_str());
				return -1;
			}

			sFunctionParam.sNode.strIndex.assign(sFunctionParam.strSourceKey.c_str(), index);
			sFunctionParam.sSeq.strIndex.assign(sFunctionParam.strSourceKey.c_str() + index + 1);

			index = sFunctionParam.sSeq.strIndex.find(".");
			if (index == string::npos)
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, function param failed. Text[%s]\n",__FUNCTION__, strParams.c_str());
				return -1;
			}
			sFunctionParam.strSourceKey.assign(sFunctionParam.sSeq.strIndex.c_str() + index + 1);
			sFunctionParam.sSeq.strIndex.erase(index);
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "$req.", 5) == 0)
		{
			sFunctionParam.enValueType = VTYPE_REQUEST;
			sFunctionParam.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "$lastcode", 9) == 0)
		{
			sFunctionParam.enValueType = VTYPE_LASTCODE;
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "$_remoteip", 10) == 0)
		{
			sFunctionParam.enValueType = VTYPE_REMOTEIP;
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "$code.", 6) == 0)
		{
			sFunctionParam.enValueType    = VTYPE_RETURN_CODE;
			sFunctionParam.sSeq.strIndex  = sFunctionParam.strSourceKey.c_str() + 6;
			sFunctionParam.sNode.strIndex = "this";
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "$", 1) == 0 && sFunctionParam.strSourceKey.size() > 1)
		{
			sFunctionParam.enValueType = VTYPE_DEF_VALUE;
			sFunctionParam.strSourceKey.erase(0, 1);
			if (!CheckDefValue(sFunctionParam.strSourceKey))
			{
				SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, def value doesn't define. def value[%s]\n",__FUNCTION__, sFunctionParam.strSourceKey.c_str());
				return -1;
			}
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "%req.", 5) == 0)
		{
			sFunctionParam.enValueType = VTYPE_DATA_REQUEST;
			sFunctionParam.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "%rsp.", 5) == 0)
		{
			sFunctionParam.enValueType = VTYPE_DATA_RESPONSE;
			sFunctionParam.strSourceKey.erase(0, 5);
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "%code", 5) == 0)
		{
			sFunctionParam.enValueType = VTYPE_DATA_CODE;
		}
		else if (strncmp(sFunctionParam.strSourceKey.c_str(), "%", 1) == 0 && sFunctionParam.strSourceKey.size() > 1)
		{
			sFunctionParam.enValueType = VTYPE_DATA_DEFVALUE;
			sFunctionParam.strSourceKey.erase(0, 1);
		}
		else
		{
			sFunctionParam.enValueType = VTYPE_NOCASE_VALUE;
		}
		stValueInCompose.sFunctionParams.push_back(sFunctionParam);
	}
	SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s, execCppFunc read function funcName[%s] paramNum[%d]\n",__FUNCTION__, stValueInCompose.strSourceKey.c_str(),stValueInCompose.sFunctionParams.size());
	return 0;
}

void CComposeServiceContainer::Dump()
{
    SS_XLOG(XLOG_TRACE,"-------------- CComposeServiceContainer::%s Start\n",__FUNCTION__);
    for (map<SComposeKey, SComposeServiceUnit>::iterator itrUnit = m_mapComposeUnit.begin();
        itrUnit != m_mapComposeUnit.end(); ++itrUnit)
    {
        SS_XLOG(XLOG_TRACE,"-- Compose service unit -----\n");
        SComposeServiceUnit& sCSUnit = itrUnit->second;
        SS_XLOG(XLOG_TRACE,"  Unit name[%s], key[%d,%d] isDataCompose[%s] dataServer size[%u]\n", sCSUnit.strServiceName.c_str(),
            sCSUnit.sKey.dwServiceId, sCSUnit.sKey.dwMsgId, sCSUnit.isDataCompose ? "Y" : "N", sCSUnit.vecDataServerId.size());

        SS_XLOG(XLOG_TRACE,"-- Node def value -----------\n");
        map<string, SDefParameter>::const_iterator itr_node_def = sCSUnit.mapDefValue.begin();
        for (; itr_node_def !=  sCSUnit.mapDefValue.end(); ++itr_node_def)
        {
            SS_XLOG(XLOG_TRACE,"  Name[%s], Type[%d], Default value[%s]\n", itr_node_def->first.c_str(), 
                itr_node_def->second.enType, itr_node_def->second.strDefaultValue.c_str());
        }

	    for (map<NodeIndex, SComposeNode>::iterator itrNode = sCSUnit.mapNodes.begin();
		    itrNode!=sCSUnit.mapNodes.end(); ++itrNode)
	    {
            SS_XLOG(XLOG_TRACE,"---- Compose Node -----------\n");
		    SComposeNode &sNode = itrNode->second;
		    SS_XLOG(XLOG_TRACE,"  Node index[%s][%d]\n", sNode.strNodeName.c_str(), itrNode->first);

		    for (map<SequeceIndex, SComposeSequence>::iterator itrSeq = sNode.mapSeqs.begin();
			    itrSeq != sNode.mapSeqs.end(); ++itrSeq)
		    {
                SS_XLOG(XLOG_TRACE,"------ Sequence Node --------\n");
			    SComposeSequence &scSeq = itrSeq->second;
			    SS_XLOG(XLOG_TRACE,"  Sequence index[%s][%d], Type[%d], Name[%s]\n", 
				    getSequenceName(sCSUnit, itrNode->first, itrSeq->first), itrSeq->first, scSeq.enSequenceType, scSeq.strServiceName.c_str());
                SS_XLOG(XLOG_TRACE,"------ Field ----------------\n");
			    for (vector<SValueInCompose>::iterator iterFld = scSeq.vecField.begin();
				    iterFld != scSeq.vecField.end(); ++iterFld)
			    {
				    SValueInCompose& sValue = *iterFld;
				    SS_XLOG(XLOG_TRACE,"  Target[%s], Source[%s], Node[%s][%d], Seq[%s][%d], ValueType[%d], TargetType[%d]\n",
				        sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(), sValue.sNode.nIndex, 
                        sValue.sSeq.strIndex.c_str(), sValue.sSeq.nIndex, sValue.enValueType, sValue.enTargetType);
			    }
		    }

            SS_XLOG(XLOG_TRACE,"---- Result assign-----------\n");
		    for (vector<SValueInCompose>::iterator iterResult = sNode.vecResultValues.begin();
			    iterResult != sNode.vecResultValues.end(); ++iterResult)
		    {
			    SValueInCompose& sValue = *iterResult;
                SS_XLOG(XLOG_TRACE,"  Target[%s], Source[%s], Node[%s][%d], Seq[%s][%d], ValueType[%d], TargetType[%d]\n",
                    sValue.strTargetKey.c_str(),sValue.strSourceKey.c_str(),sValue.sNode.strIndex.c_str(), sValue.sNode.nIndex, 
                    sValue.sSeq.strIndex.c_str(), sValue.sSeq.nIndex, sValue.enValueType, sValue.enTargetType);
		    }

            SS_XLOG(XLOG_TRACE,"---- Goto -------------------\n");
            for (vector<SComposeGoto>::iterator iterGoto = sNode.vecResult.begin();
                iterGoto != sNode.vecResult.end(); ++iterGoto)
            {
                SComposeGoto& scgoto = *iterGoto;
                SS_XLOG(XLOG_TRACE,"  Target Node[%s][%d]\n", scgoto.sTargetNode.strIndex.c_str(), scgoto.sTargetNode.nIndex);
            }
	    }
    }
    SS_XLOG(XLOG_TRACE,"-------------- CComposeServiceContainer::%s End\n",__FUNCTION__);
}

int CComposeServiceContainer::GenerateId(SComposeServiceUnit &stComposeServiceUnit)
{
    map<NodeIndex, SComposeNode>::iterator    itr_nodes = stComposeServiceUnit.mapNodes.begin();

    for (; itr_nodes != stComposeServiceUnit.mapNodes.end(); ++itr_nodes)
    {
        vector<SValueInCompose>::iterator itr_value_result = itr_nodes->second.vecResultValues.begin();
        for (; itr_value_result != itr_nodes->second.vecResultValues.end(); ++ itr_value_result)
        {
            itr_value_result->sNode.nIndex = -1;
            itr_value_result->sSeq.nIndex  = -1;

			 if (itr_value_result->enValueType == VTYPE_FUNCTION) 
			 {
				 vector<SFunctionParam>::iterator itr_func_param=itr_value_result->sFunctionParams.begin();
				 for (; itr_func_param != itr_value_result->sFunctionParams.end(); ++ itr_func_param)
				 {
					 itr_func_param->sNode.nIndex = -1;
					 itr_func_param->sSeq.nIndex  = -1;
					 if (itr_func_param->enValueType != VTYPE_RESPONSE&&
						 itr_func_param->enValueType != VTYPE_RETURN_CODE)
					 {
						 continue;
					 }
					 if (strncmp(itr_func_param->sNode.strIndex.c_str(), "this", 4) == 0)
					 {
						 itr_func_param->sNode.nIndex = itr_nodes->first;
					 }
					 else
					 {
						 map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_func_param->sNode.strIndex);
						 if (itr_node_index == stComposeServiceUnit.mapNodeIndex.end())
						 {
							 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get result node index. node[%s], sequence[%s], SourceKey[%s]\n",
								 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sSeq.strIndex.c_str(), itr_func_param->strSourceKey.c_str());
							 return -1;
						 }
						 itr_func_param->sNode.nIndex = itr_node_index->second;
					 }

					 map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_func_param->sNode.nIndex);
					 if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
					 {
						 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
							 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sNode.nIndex);
						 return -1;
					 }
					 map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_func_param->sSeq.strIndex);
					 if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
					 {
						 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get result sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
							 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sSeq.strIndex.c_str(), itr_func_param->strSourceKey.c_str());
						 return -1;
					 }
					 itr_func_param->sSeq.nIndex = itr_seq_index->second;
				 }

			 }


            if (itr_value_result->enValueType != VTYPE_RESPONSE &&
				itr_value_result->enValueType != VTYPE_RETURN_CODE&&
				itr_value_result->enValueType != VTYPE_LOOP_RETURN_CODE)
            {
                continue;
            }

            if (strncmp(itr_value_result->sNode.strIndex.c_str(), "this", 4) == 0)
            {
                itr_value_result->sNode.nIndex = itr_nodes->first;
            }
            else
            {
                map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_value_result->sNode.strIndex);
                if (itr_node_index == stComposeServiceUnit.mapNodeIndex.end())
                {
                    SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get result node index. node[%s], sequence[%s], SourceKey[%s]\n",
                        __FUNCTION__, itr_value_result->sNode.strIndex.c_str(), itr_value_result->sSeq.strIndex.c_str(), itr_value_result->strSourceKey.c_str());
                    return -1;
                }
                itr_value_result->sNode.nIndex = itr_node_index->second;
            }

            map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_value_result->sNode.nIndex);
            if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
            {
                SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
                    __FUNCTION__, itr_value_result->sNode.strIndex.c_str(), itr_value_result->sNode.nIndex);
                return -1;
            }
            map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_value_result->sSeq.strIndex);
            if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
            {
                SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get result sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
                    __FUNCTION__, itr_value_result->sNode.strIndex.c_str(), itr_value_result->sSeq.strIndex.c_str(), itr_value_result->strSourceKey.c_str());
                return -1;
            }
            itr_value_result->sSeq.nIndex = itr_seq_index->second;
        }

        vector<SComposeGoto>::iterator itr_result = itr_nodes->second.vecResult.begin();
        for (; itr_result != itr_nodes->second.vecResult.end(); ++itr_result)
        {
            itr_result->sTargetNode.nIndex = 0;
            if (itr_result->sTargetNode.strIndex.compare("this") == 0)
            {
                itr_result->sTargetNode.nIndex = itr_nodes->first;
            }
            else
            {
                map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_result->sTargetNode.strIndex);
                if (itr_node_index == stComposeServiceUnit.mapNodeIndex.end())
                {
                    SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get target node index. node[%s]\n",
                        __FUNCTION__, itr_result->sTargetNode.strIndex.c_str());
                    return -1;
                }
                itr_result->sTargetNode.nIndex = itr_node_index->second;
            }
        }

        map<SequeceIndex, SComposeSequence>::iterator itr_seqs = itr_nodes->second.mapSeqs.begin();
        for (; itr_seqs != itr_nodes->second.mapSeqs.end(); ++itr_seqs)
        {
            SReturnCode& sReturnCode = itr_seqs->second.sReturnCode;
            if (sReturnCode.enReturnType == Return_Type_Sequence)
            {
                if (!sReturnCode.bIsDefValue)
                {
                    if (strncmp(sReturnCode.sNodeId.strIndex.c_str(), "this", 4) == 0)
                    {
                        sReturnCode.sNodeId.nIndex = itr_nodes->first;
                    }
                    else
                    {
                        map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(sReturnCode.sNodeId.strIndex);
                        if ( itr_node_index == stComposeServiceUnit.mapNodeIndex.end() )
                        {
                            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get return code node index. node[%s], sequence[%s], value name[%s]\n",
                                __FUNCTION__, sReturnCode.sNodeId.strIndex.c_str(), sReturnCode.sSequenceId.strIndex.c_str(), sReturnCode.strValueName.c_str());
                            return -1;
                        }
                        sReturnCode.sNodeId.nIndex = itr_node_index->second;
                    }

                    map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(sReturnCode.sNodeId.nIndex);
                    if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
                            __FUNCTION__, sReturnCode.sNodeId.strIndex.c_str(), sReturnCode.sNodeId.nIndex);
                        return -1;
                    }
                    map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(sReturnCode.sSequenceId.strIndex);
                    if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get return code sequence index. node[%s], sequence[%s], value name[%s]\n",
                            __FUNCTION__, sReturnCode.sNodeId.strIndex.c_str(), sReturnCode.sSequenceId.strIndex.c_str(), sReturnCode.strValueName.c_str());
                        return -1;
                    }
                    sReturnCode.sSequenceId.nIndex = itr_seq_index->second;
                }
            }

            vector<SValueInCompose>::iterator itr_value = itr_seqs->second.vecField.begin();
            for (; itr_value != itr_seqs->second.vecField.end(); ++ itr_value)
            {
                itr_value->sNode.nIndex = -1;
                itr_value->sSeq.nIndex  = -1;


				if (itr_value->enValueType == VTYPE_FUNCTION) 
				 {
					 vector<SFunctionParam>::iterator itr_func_param=itr_value->sFunctionParams.begin();
					 for (; itr_func_param != itr_value->sFunctionParams.end(); ++ itr_func_param)
					 {
						 itr_func_param->sNode.nIndex = -1;
						 itr_func_param->sSeq.nIndex  = -1;
						 if (itr_func_param->enValueType != VTYPE_RESPONSE &&
							 itr_func_param->enValueType != VTYPE_RETURN_CODE)
						 {
							 continue;
						 }

						 if (strncmp(itr_func_param->sNode.strIndex.c_str(), "this", 4) == 0)
						 {
							 itr_func_param->sNode.nIndex = itr_nodes->first;
						 }
						 else
						 {
							 map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_func_param->sNode.strIndex);
							 if ( itr_node_index == stComposeServiceUnit.mapNodeIndex.end() )
							 {
								 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value node index. node[%s], sequence[%s], SourceKey[%s]\n",
									 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sSeq.strIndex.c_str(), itr_func_param->strSourceKey.c_str());
								 return -1;
							 }

							 itr_func_param->sNode.nIndex = itr_node_index->second;
						 }

						 map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_func_param->sNode.nIndex);
						 if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
						 {
							 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
								 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sNode.nIndex);
							 return -1;
						 }
						 map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_func_param->sSeq.strIndex);
						 if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
						 {
							 SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
								 __FUNCTION__, itr_func_param->sNode.strIndex.c_str(), itr_func_param->sSeq.strIndex.c_str(), itr_func_param->strSourceKey.c_str());
							 return -1;
						 }
						 itr_func_param->sSeq.nIndex = itr_seq_index->second;
					 }
				 }


                if (itr_value->enValueType != VTYPE_RESPONSE &&
					itr_value->enValueType != VTYPE_RETURN_CODE&&
					itr_value->enValueType != VTYPE_LOOP_RETURN_CODE)
                {
                    continue;
                }

                if (strncmp(itr_value->sNode.strIndex.c_str(), "this", 4) == 0)
                {
                    itr_value->sNode.nIndex = itr_nodes->first;
                }
                else
                {
                    map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_value->sNode.strIndex);
                    if ( itr_node_index == stComposeServiceUnit.mapNodeIndex.end() )
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value node index. node[%s], sequence[%s], SourceKey[%s]\n",
                            __FUNCTION__, itr_value->sNode.strIndex.c_str(), itr_value->sSeq.strIndex.c_str(), itr_value->strSourceKey.c_str());
                        return -1;
                    }
 
                    itr_value->sNode.nIndex = itr_node_index->second;
                }

                map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_value->sNode.nIndex);
                if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
                {
                    SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
                        __FUNCTION__, itr_value->sNode.strIndex.c_str(), itr_value->sNode.nIndex);
                    return -1;
                }
                map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_value->sSeq.strIndex);
                if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
                {
                    SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
                        __FUNCTION__, itr_value->sNode.strIndex.c_str(), itr_value->sSeq.strIndex.c_str(), itr_value->strSourceKey.c_str());
                    return -1;
                }
                itr_value->sSeq.nIndex = itr_seq_index->second;
            }

			//GenerateloopTimes begin
			itr_seqs->second.sLoopTimes.sNode.nIndex = -1;
			itr_seqs->second.sLoopTimes.sSeq.nIndex  = -1;

			if (itr_seqs->second.sLoopTimes.enValueType == VTYPE_RESPONSE)
			{
				if (strncmp(itr_seqs->second.sLoopTimes.sNode.strIndex.c_str(), "this", 4) == 0)
				{
					itr_seqs->second.sLoopTimes.sNode.nIndex = itr_nodes->first;
				}
				else
				{
					map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_seqs->second.sLoopTimes.sNode.strIndex);
					if ( itr_node_index == stComposeServiceUnit.mapNodeIndex.end() )
					{
						SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value node index. node[%s], sequence[%s], SourceKey[%s]\n",
							__FUNCTION__, itr_seqs->second.sLoopTimes.sNode.strIndex.c_str(), itr_seqs->second.sLoopTimes.sSeq.strIndex.c_str(), itr_seqs->second.sLoopTimes.strSourceKey.c_str());
						return -1;
					}

					itr_seqs->second.sLoopTimes.sNode.nIndex = itr_node_index->second;
				}

				map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_seqs->second.sLoopTimes.sNode.nIndex);
				if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
				{
					SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
						__FUNCTION__, itr_seqs->second.sLoopTimes.sNode.strIndex.c_str(), itr_seqs->second.sLoopTimes.sNode.nIndex);
					return -1;
				}
				map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_seqs->second.sLoopTimes.sSeq.strIndex);
				if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
				{
					SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
						__FUNCTION__, itr_seqs->second.sLoopTimes.sNode.strIndex.c_str(), itr_seqs->second.sLoopTimes.sSeq.strIndex.c_str(), itr_seqs->second.sLoopTimes.strSourceKey.c_str());
					return -1;
				}
				itr_seqs->second.sLoopTimes.sSeq.nIndex = itr_seq_index->second;
			}
			//GenerateLoopTimes end

			//GenerateSpSosAddr begin
			itr_seqs->second.sSpSosAddr.sNode.nIndex = -1;
			itr_seqs->second.sSpSosAddr.sSeq.nIndex  = -1;

			if (itr_seqs->second.sSpSosAddr.enValueType == VTYPE_RESPONSE)
			{
				if (strncmp(itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), "this", 4) == 0)
				{
					itr_seqs->second.sSpSosAddr.sNode.nIndex = itr_nodes->first;
				}
				else
				{
					map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(itr_seqs->second.sSpSosAddr.sNode.strIndex);
					if ( itr_node_index == stComposeServiceUnit.mapNodeIndex.end() )
					{
						SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value node index. node[%s], sequence[%s], SourceKey[%s]\n",
							__FUNCTION__, itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), itr_seqs->second.sSpSosAddr.sSeq.strIndex.c_str(), itr_seqs->second.sSpSosAddr.strSourceKey.c_str());
						return -1;
					}

					itr_seqs->second.sSpSosAddr.sNode.nIndex = itr_node_index->second;
					SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s,  get value node index[%d]. node[%s], sequence[%s], SourceKey[%s]\n",__FUNCTION__,itr_seqs->second.sSpSosAddr.sNode.nIndex, itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), itr_seqs->second.sSpSosAddr.sSeq.strIndex.c_str(), itr_seqs->second.sSpSosAddr.strSourceKey.c_str());
				}

				map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(itr_seqs->second.sSpSosAddr.sNode.nIndex);
				if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
				{
					SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
						__FUNCTION__, itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), itr_seqs->second.sSpSosAddr.sNode.nIndex);
					return -1;
				}
				map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(itr_seqs->second.sSpSosAddr.sSeq.strIndex);
				if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
				{
					SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get value sequence index. node[%s], sequence[%s], SourceKey[%s]\n",
						__FUNCTION__, itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), itr_seqs->second.sSpSosAddr.sSeq.strIndex.c_str(), itr_seqs->second.sSpSosAddr.strSourceKey.c_str());
					return -1;
				}
				itr_seqs->second.sSpSosAddr.sSeq.nIndex = itr_seq_index->second;
				SS_XLOG(XLOG_DEBUG,"CComposeServiceContainer::%s,  get value sequence index[%d]. node[%s], sequence[%s], SourceKey[%s]\n",__FUNCTION__,itr_seqs->second.sSpSosAddr.sSeq.nIndex, itr_seqs->second.sSpSosAddr.sNode.strIndex.c_str(), itr_seqs->second.sSpSosAddr.sSeq.strIndex.c_str(), itr_seqs->second.sSpSosAddr.strSourceKey.c_str());
			}
			//GenerateSpSosAddr end
        }
    }

    return 0;
}

int CComposeServiceContainer::GenerateNodeId(TiXmlElement *pNode,SComposeServiceUnit &stComposeServiceUnit)
{
    char index_str[128] = {0};
    if (pNode->QueryValueAttribute("index", &index_str) != TIXML_SUCCESS)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateNodeId, get index failed.\n");
        return -1;
    }
    if (stComposeServiceUnit.mapNodeIndex.find(index_str) == stComposeServiceUnit.mapNodeIndex.end())
    {
        stComposeServiceUnit.mapNodeIndex.insert(make_pair(index_str, stComposeServiceUnit.mapNodeIndex.size() - 1));
    }
    return 0;
}

int CComposeServiceContainer::GenerateSequenceId(TiXmlElement *pNode,SComposeNode &stComposeNode)
{
    char index_str[128] = {0};
    if (pNode->QueryValueAttribute("index", &index_str) != TIXML_SUCCESS)
    {
        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateSequenceId, get index failed.\n");
        return -1;
    }
    if (stComposeNode.mapSequenceIndex.find(index_str) == stComposeNode.mapSequenceIndex.end())
    {
        stComposeNode.mapSequenceIndex.insert(make_pair(index_str, stComposeNode.mapSequenceIndex.size()));
    }
    return 0;
}

int CComposeServiceContainer::GenerateConditionParam(SComposeServiceUnit& stComposeServiceUnit)
{
    vector<SComposeGoto>::iterator itr_goto;
	map<NodeIndex, SComposeNode>::iterator itr_node = stComposeServiceUnit.mapNodes.begin();
    for (; itr_node != stComposeServiceUnit.mapNodes.end(); ++itr_node)
	{
        for (itr_goto = itr_node->second.vecResult.begin();
            itr_goto != itr_node->second.vecResult.end(); ++itr_goto)
		{
            const vector<string>& param_names = itr_goto->cCondition.getParaNames();

            vector<string>::const_iterator itr_param = param_names.begin();
            for (; itr_param != param_names.end(); ++itr_param)
            {
                const string& param_str = *itr_param;
				SValueInCompose value_compose;
                if (param_str.find("$code.") == 0)
                {
                    string seq_str = param_str.c_str() + 6;
                    map<string, SequeceIndex>::const_iterator itr_seq_index = itr_node->second.mapSequenceIndex.find(seq_str);
                    if ( itr_seq_index == itr_node->second.mapSequenceIndex.end())
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, can't find sequence index. parameter[%s]\n", param_str.c_str());
                        return -1;
                    }
                    value_compose.enValueType  = VTYPE_RETURN_CODE;
                    value_compose.sSeq.nIndex  = itr_seq_index->second;
                    value_compose.sNode.nIndex = itr_node->first;
                }
                else if (param_str.find("&lastcode") == 0)
                {
                    value_compose.enValueType = VTYPE_LASTCODE;
                }
                else if (param_str.find("&_remoteip") == 0)
                {
                    value_compose.enValueType = VTYPE_REMOTEIP;
                }
                else if (param_str.find("$req.") == 0)
                {
                    value_compose.strSourceKey = param_str.c_str() + 5;
                    value_compose.enValueType  = VTYPE_REQUEST;
                }
                else if (param_str.find("$rsp.") == 0)
                {
                    string node_str = param_str.c_str() + 5;
					string seq_str;
                    string::size_type index = node_str.find(".");

                    if (index == string::npos)
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, parmeter is error. parameter[%s]\n", param_str.c_str());
                        return -1;
                    }

                    seq_str = node_str.c_str() + index + 1;
                    node_str.erase(index);

                    index = seq_str.find(".");
                    if (index == string::npos)
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, parmeter is error. parameter[%s]\n", param_str.c_str());
                        return -1;
                    }

                    value_compose.strSourceKey = seq_str.c_str() + index + 1;
                    seq_str.erase(index);

                    if (strncmp(node_str.c_str(), "this", 4) == 0)
                    {
                        value_compose.sNode.nIndex = itr_node->first;
                    }
                    else
                    {
                        map<string, NodeIndex>::const_iterator itr_node_index = stComposeServiceUnit.mapNodeIndex.find(node_str);
                        if (itr_node_index == stComposeServiceUnit.mapNodeIndex.end())
                        {
                            SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, can't find node index. parameter[%s]\n", param_str.c_str());
                            return -1;
                        }
                        value_compose.sNode.nIndex = itr_node_index->second;
                    }

                    map<NodeIndex, SComposeNode>::const_iterator itr_tmp_node_index = stComposeServiceUnit.mapNodes.find(value_compose.sNode.nIndex);
                    if (itr_tmp_node_index == stComposeServiceUnit.mapNodes.end())
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, can't get node index. node[%s], index[%d]\n",
                            __FUNCTION__, value_compose.sNode.strIndex.c_str(), value_compose.sNode.nIndex);
                        return -1;
                    }
                    map<string, SequeceIndex>::const_iterator itr_seq_index = itr_tmp_node_index->second.mapSequenceIndex.find(seq_str);
                    if (itr_seq_index == itr_tmp_node_index->second.mapSequenceIndex.end())
                    {
                        SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, can't find sequence node. parameter[%s]\n", param_str.c_str());
                        return -1;
                    }

                    value_compose.sSeq.nIndex = itr_seq_index->second;
                    value_compose.enValueType = VTYPE_RESPONSE;
                }
                else if (param_str.find("%req.") == 0)
                {
                    value_compose.strSourceKey = param_str.c_str() + 5;
                    value_compose.enValueType = VTYPE_DATA_REQUEST;
                }
                else if (param_str.find("%rsp.") == 0)
                {
                    value_compose.strSourceKey = param_str.c_str() + 5;
                    value_compose.enValueType = VTYPE_DATA_RESPONSE;
                }
                else if (param_str.find("%code") == 0)
                {
                    value_compose.enValueType = VTYPE_DATA_CODE;
                }
                else if (param_str.find("%") == 0)
                {
                    value_compose.strSourceKey = param_str.c_str() + 1;
                    value_compose.enValueType = VTYPE_DATA_DEFVALUE;
                }
                else
				{
                    if (param_str.find(".") != string::npos)
                    {
                    //    SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::GenerateConditionParam, parmeter is error. parameter[%s]\n", param_str.c_str());
                    //    return -1;
                    }
                    value_compose.enValueType  = VTYPE_DEF_VALUE;
                    value_compose.strSourceKey = param_str.c_str() + 1;
                }

                itr_goto->vecConditionParam.push_back(value_compose);
            }
        }
    }

    return 0;
}


string CComposeServiceContainer::OnGetConfigInfo()
{
    SS_XLOG(XLOG_DEBUG, "CComposeServiceContainer::%s...\n", __FUNCTION__);
    string strConfigInfo("<TABLE border=0 cellSpacing=1 cellPadding=0 width=60% align=center valign=middle>");
    strConfigInfo += "<tr bgcolor=#333333 height=26 align=center><td><font color=#dddddd>No.</font></td>" \
        "<td><font color=#dddddd>ServiceId</font></td>" \
        "<td><font color=#dddddd>MsgId</td>"    \
        "<td><font color=#dddddd>ServiceName</font></td></tr>";

    unsigned index       = 0;
    char     tmp_str[64] = {0};
    map<SComposeKey, SComposeServiceUnit>::const_iterator itr_unit = m_mapComposeUnit.begin();
    for (; itr_unit != m_mapComposeUnit.end(); ++itr_unit)
    {
        strConfigInfo += "<tr bgColor=#cccccc height=26>";
        strConfigInfo += "<td><font color=#000000>";
        sprintf(tmp_str, "%u", ++index);
        strConfigInfo += tmp_str;
        strConfigInfo += "</font></td>";
        strConfigInfo += "<td><font color=#000000>";
        sprintf(tmp_str, "%u", itr_unit->first.dwServiceId);
        strConfigInfo += tmp_str;
        strConfigInfo += "</font></td>";
        strConfigInfo += "<td><font color=#000000>";
        sprintf(tmp_str, "%u", itr_unit->first.dwMsgId);
        strConfigInfo += tmp_str;
        strConfigInfo += "</font></td>";
        strConfigInfo += "<td><font color=#000000>";
        strConfigInfo += itr_unit->second.strServiceName;
        strConfigInfo += "</font></td>";
        strConfigInfo += "</tr>";
    }
    strConfigInfo += "</table>";
    return strConfigInfo;
}

const char* getNodeName(const SComposeServiceUnit &stComposeServiceUnit, NodeIndex node_index_)
{
    map<NodeIndex, SComposeNode>::const_iterator itr = stComposeServiceUnit.mapNodes.find(node_index_);
    if (itr != stComposeServiceUnit.mapNodes.end())
    {
        return itr->second.strNodeName.c_str();
    }
    return "NULL";
}
const char* getSequenceName(const SComposeServiceUnit &stComposeServiceUnit, NodeIndex node_index_, SequeceIndex seq_index_)
{
    map<NodeIndex, SComposeNode>::const_iterator itr = stComposeServiceUnit.mapNodes.find(node_index_);
    if (itr != stComposeServiceUnit.mapNodes.end())
    {
        map<SequeceIndex, SComposeSequence>::const_iterator itr_seq = itr->second.mapSeqs.find(seq_index_);
        if (itr_seq != itr->second.mapSeqs.end())
        {
            return itr_seq->second.strSequenceName.c_str();
        }
    }
    return "NULL";
}

bool CComposeServiceContainer::CheckDefValue(const string& def_value_)
{
	if(boost::icontains(def_value_,"."))
	{
		vector<string> vecSubParams;
		boost::split(vecSubParams,def_value_,boost::algorithm::is_any_of("."),boost::algorithm::token_compress_on);
		if (vecSubParams.size()==0)
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s, NULL \n", __FUNCTION__);
			return false;
		}
		map<string, SDefParameter>::const_iterator it = m_current_unit->mapDefValue.find(vecSubParams[0]);
		if (it== m_current_unit->mapDefValue.end())
		{
			SS_XLOG(XLOG_WARNING,"CComposeServiceContainer::%s,  %s is not meta type\n", __FUNCTION__,def_value_.c_str());
			return false;
		}
		return it->second.enType==sdo::commonsdk::MSG_FIELD_META;
	}
	else
	{
		return m_current_unit->mapDefValue.find(def_value_) != m_current_unit->mapDefValue.end();
	}
}

