#include "dbserviceconfig.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "Cipher.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using std::make_pair;
using std::map;
using namespace sdo::dbp;

void HanleNIL(string& s)
{
	if (strcmp(s.c_str(),"NIL")==0)
	{
		s = "";
	}
}

CDBServiceConfig* CDBServiceConfig::sm_pInstance = NULL;


CDBServiceConfig* CDBServiceConfig::GetInstance()
{
	static CDBServiceConfig gServiceConfig;
	if(sm_pInstance == NULL)
        sm_pInstance = &gServiceConfig;
    return sm_pInstance;
}

void MakeSQLToLower(string& strOriSQL)
{
	string strSQL = strOriSQL;
	vector<string> vecParamsLower;
	vector<string> vecParamsOri;
	size_t found=strSQL.find(":");;
    while(found != string::npos)
    {
	    found=strSQL.find(":");
	    if(found != string::npos)
	    {
	        strSQL.erase(0,found);

	        found=strSQL.find_first_of(" ,)");
			string sTmp = strSQL.substr(0,found);
			vecParamsOri.push_back(sTmp);
			boost::to_lower(sTmp);
	        vecParamsLower.push_back(sTmp);
	        strSQL.erase(0,found);
	    }
    }
	vector<string>::iterator itBinderPara = vecParamsLower.begin();
	vector<string>::iterator itBinderParaOri = vecParamsOri.begin();
	for(;itBinderPara!=vecParamsLower.end();itBinderPara++,itBinderParaOri++)
	{
		if (*itBinderParaOri != *itBinderPara)
		{
			
boost::ireplace_all(strOriSQL,*itBinderParaOri,*itBinderPara);
		}
	}
}


CDBServiceConfig::CDBServiceConfig()
{
	
}

CDBServiceConfig::~CDBServiceConfig()
{
	
}

SServiceDesc* CDBServiceConfig::GetServiceDesc(int nServiceId)
{
	SServiceDesc* pDesc = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		pDesc=&(iterMap->second);
	}
	return pDesc;
}

SServiceType* CDBServiceConfig::GetSingleServiceType(int nServiceId,string& sTypeName)
{
	SServiceType* pType = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		map<string,SServiceType>::iterator iterMapType;
		iterMapType = (iterMap->second.mapServiceType).find(sTypeName);
		if(iterMapType != (iterMap->second.mapServiceType).end())
		{
			pType = &(iterMapType->second);
		}
	}

	return pType;
}

SServiceType* CDBServiceConfig::GetParamTypeByBindName(int nServiceId,int nMsgId,string& sBindName)
{
	SServiceType* pType = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		map<int,SMsgAttri>& mapAttri = (iterMap->second).mapMsgAttri;
		map<int,SMsgAttri>::iterator iterMapAttri = mapAttri.find(nMsgId);
		if(iterMapAttri != mapAttri.end())
		{
			vector<SParam>& vecParam = (iterMapAttri->second).requestParams;
			vector<SParam>::iterator iterParam;
			for(iterParam = vecParam.begin(); iterParam != vecParam.end(); ++iterParam)
			{
				if(iterParam->sBindName == sBindName)
				{
					pType = GetSingleServiceType(nServiceId,iterParam->sTypeName);
					break;
				}
			}

		}
	}

	return pType;
}

vector<SParam>* CDBServiceConfig::GetRequestParamsById(int nServiceId,int nMsgId)
{
	
	SV_XLOG(XLOG_DEBUG,"CDBServiceConfig::%s,ServiceId=[%d],MsgId=[%d]\n",__FUNCTION__,nServiceId,nMsgId);

	vector<SParam>* pVecParams = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		map<int,SMsgAttri>& mapAttri = (iterMap->second).mapMsgAttri;
		map<int,SMsgAttri>::iterator iterMapAttri = mapAttri.find(nMsgId);
		if(iterMapAttri != mapAttri.end())
		{
			pVecParams = &((iterMapAttri->second).requestParams);
		}
		else
		{
			SV_XLOG(XLOG_DEBUG,"CDBServiceConfig::%s,Can Not Find MsgId",__FUNCTION__);
		}
	}
	else
	{
		SV_XLOG(XLOG_DEBUG,"CDBServiceConfig::%s,Can Not Find ServiceId",__FUNCTION__);
	}

	return pVecParams;
}

vector<SParam>* CDBServiceConfig::GetResponeParamsById(int nServiceId,int nMsgId)
{
	vector<SParam>* pVecParams = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		map<int,SMsgAttri>& mapAttri = (iterMap->second).mapMsgAttri;
		map<int,SMsgAttri>::iterator iterMapAttri = mapAttri.find(nMsgId);
		if(iterMapAttri != mapAttri.end())
		{
			pVecParams = &((iterMapAttri->second).responeParams);
		}
	}

	return pVecParams;
}


void CDBServiceConfig::ForSpeedUp()
{
	map<int,SServiceDesc>::iterator it_service =  mapServiceConfig.begin();

	for(;it_service!=mapServiceConfig.end();it_service++)
	{
		
SServiceDesc &desc = it_service->second;
		 int nServiceId = desc.nServiceId;
		 int nMsgId ;
		 map<int,SMsgAttri>::iterator it_msg = desc.mapMsgAttri.begin();
		 for(;it_msg!= desc.mapMsgAttri.end(); it_msg++)
		 {
		 	
nMsgId = it_msg->first;
			 GetDivdeDesc(nServiceId,nMsgId,it_msg->second.divideDesc);
		 }
	}
}

map<string,SServiceType>* CDBServiceConfig::GetServiceTypeById(int nServiceId)
{
	map<string,SServiceType>* pTypes = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		pTypes=&((iterMap->second).mapServiceType);
	}
	return pTypes;
}

SMsgAttri* CDBServiceConfig::GetMsgAttriById(int nServiceId,int nMsgId)
{
	SMsgAttri* pMsgAttri = NULL;
	map<int,SServiceDesc>::iterator iterMap=mapServiceConfig.find(nServiceId);
	if(iterMap!=mapServiceConfig.end())
	{
		map<int,SMsgAttri>::iterator iterMsg = (iterMap->second).mapMsgAttri.find(nMsgId);
		if(iterMsg!=(iterMap->second).mapMsgAttri.end())
		{
			pMsgAttri=&(iterMsg->second);
		}
	}
	return pMsgAttri;
}

int CDBServiceConfig::GetDivdeDesc(int nServiceId,int nMsgId,SDivideDesc& divideDesc)
{
	SMsgAttri* pMsgAttri = GetMsgAttriById(nServiceId,nMsgId);
	if(!pMsgAttri)
	{
		return -1;
	}

	if(pMsgAttri->bIsFullScan)
	{
		divideDesc.eDivideType = EDT_FULLSCAN;
	}
	else
	{
		divideDesc.eDivideType = EDT_NODIVIDE;
		vector<SParam>& vecReqParam = pMsgAttri->requestParams;
		vector<SParam>::iterator iterReqParam;
		for(iterReqParam=vecReqParam.begin();iterReqParam!=vecReqParam.end();++iterReqParam)
		{
			if(iterReqParam->bIsDivide)
			{
				divideDesc.eDivideType = EDT_DIVIDE;
				divideDesc.nHashParamCode = iterReqParam->nTypeCode;
				SServiceType* pType = GetSingleServiceType(nServiceId,iterReqParam->sTypeName);
				if(pType)
				{
					divideDesc.eParamType = pType->eType;
					
					divideDesc.nElementCode = pType->nElementCode;
				}
			}
		}
	}

	return 0;
}



