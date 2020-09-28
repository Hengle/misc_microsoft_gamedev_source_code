//============================================================================
//
//  FileManager.inl
//
//  Copyright 2002-2007 Ensemble Studios
//
//============================================================================

template<class StringType>
inline eFileManagerError BFileManager::getDirListEntry(StringType& dir, long dirID, uint listIndex) const
{
   BScopedMonitor lock(mMonitor);
   
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;

   BWorkDirList* pWorkList = mWorkDirs[dirID];
   if (listIndex >= pWorkList->getNumberDirs())
      return cFME_INVALID_DIRID;
   
   pWorkList->getDirNameByIndex(dir, listIndex);
   return cFME_SUCCESS;
}

template<class StringType>
inline eFileManagerError BFileManager::getDirListModifier(StringType& dir, long dirID) const
{
   BScopedMonitor lock(mMonitor);
   
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;

   BWorkDirList* pWorkList = mWorkDirs[dirID];      
   dir.set(pWorkList->getModifier().getPtr());
   
   return cFME_SUCCESS;
}

template<class StringType>
inline eFileManagerError BFileManager::constructQualifiedPath(long dirID, const char* pFilename, StringType& qualifiedPath, uint fileSearchFlags, uint fileSystemFlags) 
{ 
   BFixedStringMaxPath temp; 
   eFileManagerError error = constructQualifiedPathInternal(dirID, pFilename, temp, fileSearchFlags, fileSystemFlags);
   
   if (error != cFME_SUCCESS)
   {
      qualifiedPath.empty();
      return error; 
   }
   
   qualifiedPath.set(temp.getPtr()); 
   return cFME_SUCCESS; 
}

template<class StringType>
inline bool BFileManager::BWorkDirList::getDirNameByIndex(StringType& dir, uint dirIndex)
{
   if (dirIndex >= mDirs.getSize())
      return false;

   dir.set(mDirs[dirIndex].mName.getPtr());

   if (mModifier.length())
   {
      dir += mModifier;
      dir += "\\";
   }

   return true;
}

inline eFileManagerError BFileManager::doesFileExist(const char* pFullFilename, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pFullFilename);

   BScopedMonitor lock(mMonitor);    
   
   eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;
   
   BFixedStringMaxPath path;
   return doesFileExistInternal(pFullFilename, fileSystemFlags);
}

template<class StringType>
inline eFileManagerError BFileManager::doesFileExist(long dirID, const char* pRelFilename, StringType* pFullPath, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pRelFilename);

   BScopedMonitor lock(mMonitor);    
   
   eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;

   BFixedStringMaxPath path;
   error = doesFileExistInternal(dirID, pRelFilename, path, fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;

   if (pFullPath)
      pFullPath->set(path.getPtr());

   return cFME_SUCCESS;
}
