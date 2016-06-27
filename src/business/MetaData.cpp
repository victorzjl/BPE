#include "MetaData.h"

void stValueData::Dump()
{
	SS_XLOG(XLOG_DEBUG,"stValueData::%s >>>>\n", __FUNCTION__);
	SS_XLOG(XLOG_DEBUG,"eType: %d,\n", enType);
	for(vector<sdo::commonsdk::SStructValue>::iterator it = this->vecValue.begin(); it!=this->vecValue.end(); it++)
	{
		SS_XLOG(XLOG_DEBUG,"POINT:  %p,%d\n", it->pValue,it->nLen);
	}
	vector<string>::iterator it_value = this->vecStoreValue.begin();
	for(; it_value!=this->vecStoreValue.end();it_value++)
	{
		SS_XLOG(XLOG_DEBUG,"STORE:  %p,%d\n", it_value->c_str(),it_value->size());
	}
	SS_XLOG(XLOG_DEBUG,"stValueData::%s <<<<\n", __FUNCTION__);
}


