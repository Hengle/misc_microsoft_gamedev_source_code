#include "HeapStructure.h"
#include "HeapUsage.h"
#include "stdafx.h"

bool FindBin(int *pBinIndex, int nRequest, int *rgBinSize, int nBinCount);
bool BinDivisionValidate(int *rgnStructure, int nIndex);
int TableLookup(int n, const int rgrgnTable[][2]);
void ModelMeanUsage(double *pfFractionalUsage, char *szLogName, int *rgnStructure, bool bUseStructure, unsigned *pnBinCount, unsigned nHeapOrdinal);
void ModelMeanUsage(double *pfFractionalUsage, double *pfTotalRequested, double *pfTotalAllocated, CHeapUsage *pUsage, int *rgnStructure, bool bUseStructure, unsigned *pnBinCount);
void ModelMeanUsage(double *pfFractionalUsage, CHeapUsage *pUsage, CHeapStructure *pStructure, bool bUseStructure, 
					int **prgnBinUsed,
					int **prgnBinAllocated,
					double **prgfBinFractionalUsage,
					unsigned *pnBinCount,
					FILE *streamOutputLog,
					bool bUseOutputLog
					);
void ModelMeanUsage(double *pfFractionalUsage, double *pfTotalRequested, double *pfTotalAllocated, CHeapUsage *pUsage, CHeapStructure *pStructure, bool bUseStructure, 
					int **prgnBinUsed,
					int **prgnBinAllocated,
					double **prgfBinFractionalUsage,
					unsigned *pnBinCount,
					FILE *streamOutputLog, bool bUseOutputLog
					);
bool FindLowestUtilization(int *pnLUIndex, int *rgnStructure, int *rgnBinUsed, int *rgnBinAllocated, int nBinCount);
int GCD(int nx, int ny);
int FindGCD(int *rgnStructure, int nBinSizeMin, int nBinSizeBound);
void BinCodePrint(int *rgnStructure, FILE *stream, int nBinSizeMin, int nBinSizeBound);
void BinCodePrint(int *rgnStructure, FILE *stream);
void BinSubdivide(int *rgnStructure, int nIndex);
void HeapTune(CHeapUsage *pUsage, char *szOutputLogName, char *szHeapNameRoot);
void ProcessLog(char *szLogName, int nHeapNumber, char *szOutputLogName, char *szHeapNameRoot);
int main(int argc, char* argv[]);
void LogView(char *szLogName);
void ParseUsage(CHeapUsage *pUsage, char *szLogName, int nHeapNumber);
void HeapCount(unsigned *pnCount, char *szLogName);
bool MultipleDistinct(int *rgnAddress, int nHeapCount);