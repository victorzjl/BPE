#ifndef MAP_MEMORY_H
#define MAP_MEMORY_H

#include <string>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

static const unsigned POOL_LIST_SIZE = 4;
static const unsigned BUFF_LEN_TAB[POOL_LIST_SIZE] = {256, 512, 1024, 2048};

class CMapFile;

class CMapMemory
{
public:
	static CMapMemory &Instance();

	bool Init(const std::string &strFile, 
		boost::function<void(void*, unsigned)> usedObjHandle, const unsigned *pCountList);
	void Deinit();
	void *Allocate(unsigned nLen);
	void Deallocate(void *pBuffer);

private:
	CMapMemory();
	CMapMemory(const CMapMemory &);
	unsigned GetTabId(unsigned nLen);

private:
	struct Obj
	{
		unsigned nLen;
		Obj *pNext;
	};
	CMapFile* m_pFile;
	boost::mutex m_mut;
	Obj *m_pFreeList[POOL_LIST_SIZE];
};

#endif // MAP_MEMORY_H
