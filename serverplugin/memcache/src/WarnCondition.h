#ifndef _CVS_EXCEPTION_WARN_H_
#define _CVS_EXCEPTION_WARN_H_

#include <string>
using std::string;

typedef struct stQueueWarnCondition
{
	unsigned int dwTimeoutInterval;
	unsigned int dwConditionTimes;
	unsigned int dwAlarmFrequency;
	bool bWriteQueueFullDiscard;
	stQueueWarnCondition():dwTimeoutInterval(3), dwConditionTimes(10), dwAlarmFrequency(10), bWriteQueueFullDiscard(1){}
}SQueueWarnCondition;

typedef struct stDependencyWarnCondition
{
	unsigned int dwAlarmFrequency;
	stDependencyWarnCondition():dwAlarmFrequency(10){}
}SDependencyWarnCondition;

class CWarnCondition
{
public:
	CWarnCondition(){};
	virtual ~CWarnCondition(){};

	int Init(const string &strCofigFile);
	SQueueWarnCondition GetQueueWarnCondition(){return m_objQueueWarn;}
	SDependencyWarnCondition GetDependencyWarnCondition(){return m_objDependencyWarn;}
private:
	void Dump();
private:
	SQueueWarnCondition m_objQueueWarn;
	SDependencyWarnCondition m_objDependencyWarn;
};

#endif