void CDBServiceConfig::GetAllServiceIds(vector<unsigned int>& vecServiceIds)
{
	vecServiceIds.clear();
	vector<SConnDesc>::iterator iterConnDesc;
	for(iterConnDesc=vecDBConnConfig.begin();iterConnDesc!=vecDBConnConfig.end();++iterConnDesc)
	{
		vector<int>& vecIds = iterConnDesc->vecServiceId;
		vector<int>::iterator iterId;
		for(iterId=vecIds.begin();iterId!=vecIds.end();++iterId)
		{
			vecServiceIds.push_back(*iterId);
		}
	}
	return;
}


int CDBServiceConfig::GetDBThreadsNum()
{
	return(m_nDBThreadNum = (m_nDBThreadNum<=0) ? 8:m_nDBThreadNum);
}

int CDBServiceConfig::GetDBBigQueueNum()
{
	return(m_nDBBigQueueNum = (m_nDBBigQueueNum<=0) ? 5000000:m_nDBBigQueueNum);
}

unsigned int CDBServiceConfig::GetDBDelayQueueNum()
{
	return(m_nDBDelayQueueNum = (m_nDBDelayQueueNum<=0) ? 3000:m_nDBDelayQueueNum);
}

vector<SConnDesc>* CDBServiceConfig::GetAllDBConnConfigs()
{
	return (&vecDBConnConfig);
}

void CDBServiceConfig::LoadDBConnConfig(const string& sPath)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);
	vecDBConnConfig.clear();

//	string sConfigPath = sPath + "db_config.xml";
	string sConfigPath = "./config.xml";
#ifdef WIN32
	 sConfigPath = "./bpe/config.xml";
#endif

	path pathConfig(sConfigPath);

	try
	{
		
		if (exists(pathConfig))
		{
			if(LoadXMLDBConnConfig(pathConfig.string()) !=0)
			{
				SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s LoadXMLDBConnConfig failed\n",__FUNCTION__);
			}
		}
		else
		{
			SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s NO db_config.xml\n",__FUNCTION__);
		}
	}

	catch (const filesystem_error& ex)
	{
   	 	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s EileSystemError,Info=[%s]\n",__FUNCTION__,ex.what());
	}

	ForSpeedUp();
	
}

bool isDirectory(const string& dirName)
{
	bool bDirectory = false;
	try
	{
		path pathService(dirName);
		if (exists(pathService))
		{
			if(is_directory(pathService))
			{
				bDirectory=true;
			}
		}
	}
	catch (const filesystem_error& ex)
	{
	}
	return bDirectory;
}

void CDBServiceConfig::SubLoadServiceConfig(const string& sPath,int& nLoadCount)
{
	//先检查是否存在db子目录,存在则使用db子目录,否则使用原有的配置
	string realPath = sPath;
	bool bDBDirectory = isDirectory(sPath+"db/");
	if (bDBDirectory)
	{
		realPath = sPath+"db/";
	}
	
	path pathServices(realPath);
	try
	{
		if (exists(pathServices))
		{
			if(is_directory(pathServices))
			{
				directory_iterator dirIter(realPath);
				for(; dirIter != directory_iterator(); ++dirIter)
				{
					path pSub = (*dirIter).path();

					if(is_directory(pSub))
					{
						SubLoadServiceConfig(pSub.string(),nLoadCount);
					}
					else
					{
						string  strFile = (*dirIter).filename();
						if (bDBDirectory)
						{
							if(pSub.extension() == ".xml")
							{
								LoadXmlByFileName(pSub.string());
								nLoadCount++;
							}
						}
						else if (strncmp(strFile.c_str(),"db_",3)==0)
						{
							if(pSub.extension() == ".xml")
							{
								LoadXmlByFileName(pSub.string());
								nLoadCount++;
							}
						}
					}
				}
			}
		}
		else
		{
			SC_XLOG(XLOG_DEBUG,"CDBServiceConfig::%s not a directory [%s]\n",__FUNCTION__,pathServices.string().c_str());
		}
	}
	catch (const filesystem_error& ex)
	{
   	 	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s EileSystemError,Info=[%s]\n",__FUNCTION__,ex.what());
	}
}


void CDBServiceConfig::LoadServiceConfig(const string& sPath)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);

	mapServiceConfig.clear();

	int nLoadFiles =0;

	SubLoadServiceConfig(sPath,nLoadFiles);
	
	if (nLoadFiles == 0)
	{
		SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s NO XML FILE LOAD\n",__FUNCTION__);
	}

	Dump();
	
}

int CDBServiceConfig::GetBPEThreadsNum()
{
	if (m_nBPEThreadNum<=0)
	return 8;
	
	return m_nBPEThreadNum;
}

int CDBServiceConfig::LoadXMLDBConnConfig(const string& sFileName)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());

	TiXmlDocument m_xmlDoc;
    TiXmlElement *pRoot;
	
	if(!m_xmlDoc.LoadFile(sFileName.c_str()))
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Load File Error,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}
		
	if((pRoot=m_xmlDoc.RootElement())==NULL)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Have No RootElement,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}
	m_nBPEThreadNum=8;
	TiXmlElement* pBPEThreadNum;
	if((pBPEThreadNum=pRoot->FirstChildElement("ThreadNum"))!=NULL)
	{
		string sTmp = pBPEThreadNum->GetText();
		m_nBPEThreadNum = atoi(sTmp.c_str());
	}
	else
	{
		SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s,No BPEThreadNum Use Default[8]\n",__FUNCTION__);
	}
	if (m_nBPEThreadNum<=0)
	{
		m_nBPEThreadNum=8;
	}	
	m_nDBThreadNum = 8;
	TiXmlElement* pDBThreadNum;
	if((pDBThreadNum=pRoot->FirstChildElement("dbthreadnum"))!=NULL)
	{
		string sTmp = pDBThreadNum->GetText();
		m_nDBThreadNum = atoi(sTmp.c_str());
	}
	else
	{
		SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s,No DBThreadNum Use Default[8]\n",__FUNCTION__);
	}
		
	m_nDBBigQueueNum = 500000; //50万
	TiXmlElement* pDBBigQueueNum;
	if((pDBBigQueueNum=pRoot->FirstChildElement("dbbigqueuenum"))!=NULL)
	{
		string sTmp = pDBBigQueueNum->GetText();
		m_nDBBigQueueNum = atoi(sTmp.c_str());
	}
	else
	{
		SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s,No DBBigQueueNum Use Default[5000000]\n",__FUNCTION__);
	}
	
	m_nDBDelayQueueNum = 3000; //3000
	TiXmlElement* pDBDelayQueueNum;
	if((pDBDelayQueueNum=pRoot->FirstChildElement("dbdelayqueuenum"))!=NULL)
	{
		string sTmp = pDBDelayQueueNum->GetText();
		m_nDBDelayQueueNum = atoi(sTmp.c_str());
	}
	else
	{
		SC_XLOG(XLOG_WARNING, "CDBServiceConfig::%s,No DBDelayQueueNum Use Default[3000]\n",__FUNCTION__);
	}
	
	TiXmlElement* pDBSosList;
	if((pDBSosList=pRoot->FirstChildElement("dbsoslist"))==NULL)
	{
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s no dbsoslist\n",__FUNCTION__);
		return -1; 
	}
	else
	{	
		SConnDesc connDesc;
		if(LoadDBConnDesc(pDBSosList,connDesc)<0)
		{
			return -1;
		}
		vecDBConnConfig.push_back(connDesc);
	}
	while((pDBSosList=pDBSosList->NextSiblingElement("dbsoslist"))!=NULL)
	{
		SConnDesc connDesc;
		if(LoadDBConnDesc(pDBSosList,connDesc)<0)
		{
			return -1;
		}
		vecDBConnConfig.push_back(connDesc);
	}
	return 0;

}

