//==============================================================================
// utilities.cpp
//
// Copyright (c) 1999 - 2002 Ensemble Studios
//
// Misc utility functions
//==============================================================================

//==============================================================================
//Includes
#include "xsystem.h"
#include "utilities.h"
#include "mathutil.h"
#include "hash\crc.h"
#include "string\bsnprintf.h"
#include "consoleOutput.h"

// ==============================================================================
// Compute 32-bit CRC on a stream of bytes
// ==============================================================================
void CalculateCRC32(DWORD &theCRC, const BYTE* DataBuffer, long DataLen)
{
   theCRC = calcCRC32SmallTable(DataBuffer, DataLen, theCRC, false);
}

// ==============================================================================
// Compute 32-bit CRC on a null terminated string
//          it will process the string up to the max Length supplied 
//          unless it encounters a null, in which case it will
//          stop and return.  The Null will not be part of the CRC computation.
// ==============================================================================
void CRC32String(DWORD &theCRC, const BYTE* DataBuffer, long maxDataLen)
{
   theCRC = calcCRC32SmallTable(DataBuffer, maxDataLen, theCRC, false);
}


// ==============================================================================
// Compute 32-bit CRC on a single Byte
// ==============================================================================

void Crc321Byte(DWORD &theCRC, BYTE theByte)
{
   theCRC = calcCRC32SmallTable(&theByte, sizeof(BYTE), theCRC, false);
}



// ==============================================================================
// Compute 32-bit CRC on a 4 Byte value (int, dword, float, etc)
// ==============================================================================
void Crc324Bytes(DWORD &theCRC, const DWORD *theDWORD)
{
   theCRC = calcCRC32SmallTable(reinterpret_cast<const BYTE*>(theDWORD), sizeof(DWORD), theCRC, false);
}

//==============================================================================
// Crc32Long
//==============================================================================
void Crc32Long(DWORD &theCRC, long theLong)
{
   DWORD* theDWORD=(DWORD*)&theLong;
   Crc324Bytes(theCRC, theDWORD);
}

//==============================================================================
// Crc32Float
//==============================================================================
void Crc32Float(DWORD &theCRC, float theFloat)
{
   DWORD* theDWORD=(DWORD*)&theFloat;
   Crc324Bytes(theCRC, theDWORD);
}

//==============================================================================
// Crc32WORD
//==============================================================================
void Crc32WORD(DWORD &theCRC, WORD theWORD)
{
   DWORD theDWORD=theWORD;
   Crc324Bytes(theCRC, &theDWORD);
}

//==============================================================================
// Crc32Short
//==============================================================================
void Crc32Short(DWORD &theCRC, short theShort)
{
   DWORD theDWORD=theShort;
   Crc324Bytes(theCRC, &theDWORD);
}

//==============================================================================
// hasExtension
//==============================================================================
bool hasExtension(const char *string, const char *extension)
{
   const char *loc = strrchr(string, '.');
   if (!loc)
      return (false);

   const char *loc2 = strstr(string, extension);
   if (!loc2)
      return (false);

   if (loc != loc2-1)
      return (false);

   if (loc2 != (string + strlen(string)-4))
      return (false);

   return (true);

}


#ifdef UNICODE
//==============================================================================
// trimExtension(char *string)
//==============================================================================
bool trimExtension(const char *string)
{
   char *loc = strrchr(string, '.');
   if (!loc)
      return (false);

   *loc = 0;

   return (true);
}
#endif

//==============================================================================
// trimExtension(BCHAR_T *string)
//==============================================================================
bool trimExtension(const BCHAR_T *string)
{
   BCHAR_T *loc = const_cast<BCHAR_T*>(bcsrchr(string, B('.')));
   if (!loc)
      return (false);

   *loc = 0;

   return (true);
}


//==============================================================================
DWORD getPlatformID(void)
{
#ifdef XBOX
   return 0;
#else
   OSVERSIONINFO           osVer;

   osVer.dwOSVersionInfoSize = sizeof(osVer);
   if (!GetVersionEx(&osVer))
   {
      return 0 ;
   }

   return osVer.dwPlatformId;
#endif   

} // getPlatformID

