#include "AvenueTlvGroup.h"
#include <arpa/inet.h>
#include "SdkLogHelper.h"
namespace sdo{
namespace commonsdk{

void dumppmyackage(const void *pBuffer,unsigned int nPacketlen)
{
       SDK_XLOG(XLOG_DEBUG,"CSapDecodeMsg::dumppackage\n");
       string strPacket;
        unsigned char *pChar=(unsigned char *)pBuffer;
        int nLine=nPacketlen>>3;
        int nLast=(nPacketlen&0x7);
        int i=0;
        for(i=0;i<nLine;i++)
        {
            char szBuf[128]={0};
            unsigned char * pBase=pChar+(i<<3);
            sprintf(szBuf,"                [%2d]    %02x %02x %02x %02x    %02x %02x %02x %02x    ......\n",
                i,*(pBase),*(pBase+1),*(pBase+2),*(pBase+3),*(pBase+4),*(pBase+5),*(pBase+6),*(pBase+7));
            strPacket+=szBuf;
        }
        
        unsigned char * pBase=pChar+(i<<3);
        if(nLast>0)
        {
            for(int j=0;j<8;j++)
            {
                char szBuf[128]={0};
                if(j==0)
                {
                    sprintf(szBuf,"                [%2d]    %02x ",i,*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j<nLast&&j==3)
                {
                    sprintf(szBuf,"%02x    ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else if(j>=nLast&&j==3)
                {
                    strPacket+="      ";
                }
                else if(j<nLast)
                {
                    sprintf(szBuf,"%02x ",*(pBase+j));
                    strPacket+=szBuf;
                }
                else
                {
                    strPacket+="   ";
                }
                
            }
            strPacket+="   ......\n";
        }
        SDK_XLOG(XLOG_DEBUG,strPacket.c_str());
}


CAvenueTlvGroupEncoder::CAvenueTlvGroupEncoder(const SAvenueTlvGroupRule &oTlvRule,bool bRequest)
		:m_oTlvRule(oTlvRule),m_bRequest(bRequest),m_nValidateCode(0)
{
	m_mapRequired=m_oTlvRule.mapRequired;
}
void CAvenueTlvGroupEncoder::SetValue(const string& strName,const void* pValue,unsigned int nValueLen)
{
	char szName[64] = {0};
	char szStructField[64] = {0};
	int nArrayIndex=0;
	int nNum = sscanf(strName.c_str(),"%63[^'['][%d]['%63[^''']']",szName,&nArrayIndex,szStructField);
	if(nNum != 3) 
	{
		sscanf(strName.c_str(),"%63[^'[']['%63[^''']'][%d]",szName,szStructField,&nArrayIndex);
	}
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue,[%s]=%s[%d]['%s'], value[%x,%d]\n",strName.c_str(),szName,nArrayIndex,szStructField,pValue,nValueLen);
	
	//dumppmyackage(pValue,nValueLen);
	map<string,SAvenueTlvTypeConfig>::const_iterator itr=m_oTlvRule.mapTlvType.find(szName);
	if(itr==m_oTlvRule.mapTlvType.end())
	{
		SDK_XLOG(XLOG_WARNING, "SetValue [%s]  failed: no typecode\n", strName.c_str());
		return;
	}

	const SAvenueTlvTypeConfig &oConfig=itr->second;
	m_mapRequired.erase(oConfig.nType);
	
	if(szStructField[0]!='\0'&&oConfig.emType==MSG_FIELD_STRUCT)
	{
		map<string,SAvenueStructConfig>::const_iterator itrStructConfig=oConfig.mapStructConfig.find(szStructField);
		if(itrStructConfig==oConfig.mapStructConfig.end()) 
		{
			SDK_XLOG(XLOG_WARNING, "SetValue [%s]  failed: no struct filed\n",strName.c_str());
			return ;
		}
		const SAvenueStructConfig &oStructConfig=itrStructConfig->second;
			
		char szStructLocName[64]={0};
		snprintf(szStructLocName,64,"%s[%d]",szName,nArrayIndex);
		map<string,unsigned char *>::const_iterator itrStruct=m_mapStructLoc.find(szStructLocName);
		if(itrStruct!=m_mapStructLoc.end())
		{
			unsigned char *pStructValue=itrStruct->second;
			SetStructItem(strName,szStructField,oStructConfig,pStructValue,pValue,nValueLen);
		}
		else
		{
			unsigned int nStructLen=oConfig.nStructAllLen;
			if(m_buffer.capacity()<nStructLen+4)
			{
				m_buffer.add_capacity(SAP_ALIGN(nStructLen+4-m_buffer.capacity()));
			}
			unsigned char *pStructValue=m_buffer.top()+4;
			
			SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)m_buffer.top();
			pAtti->wType=htons(oConfig.nType);
			pAtti->wLength=htons(nStructLen+sizeof(SAvenueMsgAttribute));
			memset(pAtti->acValue,0,nStructLen);
			m_buffer.inc_loc(sizeof(SAvenueMsgAttribute)+nStructLen);

			SetStructItem(strName,szStructField,oStructConfig,pStructValue,pValue,nValueLen);
			m_mapStructLoc[szStructLocName]=pStructValue;
		}
	}
	else
	{
		
		if(IsEnableLevel(SDK_MODULE,XLOG_NOTICE))
		{
			if(oConfig.emType==MSG_FIELD_INT)
			{
				SDK_XLOG(XLOG_NOTICE, "\t\tSetValue: %s = %d\n",strName.c_str(),(*(int *)(pValue)));
			}
			else if(oConfig.emType==MSG_FIELD_STRING)
			{
				string strValue=string((const char *)pValue,nValueLen);
				SDK_XLOG(XLOG_NOTICE, "\t\tSetValue: %s = %s\n",strName.c_str(),strValue.c_str());
			}
		}
		int nValue=0;
		string strEncoder;
		if(oConfig.emType==MSG_FIELD_INT)
		{
			ValidateValue(oConfig.oFeature ,pValue,nValueLen);
			nValue=htonl(*(int *)(pValue));
			pValue=&nValue;
			nValueLen=4;
		}
		else if(oConfig.emType==MSG_FIELD_STRING)
		{
			if(oConfig.oFeature.funcEncoder!=NULL&&m_bRequest==false)
			{
				strEncoder=oConfig.oFeature.funcEncoder(pValue,nValueLen,oConfig.oFeature.mapEncoderParam);
				pValue=strEncoder.data();
				nValueLen=strEncoder.length();
			}
			ValidateValue(oConfig.oFeature ,pValue,nValueLen);
		}
		

		unsigned int nFactLen=((nValueLen&0x03)!=0?((nValueLen>>2)+1)<<2:nValueLen);
		if(m_buffer.capacity()<nFactLen+4)
		{
			m_buffer.add_capacity(SAP_ALIGN(nFactLen+4-m_buffer.capacity()));
		}
		SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)m_buffer.top();
		pAtti->wType=htons(oConfig.nType);
		pAtti->wLength=htons(nValueLen+sizeof(SAvenueMsgAttribute));
		memcpy(pAtti->acValue,pValue,nValueLen);
		memset(pAtti->acValue+nValueLen,0,nFactLen-nValueLen);
			
		if(oConfig.emType==MSG_FIELD_STRUCT)
		{
			char* pCurrentPtr = (char*)(pAtti->acValue);
			vector<string>::const_iterator it_name = oConfig.vecStructName.begin();
			for(;it_name!=oConfig.vecStructName.end();it_name++)
			{
				SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue name[%s]\n",it_name->c_str());
				map<string,SAvenueStructConfig>::const_iterator itrStruct = oConfig.mapStructConfig.find(*it_name);
				const SAvenueStructConfig & oStruct=itrStruct->second;
				if(oStruct.emStructType==MSG_FIELD_INT)
				{
					SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue name[%s] INT\n",it_name->c_str());
					//ValidateValue(oStruct.oFeature ,(char *)pAtti->acValue+oStruct.nLoc,4);
					//int nValue = htonl(*(int *)((char *)pAtti->acValue+oStruct.nLoc));
					//memcpy((char *)pAtti->acValue+oStruct.nLoc, &nValue, 4);
					int nValue = htonl(*(int *)((char *)pCurrentPtr));
					memcpy((char *)pCurrentPtr, &nValue, 4);
					pCurrentPtr +=4;
				}
				else if (oStruct.bSystemString)
				{
					SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue name[%s] SYS\n",it_name->c_str());
					int nStrLen = (*(int *)(pCurrentPtr));
					SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue [%d] nStrLen[%d]\n",oStruct.nLoc,nStrLen);
					int nNetStrLen = htonl(nStrLen);
					memcpy(pCurrentPtr, &nNetStrLen, 4);
					pCurrentPtr +=4;
					if (nStrLen >0)
						pCurrentPtr +=((nStrLen&0x03)!=0?((nStrLen>>2)+1)<<2:nStrLen);;
				}
				else if(oStruct.emStructType==MSG_FIELD_STRING)
				{
					SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetValue name[%s] string\n",it_name->c_str());
					int nStrLen=(oStruct.nLen==-1?(nValueLen-oStruct.nLoc):oStruct.nLen);
					if (oStruct.nLen!=-1)
					{
						pCurrentPtr +=oStruct.nLen;
					}
					if(oStruct.oFeature.funcEncoder!=NULL&&m_bRequest==false)
					{
						strEncoder=oStruct.oFeature.funcEncoder((char *)pAtti->acValue+oStruct.nLoc,nStrLen,oStruct.oFeature.mapEncoderParam);
						memset((char *)pAtti->acValue+oStruct.nLoc, 0, nStrLen);
						if(nStrLen>strEncoder.length())
						{
							nStrLen=strEncoder.length();
						}
						memcpy((char *)pAtti->acValue+oStruct.nLoc, strEncoder.data(), nStrLen);
					}
					ValidateValue(oStruct.oFeature ,(char *)pAtti->acValue+oStruct.nLoc,nStrLen);
				}
			}	
		}
		m_buffer.inc_loc(sizeof(SAvenueMsgAttribute)+nFactLen);
	}
}
void CAvenueTlvGroupEncoder::SetStructItem(const string &strName,char *szStructField,const SAvenueStructConfig & oConfig,unsigned char *pStructValue,const void* pValue,unsigned int nValueLen)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupEncoder::SetStructItem,filed[%s] len[%d], isRequest[%d], configLen[%d]\n",szStructField,nValueLen,m_bRequest,oConfig.nLen);
	int nFactLen=0;
	string strEncoder;
	int nValue;
	if(IsEnableLevel(SDK_MODULE,XLOG_NOTICE))
	{
		if(oConfig.emStructType==MSG_FIELD_INT)
		{
			SDK_XLOG(XLOG_NOTICE, "\t\tSetValue: %s = %d\n",strName.c_str(),htonl(*(int *)pValue));
		}
		else if(oConfig.emStructType==MSG_FIELD_STRING)
		{
			string strValue=string((const char *)pValue,nValueLen);
			SDK_XLOG(XLOG_NOTICE, "\t\tSetValue: %s = %s\n",strName.c_str(),strValue.c_str());
		}
	}	
	if(oConfig.emStructType==MSG_FIELD_INT)
	{
		ValidateValue(oConfig.oFeature ,pValue,nValueLen);
		nValue=htonl(*(int *)(pValue));
		pValue=&nValue;
		nValueLen=4;
	}
	else if(oConfig.emStructType==MSG_FIELD_STRING)
	{
		if(oConfig.oFeature.funcEncoder!=NULL&&m_bRequest==false)
		{
			strEncoder=oConfig.oFeature.funcEncoder(pValue,nValueLen,oConfig.oFeature.mapEncoderParam);
			pValue=strEncoder.data();
			nValueLen=strEncoder.length();
		}
		ValidateValue(oConfig.oFeature ,pValue,nValueLen);
	}
	
	if(oConfig.nLen!=-1)
	{
		nFactLen=nValueLen<oConfig.nLen?nValueLen:oConfig.nLen;
		memcpy(pStructValue+oConfig.nLoc,pValue,nFactLen);
	}
	else if(oConfig.nLen==-1&&m_buffer.top()>pStructValue+oConfig.nLoc+AVENUE_STRUCT_LASTSTRING_DEFAULT_SIZE)
	{
		nFactLen=nValueLen<AVENUE_STRUCT_LASTSTRING_DEFAULT_SIZE?nValueLen:AVENUE_STRUCT_LASTSTRING_DEFAULT_SIZE;
		memcpy(pStructValue+oConfig.nLoc,pValue,nFactLen);
	}
	else
	{
		if(m_buffer.capacity()<nValueLen+4)
		{
			m_buffer.add_capacity(SAP_ALIGN(nValueLen+4-m_buffer.capacity()));
		}
		memcpy(pStructValue+oConfig.nLoc,pValue,nValueLen);

		unsigned int nNewStructAllLen=oConfig.nLoc+nValueLen;
		unsigned int nNewLen=((nNewStructAllLen&0x03)!=0?((nNewStructAllLen>>2)+1)<<2:nNewStructAllLen);

		SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)(pStructValue-4);
		pAtti->wLength=htons(nNewStructAllLen+sizeof(SAvenueMsgAttribute));
		m_buffer.inc_loc(nNewLen-(m_buffer.top()-pStructValue));
	}
}
int CAvenueTlvGroupEncoder::GetValidateCode()
{
	map<unsigned int,int>::iterator itr;
	for(itr=m_mapRequired.begin();itr!=m_mapRequired.end();++itr)
	{
		m_nValidateCode=itr->second;
		return m_nValidateCode;
	}
	
	return m_nValidateCode;
}
void CAvenueTlvGroupEncoder::ValidateValue(const SValueFeatueConfig &oFeature,const void* pValue,unsigned int nValueLen)
{
	if(oFeature.funcValidate!=NULL&&m_nValidateCode==0&&oFeature.funcValidate(pValue, nValueLen, oFeature.strValidatorParam)==false)
	{
		m_nValidateCode=oFeature.nReturnCodeIfValidateFail;
	}
}
void CAvenueTlvGroupEncoder::Reset()
{
	m_buffer.reset_loc(0);
	m_nValidateCode=0;
	m_mapStructLoc.clear();
	m_mapRequired.clear();
	m_mapRequired=m_oTlvRule.mapRequired;
}
CAvenueTlvGroupDecoder::CAvenueTlvGroupDecoder(const SAvenueTlvGroupRule &oTlvRule,bool bRequest,const void *pBuffer,unsigned int nLen)
	:m_oTlvRule(oTlvRule),m_bRequest(bRequest),m_pBuffer(pBuffer),m_nLen(nLen),m_nValidateCode(0)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s line[%d] nLen[%d]\n",__FUNCTION__,__LINE__,nLen);
	
