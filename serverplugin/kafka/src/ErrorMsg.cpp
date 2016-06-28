#include "ErrorMsg.h"

string GetErrorMsg( MQSErrorCode code )
{
	string strReturnMsg;
	switch (code)
	{
	case MQS_SUCESS:
		strReturnMsg = "success";
		break;
	case MQS_SYSTEM_ERROR:
		strReturnMsg = "system error";
		break;
	case MQS_SERIALIZE_ERROR:
		strReturnMsg = "serialize error";
		break;
	case MQS_CONNECT_ERROR:
		strReturnMsg = "server connect error";
		break;
	case MQS_QUEUE_FULL:
		strReturnMsg = "task queue is full";
		break;	
	case MQS_AVENUE_PACKET_ERROR:
		strReturnMsg = "avenue packet error";
		break;
	case MQS_NO_CALLBACK_FUNC:
		strReturnMsg = "no callback function";
		break;
	case MQS_NO_INITIALIZE:
		strReturnMsg = "have not initialize";
		break;
	case MQS_NEW_SPACE_ERROR:
		strReturnMsg = "new space acquiring error";
		break;	
	case MQS_UNKNOWN_SERVICE:
		strReturnMsg = "service not support";
		break;	
	case MQS_PRODUCE_ERROR:
		strReturnMsg = "message sending error";
		break;
	case MQS_CONFIG_ERROR:
		strReturnMsg = "cache config file error";
		break;
	case MQS_NO_TOPIC:
		strReturnMsg = "have no topic for some one service";
		break;
	case MQS_PARAMETER_ERROR:
		strReturnMsg = "parameter error";
		break;
	default:
		strReturnMsg = "unknown error";
		break;
	}
	return strReturnMsg;
}

string GetErrorMsg( int nCode )
{
	return GetErrorMsg((MQSErrorCode)nCode);
}
