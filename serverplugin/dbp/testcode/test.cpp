#include "stdio.h"
#include "pthread.h"

#ifndef WIN32
#include <dlfcn.h>
#else
#include "windows.h"
#endif

#include "IAsyncVirtualService.h"
#include "AsyncVirtualServiceLog.h"
#include "AvenueSmoothMessage.h"
#include "SapTLVBody.h"
typedef IAsyncVirtualService* (*PCREATE)();
typedef void (*PDESTROY)(IAsyncVirtualService*);
typedef IAsyncVirtualService* (*PRETEST)();
using namespace sdo::commonsdk;
int nSum = 0;
int nSlam  = 10;
int nTotal = 1;
int nIndex =1;
#include "vector"
using namespace std;

void SplitString(vector<string>&vecValues,const string& str)
{
	
	
	const char*ptr=(const char*)str.c_str();
	int left = 0;
	int right = 0;
	int index = 0;
	int beg = 0;
	while(ptr[index])
	{
		left = 0;
		right= 0;
		
		while (ptr[index] && ptr[index]==' ')  
		{
			index++;
		}
		if (index == str.size())
			break;
		
		if (ptr[index]=='(')	left=1;	
		if (ptr[index]==')')	right=1;
		
		beg = index;

		index++;
		int length = 1;
		while(true)
		{
			while(ptr[index] && ptr[index]!=' ')
			{
				if (ptr[index]=='(')	left++;	
				if (ptr[index]==')')	right++;
				
				index++;
				length++;
			}
			if (left == right)
			{
				vecValues.push_back(str.substr(beg,length));
				break;
			}
			if (ptr[index]==0)
				break;
			index++;
			length++;
		}

	}
		
	
}


int ContainLastComma(const string &str)
{
	for(int i=str.size()-1;i>=0;i--)
	{
		if (str.at(i) ==' ')
		{}
		else if (str.at(i) ==',')
		{
			return i;
		}
		else 
			return -1;
	}
	return -1;
}

void PrintHex(const void* Packet, int nLen)
{
	static char szTable[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	unsigned char *pChar = (unsigned char*)(Packet);
	
	char szBuf[2048]={0};
	int index=0;
	for(int i=0;i<nLen; i++)
	{
		szBuf[index++]=szTable[((pChar[i] &0xF0)>>4)];
		szBuf[index++]=szTable[((pChar[i] &0x0F))]	;
		szBuf[index++]=' ';	
		
		if ((i+1)%8==0)
			szBuf[index++]='\n';
	}
	
	szBuf[index++]='\n';
	
	printf("%s",szBuf);
	
}
void TResponseService(void*index, const void *ptr, int len)
{
	SV_XLOG(XLOG_DEBUG, "CALL %s %d\n", __FUNCTION__,(int)(index));
	printf("CALL %s %d\n", __FUNCTION__,(int)(index));

	PrintHex(ptr,len);
	
	CAvenueSmoothDecoder dec(ptr,len);
	dec.DecodeBodyAsTLV();

	unsigned int rowCount=0;
	unsigned int errorno=0;
	string errormsg;
	string spname;
	
	string ho;
	if (dec.GetValue(108,ho)==0)
	{
		printf("***************ho=%s\n",ho.c_str());
	}
	
	dec.GetValue(100,&rowCount);

	dec.GetValue(101,spname);
	
	printf("***************spname=%s\n",spname.c_str());
	
	SV_XLOG(XLOG_DEBUG, "~~~~~~~~~~~~~~~~~\nServiceId:%d Msgid:%d Code:%d BodyLen:%d \nRowCount:%d  \n", 
		dec.GetServiceId(),
		dec.GetMsgId(),
		dec.GetCode(),
		len,
	    rowCount);
		
	
	vector<sdo::commonsdk::SAvenueValueNode> sapNodes;
	dec.GetValues(103,sapNodes);
	{
		for(int i=0;i<sapNodes.size();i++)
		{
			sdo::sap::CSapTLVBodyDecoder cds(sapNodes[i].pLoc,
			sapNodes[i].nLen);
			
			string str101,str102;
			unsigned int dw104;
			unsigned int dw105;
			
			cds.GetValue(101,str101);
			cds.GetValue(102,str102);
			
			if (0!=cds.GetValue(104,&dw104))
			{
				dw104 = 999;
			}
			
			if (0!=cds.GetValue(105,&dw105))
			{
				dw105 = 999;
			}
			
			SV_XLOG(XLOG_DEBUG, "~~~~~~~~~~~~~~~~~ %d str101[%s] str102[%s] dw104[%d] dw105[%d]\n",
			i,
			str101.c_str(),str102.c_str(),
			dw104,dw105);
		}
	}
	
}

void TExceptionWarn(const string &str)
{
	SV_XLOG(XLOG_DEBUG, "CALL %s %s\n", __FUNCTION__,str.c_str());
}

void TAsyncLog(int nModel,XLOG_LEVEL level,int nInterval,const string &strMsg,int nCode,const string& strAddr,int serviceId, int msgId)
{

	if (nCode<0)
	{
		printf("nIndex = %d\n",nIndex);
	}
	nSum++;
//	if (nTotal-1 == nSum || 1==nSum)
	{
		AVS_XLOG( XLOG_INFO, "ReqTime: %s nInterval: %d nCode: %d ServerAddr: %s\n",strMsg.c_str(),nInterval,nCode,strAddr.c_str());
	}
	
	
}


//---------------------------------------------------------------------
//---------------- TEST CASE ------------------------------------------

void TestResponeDecodeCase1(CAvenueSmoothDecoder& de)
{
	//
}


void TestQuerySndaId(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 1;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(2,"ACC1");
	en.SetValue(3,"1");

	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}


void TestDeleteAccount(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 2;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(2,"ACC1");
	en.SetValue(3,"1");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}

void TestDeleteSndaId(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 4;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(1,"100");
	en.SetValue(2,"ACC1");
	en.SetValue(3,"1");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}

void TestInsertAccount(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 5;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(1,"101");
	en.SetValue(2,"ACC2");
	en.SetValue(3,"1");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}

void TestInsertSndaId(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 6;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(1,"101");
	en.SetValue(2,"ACC2");
	en.SetValue(3,"1");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}

void TestIGetNewSndaId(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 20015;
	unsigned int nMsgId = 7;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,20015,nMsgId);
	en.SetValue(4,"SNDAID");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());

}

