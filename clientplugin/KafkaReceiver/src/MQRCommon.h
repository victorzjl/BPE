#ifndef _MQR_COMMON_H_
#define _MQR_COMMON_H_

typedef struct stExtendData
{
	void *pData;
	unsigned int length;
}SExtendData;

typedef struct stConsumeInfo
{
	unsigned int serviceId;
	unsigned int partition;
	unsigned int offset;
	bool isAck;
}SConsumeInfo;

#endif //_MQR_COMMON_H_
