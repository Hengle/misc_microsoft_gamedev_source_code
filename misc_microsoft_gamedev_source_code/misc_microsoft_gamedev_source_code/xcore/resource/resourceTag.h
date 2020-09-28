//============================================================================
//
//  File: resourceTag.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//  A resource tag is always little endian. It's intended to be used by 
//  build systems/command line tools.
//============================================================================
#pragma once
#include "hash\bsha1.h"
#include "hash\adler32.h"
#include "stream\stream.h"

enum { cResourceTagECFChunkID = 0x714BFE00 };

#pragma pack(push, 1)
struct BResourceTagHeader
{
   enum { cSignature = 0x714C };
   ushort      mSignature;

   enum { cMajorVersion = 0x11, cMinorVersion = 0x00 };
   uchar       mMajorVersion;
   uchar       mMinorVersion;
   
   ushort      mHeaderSize;
   ushort      mDataSize;
   
   enum { cAdler32DWORDsToSkip = 3 };
   uint        mHeaderAdler32;         // Adler32 of all header data following this member
   
   uint64      mTagTimeStamp;          // FILETIME as a uint64 when this tag was created
   DWORD       mTagGUID[4];            // Tag's 128-bit GUID (NOT necessarily a valid Windows GUID)
   char*       mpTagMachineName;       // Machine that created this tag
   char*       mpTagUserName;          // User that created this tag
               
   char*       mpSourceFilename;       // Full filename of source file
   BSHA1       mSourceDigest;          // SHA1 of source file contents
   uint64      mSourceFileSize;        // Source file size (valid if mSourceFileSize is not 0)
   uint64      mSourceFileTimeStamp;   // Last write time FILETIME as a uint64
               
   char*       mpCreatorToolCommandLine; // Command line (or just the tool name, if it's an editor) to recreate the tagged file, with "%s" where the source filename should go
   uchar       mCreatorToolVersion;    // Version of tool used to create tagged file
   
   enum ePlatformID { cPIDAny = 0, cPIDPC, cPIDXbox };
   uchar       mPlatformID;            // Intended platform
   
   ushort      mReserved[5];
         
   bool check(void) const
   {
      if (mSignature != cSignature)
         return false;
         
      if (mMajorVersion != cMajorVersion)
         return false;
      
      if (mHeaderSize < sizeof(BResourceTagHeader))
         return false;
      
      if (calcAdler32(reinterpret_cast<const DWORD*>(this) + cAdler32DWORDsToSkip, mHeaderSize - cAdler32DWORDsToSkip * sizeof(DWORD)) != mHeaderAdler32)
         return false;

      if ( !checkPointer(mpTagMachineName) || !checkPointer(mpTagUserName) || !checkPointer(mpSourceFilename) || !checkPointer(mpCreatorToolCommandLine) )
         return false;
       
      return true;
   }
         
   void pointerize(void* pBase)
   {
      if (mpTagMachineName) Utils::Pointerize(mpTagMachineName, pBase);
      if (mpTagUserName) Utils::Pointerize(mpTagUserName, pBase);
      if (mpSourceFilename) Utils::Pointerize(mpSourceFilename, pBase);
      if (mpCreatorToolCommandLine) Utils::Pointerize(mpCreatorToolCommandLine, pBase);
   }
   
private:
   bool checkPointer(void* p) const
   {
      if ((!p) || ((DWORD)p == 0xFFFFFFFF))
         return true;
         
      const uint totalSize = mHeaderSize + mDataSize;
      DWORD val = (DWORD)p;
      return ((val >= mHeaderSize) && (val < totalSize));
   }   
};
#pragma pack(pop)

class BResourceTagBuilder
{
   BString              mTagFilename;          
   BString              mSourceFilename;
   BString              mCreatorToolCommandLine; 
   
   BByteArray           mTagData;
   
   BResourceTagHeader   mHeader;
   
   bool                 mIsFinalized;
   
public:
   BResourceTagBuilder();
   
   void clear(void);
   
   void setSourceFilename(const BString& sourceFilename);      
   bool setSourceDigestAndTimeStamp(BStream& stream, uint64 timeStamp);
   bool setSourceDigestAndTimeStamp(const char* pFilename);
         
   void setCreatorToolInfo(const BString& creatorToolCommandLine, uchar version);
   
   void setPlatformID(BResourceTagHeader::ePlatformID platformID);
   
   bool finalize(void);
   
   bool getFinalized(void) const {return mIsFinalized; }
   const BByteArray& getFinalizedData(void) const { return mTagData; }
   
private:
   BResourceTagBuilder(const BResourceTagBuilder&);
   BResourceTagBuilder& operator= (const BResourceTagBuilder&);
   
   char* createString(const char* pStr);
   
   bool setDigestAndTimeStamp(BStream& stream, uint64 timeStamp, bool sourceFile);
   bool setDigestAndTimeStamp(const char* pFilename, bool sourceFile);
};

class BResourceTagUtils
{
public:
   // pSrcFilename is the source file, which can be in any format.
   // pDstFilename is the name of an ECF file which may contain an embedded resource tag.
   // Returns true if the source file matches the file used to generate the destination file.
   // Returns false on failure, of if the source file has changed.
   static bool fileIsUnchanged(const char* pSrcFilename, const char* pDstFilename, const uint* pCreatorToolVersion = NULL);
   
   static bool printInfo(BTextDispatcher& textDispatcher, const BResourceTagHeader& header);
   
   static void printFileTime(BTextDispatcher& textDispatcher, const uint64& fileTime);
};

