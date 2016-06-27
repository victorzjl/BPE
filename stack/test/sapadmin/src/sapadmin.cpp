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
#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include "AdminSession.h"
#include "AdminLogHelper.h"
#include "SapLogHelper.h"
#include "SapStack.h"
#include "SapAgent.h"
#include <stdlib.h>

using std::string;
#define SAP_ADMIN_VERSION "1.0.1"



#ifndef _WIN32  
#include <termio.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
     
 int getch(void)
 {
         struct termios tm, tm_old;
         int fd = STDIN_FILENO, c;
         if(tcgetattr(fd, &tm) < 0)
                 return -1;
         tm_old = tm;
         cfmakeraw(&tm);
         if(tcsetattr(fd, TCSANOW, &tm) < 0)
                 return -1;
         c = fgetc(stdin);
         if(tcsetattr(fd, TCSANOW, &tm_old) < 0)
                 return -1;
         return c;
 }
     
#else                            //WIN32 platform
#include <conio.h>
#endif
     
     
#define MAX_PASSWD_LEN   20
#define BACKSPACE 8
#define ENTER     13
#define ALARM     7
     
void GetPasswd(char *szPass)
 {
     int i=0, ch;
     while((ch = getch())!= -1 && ch != ENTER)
     {
         if(i == MAX_PASSWD_LEN && ch != BACKSPACE)
         {
             putchar(ALARM);
             continue;
         }
         if(ch == BACKSPACE)
         {
             if(i==0)
             {
                 putchar(ALARM);
                 continue;
             }
             i--;
             putchar(BACKSPACE);
             putchar(' ');
             putchar(BACKSPACE);
         }
         else
         {
             szPass[i] = ch;
             putchar('*');
             i++;
         }
     }

     if(ch == -1)
     {
         while(i != -1)
         {
             szPass[i--] = '\0';
         }
         return;
     }
     szPass[i]='\0';
     printf("\n");
     return;
 }


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
static void StopStack()
{
     CSapStack::Stop();
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
    CSapStack::Start();
    atexit(StopStack);

/*login server*/
    int nPort=atoi(argv[2]);
    CAdminSession *session=new CAdminSession;
    CSapAgent::Instance()->SetCallback(session);
    session->Start();    
    
/*input username & password*/
    string strName;
    char szPass[MAX_PASSWD_LEN+1];
    
    printf("Username: ");
    getline(std::cin,strName);
    
    printf("Password: ");
    fflush( stdout );
    GetPasswd(szPass);
    string strCommand;
    
/*starting...*/
    session->OnLoginServer(argv[1],nPort,strName,szPass);
    printf("Sap>: help\n");
    while(getline(std::cin,strCommand))
    {
        if(strCommand=="q")
            exit(0);
        
        if(strCommand.length()!=0)
        {
            session->OnSendRequest(strCommand);
        }
        else
        {
            printf("Sap>: ");
            fflush( stdout );
        }
    }
    return 0;
    
}

