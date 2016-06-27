#ifndef __SAP_SERVICE_INFO_H__
#define __SAP_SERVICE_INFO_H__

struct SComposeKey
{
	unsigned dwServiceId;
	unsigned dwMsgId;
	SComposeKey(unsigned dwServiceId_=0,unsigned dwMsgId_=0)
		:dwServiceId(dwServiceId_),dwMsgId(dwMsgId_)
		{}
	
	bool operator<(const SComposeKey &rhs)const
	{
		return (dwServiceId<rhs.dwServiceId || (!(rhs.dwServiceId<dwServiceId)&&(dwMsgId<rhs.dwMsgId)));		
	}
};

#endif