int CDBServiceConfig::LoadDBConnDesc(TiXmlElement* pSosList,SConnDesc& connDesc)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);

	TiXmlElement* pServiceId;
//	int nRet = 0;
	if((pServiceId=pSosList->FirstChildElement("serviceid"))==NULL)
	{
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s no serviceid\n",__FUNCTION__);
		return -1; 
	}
	else
	{
		TiXmlElement* pSmartConfig=pSosList->FirstChildElement("smartcommit");
		if (pSmartConfig!=NULL)
		{
			if (pSmartConfig->GetText()!=NULL)
			{
				string smart = pSmartConfig->GetText();
				connDesc.dwSmart = atoi(smart.c_str());
			}
			else
			{
				connDesc.dwSmart=0;
			}
		}
		else
		{
			connDesc.dwSmart = 0;
		}
	
		string sServiceIds = pServiceId->GetText();
		vector<string> vecServiceId;
		boost::split(vecServiceId,sServiceIds,is_any_of(","),token_compress_on);

		vector<string>::iterator iterServiceId;
		for(iterServiceId=vecServiceId.begin();iterServiceId!=vecServiceId.end();++iterServiceId)
		{
			connDesc.vecServiceId.push_back(atoi(iterServiceId->c_str()));
		}
	}
	
	TiXmlElement* pPreSql;
	if((pPreSql=pSosList->FirstChildElement("presql"))!=NULL)
	{
		if (pPreSql->GetText()!=NULL)
		{
			connDesc.strPreSql=pPreSql->GetText();
		}
	}
	
	TiXmlElement* pMasterDB;
	if((pMasterDB=pSosList->FirstChildElement("masterdb"))==NULL)
	{
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s no masterdb\n",__FUNCTION__);
		return -1; 
	}
	else
	{
		if (SubLoadDBConnDesc(pMasterDB,connDesc.masterConn)<0)
			return -1;
	}

	TiXmlElement* pSlaveDB;
	if((pSlaveDB=pSosList->FirstChildElement("slavedb"))!=NULL)
	{
		if (SubLoadDBConnDesc(pMasterDB,connDesc.slaveConn)<0)
			return -1;
	}

	return 0;
	
	
}

string PrintHex(const char* ibuf, int nlen)
{
	const char* table[]={"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};
	string text;
	const unsigned char* buf = (const unsigned char*)ibuf;
	for(int i=0;i<nlen;i++)
	{
		text+=table[(buf[i]&0xf0)>>4];
		text+=table[(buf[i]&0x0f)];
	}
	return text;
	
}
 

void FromHex(char* out, int&outlen,string &str)
{
	const unsigned char*buf = (const unsigned char*)str.c_str();
	int nlen = str.size();
	outlen = 0;
	for(int i=0;i<nlen; i+=2)
	{
		int tmp;
		if (buf[i]>='0' && buf[i]<='9')
			tmp = (buf[i]-'0')*16;
		else 
			tmp = (buf[i]-'A'+10)*16;
			
		if (buf[i+1]>='0' && buf[i+1]<='9')
			tmp += (buf[i+1]-'0');
		else 
			tmp += (buf[i+1]-'A'+10);
		out[outlen++]=tmp;
	}
	out[outlen]=0;
}


bool CDBServiceConfig::DecodePassword(const string&enc_text,string& out)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s\n",__FUNCTION__);
	string enc_text2=enc_text;
	trim(enc_text2);
	
	vector<string> vecConfig;
	if( (boost::algorithm::istarts_with(enc_text2,"mysql"))||(boost::algorithm::istarts_with(enc_text2,"oracle")))
	{
		boost::split(vecConfig,enc_text2,is_any_of(" "),token_compress_on);
	}
	else if(boost::algorithm::istarts_with(enc_text2,"odbc"))
	{
		boost::split(vecConfig,enc_text2,is_any_of(";"),token_compress_on);
	}
	else
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfigX::%s Unknown DBType Error[%s] \n",__FUNCTION__,vecConfig[0].c_str());
		return false;
	}

	if (vecConfig.size()<3)
	{
		return false;
	}
	trim(vecConfig[0]);
	bool bContainPass  =false;
	string enc_pass ;
	vector<string>::iterator it = vecConfig.begin();
	for(;it!=vecConfig.end();it++)
	{
			if (boost::algorithm::istarts_with(vecConfig[0],"odbc"))
			{
				if (boost::algorithm::istarts_with(it->c_str(),"Pwd="))
				{
					bContainPass = true;
					enc_pass = (*it).substr(4);	
					break;
				}
			}
			else if (strncmp("password=",it->c_str(),9)==0)
			{
				bContainPass = true;
				enc_pass = (*it).substr(9);	
				
				break;
			}
	}
	if (!bContainPass)
		return false;
		
	char output[2048]={0};
	//int outlen;
	char clear[2048]={0};
	
	string strClear = "";
		
#ifndef M_SUPPORT_PUBLIC
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s ENC[%s]\n",__FUNCTION__,enc_pass.c_str());
	
	int base64len = CCipher::Base64Decode(output,enc_pass.c_str(),enc_pass.size());
	
	//SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s BASE64[%s]\n",
	//__FUNCTION__,PrintHex(output,base64len).c_str()
	//);
	
	int clearlen;
	CCipher::RsaPrivateDecrypt(output,base64len,clear,clearlen);
	//SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s CLEAR[%s]\n",
	//__FUNCTION__,clear
	//);
	
	strClear=clear;
#else
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s PUBLIC[%s]\n",__FUNCTION__,enc_pass.c_str());
	strClear=enc_pass;
#endif
	
	out = "";

	
	if (boost::algorithm::istarts_with(vecConfig[0],"odbc"))
	{
		for(it = vecConfig.begin();it!=vecConfig.end();it++)
		{
			if (boost::algorithm::istarts_with(it->c_str(),"Pwd="))
			{	
				out+="Pwd="+strClear+";";
			}
			else if (it->size()!=0)
			{
				out+=*it+";";
			}
		}
	}
	else
	{
		for(it = vecConfig.begin();it!=vecConfig.end();it++)
		{
			if (strncmp("password",it->c_str(),8)==0)
			{	
				out+="password="+strClear+" ";
			}
			else
			{
				out+=*it+" ";
			}
		}
	}
	return true;
}


