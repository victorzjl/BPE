#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_
#include <string>
#include <map>
using std::string;
using std::map;

class CErrorCode
{
public:
	static CErrorCode* GetInstance();
	string GetErrorMessage(int nCode);
	virtual ~CErrorCode(){}
private:
	CErrorCode();
	static CErrorCode *sm_pInstance;
	map<int, string> m_mapErrorMsg;	
};

#endif
