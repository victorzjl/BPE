#ifndef _SAP_BUFFER_H_
#define _SAP_BUFFER_H_
#include <stdlib.h>
namespace sdo{
namespace sap{
const int SAP_BUFFER_INIT_CAPACITY=8192;
//#define SAP_ALIGN(size, boundary)  (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define SAP_ALIGN(size)  (((size) + 2047) & ~2047) //SAP_ALIGN(size, 2048) 
class CSapBuffer
{
public:
	CSapBuffer(int capacity=SAP_BUFFER_INIT_CAPACITY):capacity_(capacity),loc_(0)
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
	~CSapBuffer(){free(base_);}
private:
	unsigned char *base_;
	unsigned int capacity_;
	unsigned int loc_;
};
}
}
#endif