bool CDBServiceConfig::EncodePassword(const string&clear_text,string& out)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s\n",__FUNCTION__);
	vector<string> vecConfig;
	boost::split(vecConfig,clear_text,is_any_of(" "),token_compress_on);
	bool bContainPass  =false;
	string clear_pass ;
	vector<string>::iterator it = vecConfig.begin();
	for(;it!=vecConfig.end();it++)
	{
			if (strncmp("password=",it->c_str(),9)==0)
			{
				bContainPass = true;
				clear_pass = (*it).substr(9);	
			
				break;
			}
	}
	
	if (!bContainPass)
		return false;
		
	
	char base64[2048]={0};
	char output[2048]={0};
	int outlen;
	
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s CLEAR[%s] Size[%d]\n",__FUNCTION__,clear_pass.c_str(),clear_pass.size());
		
	
	CCipher::RsaPublicEncrypt((char*)clear_pass.c_str(),clear_pass.size(),output,outlen);
	
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s Enc[%s]\n",__FUNCTION__,PrintHex(output,outlen).c_str());
	
	CCipher::Base64Encode(base64,(char*)output,outlen);
	
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s AFTERBASE64[%s]\n",__FUNCTION__,base64);


	/*
	//------------------------
	char dec[2048]={0};
	int declen;
	CCipher::RsaPrivateDecrypt((char*)output,outlen,dec,declen);
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s dec[%s]\n",__FUNCTION__,PrintHex(dec,declen).c_str());
	char clear[2048];
	CCipher::Base64Decode(clear,dec,declen);
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s clear[%s]\n",
	__FUNCTION__,clear
	);
	*/
	string strEnc = base64;

	
	out = "";
	for(it = vecConfig.begin();it!=vecConfig.end();it++)
	{
		if (strncmp("password",it->c_str(),8)==0)
		{	
			out+="password="+strEnc+" ";
		}
		else
		{
			out+=*it+" ";
		}
	}
	return true;
}

int CDBServiceConfig::SubLoadDBConnDesc(TiXmlElement* pDBDesc,SConnItem& connItem)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigX::%s\n",__FUNCTION__);

	try{
	int nRet = 0;
	int nConnNum = 0;
	nRet = pDBDesc->QueryIntAttribute("conns", (int *)&nConnNum);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get Master ConnNum Error\n",__FUNCTION__);
	}
	else
	{
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s nConnNum[%d]\n",__FUNCTION__,nConnNum);
	}
	connItem.nConnNum = nConnNum;

	int nDBFactor = 1;
	nRet = pDBDesc->QueryIntAttribute("dbfactor", (int *)&nDBFactor);
	if(nRet == TIXML_SUCCESS)
	{
		connItem.nDBFactor = nDBFactor;
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s nDBFactor[%d]\n",__FUNCTION__,nDBFactor);
	}

	int nTBFactor = 1;
	nRet = pDBDesc->QueryIntAttribute("tbfactor", (int *)&nTBFactor);
	if(nRet == TIXML_SUCCESS)
	{
		connItem.nTBFactor = nTBFactor;
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s nTBFactor[%d]\n",__FUNCTION__,nTBFactor);
	}

	char szFactorType[64] = {0};
	nRet = pDBDesc->QueryValueAttribute("factortype", &szFactorType);
	if(nRet == TIXML_SUCCESS)
	{
		connItem.sFactorType= szFactorType;
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s szFactorType[%s]\n",__FUNCTION__,szFactorType);
	}

	TiXmlElement* pDivideConns;
	if((pDivideConns=pDBDesc->FirstChildElement("divideconns"))!=NULL)
	{
		TiXmlElement* pConnStr;
		if((pConnStr=pDivideConns->FirstChildElement("conn"))!=NULL)
		{
			string strConn = pConnStr->GetText();
			string out1,out2;	
			out1= strConn;
			if (DecodePassword(out1,out2)<0)
			{
				SC_XLOG(XLOG_ERROR, "divide connectstr[%s] config error\n",strConn.c_str());
				return -1;
			}
			strConn = out2;
			//SC_XLOG(XLOG_DEBUG, ".............out2[%s]\n",out2.c_str());
			connItem.vecDivideConn.push_back(strConn);
		}

		while((pConnStr=pConnStr->NextSiblingElement("conn"))!=NULL)
		{
			string strConn = pConnStr->GetText();
			string out1,out2;	
			out1= strConn;
			if (DecodePassword(out1,out2)<0)
			{
				SC_XLOG(XLOG_ERROR, "divide connectstr[%s] config error\n",strConn.c_str());
				return -1;
			}
			strConn = out2;
			//SC_XLOG(XLOG_DEBUG, ".............out2[%s]\n",out2.c_str());
			connItem.vecDivideConn.push_back(strConn);
		}

	}
	
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfigY::%s\n",__FUNCTION__);
	TiXmlElement* pDefaultConn;
	if((pDefaultConn=pDBDesc->FirstChildElement("defaultconn"))!=NULL)
	{
		string strConn = pDefaultConn->GetText();
		
		string out1,out2;	
		out1= strConn;
		if (DecodePassword(out1,out2)<0)
		{
			SC_XLOG(XLOG_ERROR, "default contstr[%s] config error\n",strConn.c_str());
			return -1;
		}
		strConn = out2;
		//SC_XLOG(XLOG_DEBUG, ".............out2[%s]\n",out2.c_str());
		connItem.sDefaultConn = strConn;
	}
	}catch(std::exception const &e)
	{
		SC_XLOG(XLOG_ERROR, "Load SubConnect Exception e:%s\n",e.what());
		SC_XLOG(XLOG_ERROR, "Load SubConnect FAILED !!!!!!!!!!!!!\n");
		return -1;
	}
	return 0;

	
}



int CDBServiceConfig::LoadXmlByFileName(const string& sFileName)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());

	TiXmlDocument m_xmlDoc;
    TiXmlElement *pService;
	
	if(!m_xmlDoc.LoadFile(sFileName.c_str()))
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Load File Error,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}
		
	if((pService=m_xmlDoc.RootElement())==NULL)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No RootElement,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}

	SServiceDesc serviceDesc;
	
	char szName[64] = {0};
	int nRet = pService->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Service Name,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}
	serviceDesc.sServiceName = szName;
	boost::to_lower(serviceDesc.sServiceName);
	
	serviceDesc.bReadOnly = false;
	
	char szReadOnly[64]={0};
	nRet = pService->QueryValueAttribute("queryonly", &szReadOnly);
	if(nRet == TIXML_SUCCESS)
	{
		if (strncmp(szReadOnly,"true",4)==0)
		{
			serviceDesc.bReadOnly = true;
		}		
	}
	
	int nServiceId = 0;
	nRet = pService->QueryIntAttribute("id", (int *)&nServiceId);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get ServiceId Error\n",__FUNCTION__);
    	return -1;
	}
	serviceDesc.nServiceId = nServiceId;

	/* get types */
	TiXmlElement * pConfigType = NULL;
	if((pConfigType=pService->FirstChildElement("type"))==NULL)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1; 
	}
	else
	{	
		SServiceType serviceType;
		if(LoadServiceTypes(pConfigType,serviceType)<0)
		{
			return -1;
		}

		serviceDesc.mapServiceType.insert(make_pair(serviceType.sTypeName,serviceType));
	}
	while((pConfigType=pConfigType->NextSiblingElement("type"))!=NULL)
	{
		SServiceType serviceType;
		if(LoadServiceTypes(pConfigType,serviceType)<0)
		{
			return -1;
		}
		serviceDesc.mapServiceType.insert(make_pair(serviceType.sTypeName,serviceType));
	}

	PostProcessServiceTypes(serviceDesc.mapServiceType);

	//get message attribute
	TiXmlElement * pMsgAttri = NULL;
	if((pMsgAttri=pService->FirstChildElement("message"))==NULL)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Message,FileName=[%s]\n",__FUNCTION__,sFileName.c_str());
		return -1;
	}
	else
	{
		SMsgAttri sMsgAttri;
		if(LoadServiceMessage(pMsgAttri,sMsgAttri)<0)
		{
			return -1;
		}
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,OperType=[%d]\n",__FUNCTION__,sMsgAttri.eOperType);
		serviceDesc.mapMsgAttri.insert(make_pair(sMsgAttri.nMsgId,sMsgAttri));
	}

	while((pMsgAttri=pMsgAttri->NextSiblingElement("message"))!=NULL)
	{
		SMsgAttri sMsgAttri;
		if(LoadServiceMessage(pMsgAttri,sMsgAttri)<0)
		{
			return -1;
		}
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,OperType=[%d]\n",__FUNCTION__,sMsgAttri.eOperType);
		serviceDesc.mapMsgAttri.insert(make_pair(sMsgAttri.nMsgId,sMsgAttri));
	}

	PostProcessMsgAttri(serviceDesc.mapMsgAttri,serviceDesc.mapServiceType);

	mapServiceConfig.insert(make_pair(serviceDesc.nServiceId,serviceDesc));

	return 0;
	
}

