#include "StdAfx.h"
#include "HeapStructure.h"


CHeapStructure::CHeapStructure(void)
{

	m_nUsed=0;
	m_nSize=0;
	m_rgHSE=NULL;

	// todo: dynamic on size range
	m_nLogBucketSizeMin=2;
	m_rgnStructureLogBucketSizeStart=new int[17]; // for readability, index is logbucketsize even though first 2 entries are unused.
}
bool CHeapStructure::SetStructure(int *rgnStructure)
{
	for(int j=0;j<17;++j) // todo: generalize
	{
		m_rgnStructureLogBucketSizeStart[j]=rgnStructure[j];
	}
	return true;
}

CHeapStructure::~CHeapStructure(void)
{
	delete[] m_rgnStructureLogBucketSizeStart;
	if(NULL!=m_rgHSE)
	{
		delete[] m_rgHSE;
	}
}

bool CHeapStructure::AddLinear(int nDelta, int nStart)
{
	bool toReturn=false;
	if(m_nUsed < m_nSize)
	{
		m_rgHSE[m_nUsed].SetLinear(nDelta, nStart);
		++m_nUsed;
		toReturn=true;
	}
	return toReturn;
}

// don't use for now...
bool CHeapStructure::Add(int nLogDelta, int nStart)
{
	bool toReturn=false;
	if(m_nUsed < m_nSize)
	{
		m_rgHSE[m_nUsed].Set(nLogDelta, nStart);
		++m_nUsed;
		toReturn=true;
	}
	return toReturn;
}

bool CHeapStructure::SetSize(int nSize)
{
	bool toReturn=false;
	if(0==m_nSize)
	{
		assert(NULL==m_rgHSE);
	m_rgHSE=new CHeapStructureElement[nSize];
	if(NULL!=m_rgHSE)
	{
		m_nSize=nSize;
		m_nUsed=0;
		toReturn=true;
	}
	}
	return toReturn;
}
void CHeapStructure::SetMax(int nSize)
{
	m_nMaxBinSize=nSize;
}


// identifies and orders distinct values of pUsage->m_pHUE[j].m_nSuppliedSize
// slow short algorithm, 1 pass
void CHeapStructure::GenerateHeap(int **prgnBinSize, int *pnBinCount, CHeapUsage *pUsage)
{
	assert(NULL==*prgnBinSize);
	assert(pUsage->m_nElementsUsed == pUsage->m_nSize);
	int *rgBinSizeLocal = NULL;
	rgBinSizeLocal = new int[pUsage->m_nElementsUsed];

	bool bFound=true; // to enter loop

	*pnBinCount=0;
	int nLastAdded=0;
	int nCandidate;

	while(bFound)
	{
		bFound=false;
		nCandidate = 65536+1;

		for(int j=0 ; j<pUsage->m_nElementsUsed ; ++j)
		{
			if(pUsage->m_pHUE[j].m_nSuppliedSize > nLastAdded && pUsage->m_pHUE[j].m_nSuppliedSize < nCandidate)
			{
				bFound=true;
				nCandidate = pUsage->m_pHUE[j].m_nSuppliedSize;
			}
		}
		if(bFound)
		{
			rgBinSizeLocal[*pnBinCount] = nCandidate;
			nLastAdded = nCandidate;
			++(*pnBinCount);
		}
	}
	*prgnBinSize = new int[*pnBinCount];
	for(int j=0;j<*pnBinCount;++j)
	{
		(*prgnBinSize)[j]=rgBinSizeLocal[j];
	}
	if(NULL!=rgBinSizeLocal)
	{
		delete[] rgBinSizeLocal;
	}
	return;
}

// currently unused
void CHeapStructure::GenerateHeap(int **prgnBinSize, int *pnBinCount)
{
	int binSize=0;
	int binOrdinal=0;
	assert(NULL==*prgnBinSize);



	for(int pass=0;pass<2;pass++)
	{
		if(1==pass)
		{
			*pnBinCount=binOrdinal;
			assert(NULL==*prgnBinSize);
			*prgnBinSize = new int[binOrdinal];
		}
		binSize=0;
		binOrdinal=0;
		for(int j=0;j<m_nUsed;++j)
		{
			while(binSize<m_nMaxBinSize && (m_nUsed-1==j || binSize < m_rgHSE[j+1].m_nStart)) // last bin or less than next bin substride start
			{
				binSize += m_rgHSE[j].m_nDelta;
				if(1==pass)
				{
					(*prgnBinSize)[binOrdinal]=binSize;
				}
				binOrdinal++; // ordinal starts at 0
			}
		}
	}
	return; 
}

// todo: clean up
void CHeapStructure::GenerateHeapFromArray(int **prgnBinSize, int *pnBinCount)
{
	int *pnDummy=NULL;
	GenerateHeapFromArray(prgnBinSize, pnBinCount, &pnDummy);
	if(NULL!=pnDummy)
	{
		delete[] pnDummy;
		pnDummy=NULL;
	}
	return;
}

void CHeapStructure::GenerateHeapFromArray(int **prgnBinSize, int *pnBinCount, int **prgnStructureKey)
{
	int binSize=0;
	int binOrdinal=0;
	assert(NULL==*prgnBinSize);

	for(int pass=0;pass<2;++pass)
	{
		if(1==pass)
		{
			*pnBinCount=binOrdinal;
			assert(NULL==*prgnBinSize);
			assert(NULL==*prgnStructureKey);
			*prgnBinSize = new int[binOrdinal];
			*prgnStructureKey = new int[binOrdinal];
		}
		binSize=0;
		binOrdinal=0;
		for(int j=2;j<17;++j) // todo: generalize
		{
			binSize=1<<j;
			do
			{
				if(1==pass)
				{
					(*prgnBinSize)[binOrdinal]=binSize;
					(*prgnStructureKey)[binOrdinal]=j;
				}
				assert(m_rgnStructureLogBucketSizeStart[j]>=MINBINSTRUCTUREVALUE || 16==j); // last entry is 0 for 64kb limit
				binSize += 1<<m_rgnStructureLogBucketSizeStart[j];

				binOrdinal++; // ordinal starts at 0

			}while(binSize<m_nMaxBinSize && (15==j || binSize <  1<<(j+1) )); // last bin or less than next bin substride start
		}
	}
	return;
}



