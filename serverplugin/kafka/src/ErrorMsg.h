#ifndef _MQS_ERROR_MSG_H_
#define _MQS_ERROR_MSG_H_

#include <string>
using std::string;

typedef enum enErrorCode
{
	MQS_SUCESS = 0,							    //成功
	MQS_SYSTEM_ERROR = -10901201,				//系统错误
	MQS_SERIALIZE_ERROR = -10901202,			//序列化错误
	MQS_CONNECT_ERROR = -10901203,				//服务器连接失败
	MQS_QUEUE_FULL = -10901204,			        //任务队列满
	MQS_AVENUE_PACKET_ERROR = -10901205,		//Avenue包错误,缺少参数
	MQS_NO_CALLBACK_FUNC = -10901206,			//插件没有设置回调函数
	MQS_NO_INITIALIZE = -10901207,				//插件没有初始化
	MQS_NEW_SPACE_ERROR = -10901208,			//申请空间失败
	MQS_UNKNOWN_SERVICE = -10901209,			//不支持的服务号
	MQS_PRODUCE_ERROR = -10901210,		        //消息发送错误
	MQS_CONFIG_ERROR = -10901211,			    //服务器配置文件错误
	MQS_NO_TOPIC = -10901212,				    //服务没有对应的topic
	MQS_WARN_CONFIG_ERROR = -10901213,		    //Warn配置文件错误
	MQS_PARAMETER_ERROR = -10901214,		    //参数错误
	MQS_OTHER_ERROR = -10901299					//其它错误
}MQSErrorCode;

string GetErrorMsg(MQSErrorCode code);
string GetErrorMsg(int nCode);

#endif
