#include <stdio.h>
#include "AvenueServiceConfigs.h"
#include "SdkLogHelper.h"
#include <arpa/inet.h>

#include "AvenueMsgHandler.h"
#include <time.h>
#include <sys/time.h>

#include "testcommon.cpp"
#include "test1.cpp"
#include "test2.cpp"
#include "testphp.cpp"

typedef struct stTestStruct
{
	char a[64];
	int n;
	char b[16];
}STestStruct;
void testPerformance()
{	
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return;
	}
	
	time_t t1, t2;
	time(&t1);

	struct timeval tm1,tm2;
	gettimeofday(&tm1,0);
	for(int i=0;i<10000;++i)
	{
		CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
		CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
		
		m_pEncoderHandler->SetValue("areaId",9);
		m_pEncoderHandler->SetValue("areaId",12);
		m_pEncoderHandler->Encode();
		m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	
		void *pData;
		int nLen;
		m_pDecoderHandler->GetValue("version",&pData,&nLen);
		m_pDecoderHandler->GetValue("areaId",&pData,&nLen);
		m_pDecoderHandler->GetValue("areaId[1]",&pData,&nLen);
		m_pDecoderHandler->GetValue("areaId[0]",&pData,&nLen);
		delete m_pEncoderHandler;
		delete m_pDecoderHandler;

	}
	gettimeofday(&tm2,0);
	time(&t2);
	printf("performance 10000: %d   %d.%06d\n",t2-t1,tm2.tv_sec-tm1.tv_sec,tm2.tv_usec-tm1.tv_usec);
}

int main()
{
    XLOG_INIT((GetCurrentPath()+s_szLogName).c_str(), true);
    XLOG_REGISTER(SDK_MODULE, "sdk_config");
	
    CAvenueServiceConfigs oServiceConfig;
    if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
    {
		printf("%d: load avenue fail!\n",__LINE__);
        return -1;
    }
	CSapEncoder packet(SAP_PACKET_REQUEST,55810,2,0);
	packet.SetValue(1,"abc");//version
	packet.SetValue(2,5);//areaId
	packet.SetValue(2,6);//areaId
	packet.SetValue(2,7);//areaId
	STestStruct s1;
	memset(&s1,0,sizeof(s1));
	memcpy(s1.a,"ert",3);
	s1.n=htonl(10);
	packet.SetValue(15,&s1,sizeof(s1));//presentList
	
	STestStruct s2;
	memset(&s2,0,sizeof(s2));
	memcpy(s2.a,"wer",3);
	s2.n=htonl(12);
	packet.SetValue(15,&s2,sizeof(s2));//presentList
	//printf("packet len[%d]\n",packet.GetLen());
	//dump_sap_packet_notice(packet.GetBuffer());
/*
	start test getValue
*/
	CAvenueMsgHandler * m_pReqHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);	 
	//CAvenueMessageDecode oDecode;
    //oDecode.DecodeMsg((char *)(packet.GetBuffer())+44, packet.GetLen()-44, m_pReqHandler);
    	 m_pReqHandler->Decode((char *)(packet.GetBuffer())+44, packet.GetLen()-44);

	void  *pVersion=NULL;
	int nLen=0;
	m_pReqHandler->GetValue("version",&pVersion,&nLen);
	printf("1 Passed: %d, get version[%s], nlen[%d], from buffer[%d], first char[%c], buffer base[%ld],pversion[%ld]\n",
	string((char *)pVersion)=="abc",(char *)pVersion,nLen, (char *)pVersion-(char *)(packet.GetBuffer()),*((char *)pVersion),packet.GetBuffer(),pVersion);
	int  ret=m_pReqHandler->GetValue("upgradeUrl",&pVersion,&nLen);
	printf("1.1 Passed: %d , len[%d]\n",ret!=0,nLen);
	
	void  *pAreaid=NULL;
	int nAreaLen=0;

	int nAreaValue=0;
	m_pReqHandler->GetValue("areaId",&nAreaValue);
	printf("2 Passed: %d, get areaId[%d] \n",nAreaValue==5,nAreaValue);
	
	m_pReqHandler->GetValue("areaId[1]",&nAreaValue);
	printf("2 Passed: %d, get areaId[%d] \n",nAreaValue==6,nAreaValue);
	
	m_pReqHandler->GetValue("areaId[1]",&nAreaValue);
	printf("2 Passed: %d, get areaId[%d] \n",nAreaValue==6,nAreaValue);
	
	
	void  *pPresentList=NULL;
	int nPresentListLen=0;
	m_pReqHandler->GetValue("presentList['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3 Passed: %d, get presentList[%s]\n",string((char *)pPresentList)=="ert",(char *)pPresentList);
	
	m_pReqHandler->GetValue("presentList[0]['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3.1 Passed: %d, get presentList[%s]\n",string((char *)pPresentList)=="ert",(char *)pPresentList);

	m_pReqHandler->GetValue("presentList[1]['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3.2 Passed: %d, get presentList[%s]\n",string((char *)pPresentList)=="wer",(char *)pPresentList);

	m_pReqHandler->GetValue("presentList[0]['state']",&nAreaValue);
	printf("2 Passed: %d, get presentList[0]['state'][%d] \n",nAreaValue==10,nAreaValue);
	
	m_pReqHandler->GetValue("presentList[1]['state']",&nAreaValue);
	printf("2 Passed: %d, get presentList[1]['state'][%d] \n",nAreaValue==12,nAreaValue);
	
