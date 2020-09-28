// HeapTune.cpp : Defines the entry point for the console application.
//

// todo: improve the descriptive strings in the logs.
#include "HeapTune.h"
#include "stdafx.h"
#include "stdlib.h"
#include <crtdbg.h>
#include <string.h>

#include "sourcetemplate.h"
// current role of these 2d int arrays: populate remaining heap parameters consistently with FastHeap.  Later role: optimization
// Ends of arrays are flagged with {-1, -1}
// values used for interpolation for the Size Of Cache values in the Rockall CACHE_DETAILS array.  Based on FastHeap.
const int rgrgnLookupCacheSize[][2] =   {{4,256},{8,128},{20,64},{128,32},{160,16},{320,8},{512,4},{1024,8},{3072,4},{10240,0},{-1,-1}};
// values used for interpolation for the Bucket Chunks values in the Rockall CACHE_DETAILS array.  Based on FastHeap.
const int rgrgnLookupBucketChunk[][2] = {{4,32},{12,64},{24,128},{48,4096},{832,8192},{3072,65536},{-1,-1}};
// values used for interpolation for the Parent Page Size values in the Rockall CACHE_DETAILS array.  Based on FastHeap.
const int rgrgnLookupParentPageSize[][2] = {{4,4096},{832,8192},{3072,65536},{-1,-1}};

// Find the index of the bin of the smallest size that will accomodate the size of the request nRequest.
bool FindBin(int *pBinIndex, int nRequest, int *rgBinSize, int nBinCount)
{
	// linear search
	bool toReturn=false;
	int nIndex=0;
	while(nRequest > rgBinSize[nIndex] && nIndex < nBinCount)
	{
		++nIndex;
	}
	if(nIndex<nBinCount)
	{
		*pBinIndex=nIndex;
		toReturn=true;
	}
	else
	{
		// failure
		*pBinIndex=-1;  // caller must not use
		toReturn=false;
	}
	return toReturn;
}

// todo: perf
// Determine the validity of subdividing the bins based on the bin at index nIndex.
// This function prevents a subdivision operation that would produce bins of invalid sizes.
bool BinDivisionValidate(int *rgnStructure, int nIndex)
{
	bool bDivisible=false;
	int *rgnBinSize=NULL;
	int *rgnStructureKey=NULL;
	int nBinCount=0;
	CHeapStructure *pHS = new CHeapStructure();
	pHS->SetMax(65536);
	pHS->SetStructure(rgnStructure);
	pHS->GenerateHeapFromArray(&rgnBinSize, &nBinCount, &rgnStructureKey);
	int nSubdivisionTarget = nIndex-1;
	assert(nSubdivisionTarget<0 || rgnStructureKey[nSubdivisionTarget]>=0);
	if(nSubdivisionTarget>=0 && MINBINSTRUCTUREVALUE < rgnStructure[ rgnStructureKey[nSubdivisionTarget] ])
	{
		bDivisible = true;
	}
	delete pHS;
	if(NULL!=rgnBinSize)
	{
		delete[] rgnBinSize;
		rgnBinSize=NULL;
	}
	if(NULL!=rgnStructureKey)
	{
		delete[] rgnStructureKey;
		rgnStructureKey=NULL;
	}
	return bDivisible;
}


// Find a value in a 2x2 int array lookup table.
int TableLookup(int n, const int rgrgnTable[][2])
{
	int index=0;
	while( rgrgnTable[index+1][0] <= n && rgrgnTable[index+1][0] != -1) 
	{
		++index;
	}
	return rgrgnTable[index][1];
}

void ModelMeanUsage(double *pfFractionalUsage, char *szLogName, int *rgnStructure, bool bUseStructure, unsigned *pnBinCount, unsigned nHeapOrdinal)
{
	CHeapUsage huUsage;
	ParseUsage(&huUsage, szLogName, nHeapOrdinal);
	double fTotalRequested=0.0; // dummy
	double fTotalAllocated=0.0; // dummy
	ModelMeanUsage(pfFractionalUsage, &fTotalRequested, &fTotalAllocated, &huUsage, rgnStructure, bUseStructure, pnBinCount);
}

