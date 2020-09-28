#pragma once


#if 0
#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "string\ustringtable.h"

// jce [7/20/2005] -- case sensitive hash table for speed.  we're going to work exclusively with pre-lowercased strings. 
typedef BUStringTable<BArchiveFileEntry*, true, 16384> BFileTable;

class BArchive
{
   public:
      BArchive();
      ~BArchive(){close();}

      void     close          ();
      bool     openRawArchive (long dirID, const BString& path, bool crcCheck = true);
      
      // jce [7/21/2005] -- this now expects path to be "standardized" first (strPathStandardizePath)!! 
      bool     findFile       (const BString& path, bool searchOnWildCards, BDynamicArray<BArchiveFileEntry>* pEntryList);


      const BString&     getRootDir() const {return(mRootDirectory);}
		const BFileTable&  getFileTable() const { return(mFileTable);}

   protected:

      friend BFileStorageArchive;
      
      bool     checkAdler32   ();
      bool     makeFileTable  (long dirID);
      BFile*   getFile        (){return(&mFile);}
		bool		readHeader		();

      BFileTable                          mFileTable;
      BFile                               mFile;
      BArchiveHeader                      mHeader;
      BArchiveFileEntry*                  mpEntries;
      BString                             mRootDirectory;
      DWORD                               mCachedOffset;

   private:
      //-- Disable copy constructor and assignment operator.
      BArchive(const BArchive& arc);
      const BArchive& operator = (const BArchive& arc);
};

#endif // __ARCHIVE_H__
#endif