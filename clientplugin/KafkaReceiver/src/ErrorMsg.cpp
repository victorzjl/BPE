#include "ErrorMsg.h"

string GetErrorMsg( MQRErrorCode code )
{
	string strReturnMsg;
	switch (code)
	{
	case MQR_SUCESS:
		strReturnMsg = "success";
		break;
	case MQR_SYSTEM_ERROR:
		strReturnMsg = "system error";
		break;
	case MQR_SERIALIZE_ERROR:
		strReturnMsg = "serialize error";
		break;
	case MQR_KAFKA_CONNECT_ERROR:
		strReturnMsg = "kafka server connect error";
		break;
	case MQR_ZOOKEEPER_CONNECT_ERROR:
		strReturnMsg = "zookeeper server connect error";
		break;	
	case MQR_AVENUE_PACKET_ERROR:
		strReturnMsg = "avenue packet error";
		break;
	case MQR_NO_CALLBACK_FUNC:
		strReturnMsg = "no callback function";
		break;
	case MQR_NO_INITIALIZE:
		strReturnMsg = "have not initialize";
		break;
	case MQR_NEW_SPACE_ERROR:
		strReturnMsg = "new space acquiring error";
		break;	
	case MQR_UNKNOWN_SERVICE:
		strReturnMsg = "service not support";
		break;	
	case MQR_PRODUCE_ERROR:
		strReturnMsg = "message sending error";
		break;
	case MQR_CONFIG_ERROR:
		strReturnMsg = "cache config file error";
		break;
	case MQR_NO_TOPIC:
		strReturnMsg = "have no topic for some one service";
		break;
	case MQR_PARAMETER_ERROR:
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
	return GetErrorMsg((MQRErrorCode)nCode);
}
