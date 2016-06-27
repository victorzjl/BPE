#ifndef _TEST_PHP_CPP_
#define _TEST_PHP_CPP_
#include "PhpSerializer.h"
#include <time.h>
#include <sys/time.h>

using namespace sdo::commonsdk;
void TestPhp1()
{
	int nValue=0;
	string strValue;
	CPhpSerializer phpEncoder,phpDecoder;
	phpEncoder.SetValue("name","zong");
	phpEncoder.SetValue("age",56);
	phpEncoder.GetValue("age",nValue);
	printf ("TestPhp1::passed %d: age[%d]\n",nValue==56,nValue);

	const string & strSerializer=phpEncoder.ToString();

	phpDecoder.FromString(strSerializer);
	
	phpDecoder.GetValue("age", nValue);
	phpDecoder.GetValue("name", strValue);
	printf ("TestPhp1::passed %d: name[%s]\n",strValue==string("zong"),strValue.c_str());
	
	const string & strSerializer2=phpDecoder.ToString();
	printf ("TestPhp1::passed %d: age[%d],name[%s], serializer[%s]\n",strSerializer2==strSerializer,nValue,strValue.c_str(),strSerializer2.c_str());
}
void TestPhp2()
{
	int nValue=0;
	string strValue;
	CPhpSerializer phpEncoder,phpDecoder;
	phpEncoder.SetValue("name","zong");
	phpEncoder.SetValue("age",56);
	phpEncoder.SetValue("house['name']","my house");
	phpEncoder.SetValue("house['age']",25);
	phpEncoder.SetValue("house['aa']","bb");
	phpEncoder.SetValue("house['cc']","dd");
	phpEncoder.SetValue("house['num']",1);
	phpEncoder.SetValue("user['_login_info']['is_login']",1);
	phpEncoder.SetValue("user['_login_info']['mid']",45435);

	phpEncoder.GetValue("house['age']", nValue);
	phpEncoder.GetValue("house['name']", strValue);
	printf ("TestPhp2::passed %d: house['age'][%d]\n",nValue==25,nValue);
	printf ("TestPhp2::passed %d: house['name'][%s]\n",strValue==string("my house"),strValue.c_str());

	const string & strSerializer=phpEncoder.ToString();

	phpDecoder.FromString(strSerializer);
	phpDecoder.GetValue("house['age']", nValue);
	phpDecoder.GetValue("house['name']", strValue);
	printf ("TestPhp2::passed %d: house['age'][%d]\n",nValue==25,nValue);
	printf ("TestPhp2::passed %d: house['name'][%s]\n",strValue==string("my house"),strValue.c_str());
	phpDecoder.GetValue("user['_login_info']['is_login']", nValue);
	printf ("TestPhp2::passed %d: user['_login_info']['is_login'][%d]\n",nValue==1,nValue);
	
	const string & strSerializer2=phpDecoder.ToString();
	printf ("TestPhp2::passed %d: serializer[%s]\n",strSerializer2==strSerializer,strSerializer2.c_str());
}
void TestPhp3()
{
	int nValue=0;
	string strValue;
	CPhpSerializer phpEncoder,phpDecoder;
	phpEncoder.SetValue("name","zong");
	phpEncoder.SetValue("age",56);
	phpEncoder.SetValue("house[0]['name']","my house");
	phpEncoder.SetValue("house[0]['age']",25);
	phpEncoder.SetValue("house[0]['aa']","bb");
	phpEncoder.SetValue("house[0]['cc']","dd");
	phpEncoder.SetValue("house[0]['num']",1);

	phpEncoder.SetValue("house[1]['name']","2my house");
	phpEncoder.SetValue("house[1]['age']",225);
	phpEncoder.SetValue("house[1]['aa']","2bb");
	phpEncoder.SetValue("house[1]['cc']","2dd");
	phpEncoder.SetValue("house[1]['num']",21);

	phpEncoder.Dump();

	phpEncoder.GetValue("house[0]['age']", nValue);
	phpEncoder.GetValue("house[1]['name']", strValue);
	printf ("TestPhp3::passed %d: house['age'][%d]\n",nValue==25,nValue);
	printf ("TestPhp3::passed %d: house['name'][%s]\n",strValue==string("2my house"),strValue.c_str());

	const string & strSerializer=phpEncoder.ToString();

	phpDecoder.FromString(strSerializer);
	phpDecoder.GetValue("house[0]['age']", nValue);
	phpDecoder.GetValue("house[1]['name']", strValue);
	printf ("TestPhp3::passed %d: house['age'][%d]\n",nValue==25,nValue);
	printf ("TestPhp3::passed %d: house['name'][%s]\n",strValue==string("2my house"),strValue.c_str());
	
	const string & strSerializer2=phpDecoder.ToString();
	printf ("TestPhp3::passed %d: serializer[%s]\n",strSerializer2==strSerializer,strSerializer2.c_str());
}
void TestPhp4()
{
	int nValue=0;
	string strValue;
	void *pBuffer=NULL;
	unsigned int nLen=0;
	CPhpSerializer phpEncoder,phpDecoder;
	phpEncoder.SetValue("name","zong");
	phpEncoder.SetValue("age",56);
	phpEncoder.SetValue("house[0]['name']['n1']","my house1");
	phpEncoder.SetValue("house[0]['name']['n2']","my house2");
	phpEncoder.SetValue("house[0]['age']",25);
	phpEncoder.SetValue("house[0]['aa']","bb");
	phpEncoder.SetValue("house[0]['cc']","dd");
	phpEncoder.SetValue("house[0]['num']",1);

	phpEncoder.SetValue("house[1]['name']","2my house");
	phpEncoder.SetValue("house[1]['age']",225);
	phpEncoder.SetValue("house[1]['aa']","2bb");
	phpEncoder.SetValue("house[1]['cc']","2dd");
	phpEncoder.SetValue("house[1]['num']",21);
	

	phpEncoder.Dump();

	phpEncoder.GetValue("house[0]['age']", nValue);
	phpEncoder.GetValue("house[0]['age']", &pBuffer,&nLen);
	phpEncoder.GetValue("house[0]['name']['n2']", strValue);
	printf ("TestPhp4::passed %d: house['age'][%d]\n",nValue==25,nValue);
	printf ("TestPhp4::passed %d: house['name'][%s]\n",strValue==string("my house2"),strValue.c_str());

	phpEncoder.SetValue("house[0]['age']",35);

	const string & strSerializer=phpEncoder.ToString();

	phpDecoder.FromString(strSerializer);
	phpDecoder.GetValue("house[0]['age']", nValue);
	phpDecoder.GetValue("house[1]['name']", strValue);
	printf ("TestPhp4::passed %d: house['age'][%d]\n",nValue==35,nValue);
	printf ("TestPhp4::passed %d: house['age'][%d]\n",*(int *)pBuffer==25,*(int *)pBuffer);
	printf ("TestPhp4::passed %d: house['name'][%s]\n",strValue==string("2my house"),strValue.c_str());
	
	const string & strSerializer2=phpDecoder.ToString();
	printf ("TestPhp4::passed %d: serializer[%s]\n",strSerializer2==strSerializer,strSerializer2.c_str());
}
void testPhpPerformance()
{	
	time_t t1, t2;
	time(&t1);

	struct timeval tm1,tm2;
	gettimeofday(&tm1,0);
	for(int i=0;i<10000;++i)
	{
		int nValue=0;
		string strValue;
		CPhpSerializer phpEncoder,phpDecoder;
		phpEncoder.SetValue("name","zong");
		phpEncoder.SetValue("age",56);

		
		const string & strSerializer=phpEncoder.ToString();

		phpDecoder.FromString(strSerializer);
		phpDecoder.GetValue("age", nValue);
		phpDecoder.GetValue("name", strValue);
	}
	gettimeofday(&tm2,0);
	time(&t2);
	printf("performance 10000: %d   %d.%06d\n",t2-t1,tm2.tv_sec-tm1.tv_sec,tm2.tv_usec-tm1.tv_usec);
}
void TestPhp()
{
	TestPhp1();
	TestPhp2();
	TestPhp3();
	TestPhp4();
	//testPhpPerformance();
}
#endif

