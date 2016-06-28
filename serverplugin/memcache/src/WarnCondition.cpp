#include "WarnCondition.h"
#include <XmlConfigParser.h>
#include <stdlib.h>
#include "ErrorMsg.h"
#include "AsyncVirtualServiceLog.h"

using namespace sdo::common;

int CWarnCondition::Init(const string &strCofigFile)
{
	CVS_XLOG(XLOG_DEBUG, "CWarnCondition::%s\n", __FUNCTION__);
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile(strCofigFile) != 0)
	{
		CVS_XLOG(XLOG_ERROR, "CWarnCondition::%s, Config Parser fail, file[%s], error[%s]\n", __FUNCTION__, strCofigFile.c_str(), objConfigParser.GetErrorMessage().c_str());
		return CVS_WARN_CONFIG_ERROR;
	}

	m_objQueueWarn.dwTimeoutInterval = objConfigParser.GetParameter("Queue/QueueTimeoutInterval", 3);
	m_objQueueWarn.dwConditionTimes = objConfigParser.GetParameter("Queue/ConditionTimes", 10);
	m_objQueueWarn.dwAlarmFrequency = objConfigParser.GetParameter("Queue/AlarmFrequency", 10);
	m_objQueueWarn.bWriteQueueFullDiscard = objConfigParser.GetParameter("Queue/WriteQueueFullDiscard", 1);
	
	m_objDependencyWarn.dwAlarmFrequency = objConfigParser.GetParameter("Dependency/AlarmFrequency", 10);

	Dump();
	return 0;
}

void CWarnCondition::Dump()
{
	CVS_XLOG(XLOG_DEBUG, "CWarnCondition::%s\n", __FUNCTION__);
	CVS_XLOG(XLOG_DEBUG, "------------------- Quere -------------------\n", __FUNCTION__);
	CVS_XLOG(XLOG_DEBUG, "QueueTimeoutInterval:%d \n", m_objQueueWarn.dwTimeoutInterval);
	CVS_XLOG(XLOG_DEBUG, "ConditionTimes:%d \n", m_objQueueWarn.dwConditionTimes);
	CVS_XLOG(XLOG_DEBUG, "AlarmFrequency:%d \n", m_objQueueWarn.dwAlarmFrequency);
	CVS_XLOG(XLOG_DEBUG, "WriteQueueFullDiscard:%d \n", m_objQueueWarn.bWriteQueueFullDiscard);
	CVS_XLOG(XLOG_DEBUG, "----------------- Dependency -------------------\n", __FUNCTION__);
	CVS_XLOG(XLOG_DEBUG, "AlarmFrequency:%d \n", m_objDependencyWarn.dwAlarmFrequency);
}
