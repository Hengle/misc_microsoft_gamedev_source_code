//============================================================================
//  File: ContentReader.h
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "xsystem.h"
#include "contentfile.h"
#include "contentreader.h"
#include "buffercache.h"

#ifdef XBOX

//============================================================================
// This class needs buffering and async support still.
//============================================================================
BContentReader::BContentReader(BContentFile * file) :
   mFileOfs(0),
   mpCache(NULL)
{
   mpCache = new BBufferCache();
   mpContentFile = file;
}

//============================================================================
//============================================================================
BContentReader::~BContentReader()
{
   if (mpCache)
      delete mpCache;
}

//============================================================================
//============================================================================
bool BContentReader::close()
{
   if (!mpContentFile)
      return true;

   mpContentFile->close();
   return true;
}

//============================================================================
//============================================================================
BOOL BContentReader::readBytes(void* p, uint n, uint *pNumActual)
{
   // does the cache have enough for the request?
   if ( (bytesAvailable() >= n) )
   {
      // this is a simple read from the cache
      mpCache->readFromCache(p, n, *pNumActual);
      return true;
   }

   // create working variables
   uint bytesRead = 0;        
   uint bytesNeeded = n;
   void *pWorking = p;

   while (bytesNeeded > 0)
   {
      // are we at the end of the file?
      if (atEOF())
         break;            // then we are done

      // does the cache have data?
      if (bytesAvailable()==0)
         fillCache();            // fill it up

      // read from the cache
      uint numRead=0;

      // write what we can, then refill the cache
      mpCache->readFromCache(pWorking, bytesNeeded, numRead);

      // adjust our pointers
      bytesNeeded -= numRead;    // update how much is needed
      bytesRead += numRead;      // Increment how much we have read so far
      pWorking = static_cast<BYTE*>(pWorking) + bytesRead;  // move our pointer along
   }

   // return how many bytes we actually read
   *pNumActual = bytesRead;

   return true;
}

//============================================================================
//============================================================================
BOOL BContentReader::setFilePointer(uint64 filepointer)
{
   // move the file offset to the new location
   if (!mpContentFile->setFilePointer(filepointer, FILE_BEGIN))
      return false;

   mFileOfs = filepointer;

   return true;
}

//==============================================================================
// readBool
//==============================================================================
BOOL BContentReader::readBool(bool *data)
{
   uint actual=0;
   return (readBytes(data, sizeof(*data), &actual));

} //readBool

//==============================================================================
// readLong
//==============================================================================
BOOL BContentReader::readInt(int *data)
{
   uint actual=0;
#ifdef XBOX
   if (!readBytes(data, sizeof(*data), &actual))
      return false;
   EndianSwitchDWords((DWORD*)data, 1);
   return true;
#else
   return readBytes(data, sizeof(*data), &actual);
#endif
} //readLong

//==============================================================================
// readLong
//==============================================================================
BOOL BContentReader::readUint(uint *data)
{
   uint actual=0;
#ifdef XBOX
   if (!readBytes(data, sizeof(*data), &actual))
      return false;
   EndianSwitchDWords((DWORD*)data, 1);
   return true;
#else
   return readBytes(data, sizeof(*data), &actual);
#endif
} //readLong

//==============================================================================
// readBString
//==============================================================================
BOOL BContentReader::readBString(BString& dstString)
{
   uint actual = 0;
   //-- JER [1/9/2003] Read how far we need to read to get the whole string.
   int length=0;
   if(readBytes(&length, sizeof(length), &actual) == false)
      return(false);

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&length, 1);
   if(length>1024)
   {
      BASSERT(0); // possible endian switch error
      return(false);
   }
#endif

   const bool unicodeString = (length & 0x80000000) == 0;
   length &= 0x7FFFFFFF;

   //-- Handle an empty string.
   if (length <= 0)
   {
      dstString.empty();
      return(true);
   }

   if (unicodeString)
   {
      BUString temp;
      temp.makeRawString(length);

      // Read.
      if( readBytes(temp.getString(), sizeof(WCHAR) * length, &actual) == false)
         return(false);

      // Terminate the string since we don't save out the null.
      temp.getString()[length] = L'\0';

#ifdef XBOX
      temp.endianSwap();
#endif

      temp.check();

      dstString.set(temp);
   }
   else
   {
      // Manually acquire space in the string.
      dstString.makeRawString(length);

      // Read.
      if( readBytes(dstString.getString(), sizeof(BCHAR_T) * length, &actual) == false)
         return(false);

      // Terminate the string since we don't save out the null.
      dstString.getString()[length] = B('\0');
   }      

   dstString.check();

   return(true);
}

//==============================================================================
// BContentReader::atEOF
//==============================================================================
bool BContentReader::atEOF() 
{
   // the cache needs to be empty and the cache needs to have an EOF indicator on it.
   if ( (bytesAvailable()==0) && mpCache->atEOF())
      return true;

   return false; 
}

//==============================================================================
// BContentReader::fillCache
//==============================================================================
BOOL BContentReader::fillCache()
{
   // we are now done
   mpCache->resetCache();

   uint numActual;
   uint numToRead = mpCache->getCacheSize();

   // set the file pointer appropriately
   if (!mpContentFile->setFilePointer(mFileOfs, FILE_BEGIN))
      return false;

   // read the bytes into the cache.
   BOOL result = mpContentFile->read(mpCache->getPtr(), numToRead, &numActual);
   if (result && (numActual != numToRead) )
      mpCache->setEOF(true);

   // give our cache the info it needs
   mpCache->setBytesInBuffer(numActual);
   mpCache->setFilePointer(mFileOfs);

   // increment our file offset for the next read
   mFileOfs += numActual;

   return result;
}

#endif // XBOX