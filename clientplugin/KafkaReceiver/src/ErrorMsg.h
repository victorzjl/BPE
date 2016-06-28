#ifndef _MQR_ERROR_MSG_H_
#define _MQR_ERROR_MSG_H_

#include <string>
using std::string;

typedef enum enErrorCode
{
	MQR_SUCESS = 0,							    //成功
	MQR_SYSTEM_ERROR = -10901301,				//系统错误
	MQR_SERIALIZE_ERROR = -10901302,			//序列化错误
	MQR_ZOOKEEPER_CONNECT_ERROR = -10901303,	//zookeeper服务器连接失败
	MQR_NO_CORRESPONDING_REQUEST = -10901304,	//响应无对应的请求
	MQR_AVENUE_PACKET_ERROR = -10901305,		//Avenue包错误,缺少参数
	MQR_NO_CALLBACK_FUNC = -10901306,			//插件没有设置回调函数
	MQR_NO_INITIALIZE = -10901307,				//插件没有初始化
	MQR_NEW_SPACE_ERROR = -10901308,			//申请空间失败
	MQR_UNKNOWN_SERVICE = -10901309,			//不支持的服务号
	MQR_PRODUCE_ERROR = -10901310,		        //消息发送错误
	MQR_CONFIG_ERROR = -10901311,			    //服务器配置文件错误
	MQR_NO_TOPIC = -10901312,				    //服务没有对应的topic
	MQR_WARN_CONFIG_ERROR = -10901313,		    //Warn配置文件错误
	MQR_PARAMETER_ERROR = -10901314,		    //参数错误
	MQR_KAFKA_CONNECT_ERROR = -10901315,		    //kafka服务器连接失败
	MQR_OTHER_ERROR = -10901399					//其它错误
}MQRErrorCode;

string GetErrorMsg(MQRErrorCode code);
string GetErrorMsg(int nCode);

#endif
