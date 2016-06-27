#ifndef __TRIGGER_CONFIG_H__
#define __TRIGGER_CONFIG_H__
#include "SapServiceInfo.h"
#include <map>
#include <string>
#include <vector>

namespace sdo{
namespace commonsdk{   
	class CAvenueMsgHandler;
}
}
/*namespace sdo
{
namespace util
{*/
class TiXmlElement;
//}
//}

using std::map;
using std::string;
using std::vector;
using sdo::commonsdk::CAvenueMsgHandler;
//using sdo::util::TiXmlElement;

typedef struct stLogPara
{
	string strDefault;
	int nKey;
	stLogPara(const string& strDefault_, int nKey_)
		:strDefault(strDefault_),nKey(nKey_)
		{}
}SLogPara;

typedef string (* GetValueFunc) (const string&, CAvenueMsgHandler*, const SLogPara&, const void*);

typedef struct stTriggerInfo
{
	string strName;
	GetValueFunc pFunc;
	int keyType;
	string strDefault;
	int nKey; // key in keyvalue

    bool bIsProcValue;
}STriggerInfo;
typedef vector<STriggerInfo> vecTriggerItem;

typedef struct stTriggerItem
{
	int nReqSize;
	int nResSize;
    int nDataDefSize;
	vecTriggerItem vecReq;
	vecTriggerItem vecRes;
    vecTriggerItem vecDataDef;
	int nCsosLog;   //是否记录CSOS详细日志
	stTriggerItem():nReqSize(0),nResSize(0), nDataDefSize(0),nCsosLog(0){}
}STriggerItem;

typedef enum
{
	RECORD_LOG = 1,
	CHECK_SUBSERVICE = 2
}ETRIGGERTYPE;



class CTriggerConfig
{
typedef map<SComposeKey, STriggerItem> mapTriggerInfo;
typedef struct stTriggerType
{
	STriggerItem defaultItem;
	mapTriggerInfo mapInfo;
}STriggerType;
typedef map<int, STriggerType> mapTrigger;

public:
	static CTriggerConfig* Instance();
	~CTriggerConfig();
	int LoadConfig(const string &fileName);
	STriggerItem* GetInfo(int nType, int nServId, int nMsgId);	
	void Dump();
private:
	CTriggerConfig(){}
    int LoadFiled(TiXmlElement *pItem, vecTriggerItem& vecItem);
    int LoadDataFiled(TiXmlElement *pItem, vecTriggerItem& vecItem);
private:
	static CTriggerConfig* m_instance;	
	mapTrigger m_mapTrigger;			
};
#endif