// no array statistics - only aggregage statistics
void ModelMeanUsage(double *pfFractionalUsage, double *pfTotalRequested, double *pfTotalAllocated, CHeapUsage *pUsage, int *rgnStructure, bool bUseStructure, unsigned *pnBinCount)
{
	CHeapStructure hsStructure;		
	int *rgnBinUsed=NULL;
	int *rgnBinAllocated=NULL;
	double *rgfBinFractionalUsage=NULL;

	hsStructure.SetMax(65536);

	if(bUseStructure)
	{
		hsStructure.SetStructure(rgnStructure);
	}

	ModelMeanUsage(pfFractionalUsage, pfTotalRequested, pfTotalAllocated, pUsage, &hsStructure, bUseStructure, 
				&rgnBinUsed,
				&rgnBinAllocated,
				&rgfBinFractionalUsage,
				pnBinCount,
				NULL, false);

		// cleanup
		if(NULL!=rgnBinUsed)
		{
			delete[] rgnBinUsed;
			rgnBinUsed=NULL;
		}
		if(NULL!=rgnBinAllocated)
		{
			delete[] rgnBinAllocated;
			rgnBinAllocated=NULL;
		}
		if(NULL!=rgfBinFractionalUsage)
		{
			delete[] rgfBinFractionalUsage;
			rgfBinFractionalUsage=NULL;
		}
}

// Determine the projected usage of a heap at a per-bin level based on empirical usage data
void ModelMeanUsage(double *pfFractionalUsage, CHeapUsage *pUsage, CHeapStructure *pStructure, bool bUseStructure, 
					int **prgnBinUsed,
					int **prgnBinAllocated,
					double **prgfBinFractionalUsage,
					unsigned *pnBinCount,
					FILE *streamOutputLog, bool bUseOutputLog
					)
{
	double fTotalRequested=0.0; // discarded
	double fTotalAllocated=0.0; // discarded
	ModelMeanUsage(pfFractionalUsage, &fTotalRequested, &fTotalAllocated, pUsage, pStructure, bUseStructure, 
					prgnBinUsed,
					prgnBinAllocated,
					prgfBinFractionalUsage,
					pnBinCount,
					streamOutputLog, bUseOutputLog);
	return;
}
void ModelMeanUsage(double *pfFractionalUsage, double *pfTotalRequested, double *pfTotalAllocated, CHeapUsage *pUsage, CHeapStructure *pStructure, bool bUseStructure, 
					int **prgnBinUsed,
					int **prgnBinAllocated,
					double **prgfBinFractionalUsage,
					unsigned *pnBinCount,
					FILE *streamOutputLog, bool bUseOutputLog
					)
{
	int *rgBinSize=NULL;
	int nBinCount=0;
	int nBinIndex=0;

	double fTotalRequested=0.0;
	double fTotalAllocated=0.0;

	int nBinTotalRequested;
	int nBinTotalAllocated;

	assert(NULL==*prgnBinUsed);
	assert(NULL==*prgnBinAllocated);
	assert(NULL==*prgfBinFractionalUsage);
	if(bUseStructure)
	{
		pStructure->GenerateHeapFromArray(&rgBinSize, &nBinCount);
	}
	else
	{
		pStructure->GenerateHeap(&rgBinSize, &nBinCount, pUsage);
	}

	assert(pUsage->m_nElementsUsed == pUsage->m_nSize);

	// todo: cleanup
	// make this part of heap structure?
	assert(NULL==*prgnBinUsed);
	assert(NULL==*prgnBinAllocated);
	assert(NULL==*prgfBinFractionalUsage);

	// handle the empty heap
	if(nBinCount>0)
	{
		assert(0 != pUsage->m_nElementsUsed);
		assert(0 != pUsage->m_nSize);
		*prgnBinUsed = new int[nBinCount];
		*prgnBinAllocated = new int[nBinCount];
		*prgfBinFractionalUsage = new double[nBinCount];
		for(int j=0;j<nBinCount;++j)
		{
			(*prgnBinUsed)[j]=0;
			(*prgnBinAllocated)[j]=0;
			(*prgfBinFractionalUsage)[j]=0.0;
		}
	}

	for(int j=0;j<pUsage->m_nElementsUsed;++j)
	{
		if(bUseStructure)
		{
			EIFAIL(FindBin(&nBinIndex, pUsage->m_pHUE[j].m_nRequestedSize, rgBinSize, nBinCount));
			nBinTotalRequested = pUsage->m_pHUE[j].m_nRequestedSize * pUsage->m_pHUE[j].m_nCallCount;  // this is per request size not bin size
			nBinTotalAllocated = rgBinSize[nBinIndex] * pUsage->m_pHUE[j].m_nCallCount;// this is per request size not bin size
		}
		else
		{
			nBinTotalRequested = pUsage->m_pHUE[j].m_nRequestedSize * pUsage->m_pHUE[j].m_nCallCount;  // this is per request size not bin size
			nBinTotalAllocated = pUsage->m_pHUE[j].m_nSuppliedSize * pUsage->m_pHUE[j].m_nCallCount;// this is per request size not bin size
		}

		(*prgnBinUsed)[nBinIndex] += nBinTotalRequested;
		(*prgnBinAllocated)[nBinIndex] += nBinTotalAllocated;
		fTotalRequested += (double)nBinTotalRequested;
		// todo: added factors: cache line overhead, chunk overhead, packing efficiency, ...
		fTotalAllocated += (double)nBinTotalAllocated;
	}

	if(bUseOutputLog)
	{
		fprintf(streamOutputLog, "Bin Usage:\nbinnumber  binsize  used  allocated  fractional\n");
	}
	for(int j=0;j<nBinCount;++j)
	{
		double fUtilization = (0==(*prgnBinAllocated)[j] ? 0 : (double)(*prgnBinUsed)[j]  / (double)(*prgnBinAllocated)[j] );
		(*prgfBinFractionalUsage)[j]=fUtilization; // convenience to caller
		if(bUseOutputLog)
		{
			fprintf(streamOutputLog, "%d\t%d\t%d\t%d\t%f\n", j, rgBinSize[j], (*prgnBinUsed)[j], (*prgnBinAllocated)[j], fUtilization);
		}
	}

	*pfFractionalUsage = fTotalAllocated<1.0f ? 0.0f : (fTotalRequested / fTotalAllocated);
	*pnBinCount=nBinCount;
	if(bUseOutputLog)
	{
		fprintf(streamOutputLog, "ModelMeanUsage: %d\t%f\n", nBinCount, *pfFractionalUsage);
	}
	*pfTotalRequested = fTotalRequested;
	*pfTotalAllocated = fTotalAllocated;

	if(NULL!=rgBinSize)
	{
		delete[] rgBinSize;
		rgBinSize=NULL;
	}

	return;
}

