#include "Common.h"
#include "CommonMacro.h"
#include <stdio.h>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>

////////////////////////////////////////////////////////////////////////////////////////////
//时间相关

string GetCurrenStringTime()
{
	struct timeval_a tNow;
	gettimeofday_a(&tNow, 0);
	struct tm tmStart;
	localtime_r(&(tNow.tv_sec),&tmStart);

	char temp[256] = {0};
	snprintf(temp, sizeof(temp)-1, "%04u-%02u-%02u %02u:%02u:%02u",
						tmStart.tm_year+1900,
						tmStart.tm_mon+1,
						tmStart.tm_mday,
						tmStart.tm_hour,
						tmStart.tm_min,
						tmStart.tm_sec
						);
	return temp;
}

string TimevalToString(const timeval_a &tvTime)
{
	struct tm tmStart;
	localtime_r(&(tvTime.tv_sec),&tmStart);

	char temp[256] = {0};
	snprintf(temp, sizeof(temp)-1, "%04u-%02u-%02u %02u:%02u:%02u %03u.%03lu",
						tmStart.tm_year+1900,
						tmStart.tm_mon+1,
						tmStart.tm_mday,
						tmStart.tm_hour,
						tmStart.tm_min,
						tmStart.tm_sec,
						static_cast<int>(tvTime.tv_usec/1000),
						tvTime.tv_usec%1000
						);
	return temp;
}

//
//获取时间间隔，单位为ms
//
float GetTimeInterval(const timeval_a &tStart)
{
	struct timeval_a now;
	struct timeval_a interval;
	gettimeofday_a(&now,0); 	
	timersub(&now, &tStart, &interval);
	float lfInterval = (static_cast<float>(interval.tv_sec))*1000 + (static_cast<float>(interval.tv_usec))/1000;
	return lfInterval;
}

////////////////////////////////////////////////////////////////////////////////////////////
//网络相关

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
int GetLocalAddress(char* ip)  
{  
	//1.初始化wsa   
	WSADATA wsaData;  
	int ret=WSAStartup(MAKEWORD(2,2),&wsaData);  
	if (ret!=0)  
	{  
		return false;  
	}  
	//2.获取主机名   
	char hostname[256];  
	ret=gethostname(hostname,sizeof(hostname));  
	if (ret==SOCKET_ERROR)  
	{  
		return false;  
	}  
	//3.获取主机ip   
	HOSTENT* host=gethostbyname(hostname);  
	if (host==NULL)  
	{  
		return false;  
	}  
	//4.转化为char*并拷贝返回   
	strcpy(ip,inet_ntoa(*(in_addr*)*host->h_addr_list));  
	return true;  
}

#else
#include <sys/socket.h>			//socket
#include <sys/ioctl.h>			//ioctl
#include <net/if.h>				//ifreq
#include <netinet/in.h>			//SIOCGIFADDR常量，ifconf
#include <unistd.h>   			//colse(socket) 需包含该函数
#include <arpa/inet.h>

#define MAX_PATH_MAX 512

int get_eth_address (struct in_addr *addr)
{
	struct ifreq req;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	int i;
	for(i=0; i<6; i++)
	{
		char szEth[5]={0};
		sprintf(szEth,"eth%d",i);
		strncpy (req.ifr_name, szEth, IFNAMSIZ);
		if (0 == ioctl(sock, SIOCGIFADDR, &req))
		{
			break;
		}
	}
	close(sock);

	if (6 == i)
	{
		return -1;
	}

	memcpy (addr, &((struct sockaddr_in *) &req.ifr_addr)->sin_addr, sizeof (struct in_addr));

	return 0;
}

int GetLocalAddress (char *szLocalAddr)
{
	struct in_addr addr;

	get_eth_address(&addr);

	char *szAddr = inet_ntoa(addr);
	memcpy(szLocalAddr, szAddr, strlen(szAddr));
	return 0;
}

#endif

void EndpointToIpPort(const udp::endpoint &endpoint, string &strIp, unsigned int &dwPort)
{
	strIp = endpoint.address().to_string();
	dwPort = endpoint.port();
}

string EndpointToString(const udp::endpoint &endpoint)
{	
	string strIp = endpoint.address().to_string();
	unsigned int dwPort = endpoint.port();
	char szIpPort[32] = {0};
	sprintf(szIpPort, "%s:%d", strIp.c_str(), dwPort);
	return szIpPort;
}

udp::endpoint StringToEndpoint(const string &strIpPort)
{
	char szIp[16] = {0};
	unsigned int dwPort;
	sscanf(strIpPort.c_str(), "%[^:]:%d", szIp, &dwPort);
	udp::endpoint endpoint(boost::asio::ip::address::from_string(szIp), dwPort);
	return endpoint;
}

udp::endpoint IpPortToEndpoint(const string &strIp, unsigned int dwPort)
{
	udp::endpoint endpoint(boost::asio::ip::address::from_string(strIp), dwPort);
	return endpoint;
}

string IpPortToString(const string &strIp, unsigned int dwPort)
{
	char szIpPort[32] = {0};
	sprintf(szIpPort, "%s:%d", strIp.c_str(), dwPort);
	return szIpPort;
}

void StringToIpPort(const string &strIpPort, string &strIp, unsigned int &dwPort)
{
	char szIp[16] = {0};
	sscanf(strIpPort.c_str(), "%[^:]:%d", szIp, &dwPort);
	strIp = szIp;
}


//获取ServiceID拼接字符串
string GetServiceIdString(vector<unsigned int> dwVecServiceId)
{
	unsigned int dwIndex = 0;
	string szServiceId;
	while(dwIndex < dwVecServiceId.size())
	{
		char szBuf[36]={0};
		snprintf(szBuf,sizeof(szBuf),"%u",dwVecServiceId[dwIndex]);
		szServiceId+=szBuf;	
		if (dwIndex+1!=dwVecServiceId.size())
		{
			szServiceId+=",";
		}
		dwIndex++;
	}
	return szServiceId;
}


