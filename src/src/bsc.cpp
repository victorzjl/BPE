#include "SapStack.h"
#include "SapServer.h"
#include <cstdio>
#include "SapLogHelper.h"
#include "AsyncLogHelper.h"
#include "AsyncLogThread.h"

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

#include "XmlConfigParser.h"
#include "CohStack.h"
#include "CohLogHelper.h"
#include "CohServerImp.h"
#include "SdkLogHelper.h"
#include "AvenueServiceConfigs.h"
#include "CohClientImp.h"
#include "FunctionParse.h"
#define SPS_VERSION "1.0.0.8"
#define MAX_PATH_MAX 512

using namespace sdo::coh;
char* GetExeName()
{
	static char buf[MAX_PATH_MAX] = {0};
	int rslt   =   readlink("/proc/self/exe",   buf,   MAX_PATH_MAX);
	if(rslt < 0 || rslt >= MAX_PATH_MAX)
	{
		return   NULL;
	}
	buf[rslt]   = '\0';
	return   buf;
}

string GetCurrentPath()
{
	static char buf[MAX_PATH_MAX];
	strcpy(buf, GetExeName());
	char* pStart    =   buf + strlen(buf) - 1;
	char* pEnd      =   buf;

	if(pEnd < pStart)
	{
		while(pEnd !=  --pStart)
		{
			if('/' == *pStart)
			{
				*++pStart = '\0';
				break;
			}
		}
	}
	return buf;
}

void run()
{
    XLOG_INIT((GetCurrentPath()+"log.properties").c_str(), true);
    XLOG_REGISTER(SAP_STACK_MODULE, "sap");
    XLOG_REGISTER(COH_STACK_MODULE, "coh");
    
	XLOG_REGISTER(SOC_AUDIT_MODULE, "soc_audit");
    XLOG_REGISTER(SOS_AUDIT_MODULE, "sos_audit");
    XLOG_REGISTER(SOS_STATIS_MODULE, "sos_stat");
    XLOG_REGISTER(SOC_PLATFORM_MODULE, "soc_request");

    XLOG_REGISTER(SELF_MODULE, "self_check");    
    XLOG_REGISTER(SDK_MODULE, "sdk_config");

    SS_XLOG(XLOG_DEBUG,"running...\n");

    CXmlConfigParser oConfig;
    if(oConfig.ParseFile(GetCurrentPath()+"config.xml")!=0)
    {
        SS_XLOG(XLOG_ERROR, "parse config error:%s\n", 
                oConfig.GetErrorMessage().c_str());
        return ;
    }    

    
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

    CCohStack::Start();
	CohServerImp m_oServerImp;
	if(m_oServerImp.Start(oConfig.GetParameter("CohPort", 5237))!=0)
	{
		SS_XLOG(XLOG_ERROR, "run(), coh can't bind port!\n");
		return;
	}
	extern_fun_manager::GetInstance();//void multi-thread create object, new in main thread.
    CSapStack::Instance()->Start(oConfig.GetParameter("ThreadNum", 1));
    /*read sos & supersoc config*/
     
    if(CSapStack::Instance()->LoadConfig()==-1)
    {
        SS_XLOG(XLOG_ERROR, "load config error\n");
        return;
    }
	CSapStack::isLocalPrivilege=1;

     if(CSapStack::Instance()->LoadServiceConfig()==-1)
    {
        SS_XLOG(XLOG_WARNING, "load compose service config error\n");
    }
     if(CSapStack::Instance()->LoadVirtualService() == -1)
    {
        SS_XLOG(XLOG_WARNING, "load virtual service error\n");
    }
    if(CSapServer::Instance()->StartServer(oConfig.GetParameter("SapPort", 9022))!=0)
    {
		SS_XLOG(XLOG_ERROR, "run(), sap server start fail!\n");
		return;
	}

    CCohClientImp::GetInstance()->Init(
            oConfig.GetParameter("ReportUrl","http://127.0.0.1/"),
            oConfig.GetParameter("DetailReportUrl","http://127.0.0.1/"),
            oConfig.GetParameter("DgsUrl","http://127.0.0.1/"),
            oConfig.GetParameter("ErrReportUrl","http://127.0.0.1/"));
    
	CAsyncLogThread::GetInstance()->Start();
    
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);


    while (1)
    {
       sleep(1000);
    }
    /*
    char c;
    while(c=getchar())
    {
        if(c=='q')
            exit(-1);
        else if(c=='p')
            {
            CSapStack::Instance()->Dump();
            }
        
        else if(c!='\n')
                   SS_XLOG(XLOG_WARNING,"can not regnize char [%c]\n",c);
    }*/
}

bool process_arg(int argc, char* argv[])
{

        char c;
        while((c = getopt (argc,argv,"v")) != -1 )
        {
                switch (c)
                {
                        case 'v':
                                printf("SAP PROXY Server version: %s\n",SPS_VERSION);
                                printf("Build time: %s %s\n",__TIME__,__DATE__);
                                return false;
                        default:
                                printf("Use -v to print Version infomation\n");
                                return false;
                }
        }
        return true;
}

int main(int argc, char* argv[])
{
        if(!process_arg(argc,argv))
                return 0;

        pid_t chId,rebornId;

        chId = fork();
        if (chId != 0)
        {
                XLOG_INIT((GetCurrentPath()+"log.properties").c_str(), true);
                XLOG_REGISTER(SAP_STACK_MODULE, "sap");                
                XLOG_REGISTER(COH_STACK_MODULE, "coh");
                
            	XLOG_REGISTER(SOC_AUDIT_MODULE, "soc_audit");
                XLOG_REGISTER(SOS_AUDIT_MODULE, "sos_audit");
                XLOG_REGISTER(SOS_STATIS_MODULE, "sos_stat");
                XLOG_REGISTER(SOC_PLATFORM_MODULE, "soc_request");

                XLOG_REGISTER(SELF_MODULE, "self_check");                
                XLOG_REGISTER(SDK_MODULE, "sdk_config");
        }
        else
        {
                run();
                exit(0);
        }
        while (1)
        {
                struct timeval_a tBefore,tAfter,tInterval;
    		    gettimeofday_a(&tBefore,0);
                rebornId = wait(NULL);
                gettimeofday_a(&tAfter,0);
                timersub(&tAfter,&tBefore,&tInterval);
                if(tInterval.tv_sec+tInterval.tv_usec/1000000<1)
                {
                    SS_XLOG(XLOG_FATAL,"child process execute exception, parent exit! interval[%d]\n",tInterval.tv_sec+tInterval.tv_usec/1000000);
                    return -1;
                }
                
                sleep(1);
                SS_XLOG(XLOG_FATAL,"--------reboot!!!--------\n");
                if (rebornId == chId)
                {
                        chId = fork();
                        if (chId == 0)
                        {
                                run();
                                exit(0);
                        }
                }
        }
      
        return 0;
}