// greatest total unused bytes, with the requirement that bin subdivision does not violate minimum binsize delta
bool FindLowestUtilization(int *pnLUIndex, int *rgnStructure, int *rgnBinUsed, int *rgnBinAllocated, int nBinCount)
{
	bool bFound=false;
	int nLUIndex=-1; // flag value init
	int nMaxAbsoluteUnused=0;
	for(int j=0;j<nBinCount;++j)
	{
		if(
			(-1==nLUIndex || 
			nMaxAbsoluteUnused < rgnBinAllocated[nLUIndex]-rgnBinUsed[nLUIndex]) 
			&&
			rgnBinUsed[j]>0
			&&
			BinDivisionValidate(rgnStructure, j))
			
		{
			nLUIndex=j;
			bFound=true;
		}
	}
	*pnLUIndex=nLUIndex;
	return bFound;
}
// euclidean algorithm
int GCD(int nx, int ny)
{
	for(;;)
	{
		nx = nx % ny;
		if(0==nx)
			return ny;
		ny = ny % nx;
		if(0==ny)
			return nx;
	}
}

// todo: determine directly from the rgnStructure values without GenerateHeapFromArray, for regularized heaps
int FindGCD(int *rgnStructure, int nBinSizeMin, int nBinSizeBound)
{
	assert(nBinSizeMin < nBinSizeBound); // assumes nonempty range
	int *rgnBinSize=NULL;
	int *rgnStructureKey=NULL;
	int nBinCount=0;
	CHeapStructure *pHS = new CHeapStructure();
	pHS->SetMax(65536);
	pHS->SetStructure(rgnStructure);
	pHS->GenerateHeapFromArray(&rgnBinSize, &nBinCount, &rgnStructureKey);

	int nGCD = -1; // invalid GCD
	for(int j=0;j<nBinCount;++j)
	{
		if(rgnBinSize[j] >= nBinSizeMin && rgnBinSize[j] < nBinSizeBound)
		{
			if(-1==nGCD)
			{
				nGCD = rgnBinSize[j];
			}
			else
			{
				nGCD = GCD(nGCD, rgnBinSize[j]);
			}
		}
	}

	delete pHS;
	if(NULL!=rgnBinSize)
	{
		delete[] rgnBinSize;
		rgnBinSize=NULL;
	}
	if(NULL!=rgnStructureKey)
	{
		delete[] rgnStructureKey;
		rgnStructureKey=NULL;
	}
	assert(nGCD>=4);
	return nGCD;
}