//==============================================================================
//
//==============================================================================
void getOSDXVersion(DWORD &dwDXVersion, DWORD &dwDXPlatform, DWORD &dwMajorVersion, DWORD &dwMinorVersion)
{
#ifdef XBOX
   dwDXVersion = 0x900;
#else
   OSVERSIONINFO osVer;

   dwDXVersion = 0;
   dwDXPlatform = 0;

   // determine OS

   osVer.dwOSVersionInfoSize = sizeof(osVer);

   if (!GetVersionEx(&osVer))
      return;

   dwDXPlatform = osVer.dwPlatformId;

   dwMajorVersion = osVer.dwMajorVersion;

   dwMinorVersion = osVer.dwMinorVersion;
  
   // NOTE: Since we compile with DX8.1, any earlier version (even DX8.0) is likely to explode
   //       Therefore, we only care about DX8.1 being installed


   // Simply see if D3D8.dll exists.
/*
   HINSTANCE hD3D8DLL = LoadLibrary( "D3D8.DLL" );

   if(!hD3D8DLL)
      return;

   dwDXVersion = 0x800;

   FreeLibrary( hD3D8DLL );
*/
   // DX8.1 check

   // Simply see if dpnhpast.dll exists.
   HINSTANCE hDPNHPASTDLL = LoadLibrary(B("dpnhpast.dll"));
   
   if(!hDPNHPASTDLL) 
     return;

   FreeLibrary(hDPNHPASTDLL);

   // dpnhpast.dll exists. We must be at least DX8.1
   dwDXVersion = 0x801;
#endif   

} // getOSDXVersion

//=============================================================================
// maxQuadtreeLevel(long xTiles, long zTiles, long desiredLeafTiles)
//
// Returns the number of levels deep the quadtree should be given the dimensions
// xTiles and zTiles and the desired number of tiles in a leaf node
//=============================================================================
long maxQuadtreeLevel(long xTiles, long zTiles, long desiredLeafTiles)
{
   if(xTiles*zTiles <= desiredLeafTiles)
      return(0);

   // First, compute the square root of the desired number of leaf tiles.
   // This is the number of tiles in each direction we'd want if the world was square.
   long sqrtDesiredLeafTiles = (long)sqrt((float)desiredLeafTiles);
   // Use this number to determine a number of levels desired in each direction
   // by determining the number of times we'd have to divide in that direction to
   // get sqrtNumberLeafTiles.
   long xLevel = logBase2(xTiles/sqrtDesiredLeafTiles);
   long zLevel = logBase2(zTiles/sqrtDesiredLeafTiles);

   // If the result is the same for both directions, we're done
   if(xLevel == zLevel)
      return xLevel;

   long minLevel, maxLevel;
   if(xLevel < zLevel)
   {
      minLevel = xLevel;
      maxLevel = zLevel;
   }
   else
   {
      minLevel = zLevel;
      maxLevel = xLevel;
   }

   long resultDifference = INT_MAX;
   long resultLevel = 0;
   for(long level=minLevel; level<=maxLevel; level++)
   {

      // Get two raised to the number of levels -- this is the factor we'd
      // divide each dimension by if we used this level
      long twoToLevel = twoTo(level);   // 2^(number levels)

      // Get the number of polys per leaf node we'd get if we used this
      // number of levels
      long thisXTiles = xTiles/twoToLevel;
      long thisZTiles = zTiles/twoToLevel;
      if(thisXTiles < 1 || thisZTiles < 1)
         continue;
      long leafTiles = thisXTiles*thisZTiles;

      // Compute the differences between these results and the desired number of leaf mPolys
      long difference = abs(leafTiles-desiredLeafTiles);

      // Compare the differences and return the level which gives us the smallest 
      //   difference (i.e. the level that will result in a number of tiles closest to
      // the desired number)
      if(difference < resultDifference)
      {
         resultDifference = difference;
         resultLevel = level;
      }
   }

   return(resultLevel);
}


//=============================================================================
// zeroBuffSkip
//=============================================================================
void zeroBuffSkip(BYTE *buffer, DWORD buffStart, DWORD buffEnd, DWORD skipStart, DWORD skipEnd)
{
   if(skipStart>buffEnd)
      return;
   if(skipEnd<buffStart)
      return;
   
   DWORD adjustedStart = buffStart;
   if(skipStart > buffStart)
      adjustedStart = skipStart;
      
   DWORD adjustedEnd = buffEnd;
   if(skipEnd < buffEnd)
      adjustedEnd = skipEnd;
   DWORD count = adjustedEnd-adjustedStart+1;
      
   memset(buffer+(adjustedStart-buffStart), 0, count);
}


