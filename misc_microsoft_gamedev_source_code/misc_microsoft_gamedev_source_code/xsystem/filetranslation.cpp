//============================================================================
//
//  File.cpp
//
//  Copyright 2002 Ensemble Studios
//
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"

//============================================================================
//============================================================================
//============================================================================
BFileTranslation::BFileTranslation() :
   mpStorage(NULL)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileTranslation::~BFileTranslation()
{
   if(close() == false)
   {
      if(getPath())
      {
         BString msg;
         msg.format(B("File failed to close: %s"), BStrConv::toB(getPath()));
         BFAIL(msg.getPtr());
      }
   }

   delete mpStorage;
   mpStorage = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::close()
{
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->close());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::openReadOnly(const BString& path, unsigned long flags)
{
   //-- Based on flags, make the correct storage class.
   if(mpStorage == NULL)
   {
      mpStorage = new BFileStorageWin32;
   }

   return(mpStorage->openReadOnly(path, flags));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::openWriteable(const BString& path, unsigned long flags, BYTE compressionLevel)
{
   compressionLevel;

   //-- Based on flags, make the correct storage class.
   if(mpStorage == NULL)
   {
      mpStorage = new BFileStorageWin32;
   }

   return(mpStorage->openWriteable(path, flags));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::openReadWrite(const BString& path, unsigned long flags)
{
   //-- Based on flags, make the correct storage class.
   if(mpStorage == NULL)
   {
      mpStorage = new BFileStorageWin32;
   }

   return(mpStorage->openReadWrite(path, flags));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::setOffset(unsigned __int64 offset, unsigned long fromPosition)
{
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->setOffset(offset, fromPosition));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::getOffset(unsigned long& offset, bool realPositionIfCompressed) const
{
   realPositionIfCompressed;
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->getOffset(offset));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::getOffset(unsigned __int64& offset, bool realPositionIfCompressed) const
{
   realPositionIfCompressed;
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->getOffset(offset));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::getSize(unsigned long& size, bool realSizeIfCompressed) const
{
   realSizeIfCompressed;
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->getSize(size));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::getSize(unsigned __int64& size, bool realSizeIfCompressed) const
{
   realSizeIfCompressed;
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->getSize(size));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::getTime(BFileTime& time) const
{
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->getTime(time));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileTranslation::readEx(void* pBuffer, unsigned long numBytes)
{
   if(mpStorage==NULL)
      return(0);
   return(mpStorage->readEx(pBuffer, numBytes));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::read(void* pBuffer, unsigned long numBytes)
{
   if(mpStorage==NULL)
      return(0);
   return(mpStorage->read(pBuffer, numBytes));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::write(const void* pBuffer, unsigned long numBytes)
{
   if(mpStorage==NULL)
      return(0);
   return(mpStorage->write(pBuffer, numBytes));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BFileTranslation::flush()
{
   if(mpStorage==NULL)
      return;
   return(mpStorage->flush());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BYTE* BFileTranslation::getAndIncrementPtr(DWORD dwbytes)
{
   if(mpStorage==NULL)
      return(NULL);
   return(mpStorage->getAndIncrementPtr(dwbytes));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslation::isOpen() const
{
   if(mpStorage==NULL)
      return(false);
   return(mpStorage->isOpen());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileTranslation::getFlags() const
{
   if(mpStorage==NULL)
      return(0);
   return(mpStorage->getFlags());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const BString* BFileTranslation::getPath() const
{
   if(mpStorage==NULL)
      return(NULL);
   return(&mpStorage->getPath());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileTranslation::getError() const
{
   if(mpStorage==NULL)
      return(BFILE_NOT_OPEN);
   return(mpStorage->getError());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HANDLE BFileTranslation::getFileHandle() const
{
   if(mpStorage==NULL)
      return(NULL);
   return(mpStorage->getFileHandle());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BFileTranslation::setOwnerThread(DWORD threadID)
{
   if(mpStorage==NULL)
      return;
   mpStorage->setOwnerThread(threadID);
}

//-----------------------------------------------------------------------------
//
//
// BFileTranslationCompressed
// 
// 
//-----------------------------------------------------------------------------
BFileTranslationCompressed::BFileTranslationCompressed()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileTranslationCompressed::~BFileTranslationCompressed()
{
   close();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::close()
{
   return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::openReadOnly(const BString& path, unsigned long flags)
{
   return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::openWriteable(const BString& path, unsigned long flags, BYTE compressionLevel)
{
   return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::openReadWrite(const BString& path, unsigned long flags)
{
   path; flags;
   BFAIL("Can't open a Compressed ReadWrite file!");
   return(false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::setOffset(unsigned __int64 offset, unsigned long fromPosition)
{
   // Success.
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::getOffset(unsigned long& offset, bool realPositionIfCompressed) const
{
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::getOffset(unsigned __int64& offset, bool realPositionIfCompressed) const
{
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::getSize(unsigned long& size, bool realSizeIfCompressed) const
{
   return false;   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::getSize(unsigned __int64& size, bool realSizeIfCompressed) const
{
   return false;   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileTranslationCompressed::readEx(void* pBuffer, unsigned long numBytes)
{
   return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::read(void* pBuffer, unsigned long numBytes)
{
   return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileTranslationCompressed::write(const void* pBuffer, unsigned long numBytes)
{
   return false;
}


