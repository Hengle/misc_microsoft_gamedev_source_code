//============================================================================
//
//  StaticBitStateArray.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "terrainPCH.h" //PCH
#include "TerrainMetric.h"

#include "StaticBitStateArray.h"

//--------------------------------
#include <ppcintrinsics.h>
#define cNumBits	   64
#define cSecAnd		cNumBits-1
#define cPrimShift	6           // shifting eqivilant of doing x/cNumBits
#define cTopMask	   0x8000000000000000
#define cMaxPos		0x7FFFFFFFFFFFFFFF
#define cMinPos		0xFFFFFFFFFFFFFFFE
#define cFull        0xFFFFFFFFFFFFFFFF
#define cEmpty       0x0000000000000000
//--------------------------------
BStaticBitStateArray::BStaticBitStateArray():
mpArray(0),
mNumPrimary(0),
mNumElements(0)
{

}
//--------------------------------
BStaticBitStateArray::~BStaticBitStateArray()
{
   destroy();
}
//--------------------------------
void BStaticBitStateArray::destroy(void)
{
   if(mpArray)
   {
      delete [] mpArray;
      mpArray=0;
   }
}
//--------------------------------
void BStaticBitStateArray::init(int numElements,bool defaultState/*=true*/)
{
   mNumElements = numElements;
   mNumPrimary = (numElements >> cPrimShift)+1;
   mpArray = new __int64[mNumPrimary];
   setAll(defaultState);
   BTerrainMetrics::addCacheMemCPU(sizeof(__int64) * mNumPrimary);
}
//--------------------------------
void BStaticBitStateArray::setState(int index, bool state)
{
   int sIndex = (index & cSecAnd);
   int pIndex = index >> cPrimShift;
   setContainerState(mpArray[pIndex],state,(cTopMask>>sIndex));
}
//--------------------------------
inline void BStaticBitStateArray::setContainerState(__int64 &value, bool state, __int64 mask)
{
   const int istate = state;
   value ^= (-istate ^ value) & mask;
}
//--------------------------------
void  BStaticBitStateArray::setSpanState(int index, int stride, bool state)
{
   int pIndexStart = index >> cPrimShift;
   int sIndexStart = (index & cSecAnd);
   int pIndexEnd = (index+stride) >> cPrimShift;
   int sIndexEnd = ((index+stride) & cSecAnd);

   if(pIndexEnd == pIndexStart)
   {
      __int64 msk = makeSpanBitMask((byte)sIndexStart, (byte)cNumBits);
      setContainerState(mpArray[pIndexStart],state,msk);
   }
   else
   {

      //handle the edge cases first
      __int64 msk = makeSpanBitMask((byte)sIndexStart, (byte)cNumBits);
      setContainerState(mpArray[pIndexStart], state, msk);

      for(int i=pIndexStart+1;i<pIndexEnd;i++)
      {
         const int istate = state;
         mpArray[i] = -istate;
      }


      // end case
      msk = makeSpanBitMask(0, (byte)sIndexEnd);
      setContainerState(mpArray[pIndexEnd], state, msk);
   }
}
//--------------------------------
bool BStaticBitStateArray::isSet(int index)
{
   int pIndex = index >> cPrimShift;
   int sIndex = (index & cSecAnd);

   return (mpArray[pIndex] & (cTopMask>>sIndex)) != 0;
}
//--------------------------------
void BStaticBitStateArray::setAll(bool onOff)
{
   const int iOnOff = onOff;
   for(int i=0;i<mNumPrimary;i++)
      mpArray[i] = -iOnOff;
}
//--------------------------------
int BStaticBitStateArray::giveNextFALSE(int startIndex)
{
   if(startIndex >= mNumElements || startIndex<0)
      return -1;
   int retValue =0;
   int primStart = startIndex >> cPrimShift;
   int sIndex = (startIndex & cSecAnd);
   __int64 msk = makeSpanBitMask((byte)sIndex,(byte)cNumBits);

   for(int i=primStart;i<mNumPrimary;i++)
   {
      __int64 v = mpArray[i];
      if(v != cFull)
      {
         int numZeros = _CountLeadingZeros64( (~v)&msk );
         if(numZeros==cNumBits)
            continue;

         retValue= (i*cNumBits) + numZeros;
         if(retValue<mNumElements)
            return retValue;
         return -1;
      }
      msk = cFull;
   }
   return -1;
}
//--------------------------------
int BStaticBitStateArray::giveNextTRUE(int startIndex)
{
   if(startIndex >= mNumElements || startIndex<0)
      return -1;
   
   int retValue = 0;
   int primStart = startIndex >> cPrimShift;
   int sIndex = (startIndex & cSecAnd);
   __int64 msk = makeSpanBitMask((byte)sIndex,(byte)cNumBits);

   for(int i=primStart;i<mNumPrimary;i++)
   {
      __int64 v = mpArray[i];
      if(v)
      {
         int numZeros = _CountLeadingZeros64( v&msk );
         if(numZeros==cNumBits)
            continue;

         retValue= (i*cNumBits) + numZeros;
         if(retValue<mNumElements)
            return retValue;
         return -1;
      }
      msk = cFull;
   }
   return -1;
}
//--------------------------------
__int64 BStaticBitStateArray::makeSpanBitMask(byte start, byte end)
{

   byte l = (end-start);
   byte es = (cNumBits-end);

   return (cFull >> (cNumBits-l)) & (-1<<es);
}
//--------------------------------
void BStaticBitStateArray::doPrecache()
{
   //Prefetching more than 1024 bytes in advance would not help 
   //since there can only be eight cache line reads being processed simultaneously
   int memToGoTo=mNumPrimary*sizeof(__int64);
   if(memToGoTo > 1024) memToGoTo = 1024;
   for( int i = 0; i < memToGoTo; i += 128 )
      __dcbt( i, mpArray );
}