//=============================================================================
// getFileCRC32
//=============================================================================
DWORD getFileCRC32(long dirID, const BCHAR_T *filename, DWORD initialCRC /*=0*/, const DWORD *key, DWORD skipOffset, DWORD skipLength)
{
   BASSERT(filename);

   DWORD crc = initialCRC;
   
   // jce [7/15/2002] -- i wish this took a wchar...
   BFile file;
   DWORD currOffset = 0;
   DWORD skipEndOffset = skipOffset + skipLength - 1;
   if (file.openReadOnly(dirID, filename, 0) == true)
   {
      const long cBufSize = 1024U * 1024U;
      BByteArray buffer(cBufSize);
      
      DWORD size;

      size = file.readEx(buffer.getPtr(), cBufSize);
      if(skipLength)
         zeroBuffSkip(buffer.getPtr(), currOffset, currOffset+size-1, skipOffset, skipEndOffset);
      currOffset += size;
      
      while (size != 0)
      {
         CalculateCRC32(crc, buffer.getPtr(), size);

         // If we have an extra "key" then xor that in to the CRC.
         if(key)
            crc ^= *key;

         if (size != cBufSize)
            break;

         size = file.readEx(buffer.getPtr(), cBufSize);
         if(skipLength)
            zeroBuffSkip(buffer.getPtr(), currOffset, currOffset+size-1, skipOffset, skipEndOffset);
         currOffset += size;
      }

      file.close();
   }
   else
   {
      gConsoleOutput.warning("getFileCRC32: Failed opening file \"%s\"\n", filename);
   }

   return(crc);
}

//=============================================================================
// getLocalFileCRC32
//
// This function bypasses XFS and is intended to handle files directly on the
// console.
//=============================================================================
DWORD getLocalFileCRC32(const BCHAR_T *path, DWORD initialCRC, const DWORD *key)
{
   DWORD crc = initialCRC;

   FILE* pFile=NULL;
   fopen_s(&pFile, path, "rb");
   if(pFile)
   {
      const long cBufSize = 1024U * 1024U;
      BByteArray buffer(cBufSize);
            
      DWORD size = fread(buffer.getPtr(), 1, cBufSize, pFile);

      while (size != 0)
      {
         CalculateCRC32(crc, buffer.getPtr(), size);

         // If we have an extra "key" then xor that in to the CRC.
         if(key)
            crc ^= *key;

         if (size != cBufSize)
            break;

         size = fread(buffer.getPtr(), 1, cBufSize, pFile);
      }

      fclose(pFile);
   }
   else
   {
      gConsoleOutput.error("getLocalFileCRC32: Failed opening file \"%s\"\n", path);
   }

   return(crc);
}

//=============================================================================
// findFirstNonWhitespace
//=============================================================================
const char *findFirstNonWhitespace(const char *str)
{
   // Find first non-whitespace char.
   long pos;
   for(pos=0; str[pos]!='\0' && (str[pos]==' ' || str[pos]=='\t' || str[pos]==13 || str[pos]==10); pos++)
   {
      // spin.
   }

   return(str+pos);
}



//=============================================================================
// rtrimWhitespace
//=============================================================================
void rtrimWhitespace(char *str)
{
   // Get len.
   long len=strlen(str);

   // Skip if zero length.
   if(len<1)
      return;

   // Walk from right hand side.
   long pos;
   for(pos=len-1; pos>=0 && (str[pos]==' ' || str[pos]=='\t' || str[pos]==13 || str[pos]==10); pos--)
   {
      // spin.
   }
   
   // Chop string.
   str[pos+1]='\0';
}

//=============================================================================
// ltrimWhitespace
//=============================================================================
void ltrimWhitespace2(char *str)
{
   // Find first non-whitespace char.
   long pos;
   for(pos=0; str[pos]!='\0' && (str[pos]==' ' || str[pos]=='\t' || str[pos]==13 || str[pos]==10); pos++)
   {
      // spin.
   }

   // Bail now if empty.
   if(pos==0)
      return;

   // Move string down.
   for(long i=0; str[i]!='\0'; i++)
      str[i]=str[i+pos];
}

//==============================================================================
// allTrimWhitespace
//==============================================================================
void allTrimWhitespace(char *str)
{
   ltrimWhitespace2(str);
   rtrimWhitespace(str);
}


//==============================================================================
// leftShiftForMask
//==============================================================================
UINT leftShiftForMask(DWORD mask)
{
	UINT i=0;
	DWORD bitMask = 1;
	while(i<sizeof(DWORD)*8 && !(mask & bitMask))
	{
		i++;
		bitMask = bitMask << 1;
	}
	return i;
} // leftShiftForMask

//==============================================================================
// convertTo565
//==============================================================================
#if _MSC_VER < 1400
#pragma optimize( "atgw", on )
#endif