// Format for source code usage the binsize array corresponding to the given structure.
// Prints up to and not including the upper bound
void BinCodePrint(int *rgnStructure, FILE *stream, int nBinSizeMin, int nBinSizeBound){
	int *rgnBinSize=NULL;
	int *rgnStructureKey=NULL;
	int nBinCount=0;
	CHeapStructure *pHS = new CHeapStructure();
	pHS->SetMax(65536);
	pHS->SetStructure(rgnStructure);
	pHS->GenerateHeapFromArray(&rgnBinSize, &nBinCount, &rgnStructureKey);

	for(int j=0;j<nBinCount;++j)
	{
		if(rgnBinSize[j] >= nBinSizeMin && rgnBinSize[j] < nBinSizeBound)
		{
		fprintf(stream, "{\t%d,\t%d,\t%d,\t%d},\n",
			rgnBinSize[j],
			TableLookup(rgnBinSize[j], rgrgnLookupCacheSize),
			TableLookup(rgnBinSize[j], rgrgnLookupBucketChunk),
			TableLookup(rgnBinSize[j], rgrgnLookupParentPageSize)
			);
		}
	}

	delete pHS;
	if(NULL!=rgnBinSize)
	{
		delete[] rgnBinSize;
		rgnBinSize=NULL;
	}
	if(NULL!=rgnStructureKey)
	{
		delete[] rgnStructureKey;
		rgnStructureKey=NULL;
	}
	return;
}
// Format for source code usage the binsize array corresponding to the given structure.
void BinCodePrint(int *rgnStructure, FILE *stream) // note in-out param
{
	BinCodePrint(rgnStructure, stream, 0, 65536+1);
}


// Perform a subdivision of the array of bins, maintaining the property that the the values in the structure array are nondecreasing.
void BinSubdivide(int *rgnStructure, int nIndex) // note in-out param
{
	// todo: eliminate table from method
	int *rgnBinSize=NULL;
	int *rgnStructureKey=NULL;
	int nBinCount=0;
	CHeapStructure *pHS = new CHeapStructure();
	pHS->SetMax(65536);
	pHS->SetStructure(rgnStructure);
	pHS->GenerateHeapFromArray(&rgnBinSize, &nBinCount, &rgnStructureKey);


	int nSubdivisionTarget = nIndex-1;
	rgnStructure[ rgnStructureKey[nSubdivisionTarget] ] -= 1; // subdivide bin below bin of least fractional usage
	assert(rgnStructure[ rgnStructureKey[nSubdivisionTarget] ] >= MINBINSTRUCTUREVALUE);

	// assume bin separation intervals are nondecreasing
	int nKey=rgnStructureKey[nSubdivisionTarget]-1;
	while(nKey>2 && rgnStructure[nKey] > rgnStructure[ rgnStructureKey[nSubdivisionTarget] ] )
	{
		rgnStructure[nKey] -= 1;
		nKey--;
	}

	delete pHS;
	if(NULL!=rgnBinSize)
	{
		delete[] rgnBinSize;
	}
	if(NULL!=rgnStructureKey)
	{
		delete[] rgnStructureKey;
	}
	return;
}

