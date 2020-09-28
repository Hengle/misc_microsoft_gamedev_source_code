#include "StdAfx.h"
#include "HeapUsageEntry.h"

CHeapUsageEntry::~CHeapUsageEntry(void)
{
}

void CHeapUsageEntry::Set(int nRequest, int nSupplied, int nCount)
{
	m_nRequestedSize=nRequest;
	m_nSuppliedSize=nSupplied;
	m_nCallCount=nCount;
}
CHeapUsageEntry::CHeapUsageEntry(int nRequest, int nSupplied, int nCount)
{
	Set(nRequest, nSupplied, nCount);
}
CHeapUsageEntry::CHeapUsageEntry()
{
	;
}