#ifndef TIMER_RUN_LIST_H_
#define TIMER_RUN_LIST_H_
typedef struct tagRunElement
{
	void (*pfn)(void *);
	void * pData;
	struct tagRunElement * pNext;
}SRunElement;
/*object pool*/
class CTimerRunList
{
public:
	CTimerRunList()
	{
		m_stHead.pNext=NULL;
		m_pstTail=&m_stHead;
	}
	void Add(void (*pfn)(void *),void * pData)
	{
		if(m_pstTail->pNext==NULL)
		{
			SRunElement * pElement=new SRunElement;
			pElement->pfn=pfn;
			pElement->pData=pData;
			pElement->pNext=NULL;
			m_pstTail->pNext=pElement;
			m_pstTail=pElement;
		}
		else
		{
			SRunElement * pElement=m_pstTail->pNext;
			pElement->pfn=pfn;
			pElement->pData=pData;
			m_pstTail=pElement;
		}
	}
	void Execute()
	{
		SRunElement * pElement;
		for(pElement=m_stHead.pNext;pElement!=m_pstTail->pNext;pElement=pElement->pNext)
		{
			pElement->pfn(pElement->pData);
		}
	}
	void Clean()
	{
		m_pstTail=&m_stHead;
	}
private:
	SRunElement m_stHead;
	SRunElement * m_pstTail;
};
#endif

