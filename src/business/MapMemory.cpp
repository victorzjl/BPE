#include "MapMemory.h"
#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

using namespace std;

//////////////////////////////////////////////////////////////////////////
// CMapFile

static int setflen(int fd, int flen)
{
	lseek(fd, flen - 1, SEEK_SET);   
	write(fd, "", 1);
	return 0;
}

class CMapFile
{
public:
	CMapFile()
	{
		m_pMem = NULL;
#ifdef WIN32
		m_hMap = NULL;
		m_hFile = INVALID_HANDLE_VALUE;		
#else
		m_hFile = -1;
#endif
	}

	void *Create(const char *szPath, long lSize)
	{
		m_lSize = lSize;
#ifdef WIN32
		m_hFile = CreateFile(szPath,
			GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			return NULL;
		if ((m_hMap = CreateFileMapping(
			m_hFile, NULL, PAGE_READWRITE, 0, lSize, NULL)) == NULL)
		{
			return NULL;
		}
		m_pMem = MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		return m_pMem;
#else
		m_hFile = open(szPath,  O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
		if (m_hFile == -1 || setflen(m_hFile, lSize) != 0)
			return NULL;
		if ((m_pMem = mmap(0, m_lSize, 
			PROT_READ | PROT_WRITE, MAP_SHARED, m_hFile, 0)) == MAP_FAILED)
		{
			m_pMem = NULL;
			return NULL;
		}
		return m_pMem;
#endif
	}

	void Release()
	{
#ifdef WIN32
		if (m_pMem != NULL)
			UnmapViewOfFile(m_pMem);
		if (m_hMap != NULL)
			CloseHandle(m_hMap);
		if (m_hFile != INVALID_HANDLE_VALUE)
			CloseHandle(m_hFile);
#else
		if (m_pMem != NULL)
			munmap((void*)m_pMem, m_lSize);
		if (m_hFile != -1)
			close(m_hFile);
#endif
	}

private:
	void* m_pMem;
	long m_lSize;
#ifdef WIN32
	HANDLE m_hMap;
	HANDLE m_hFile;
#else
	int m_hFile;
#endif	
};

//////////////////////////////////////////////////////////////////////////
// CMapQManager

CMapMemory::CMapMemory()
: m_pFile(NULL)
{
}

CMapMemory &CMapMemory::Instance()
{
	static CMapMemory instance;
	return instance;
}

unsigned CMapMemory::GetTabId(unsigned nLen)
{
	for (unsigned i = 0; i < POOL_LIST_SIZE; ++i)
	{
		if (nLen <= BUFF_LEN_TAB[i])
		{
			return i;
		}
	}
	return 0;
}

bool CMapMemory::Init(const string &strFile, 
	boost::function<void(void*, unsigned)> usedObjHandle, const unsigned *pCountList)
{
	unsigned nMemLen = POOL_LIST_SIZE * sizeof(unsigned);
	for (unsigned i = 0; i < POOL_LIST_SIZE; ++i)
	{
		m_pFreeList[i] = 0;
		nMemLen += pCountList[i] * (BUFF_LEN_TAB[i] + sizeof(unsigned));
	}

	m_pFile = new CMapFile;
	char *pMem = (char *)m_pFile->Create(strFile.c_str(), nMemLen);
	if (pMem == NULL)
	{
		return false;
	}

	bool bCountChanged = false;
	unsigned *pCountListMem = (unsigned *)pMem;
	char *pSizeMem[POOL_LIST_SIZE];
	pMem += POOL_LIST_SIZE * sizeof(unsigned);
	for (unsigned i = 0; i < POOL_LIST_SIZE; ++i)
	{
		if (pCountListMem[i] != pCountList[i])
		{
			pCountListMem[i] = pCountList[i];
			bCountChanged = true;
		}
		pSizeMem[i] = pMem;
		pMem += pCountList[i] * (BUFF_LEN_TAB[i] + sizeof(unsigned));
	}

	if (bCountChanged)
	{
		memset(pSizeMem[0], 0, nMemLen - POOL_LIST_SIZE * sizeof(unsigned));
	}

	boost::mutex::scoped_lock lock(m_mut);
	for (unsigned i = 0; i < POOL_LIST_SIZE; ++i)
	{
		char *pTmp = pSizeMem[i];
		Obj *pLastObj = NULL;
		for (unsigned j = 0; j < pCountList[i]; ++j)
		{
			Obj *pObj = (Obj *)pTmp;
			if (pObj->nLen != 0)
			{
				usedObjHandle(pTmp + sizeof(unsigned), pObj->nLen);
			}
			else
			{
				if (pLastObj == NULL)
				{
					m_pFreeList[i] = pObj;
				}
				else
				{
					pLastObj->pNext = pObj;
				}
				pLastObj = pObj;
				pLastObj->pNext = NULL;
			}
			pTmp += (sizeof(unsigned) + BUFF_LEN_TAB[i]);
		}
	}

	return true;
}

void CMapMemory::Deinit()
{
	if (m_pFile != NULL)
	{
		m_pFile->Release();
		m_pFile = NULL;
	}
}

void *CMapMemory::Allocate(unsigned nLen)
{
	if (m_pFile == NULL || nLen > BUFF_LEN_TAB[POOL_LIST_SIZE-1])
	{
		return NULL;
	}
	unsigned id = GetTabId(nLen);

	boost::mutex::scoped_lock lock(m_mut);
	Obj *pObj = m_pFreeList[id];
	if (pObj != NULL)
	{
		pObj->nLen = nLen;
		m_pFreeList[id] = pObj->pNext;
		return (char*)pObj + sizeof(unsigned);
	}
	return NULL;
}

void CMapMemory::Deallocate(void *pBuffer)
{
	if (m_pFile == NULL || pBuffer == NULL)
	{
		return;
	}
	Obj *pObj = (Obj*)((char*)pBuffer - sizeof(unsigned));
	unsigned id = GetTabId(pObj->nLen);

	boost::mutex::scoped_lock lock(m_mut);
	pObj->nLen = 0;
	pObj->pNext = m_pFreeList[id];
	m_pFreeList[id] = pObj;
}
