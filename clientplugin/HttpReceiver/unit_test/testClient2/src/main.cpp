#include "HttpResponseDecoder.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
		char *pResponse = "HTTP/1.1 200 OK\r\n\
Date:Sun, 21 Aug 2011 12:13:30 GMT\r\n\
Server:Apache/2.2.8 (Win32) PHP/5.2.5\r\n\
X-Powered-By:PHP/5.2.5\r\n\
Expires:Thu, 19 Nov 1981 08:52:00 GMT\r\n\
Cache-Control:no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n\
Pragma:no-cache\r\n\
Content-Length:6623\r\n\
Keep-Alive:timeout=5, max=99\r\n\
Connection:Keep-Alive\r\n\
Content-Type:text/html\r\n\
\r\n";

	CHttpResponseDecoder decoder;
	decoder.Decode(pResponse, strlen(pResponse));
	
	fprintf(stdout,"body{%s}\n",decoder.GetBody().c_str());
	fprintf(stdout,"GetHttpCode{%d}\n",decoder.GetHttpCode());
	decoder.Dump();
	
	return 0;
}



