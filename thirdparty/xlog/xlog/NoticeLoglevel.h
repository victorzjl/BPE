#ifndef _NOTICE_LOGLOVEL_H_
#define _NOTICE_LOGLOVEL_H_
#include <log4cplus/logger.h>
#include <log4cplus/helpers/loglog.h>

using namespace log4cplus;
using namespace log4cplus::helpers;

const LogLevel NOTICE_LOG_LEVEL = 25000;
#define _NOTICE_STRING LOG4CPLUS_TEXT("NOTICE")

	static tstring
noticeToStringMethod(LogLevel ll)
{
	if(ll == NOTICE_LOG_LEVEL) {
		return _NOTICE_STRING;
	}
	else {
		return tstring();
	}
}



	static LogLevel
noticeFromStringMethod(const tstring& s)
{
	if(s == _NOTICE_STRING) return NOTICE_LOG_LEVEL;

	return NOT_SET_LOG_LEVEL;
}



class NoticeLogLevelInitializer {
	public:
		NoticeLogLevelInitializer() {
			getLogLevelManager().pushToStringMethod(noticeToStringMethod);
			getLogLevelManager().pushFromStringMethod(noticeFromStringMethod);
		}
};
#endif
