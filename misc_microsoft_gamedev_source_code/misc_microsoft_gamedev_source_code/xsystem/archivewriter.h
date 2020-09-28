#pragma once

#if 0
#ifndef __ARCHIVE_WRITER_H__
#define __ARCHIVE_WRITER_H__

#include "watermark.h"
class BXMLNode;
class BArchive;

//==============================================================================
// BArchiveEntry
//==============================================================================
class BArchiveFileEntry
{
   public:
      BArchiveFileEntry() :
         mdwOffset(0),
         mdwLength(0),
         mdwArchiveLength(0)
      {
      }

      ~BArchiveFileEntry(){}


      DWORD       mdwOffset;
      DWORD       mdwLength;
      DWORD       mdwArchiveLength;
      BFileTime   mFileTime;
      BString     mFileName;
};

//==============================================================================
// BArchiveHeader
//==============================================================================
class BArchiveHeader
{
   public:
      BArchiveHeader(void) : mdwVersion(cVersion),
                             mdwEntries(0),
                             mdwManifestOffset(0),
                             mdwAdler32(0),
                             mdwFlags(0)
      {
         mcTagID[0] = 'E';
         mcTagID[1] = 'S';
         mcTagID[2] = 'P';
         mcTagID[3] = 'N';
			memcpy(mWatermark, watermark, sizeof(mWatermark));
      }

      enum
      {
         cVersion = 0x02
      };

		
		char     mcTagID[4];
		DWORD    mdwVersion;
      char		mWatermark[WATERMARK_SIZE];
		DWORD    mdwAdler32;
      DWORD    mdwEntries;
      DWORD    mdwManifestOffset;
      DWORD    mdwFlags;
}; // BArchiveHeader

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class BArchiveWriterInput
{
   public:
      BArchiveWriterInput(){mpCompareArchive=NULL;mArchivesAreDifferent=false;mSupressOutput=false;mbDeleteSourceFile=false;}
      ~BArchiveWriterInput(){mpCompareArchive=NULL;mArchivesAreDifferent=false;mSupressOutput=false;mbDeleteSourceFile=false;}

      BString               mRoot;
      BString               mArchiveName;
      BDynamicArray<BString> mExcludeDirs;
      BDynamicArray<BString> mExcludeExts;
      BDynamicArray<BString> mExcludeFiles;

      BDynamicArray<BString> mDirectories;
		BDynamicArray<BString> mXmlToBinaryExtensions;
		BDynamicArray<BString> mExcludeFromXMBConversion;
      BDynamicArray<BArchiveFileEntry> mFileEntries;

      BFile                   mArchiveFile;

      BArchive*               mpCompareArchive;
      bool                   mArchivesAreDifferent;
      bool                   mSupressOutput;
		bool							mbDeleteSourceFile;
   private:
      //-- Disable copy constructor and assignment operator.
      BArchiveWriterInput(const BArchiveWriterInput& input);
      const BArchiveWriterInput& operator = (const BArchiveWriterInput& input);
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class BArchiveWriter
{
   public:
      BArchiveWriter(){}
      ~BArchiveWriter(){}

      bool  parseInput  (BArchiveWriterInput& input, const BXMLNode* pNode);
      bool  build       (BArchiveWriterInput& input);
      bool  diff        (BArchiveWriterInput& input);
      
   protected:

      bool  buildDirList      (BArchiveWriterInput& input);
      bool  buildFileList     (BArchiveWriterInput& input);
      bool  diffFileList      (BArchiveWriterInput& input);
      bool  writeFileEntries  (BArchiveWriterInput& input);
      bool  calcAdler32       (BArchiveWriterInput& input, DWORD& dwAdler);
      bool  parseNode         (BArchiveWriterInput& input, const BXMLNode* pNode);
};

#endif //__ARCHIVE_WRITER_H__

#endif