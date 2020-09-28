//==============================================================================
// utilities.h
//
// Copyright (c) 1998-2002 Ensemble Studios
//==============================================================================


#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)        delete   p; p=NULL;
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)  delete[] p; p=NULL;
#endif

//==============================================================================
//
//==============================================================================
bool     hasExtension(const char *string, const char *extension);
#ifdef UNICODE
bool     trimExtension(const char *string);
#endif
bool     trimExtension(const BCHAR_T *string);
void     getOSDXVersion(DWORD &dwDXVersion, DWORD &dwDXPlatform, DWORD &dwMajorVersion, DWORD &dwMinorVersion);
long     maxQuadtreeLevel(long xTiles, long zTiles, long desiredLeafTiles);

DWORD    getPlatformID(void);



// 32-bit poly CRC functions

DWORD    getFileCRC32(long dirID, const BCHAR_T *filename, DWORD initialCRC=0, const DWORD *key=NULL, DWORD skipOffset=0, DWORD skipLength=0);
DWORD    getLocalFileCRC32(const BCHAR_T *path, DWORD initialCRC=0, const DWORD *key=NULL);

void     CalculateCRC32(DWORD &theCRC, const BYTE* DataBuffer, long DataLen);
void     Crc321Byte(DWORD &theCRC, BYTE theByte);
void     Crc324Bytes(DWORD &theCRC, const DWORD *theDWORD);
void     Crc32Long(DWORD &theCRC, long theLong);
void     Crc32Float(DWORD &theCRC, float theFloat);
void     Crc32WORD(DWORD &theCRC, WORD theWORD);
void     Crc32Short(DWORD &theCRC, short theShort);
void     CRC32String(DWORD &theCRC, const BYTE* DataBuffer, long maxDataLen);

// Returns the same results as CalculateCRC32(), but only uses 64 bytes of CPU cache vs. 1024.
DWORD    Crc32SmallTable(const BYTE *ptr, DWORD cnt, DWORD crc);


char* __cdecl unsafeStrncpy (char *dest, const char *source, size_t count);




const char  *findFirstNonWhitespace(const char *str);
void        rtrimWhitespace(char *str);
void        allTrimWhitespace(char *str);

UINT        leftShiftForMask(DWORD mask);
void        convertTo565(WORD *dest, DWORD *src, DWORD num);
void        convertTo555(WORD *dest, DWORD *src, DWORD num);
void        convertTo4444(WORD *dest, DWORD *src, DWORD num);
void        convertTo1555(WORD *dest, DWORD *src, DWORD num);
UINT        countOnes(DWORD dw);

bool        getCubTgaNames(long dirID, const char *cubFilename, char cubTgaNames[6][_MAX_PATH]);

//==============================================================================
//
//==============================================================================
DWORD        hash(const unsigned char *key, DWORD length, DWORD initialValue);

//==============================================================================
//
//==============================================================================
void filterIntoVectorArray(const BVector* srcPts, long numSrcPts, BDynamicSimArray<BVector> &destPts, 
                           const float* pFilter, long numFilterElements, long SkipSize, bool filterEnds=true);


//==============================================================================
// Reverses the bits of i without using lookup tables.
//==============================================================================
unsigned int reverseBits(unsigned int i);

#include "utilities.inl"

#endif