// iteratively refine a heap definition in order to increase the total fractional memory element usage.
void HeapTune(CHeapUsage *pUsage, char *szOutputLogName, char *szHeapNameRoot)
{
	int nSummaryBufferRemaining = 4096;
	char szSummaryBuffer[4096]; // summary string of HeapTune heap sequence
	char szLineBuffer[1024];
	char szFormat[] = "Iteration %d\tBins: %d\tUsage %.3f\n";
	CHeapStructure *pStructure=NULL;		
	double fFractionalUsage;
	int *rgnBinUsed=NULL;
	int *rgnBinAllocated=NULL;
	double *rgfBinFractionalUsage=NULL;
	unsigned nBinCount=0;
	int nLUIndex=-1;
	int nGCDStride1=0;
	int nGCDStride2=0;

	char szSourceFileNameCPPBuffer[1024];
	char szSourceFileNameHPPBuffer[1024];
	char szClassNameBuffer[1024];
	char szIncludeFlagBuffer[1024];
	char *szSourceFileNameTemplate = "%s_%03d%s"; // myheap_001.cpp

	char *szSummary = szSummaryBuffer;

	FILE *streamOutputLog=NULL;
	if(0!=fopen_s(&streamOutputLog,szOutputLogName,"w"))
	{
		fprintf(stderr,"Unable to open output %s\n", szOutputLogName);
		exit(-1);
	}

	FILE *streamSourceCPP=NULL;
	FILE *streamSourceHPP=NULL;


	// fastheap canonicalized
	int rgnStructure[] = {
		0,  0,  2,  2,  2, 
		3,  4,  5,  6,  6, 
		10,  10,  10, 11, 12, 
		15, 0
	};

	sprintf_s(szClassNameBuffer,_countof(szClassNameBuffer), "%s", szHeapNameRoot);
	_strupr_s(szClassNameBuffer, _countof(szClassNameBuffer)); // uppercase class name
	sprintf_s(szIncludeFlagBuffer, _countof(szIncludeFlagBuffer),"_%s_HPP_", szClassNameBuffer);
	_strupr_s(szIncludeFlagBuffer, _countof(szIncludeFlagBuffer)); // uppercase include flag

	// refine the heap design
	// todo - user-specified exit criterion?
	for(int j=0;j<10;++j)
	{
		assert(NULL==pStructure);
		bool bSuccess = true;
		pStructure = new CHeapStructure();

		pStructure->SetMax(65536);
		fFractionalUsage=0.0;
		pStructure->SetStructure(rgnStructure);
		fprintf(streamOutputLog, "\n\nIteration %d START\n",j); 

		// iteration j=0 is the regularized FastHeap starting point described by the initial array above
		ModelMeanUsage(&fFractionalUsage, pUsage, pStructure, true, &rgnBinUsed, &rgnBinAllocated, &rgfBinFractionalUsage, &nBinCount, streamOutputLog, true); // prints

		// output sourcecode HPP
		sprintf_s(szSourceFileNameHPPBuffer, _countof(szSourceFileNameHPPBuffer), szSourceFileNameTemplate, szHeapNameRoot, nBinCount, szFilenameSuffixHPP); // name is number of bin sizes
		if(0!=fopen_s(&streamSourceHPP,szSourceFileNameHPPBuffer,"w"))
		{
			fprintf(stderr,"Unable to open output %s\n", szSourceFileNameHPPBuffer);
			bSuccess = false;
			goto cleanup;
		}
		fprintf(streamSourceHPP, g_szSourceTemplateHPP01, szIncludeFlagBuffer, szIncludeFlagBuffer, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer);
		if(NULL!=streamSourceHPP)
		{
			fclose(streamSourceHPP);
		}	


		// output sourcecode CPP start
		sprintf_s(szSourceFileNameCPPBuffer, _countof(szSourceFileNameCPPBuffer), szSourceFileNameTemplate, szHeapNameRoot, nBinCount, szFilenameSuffixCPP); // name is number of bin sizes
		if(0!=fopen_s(&streamSourceCPP,szSourceFileNameCPPBuffer,"w"))
		{
			fprintf(stderr,"Unable to open output %s\n", szSourceFileNameCPPBuffer);
			bSuccess = false;
			goto cleanup;
		}
		nGCDStride1 = FindGCD(rgnStructure, 0, 1024);
		nGCDStride2 = FindGCD(rgnStructure, 1024, 65536+1);
		fprintf(streamSourceCPP, g_szSourceTemplateCPP01, j, nBinCount, fFractionalUsage, szSourceFileNameHPPBuffer, nGCDStride1, nGCDStride2);
		BinCodePrint(rgnStructure, streamSourceCPP, 0, 1024);
		fprintf(streamSourceCPP, g_szSourceTemplateCPP02);
		BinCodePrint(rgnStructure, streamSourceCPP, 1024, 65536+1);
		fprintf(streamSourceCPP, g_szSourceTemplateCPP03, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer, szClassNameBuffer);
		if(NULL!=streamSourceCPP)
		{
			fclose(streamSourceCPP);
		}
		// output sourcecode CPP end

		fprintf(streamOutputLog, "\n\nIteration %d\tBins: %d\tUsage %.3f\n",j, nBinCount, fFractionalUsage); 
		printf(szFormat,j, nBinCount, fFractionalUsage);
		sprintf_s(szLineBuffer, 1024, szFormat,j, nBinCount, fFractionalUsage);
		strcpy_s(szSummary, nSummaryBufferRemaining, szLineBuffer);
		szSummary += strlen(szLineBuffer);
		*szSummary=0;
		nSummaryBufferRemaining -= (int)strlen(szLineBuffer);

		for(int k=0;k<17;++k)
		{
			fprintf(streamOutputLog, "%d ",rgnStructure[k]);
		}

		// avoid unnecessary subdivide.  Todo: set exit criterion.
		if(FindLowestUtilization(&nLUIndex, rgnStructure, rgnBinUsed, rgnBinAllocated, nBinCount))
		{
			if(j<10-1)// Todo: set exit criterion.
			{
				BinSubdivide(rgnStructure, nLUIndex);
			}
		}
		fprintf(streamOutputLog, "\n");

cleanup:
		if(NULL!=pStructure)
		{
			delete pStructure;
			pStructure=NULL;
		}
		if(NULL!=rgnBinUsed)
		{
			delete[] rgnBinUsed;
			rgnBinUsed=NULL;
		}
		if(NULL!=rgnBinAllocated)
		{
			delete[] rgnBinAllocated;
			rgnBinAllocated=NULL;
		}
		if(NULL!=rgfBinFractionalUsage)
		{
			delete[] rgfBinFractionalUsage;
			rgfBinFractionalUsage=NULL;
		}
		if(!bSuccess)
		{
			exit(1);
		}
	}
	fprintf(streamOutputLog, "\n\nHeapTune Iteration Summary:\n%s",szSummaryBuffer);

	if(NULL!=streamOutputLog)
	{
		fclose(streamOutputLog);
	}


	return;
}


