#ifndef _REDIS_TYPE_MSG_OPERATOR_H_
#define _REDIS_TYPE_MSG_OPERATOR_H_
#include "ServiceConfig.h"

#include <string>
#include <vector>
#include <map>
#include <detail/_time.h>

using std::string;
using std::vector;
using std::map;
using namespace redis;

class CRedisVirtualService;
class CRedisContainer;

class IRedisTypeMsgOperator
{
public:
	virtual void OnProcess(unsigned int dwServiceId, 
		unsigned int dwMsgId,
		unsigned int dwSequenceId, 
		const string &strGuid, 
		void *pHandler, 
		void *pBuffer, 
		int nLen, 
		const timeval_a &tStart,
		float fSpentInQueueTime,
		CRedisContainer *pRedisContainer) = 0;
	IRedisTypeMsgOperator(CRedisVirtualService *pRedisVirtualService):m_pRedisVirtualService(pRedisVirtualService){}
	virtual ~IRedisTypeMsgOperator(){}
	
protected:
	void Ttl(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer, int responseType);
	void DeleteAll(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
	void BatchExpire(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
	void BatchDelete(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, CRedisContainer *pRedisContainer);
    void GetKey(unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, const string &strGuid, void *pHandler, void *pBuffer, int nLen, const timeval_a &tStart, float fSpentInQueueTime, int responseType);

	int GetFirstResponseCodeByMsgId(unsigned int dwServiceId, int operate);
	int GetFirstResponseValueCodeByMsgId(unsigned int dwServiceId, int operate, int minValueCode);
    int GetRequestKey(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strKey);
	int GetRequestVecKeyValue(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, vector<SKeyValue> &vecKeyValue, bool onlyResolveKey=false);
	int GetRequestEnocdedInfo(unsigned int dwServiceId, const void *pBuffer, unsigned int dwLen, string &strField, string &strValue);
	int GetKeyValueFromStruct(unsigned int dwServiceId, unsigned int nCode, const void *pBuffer, unsigned int dwLen, string &strKey, string &strValue);
		
	void ResponseStructArrayFromVec( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, vector<string> &vecValue, int structCode, const vector<SConfigField> &vecConfigField);
	void ResponseStructArrayFromMap( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, map<string, string> &mapFieldValue, int structCode, const vector<SConfigField> &vecConfigField);
	void ResponseStructArrayFromVecKFV( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const void *pBuffer, unsigned int dwLen, const vector<SKeyFieldValue> &vecKFV);
	void Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const string &strValue);
	void Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode, const void *pBuffer, unsigned int dwLen, const vector<SKeyValue> &vecKeyValue);

    bool IsRedisKeyValid(const string &strKey);
	bool IsStrDigit(const string &str);
	bool IsDigit(const string &str);
	string MakeRedisKeyValid(const string &strKey);
	void VectorToString(const vector<string> &vecValue, string &strValue);
	string VectorToString1(const vector<SKeyValue> &vecKeyValue);
	void MapToString(map<string, string> &mapFieldValue, string &str);
protected:
	CRedisVirtualService *m_pRedisVirtualService;	
};
#endif
