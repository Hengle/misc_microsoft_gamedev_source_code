//==============================================================================
// bitarray.cpp
//
// Copyright (c) 1998-2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "bitarray.h"
#include "chunker.h"

//==============================================================================
// Defines

//==============================================================================
// Static variables
const DWORD BBitArray::msSaveVersion=0;
DWORD BBitArray::msLoadVersion=0xFFFF;


//=============================================================================
// BBitArray::~BBitArray(void)
//=============================================================================
BBitArray::~BBitArray()
{
   if (mBits)
   {
      delete [] mBits;
      mBits = NULL;
   }
   else
   {
      BDEBUG_ASSERT(0 == mNumberBytes);
   }
} // BBitArray::~BBitArray


//=============================================================================
// BBitArray::setNumber(long newNumber, bool force)
//
// Sets the number of bits to the desired number.  If force is specified, space 
// for the exact number of requested bits is allocated.  Otherwise the array
// size is doubled as needed (and never contracted).
//=============================================================================
bool BBitArray::setNumber(long newNumber, bool force)
{
   if(force && newNumber<=0)
   {
      if(mBits)
      {
         delete []mBits;
         mBits = NULL;
      }
      mNumber = 0;
      mNumberBytes = 0;
      return(true);
   }

   // Adjust from # of bits to # of bytes
   DWORD numberBytes = newNumber >> 3;
   if (newNumber & 7)
      numberBytes++;

   // If we are not forcing and there is already enough space,
   // just bail out now.
   if(!force && numberBytes<=mNumberBytes)
   {
      mNumber = newNumber;
      return(true);
   }

   // Figure out the number of bytes we want to allocate
   if(force)
   {
      // If we already have the requested number of bytes, we're done.
      if(mNumberBytes == numberBytes)
      {
         mNumber = newNumber;
         return(true);
      }

      // Otherwise, force the new number of bytes to the appropriate amount.
      mNumberBytes = numberBytes;
   }
   else
   {
      // Grow the array.
      if(mNumberBytes < 1)
         mNumberBytes = 1;
      while(numberBytes > mNumberBytes)
         mNumberBytes *= 2;
   }

   unsigned char *temp = new unsigned char[mNumberBytes];
   if(!temp)
      return(false);

   memset(temp, 0x00, mNumberBytes);

   if(mBits)
   {
      DWORD currentNumberBytes = mNumber>>3;
      if(mNumber&7)
         currentNumberBytes++;

      DWORD numberToCopy = min(currentNumberBytes, mNumberBytes);
      memcpy(temp, mBits, numberToCopy*sizeof(unsigned char));
      delete []mBits;
   }

   mBits = temp;
   mNumber = newNumber;

   return(true);
} // BBitArray::setNumber

//==============================================================================
// BBitArray::and
//==============================================================================
void BBitArray::and(const BBitArray& a)
{
   if (mNumber != a.mNumber)
   {
      BASSERT(0);
      return;
   }

   for (DWORD i=0; i < mNumberBytes; i++)
      mBits[i]&=a.mBits[i];
}

//==============================================================================
// BBitArray::or
//==============================================================================
void BBitArray::or( const BBitArray& a)
{
   if (mNumber != a.mNumber)
   {
      BASSERT(0);
      return;
   }

   for (DWORD i=0; i < mNumberBytes; i++)
      mBits[i]|=a.mBits[i];
}

//==============================================================================
// BBitArray::writeVersion
//==============================================================================
bool BBitArray::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4002); blogerror("BBitArray::writeVersion -- bad chunkWriter");}
      return(false);
   }

   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("BA"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4003); blogerror("BBitArray::writeVersion -- failed to write version");}
      return(false);
   }

   return(true);
}


//==============================================================================
// BBitArray::readVersion
//==============================================================================
bool BBitArray::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4004); blogerror("BBitArray::readVersion -- bad chunkReader");}
      return(false);
   }

   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("BA"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4005); blogerror("BBitArray::readVersion -- failed to read version");}
      return(false);
   }

   return(true);
}

//==============================================================================
// BBitArray::save
//==============================================================================
bool BBitArray::save(BChunkWriter *chunkWriter) const
{
   if(!chunkWriter)
   {
      BASSERT(0);
      return(false);
   }

   long result=chunkWriter->writeBYTEArray(mNumberBytes, mBits);
   if (!result)
      return(false);

   return(true);
}

//==============================================================================
// BBitArray::load
//==============================================================================
bool BBitArray::load(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   //Get the number of BYTEs we want to read.
   long countExpected=0;
   long result=chunkReader->peekArrayLength(&countExpected);
   if (!result)
      return(false);
   //Set the number of bytes by taking the count (times 8 for BYTE size) and
   //calling the setNumber method that takes the number of bits.
   // ajl 6/8/01 - made it not resize down because it makes loading old scenarios not
   // work after you increase the number of flags in BUnit.
   DWORD newNumber=countExpected*8;
   if (newNumber > mNumber)
   {
      if (setNumber(newNumber, true) == false)
         return(false);
   }
   // ajl 6/8/01 - make sure the bits are all reset in case the number of flags
   // being loaded in is different.
   if (mBits)
      memset(mBits, 0x00, mNumberBytes);
   //Actually read the BYTEs.
   long countRead=0;
   result=chunkReader->readBYTEArray(&countRead, mBits, countExpected);
   if (!result)
      return(false);
   BASSERT(countRead == countExpected);

   return(true);
}

//==============================================================================
// eof: bitarray.cpp
//==============================================================================

