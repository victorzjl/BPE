#ifndef  BPE_VALUE_ENCODER_H
#define BPE_VALUE_ENCODER_H
#include <string>
#include<map>
using std::map;
using std::string;
namespace sdo{
namespace commonsdk{
class CValueEncoder
{
public:
	static string normal_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	static string html_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	static string html_filter(const void *pBuffer, int nLen, const map<string,string> &mapParam);

	static string nocase_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	static string attack_filter(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	static string escape_encode(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	
	static string sbc2dbc_utf8(const void *pBuffer, int nLen, const map<string,string> &mapParam);
	static string sbc2dbc_gbk(const void *pBuffer, int nLen, const map<string,string> &mapParam);
};
typedef string (* FuncValueEncoder)(const void *, int ,const map<string,string> &);
}
}
#endif

