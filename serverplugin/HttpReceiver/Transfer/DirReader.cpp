#include "AvenueHttpTransferLogHelper.h"
#include "DirReader.h"
#include "CommonMacro.h"

#include <boost/filesystem.hpp>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <Shlwapi.h>

#ifdef access
#   undef access
#endif
#define access _access

#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace nsHpsTransfer
{

CDirReader::CDirReader(const char* szFilter):m_bOpen(false)
{
	TS_XLOG(XLOG_DEBUG,"CDirReader::%s, szFilter[%s]\n",__FUNCTION__,szFilter);
	if (szFilter == NULL || szFilter[0] == '\0')
		strcpy(m_szFilter,"*.*");
	else
		strcpy(m_szFilter,szFilter);
		
	m_itr = m_vecFiles.begin();
}

CDirReader::~CDirReader()
{
}

bool CDirReader::GetNextFilePath(char *pFileName)
{
	TS_XLOG(XLOG_DEBUG,"CDirReader::%s, fileFilter[%s], m_vecFiles.size[%u]\n",__FUNCTION__,m_szFilter,m_vecFiles.size());
	if(m_itr == m_vecFiles.end())
		return false;
		
	string &strFileName = *m_itr++;
	snprintf(pFileName, 255, "%s", strFileName.c_str());
	
	return true;
}

bool CDirReader::GetFirstFilePath(char *pFileName)
{
	TS_XLOG(XLOG_DEBUG,"CDirReader::%s, fileFilter[%s], m_vecFiles.size[%u]\n",__FUNCTION__,m_szFilter,m_vecFiles.size());
	m_itr = m_vecFiles.begin();
	
	if(m_itr == m_vecFiles.end())
		return false;

	string &strFileName = *m_itr++;
	snprintf(pFileName, 255, "%s", strFileName.c_str());
	
	return true;

}

bool CDirReader::OpenDir(const char* szPath)
{
	if (access(szPath,0) < 0)
	{
		return false;        
	}

	ReadFiles(szPath);
	
	vector<string>::iterator itr;
	for(itr=m_vecFiles.begin(); itr != m_vecFiles.end(); ++itr)
	{
		TS_XLOG(XLOG_TRACE,"CDirReader::%s\n.   %s\n",__FUNCTION__, (*itr).c_str());
	}
	
	return true;
}


void CDirReader::ReadFiles(const char *dirName)
{
    TS_XLOG(XLOG_DEBUG, "CDirReader::%s.   dir[%s]\n", __FUNCTION__, dirName);


    if (!boost::filesystem::exists(dirName))
    {
        TS_XLOG(XLOG_WARNING, "CDirReader::%s Directory [%s] is not exist.\n", __FUNCTION__, dirName);
        return;
    }

    boost::filesystem::recursive_directory_iterator iter(dirName);
    boost::filesystem::recursive_directory_iterator end_itr;

    while (iter != end_itr)
    {
        if (boost::filesystem::is_regular_file(iter->path()))
        {
            string filename = iter->path().string();

#ifdef WIN32
            if (S_OK == PathMatchSpecEx(filename.c_str(), m_szFilter, PMSF_NORMAL))
#else
            if (0 == fnmatch(m_szFilter, filename.c_str(), FNM_CASEFOLD))
#endif
            {
                TS_XLOG(XLOG_DEBUG, "CDirReader::%s.   matched fileName[%s]\n", __FUNCTION__, filename.c_str());
                m_vecFiles.push_back(filename);
            }
        }
        ++iter;
    }
}
}
