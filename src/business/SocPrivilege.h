#ifndef _SOC_PRIVILEGE_H_
#define _SOC_PRIVILEGE_H_
#include <vector>
#include <string>

using std::string;
using std::vector;
typedef struct stServicePrivilege
{
	unsigned int dwServiceId;
	vector<unsigned int> vecMsgId;
	
	int operator <(const stServicePrivilege &p)const
	{
		 return (this->dwServiceId < p.dwServiceId);
	}
	bool operator==(unsigned int nKey)const
    {
          return (this->dwServiceId == nKey);
    }
	stServicePrivilege(unsigned int _svrId=0):dwServiceId(_svrId){}
}SServicePrivilege;

class CSocPrivilege
{
public:
	void SetPrivilege(const string &strPrivilege);
	bool IsOperate(unsigned int dwSeiceId, unsigned int dwMsgId);
	void Dump();
private:
	vector<SServicePrivilege> vecPrivilege;
};
#endif
