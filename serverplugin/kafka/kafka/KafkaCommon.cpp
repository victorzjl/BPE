#include "KafkaCommon.h"
bool operator<(const SKafkaBTP& xObj, const SKafkaBTP& yObj)
{
	return (xObj.brokerlist<yObj.brokerlist)||(xObj.topic<yObj.topic);
}
