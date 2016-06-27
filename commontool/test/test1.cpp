#ifndef __TEST_1_CPP_
#define __TEST_1_CPP_
#include <stdio.h>
#include "AvenueServiceConfigs.h"
#include "SdkLogHelper.h"
#include <arpa/inet.h>

#include "AvenueMsgHandler.h"
#include <time.h>
#include <sys/time.h>
#include "testcommon.cpp"
#include "CodeConvert.h"

void  TestValidate1()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",false,&oServiceConfig);
	
	m_pEncoderHandler->SetValue("areaId",9);
	m_pEncoderHandler->SetValue("areaId",12);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","dfg");
	m_pEncoderHandler->SetValue("version","345");
	m_pEncoderHandler->SetValue("presentList[0]['state']",2);
	m_pEncoderHandler->SetValue("presentList[1]['state']",3);
	m_pEncoderHandler->SetValue("presentList[1]['FromOpenId']","456");
	m_pEncoderHandler->SetValue("presentList[1]['name']","789");
	m_pEncoderHandler->SetValue("areaId",7);
	m_pEncoderHandler->SetValue("areaId",14);
	m_pEncoderHandler->SetValue("presentList[0]['name']","dfg");
	int nEncodeCode=m_pEncoderHandler->Encode();
	printf("TestValidate1  passed: %d, code[%d]\n",nEncodeCode==-2001,nEncodeCode);

	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("needupgrade",12);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","dfg");
	m_pEncoderHandler->SetValue("presentList[0]['state']",htonl(2));
	nEncodeCode=m_pEncoderHandler->Encode();
	printf("TestValidate1  passed: %d, code[%d]\n",nEncodeCode==-2002,nEncodeCode);

	delete m_pEncoderHandler;
}
void  TestValidate2()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",false,&oServiceConfig);
	CAvenueMsgHandler * m_pDecoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",false,&oServiceConfig);
	
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","dfg");
	m_pEncoderHandler->SetValue("version","345");
	m_pEncoderHandler->SetValue("presentList[0]['state']",26);
	m_pEncoderHandler->SetValue("presentList[1]['state']",37);
	m_pEncoderHandler->SetValue("presentList[1]['FromOpenId']","456");
	m_pEncoderHandler->SetValue("presentList[1]['name']","789");
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->SetValue("presentList[0]['name']","dfg");
	m_pEncoderHandler->Encode();
	
	int nRetrunMsgCode=0;
	string strReturnMsg;
	int nDecodeCode=m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen(),nRetrunMsgCode,strReturnMsg);
	printf("TestValidate2  passed: %d, code[%d]\n",nDecodeCode==-2001,nDecodeCode);
	printf("TestValidate2  passed: %d, nRetrunMsgCode[%d],msg[%s]\n",nRetrunMsgCode==95,nRetrunMsgCode,strReturnMsg.c_str());

	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("needupgrade",12);
	m_pEncoderHandler->SetValue("presentList[0]['FromOpenId']","dfg");
	m_pEncoderHandler->SetValue("presentList[0]['state']",2);
	m_pEncoderHandler->Encode();
	nDecodeCode=m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen(),nRetrunMsgCode,strReturnMsg);
	printf("TestValidate2  passed: %d, code[%d]\n",nDecodeCode==-2002,nDecodeCode);
	printf("TestValidate2  passed: %d, nRetrunMsgCode[%d],msg[%s]\n",nRetrunMsgCode==95,nRetrunMsgCode,strReturnMsg.c_str());
	
	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("needupgrade",12);
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->SetValue("areaId",23);
	m_pEncoderHandler->Encode();
	nDecodeCode=m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen(),nRetrunMsgCode,strReturnMsg);
	printf("TestValidate2  passed: %d, code[%d]\n",nDecodeCode==-2003,nDecodeCode);
	printf("TestValidate2  passed: %d, nRetrunMsgCode[%d],msg[%s]\n",nRetrunMsgCode==95,nRetrunMsgCode,strReturnMsg.c_str());

	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("needupgrade",12);
	m_pEncoderHandler->SetValue("areaId",2);
	m_pEncoderHandler->SetValue("areaId",3);
	m_pEncoderHandler->Encode();
	nDecodeCode=m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());
	printf("TestValidate2  passed: %d, code[%d]\n",nDecodeCode==0,nDecodeCode);


	delete m_pEncoderHandler;
	delete m_pDecoderHandler;
	
}

