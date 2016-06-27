#include "WarnCondition.h"
#include <XmlConfigParser.h>
#include <stdlib.h>
#include "HTErrorMsg.h"
#include "HTDealerServiceLog.h"

using namespace sdo::common;
namespace HT{
	
int CWarnCondition::Init(const string &strCofigFile)
{
	HT_XLOG(XLOG_DEBUG, "CWarnCondition::%s\n", __FUNCTION__);
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile(strCofigFile) != 0)
	{
		HT_XLOG(XLOG_ERROR, "CWarnCondition::%s, Config Parser fail, file[%s], error[%s]\n", __FUNCTION__, strCofigFile.c_str(), objConfigParser.GetErrorMessage().c_str());
		return HT_WARN_CONFIG_ERROR;
	}

	m_objQueueWarn.dwTimeoutInterval = objConfigParser.GetParameter("Queue/QueueTimeoutInterval", 3);
	m_objQueueWarn.dwConditionTimes = objConfigParser.GetParameter("Queue/ConditionTimes", 10);
	m_objQueueWarn.dwAlarmFrequency = objConfigParser.GetParameter("Queue/AlarmFrequency", 10);
	m_objQueueWarn.bWriteQueueFullDiscard = objConfigParser.GetParameter("Queue/WriteQueueFullDiscard", 1);
	
	m_objDependencyWarn.dwAlarmFrequency = objConfigParser.GetParameter("Dependency/AlarmFrequency", 10);
	m_objDependencyWarn.dwAlarmListLength = objConfigParser.GetParameter("Dependency/AlarmListLength", 60000);

	Dump();
	return 0;
}

void CWarnCondition::Dump()
{
	HT_XLOG(XLOG_DEBUG, "CWarnCondition::%s\n", __FUNCTION__);
	HT_XLOG(XLOG_DEBUG, "------------------- Queue -------------------\n", __FUNCTION__);
	HT_XLOG(XLOG_DEBUG, "QueueTimeoutInterval:%d \n", m_objQueueWarn.dwTimeoutInterval);
	HT_XLOG(XLOG_DEBUG, "ConditionTimes:%d \n", m_objQueueWarn.dwConditionTimes);
	HT_XLOG(XLOG_DEBUG, "AlarmFrequency:%d \n", m_objQueueWarn.dwAlarmFrequency);
	HT_XLOG(XLOG_DEBUG, "WriteQueueFullDiscard:%d \n", m_objQueueWarn.bWriteQueueFullDiscard);
	HT_XLOG(XLOG_DEBUG, "----------------- Dependency -------------------\n", __FUNCTION__);
	HT_XLOG(XLOG_DEBUG, "AlarmFrequency:%d \n", m_objDependencyWarn.dwAlarmFrequency);
}

}
