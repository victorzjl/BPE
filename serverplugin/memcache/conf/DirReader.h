#ifndef DIR_READER_H
#define DIR_READER_H

#include<stdio.h>
#include <string>
#ifndef WIN32
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>
#include <sys/stat.h>
#else
#include <windows.h>
#include <io.h>
#endif

#ifndef WIN32
#define  MAX_PATH 256
#endif

class CDirReader
{
public:
	CDirReader(const char* szFilter = NULL);	
	virtual ~CDirReader();

public:
	//start open direcotry	
	bool OpenDir(const char* szPath);

	//get the first filename in directory
	//return FALSE when reach then end
	bool GetFirstFilePath(char* szFileName);

	//get the next filename in directory
	//return FALSE when reach the end
	bool GetNextFilePath(char* szFileName);


private:
	char	m_szFindPath[MAX_PATH];
	char	m_szFilter[32];

#ifdef WIN32
	HANDLE	m_hFinder;
#else

	DIR*	m_pDir;

#endif
	bool    m_bOpen;

};

#endif