void convertTo565(WORD *dest, DWORD *src, DWORD num)
{
   DWORD y = 0;
	WORD *pdest = dest;
   BYTE *psrc = (BYTE *) src;

   #pragma warning(disable: 4244)

   do
   {
      *pdest = ((psrc[0]&0xF8)>>3)|((psrc[1]&0xFC)<<3)|((psrc[2]&0xF8)<<8);

      pdest++;
      psrc+= 4;
      y++;
   } while (y < num);

   #pragma warning(default: 4244)
} // convertTo565

#if _MSC_VER < 1400
#pragma optimize( "", on )
#endif

//==============================================================================

//==============================================================================
#if _MSC_VER < 1400
#pragma optimize( "atgw", on )
#endif

void convertTo555(WORD *dest, DWORD *src, DWORD num)
{
   DWORD y = 0;
	WORD *pdest = dest;
	BYTE *imageData = (BYTE *) src;
	UINT rShift = 10;
	UINT gShift = 5;

	UINT rightShift = 3;

   #pragma warning(disable: 4244)

   do
	{
		*pdest = (imageData[0] >> rightShift);
		*pdest |= ((imageData[1] >> rightShift) << gShift);
		*pdest |= ((imageData[2] >> rightShift) << rShift);

      y++;
      imageData += 4;
      pdest++;

   } while (y < num);

   #pragma warning(default: 4244)
} // convertTo555

#if _MSC_VER < 1400
#pragma optimize( "", on )
#endif

//==============================================================================
// convertTo4444
//==============================================================================
#if _MSC_VER < 1400
#pragma optimize( "atgw", on )
#endif

void convertTo4444(WORD *dest, DWORD *src, DWORD num)
{
   DWORD y = 0;
	WORD *sp = dest;
	BYTE *imageData = (BYTE *) src;

   #pragma warning(disable: 4244)

   do 
	{
		*sp = ((imageData[0] & 0xF0) >> 4)|(imageData[1] & 0xF0)|((imageData[2] & 0xF0) << 4)|((imageData[3] & 0xF0) << 8);

      y++;
      imageData += 4;
      sp++;

   } while (y < num);

   #pragma warning(default: 4244)
} // convertTo4444

#if _MSC_VER < 1400
#pragma optimize( "", on )
#endif

//==============================================================================
// convertTo1555
//==============================================================================
#if _MSC_VER < 1400
#pragma optimize( "atgw", on )
#endif

void convertTo1555(WORD *dest, DWORD *src, DWORD num)
{
   DWORD y = 0;
	WORD *sp = dest;
	BYTE *imageData = (BYTE*)src;
	UINT aShift = 15;
	UINT rShift = 10;
	UINT gShift = 5;

	UINT aRightShift = 7;
	UINT rightShift = 3;

   #pragma warning(disable: 4244)

	do
	{
		*sp = (imageData[0] >> rightShift);
		*sp |= ((imageData[1] >> rightShift) << gShift);
		*sp |= ((imageData[2] >> rightShift) << rShift);
		*sp |= ((imageData[3] >> aRightShift) << aShift);
   
      y++;
      imageData += 4;
      sp++;

   } while (y < num);

   #pragma warning(default: 4244)

} // convertTo1555

#if _MSC_VER < 1400
#pragma optimize( "", on )
#endif

//==============================================================================
// countOnes
//==============================================================================
UINT countOnes(DWORD dw)
{
	UINT count=0;
	for(long i=0; i<sizeof(DWORD)*8; i++)
	{
		if(dw & 1)
			count++;
		dw=dw>>1;
	}
	return count;
} // countOnes

