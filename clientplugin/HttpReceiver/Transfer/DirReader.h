#ifndef DIR_READER_H
#define DIR_READER_H

#include<stdio.h>
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

#include <string>
#include <vector>
using std::string;
using std::vector;

namespace nsHpsTransfer
{
class CDirReader
{
public:
	CDirReader(const char* szFilter = NULL);	
	virtual ~CDirReader();

public:
	bool OpenDir(const char* szPath);

	bool GetFirstFilePath(char *pFileName);
	bool GetNextFilePath(char *pFileName);

private:
	void ReadFiles(const char*szPath);
	
private:
	char	m_szFindPath[MAX_PATH];
	char	m_szFilter[64];
	vector<string> m_vecFiles;
	vector<string>::iterator m_itr;
	
	bool	m_bOpen;

#ifndef WIN32
	DIR*	m_pDir;
#endif
};
}
#endif
