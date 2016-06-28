#ifndef _COMMON_H_
#define _COMMON_H_

#include "detail/_time.h"
#include <string>
#include <vector>
#include <boost/asio.hpp>


using std::string;
using std::vector;
using namespace boost;
using boost::asio::ip::udp;

////////////////////////////////////////////////////////////////////////////////////////////
//网络相关
int GetLocalAddress(char *szLocalAddr);
void EndpointToIpPort(const udp::endpoint &endpoint, string &strIp, unsigned int &dwPort);
string EndpointToString(const udp::endpoint &endpoint);
udp::endpoint StringToEndpoint(const string &strIpPort);
udp::endpoint IpPortToEndpoint(const string &strIp, unsigned int dwPort);
string IpPortToString(const string &strIp, unsigned int dwPort);
void StringToIpPort(const string &strIpPort, string &strIp, unsigned int &dwPort);


////////////////////////////////////////////////////////////////////////////////////////////
//时间相关
string GetCurrenStringTime();
string TimevalToString(const timeval_a &tvTime);
float GetTimeInterval(const timeval_a &tStart);

/////////////////////////////////////////////////////////////////////////////////////////////
//string处理相关
void Replace(string &strSrc, const string &strOld, const string &strNew);
char* strlwr(char *str);

string GetServiceIdString(vector<unsigned int> dwVecServiceId);

#endif

