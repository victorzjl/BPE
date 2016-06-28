#ifndef TIMERCOMMON_H_INCLUDED
#define TIMERCOMMON_H_INCLUDED
#include "detail/_time.h"
#ifdef WIN32
#include <objbase.h>
#else
#include <uuid/uuid.h>
#endif

typedef struct stConfigTimer
{
    int servId;
    int msgId;
    struct timeval_a tmBegin;
    int invl;
    int type;
    char addr[128];
public:
    bool operator<( const stConfigTimer& oComp ) const
    {
    return this->servId < oComp.servId || (this->servId == oComp.servId && this->msgId < oComp.msgId);
    }
    bool operator==( const stConfigTimer& oComp ) const
    {
    return this->servId == oComp.servId && this->msgId == oComp.msgId;
    }
} SConfigTimer;

typedef struct stTimerMsg
{
    struct timeval_a tmStart;
    char guid[64];
public:
    stTimerMsg()
    {
        gettimeofday_a(&tmStart,0);

        uuid_t uu;
        #ifdef WIN32
        if (S_OK == ::CoCreateGuid(&uu))
        {
		snprintf(guid, sizeof(guid),"%X%X%X%02X%02X%02X%02X%02X%02X%02X%02X",
            uu.Data1,uu.Data2,uu.Data3,uu.Data4[0],uu.Data4[1],uu.Data4[2],uu.Data4[3],uu.Data4[4],uu.Data4[5],uu.Data4[6],uu.Data4[7]);
        }
        #else
            uuid_generate( uu );
            snprintf(guid, sizeof(guid), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                    uu[0], uu[1], uu[2],  uu[3],  uu[4],  uu[5],  uu[6],  uu[7],
                    uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
        #endif
    }
    struct timeval_a calcInteval()
    {
        struct timeval_a now;
        gettimeofday_a(&now,0);

        struct timeval_a diff;
        timersub(&now, &tmStart, &diff);
        return diff;
    }
} STimerMsg;

const unsigned int SERVICE_ID = 41646;

#endif // TIMERCOMMON_H_INCLUDED
