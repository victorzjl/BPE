#ifndef _HT_ERROR_MSG_H_
#define _HT_ERROR_MSG_H_

#include <string>
using std::string;

typedef enum enHTErrorCode
{
	HT_SUCESS = 0,							        //成功
	HT_SYSTEM_ERROR = -10905001,					//系统错误
	HT_SERIALIZE_ERROR = -10905002,				//序列化错误
	HT_KEY_IS_NULL = -10905003,                    //Key为空
	HT_KEY_TOO_LONG = -10905004,				    //Key过长
	HT_NO_CALLBACK_FUNC = -10905007,				//插件没有设置回调函数
	HT_NO_INITIALIZE = -10905008,				    //插件没有初始化
	HT_NEW_SPACE_ERROR = -10905009,			    //申请空间失败
	HT_MSG_QUEUE_FULL = -10905011,              //消息队列已满
	HT_UNSUPPORT_SERVICE = -10905010,				//不支持的服务号
	HT_WARN_CONFIG_ERROR = -10905011,               //warn配置文件错误
	HT_UNSUPPORT_MESSAGE = -10905012,					//不支持的消息号
	HT_BUSINESS_CONFIG_ERROR = -10905013,		    //业务配置文件错误
	HT_REDIS_CONFIG_ERROR = -10905014,			    //服务配置文件错误
	HT_UNCORRECT_PARA_SENDFUNC = -10905015,			    //发送函数参数个数错误

	HT_PARAMETER_ERROR = -10905017,				//参数错误	
	HT_BAD_AVENUE = -10905018,			         //Avenue包错误,缺少参数
	HT_CONFIG_TYPE_NOT_EXIST = -10905019,        //类型不存在
	HT_UNCORRECT_SNEDFUNC = -10905020,           //发送函数格式错误
	HT_STRUCT_IS_WRONG = -10905022,             //struct结构体解析出错
	HT_NO_HOST = -10905023,                     //对应服务号的主机名不存在
	HT_NO_URI = -10905024,                      //相应的uri不存在
	HT_OPERATION_FAILED = -10905025,         //操作失败
	HT_CONFIG_ERROR = -10905026,             //读取config文件错误
	HT_OTHER_ERROR = -10905029,				          //其它错误
	HT_DATA_FORMAT_ERROR = -10905030 ,              //数据格式错误
	HT_DATA_TYPE_NOT_EXIST = -10905031 ,            //数据类型不存在
	HT_AVENUE_PACKET_TOO_LONG = -10905032,
	HT_RECVFUNC_NOT_SUPPORT = -10905033,            //用户定义接受函数不存在
	HT_SENDFUNC_NOT_SUPPORT = -10905034,			//用户定义发送函数不存在
	HT_HTTP_TO_JSON_FAIL = -10905035  ,              //http to  json 失败
	HT_BUILD_HTTPBODY_FAIL = -10905036   ,              //http包体构建失败
	HT_READ_QUEUE_FULL = -10905037
	
}HTErrorCode;

#endif
