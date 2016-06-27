#ifndef _TEST_2_AVENUE_AVENUE_PHP_
#define _TEST_2_AVENUE_AVENUE_PHP_
#include <stdio.h>
#include "AvenueServiceConfigs.h"
#include "SdkLogHelper.h"
#include <arpa/inet.h>

#include "AvenueMsgHandler.h"
#include <time.h>
#include <sys/time.h>
#include "PhpSerializer.h"
#include "testcommon.cpp"

using namespace sdo::commonsdk;
void TestPhpToAvenue()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",false,&oServiceConfig);
	CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);

	CPhpSerializer phpEncoder;
	phpEncoder.SetValue("areaid[0]",9);
	phpEncoder.SetValue("areaid[1]",12);
	phpEncoder.SetValue("presentlist[0]['fromopenid']","dfg");
	phpEncoder.SetValue("version","345");
	phpEncoder.SetValue("presentlist[0]['state']",2);
	phpEncoder.SetValue("presentlist[1]['state']",3);
	phpEncoder.SetValue("presentlist[1]['fromopenid']","456");
	phpEncoder.SetValue("presentlist[1]['name']","789");
	phpEncoder.SetValue("areaid[2]",7);
	phpEncoder.SetValue("areaid[3]",14);
	phpEncoder.SetValue("presentlist[0]['name']","dfg");
	phpEncoder.SetValue("upgradeurl","abcd45");

	phpEncoder.Dump();
	m_pEncoderHandler->SetPhpSerializer(&phpEncoder);

	int nEncodeCode=m_pEncoderHandler->Encode();

	m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	void *pStrValue=NULL;
	string strValue;
	int nStrLen=0;
	m_pDecoderHandler->GetValue("presentList[1]['FromOpenId']",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("456"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("dfg"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("dfg"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.3 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("dfg"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);
	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestPhpToAvenue.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);


	vector<SStructValue> vecValues;
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str1=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str2=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestPhpToAvenue.1 Passed: %d, get presentList[%s]\n",!(strcmp(str1.c_str(),"dfg")),str1.c_str());
	printf("TestPhpToAvenue.2 Passed: %d, get presentList[%s]\n",!(strcmp(str2.c_str(),"456")),str2.c_str());

	vecValues.clear();
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str3=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str4=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestPhpToAvenue.3 Passed: %d, get presentList[%s]\n",!(strcmp(str3.c_str(),"dfg")),str3.c_str());
	printf("TestPhpToAvenue.4 Passed: %d, get presentList[%s]\n",!(strcmp(str4.c_str(),"456")),str4.c_str());

	delete m_pEncoderHandler;
	delete m_pDecoderHandler;
}

void TestPhpToAvenue2()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",false,&oServiceConfig);
	CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);

	CPhpSerializer phpEncoder,phpDecoder;
	phpEncoder.SetValue("areaid[0]",9);
	phpEncoder.SetValue("areaid[1]",12);
	phpEncoder.SetValue("presentlist[0]['fromopenid']","dfg");
	phpEncoder.SetValue("version","345");
	phpEncoder.SetValue("presentlist[0]['state']",2);
	phpEncoder.SetValue("presentlist[1]['state']",3);
	phpEncoder.SetValue("presentlist[1]['fromopenid']","456");
	phpEncoder.SetValue("presentlist[1]['name']","789");
	phpEncoder.SetValue("areaid[2]",7);
	phpEncoder.SetValue("areaid[3]",14);
	phpEncoder.SetValue("presentlist[0]['name']","dfg");
	phpEncoder.SetValue("upgradeurl","abcd45");

	phpEncoder.Dump();
	m_pEncoderHandler->SetPhpSerializer(&phpEncoder);

	int nEncodeCode=m_pEncoderHandler->Encode();
	m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	phpDecoder.SetTlvGroup(m_pDecoderHandler);
	
	int nValue=0;
	string strValue;
	phpDecoder.GetValue("areaid[0]",nValue);
	printf ("TestPhpToAvenue2::passed %d: areaid[0][%d]\n",nValue==9,nValue);

	phpDecoder.GetValue("areaid[2]",nValue);
	printf ("TestPhpToAvenue2::passed %d: areaid[2][%d]\n",nValue==7,nValue);

	phpDecoder.GetValue("presentlist[1]['name']",strValue);
	printf ("TestPhpToAvenue2::passed %d: presentlist[1]['name'][%s]\n",strValue==string("789"),strValue.c_str());

	phpDecoder.GetValue("presentlist[0]['name']",strValue);
	printf ("TestPhpToAvenue2::passed %d: presentlist[0]['name'][%s]\n",strValue==string("dfg"),strValue.c_str());
	delete m_pEncoderHandler;
	delete m_pDecoderHandler;
}
void TestAvenueToAvenue()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	CAvenueMsgHandler * m_pTmp = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	CAvenueMsgHandler * m_pEncoderHandler2 = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
	
	m_pEncoderHandler->SetValue("areaId",9);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","<123");
	m_pEncoderHandler->SetValue("presentList[0]['state']",2);
	m_pEncoderHandler->SetValue("presentList[1]['state']",3);
	m_pEncoderHandler->SetValue("presentList[1]['FromOpenId']","456");
	m_pEncoderHandler->SetValue("version","inSert123");
	m_pEncoderHandler->SetValue("upgradeUrl","abcd45");
	m_pEncoderHandler->SetValue("toOpenId","abcd45");
	m_pEncoderHandler->SetValue("presentList[0]['name']","cd3");
	m_pEncoderHandler->Encode();
	
	m_pTmp->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	m_pEncoderHandler2->SetTlvGroup(m_pTmp);
	m_pDecoderHandler->Decode(m_pEncoderHandler2->GetBuffer(),m_pEncoderHandler2->GetLen());

	void *pStrValue=NULL;
	string strValue;
	int nStrLen=0;
	m_pDecoderHandler->GetValue("presentList[0]['FromOpenId']",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("123"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.3 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);
	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("toOpenId",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("abcd45"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("version",&pStrValue,&nStrLen);
	printf("TestAvenueToAvenue Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("123"),(char *)pStrValue);

	vector<SStructValue> vecValues;
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str1=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str2=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestAvenueToAvenue.1 Passed: %d, get presentList[%s]\n",!(strcmp(str1.c_str(),"123")),str1.c_str());
	printf("TestAvenueToAvenue.2 Passed: %d, get presentList[%s]\n",!(strcmp(str2.c_str(),"456")),str2.c_str());

	vecValues.clear();
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str3=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str4=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestAvenueToAvenue.3 Passed: %d, get presentList[%s]\n",!(strcmp(str3.c_str(),"123")),str3.c_str());
	printf("TestAvenueToAvenue.4 Passed: %d, get presentList[%s]\n",!(strcmp(str4.c_str(),"456")),str4.c_str());

	delete m_pEncoderHandler;
	delete m_pTmp;
	delete m_pEncoderHandler2;
	delete m_pDecoderHandler;
}
void TestStarSetting()
{
	TestPhpToAvenue();
	TestPhpToAvenue2();
	TestAvenueToAvenue();
}
#endif

