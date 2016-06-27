#ifndef _SNDA_TLV_CONTENT_CPP_H_
#define _SNDA_TLV_CONTENT_CPP_H_
#include "SapBuffer.h"
#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::multimap;

typedef struct tagSapMsgAttribute
{
	unsigned short wType;
	unsigned short wLength;
	unsigned char acValue[0];
}SSapMsgAttribute;

typedef struct tagSapValueNode
{
	void * pLoc;
	unsigned int nLen;
}SSapValueNode;

class CSapTLVBodyEncoder
{
public:
	void SetValue(unsigned short wKey,const void* pValue,unsigned int nValueLen);
	void SetValue(unsigned short wKey,const string &strValue);
	void SetValue(unsigned short wKey,unsigned int wValue);

	void BeginValue(unsigned short wType);
	void AddValueBloc(const void *pData,unsigned int nLen);
	void EndValue();

	const void * GetBuffer()const{return m_buffer.base();}
	int GetLen()const{return m_buffer.len();}
private:
	CSapBuffer m_buffer;
	unsigned char *pAttriBlock;
};

class CSapTLVBodyDecoder
{
	typedef multimap<unsigned short,const unsigned char *> AttriMultiMap;
public:
	CSapTLVBodyDecoder() {};
    void SetBuffer(const void * pBuffer, unsigned int nLen);
	CSapTLVBodyDecoder(const void * pBuffer, unsigned int nLen);
	/*Get attribute*/
	int GetValue(unsigned short wKey,void** pValue, unsigned int * pValueLen) ;
	int GetValue(unsigned short wKey, string & strValue);
	int GetValue(unsigned short wKey, unsigned int * pValue) ;
	
	void GetValues(unsigned short wKey,vector<SSapValueNode> &vecValues);
	void GetValues(unsigned short wKey,vector<string> &vecValues);
	void GetValues(unsigned short wKey,vector<unsigned int> &vecValues);
private:
	const unsigned char * m_pBuffer;
	unsigned int m_nLen;
	
	AttriMultiMap m_mapMultiAttri;
};

#endif

