#include "CohServerImp.h"
#include "CohResponseMsg.h"
#include "CohLogHelper.h"
#include "AsyncLogThread.h"
#include "SapStack.h"
#include "DbDataHandler.h"


namespace sdo{
	namespace coh{

		void CohServerImp::OnReceiveRequest(void * identifer,const string &request,const string &remote_ip,unsigned int remote_port)
		{
			CS_XLOG(XLOG_DEBUG,"CohServerImp::%s,remote_ip[%s],remote_port[%d],\n",__FUNCTION__,remote_ip.c_str(),remote_port);

			CCohRequestMsg cohRequest;
			if(cohRequest.Decode(request)==0)
			{
				if(cohRequest.GetCommand()=="NotifyChanged.do")
				{
					OnNotifyChanged(identifer);
				}
                else if(cohRequest.GetCommand()=="NotifyLoadData.do")
				{
					OnNotifyLoadData(identifer);
				}
                else if(cohRequest.GetCommand()=="UninstallSO.do")
				{
                    string strName = cohRequest.GetAttribute("LibName");
                    if(strName.empty())
                    {
                        CCohResponseMsg msg;
    					msg.SetStatus(-1,"Get lib name failed");
    					DoSendResponse(identifer,msg.Encode());
                        return;
                    }
					OnUninstallSO(identifer,strName);
				}
                else if(cohRequest.GetCommand()=="InstallSO.do")
				{
                    string strName = cohRequest.GetAttribute("LibName");
                    if(strName.empty())
                    {
                        CCohResponseMsg msg;
    					msg.SetStatus(-1,"Get lib name failed");
    					DoSendResponse(identifer,msg.Encode());
                        return;
                    }
					OnInstallSO(identifer,strName);
				}
                else if(cohRequest.GetCommand()=="QuerySelfCheck.do")
				{
					OnQuerySelfCheck(identifer);
				}
                else if(cohRequest.GetCommand()=="QueryPluginVersion.do")
				{
					QueryPluginVersion(identifer);
				}
                else if(cohRequest.GetCommand()=="QueryConfigInfo.do")
                {
                    QueryConfigInfo(identifer);
                }
                else if(cohRequest.GetCommand()=="help.do")
                {
                    QueryHelp(identifer);
                }
				else
				{       	
					CS_XLOG(XLOG_WARNING,"CohServerImp::%s Command not found!\n",__FUNCTION__);
					CCohResponseMsg msg;
					msg.SetStatus(-1,"Not Found Command");
					DoSendResponse(identifer,msg.Encode());
				}
			}
			else
			{
				CCohResponseMsg msg;
				msg.SetStatus(-1,"Can't Parse Request");
				DoSendResponse(identifer,msg.Encode());
			}
		}
        
		void CohServerImp::OnNotifyChanged(void * identifer)
		{
			CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
			CCohResponseMsg ResMsg;
			if(0==CSapStack::Instance()->LoadConfig())
			{
                ResMsg.SetStatus(0,"OK");
			}
            else
            {
                ResMsg.SetStatus(-1,"Load failed");
            }
			
			DoSendResponse(identifer,ResMsg.Encode());
		}
        
        void CohServerImp::OnNotifyLoadData(void * identifer)
		{
			CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
			CCohResponseMsg ResMsg;
			if(0==CDbDataHandler::GetInstance()->LoadData())
			{
                ResMsg.SetStatus(0,"OK");
			}
            else
            {
                ResMsg.SetStatus(-1,"Load failed");
            }			
			DoSendResponse(identifer,ResMsg.Encode());
		}

        void CohServerImp::OnUninstallSO(void * identifer, const string& strName)
		{
			CS_XLOG(XLOG_DEBUG,"CohServerImp::%s,lib[%s]\n",__FUNCTION__,strName.c_str());
			CCohResponseMsg ResMsg;
            int nCode = CSapStack::Instance()->UnInstallVirtualService(strName);
			if(0==nCode)
			{
                ResMsg.SetStatus(0,"OK");
			}    
            else if(1==nCode)
            {
                ResMsg.SetStatus(-1,strName+" Not Found");
            }
            else
            {
                ResMsg.SetStatus(-1,"Uninstall failed");
            }
			DoSendResponse(identifer,ResMsg.Encode());
		}
        
