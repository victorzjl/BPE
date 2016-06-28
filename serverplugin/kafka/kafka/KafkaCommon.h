#ifndef _KAFKA_COMMON_H_
#define _KAFKA_COMMON_H_

#include <string>
#include "rdkafkacpp.h"

typedef struct stKafkaBTP
{
	std::string   brokerlist;   //server addresses
	std::string   topic;        //topic name
    int  partition;    //partition number
	
	stKafkaBTP():partition(RdKafka::Topic::PARTITION_UA) {}
}SKafkaBTP;

bool operator<(const SKafkaBTP& xObj, const SKafkaBTP& yObj);


class IKafka
{
public:
	virtual bool Connect() = 0;
	virtual bool Close() = 0;
	virtual bool IsConnected() { return m_IsConnected; }
	virtual ~IKafka() {}

protected:
    bool m_IsConnected;
};

class IKafkaErrorCallBack
{
public:
	virtual int OnKafkaError(IKafka *pSrc, const std::string& msg) = 0;
	virtual ~IKafkaErrorCallBack() {}		
};

#endif //_KAFKA_COMMON_H_