int CDBServiceConfig::LoadTlvServiceTypes(TiXmlElement* pType, SServiceType& sServiceType)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s Start!\n",__FUNCTION__);

	char szName[64] = {0};
	int nRet = pType->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type Name,FileName=[%s]\n",__FUNCTION__);
		return -1;
	}
	
	sServiceType.sTypeName = szName;
	boost::to_lower(sServiceType.sTypeName);
	//
	char szType[128] = {0};
	nRet = pType->QueryValueAttribute("type", &szType);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type\n",__FUNCTION__);
		return -1;
	}

	sServiceType.sElementName = szType;
	
	return 0;
}

int CDBServiceConfig::LoadStructServiceTypes(TiXmlElement* pType, SServiceType& sServiceType)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s Start!\n",__FUNCTION__);

	char szName[64] = {0};
	int nRet = pType->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type Name,FileName=[%s]\n",__FUNCTION__);
		return -1;
	}
	
	sServiceType.sTypeName = szName;
	boost::to_lower(sServiceType.sTypeName);
	//
	char szType[16] = {0};
	nRet = pType->QueryValueAttribute("type", &szType);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type\n",__FUNCTION__);
		return -1;
	}

	if(strncmp(szType,"int",3)==0)
	{
		sServiceType.eType = DBP_DT_INT;
	}
	else if(strncmp(szType,"string",6) == 0)
	{
		sServiceType.eType = DBP_DT_STRING;

		int nStrLen = 0;
		nRet = pType->QueryValueAttribute("length", (int *)&nStrLen);
		if(nRet == TIXML_SUCCESS)
		{
			sServiceType.nStrLen = nStrLen;
		}
		else 
		{
			nRet = pType->QueryValueAttribute("len", (int *)&nStrLen);
			if(nRet == TIXML_SUCCESS)
			{
				sServiceType.nStrLen = nStrLen;
			}
		}
	}
	else if(strncmp(szType,"systemstring",12)==0)
	{
		sServiceType.eType = DBP_DT_SYSTEMSTRING;
	}
	else
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Struct Element Just Support Int Or String \n",
			__FUNCTION__);
		return -1;
	}
	
	return 0;
}



int CDBServiceConfig::LoadServiceTypes(TiXmlElement* pType, SServiceType& sServiceType)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s Start!\n",__FUNCTION__);
	
	char szName[256] = {0};
	int nRet = pType->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type Name\n",__FUNCTION__);
		return -1;
	}
	sServiceType.sTypeName = szName;
	boost::to_lower(sServiceType.sTypeName);
	//
	char szClass[16] = {0};
	nRet = pType->QueryValueAttribute("class", &szClass);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,No Type Class\n",__FUNCTION__);
		return -1;
	}

	if(strncmp(szClass,"int",3)==0)
	{
		sServiceType.eType = DBP_DT_INT;
	}
	else if(strncmp(szClass,"string",6) == 0)
	{
		sServiceType.eType = DBP_DT_STRING;

		int nStrLen = 0;
		nRet = pType->QueryValueAttribute("length", (int *)&nStrLen);
		if(nRet == TIXML_SUCCESS)
		{
			sServiceType.nStrLen = nStrLen;
		}
	}
	else if(strncmp(szClass,"array",5) == 0)
	{
		sServiceType.eType = DBP_DT_ARRAY;
		char szItemType[256] = {0};
		nRet = pType->QueryValueAttribute("itemtype", &szItemType);

		if(nRet != TIXML_SUCCESS)
		{
			SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get Array Element Error,Name=[%s]\n",__FUNCTION__,szName);
			return -1;
		}
		sServiceType.sElementName = szItemType;
	}
	else if(strncmp(szClass,"tlv",3) == 0)
	{
		char szArrayField[32] = {0};
		nRet = pType->QueryValueAttribute("arrayfield", &szArrayField);
		if((nRet == TIXML_SUCCESS)&&(strncmp(szArrayField,"true",4) == 0))
		{
			sServiceType.eType = DBP_DT_TLV_ARRAY;
			TiXmlElement * pTlvType = NULL;
			
			int nTypeCode = 0;
			nRet = pType->QueryValueAttribute("code", (int *)&nTypeCode);
			if(nRet != TIXML_SUCCESS)
			{
				SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s, Name=[%s] TLV-Array no code\n",__FUNCTION__,szName);
				return -1;
			}
			sServiceType.nTypeCode = nTypeCode;
			
			if((pTlvType=pType->FirstChildElement("field"))==NULL)
			{
				SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s, Name=[%s] TLV-Array Child No Type_Type\n",__FUNCTION__,szName);
				return -1; 
			}
			else
			{
				char szItemType[256] = {0};
				nRet = pTlvType->QueryValueAttribute("type", &szItemType);

				if(nRet != TIXML_SUCCESS)
				{
					SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get TLV-Array Element Error,Name=[%s]\n",__FUNCTION__,szName);
					return -1;
				}
				sServiceType.sElementName = szItemType;
			}
		}
		else
		{
			sServiceType.eType = DBP_DT_TLV;
			TiXmlElement * pTlvType = NULL;
			
			if((pTlvType=pType->FirstChildElement("field"))==NULL)
			{
				SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,TLV Child No Type_Type\n",__FUNCTION__);
				return -1; 
			}
			else
			{	
				SServiceType tlvElementType;
				if(LoadTlvServiceTypes(pTlvType,tlvElementType)<0)
				{
					return -1;
				}
				sServiceType.vecSubType.push_back(tlvElementType);
			}
			
			while((pTlvType=pTlvType->NextSiblingElement("field"))!=NULL)
			{
				SServiceType tlvElementType;
				if(LoadTlvServiceTypes(pTlvType,tlvElementType)<0)
				{
					return -1;
				}
				sServiceType.vecSubType.push_back(tlvElementType);
			}
		}
			
	}
	else if(strncmp(szClass,"struct",6) == 0)
	{
		sServiceType.eType = DBP_DT_STRUCT;
		
		TiXmlElement * pStructType = NULL;
		if((pStructType=pType->FirstChildElement("field"))==NULL)
		{
			SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Struct Child No Type_Type\n",__FUNCTION__);
			return -1; 
		}
		else
		{	
			SServiceType structElementType;
			if(LoadStructServiceTypes(pStructType,structElementType)<0)
			{
				return -1;
			}
			sServiceType.vecSubType.push_back(structElementType);
		}
		
		while((pStructType=pStructType->NextSiblingElement("field"))!=NULL)
		{
			SServiceType structElementType;
			if(LoadStructServiceTypes(pStructType,structElementType)<0)
			{
				return -1;
			}
			sServiceType.vecSubType.push_back(structElementType);
		}

		
	}

	int nTypeCode = 0;
	nRet = pType->QueryValueAttribute("code", (int *)&nTypeCode);
	if(nRet == TIXML_SUCCESS)
	{
		sServiceType.nTypeCode = nTypeCode;
	}
	else
	{
		sServiceType.nTypeCode = 0;
	}

	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,code=[%d],name=[%s]\n",
		__FUNCTION__,sServiceType.nTypeCode,sServiceType.sTypeName.c_str());
	
	
	return 0;
}


