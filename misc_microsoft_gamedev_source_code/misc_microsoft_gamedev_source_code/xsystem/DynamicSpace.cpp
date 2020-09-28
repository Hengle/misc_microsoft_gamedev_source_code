//==============================================================================
// DynamicSpace.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "chunker.h"
#include "DynamicSpace.h"

//==============================================================================
// Defines
struct SaveHeader
{
   long tag;
   long checksum;
   long count;
};

//==============================================================================
// BDynamicSpace::BDynamicSpace
//==============================================================================
BDynamicSpace::BDynamicSpace(long tag, long itemSize, long extentCount) :
   mTag(tag),                   
   mItemSize(itemSize),
   mExtentCount(extentCount), 
   mAllocationCount(0),             
   mUsedCount(0),
   mpSpace(NULL)
{
} // BDynamicSpace::BDynamicSpace

//==============================================================================
// BDynamicSpace::~BDynamicSpace
//==============================================================================
BDynamicSpace::~BDynamicSpace(void)
{
   if (mpSpace)
   {
      delete mpSpace;
      mpSpace = NULL;
      mAllocationCount = 0;            
      mUsedCount = 0;
   }
} // BDynamicSpace::~BDynamicSpace

//==============================================================================
// BDynamicSpace::setAllocationCount
//==============================================================================
bool BDynamicSpace::setAllocationCount(
   bool * pMoved,
   long count)
{
   // 8/20/00 - ham - rewritten to fix dangling ptr problem

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);

   if (pMoved)
      *pMoved = false;

   if (count < mUsedCount)
   {
      blog("Attempt to set DynamicSpace count too small");
      return true;
   }

   if (count == 0)
   {
      if (mpSpace)
      {
         delete mpSpace;               // free the old buffer
         mpSpace = NULL;
         if (pMoved)
            *pMoved = true;
      }
      mAllocationCount = 0;
      return true;
   }

   if (count <= mAllocationCount) //size is ok
   {
      return true;
   }

   unsigned char *pTemp;

   BASSERT(count != mAllocationCount);
   pTemp = new unsigned char[count * mItemSize];    
   if (pTemp)
   {
      if (mpSpace)            // we have a buffer
      {
         if (mUsedCount)      // we have data
            memcpy(pTemp, mpSpace, mUsedCount * mItemSize);
         delete mpSpace;      // free the old buffer
         mpSpace = NULL;
         if (pMoved)
            *pMoved = true;
      }
      mpSpace = pTemp;        // save the new buffer and count
      mAllocationCount = count;

      return true;
   }
            
   {setBlogError(3884); blogerror("setAllocationCount failed because of a new failure");}
   return false;

} // BDynamicSpace::setAllocationCount

//==============================================================================
// BDynamicSpace::add - add a zero filled entry.
//==============================================================================
long BDynamicSpace::add(
   bool * pMoved)
{
   long lResult = BDynamicSpace::AddFailed;

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);
   
   // clear it - it will only change if setAllocationCount changes it
   if (pMoved)
      *pMoved = false;

   if ((1 + mUsedCount) > mAllocationCount)            // if it doesn't fit
   { 
      BASSERT(1 <= mExtentCount);   // add must be smaller than extent
      if (! setAllocationCount(pMoved, mAllocationCount + mExtentCount))
      {
         {setBlogError(3885); blogerror("Unable to set the DynamicSpace size");}
         goto bail;
      }
   }

   memset(mpSpace+(mUsedCount*mItemSize),0,mItemSize);
   lResult = mUsedCount;
   mUsedCount += 1;
   
   BASSERT(mUsedCount <= mAllocationCount);

bail:
   return lResult;
}

