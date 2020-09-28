#include "xsystem.h"
#include "archivewriter.h"
#include "archive.h"
//#include "strPathHelper.h"
#include "regexp.h"

#pragma warning (disable: 4127)

uint gArchiveDummy;

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BArchive::BArchive() :
 mpEntries(NULL),
 mCachedOffset(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BArchive::close()
{
   delete [] mpEntries;
   mpEntries = NULL;
   
   mCachedOffset = 0;

   mFile.close();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchive::openRawArchive(long dirID, const BString& path, bool crcCheck /*= true*/)
{
   mFile.close();
   mCachedOffset = 0;

   //-- Open file for reading.
   if(mFile.openReadOnly(dirID, path)==false)
   {
      return(false);
   }

   if(readHeader() == false)
	{
		return(false);
	}

   //-- verify file integrity.
	if(crcCheck == true)
	{
		if(checkAdler32() == false)
		{
			return(false);
		}
	}

   if(makeFileTable(dirID) == false)
   {
      return(false);
   }
   
   // Sync offset.
   mFile.getOffset(mCachedOffset);
   
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchive::readHeader()
{
	//-- read tag.
	if(mFile.read(mHeader.mcTagID, sizeof(mHeader.mcTagID)) == false)
	{
		return(false);
	}

	//-- check for tag.

	//--read version 
	if(mFile.read(&mHeader.mdwVersion, sizeof(mHeader.mdwVersion)) == false)
	{
		return(false);
	}
	
	if(mHeader.mdwVersion == 0x02)
	{
		if(mFile.read(mHeader.mWatermark, sizeof(mHeader.mWatermark)) == false)
		{
			return(false);
		}
	}
	else if(mHeader.mdwVersion != 0x01)
	{
		return(false);
	}

	if(mFile.read(&mHeader.mdwAdler32, sizeof(mHeader.mdwAdler32)) == false)
	{
		return(false);
	}
	if(mFile.read(&mHeader.mdwEntries, sizeof(mHeader.mdwEntries)) == false)
	{
		return(false);
	}
	if(mFile.read(&mHeader.mdwManifestOffset, sizeof(mHeader.mdwManifestOffset)) == false)
	{
		return(false);
	}
	if(mFile.read(&mHeader.mdwFlags, sizeof(mHeader.mdwFlags)) == false)
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchive::findFile(const BString& path, bool searchOnWildCards, BDynamicArray<BArchiveFileEntry>* pEntryList)
{
   if(mFile.isOpen() == false)
      return(false);
   
   if(pEntryList)
      pEntryList->clear();
   
   // If the root of this path is not the root of the archive file, then we're not going to find it in here.
   if(path.compare(mRootDirectory, true, mRootDirectory.length()) != 0)
      return(false);
   
	BArchiveFileEntry* plocalEntry = NULL;
   bool found = mFileTable.find(path, &plocalEntry);
   if(found == true)
   {
      if(pEntryList)
         pEntryList->add(*plocalEntry);
      return(true);
   }

   if(searchOnWildCards == false)
      return(false);

   BString filename, dir(path), testPath, localFilename;
   if(strPathGetFilename(path, filename) == false)
      return(false);
	
	long pos = dir.findRight(B("\\"));
	dir.crop(0,pos);
   
   //-- now try it via regex.
   const BDynamicSimArray<BArchiveFileEntry*>& values = mFileTable.getValues();
   const BDynamicSimArray<BString>& tags = mFileTable.getTags();
   long numValues = values.getNumber();
   found = false;
   for(long i=0; i<numValues; i++)
   {
      BArchiveFileEntry* plocalEntry2 = values[i];
      if(wildcmp(filename, plocalEntry2->mFileName))
      {
         testPath = dir;
			strPathGetFilename(plocalEntry2->mFileName, localFilename);
         testPath += localFilename;
         
         // jce [7/19/2005] -- case sensitive for perf since we've toLower'ed everything 
         if(testPath.compare(tags[i], true) == 0)
         {
            found = true;
            if(pEntryList)
				{
               long index = pEntryList->add(*plocalEntry2);
					pEntryList->get(index).mFileName = localFilename;
				}
         }
      }
   }
   return(found);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchive::checkAdler32()
{
   return false;
#if 0
   //-- Move to the end of the header.
	long offset = sizeof(mHeader);
	if(mHeader.mdwVersion == 0x01)
	{
		offset -= sizeof(mHeader.mWatermark);
	}

   if(mFile.setOffset(offset) == false)
   {
      return(false);
   }

   DWORD newAdler32 = 0;
   while (true)
   {
      const long cBufferSize=4096;
      BYTE buffer[cBufferSize];
      DWORD dwreads = mFile.readEx(buffer, cBufferSize);
      if (!dwreads)
         break;

      newAdler32 = adler32(newAdler32, (BYTE *)buffer, dwreads);
   }

   return(newAdler32 == mHeader.mdwAdler32);
#endif   
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchive::makeFileTable(long dirID)
{
   //-- Move to the table offset.
   if(mFile.setOffset(mHeader.mdwManifestOffset) == false)
   {
      return(false);
   }

   //-- read in the root.
   mRootDirectory.empty();
   if(mFile.readBString(mRootDirectory) == false)
   {
      return(false);
   }
   mRootDirectory;
   gFileManager.getDirectory(mRootDirectory, dirID);
	strPathAddBackSlash(mRootDirectory);
	mRootDirectory.standardizePath();

   long numEntries = 0;
   if(mFile.read(&numEntries, sizeof(numEntries)) == false)
   {
      return(false);
   }
   if(numEntries == 0)
   {
      return(false);
   }

   if(mpEntries != NULL)
   {
      delete [] mpEntries;
      mpEntries = NULL;
   }

   //-- Raad all entries and make string table.
   BString fileName;
   mpEntries = new BArchiveFileEntry[numEntries];
   BArchiveFileEntry* pEntry = mpEntries, *dummy;
   for(long i=0; i<numEntries; i++)
   {
      if(mFile.read(&(pEntry->mdwOffset), sizeof(pEntry->mdwOffset)) == false)
      {
         return(false);
      }
      if(mFile.read(&(pEntry->mdwLength), sizeof(pEntry->mdwLength)) == false)
      {
         return(false);
      }
      if(mFile.read(&(pEntry->mdwArchiveLength), sizeof(pEntry->mdwArchiveLength)) == false)
      {
         return(false);
      }
      if(mFile.read(&(pEntry->mFileTime), sizeof(pEntry->mFileTime)) == false)
      {
         return(false);
      }
      if(mFile.readBString(pEntry->mFileName) == false)
      {
         return(false);
      }

      // jce [7/19/2005] -- put into table lowercase for faster compares later. 
      pEntry->mFileName.standardizePath();

      fileName.set(mRootDirectory);
      fileName += pEntry->mFileName;
      
      mFileTable.add(fileName, pEntry, dummy);
      pEntry++;
   }
   return(true);
}
#endif