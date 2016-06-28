#include "DirReader.h"

namespace redis{

CDirReader::CDirReader(const char* szFilter)
#ifdef WIN32
: m_hFinder(NULL)
#else
: m_pDir(NULL)
#endif
, m_bOpen(false)
{
	if (szFilter == NULL || szFilter[0] == '\0')
		strcpy(m_szFilter,"*.*");
	else
		strcpy(m_szFilter,szFilter);
}

CDirReader::~CDirReader()
{
#ifdef WIN32
	if (m_hFinder != NULL)
		FindClose(m_hFinder);
#else
	if (m_pDir)
		closedir(m_pDir);

#endif
}

bool CDirReader::GetNextFilePath(char* szFileName)
{
	if (!m_bOpen)
		return false;

#ifdef WIN32

	WIN32_FIND_DATA fd;
	while(1)
	{
		if (!FindNextFile(m_hFinder,&fd))
			return false;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		strcpy(szFileName, fd.cFileName);
		return true;
	}
	return false;

#else

	struct dirent   *dirp;
	struct stat      buf;
	char szFilePath[MAX_PATH * 2];
	while ( (dirp = readdir(m_pDir)) != NULL)
	{
		if ((!strcmp(dirp->d_name, ".")) || (!strcmp(dirp->d_name, ".."))) 
		{
			continue;
		}

		sprintf(szFilePath, "%s%s", m_szFindPath, dirp->d_name);
		if ( stat(szFilePath, &buf) == -1 )
		{
			continue;		
		}

		//is directory
		if (buf.st_mode & S_IFDIR)
		{
			continue;
		}

		//not match
		if (fnmatch(m_szFilter, dirp->d_name, FNM_PATHNAME|FNM_PERIOD) !=0)
		{
			continue;
		}

		strcpy(szFileName, dirp->d_name);
		return true;
	}
	return false;

#endif
}

bool CDirReader::GetFirstFilePath(char* szFileName)
{
	if (!m_bOpen)
		return false;

#ifdef WIN32
	WIN32_FIND_DATA fd;

	m_hFinder = FindFirstFile(m_szFindPath, &fd);	
	if (m_hFinder == INVALID_HANDLE_VALUE )
		return false;	

	bool bResult = true;
	if (strcmp(fd.cFileName,".") == 0)							//ignore "."
		bResult = (FindNextFile(m_hFinder, &fd) == TRUE);
	if (bResult && strcmp(fd.cFileName, "..") == 0)				//ignore ".."
		bResult = (FindNextFile(m_hFinder, &fd) == TRUE);

	while(bResult)
	{		
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			bResult = (FindNextFile(m_hFinder, &fd) == TRUE);	
			continue;
		}
		strcpy(szFileName, fd.cFileName);
		return true;
	}
	return false;

#else

	return GetNextFilePath(szFileName);

#endif
}

bool CDirReader::OpenDir(const char* szPath)
{
	if (access(szPath,0) < 0)
	{
		return false;        
	}

#ifdef WIN32

	if (m_hFinder != NULL)
		FindClose(m_hFinder);
	m_hFinder = NULL;

	sprintf(m_szFindPath, "%s/%s", szPath, m_szFilter);
	m_bOpen = true;

#else

	if (m_pDir)
		closedir(m_pDir);
	sprintf(m_szFindPath, "%s/", szPath);
	if ((m_pDir = opendir(m_szFindPath)) == NULL)
	{
		return false;
	}
	m_bOpen = true;   	

#endif
	return true;
}

}