        void CohServerImp::OnInstallSO(void * identifer, const string& strName)
		{
			CS_XLOG(XLOG_DEBUG,"CohServerImp::%s,lib[%s]\n",__FUNCTION__,strName.c_str());
			CCohResponseMsg ResMsg;
            int nCode = CSapStack::Instance()->InstallVirtualService(strName);
			if(nCode==0)
			{
                ResMsg.SetStatus(0,"OK");
			}
            else if(nCode == -2)
            {
                ResMsg.SetStatus(-1,strName+" has loaded, uninstall first");
            }	
            else
            {
                ResMsg.SetStatus(-1,"Install failed");
            }
			DoSendResponse(identifer,ResMsg.Encode());
		}
        
        void CohServerImp::OnQuerySelfCheck(void * identifer)
        {
            CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
            string strCheck=CAsyncLogThread::GetInstance()->GetSelfCheckData();
            
            char bufLength[48]={0};
	        sprintf(bufLength,"Content-Length: %d\r\n\r\n",strCheck.length());
            
            string strResponse=string("HTTP/1.0 200 OK\r\n" \
                "Content-Type: text/html;charset=utf-8\r\n" \
                "Server: COH Server/1.0\r\n") +bufLength+strCheck.c_str();
            
            DoSendResponse(identifer,strResponse);
        }
        
        void CohServerImp::QueryPluginVersion(void * identifer)
        {
            CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
            string strInfo=CSapStack::Instance()->GetPluginInfo();
            
            char bufLength[48]={0};
	        sprintf(bufLength,"Content-Length: %d\r\n\r\n",strInfo.length());
            
            string strResponse=string("HTTP/1.0 200 OK\r\n" \
                "Content-Type: text/html;charset=utf-8\r\n" \
                "Server: COH Server/1.0\r\n")+bufLength+strInfo;
            
            DoSendResponse(identifer,strResponse);
        }

        void CohServerImp::QueryConfigInfo(void *identifer)
        {
            CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
            string strInfo=CSapStack::Instance()->GetConfigInfo();

            char bufLength[48]={0};
            sprintf(bufLength,"Content-Length: %d\r\n\r\n",strInfo.length());

            string strResponse=string("HTTP/1.0 200 OK\r\n" \
                "Content-Type: text/html;charset=utf-8\r\n" \
                "Server: COH Server/1.0\r\n")+bufLength+strInfo;

            DoSendResponse(identifer,strResponse);
        }

        void CohServerImp::QueryHelp(void *identifer)
        {
            CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
            string strInfo = "<table>" \
                "<tr><th>Command</th><th>Parameter</th></tr>" \
                "<tr><td>NotifyChanged.do</td><td>[NULL]</td></tr>" \
                "<tr><td>OnNotifyLoadData.do</td><td>[NULL]</td></tr>" \
                "<tr><td>UninstallSO.do</td><td>LibName</td></tr>" \
                "<tr><td>InstallSO.do</td><td>LibName</td></tr>" \
                "<tr><td>QuerySelfCheck.do</td><td>[NULL]</td></tr>" \
                "<tr><td>QueryPluginVersion.do</td><td>[NULL]</td></tr>" \
                "<tr><td>QueryConfigInfo.do</td><td>[NULL]</td></tr>" \
                "</table>";

            char bufLength[48]={0};
            sprintf(bufLength,"Content-Length: %d\r\n\r\n",strInfo.length());

            string strResponse=string("HTTP/1.0 200 OK\r\n" \
                "Content-Type: text/html;charset=utf-8\r\n" \
                "Server: COH Server/1.0\r\n")+bufLength+strInfo;

            DoSendResponse(identifer,strResponse);
        }
	}
}
