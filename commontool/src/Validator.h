#ifndef  BPE_VALIDATOR_H
#define BPE_VALIDATOR_H
#include <string>
using std::string;
namespace sdo{
namespace commonsdk{
class CValidator
{
public:
	static bool require_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool number_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool regex_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool email_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool url_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool stringlength_validate(const void *pBuffer, int nLen, const string &strParam="");
	static bool numberset_validate(const void *pBuffer, int nLen, const string &strParam="");

	static bool timerange_validate(const void *pBuffer, int nLen, const string &strParam="");
};
typedef bool (* FuncValidator)(const void *, int , const string &);
}
}
#endif

