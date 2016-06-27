#include "SocPrivilege.h"
#include "SapLogHelper.h"
#include <boost/algorithm/string.hpp> 

#define ALL_SERVICE_ID 0xFFFFFFFF

void CSocPrivilege::SetPrivilege(const string &strPrivilege)
{
    SS_XLOG(XLOG_DEBUG,"CSocPrivilege::%s,strPrivilege[%s]\n",__FUNCTION__,strPrivilege.c_str());

	if (atoi(strPrivilege.c_str()) == -1)
	{
		SServicePrivilege allPrivilege;
		allPrivilege.dwServiceId = ALL_SERVICE_ID;
		vecPrivilege.push_back(allPrivilege);
		return;
	}

	vector<string> vecService;
    boost::algorithm::split( vecService, strPrivilege,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on); 
    vector<string>::const_iterator itrService;
    for(itrService=vecService.begin();itrService!=vecService.end();++itrService)
    {
        const string & strService=*itrService;
        SServicePrivilege obj;
        char szMsg[1024]={0};
        if(sscanf(strService.c_str(),"%u{%1023[^}]}",&(obj.dwServiceId),szMsg)==2)
        {
			if (atoi(szMsg) == -1)
			{
				//obj.vecMsgId.push_back(-1);
			}
			else
			{
	            vector<string> vecMsgId;
	            boost::algorithm::split( vecMsgId, szMsg,boost::algorithm::is_any_of("/"),boost::algorithm::token_compress_on); 

	            vector<string>::const_iterator itrMsgId;
	            for(itrMsgId=vecMsgId.begin();itrMsgId!=vecMsgId.end();++itrMsgId)
	            {
	                const string & strMsgId=*itrMsgId;
	                obj.vecMsgId.push_back(atoi(strMsgId.c_str()));
	            }
	            sort(obj.vecMsgId.begin(),obj.vecMsgId.end());
			}
        }
        vecPrivilege.push_back(obj);
    }
    sort(vecPrivilege.begin(),vecPrivilege.end());
}
bool CSocPrivilege::IsOperate(unsigned int dwSeiceId, unsigned int dwMsgId)
{
    SS_XLOG(XLOG_DEBUG,"CSocPrivilege::%s, seviceid[%u],msgid[%u]\n",__FUNCTION__,dwSeiceId,dwMsgId);

	if(std::binary_search(vecPrivilege.begin(), vecPrivilege.end(), SServicePrivilege(ALL_SERVICE_ID)))
		return true;

	vector<SServicePrivilege>::iterator itr=std::find( vecPrivilege.begin(), vecPrivilege.end(), dwSeiceId );
    if(itr==vecPrivilege.end())
        return false;

    SServicePrivilege &obj=*itr;
    if(obj.vecMsgId.size()==0)
        return true;

    if(std::binary_search( obj.vecMsgId.begin(), obj.vecMsgId.end(), dwMsgId ) == false)
        return false;

    return true;
}
void CSocPrivilege::Dump()
{
    string strPrivilege;
    vector<SServicePrivilege>::iterator itr;
    for(itr=vecPrivilege.begin();itr!=vecPrivilege.end();++itr)
    {
        const SServicePrivilege & obj=*itr;
        char szServiceId[16]={0};
        sprintf(szServiceId,"%d",obj.dwServiceId);
        strPrivilege+=szServiceId;
        if(obj.vecMsgId.size()>0)
        {
            strPrivilege+="{";
            vector<unsigned int>::const_iterator itrMsgId;
            for(itrMsgId=obj.vecMsgId.begin();itrMsgId!=obj.vecMsgId.end();++itrMsgId)
            {
                unsigned int dwMsgId=*itrMsgId;
                char szMsgId[16]={0};
                sprintf(szMsgId,"%d/",dwMsgId);
                strPrivilege+=szMsgId;
            }
            strPrivilege+="}";
        }
	strPrivilege+=",";
    }
    SS_XLOG(XLOG_NOTICE,"        privilege:%s\n",strPrivilege.c_str());
}