// number of heap dump entries in log
void HeapCount(unsigned *pnCount, char *szLogName)
{
	unsigned nHeapCount=0;
	int dummy=0;
	FILE *streamLog=NULL;
	if(0!=fopen_s(&streamLog,szLogName,"r"))
	{
		fprintf(stderr,"Unable to open input %s\n", szLogName);
		exit(-1);
	}

	EINONZERO(fseek(streamLog, SEEK_SET, 0));

	char szLine[MaxLineLength]; 
	szLine[0]=NULL;

	char *szSource=NULL;  // initial value not used
	do
	{
		int nFound=0;
		do
		{
			szSource = fgets(szLine, MaxLineLength, streamLog);
			nFound=sscanf_s(szLine, "Heap at %x", &dummy);
		}while(1!=nFound && NULL!=szSource);
		if(nFound)
		{
			++nHeapCount;
		}
	}while(NULL!=szSource);
	*pnCount = nHeapCount;
	if(NULL!=streamLog)
	{
		fclose(streamLog);
	}
}


// Parse the log and extract the usage info for the specified heap
void ParseUsage(CHeapUsage *pUsage, char *szLogName, int nHeapNumber)
{
	FILE *streamLog=NULL;
	fpos_t posHeapStart;
	if(0!=fopen_s(&streamLog,szLogName,"r"))
	{
		fprintf(stderr,"Unable to open input %s\n", szLogName);
		exit(-1);
	}

	int nHeapHeaderLineCount=7;
	int nHeapLineCount=0;
	int nHeapElementCount=0;

	EINONZERO(fseek(streamLog, SEEK_SET, 0));

	char szLine[MaxLineLength]; 
	szLine[0]=NULL;

	char *szSource=NULL;  // initial value not used
	for(int j=0;j<nHeapNumber;++j)
	{
		int nFound=0;
		do
		{
			szSource = fgets(szLine, MaxLineLength, streamLog);
			nFound=sscanf_s(szLine, "Heap at %x", (&(pUsage->m_nBaseAddress)));
		}while(1!=nFound && NULL!=szSource);
	}
	for(int j=0;j<nHeapHeaderLineCount;++j)
	{
		fgets(szLine, MaxLineLength, streamLog);
	}

	// bookmark
	fgetpos(streamLog, &posHeapStart);
	// line count
	int nParameterCount;
	int d1=0;
	int d2=0;
	int d3=0;
	int d4=0;
	int d5=0;
	int d6=0;
	// value not used
	do
	{
		fgets(szLine, MaxLineLength, streamLog);
		nParameterCount = sscanf_s(szLine, " %d %d %d %d %d %d", &d1, &d2, &d3, &d4, &d5, &d6);
		if(6==nParameterCount)
		{
			++nHeapLineCount;
			nHeapElementCount+=2;
		}
		else if(3==nParameterCount)
		{
			++nHeapLineCount;
			++nHeapElementCount;
			break; // do not continue past a 3 line count
		}
		else
		{
			break;  // do not read if line is not as expected
		}
	
	} while(nParameterCount>0); // log line may have 3 or 6 parameters


	EINONZERO(fsetpos(streamLog, &posHeapStart));

	EIZERO(pUsage->SetSize(nHeapElementCount))

	// data read
	for(int j=0;j<nHeapLineCount;++j)
	{
		fgets(szLine, MaxLineLength, streamLog);
		nParameterCount = sscanf_s(szLine, " %d %d %d %d %d %d", &d1, &d2, &d3, &d4, &d5, &d6);
		assert(6==nParameterCount || 3==nParameterCount);
		pUsage->AddElement(d1, d2, d3);
		if(6==nParameterCount)
		{
			pUsage->AddElement(d4, d5, d6);
		}
	}
	if(NULL!=streamLog)
	{
		fclose(streamLog);
	}
}

