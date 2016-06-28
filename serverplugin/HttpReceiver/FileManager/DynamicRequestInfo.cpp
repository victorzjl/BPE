#include "DynamicRequestInfo.h"
#include "FileManagerMacro.h"
#include "FileManagerLogHelper.h"

void SURLInfo::ConstructParams(const vector<string> &vecValue, string &strParams) const
{
    FM_XLOG(XLOG_DEBUG, "SURLInfo::%s\n", __FUNCTION__);
    const unsigned int nVecValueSize = vecValue.size();

    unsigned int nSize = 0;
    for (unsigned int iIter = 0; iIter < nVecValueSize; ++iIter)
    {
        nSize += vecValue[iIter].size();
    }
    strParams.resize(nSize + _strFormatParams.size() - 2 * nVecValueSize + 1);
    SPRINTF_CONTENT((char *)strParams.c_str(), _strFormatParams, vecValue, nVecValueSize);
}
