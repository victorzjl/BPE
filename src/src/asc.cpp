#include "SapStack.h"
#include "SapServer.h"
#include <cstdio>
#include "SapLogHelper.h"
#include "AsyncLogHelper.h"
#include "AsyncLogThread.h"
#include "MapMemory.h"

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

#include "XmlConfigParser.h"
#include "CohStack.h"
#include "CohLogHelper.h"
#include "CohServerImp.h"

#include "SdkLogHelper.h"
//#include "AvenueServiceConfigs.h"
#include "CohClientImp.h"
#include "DbDataHandler.h"
#include "JobTimerThread.h"
#include "FunctionParse.h"
#define RELEASE_MODLE 1 
#define MAX_PATH_MAX 512

static char s_szConfigName[MAX_PATH_MAX]="config.xml";
static char s_szLogName[MAX_PATH_MAX]="log.properties";
extern const char* ASC_VERSION;

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

static void signal_termination(int nSignal)
{
    SS_XLOG(XLOG_FATAL,"--------stop[%d]--------\n", nSignal);
    CSapServer::Instance()->StopServer();
    CSapStack::Instance()->Stop();
    time_t tBegin = time(NULL);

    do
    {
        usleep(10000);
    }while ((time(NULL) - tBegin < 6 ) && !CSapStack::Instance()->IsClosed());

    do
    {
        CAsyncLogThread::GetInstance()->OnServiceStop();
        usleep(10000);
    }while ((time(NULL) - tBegin < 3 ) && !CAsyncLogThread::GetInstance()->isFlushAll());

    SS_XLOG(XLOG_FATAL,"--------stop end--------\n");
    exit(0);
}


void run(int rebootFlag=0)
{

    XLOG_INIT((GetCurrentPath()+s_szLogName).c_str(), true);
    XLOG_REGISTER(SAP_STACK_MODULE, "sap");
    XLOG_REGISTER(COH_STACK_MODULE, "coh");

	XLOG_REGISTER(SAP_VIRTUAL_MODULE, "virtual");
    XLOG_REGISTER(SAP_STORE_FORWARD_MODULE, "store_forward");

	XLOG_REGISTER(SOC_AUDIT_MODULE, "soc_audit");
    XLOG_REGISTER(SOS_AUDIT_MODULE, "sos_audit");
    XLOG_REGISTER(SOS_STATIS_MODULE, "sos_stat");
    XLOG_REGISTER(SCS_AUDIT_MODULE, "scs_audit");
    XLOG_REGISTER(REQ_STATIS_MODULE, "req_stat");
    XLOG_REGISTER(SOC_PLATFORM_MODULE, "soc_platform");
    XLOG_REGISTER(REQ_CONN_MODULE, "req_conn");
    XLOG_REGISTER(DC_REPORT_MODULE, "request_audit_sync");
    XLOG_REGISTER(SOS_DATA_MODULE, "sos_data");

    XLOG_REGISTER(SELF_MODULE, "self_check");
    XLOG_REGISTER(SDK_MODULE, "sdk_config");

    if (rebootFlag != 0)
    {
        SS_XLOG(XLOG_FATAL,"--------reboot!!!--------\n");
    }
    SS_XLOG(XLOG_DEBUG,"running...\n");
    string strConfigPath = GetCurrentPath()+s_szConfigName;
    CXmlConfigParser oConfig;
    if(oConfig.ParseFile(strConfigPath)!=0)
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
    SS_XLOG(XLOG_ERROR,"CCohStack::Start success\n");

	CohServerImp m_oServerImp;
    int nCohPort = oConfig.GetParameter("CohPort", 9095);
	if(m_oServerImp.Start(nCohPort)!=0)
	{
		SS_XLOG(XLOG_ERROR, "run(), coh can't bind port!\n");
		return;
	}
    SS_XLOG(XLOG_ERROR,"CohServerImp::Start success\n");

    string strReportUrl = oConfig.GetParameter("ReportUrl","http://api.monitor.sdo.com/stat_more_actions.php");
    string strDetailReportUrl = oConfig.GetParameter("DetailReportUrl","http://api.monitor.sdo.com/stat_pages.php");
    string strDgsUrl = oConfig.GetParameter("DgsUrl","http://127.0.0.1/");
    string strErrReportUrl = oConfig.GetParameter("ErrReportUrl","http://api.monitor.sdo.com/error_report.php");
    CCohClientImp::GetInstance()->Init(strReportUrl, strDetailReportUrl, strDgsUrl, strErrReportUrl);

	CAsyncLogThread::GetInstance()->Start();
    SS_XLOG(XLOG_ERROR,"CAsyncLogThread::Start success\n");
//	    if (CAsyncVirtualService::Instance()->Load() != 0)
//	    {
//	        SS_XLOG(XLOG_WARNING, "load async virtual service error\n");
//	    }
	extern_fun_manager::GetInstance();//void multi-thread create object, new in main thread.
    int nThreadNum = oConfig.GetParameter("ThreadNum", 8);
    CSapStack::Instance()->Start(nThreadNum);
    SS_XLOG(XLOG_ERROR,"CSapStack::Start success\n");

    /*read sos & supersoc config*/

    if(CSapStack::Instance()->LoadConfig(strConfigPath.c_str())==-1)
    {
        SS_XLOG(XLOG_ERROR, "load config error\n");
        return;
    }
    SS_XLOG(XLOG_ERROR,"CSapStack::LoadConfig success\n");

    if(CSapStack::Instance()->LoadServiceConfig()==-1)
    {
        SS_XLOG(XLOG_WARNING, "load service config error\n");
    }
    SS_XLOG(XLOG_ERROR,"CSapStack::LoadServiceConfig success\n");

    CSapStack::Instance()->LoadVirtualService();
    SS_XLOG(XLOG_ERROR,"CSapStack::LoadVirtualService success\n");
    CJobTimerThread::GetInstance()->Start();
    CDbDataHandler* pInstance = CDbDataHandler::GetInstance();
    if(0 == pInstance->LoadPlugins())
    {
        pInstance->FastLoadData();
        if (0 == pInstance->LoadData())
        {
            SS_XLOG(XLOG_ERROR,"Load Data success\n");
        }
    }
    
    CSapStack::Instance()->LoadAsyncVirtualService();
    SS_XLOG(XLOG_ERROR,"CSapStack::LoadAsyncVirtualService success\n");
	
	CSapStack::Instance()->LoadAsyncVirtualClientService();
	SS_XLOG(XLOG_ERROR,"CSapStack::LoadAsyncVirtualClientService success\n");
	
    int nSapPort = oConfig.GetParameter("SapPort", 8095);
    if(CSapServer::Instance()->StartServer(nSapPort)!=0)
    {
        SS_XLOG(XLOG_ERROR, "run(), sap server start fail!\n");
		exit(0);
        return;
    }
    SS_XLOG(XLOG_ERROR,"CSapServer::StartServer success\n");

	if(CSapStack::isAsc!=0)
	{
		string strCountList = oConfig.GetParameter("MapQCountList", "40960,20480,10240,5120");
		unsigned nCountList[4] = {40960,20480,10240,5120};
		sscanf(strCountList.c_str(), "%d,%d,%d,%d",
			&nCountList[0], &nCountList[1], &nCountList[2], &nCountList[3]);
		if (!CMapMemory::Instance().Init(
			oConfig.GetParameter("MapQFile", "ascmapfile"),
			boost::bind(&CSapStack::HandleRequest, CSapStack::Instance(), _1, _2),
			nCountList))
		{
			SS_XLOG(XLOG_ERROR, "run(), init mapping file fail!\n");
			return;
		}
        SS_XLOG(XLOG_ERROR,"CMapMemory::Init success\n");
	}

    SS_XLOG(XLOG_ERROR, "\n" \
                "------------------------------- Start Success ------------------------------------\n" \
                "        CohPort                 %d\n" \
                "        SapPort                 %d\n" \
                "        ThreadNum               %d\n" \
                "        IsAsc                   %d\n" \
                "        ReportUrl               %s\n" \
                "        DetailReportUrl         %s\n" \
                "        DgsUrl                  %s\n" \
                "        ErrReportUrl            %s\n" \
                "        LogType                 %d\n" \
                "----------------------------------------------------------------------------------\n",
                nCohPort, nSapPort, nThreadNum, CSapStack::isAsc, strReportUrl.c_str(),
                strDetailReportUrl.c_str(), strDgsUrl.c_str(), strErrReportUrl.c_str(),
                CSapStack::nLogType);
    
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);
  //  signal(SIGTERM, signal_termination);