// Parse the heap usage data and invoke HeapTune to refine the heap defition.
void ProcessLog(char *szLogName, int nHeapNumber, char *szOutputLogName, char *szHeapNameRoot)
{
	CHeapUsage huUsage;
	ParseUsage(&huUsage, szLogName, nHeapNumber);
	HeapTune(&huUsage, szOutputLogName, szHeapNameRoot);
	return;
}


int main(int argc, char* argv[])
{
	char *szHeapNameRoot = "HeapMGS"; // default
	if(2==argc)
	{
		char *szLogName=argv[1];
		LogView(szLogName);
	}
	else if(4==argc || 5==argc)
	{
		char *szLogName=argv[1];
		char *szOutputLogName=argv[3];
		int nHeapNumber = atoi(argv[2]);


		printf("\nRunning HeapTune\n\n");

		if(5==argc)
		{
			szHeapNameRoot = argv[4];
		}

		ProcessLog(szLogName, nHeapNumber, szOutputLogName, szHeapNameRoot);

		printf("\n\nHeapTune Done.  Rockall source files generated.\n");
		printf("Output log:\n     %s\n", szOutputLogName);

	}
	else
	{   //      00000000001111111111222222222233333333334444444444555555555566666666667777777777
		//      01234567890123456789012345678901234567890123456789012345678901234567890123456789
		printf("\n");
		printf("========\n");
		printf("HeapTune\n");
		printf("========\n");
		printf("\n");
		printf("\n");
		printf("Utility to generate Rockall Heaps based on Rockall DebugTrace profiling data\n");
		printf("(C)MGS 2006  Microsoft Confidential\n");
		printf("Support Alias: tntssp\n");
		printf("\n");
		printf("Usage 1 - Rockall Log file heap enumeration:\n");
		printf("     HeapTune.exe <logfile> \n");
		printf("Usage 2 - Tune Rockall heap based on memory usage profile:\n");
		printf("     HeapTune.exe <logfile> <ordinal within logfile> <output log> <HeapName>\n");
		printf("\n");
		printf("\n");
		printf("Param 1: Rockall log file\n");
		printf("Param 2: heap statistic ordinal within log (1 for 1st heap statistic set, etc.)\n");
		printf("Param 3: HeapTune output log\n");
		printf("Param 4: HeapName to be used for filenames and classnames (default: %s)\n", szHeapNameRoot);
		printf("\n");
		printf("ex.\n");
		printf("     HeapTune.exe C:\\DebugTrace.log 2 c:\\HeapTuneLog.txt\n");
		printf("Uses the 2nd entry in the Rockall log C:\\DebugTrace.log to generate new heap\n");
		printf("sourcecode (CPP, HPP), with projected usage details logged to c:\\HeapTuneLog.txt\n");
		printf("\n");
		printf("ex.\n");
		printf("     HeapTune.exe C:\\DebugTrace.log\n");
		printf("Overview of heap data within the Rockall log file C:\\DebugTrace.log.\n");
		printf("Heap base address, fractional and total usage.\n");
	}

	_CrtDumpMemoryLeaks();
	return 0;
}
// whether more than 1 distinct entries appear in the array
bool MultipleDistinct(int *rgnAddress, int nHeapCount)
{
	bool toReturn=false;
	// handle heap counts of 0, 1
	for(int j=1;j<nHeapCount;++j)
	{
		if(rgnAddress[0] != rgnAddress[j])
		{
			toReturn=true;
			break;
		}
	}
	return toReturn;
}