void  TestEncode1()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.checkVersion",true,&oServiceConfig);
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
	m_pEncoderHandler->SetValue("utf8test","abcＡＢＣ");
	
	string utf8str = "abcＡＢＣ";
	Utf82Ascii UToG;
	char szGbk[20] = {0};
	UToG.Convert(const_cast<char*>(utf8str.c_str()), utf8str.size(), szGbk, sizeof(szGbk));
	m_pEncoderHandler->SetValue("gbktest",szGbk);
	
	m_pEncoderHandler->Encode();
	m_pDecoderHandler->Decode(m_pEncoderHandler->GetBuffer(),m_pEncoderHandler->GetLen());

	void *pStrValue=NULL;
	string strValue;
	int nStrLen=0;
	m_pDecoderHandler->GetValue("presentList[0]['FromOpenId']",&pStrValue,&nStrLen);
	printf("TestEncode1.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("123"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestEncode2.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestEncode2.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("presentList[0]['name']",&pStrValue,&nStrLen);
	printf("TestEncode2.3 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("563"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestEncode3.1 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);
	m_pDecoderHandler->GetValue("upgradeUrl",&pStrValue,&nStrLen);
	printf("TestEncode3.2 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("345645"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("toOpenId",&pStrValue,&nStrLen);
	printf("TestEncode4 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("abcd45"),(char *)pStrValue);

	m_pDecoderHandler->GetValue("version",&pStrValue,&nStrLen);
	printf("TestEncode5 Passed: %d, get presentList[%s]\n",string((char *)pStrValue)==string("123"),(char *)pStrValue);

	vector<SStructValue> vecValues;
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str1=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str2=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestEncode6.1 Passed: %d, get presentList[%s]\n",!(strcmp(str1.c_str(),"123")),str1.c_str());
	printf("TestEncode6.2 Passed: %d, get presentList[%s]\n",!(strcmp(str2.c_str(),"456")),str2.c_str());

	vecValues.clear();
	m_pDecoderHandler->GetValues("presentList['FromOpenId']",vecValues);
	string str3=string((char *)(vecValues[0].pValue),vecValues[0].nLen);
	string str4=string((char *)(vecValues[1].pValue),vecValues[1].nLen);
	printf("TestEncode6.3 Passed: %d, get presentList[%s]\n",!(strcmp(str3.c_str(),"123")),str3.c_str());
	printf("TestEncode6.4 Passed: %d, get presentList[%s]\n",!(strcmp(str4.c_str(),"456")),str4.c_str());

	m_pDecoderHandler->GetValue("utf8test",&pStrValue,&nStrLen);
	printf("TestEncode7.1 Passed: %d, get utf8test[%s]\n",string((char *)pStrValue)==string("abcABC"),(char *)pStrValue);
	m_pDecoderHandler->GetValue("gbktest",&pStrValue,&nStrLen);
	char szUtf8[20] = {0};
	Ascii2Utf8 GToU;
	GToU.Convert((char *)pStrValue,nStrLen,szUtf8,sizeof(szUtf8));
	printf("TestEncode7.2 Passed: %d, get gbktest[%s]\n",string(szUtf8)==string("abcABC"),szUtf8);
	
	
	delete m_pEncoderHandler;
	delete m_pDecoderHandler;
}
void  TestValidate4()
{
	CAvenueServiceConfigs oServiceConfig;
	if(oServiceConfig.LoadAvenueServiceConfigs(GetCurrentPath()+"avenue_conf") != 0)
	{
		return ;
	}
	CAvenueMsgHandler * m_pEncoderHandler = new CAvenueMsgHandler("serviceForTX.queryPresent",true,&oServiceConfig);
	
	int nEncodeCode=m_pEncoderHandler->Encode();
	printf("TestValidate4  passed: %d, code[%d]\n",nEncodeCode==-10242400,nEncodeCode);

	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("version3","zongjinliang@qq.com");
	nEncodeCode=m_pEncoderHandler->Encode();
	printf("TestValidate4  passed: %d, code[%d]\n",nEncodeCode==0,nEncodeCode);

	m_pEncoderHandler->Reset();
	m_pEncoderHandler->SetValue("version2","zongjinliang");
	m_pEncoderHandler->SetValue("version3","zongjinliang@qq.com");
	nEncodeCode=m_pEncoderHandler->Encode();
	printf("TestValidate4  passed: %d, code[%d]\n",nEncodeCode==-10242400,nEncodeCode);

	delete m_pEncoderHandler;
}
void TestValidateEncode()
{
	TestValidate1();
	TestValidate2();
	TestEncode1();
	TestValidate4();
}
#endif
