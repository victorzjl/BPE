#ifndef  BPE_PHP_SEREALIZER_H
#define BPE_PHP_SEREALIZER_H
#include <string>
#include<map>
#include "AvenueMsgHandler.h"
using std::map;
using std::string;
namespace sdo{
namespace commonsdk{
typedef enum emPhpSerializerUnitType
{
	Php_Serializer_NULL=0,
	Php_Serializer_INT=1,
	Php_Serializer_STRING=2,
	Php_Serializer_ARRAY=3,
	Php_Serializer_ALL
}EPhpSerializerUnitType;
struct stPhpSerializerUnit
{
	string strName;
	EValueType eType;

	int nValue;
	string strValue;
	map<string, stPhpSerializerUnit * > mapArray;
};

class CAvenueMsgHandler;
class CPhpSerializer
{
public:
	CPhpSerializer();
	~CPhpSerializer();
	CPhpSerializer(const string & strSerializer);
	void FromString(const string & strSerializer);
	string& ToString();

	int GetValue(const string& strName, string & strVlaue);
	int GetValue(const string& strName, int & nValue);
	int GetValue(const string& strName, stPhpSerializerUnit **pPhpSerializerUnit);
	int GetValue(const string& strName,void **pBuffer, unsigned int *pLen);
	int GetValue(const string& strName,void **pBuffer, unsigned int *pLen,EValueType &eValueType);

	void SetValue(const string& strName, const string & strValue);
	void SetValue(const string& strName, int nValue);
	void SetValue(const string& strName, stPhpSerializerUnit *pPhpSerializerUnit);
	void SetTlvGroup(CAvenueMsgHandler *pHandler);
	int& IsHasChanged(){m_vecNGetReference.push_back(m_isHasChanged);int & nValue=*(m_vecNGetReference.rbegin());return nValue;}

	void Dump();
	void DumpUnit(stPhpSerializerUnit * pUnit);
private:
	stPhpSerializerUnit * GetElement(const string& strName);
	int SetValueNoCopy(const string& strName, stPhpSerializerUnit *pPhpSerializerUnit);
	
	int ReadRootElement(const char **ppLoc);
	int ReadInt(const char **ppLoc,char cEnd,int *pValue);
	int ReadStr(const char **ppLoc,int nLen,string &strValue);
	int ReadStrByEnd(const char **ppLoc,char cEnd,string &strValue);
	int ReadArrayKey(const char **ppLoc,string &strKey);
	int ReadArray(const char **ppLoc,int nArrayNum,map<string, stPhpSerializerUnit * > &mapArray);
	
	int ElementToString(stPhpSerializerUnit * pUnit, char **ppLoc);
	int ArrayToString(map<string, stPhpSerializerUnit * > &mapArray, char **ppLoc);
	
private:
	map<string, stPhpSerializerUnit * > m_oElements;
	int m_nLoc;
	bool m_isHasChanged;
	vector<int> m_vecNGetReference;
	vector<string> m_vecStrGetReference;
};
}
}
#endif
