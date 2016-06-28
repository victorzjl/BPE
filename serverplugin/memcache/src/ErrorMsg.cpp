#include "ErrorMsg.h"

string GetErrorMsg( CVSErrorCode code )
{
	string strReturnMsg;
	switch (code)
	{
	case CVS_SUCESS:
		strReturnMsg = "success";
		break;
	case CVS_SYSTEM_ERROR:
		strReturnMsg = "system error";
		break;
	case CVS_SERIALIZE_ERROR:
		strReturnMsg = "serialize error";
		break;
	case CVS_NO_CALLBACK_FUNC:
		strReturnMsg = "no callback function";
		break;
	case CVS_NO_INITIALIZE:
		strReturnMsg = "have not initialize";
		break;
	case CVS_UNKNOWN_SERVICE:
		strReturnMsg = "service not support";
		break;
	case CVS_UNKNOWN_MSG:
		strReturnMsg = "unknown msg";
		break;
	case CVS_BUSINESS_CONFIG_ERROR:
		strReturnMsg = "business config file error";
		break;
	case CVS_CACHE_CONFIG_ERROR:
		strReturnMsg = "cache config file error";
		break;
	case CVS_NO_CACHE_SERVER:
		strReturnMsg = "service without cache server";
		break;
	case CVS_PARAMETER_ERROR:
		strReturnMsg = "parameter error";
		break;
	case CVS_AVENUE_PACKET_ERROR:
		strReturnMsg = "avenue packet error";
		break;
	case CVS_CACHE_CONNECT_ERROR:
		strReturnMsg = "cache connect error";
		break;
	case CVS_CACHE_KEY_NOT_FIND:
		strReturnMsg = "key not find";
		break;
	default:
		strReturnMsg = "unknown error";
		break;
	}
	return strReturnMsg;
}

string GetErrorMsg( int nCode )
{
	return GetErrorMsg((CVSErrorCode)nCode);
}
