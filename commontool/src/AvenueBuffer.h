#ifndef _AVENUE_BUFFER_H_
#define _AVENUE_BUFFER_H_
#include <stdlib.h>
#include <deque>


namespace sdo{
namespace commonsdk{
const int SAP_BUFFER_INIT_CAPACITY=2048;
//#define SAP_ALIGN(size, boundary)  (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define SAP_ALIGN(size)  (((size) + 2047) & ~2047) //SAP_ALIGN(size, 2048) 
#define SDO_ALIGN(size, boundary)  (((size) + ((boundary) - 1)) & ~((boundary) - 1))
class CAvenueBuffer
{
public:
	CAvenueBuffer(int capacity=SAP_BUFFER_INIT_CAPACITY):capacity_(capacity),loc_(0)
	{
		base_=(unsigned char *)malloc(capacity);
	}
	void add_capacity()
	{
		capacity_+=capacity_;
		base_=(unsigned char *)realloc(base_,capacity_);
	}
	void add_capacity(int nLeft)
	{
		capacity_+=nLeft;
		base_=(unsigned char *)realloc(base_,capacity_);
	}
	unsigned char * base() const {return base_;}
	unsigned char * top() const {return base_+loc_;}
	void inc_loc(int loc){loc_+=loc;}
	void reset_loc(int loc){loc_=loc;}
	unsigned int capacity() const {return capacity_-loc_;}
	unsigned int len() const{return loc_;};
	~CAvenueBuffer(){free(base_);}
private:
	unsigned char *base_;
	unsigned int capacity_;
	unsigned int loc_;
};

typedef struct stSapLenMsg
{
	int nLen;
	const void *pBuffer;
}SAvenusLenMsg;
typedef std::deque<SAvenusLenMsg> AvenueLenMsgQueue;
}
}
#endif
