#ifndef __CPP_FUNCTION_MANAGER_H_
#define __CPP_FUNCTION_MANAGER_H_
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <boost/unordered_map.hpp>
#include "FunctionParse.h"
#include "FunctionLuaParse.h"
using namespace std;

class CCPPFunctionManager
{
	typedef void(*FnCreateObj)();

public:
	CCPPFunctionManager();
	~CCPPFunctionManager();
	int LoadExecFunction();
private:
	int LoadSO(const char* pSoObj);
private:
	static const char* CPP_FUNCTION_PATH;
};

#endif

