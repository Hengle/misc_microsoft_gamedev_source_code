#include "StdAfx.h"
#include "HeapUsage.h"

CHeapUsage::CHeapUsage(void)
{
	m_pHUE=NULL;
	Reset();
}
bool CHeapUsage::SetSize(int size)
{
	bool toReturn=false;
	if(0==m_nSize)
	{
	assert(NULL==m_pHUE);
	m_pHUE = new CHeapUsageEntry[size];
	if(NULL!=m_pHUE)
		{
			m_nSize = size; // caller can retry allocation on failure
			toReturn=true;
		}
	}
	return toReturn;
}
bool CHeapUsage::AddElement(int nRequest, int nSupplied, int nCount)
{
	bool toReturn=false;
	if(m_nElementsUsed < m_nSize)
	{
		m_pHUE[m_nElementsUsed].Set(nRequest, nSupplied, nCount);
		++m_nElementsUsed;
		toReturn=true;
	}
	return toReturn;
}
CHeapUsage::~CHeapUsage(void)
{
	if(NULL!=m_pHUE)
	{
		delete[] m_pHUE;
	}

}
void CHeapUsage::Reset(void)
{
	m_nSize=0;
	m_nElementsUsed=0;
	m_nBaseAddress=NULL;
	if(NULL!=m_pHUE)
	{
		delete[] m_pHUE;
		m_pHUE=NULL;
	}
	return;
}