void TestNULLParameter1(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 3;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	//en.SetValue(101,"111");
	en.SetValue(102,"222");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}


void TestNULLParameter4(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 4;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	//en.SetValue(101,"111");
	//en.SetValue(102,"222");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}

void TestNULLParameter5(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 5;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	en.SetValue(101,"²âÊÔÒ»ÏÂ");

	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}

void TestNULLParameter6(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 6;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	en.SetValue(101,"123456789012345678901234567890");
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}


void TestNULLParameter7(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 7;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	en.SetValue(104,1000);
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}



void TestNULLParameter8(IAsyncVirtualService* pService)
{
	unsigned int nIdentifer = 10001;
	unsigned int nMsgId = 8;
	CAvenueSmoothEncoder en(SAP_PACKET_REQUEST,50011,nMsgId);
	
	pService->RequestService((void*)nIdentifer,en.GetBuffer(),en.GetLen());
}


IAsyncVirtualService* pService=NULL;
void* test_func(void*)
{
	while(1)
	{
		TestIGetNewSndaId(pService);
	}
	return (void*)0;
}



int main()
{
	printf("Begin test\n");
	/*
	string strCore ="selga l";
	
	int dwLastComma = ContainLastComma(strCore);
		
	if (dwLastComma!=-1)
			strCore=strCore.substr(0,dwLastComma);
			
	printf("%d %s\n", dwLastComma,strCore.c_str());
	return 1; 
	string str="update tbcodegroup set  group_name='zyp',  showbegintime=to_date('2099-1-1 00:00:00','yyyy-mm-dd hh24:mi:s'), showendtime=to_date('2099-1-1 00:00:00','yyyy-mm-dd hh24:mi:s'), begintime=to_date('2099-1-1 00:00:00','yyyy-mm-dd hh24:mi:s'), endtime=to_date('2099-1-1 00:00:00','yyyy-mm-dd hh24:mi:s') where area_id=(select b.area_id from tbcodeservice a inner join tbcodearea b on a.service_id=b.service_id  where a.service_code=2 and b.area_code=36) and group_code=3";
	vector<string> vec;
	SplitString(vec,str);
	return 1;
	*/
	XLOG_INIT("./log.properties",true);

#ifdef WIN32
	HMODULE module = LoadLibrary("E:/Proj/DBP2/Debug/DBP2.dll");
	printf("module: %x\n",module);

	PCREATE pcr = (PCREATE)::GetProcAddress(module,"create");
	PDESTROY pcr_destroy = (PDESTROY)::GetProcAddress(module,"destroy");

	IAsyncVirtualService* pService = pcr();
	pService->Initialize(TResponseService,TExceptionWarn,TAsyncLog);

	Sleep(2*1000);
	
	printf("test insert\n");
	TestInsert(pService);

	Sleep(25*1000);
	pcr_destroy(pService);

	printf("End test %x\n",pService);
	Sleep(2*1000);
#else

//	void *module  =dlopen("./plugin/dbplugin/libdbp.so",RTLD_NOW);
	void *module  =dlopen("./async_virtual_service/libdbp.so",RTLD_NOW);
	printf("module: %x\n",module);
	void* ptr = dlsym(module,"create");
	void* ptr_destroy = dlsym(module,"destroy");
	void* ptr_ReTest = dlsym(module,"retest");
	printf("function: %x %x\n",ptr,ptr_ReTest);
	PCREATE pcr = (PCREATE)(ptr);
	PDESTROY pcr_destroy = (PDESTROY)(ptr_destroy);
	PRETEST pcr_Retest = (PRETEST)(ptr_ReTest);
	if (pcr_Retest == NULL)
	{
		printf("NULL retest\n");
		return 1;
	}
	
	pService = pcr();
	pService->Initialize(TResponseService,TExceptionWarn,TAsyncLog);

	sleep(5);

	//	TestDeleteSndaId(pService);
		
//	TestDeleteAccount(pService);
	
//TestInsertAccount(pService);
	
//TestInsertSndaId(pService);
	
//TestQuerySndaId(pService);	

//	TestIGetNewSndaId(pService);
	
// pthread_t ntid; 
 //for(int i=0;i<2;i++)
	//	pthread_create(&ntid,NULL,test_func,NULL);
	
//	TestNULLParameter1(pService);
//	TestNULLParameter4(pService);
	
//	TestNULLParameter5(pService);
	
//	TestNULLParameter6(pService);
//	TestNULLParameter7(pService);
	TestNULLParameter8(pService);
	
	
	sleep(25);
	pcr_destroy(pService);
	printf("End test %x\n",pService);
	sleep(2);
#endif


	return 0;
}