int CDBServiceConfig::LoadSQLs(TiXmlElement* pSQL,SMsgAttri& sMsgAtrri)
{

	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);
	char szIsFullScan[16] = {0};
	int nRet = pSQL->QueryValueAttribute("isfullscan", &szIsFullScan);
	if(nRet == TIXML_SUCCESS)
	{
		if(strncmp(szIsFullScan,"true",4) == 0)
		{
			sMsgAtrri.bIsFullScan = true;
		}
		else
		{
			sMsgAtrri.bIsFullScan = false;
		}
	}

	TiXmlElement *pMulPart = NULL;
	vector<SPartSql> parts;
	pMulPart = pSQL->FirstChildElement("mulpart");
	while(pMulPart != NULL)
	{
		SPartSql ss;
		string strSql;
		if(pMulPart->GetText()==NULL)
		{
			/* cdata */
			const char* pCDataSQL = pMulPart->FirstChild()->Value();
			if(pCDataSQL)
			{
				strSql = pCDataSQL;
			}
		}
		else
		{
			strSql = pMulPart->GetText();
		}
		ss.strSql = strSql;
		ss.dwConditon = OP_NOTHING;
		char szConditon[128] = {0};
		char szVariable[128] = {0};
		if (pMulPart->QueryValueAttribute("test", &szConditon)==TIXML_SUCCESS)
		{
			if (strcmp(szConditon,"null")==0)
			{
				ss.dwConditon=OP_NULL;
			}
			else if (strcmp(szConditon,"notnull")==0)
			{
				ss.dwConditon=OP_NOTNULL;
			}
		}
		if (pMulPart->QueryValueAttribute("variable", &szVariable)==TIXML_SUCCESS)
		{
			ss.strVariable=szVariable;
			boost::to_lower(ss.strVariable);
		}
		parts.push_back(ss);
		pMulPart=pMulPart->NextSiblingElement("mulpart");
	}
	
	sMsgAtrri.exePartSQLs.push_back(parts);
	
	TiXmlElement *pPrefix = NULL;
	if(( pPrefix = pSQL->FirstChildElement("prefix")) == NULL)
	{
		/* simple sql */
		string strTmp;
		if(pSQL->GetText()==NULL)
		{
			/* cdata */
			const char* pCDataSQL = pSQL->FirstChild()->Value();
			if(pCDataSQL)
			{
				strTmp = pCDataSQL;
			}
		}
		else
		{
			strTmp = pSQL->GetText();
		}
		
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,ORISQL=[%s]\n",__FUNCTION__,strTmp.c_str());
		
		ProcessXMLCDATA(strTmp);

		MakeSQLToLower(strTmp);
		sMsgAtrri.exeSQLs.push_back(strTmp);
		
		char szParaCounts[16] = {0};
		int paraCounts = -1;
		
		if (pSQL->QueryValueAttribute("paracounts", &szParaCounts)==TIXML_SUCCESS)
		{
			paraCounts = atoi(szParaCounts);
		}
		
		sMsgAtrri.paraCounts.push_back(paraCounts);
		
		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,SQL=[%s] ParaCounts=[%d]\n",__FUNCTION__,strTmp.c_str(),
		paraCounts);
		

		if(boost::istarts_with(strTmp,"update") 
			|| boost::istarts_with(strTmp,"delete") 
			|| boost::istarts_with(strTmp,"insert"))
		{
			sMsgAtrri.eOperType = DBO_WRITE;
		}
	}
	else
	{
		/* compose sql */
		SComposeSQL composeSQL;
		composeSQL.sPrefix = pPrefix->GetText();

		TiXmlElement *pSuffix = NULL;
		if( (pSuffix = pSQL->FirstChildElement("suffix")) != NULL)
		{
			composeSQL.sSuffix = pSuffix->GetText();
		}

		TiXmlElement *pDetail = NULL;
		if( (pDetail = pSQL->FirstChildElement("detail")) != NULL)
		{
			string strDetail = pDetail->GetText();
			MakeSQLToLower(strDetail);
			composeSQL.vecDetail.push_back(strDetail);
			
			while((pDetail=pDetail->NextSiblingElement("detail"))!=NULL)
			{
				strDetail = pDetail->GetText();
				MakeSQLToLower(strDetail);
				composeSQL.vecDetail.push_back(strDetail);
			}
		}

		sMsgAtrri.exeComposeSQLs.push_back(composeSQL);

		if(boost::istarts_with(composeSQL.sPrefix,"update") || boost::istarts_with(composeSQL.sPrefix,"delete"))
		{
			sMsgAtrri.eOperType = DBO_WRITE;
		}
	}

	return 0;
}