//==============================================================================
// BDynamicSpace::add
//==============================================================================
long BDynamicSpace::add(
   bool * pMoved,
   const void * ptr, 
   long numberToAdd)
{
   long lResult = BDynamicSpace::AddFailed;

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);
   
   // clear it - it will only change if setAllocationCount changes it
   if (pMoved)
      *pMoved = false;

   if ((numberToAdd + mUsedCount) > mAllocationCount)            // if it doesn't fit
   { 
      BASSERT(numberToAdd <= mExtentCount);   // add must be smaller than extent
      if (! setAllocationCount(pMoved, mAllocationCount + mExtentCount))
      {
         {setBlogError(3886); blogerror("Unable to set the DynamicSpace size");}
         goto bail;
      }
   }

   memcpy(mpSpace+(mUsedCount*mItemSize),ptr,numberToAdd*mItemSize);
   lResult = mUsedCount;
   mUsedCount += numberToAdd;
   
   BASSERT(mUsedCount <= mAllocationCount);

bail:
   return lResult;
} // BDynamicSpace::add

//==============================================================================
// BDynamicSpace::checksum
//==============================================================================
long BDynamicSpace::checksum(void) const
{
   long lResult = 0;

   BASSERT(mItemSize > 0);
   BASSERT(mTag != 0);
   if (mUsedCount)
   {
      BASSERT(mpSpace != 0);
      lResult = dataChecksum(mpSpace, mUsedCount * mItemSize, mTag);
   }
   else
      lResult = 0;

   return lResult;
} // BDynamicSpace::checksum


//==============================================================================
// BDynamicSpace::dataChecksum
//==============================================================================
long BDynamicSpace::dataChecksum(const void * ptr, long length, long initial)
{
   long           lResult;
   long           lTemp;
   long           lByteCount;
   unsigned char  uc;
   const unsigned char *pTemp;

   lResult = ~initial;     // hidden initial value to make editing binaries harder
   for (pTemp = (const unsigned char *) ptr, lByteCount = length; 
        lByteCount >= 4; 
        pTemp += 4, lByteCount -= 4)
   {
      lTemp = *(long *)(pTemp);
      #ifdef _BIGENDIAN_
         trace("Swap the bytes to little endian first");
      #endif
      lResult ^= lTemp;
   }
   for( ; lByteCount > 0 ; pTemp++, lByteCount-- )   // get the remaining bytes
   {
      uc = *pTemp;
      lResult += (long) uc;
   }
   return lResult;
} // BDynamicSpace::dataChecksum



//==============================================================================
// BDynamicSpace::save
//==============================================================================
bool BDynamicSpace::save(BFile *pfile)
{
   bool              bResult = false;
   struct SaveHeader saveHeader;
   size_t            written;

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);
   
   saveHeader.tag       = mTag;
   saveHeader.checksum  = checksum();
   saveHeader.count     = mUsedCount;

   written = pfile->write(&saveHeader, sizeof(saveHeader));

   if (written != 1)
   {
      {setBlogError(3887); blogerror("failed saving DynamicSpace save header");}
      goto bail;
   }
   if (mUsedCount)
   {
      written = pfile->write(mpSpace, mItemSize * mUsedCount);

      if (written != 1)
      {
         {setBlogError(3888); blogerror("failed saving DynamicSpace data");}
         goto bail;
      }
   }

   bResult = true;

bail:
   return bResult;
} // BDynamicSpace::save

//==============================================================================
// BDynamicSpace::load
//==============================================================================
bool BDynamicSpace::load(BFile *pfile)
{
   bool              bResult = false;
   struct SaveHeader saveHeader;

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);
   
   // Don't want to use any old data
   clearData();

   // read the header
   if (pfile->read(&saveHeader, sizeof(saveHeader)) == false)
   {
      {setBlogError(3889); blogerror("failed loading DynamicSpace save header - reading the header");}
      goto bail;
   }
   if (saveHeader.tag != mTag)
   {
      {setBlogError(3890); blogerror("failed loading DynamicSpace save header - tag error");}
      goto bail;
   }
   
   // make space and load the new data
   clearData();   // allows us to load a smaller number of units
   if (! setAllocationCount(NULL, saveHeader.count))
   {
      {setBlogError(3891); blogerror("failed loading DynamicSpace save header - setting the allocation count");}
      goto bail;
   }

   // read the data
   if (saveHeader.count)
   {
      if (pfile->read(mpSpace, mItemSize * saveHeader.count))
      {
         {setBlogError(3892); blogerror("failed loading DynamicSpace save header - reading the data");}
         goto bail;
      }
   }
   mUsedCount = saveHeader.count;

   // check the checksum
   if (saveHeader.checksum != checksum())
   {
      {setBlogError(3893); blogerror("failed loading DynamicSpace save header - checksum on the new data");}
      goto bail;
   }

   bResult = true;

