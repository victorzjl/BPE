#ifndef _RVS_REDIS_ERROR_MSG_H_
#define _RVS_REDIS_ERROR_MSG_H_

#include <string>
using std::string;

typedef enum enRedisErrorCode
{
	RVS_SUCESS = 0,							        //成功
	RVS_SYSTEM_ERROR = -10903001,					//系统错误
	RVS_SERIALIZE_ERROR = -10903002,				//序列化错误
	RVS_KEY_IS_NULL = -10903003,                    //Key为空
	RVS_KEY_TOO_LONG = -10903004,				    //Key过长
	RVS_READ_QUEUE_FULL = -10903005,			    //缓存读队列满
	RVS_WRITE_QUEUE_FULL = -10903006,			    //缓存写队列满
	RVS_NO_CALLBACK_FUNC = -10903007,				//插件没有设置回调函数
	RVS_NO_INITIALIZE = -10903008,				    //插件没有初始化
	RVS_NEW_SPACE_ERROR = -10903009,			    //申请空间失败
	RVS_UNKNOWN_SERVICE = -10903010,				//不支持的服务号
	RVS_UNKNOWN_REDIS_TYPE = -10903011,             //不支持的redis数据类型
	RVS_UNKNOWN_MSG = -10903012,					//不支持的消息号
	RVS_BUSINESS_CONFIG_ERROR = -10903013,		    //业务配置文件错误
	RVS_REDIS_CONFIG_ERROR = -10903014,			    //Redis服务配置文件错误
	RVS_NO_REDIS_SERVER = -10903015,				//服务没有对应的Redis服务器
	RVS_WARN_CONFIG_ERROR = -10903016,				//Warn配置文件错误
	RVS_PARAMETER_ERROR = -10903017,				//参数错误
	RVS_AVENUE_PACKET_ERROR = -10903018,			//Avenue包错误,缺少参数
	RVS_REDIS_CONNECT_ERROR = -10903019,			//Redis服务器连接失败
	RVS_REDIS_KEY_NOT_FOUND = -10903020,			//Redis服务器中末找到对应的key值
	RVS_REDIS_KEY_VERSION_NOT_ACCORD = -10903021,	//key值version不一致
	RVS_REDIS_STRUCT_IS_WRONG = -10903022,          //struct结构体解析出错
	RVS_REDIS_FIELD_IS_NULL = -10903023,            //Hash结构时Field为空
	RVS_REDIS_VALUE_IS_NULL = -10903024,            //Hash结构时Value为空
	RVS_REDIS_OPERATION_FAILED = -10903025,         //操作失败
	RVS_REDIS_NOT_SET_EXPIRE_TIME = -10903026,        //key没有设置超时时间
	RVS_REDIS_LIST_INDEX_OUT_RANGE = -10903027,      //list的index越界或指定key不存在
	RVS_REDIS_ZSET_NO_MEMBERS = -10903028,           //zset在某区间范围内没有元素
	RVS_OTHER_ERROR = -10903029,				          //其它错误
	RVS_REDIS_DATA_FORMAT_ERROR = -10903030               //数据格式错误
}RVSErrorCode;

#endif