int CDBServiceConfig::LoadServiceMessage(TiXmlElement* pMessage, SMsgAttri& sMsgAtrri)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s Start\n",__FUNCTION__);
	char szName[64] = {0};
	int nRet = pMessage->QueryValueAttribute("name", &szName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get Message Name Error\n",__FUNCTION__);
		return -1;
	}
	sMsgAtrri.sMsgName = szName;
	boost::to_lower(sMsgAtrri.sMsgName);

	//
	int nMsgId;
	nRet = pMessage->QueryIntAttribute("id", (int *)&nMsgId);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get MessageID Error,MsgName=[%s] Error\n",__FUNCTION__,szName);
    	return -1;
	}
	sMsgAtrri.nMsgId = nMsgId;
	
	char szMultiArray[64] = {0};
	nRet = pMessage->QueryValueAttribute("multiarrayasone", &szMultiArray);
	if(nRet == TIXML_SUCCESS)
	{
		if (strcmp(szMultiArray,"true")==0)
		{
			sMsgAtrri.bMultArray = true;
		}
	}
	

	/* procedure */
	TiXmlElement *pProcedure = NULL;
	if( (pProcedure= pMessage->FirstChildElement("procedure")) != NULL)
	{
		SProcedure proc;
		proc.sProcName = pProcedure->GetText();
		sMsgAtrri.exeSPs.push_back(proc);

		SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,procedure=[%s]\n",__FUNCTION__,proc.sProcName.c_str());
		
		while((pProcedure=pProcedure->NextSiblingElement("procedure"))!=NULL)
		{
			SProcedure proc;
			string strProcedure = pProcedure->GetText();
			MakeSQLToLower(strProcedure);
			proc.sProcName = strProcedure;
			sMsgAtrri.exeSPs.push_back(proc);
			SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s,procedure=[%s]\n",__FUNCTION__,proc.sProcName.c_str());
		}

		sMsgAtrri.eOperType = DBO_WRITE;
	}
	

	/* simple sql and compose sql */
	TiXmlElement *pExeSQLs = NULL;
	if((pExeSQLs=pMessage->FirstChildElement("sql")) != NULL)
	{

		LoadSQLs(pExeSQLs,sMsgAtrri);
		while((pExeSQLs=pExeSQLs->NextSiblingElement("sql"))!=NULL)
		{
			LoadSQLs(pExeSQLs,sMsgAtrri);
		}
	}

	/* request param */
	TiXmlElement * pRequestParams = NULL;
	if((pRequestParams=pMessage->FirstChildElement("requestparameter"))!=NULL)
	{
		TiXmlElement * pField = NULL;
		if((pField=pRequestParams->FirstChildElement("field"))!=NULL)
		{
			SParam param;
			if(LoadRequestParam(pField,param)<0)
			{
				return -1;
			}
			sMsgAtrri.requestParams.push_back(param);
			
			while((pField=pField->NextSiblingElement("field"))!=NULL)
			{
				SParam param;
				if(LoadRequestParam(pField,param)<0)
				{
					return -1;
				}
				sMsgAtrri.requestParams.push_back(param);
			}
		}
	}


	/* respone */
	TiXmlElement * pResponeParams = NULL;
	if((pResponeParams=pMessage->FirstChildElement("responseparameter"))!=NULL)
	{
		TiXmlElement * pField = NULL;
		
		if((pField=pResponeParams->FirstChildElement("field"))!=NULL)
		{
			SParam param;
			if(LoadResponeParam(pField,param)<0)
			{
				return -1;
			}
			sMsgAtrri.responeParams.push_back(param);
			
			while((pField=pField->NextSiblingElement("field"))!=NULL)
			{
				SParam param;
				if(LoadResponeParam(pField,param)<0)
				{
					return -1;
				}
				sMsgAtrri.responeParams.push_back(param);
			}
		}

	}
	
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s End,MsgId=[%d],MsgName=[%s]\n\n",
		__FUNCTION__,sMsgAtrri.nMsgId,sMsgAtrri.sMsgName.c_str());

	return 0;
}

int CDBServiceConfig::LoadRequestParam(TiXmlElement* pField, SParam& sParam)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);

	sParam.nTypeCode  = 0;
	
	char szParamName[64] = {0};
	int nRet = pField->QueryValueAttribute("name", &szParamName);
	if(nRet != TIXML_SUCCESS)
	{
		return -1;
	}
	sParam.sParamName = szParamName;
	boost::to_lower(sParam.sParamName);

	char szTypeName[64] = {0};
	nRet = pField->QueryValueAttribute("type", &szTypeName);
	if(nRet != TIXML_SUCCESS)
	{
		return -1;
	}
	sParam.sTypeName = szTypeName;
	boost::to_lower(sParam.sTypeName);
	
	
	char szBindName[1024] = {0};
	nRet = pField->QueryValueAttribute("to", &szBindName);
	
	if(nRet != TIXML_SUCCESS)
	{
		return -1;
	}

	sParam.sBindName = szBindName;
	boost::to_lower(sParam.sBindName);

	char szPlain[64] = {0};
	nRet = pField->QueryValueAttribute("plain", &szPlain);
	if(nRet == TIXML_SUCCESS)
	{
		if (strncmp(szPlain,"true",4)==0)
		{
			sParam.bSQL = true;
		}
	}
	
	char szDefault[256] = {0};
	nRet = pField->QueryValueAttribute("default", &szDefault);
	if(nRet == TIXML_SUCCESS)
	{
	
		sParam.bDefault = true;
		sParam.sDefaultVal = szDefault;
	}
	
	SV_XLOG(XLOG_DEBUG,"^^^^^^^^BindName=[%s] sParam.bSQL=[%d]\n",sParam.sBindName.c_str(),
	(int)sParam.bSQL);

	if(boost::icontains(sParam.sBindName,"$hash"))
	{
		sParam.bIsDivide = true;
		vector<string> vecHashTmp;
		boost::split(vecHashTmp,sParam.sBindName,is_any_of(","),token_compress_on);
		if(vecHashTmp.size() == 2)
		{
			sParam.sBindName = vecHashTmp[0];
			sParam.sHashName = vecHashTmp[1];
		}
		
	}

	char szExtend[8] = {0};
	nRet = pField->QueryValueAttribute("extend", &szExtend);
	if(nRet == TIXML_SUCCESS)
	{
		if (0 == strncmp(szExtend, "true", 4))
		{
			sParam.bExtend = true;
		}
	}

	return 0;
	
}


int CDBServiceConfig::LoadResponeParam(TiXmlElement* pField, SParam& sParam)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);

	sParam.nTypeCode  = 0;
		
	char szParamName[64] = {0};
	int nRet = pField->QueryValueAttribute("name", &szParamName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get Name Error\n",__FUNCTION__);
		return -1;
	}
	sParam.sParamName = szParamName;
	boost::to_lower(sParam.sParamName);

	char szTypeName[64] = {0};
	nRet = pField->QueryValueAttribute("type", &szTypeName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get Type Error\n",__FUNCTION__);
		return -1;
	}
	sParam.sTypeName = szTypeName;
	boost::to_lower(sParam.sTypeName);

	char szBindName[1024] = {0};
	nRet = pField->QueryValueAttribute("from", &szBindName);
	if(nRet != TIXML_SUCCESS)
	{
		SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,Get BindName Error\n",__FUNCTION__);
		return -1;
	}
	
	sParam.sBindName = szBindName;
	
	
	SV_XLOG(XLOG_DEBUG,"^^^^^^^^BindName=[%s] bSQL[%d]\n",sParam.sBindName.c_str(),(int)sParam.bSQL);
	
	boost::to_lower(sParam.sBindName);

	return 0;
}

int CDBServiceConfig::PostProcessMsgAttri(map<int,SMsgAttri>& mapAttri,map<string,SServiceType>& mapType)
{
	SC_XLOG(XLOG_DEBUG, "CDBServiceConfig::%s\n",__FUNCTION__);
	map<int,SMsgAttri>::iterator iterMapMsgAttri = mapAttri.begin();
	for(;iterMapMsgAttri != mapAttri.end(); ++iterMapMsgAttri)
	{
		vector<SParam>::iterator iterParam;
		vector<SParam>& requestParam = iterMapMsgAttri->second.requestParams;
		for(iterParam=requestParam.begin();iterParam!=requestParam.end();++iterParam)
		{
			map<string,SServiceType>::iterator iterType = mapType.find(iterParam->sTypeName);
			if(iterType!=mapType.end())
			{
				iterParam->nTypeCode = (iterType->second).nTypeCode;
			}
			else
			{
				return -1;
			}
		}

		vector<SParam>& responeParam = iterMapMsgAttri->second.responeParams;
		for(iterParam=responeParam.begin();iterParam!=responeParam.end();++iterParam)
		{
			map<string,SServiceType>::iterator iterType = mapType.find(iterParam->sTypeName);
			if(iterType!=mapType.end())
			{
				iterParam->nTypeCode = (iterType->second).nTypeCode;
			}
			else
			{
				return -1;
			}
		}
	}

	return 0;
}

