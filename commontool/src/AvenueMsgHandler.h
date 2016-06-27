#ifndef _MSG_HANDLER_H_
#define _MSG_HANDLER_H_

#include "CommonSDK.h"
#include "AvenueServiceConfigs.h"
#include "AvenueTlvGroup.h"
#include <sstream>
#include "PhpSerializer.h"

using std::multimap;
using std::stringstream;
namespace sdo{
namespace commonsdk{

class CPhpSerializer;
class CAvenueMsgHandler: public ISDOCommonMsg
{
public:
	SNDAMETHOD(int)	GetValue(IN const char* pKey, OUT void ** pValue, OUT int * pLen);
	SNDAMETHOD(int)	GetValue(IN const char* pKey, OUT char ** ppValue);
	SNDAMETHOD(int)	GetValue(IN const char* pKey, OUT int *pValue);

	SNDAMETHOD(void)	SetValueNoCaseType(IN const char * pKey, IN const char *pValue);
	SNDAMETHOD(void)	SetValue(IN const char* pKey, IN const void * pValue, IN int nLen);
	SNDAMETHOD(void)	SetValue(IN const char* pKey, IN const char* pValue);
	SNDAMETHOD(void)	SetValue(IN const char* pKey, IN int nValue);

	SNDAMETHOD(void) SetExtHead(IN const void * pExtHead, IN int nLen);
	SNDAMETHOD(void) GetExtHead(OUT void ** pExtHead, OUT int * pLen);
	SNDAMETHOD(char *)	GetServiceName();
	
	SNDAMETHOD(void) Release();
	
public:
	CAvenueMsgHandler(const string & strServiceName, bool bRequest = true,CAvenueServiceConfigs * pConfig=NULL);
	virtual ~CAvenueMsgHandler();
	SSeriveConfig * GetServiceConfig() {return m_oServiceConfig;}
	map<string, SAvenueTlvTypeConfig> &GetTlvMap()
	{
		if(m_bRequest) 
			return m_oMessage->oRequestTlvRule.mapTlvType;
		else 
			return m_oMessage->oResponseTlvRule.mapTlvType;
	}
	bool IsRequest() {return m_bRequest;}
	EValueType GetType(IN const char* pKey);
	
	
	int Decode(const void *pBuffer,int nLen);
	int Decode(const void *pBuffer,int nLen,int & nReturnMsgType,string &strReturnMsg);
	int GetValues(IN const char* pKey, OUT vector<SStructValue> & vecValue);
	int GetValues(IN const char* pKey, OUT vector<SStructValue> & vecValue, OUT EValueType &eValueType);
	int GetValue(IN const char* pKey, OUT void ** pValue, OUT int * pLen, OUT EValueType &eValueType);

	void SetValues(IN const char* pKey, IN const vector<SStructValue> & vecValue);
	void SetTlvGroup(CAvenueMsgHandler *pHandler);
	void SetPhpSerializer(CPhpSerializer *pPhpSerializer);
	int Encode();
	void* GetBuffer() {return (void *)m_pMsgEnCoder->GetBuffer();}
	int GetLen() {return m_pMsgEnCoder->GetLen();}
	void Reset();
	
	void Dump();

	/*for ext head */
	void *GetExtHead(){return m_pExtHeadBuf;}
	int GetExtHeadLen(){return m_nExtHeadLen;}

	/*get msg param*/
	void GetMsgParam(string &strParam);
	
private:
	void ConverterCoder_(const string & strSrcEncoding, const string & strSrc, const string & strDstEncoding, string &strDst);
	string GetStringArrayLog(vector<SStructValue>& vecValues);
private:
	int m_nIndex;
	string m_strSericeName;
	SSeriveConfig *m_oServiceConfig;
	SAvenueMessageConfig *m_oMessage;
	bool m_bRequest;
	CAvenueTlvGroupEncoder * m_pMsgEnCoder;
	CAvenueTlvGroupDecoder * m_pMsgDeCoder;

	void * m_pBuffer;
	unsigned int m_nLen;
	unsigned int m_nBufferLen;

	void *m_pExtHeadBuf;
	unsigned int m_nExtHeadLen;
	unsigned int m_nExtHeadBufLen;

};
}
}
#endif







