#ifndef TIMERSERVICECONFIG_H_INCLUDED
#define TIMERSERVICECONFIG_H_INCLUDED
#include <string>
#include "detail/_time.h"
#include "tinyxml/ex_tinyxml.h"
#include <vector>
#include "SapMessage.h"
#include "SapTLVBody.h"
using namespace sdo::sap;

using namespace sdo::util;
using std::string;
using std::vector;

typedef struct stConfigRequest
{
    unsigned int nServiceId;
    unsigned int nMessageId;
    unsigned int nInterval;
    void *pPacket;
    unsigned int nPacketLen;
    struct timeval_a tmStart;
}SConfigRequest;

class CTimerServiceConfig
{
public:
    CTimerServiceConfig();
	~CTimerServiceConfig();

    int loadServiceConfig(const string &strConfig);

    const vector<SConfigRequest>& getRequests(){return m_vecRequest;}

    void Dump() const;

    void dump_sap_packet_info(const void *pBuffer);
private:
    string GetInnerTextByElement(TiXmlElement *pElement, const string& defaultValue);
	int GetInnerTextByElement(TiXmlElement *pElement, int defaultValue);
private:
    vector<SConfigRequest> m_vecRequest;
};


#endif // TIMERSERVICECONFIG_H_INCLUDED