// rockall log parser and summary
void LogView(char *szLogName)
{
	FILE *streamLog=NULL;
	double fFractionalUsage=0.0;
	double fTotalRequested=0.0;
	double fTotalAllocated=0.0;

	int *rgnAddress=NULL;
	int *rgnBinCount=NULL;
	double *rgfUsage=NULL;
	double *rgfRequested=NULL;
	double *rgfAllocated=NULL;

	CHeapUsage huUsage;
	unsigned nBinCount=0;

	bool bSuccess = true;

	unsigned nHeapCount=0;
	HeapCount(&nHeapCount, szLogName);
	if(nHeapCount>0)
	{
		rgnAddress = new int[nHeapCount];
		rgnBinCount = new int[nHeapCount];
		rgfUsage = new double[nHeapCount];
		rgfRequested = new double[nHeapCount];
		rgfAllocated = new double[nHeapCount];
	}


	if(0!=fopen_s(&streamLog,szLogName,"r"))
	{
		fprintf(stderr,"Unable to open input %s\n", szLogName);
		bSuccess = false;
		goto cleanup;
	}

	EINONZERO(fseek(streamLog, SEEK_SET, 0));

	char szLine[MaxLineLength]; 
	szLine[0]=NULL;

	char *szSource=NULL;  // initial value not used
	unsigned nHeapOrdinal=0; // initial value only - heap ordinal starts at 1
	unsigned nHeapBaseAddress=0;

	//      00000000001111111111222222222233333333334444444444555555555566666666667777777777
	//      01234567890123456789012345678901234567890123456789012345678901234567890123456789

	printf("\nRunning HeapTune\n\n");


	do
	{
		int nFound=0;
		do
		{
			szSource = fgets(szLine, MaxLineLength, streamLog);
			nFound=sscanf_s(szLine, "Heap at %x", &nHeapBaseAddress);
		}while(1!=nFound && NULL!=szSource);
		if(1==nFound)
		{
			++nHeapOrdinal;
			ParseUsage(&huUsage, szLogName, nHeapOrdinal);
			ModelMeanUsage(&fFractionalUsage, &fTotalRequested, &fTotalAllocated, &huUsage, NULL, false, &nBinCount); // empirical usage only - not modeled
			huUsage.Reset();

			if(nHeapOrdinal > nHeapCount) 
			{	// this is to disable warning C6386
				// we do not expect this code path to ever execute
				nHeapOrdinal = nHeapCount;
			}
			assert(nHeapOrdinal-1 < nHeapCount);
			assert(nHeapOrdinal-1 >= 0);
			rgnAddress[nHeapOrdinal-1] = nHeapBaseAddress;
			rgnBinCount[nHeapOrdinal-1] = nBinCount;
			rgfUsage[nHeapOrdinal-1] = fFractionalUsage;
			rgfRequested[nHeapOrdinal-1] = fTotalRequested;
			rgfAllocated[nHeapOrdinal-1] = fTotalAllocated;
		}
	}while(NULL!=szSource);

	// print
	printf("Description of heap usage dumps appearing in logfile.\n");
	printf("Logfile:\n");
	printf("     %s\n", szLogName);
	printf("\n");
	printf("Column 1: ordinal of heap usage dump within log file\n");
	printf("Column 2: Base address of heap\n"); 
	printf("     (log file may contain data from several instances of a Rockall heap)\n");
	printf("Column 3: Bin count (distinct bin sizes supplied)\n");
	printf("Column 4: Fractional usage of memory allocated by heap.\n");
	printf("     (Lower values indicate greater potential for improvement via HeapTune)\n");
	printf("\n\n");

	printf("Ordinal\tHeap\t\tBin\tFrac\tTtlBytes\tTtlBytes\n");
	printf(" \tAddress\t\tCount\tUsage\tRequested\tAllocated\n");
	printf("\n");

	printf("\n");
	printf("===== All heaps in log\n");
	printf("\n");
	for(unsigned j=0;j<nHeapCount;++j)
	{
		printf("%3d\t0x%08x\t%3d\t%1.4f\t%12.0f\t%12.0f\n", j+1, rgnAddress[j], rgnBinCount[j], rgfUsage[j], rgfRequested[j], rgfAllocated[j]);
	}

	if(MultipleDistinct(rgnAddress, nHeapCount))
	{
		// print by heap
		printf("\n");
		printf("===== Breakout by Heap\n");
		printf("\n");

		int nCurrentHeap=-1;
		for(unsigned j=0;j<nHeapCount;++j)
		{
			if(rgnAddress[j]!=0) // Print is in order of entry in log, not by address.
			{
				printf("\nHeap: 0x%08x\n", rgnAddress[j]);
				nCurrentHeap = rgnAddress[j];
				for(unsigned k=j;k<nHeapCount;++k)
				{
					if(rgnAddress[k] == nCurrentHeap)
					{
					printf("%3d\t0x%08x\t%3d\t%1.4f\t%12.0f\t%12.0f\n", k+1, rgnAddress[k], rgnBinCount[k], rgfUsage[k], rgfRequested[k], rgfAllocated[k]);
					rgnAddress[k]=0; // zero rgnAddress[j] to indicate it has been printed.  
					}
				}
			}
		}
	}

cleanup:
	if(NULL!=rgnAddress)
	{
		delete[] rgnAddress;
	}
	if(NULL!=rgnBinCount)
	{
		delete[] rgnBinCount;
	}
	if(NULL!=rgfUsage)
	{
		delete[] rgfUsage;
	}
	if(NULL!=rgfRequested)
	{
		delete[] rgfRequested;
	}
	if(NULL!=rgfAllocated)
	{
		delete[] rgfAllocated;
	}
	if (!bSuccess)
	{
		exit(1);
	}
	return;
}