bail:
   return bResult;
} // BDynamicSpace::load

//==============================================================================
// BDynamicSpace::save
//==============================================================================
bool BDynamicSpace::save(BChunkWriter *writer)
{   
   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);

   long pos;

   long result=writer->writeTagPostSized(BCHUNKTAG("DS"), pos);
   if (!result)
   {
      {setBlogError(3894); blogerror("BDynamicSpace::save-- error writing tag");}
      return(false);
   }   
  
   // write header   
   CHUNKWRITESAFE(writer, Long, mTag);
   CHUNKWRITESAFE(writer, Long, checksum());
   CHUNKWRITESAFE(writer, Long, mUsedCount);   
   
   if (mUsedCount)   
   {
      CHUNKWRITEARRAYSAFE(writer, BYTE, mItemSize*mUsedCount, mpSpace);         
   }

   //Finish chunk.   
   result=writer->writeSize(pos);
   if (!result)
   {
      {setBlogError(3895); blogerror("BDynamicSpace::save - error with writeSize");}
      return(false);
   }

   return true;
} // BDynamicSpace::save

//==============================================================================
// BDynamicSpace::load
//==============================================================================
bool BDynamicSpace::load(BChunkReader *reader)
{      
   long tag, csum, count;

   BASSERT(mItemSize > 0);
   BASSERT(mExtentCount > 0);
   BASSERT(mTag != 0);
   
   // Don't want to use any old data
   clearData();

   // read the header
   long result=reader->readExpectedTag(BCHUNKTAG("DS"));
   if (!result)
   {
      {setBlogError(3896); blogerror("BSymbolTable::load -- error reading tag");}
      return(false);
   }   

   // read the header   
   CHUNKREADSAFE(reader, Long, tag);
   CHUNKREADSAFE(reader, Long, csum);
   CHUNKREADSAFE(reader, Long, count);
   
   if (tag != mTag)
   {
      {setBlogError(3897); blogerror("failed loading DynamicSpace save header - tag error");}
      return false;
   }
      
   if (! setAllocationCount(NULL, count))
   {
      {setBlogError(3898); blogerror("failed loading DynamicSpace save header - setting the allocation count");}
      return false;
   }

   // read the data
   if (count)
   {
      long tcount = mItemSize*count;
      CHUNKREADARRAYSAFE(reader, BYTE, &tcount, mpSpace, mItemSize*count);      
      if (tcount != count)
      {
         {setBlogError(3899); blogerror("failed loading mpSpace in DynamicSpace load");}
         return false;
      }
   }
   mUsedCount = count;

   //Validate our reading of the chunk.
   result=reader->validateChunkRead(BCHUNKTAG("DS"));
   if (!result)
   {
      {setBlogError(3900); blogerror("BSymbolTable::load didn't validate chunk correctly");}
      return(false);
   }

   // check the checksum
   if (csum != checksum())
   {
      {setBlogError(3901); blogerror("failed loading DynamicSpace save header - checksum on the new data");}
      return false;
   }

   return true;
} // BDynamicSpace::load



//==============================================================================
// BDynamicSpace::copyData
//==============================================================================
long BDynamicSpace::copyData(unsigned char * buffer)
{
   long copied = 0;
   const unsigned char * p;

   p = static_cast<const unsigned char *>(getPointer(0));
   if (p)
   {
      memcpy(buffer, p, mUsedCount * mItemSize);
      copied = mUsedCount * mItemSize;
   } 

   return copied;
} // BDynamicSpace::copyData


//==============================================================================
// eof: DynamicSpace.cpp
//==============================================================================
