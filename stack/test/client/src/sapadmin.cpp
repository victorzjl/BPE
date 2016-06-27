/*
 * =====================================================================================
 *
 *       Filename:  sapadmin.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/03/09 09:46:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZongJinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string>
#include <iostream>
#include "AdminSession.h"
#include "AdminLogHelper.h"
#include "SapLogHelper.h"
#include "ISapStack.h"
#include "ISapAgent.h"
#include <stdlib.h>

using std::string;
#define SAP_ADMIN_VERSION "1.0.1"

bool process_arg(int argc, char* argv[])
{

	char c;
	while((c = getopt (argc,argv,"v")) != -1 )
	{
		switch (c)
		{
			case 'v':
				printf("sap admin version: %s\n",SAP_ADMIN_VERSION);
				printf("Build time: %s %s\n",__TIME__,__DATE__);
				return false;
			default:
				printf("Use -v to print Version infomation\n");
                printf("Use sapadmin ip port\n");
				return false;
		}
	}
	return true;			
}

int main(int argc, char*argv[])
{
    if(!process_arg(argc,argv))
		return 0;
    
/*arg valid*/
    if(argc<3)
    {
        printf("Usage: sapadmin ip port\n");
        return -1;
    }
/*set log*/
    XLOG_INIT("log.properties",true);
    XLOG_REGISTER(SAP_STACK_MODULE,"sap_stack");
    XLOG_REGISTER(ADMIN_MODULE,"sap_bussiness");
    A_XLOG(XLOG_DEBUG,"test\n");
    
/*stack start*/
    GetSapStackInstance()->Start(3);

/*login server*/
    int nPort=atoi(argv[2]);
    CAdminSession *session=new CAdminSession;
    GetSapAgentInstance()->SetCallback(session);
    session->Start();    
    
/*starting...*/
    session->OnLoginServer(argv[1],nPort);
    while(1)
    {
        sleep(1000);
    }
    return 0;
    
}