void CDBServiceConfig::PostProcessServiceTypes(map<string,SServiceType>& mapType)
{
	map<string,SServiceType>::iterator iterMapType;
	
	for(iterMapType = mapType.begin(); iterMapType != mapType.end(); ++iterMapType)
	{
		SServiceType& serviceType = iterMapType->second;
		
		if(serviceType.eType == DBP_DT_ARRAY)
		{
			map<string,SServiceType>::iterator iterFind = mapType.find(serviceType.sElementName);
			if(iterFind != mapType.end())
			{
				serviceType.nElementCode = (iterFind->second).nTypeCode;
				serviceType.nTypeCode = serviceType.nElementCode;
			}
		}
		else if(serviceType.eType == DBP_DT_TLV_ARRAY)
		{
			map<string,SServiceType>::iterator iterFind = mapType.find(serviceType.sElementName);
			if(iterFind != mapType.end())
			{
				serviceType.nElementCode = (iterFind->second).nTypeCode;
			}
		}
		else if (serviceType.eType == DBP_DT_TLV)
		{
			for(unsigned int i=0;i<serviceType.vecSubType.size();i++)
			{
				map<string,SServiceType>::iterator iterFind = mapType.find(serviceType.vecSubType[i].sElementName);
				
				if(iterFind != mapType.end())
				{
					serviceType.vecSubType[i].nTypeCode = (iterFind->second).nTypeCode;
					
					serviceType.vecSubType[i].eType = (iterFind->second).eType;
					
				}
				else
				{
					SC_XLOG(XLOG_ERROR, "CDBServiceConfig::%s,TLV TYPE %s Error\n",__FUNCTION__,serviceType.vecSubType[i].sElementName.c_str());
					break;
				}
			}
		}
	}

	return;
}

void CDBServiceConfig::ProcessXMLCDATA(string& strData)
{
	boost::replace_first(strData, "![cdata[", "");
	boost::replace_first(strData, "]]", "");
	return;
}


//---------------------------------------------------
void CDBServiceConfig::Dump()
{	
	map<int,SServiceDesc>::iterator iterServiceDesc;
	for(iterServiceDesc=mapServiceConfig.begin();iterServiceDesc!=mapServiceConfig.end();++iterServiceDesc)
	{
		SServiceDesc& desc = iterServiceDesc->second;

		SC_XLOG(XLOG_DEBUG,"-------DUMP SERVICE CONFIG-------\n");
		SC_XLOG(XLOG_DEBUG, "ServiceId=[%d],ServiceName=[%s]\n",
			desc.nServiceId,desc.sServiceName.c_str());

		SC_XLOG(XLOG_DEBUG,"-------TYPE INFO-------\n");
		map<string,SServiceType>::iterator iterType;
		for(iterType=desc.mapServiceType.begin();iterType!=desc.mapServiceType.end();++iterType)
		{	
			DumpTypeInfo(iterType->second);
		}
		
		SC_XLOG(XLOG_DEBUG,"-------MESSAGE INFO-------\n");
		
		map<int,SMsgAttri>::iterator iterMsgAttr;
		for(iterMsgAttr=desc.mapMsgAttri.begin();iterMsgAttr!=desc.mapMsgAttri.end();++iterMsgAttr)
		{
			DumpMsgAttri(iterMsgAttr->second);
		}

		SC_XLOG(XLOG_DEBUG,"-------END DUMP SERVICE CONFIG-------\n");
	}
}


void CDBServiceConfig::DumpMsgAttri(SMsgAttri& msgAttri)
{
	SC_XLOG(XLOG_DEBUG,"MsgId=[%d],MsgName=[%s]\n",msgAttri.nMsgId,msgAttri.sMsgName.c_str());
	
	vector<string>::iterator iterSQLs;
	for(iterSQLs=msgAttri.exeSQLs.begin();iterSQLs!=msgAttri.exeSQLs.end();++iterSQLs)
	{
		SC_XLOG(XLOG_DEBUG,"SQL=[%s]\n",(*iterSQLs).c_str());
	}

	SC_XLOG(XLOG_DEBUG,"--request params--\n");
	vector<SParam>::iterator iterReq;
	for(iterReq=msgAttri.requestParams.begin();iterReq!=msgAttri.requestParams.end();++iterReq)
	{
		SC_XLOG(XLOG_DEBUG,"Code=[%d],TypeName=[%s],BindName=[%s],ParamType=[%s]\n",
				iterReq->nTypeCode,iterReq->sTypeName.c_str(),
				iterReq->sBindName.c_str(),iterReq->sParamName.c_str());
	}

	SC_XLOG(XLOG_DEBUG,"--respone params--\n");
	vector<SParam>::iterator iterRes;
	for(iterRes=msgAttri.responeParams.begin();iterRes!=msgAttri.responeParams.end();++iterRes)
	{
		SC_XLOG(XLOG_DEBUG,"Code=[%d],TypeName=[%s],BindName=[%s],ParamType=[%s]\n",
				iterRes->nTypeCode,iterRes->sTypeName.c_str(),
				iterRes->sBindName.c_str(),iterRes->sParamName.c_str());
	}
	
}

void CDBServiceConfig::DumpTypeInfo(SServiceType& typeInfo)
{
	if(typeInfo.eType == DBP_DT_STRING)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[string],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
	}
	else if(typeInfo.eType == DBP_DT_INT)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[int],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
	}
	else if(typeInfo.eType == DBP_DT_LONG)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[long],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
	}
	else if(typeInfo.eType == DBP_DT_FLOAT)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[float],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
	}
	else if(typeInfo.eType == DBP_DT_STRUCT)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[struct],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
		SC_XLOG(XLOG_DEBUG,"--Struct SubTypes--\n");
		vector<_tagServiceType>::iterator iterSubTypes;
		for(iterSubTypes=typeInfo.vecSubType.begin();iterSubTypes!=typeInfo.vecSubType.end();++iterSubTypes)
		{
			DumpTypeInfo(*iterSubTypes);
		}
		SC_XLOG(XLOG_DEBUG,"--End Struct SubTypes--\n");
	}
	else if(typeInfo.eType == DBP_DT_TLV)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[tlv],TypeCode=[%d],TypeName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str());
		SC_XLOG(XLOG_DEBUG,"--TLV SubTypes--\n");
		vector<_tagServiceType>::iterator iterSubTypes;
		for(iterSubTypes=typeInfo.vecSubType.begin();iterSubTypes!=typeInfo.vecSubType.end();++iterSubTypes)
		{
			DumpTypeInfo(*iterSubTypes);
		}
		SC_XLOG(XLOG_DEBUG,"--End TLV SubTypes--\n");
	}
	else if(typeInfo.eType == DBP_DT_ARRAY)
	{
		SC_XLOG(XLOG_DEBUG,"Type=[array],TypeCode=[%d],TypeName=[%s],ElementCode=[%d],ElementName=[%s]\n",
			typeInfo.nTypeCode,typeInfo.sTypeName.c_str(),typeInfo.nElementCode,typeInfo.sElementName.c_str());
	}
	
}







