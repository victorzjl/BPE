#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H
#include<string>
#include "PluginRegister.h"	
#include "HTInfo.h"
using std::string;


#ifdef WIN32
	#define CCH_PATH_SEP	'\\'
	#define CST_PATH_SEP	"\\"
#else
	#define CCH_PATH_SEP	'/'
	#define CST_PATH_SEP	"/"	
#endif

class CPluginManager
{
public:
	static CPluginManager* GetInstance(){ return m_pInstance;}
	int InitPlugin(const string &strPath);
	
private:
	CPluginManager();
	~CPluginManager();
	int LoadAllPlugins( const string &strPath );
	int LoadPlugins(const char *szSoName);
private:
	static CPluginManager* m_pInstance;
	
	class CGarbo
    {
    public:
        ~CGarbo()
        {  
			//printf("~CPluginManager\n");
            if (CPluginManager::m_pInstance)
                delete CPluginManager::m_pInstance;
        }
    };
    static CGarbo Garbo;
	
};

#endif
