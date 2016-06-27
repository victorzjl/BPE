#include "DirReader.h"
#include <string.h>

#ifdef WIN32
#define snprintf _snprintf
#endif
namespace sdo{
    namespace commonsdk{

        const char* pathFindExtension(const char* cFileName)
        {
            unsigned len = strlen(cFileName);
            const char* cR = cFileName + len - 1;
            while (*cR != '.' && cR != cFileName)
            {
                cR -= 1;
            }
            return cR;
        }

        CDirReader::CDirReader(const char* szFilter)
            :m_currentIndex(0) , m_bOpen(false)
        {
            if (szFilter == NULL || szFilter[0] == '\0')
                strcpy(m_szFilter,"*.*");
            else
                strcpy(m_szFilter,szFilter);
        }

        CDirReader::~CDirReader(){}

        bool CDirReader::GetNextFilePath(char* szFileName)
        {
            if (!m_bOpen)
                return false;

            if (m_currentIndex < m_vecFile.size())
            {
                strcpy(szFileName, m_vecFile[m_currentIndex++].c_str());
                return true;
            }
            return false;
        }

        bool CDirReader::GetFirstFilePath(char* szFileName)
        {
            if (!m_bOpen)
                return false;

            return GetNextFilePath(szFileName);
        }

        bool CDirReader::OpenDir(const char* szPath,  bool bReadSubDir)
        {
            if (access(szPath,0) < 0)
            {
                return false;
            }

#ifdef WIN32

            WIN32_FIND_DATA fd;
            char szFindPath[MAX_PATH] = {0};
            unsigned len = strlen(szPath);
            snprintf(szFindPath, MAX_PATH - 1, "%s/*", szPath);
            HANDLE hFindFile = FindFirstFile(szFindPath, &fd);
            if(hFindFile == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            BOOL bIsDirectory;
            BOOL bFinish = FALSE;
            unsigned dwFilterLen = strlen(m_szFilter);
            const char* pFilter = pathFindExtension(m_szFilter);

            while(!bFinish)
            {
                bIsDirectory = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                if( bIsDirectory && (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") ==0 ))
                {
                    bFinish = (FindNextFile(hFindFile, &fd) == FALSE);
                    continue;
                }

                char szFullFilePath[MAX_PATH] = {0};
                snprintf(szFullFilePath, sizeof(szFullFilePath), "%s\\%s", szPath, fd.cFileName);

                if(bIsDirectory)
                {
                    if (bReadSubDir)
                    {
                        OpenDir(szFullFilePath);
                    }
                }
                else  if (strcmp(pathFindExtension(fd.cFileName), pFilter) == 0)
                {
                    m_vecFile.push_back(szFullFilePath);
                    
                }

                bFinish = (FindNextFile(hFindFile, &fd) == FALSE);
            }
            FindClose(hFindFile);
            m_bOpen = true;

#else
            DIR *dp;
            struct dirent *entry;
            struct stat statbuf;
            if((dp = opendir(szPath)) == NULL)
            {
                return false;
            }

            while((entry = readdir(dp)) != NULL)
            {
                char szFullFilePath[MAX_PATH] = {0};
                snprintf(szFullFilePath, sizeof(szFullFilePath), "%s/%s", szPath, entry->d_name);

                lstat(szFullFilePath, &statbuf);
                if(S_IFDIR & statbuf.st_mode)
                {
                    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                    {
                        continue;
                    }

                    if (bReadSubDir)
                    {
                        OpenDir(szFullFilePath);
                    }
                }
                else if (fnmatch(m_szFilter, entry->d_name, FNM_PATHNAME|FNM_PERIOD) ==0)
                {
                    m_vecFile.push_back(szFullFilePath);
                }
            }

            closedir(dp);
            m_currentIndex = 0;
            m_bOpen = true;

#endif
            return true;
        }
    }
}