/*
	start test getValues
*/	
	char szNull[1]={0};
	vector<SStructValue> vecValue;
	EValueType eValueType;
	m_pReqHandler->GetValues("version",vecValue,eValueType);
	printf("\t\tGetvalues version size: %d, first[%s],type[%d]\n",vecValue.size(),vecValue.size()==0?szNull:(char *)(vecValue[0].pValue),eValueType);
	if(vecValue.size()>0) 
	{
		string strValue=string((char *)(vecValue[0].pValue),vecValue[0].nLen);
		printf("11 passed: %d, version value[%s]\n", strValue=="abc",strValue.c_str());
	}
	
	vecValue.clear();
	m_pReqHandler->GetValues("areaId",vecValue,eValueType);
	printf("\t\tGetvalues areaId size: %d, first[%d],type[%d]\n",vecValue.size(),vecValue.size()==0?0:*((int *)(vecValue[0].pValue)),eValueType);
	if(vecValue.size()>0) 
	{
		printf("12 passed: %d, areaId value[%d]\n", *((int *)(vecValue[0].pValue))==5,*((int *)(vecValue[0].pValue)));
	}
	if(vecValue.size()>1) 
	{
		printf("12.1 passed: %d, areaId value[%d]\n", *((int *)(vecValue[1].pValue))==6,*((int *)(vecValue[1].pValue)));
	}
	
	vecValue.clear();
	m_pReqHandler->GetValues("areaId[1]",vecValue,eValueType);
	printf("\t\tGetvalues areaId size: %d, first[%d],type[%d]\n",vecValue.size(),vecValue.size()==0?0:*((int *)(vecValue[0].pValue)),eValueType);
	if(vecValue.size()>0) printf("12.2 passed: %d, areaId value[%d]\n", *((int *)(vecValue[0].pValue))==6,*((int *)(vecValue[0].pValue)));

	vecValue.clear();
	ret=m_pReqHandler->GetValues("upgradeUrl",vecValue,eValueType);
	printf("12.3 passed: %d \n",ret!=0);
	
	
	vecValue.clear();
	m_pReqHandler->GetValues("presentList['FromOpenId']",vecValue,eValueType);
	if(vecValue.size()>1) 
	{
		string strValue=string((char *)(vecValue[1].pValue),vecValue[1].nLen);
		printf("13.1 passed: %d, presentList.FromOpenId value[%s]\n", !strcmp(strValue.c_str(),"wer"),strValue.c_str());
	}
	
	vecValue.clear();
	m_pReqHandler->GetValues("presentList[0]['FromOpenId']",vecValue,eValueType);
	printf("\t\tGetvalues presentList size: %d, first[%s],type[%d]\n",vecValue.size(),vecValue.size()==0?szNull:(char *)(vecValue[0].pValue),eValueType);
	if(vecValue.size()>0)
	{
		string strValue=string((char *)(vecValue[0].pValue),vecValue[0].nLen);
		printf("13.2 passed: %d, presentList.FromOpenId value[%s]\n", !strcmp(strValue.c_str(),"ert"),strValue.c_str());
	}
	
	vecValue.clear();
	m_pReqHandler->GetValues("presentList",vecValue,eValueType);
	printf("\t\tGetvalues presentList size: %d, type[%d]\n",vecValue.size(),eValueType);
	printf("14 passed: %d,size[%d]\n", vecValue.size()==2,vecValue.size());

	delete m_pReqHandler;
