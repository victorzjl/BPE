#include "ISapStack.h"
#include "ISapAgent.h"
#include "TimerManager.h"
#include "SessionManager.h"
#include "TestLogHelper.h"
#include "SapLogHelper.h"
#include "SapAdminCommand.h"
#include <signal.h>


#define SAPTOOL_VERSION "1.1.2"

bool process_arg(int argc, char* argv[])
{

	char c;
	while((c = getopt (argc,argv,"v")) != -1 )
	{
		switch (c)
		{
			case 'v':
				printf("SapTool version: %s\n",SAPTOOL_VERSION);
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

	XLOG_INIT("log.properties",true);
	XLOG_REGISTER(SAP_STACK_MODULE,"sap_stack");
	XLOG_REGISTER(TEST_MODULE,"sap_bussiness");
	XLOG_REGISTER(SAPPER_STACK_MODULE,"sap_250ms");

	sigset_t new_mask;
	sigfillset(&new_mask);
	sigset_t old_mask;
	pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

	CSessionManager::Instance()->LoadClientSetting("./client.xml");
	CSessionManager::Instance()->LoadServerSetting("./server.xml");
	CSapAdminCommand::Instance()->LoadManagerSetting("./manager.xml");

	GetSapStackInstance()->Start();
	GetSapAgentInstance()->SetCallback(CSessionManager::Instance());
	CSessionManager::Instance()->Start();
	CSessionManager::Instance()->StartServer();
	CSessionManager::Instance()->StartClient();
	pthread_sigmask(SIG_SETMASK, &old_mask, 0);

	char c;
	while(1)
	{
		sleep(10000);
	}
}

