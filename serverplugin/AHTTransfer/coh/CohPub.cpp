#include <stdio.h>
#include <stdlib.h>
#include "CohPub.h"
#include <boost/algorithm/string.hpp>
#include "vector"
void ParseHttpUrl(const string &strUrl,SHttpServerInfo & oUrl)
{
    char szHost[2048]={0};
    char szPath[2048]={'/','\0'};
    
    oUrl.dwPort=80;
	if(boost::istarts_with(strUrl,"https"))
	{
		if (sscanf (strUrl.c_str(), "%*[HTTPShttps]://%[^:/]:%u/%2000s", szHost, &oUrl.dwPort, szPath)!=3)
		{
			if (sscanf (strUrl.c_str(), "%*[HTTPShttps]://%[^:/]:%u", szHost, &oUrl.dwPort)!=2)
			{      
				if (sscanf (strUrl.c_str(), "%*[HTTPShttps]://%[^:/]/%s", szHost, szPath)!=2)
				{
					string strHost=szHost;
					if (strHost.find('?')!=string::npos)
					{
						//Top level URL handle
						std::vector<string> vecKeyValue;
						split(vecKeyValue, strHost, boost::is_any_of("?"), boost::algorithm::token_compress_on);
						if (vecKeyValue.size()==2)
						{
							if (strHost.find(':')!=string::npos)
							{
								std::vector<string> vecKeyValue2;
								split(vecKeyValue2, vecKeyValue[0], boost::is_any_of(":"), boost::algorithm::token_compress_on);
								if (vecKeyValue2.size()==2)
								{
									strcpy(szHost,vecKeyValue2[0].c_str());
									oUrl.dwPort=atoi(vecKeyValue2[1].c_str());
								}
							}
							else
							{
								strcpy(szHost,vecKeyValue[0].c_str());
							}
							strcpy(szPath,"?");   //must GET /?appid=1
							strcpy(szPath,vecKeyValue[1].c_str());
						}
					}
				}
			}
		}
	}
	else
	{
		if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u/%2000s", szHost, &oUrl.dwPort, szPath)!=3)
		{
			if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]:%u", szHost, &oUrl.dwPort)!=2)
			{      
				if (sscanf (strUrl.c_str(), "%*[HTTPhttp]://%[^:/]/%s", szHost, szPath)!=2)
				{
					string strHost=szHost;
					if (strHost.find('?')!=string::npos)
					{
						//Top level URL handle
						std::vector<string> vecKeyValue;
						split(vecKeyValue, strHost, boost::is_any_of("?"), boost::algorithm::token_compress_on);
						if (vecKeyValue.size()==2)
						{
							if (strHost.find(':')!=string::npos)
							{
								std::vector<string> vecKeyValue2;
								split(vecKeyValue2, vecKeyValue[0], boost::is_any_of(":"), boost::algorithm::token_compress_on);
								if (vecKeyValue2.size()==2)
								{
									strcpy(szHost,vecKeyValue2[0].c_str());
									oUrl.dwPort=atoi(vecKeyValue2[1].c_str());
								}
							}
							else
							{
								strcpy(szHost,vecKeyValue[0].c_str());
							}
							strcpy(szPath,"?");   //must GET /?appid=1
							strcat(szPath,vecKeyValue[1].c_str());
						}
					}
				}
			}
		}
	}
    oUrl.strIp= szHost;
    oUrl.strPath=szPath;
}