/*
*
* test encoder & decoder
*
*/
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","dfg");
	m_pEncoderHandler->SetValue("version","345");
	m_pEncoderHandler->SetValue("presentList[0]['state']",2);
	m_pEncoderHandler->SetValue("presentList[1]['state']",3);
	m_pEncoderHandler->SetValue("presentList[1]['FromOpenId']","456");
	m_pEncoderHandler->SetValue("presentList[1]['name']","789");
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->SetValue("presentList[0]['name']","dfg");

	STestStruct s3;
	memset(&s3,0,sizeof(s3));
	memcpy(s3.a,"ert",3);
	s3.n=10;
	m_pEncoderHandler->SetValue("presentList",&s3,sizeof(s1));
	int nEncodeCode=m_pEncoderHandler->Encode();
	printf("0 Passed: %d, validator code[%d]\n",nEncodeCode==-2002,nEncodeCode);
	//dump_buffer_notice(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());

	int nDecodeCode=m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	

	m_pDecoderHandler->GetValue("version",&pVersion,&nLen);
	printf("1 Passed: %d, get version[%s], nlen[%d]\n",string((char *)pVersion)=="345",(char *)pVersion,nLen);

	m_pDecoderHandler->GetValue("areaId",&pAreaid,&nAreaLen);
	nAreaValue=*((int *)pAreaid);
	printf("2 Passed: %d, get areaId[%d]\n",nAreaValue==2,nAreaValue);
	
	m_pDecoderHandler->GetValue("areaId[1]",&pAreaid,&nAreaLen);
	nAreaValue=*((int *)pAreaid);
	printf("2.1 Passed: %d, get areaId[%d]\n",nAreaValue==3,nAreaValue);
	
	m_pDecoderHandler->GetValue("areaId[0]",&pAreaid,&nAreaLen);
	nAreaValue=*((int *)pAreaid);
	printf("2.2 Passed: %d, get areaId[%d]\n",nAreaValue==2,nAreaValue);


	ret=m_pDecoderHandler->GetValue("presentList['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3 Passed: %d, get presentList[%s], ret[%d]\n",string((char *)pPresentList)=="dfg",(char *)pPresentList, ret);
	m_pDecoderHandler->GetValue("presentList[0]['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3.1 Passed: %d, get presentList[%s]\n",string((char *)pPresentList)=="dfg",(char *)pPresentList);
	m_pDecoderHandler->GetValue("presentList[1]['FromOpenId']",&pPresentList,&nPresentListLen);
	printf("3.2 Passed: %d, get presentList[%s]\n",string((char *)pPresentList)=="456",(char *)pPresentList);

	void  *pState;
	int nStateLen=0;
	m_pDecoderHandler->GetValue("presentList['state']",&pState,&nStateLen);
	int nState=*((int *)pState);
	printf("4.1 Passed: %d, get state[%d]\n",nState==2,nState);

	m_pDecoderHandler->GetValue("presentList[1]['state']",&pState,&nStateLen);
	nState=*((int *)pState);
	printf("4.2 Passed: %d, get state[%d]\n",nState==3,nState);

	m_pDecoderHandler->GetValue("presentList[2]['state']",&nState);
	printf("4.3 Passed: %d, get presentList[2]['state'][%d] \n",nState==10,nState);

	void *pName=NULL;
	int nNameLen=0;
	m_pDecoderHandler->GetValue("presentList[0]['name']",&pName,&nNameLen);
	printf("5.1 Passed: %d, get presentList name[%s]\n",string((char *)pName)=="dfg",(char *)pName);
	m_pDecoderHandler->GetValue("presentList[1]['name']",&pName,&nNameLen);
	printf("5.2 Passed: %d, get presentList name[%s]\n",string((char *)pName)=="789",(char *)pName);


	void *pStruct=NULL;
	int nStructLen=0;
	m_pDecoderHandler->GetValue("presentList[0]",&pStruct,&nStructLen);
	m_pEncoderHandler->SetValue("presentList[0]",pStruct,nStructLen);


	vector<SStructValue> vecValues;
	m_pDecoderHandler->GetValues("presentList",vecValues);
	m_pEncoderHandler->SetValues("presentList",vecValues);	
	//dump_buffer_notice(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());

	void *pIntValue=NULL;
	int nIntValue=0;
	int nIntLen=0;
	
	void *pStrValue=NULL;
	string strValue;
	int nStrLen=0;
	m_pDecoderHandler->GetValue("serverId",&pIntValue,&nIntLen);
	printf("default server addr[%x]\n",pIntValue);
	nIntValue=*((int *)pIntValue);
	printf("default 1 Passed: %d, get serverId[%d]\n",nIntValue==4,nIntValue);

	m_pDecoderHandler->GetValue("toOpenId",&pStrValue,&nStrLen);
	printf("default 2 Passed: %d, get serverId[%s]\n",string((char *)pStrValue)=="123",(char *)pStrValue);
	
	
	delete m_pEncoderHandler;
	delete m_pDecoderHandler;

	//testPerformance();

	TestValidateEncode();
	TestPhp();
	TestStarSetting();
	return 0;
}

