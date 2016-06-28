#ifndef _CVS_ERROR_MSG_H_
#define _CVS_ERROR_MSG_H_

#include <string>
using std::string;

typedef enum enErrorCode
{
	CVS_SUCESS = 0,							//成功
	CVS_SYSTEM_ERROR = -10901101,					//系统错误
	CVS_SERIALIZE_ERROR = -10901102,				//序列化错误
	CVS_KEY_TOO_LONG = -10901103,				//Key过长
	CVS_READ_QUEUE_FULL = -10901104,			//缓存读队列满
	CVS_WRITE_QUEUE_FULL = -10901105,			//缓存写队列满
	CVS_NO_CALLBACK_FUNC = -10901110,				//插件没有设置回调函数
	CVS_NO_INITIALIZE = -10901111,				//插件没有初始化
	CVS_NEW_SPACE_ERROR = -10901112,			//申请空间失败
	CVS_UNKNOWN_SERVICE = -10901120,				//不支持的服务号
	CVS_UNKNOWN_MSG = -10901121,					//不支持的消息号，目前仅支持1,2,3,4
	CVS_BUSINESS_CONFIG_ERROR = -10901122,		//业务配置文件错误
	CVS_CACHE_CONFIG_ERROR = -10901123,			//Cache服务器配置文件错误
	CVS_NO_CACHE_SERVER = -10901124,				//服务没有对应的Cache服务器
	CVS_WARN_CONFIG_ERROR = -10901125,				//Warn配置文件错误
	CVS_PARAMETER_ERROR = -10901130,				//参数错误
	CVS_AVENUE_PACKET_ERROR = -10901131,			//Avenue包错误,缺少参数
	CVS_CACHE_CONNECT_ERROR = -10901141,			//Cache服务器连接失败
	CVS_CACHE_KEY_NOT_FIND = -10901142,			//Cache服务器中末找到对应的key值
	CVS_CACHE_KEY_VERSION_NOT_ACCORD = -10901143,	//key值version不一致
	CVS_CACHE_CAS_NOT_SUPPORT = -10901144,			//cache不支持cas
	CVS_CACHE_ALREADY_EXIST = -10901145,			//key已经存在不允许add
	CVS_OTHER_ERROR = -10901199					//其它错误
}CVSErrorCode;

string GetErrorMsg(CVSErrorCode code);
string GetErrorMsg(int nCode);

#endif