//=============================================================================
// filterIntoVectorArray
//
// Will take the scrPts Array and filter it into destPts using the pFilter
// The Skipsize will skip over that many elements in the src when doing the
// filtering. - JER
// FilterEnds tells us that we can wrap over the end to filter the endpoints.
//=============================================================================
void filterIntoVectorArray(const BVector* srcPts, long numSrcPts, BDynamicSimVectorArray& destPts, const float* pFilter, long numFilterElements, 
                           long SkipSize, bool filterEnds /* =true */)
{
   BASSERT(pFilter);
   if(!pFilter)
      return;

   BASSERT(srcPts);
   if(!srcPts)
      return;

   BASSERT(numSrcPts);
   if(!numSrcPts)
      return;

   //-- filter the points
   float recipFilterSize = 0.0f;
   for(long s=0; s<numFilterElements; s++)
   {
      recipFilterSize += pFilter[s];
   }
   recipFilterSize = 1.0f / recipFilterSize;

   BASSERT(SkipSize >= 1);
   if(SkipSize < 1)
      SkipSize = 1;

   BDynamicSimVectorArray origPts;
   for(long n=0; n<numSrcPts; n+=SkipSize)
   {
      origPts.add(srcPts[n]);
   }
   
   long numPts = origPts.getNumber();
   long p;
   
   long* pIndices = new long[numFilterElements];
   if(!pIndices)
      return;
  
   //-- walk each point, filtering and looping through the end if wanted
   long start = 0;
   long end = numPts;
   if( !filterEnds )
   {
      start += (numFilterElements/2);
      end   -= (numFilterElements/2);
   }

   destPts.setNumber(numPts);

   // Cannot filter, just return the same data
   if(start >= end)
   {
      for(p = 0; p < numPts; p++)
      {
         destPts[p].x = origPts[p].x;
         destPts[p].y = origPts[p].y;
         destPts[p].z = origPts[p].z;
      }
   }
   else
   {
      for(p = 0; p < numPts; p++)
      {
         float x = 0.0f, y = 0.0f, z = 0.0f;
         if((p < start) || (p >= end)) // keep the start and end as they are
         {
            x = origPts[p].x;
            y = origPts[p].y;
            z = origPts[p].z;
         }
         else
         {
            for(long s=0;s<numFilterElements;s++)
            {
               pIndices[s] = p + s + (-numFilterElements/2);

               if(pIndices[s]<0)
                  pIndices[s] += numPts;
               if(pIndices[s]>=numPts)
                  pIndices[s] -= numPts;
            }

            for(long s=0;s<numFilterElements;s++)
            {
               x+=origPts[pIndices[s]].x*pFilter[s];
               y+=origPts[pIndices[s]].y*pFilter[s];
               z+=origPts[pIndices[s]].z*pFilter[s];
            }
            x *= recipFilterSize;
            y *= recipFilterSize;
            z *= recipFilterSize;
         }

         destPts[p].x = x;
         destPts[p].y = y;
         destPts[p].z = z;
      }
   }

   delete [] pIndices;
}

//==============================================================================
// getCubTgaNames
//==============================================================================
bool getCubTgaNames(long dirID, const char *cubFilename, char cubTgaNames[6][_MAX_PATH])
{
   // Make sure params are valid.
   if(!cubFilename)
   {
      BASSERT(0);
      return(false);
   }

   // Open it up.
   BFile file;
   if(!file.openReadOnly(dirID, BString(cubFilename)))
      return(false);

   // Read the lines out of the file.
   char line[1024];
   long num=0;
   while(file.fgets(line, 1024) && num<6)
   {
      // Skip leading whitespace.
      char *temp=(char *)findFirstNonWhitespace(line);

      // Skip comment.
      if(temp[0]=='/' && temp[1]=='/')
         continue;

      // Trim trailing whitespace.
      rtrimWhitespace(temp);

      // Skip empty line.
      if(strlen(temp) < 1)
         continue;

      // Copy filename.
      BString dir;
      gFileManager.getDirListEntry(dir, dirID);
      bsnprintf(cubTgaNames[num], _MAX_PATH, "%s%s.tga", dir.getPtr(), temp);
      num++;
   }

   // Clean up.
   file.close();

   // If we have exactly 6 names, we have success.
   if(num==6)
      return(true);

   // Fewer than 6 names read, so we failed.
   return(false);
}

namespace Utils
{
   //=============================================================================
   // Utils::IsValidFloat
   //=============================================================================
   bool IsValidFloat(float i)
   {
      const unsigned int bits = *reinterpret_cast<unsigned int*>(&i);
      const unsigned int exponent = (bits >> 23) & 0xFF;
      if (exponent == 255) 
      {
         // If mantissa is 0 the value is infinity, otherwise it's a NaN.
         return false;
      }	  
      const unsigned int mantissa = bits & ((1 << 23) - 1);
      if ((exponent == 0) && (mantissa != 0))
      {
         // The value is a denormal.
         return false; 
      }
      return true;
   }
}

//==============================================================================
// Reverses the bits of i without using lookup tables.
// From http://www.df.lth.se/~john_e/fr_gems.html
//==============================================================================
unsigned int reverseBits(unsigned int i)
{
   // FIXME: Untested!
   unsigned int j = 0;
   for (int l = 0; l < 32; l++)
   {
      j <<= 1;
      if (i & (1 << l))
         j |= 1;
   }
   return j;
}
   
//=============================================================================
// eof: util.cpp
//=============================================================================


