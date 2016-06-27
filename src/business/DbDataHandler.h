#ifndef __DB_DATA_HANDLER_H__
#define __DB_DATA_HANDLER_H__
#include <string>
#include <vector>

using std::vector;
using std::string;

class IConfigureService;
class CDbDataHandler
{
    typedef IConfigureService *(*FnCreateObj)();
    typedef void  (*FnDestroyObj)(IConfigureService*);
    struct SConfServcie
    {
        IConfigureService *pServiceObj;
        FnCreateObj fnCreateObj;
        FnDestroyObj fnDestroyObj;
        void *pLibHandle;
    };
	
	typedef vector<SConfServcie> plugin_container;
    typedef plugin_container::iterator plugin_iterator;
    typedef plugin_container::const_iterator plugin_citerator;
	
public:
    static CDbDataHandler* GetInstance();
public:    
	int LoadPlugins();
	int LoadData()const;	
    int FastLoadData()const; 
	void Dump();
private:
    CDbDataHandler(){}
    CDbDataHandler(const CDbDataHandler& rhd){}
    CDbDataHandler& operator=(const CDbDataHandler& rhd){return *this;}
private:
	int LoadSO(const char* pSoObj, SConfServcie &sService);
private:
    static CDbDataHandler* pInstance_;
	vector<SConfServcie> m_confServcies;
	static const char* SERVICE_PATH;
};

#endif


