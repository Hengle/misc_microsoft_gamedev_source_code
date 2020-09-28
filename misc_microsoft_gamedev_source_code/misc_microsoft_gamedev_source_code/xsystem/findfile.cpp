//============================================================================
//
//  FindFile.cpp
//
//  Copyright 2002 Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "file\lowLevelFileIO.h"
#include "fileUtils.h"

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================

//============================================================================
//  STATIC DATA
//============================================================================
static BCHAR_T             sgBadDir1[] = B(".");
static BCHAR_T             sgBadDir2[] = B("..");


//============================================================================
// BFindFile::BFindFile
//============================================================================
BFindFile::BFindFile(const BString& path, const BString& wildCard, long flags /*=BFFILE_WANT_FILES*/) :
   mhFindHandle(INVALID_HANDLE_VALUE),
   mFlags(-1)
{
   //-- Save off the passed in info.
   if(path.isEmpty() == true)
      mStartPath = BFileUtils::getXboxGamePath();
   else
      mStartPath = path;
   
   mWildCard = wildCard;
   mFlags = flags;
   init();
}

//============================================================================
// BFindFile::BFindFile
//============================================================================
BFindFile::BFindFile(const BString& pathAndWildCard, long flags /*= BFFILE_WANT_FILES*/) :
   mhFindHandle(INVALID_HANDLE_VALUE),
   mFlags(-1)
{
   //-- if there is directory sent in, then use the current directory.
   long index = pathAndWildCard.findRight(B("\\"));
   if(index < 0)
   {
      mStartPath = BFileUtils::getXboxGamePath();
      mWildCard = pathAndWildCard;
   }
   else
   {
      index++;
      mStartPath.set(pathAndWildCard, index);
      mWildCard.set(pathAndWildCard, pathAndWildCard.length()-index, index);
   }

   mFlags = flags;
   init();
}

//============================================================================
// BFindFile::BFindFile
//============================================================================
BFindFile::BFindFile(long dirID, const BString& wildCard, long flags /*=BFFILE_WANT_FILES*/) :
   mhFindHandle(INVALID_HANDLE_VALUE),
   mFlags(-1)
{
   gFileManager.getDirectory(mStartPath, dirID);
   
   mWildCard = wildCard;
   mFlags = flags;
   init();
}
   
//============================================================================
// BFindFile::init
//============================================================================
void BFindFile::init()
{
   //-- The beginning search path needs a backslask at the end.
   if(mStartPath.findRight(B("\\")) != (mStartPath.length()-1))
      mStartPath.append(B("\\"));
   
   //-- Setup some path stuff.
   mFindFilePath.set(mStartPath);
   mFindFilePath += mWildCard;
   
   mFindFilePath.standardizePath();

   //-- Search the current directory for files that match.
   mhFindHandle = INVALID_HANDLE_VALUE;
   mStarted = false;
}

//============================================================================
// BFindFile::close
//============================================================================
void BFindFile::close()
{
   if(mhFindHandle != INVALID_HANDLE_VALUE)
   {
      ILowLevelFileIO::getDefault()->findClose(mhFindHandle);
      mhFindHandle = INVALID_HANDLE_VALUE;
   }
}

//============================================================================
// BFindFile::getNext
//============================================================================
bool BFindFile::getNext(BFileInfo& info)
{
   //-- If our flags were never set, then there was an error in the constructor.
   if(mFlags == -1)
      return(false);

   WIN32_FIND_DATA   findData;
   
   //-- if not already started, do find file first.
   if(mStarted == false)
   {
      mhFindHandle = ILowLevelFileIO::getDefault()->findFirstFile(mFindFilePath.asNative(), &findData); 
      
      if (mhFindHandle == INVALID_HANDLE_VALUE)
         return false;
         
      mStarted = true;
   }
   else //-- otherwise, its find file next.
   {
      if(ILowLevelFileIO::getDefault()->findNextFile(mhFindHandle, &findData) == 0)
      {
         ILowLevelFileIO::getDefault()->findClose(mhFindHandle);
         mhFindHandle = INVALID_HANDLE_VALUE;

         return(false);
      }
   }

   //-- if this is something we don't care about, keep going
   bool bDir         = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
   bool bWantDirs    = (mFlags & BFFILE_WANT_DIRS) ? true : false;
   bool bWantFiles   = (mFlags & BFFILE_WANT_FILES) ? true : false;
         
   if ((bDir && !bWantDirs) || (!bDir && !bWantFiles))
   {
      return(getNext(info));
   }

   //-- Ignore "." and ".."
   bool skip = false;
   const BString fileName(findData.cFileName);
   if (strCompare(fileName.getPtr(), MAX_PATH, sgBadDir1, 1, false) == 0) skip = true;
   if (strCompare(fileName.getPtr(), MAX_PATH, sgBadDir2, 2, false) == 0) skip = true;
   if (skip)
      return(getNext(info));

   //-- Fill in the info data.
   info.musFullPathname = mStartPath;
   info.musFullPathname += fileName;
   info.musFilename = fileName;
   
   if (bDir)
   {
      info.mnFlags = BFileInfo::cDir;
      info.mTime.makeError();
   }
   else
   {
      info.mnFlags = BFileInfo::cFile;
      gFileManager.getFileTime(info.musFullPathname, info.mTime);
   }
   return(true);
}