	//dumppmyackage(m_pBuffer,m_nLen);
	const unsigned char *ptrLoc= (unsigned char *)m_pBuffer;

	while(ptrLoc< (unsigned char *)m_pBuffer+m_nLen)
	{
		SAvenueMsgAttribute *pAtti=(SAvenueMsgAttribute *)ptrLoc;
		unsigned short nAttriLen=ntohs(pAtti->wLength);
		unsigned short uType=ntohs(pAtti->wType);
		if(nAttriLen<sizeof(SAvenueMsgAttribute))
		{
		    break;
		}	
		
		SStructValue objValue;
		objValue.nLen=nAttriLen-sizeof(SAvenueMsgAttribute);
		objValue.pValue=pAtti->acValue;
		m_mapValueBuffer.insert(TlvValueMultiMap::value_type(uType,objValue));
		int nFactLen=((nAttriLen&0x03)!=0?((nAttriLen>>2)+1)<<2:nAttriLen);
		ptrLoc+=nFactLen;

		map<unsigned int, SAvenueTlvTypeConfig>::const_iterator itrId=m_oTlvRule.mapIdTlvType.find(uType);
		if(itrId!=m_oTlvRule.mapIdTlvType.end())
		{
			const SAvenueTlvTypeConfig & oConfig=itrId->second;
			if(oConfig.emType==MSG_FIELD_INT)
			{
				int nValue = ntohl(*(int *)pAtti->acValue);
				memcpy(pAtti->acValue, &nValue, 4);
			}
			else if(oConfig.emType==MSG_FIELD_STRUCT)
			{
				char* pCurrentPtr = (char*)(pAtti->acValue);
				vector<string>::const_iterator it_name = oConfig.vecStructName.begin();
				for(;it_name!=oConfig.vecStructName.end();it_name++)
				{
					map<string,SAvenueStructConfig>::const_iterator itrStruct = oConfig.mapStructConfig.find(*it_name);
					const SAvenueStructConfig & oStruct=itrStruct->second;
					if(oStruct.emStructType==MSG_FIELD_INT)
					{
						//int nValue = ntohl(*(int *)(pAtti->acValue+oStruct.nLoc));
						//memcpy(pAtti->acValue+oStruct.nLoc, &nValue, 4);
						int nValue = ntohl(*(int *)(pCurrentPtr));
						memcpy(pCurrentPtr, &nValue, 4);
						pCurrentPtr +=4;
					}
					else if (oStruct.bSystemString)
					{
						int nStrLen = ntohl(*(int *)(pCurrentPtr));
						SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s [%d] nStrLen[%d]\n",__FUNCTION__,oStruct.nLoc,nStrLen);
						memcpy(pCurrentPtr, &nStrLen, 4);
						pCurrentPtr +=4;
						if (nStrLen >0)
							pCurrentPtr +=((nStrLen&0x03)!=0?((nStrLen>>2)+1)<<2:nStrLen);;
					}
					else if(oStruct.emStructType==MSG_FIELD_STRING)
					{
						int nStrLen=oStruct.nLen;
						SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::%s nStrLen[%d]\n",__FUNCTION__,nStrLen);
						if (oStruct.nLen!=-1)
						{
							pCurrentPtr +=oStruct.nLen;
						}
					}
				}
			}
			else if(oConfig.emType==MSG_FIELD_ARRAY)
			{
				SDK_XLOG(XLOG_WARNING, "CAvenueTlvGroupDecoder:%s line[%d] not support\n",__FUNCTION__,__LINE__);
			}
			
			if(oConfig.oFeature.funcValidate!=NULL)
			{
				SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s line[%d]\n",__FUNCTION__,__LINE__);
				ValidateValue(oConfig,objValue.pValue,objValue.nLen);
			}
		}
	}