#if RELEASE_MODLE
    while (1)
    {
       sleep(1000);
    }
#else
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
    }
#endif
}

bool process_arg(int argc, char* argv[])
{

    char c;
    while((c = getopt (argc,argv,"v,h")) != -1 )
    {
        switch (c)
        {
            case 'v':
                    printf("BPE version: %s\n",ASC_VERSION);
                    printf("Build time: %s %s\n",__TIME__,__DATE__);
                    return false;
            case 'h':
            {
                snprintf(s_szConfigName, sizeof(s_szConfigName)-1, "%s", "hd_config.xml");
                snprintf(s_szLogName, sizeof(s_szLogName)-1, "%s", "hd_log.properties");
                return true;
            }
            default:
                    printf("Use -v to print Version infomation\n");
                    return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
#if RELEASE_MODLE
    if(!process_arg(argc,argv))
            return 0;

    pid_t chId,rebornId;

    chId = fork();
    if (chId != 0)
    {
            //XLOG_INIT((GetCurrentPath()+s_szLogName).c_str(), true);
            //XLOG_REGISTER(SAPPER_STACK_MODULE, "sapper");
    }
    else
    {
            run();
            exit(0);
    }
    struct timeval_a tBefore,tAfter,tInterval;
    gettimeofday_a(&tBefore,0);
    while (1)
    {
            rebornId = wait(NULL);
            gettimeofday_a(&tAfter,0);
            timersub(&tAfter,&tBefore,&tInterval);
            if(tInterval.tv_sec+tInterval.tv_usec/1000000<1)
            {
                XLOG_INIT((GetCurrentPath()+s_szLogName).c_str(), true);
                XLOG_REGISTER(SAP_STACK_MODULE, "sap");
                SS_XLOG(XLOG_FATAL,"child process execute exception, parent exit! interval[%d]\n",tInterval.tv_sec+tInterval.tv_usec/1000000);
                return -1;
            }

            sleep(1);
            //SP_XLOG(XLOG_FATAL,"--------reboot!!!--------\n");
            //if (rebornId == chId)
            {
                    chId = fork();
                    if (chId == 0)
                    {
                            run(-1);
                            exit(0);
                    }
            }
    }
#else
    run();
#endif
    return 0;
}