	map<unsigned int,string>::const_iterator itrDefaultString;
	for(itrDefaultString=m_oTlvRule.mapStringDefault.begin();itrDefaultString!=m_oTlvRule.mapStringDefault.end();itrDefaultString++)
	{
		unsigned short nCode=itrDefaultString->first;
		const string & strDefault=itrDefaultString->second;
		TlvValueMultiMap::iterator itrBuffer=m_mapValueBuffer.find(nCode);
		if(itrBuffer==m_mapValueBuffer.end()) 
		{
			SStructValue objValue;
			objValue.pValue=(void *)(strDefault.data());
			objValue.nLen=strDefault.length();
			m_mapValueBuffer.insert(TlvValueMultiMap::value_type(nCode,objValue));
			SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s line[%d] nCode[%d] %p\n",__FUNCTION__,__LINE__,nCode,objValue.pValue);
		}
	}
	map<unsigned int,int>::const_iterator itrDefaultInt;
	for(itrDefaultInt=m_oTlvRule.mapIntDefault.begin();itrDefaultInt!=m_oTlvRule.mapIntDefault.end();itrDefaultInt++)
	{
		unsigned short nCode=itrDefaultInt->first;
		const int &nDefault=itrDefaultInt->second;
		TlvValueMultiMap::iterator itrBuffer=m_mapValueBuffer.find(nCode);
		if(itrBuffer==m_mapValueBuffer.end()) 
		{
			SStructValue objValue;
			objValue.pValue=(void *)(&(nDefault));
			objValue.nLen=4;
			m_mapValueBuffer.insert(TlvValueMultiMap::value_type(nCode,objValue));
			SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s line[%d] nCode[%d] %p\n",__FUNCTION__,__LINE__,nCode,objValue.pValue);
		}
	}
	if(m_nValidateCode==0) 
	{
		map<unsigned int,int>::const_iterator itrRequired;
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s required rulesize[%d]\n",__FUNCTION__,m_oTlvRule.mapRequired.size());
		for(itrRequired=m_oTlvRule.mapRequired.begin();itrRequired!=m_oTlvRule.mapRequired.end();itrRequired++)
		{
			SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder:%s required [%d][%d]\n",__FUNCTION__,itrRequired->first,
			itrRequired->second);
			unsigned short nCode=itrRequired->first;
			TlvValueMultiMap::iterator itrBuffer=m_mapValueBuffer.find(nCode);
			if(itrBuffer==m_mapValueBuffer.end()) 
			{
				m_nValidateCode=itrRequired->second;

				map<unsigned int,string>::const_iterator itrRequiredMsg;
				if((itrRequiredMsg=m_oTlvRule.mapRequiredReturnMsg.find(nCode))!=m_oTlvRule.mapRequiredReturnMsg.end())
				{
					m_strValidateMsg=itrRequiredMsg->second;
				}
				break;
			}
		}
	}
	if(IsEnableLevel(SDK_MODULE,XLOG_TRACE))
	{
		Dump();
	}
}
void CAvenueTlvGroupDecoder::ValidateValue(const SAvenueTlvTypeConfig &oConfig,const void* pValue,unsigned int nValueLen)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::ValidateValue, ValidateCode[%d],type[%d],func[%x]\n",m_nValidateCode,oConfig.emType,oConfig.oFeature.funcValidate);
	if(m_nValidateCode!=0) return;
	
	if(oConfig.emType==MSG_FIELD_INT||oConfig.emType==MSG_FIELD_STRING)
	{
		if(oConfig.oFeature.funcValidate!=NULL&&oConfig.oFeature.funcValidate(pValue, nValueLen, oConfig.oFeature.strValidatorParam)==false)
		{
			m_nValidateCode=oConfig.oFeature.nReturnCodeIfValidateFail;
			m_strValidateMsg=oConfig.oFeature.strReturnMsg;
			return;
		}
	}
	else if(oConfig.emType==MSG_FIELD_STRUCT&&oConfig.oFeature.funcValidate!=NULL)
	{
		map<string,SAvenueStructConfig>::const_iterator itr;
		for(itr=oConfig.mapStructConfig.begin();itr!=oConfig.mapStructConfig.end();++itr)
		{
			const SAvenueStructConfig& oStruct=itr->second;
			SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::ValidateValue %d, ValidateCode[%d],type[%d],struct func[%x]\n",
				__LINE__,m_nValidateCode,oStruct.emStructType,oStruct.oFeature.funcValidate);
			if(oStruct.oFeature.funcValidate!=NULL)
			{
				int nFactLen=(oStruct.nLen==-1?(nValueLen-oStruct.nLoc):oStruct.nLen);
				if(oStruct.oFeature.funcValidate((unsigned char *)pValue+oStruct.nLoc, nFactLen, oStruct.oFeature.strValidatorParam)==false)
				{
					m_nValidateCode=oStruct.oFeature.nReturnCodeIfValidateFail;
					m_strValidateMsg=oStruct.oFeature.strReturnMsg;
					return;
				}
			}
		}
	}		
}
int CAvenueTlvGroupDecoder::GetValue(const string& strName,void **pBuffer, unsigned int *pLen,EValueType &eValueType)
{
	char szName[64] = {0};
	char szStructField[64] = {0};
	int nArrayIndex=-1;
	int nNum = sscanf(strName.c_str(),"%63[^'['][%d]['%63[^''']']",szName,&nArrayIndex,szStructField);
	if(nNum != 3) 
	{
		sscanf(strName.c_str(),"%63[^'[']['%63[^''']']",szName,szStructField);
	}
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValue, [%s]=%s[%d]['%s']\n",strName.c_str(),szName,nArrayIndex,szStructField);
	
	map<string,SAvenueTlvTypeConfig>::const_iterator itr=m_oTlvRule.mapTlvType.find(szName);
	if(itr==m_oTlvRule.mapTlvType.end())
	{
		SDK_XLOG(XLOG_WARNING, "GetValue [%s]  failed: no typecode\n", strName.c_str());
		return -1;
	}
	
	const SAvenueTlvTypeConfig &oConfig=itr->second;
	eValueType=oConfig.emType;
	const SValueFeatueConfig * pFeature=&(oConfig.oFeature);

	if(m_bRequest&&pFeature->funcEncoder!=NULL)
	{
		map<string ,string>::iterator itr=m_mapEncoderResult.find(strName);
		if(itr!= m_mapEncoderResult.end())
		{
			string &strEncoder=itr->second;
			*pBuffer=(void *)(strEncoder.data());
			*pLen=strEncoder.length();
			eValueType=MSG_FIELD_STRING;
			return 0;
		}
	}
	
	const SStructValue * pTlvValueBuffer=NULL;
	if(nArrayIndex==0||nArrayIndex==-1)
	{
		TlvValueMultiMap::iterator itrBuffer=m_mapValueBuffer.find(oConfig.nType);
		if(itrBuffer==m_mapValueBuffer.end()) 
		{
			SDK_XLOG(XLOG_WARNING, "GetValue [%s]  failed: buffer no this field & no default\n", strName.c_str());
			return -1;
		}

		pTlvValueBuffer=&(itrBuffer->second);
	}
	else
	{
		std::pair<TlvValueMultiMap::iterator, TlvValueMultiMap::iterator> itr_pair = m_mapValueBuffer.equal_range(oConfig.nType);
		if (itr_pair.first == m_mapValueBuffer.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValues [%s] no oConfig.nType[%d]\n", strName.c_str(),oConfig.nType);
			return -1;
		}
		
		TlvValueMultiMap::iterator itrItem;
		int nIndex=0;
		bool isFind=false;
		for(itrItem=itr_pair.first; itrItem!=itr_pair.second;++itrItem,++nIndex)
		{
			if(nIndex==nArrayIndex)
			{
				pTlvValueBuffer=&(itrItem->second);
				isFind=true;
			}
		}
		if(isFind==false)
		{
			SDK_XLOG(XLOG_WARNING, "GetValue [%s]  failed: buffer no this field & no default\n", strName.c_str());
			return -1;
		}
	}
	
	if(szStructField[0]!='\0'&&oConfig.emType==MSG_FIELD_STRUCT)
	{
		map<string,SAvenueStructConfig>::const_iterator itrStruct=oConfig.mapStructConfig.find(szStructField);
		if(itrStruct==oConfig.mapStructConfig.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValue [%s]  failed: buffer no this struct field \n", strName.c_str());
			return -1;
		}
		
		
		const SAvenueStructConfig &oStructConfig=itrStruct->second;
		eValueType=oStructConfig.emStructType;
		pFeature=&(oStructConfig.oFeature);
		
		*pBuffer=(unsigned char *)(pTlvValueBuffer->pValue)+oStructConfig.nLoc;
		if(oStructConfig.nLen==-1)
		{
			*pLen=pTlvValueBuffer->nLen-oStructConfig.nLoc;
		}
		else
		{
			*pLen=oStructConfig.nLen;
		}
		
		
	}
	else
	{
		*pBuffer=pTlvValueBuffer->pValue;
		*pLen=pTlvValueBuffer->nLen;
	}

	if(m_bRequest&&eValueType==MSG_FIELD_STRING&&pFeature->funcEncoder!=NULL)
	{
		SDK_XLOG(XLOG_DEBUG, "[%s]--------------------- \n", string((const char*)*pBuffer,*pLen).c_str());
		m_mapEncoderResult[strName]=pFeature->funcEncoder(*pBuffer,*pLen,pFeature->mapEncoderParam);
		string &strEncoder=m_mapEncoderResult[strName];
		*pBuffer=(void *)(strEncoder.data());
		*pLen=strEncoder.length();
	}

	if(IsEnableLevel(SDK_MODULE,XLOG_NOTICE))
	{
		if(eValueType==MSG_FIELD_INT)
		{
			SDK_XLOG(XLOG_NOTICE, "\t\tGetValue: %s = %d\n",strName.c_str(),(*(int *)(*pBuffer)));
		}
		else if(eValueType==MSG_FIELD_STRING)
		{
			string strValue=string((const char *)(*pBuffer),*pLen);
			SDK_XLOG(XLOG_NOTICE, "\t\tGetValue: %s = %s\n",strName.c_str(),strValue.c_str());
		}
	}
	return 0;
}
int CAvenueTlvGroupDecoder::GetValues(const string& strName,vector<SStructValue> &vecValues,EValueType &eValueType)
{
	char szName[64] = {0};
	char szStructField[64] = {0};
	int nArrayIndex=-1;
	int nNum = sscanf(strName.c_str(),"%63[^'['][%d]['%63[^''']']",szName,&nArrayIndex,szStructField);
	if(nNum != 3) 
	{
		sscanf(strName.c_str(),"%63[^'[']['%63[^''']']",szName,szStructField);
	}
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, [%s]=%s[%d]['%s']\n",strName.c_str(),szName,nArrayIndex,szStructField);
	
	map<string,SAvenueTlvTypeConfig>::const_iterator itr=m_oTlvRule.mapTlvType.find(szName);
	if(itr==m_oTlvRule.mapTlvType.end())
	{
		SDK_XLOG(XLOG_WARNING, "GetValues [%s]  failed: no typecode\n", strName.c_str());
		return -1;
	}
	
	const SAvenueTlvTypeConfig &oConfig=itr->second;
	eValueType=oConfig.emType;
	const SValueFeatueConfig * pFeature=&(oConfig.oFeature);
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
	if(m_bRequest&&pFeature->funcEncoder!=NULL)
	{
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
		map<string ,vector<string> >::iterator itr=m_mapEncoderResults.find(strName);
		if(itr!= m_mapEncoderResults.end())
		{
			vector<string> & vecEncoders=itr->second;
			for(vector<string> ::iterator itrDetail=vecEncoders.begin();itrDetail!=vecEncoders.end();itrDetail++)
			{
				string &strEncoder=*itrDetail;
				SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d, strEncoder[%s]\n", __LINE__,strEncoder.c_str());
				SStructValue oValue;
				oValue.pValue=(void *)(strEncoder.data());
				oValue.nLen=strEncoder.length();
				vecValues.push_back(oValue);
				eValueType=MSG_FIELD_STRING;
			}
			return 0;
		}
	}
	
	const SStructValue * pTlvValueBuffer=NULL;
	if(oConfig.isArray==false||nArrayIndex==0)
	{
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
		TlvValueMultiMap::iterator itrBuffer=m_mapValueBuffer.find(oConfig.nType);
		if(itrBuffer==m_mapValueBuffer.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValues [%s]  failed: buffer no this field & no default\n", strName.c_str());
			return -1;
		}

		pTlvValueBuffer=&(itrBuffer->second);
		GetValueFromBuffer(oConfig,strName,szStructField,pTlvValueBuffer,vecValues,eValueType);
	}
	else if(oConfig.isArray && nArrayIndex>0)
	{
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
		std::pair<TlvValueMultiMap::iterator, TlvValueMultiMap::iterator> itr_pair = m_mapValueBuffer.equal_range(oConfig.nType);
		if (itr_pair.first == m_mapValueBuffer.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValues [%s] no oConfig.nType[%d]\n", strName.c_str(),oConfig.nType);
			return -1;
		}
		TlvValueMultiMap::iterator itrItem;
		int nIndex=0;
		bool isFind=false;
		for(itrItem=itr_pair.first; itrItem!=itr_pair.second;++itrItem,++nIndex)
		{
			if(nIndex==nArrayIndex)
			{
				pTlvValueBuffer=&(itrItem->second);
				isFind=true;
			}
		}
		if(isFind==false) 
		{
			SDK_XLOG(XLOG_WARNING, "GetValues [%s]  failed: buffer no this field & no default\n", strName.c_str());
			return -1;
		}
		GetValueFromBuffer(oConfig,szStructField,strName,pTlvValueBuffer,vecValues,eValueType);
	}
	else
	{
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d  type[%d]\n", __LINE__,oConfig.nType);
		std::pair<TlvValueMultiMap::iterator, TlvValueMultiMap::iterator> itr_pair = m_mapValueBuffer.equal_range(oConfig.nType);
		if (itr_pair.first == m_mapValueBuffer.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValues [%s] no oConfig.nType[%d]\n", strName.c_str(),oConfig.nType);
			return -1;
		}
		TlvValueMultiMap::iterator itrItem;
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
		for(itrItem=itr_pair.first; itrItem!=itr_pair.second;++itrItem)
		{
			SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValues, %d\n", __LINE__);
			pTlvValueBuffer=&(itrItem->second);
			GetValueFromBuffer(oConfig,strName,szStructField,pTlvValueBuffer,vecValues,eValueType);
		}
	}
	if(IsEnableLevel(SDK_MODULE,XLOG_NOTICE))
	{
		if(oConfig.emType==MSG_FIELD_INT && vecValues.size()>0)
		{
			SDK_XLOG(XLOG_NOTICE, "\t\tGetValues: %d elements, %s[0] = %d\n",vecValues.size(),strName.c_str(),(*(int *)(vecValues[0].pValue)));
		}
		else if(oConfig.emType==MSG_FIELD_STRING && vecValues.size()>0)
		{
			string strValue=string((const char *)(vecValues[0].pValue),vecValues[0].nLen);
			SDK_XLOG(XLOG_NOTICE, "\t\tGetValues: %d elements, %s[0] = %s\n",vecValues.size(),strName.c_str(),strValue.c_str());
		}
	}
	return 0;
}
int CAvenueTlvGroupDecoder::GetValueFromBuffer(const SAvenueTlvTypeConfig& oConfig,const string & strName,const string & strStructField, const SStructValue * pBuffer,vector<SStructValue> &vecValues,EValueType &eValueType)
{
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder::GetValueFromBuffer, filed [%s], struct field['%s'], buffer type[%d]\n",strName.c_str(),strStructField.c_str(),oConfig.emType);
	SStructValue oBufferValue=*pBuffer;
	eValueType=oConfig.emType;
	const SValueFeatueConfig * pFeature=&(oConfig.oFeature);
	if(strStructField!=""&&oConfig.emType==MSG_FIELD_STRUCT)
	{
		map<string,SAvenueStructConfig>::const_iterator itrStruct=oConfig.mapStructConfig.find(strStructField);
		if(itrStruct==oConfig.mapStructConfig.end())
		{
			SDK_XLOG(XLOG_WARNING, "GetValue [%s]  failed: no struct filed\n",strName.c_str());
			return -1;
		}

		const SAvenueStructConfig &oStructConfig=itrStruct->second;
		eValueType=oStructConfig.emStructType;
		pFeature=&(oStructConfig.oFeature);
	
		oBufferValue.pValue=(unsigned char *)(pBuffer->pValue)+oStructConfig.nLoc;
		if(oStructConfig.nLen==-1)
		{
			oBufferValue.nLen=pBuffer->nLen-oStructConfig.nLoc;
		}
		else
		{
			oBufferValue.nLen=oStructConfig.nLen;
		}
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder %d",__LINE__);
	}
	if(m_bRequest&&eValueType==MSG_FIELD_STRING&&pFeature->funcEncoder!=NULL)
	{
		vector<string> &vecEncoders=m_mapEncoderResults[strName];
		vecEncoders.push_back(pFeature->funcEncoder(oBufferValue.pValue,oBufferValue.nLen,pFeature->mapEncoderParam));
		string& strEncoder=*(vecEncoders.rbegin());
		oBufferValue.pValue=(void *)(strEncoder.data());
		oBufferValue.nLen=strEncoder.length();
		SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder %d",__LINE__);
	}
	SDK_XLOG(XLOG_DEBUG, "CAvenueTlvGroupDecoder %d\n",__LINE__);
	vecValues.push_back(oBufferValue);
	return 0;
}
void CAvenueTlvGroupDecoder::Dump()
{
	SDK_XLOG(XLOG_TRACE, "#####begin CAvenueTlvGroupDecoder::decode result  Dump\n");
	
	TlvValueMultiMap::iterator itrBuffer;
	for(itrBuffer=m_mapValueBuffer.begin();itrBuffer!=m_mapValueBuffer.end();++itrBuffer)
	{
		unsigned short nKey=itrBuffer->first;
		SStructValue oValue=itrBuffer->second;
		SDK_XLOG(XLOG_TRACE, "    %d  =  loc[%d]  len[%d]\n",nKey,(char *)oValue.pValue-(char *)m_pBuffer,oValue.nLen);
	}
	//dumppmyackage(m_pBuffer,m_nLen);
	SDK_XLOG(XLOG_TRACE, "#####end CAvenueTlvGroupDecoder::Dump\n");
}
}
}

