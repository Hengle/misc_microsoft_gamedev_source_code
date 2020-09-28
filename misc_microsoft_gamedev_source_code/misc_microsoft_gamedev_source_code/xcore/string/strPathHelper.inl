// File: strPathHelper.inl

template<class StringAllocator>
inline bool strPathHasDrive(const BStringTemplate<BCHAR_T, StringAllocator>& string)
{
   if (string.isEmpty())
      return(false);

   // need at least 2 chars (i.e. C:)
   if (string.length() < 2)
      return(false);

   const BCHAR_T* pBuffer = string.getPtr();

   // Make sure it is a valid drive letter (afaik anyway)
   BCHAR_T first = strToUpperCh(pBuffer[0]);
   if (first < B('A') && first > B('Z'))
      return(false);

   if (pBuffer[1] != B(':'))
      return(false);
   return(true);
}


template<class StringAllocator> inline 
bool strPathCreateFullPath(const BStringTemplate<BCHAR_T, StringAllocator>& fullPathName)
{
   BStringTemplate<BCHAR_T, StringAllocator> curPath;

   const int l = fullPathName.length();

   bool gotUNCServer = false;

   int n = 0;
   while (n < l)
   {
      const BCHAR_T c = fullPathName.getPtr()[n];

      const bool isSep = (c == B('/')) || (c == B('\\'));

      if ((isSep) || (n == (l - 1)))
      {
         if ((n == (l - 1)) && (!isSep))
         {
            BCHAR_T buf[2] = { c, 0 };
            curPath += BStringTemplate<BCHAR_T, StringAllocator>(buf);
         }

         bool valid = false;
         if ((curPath.length() > 3) && (curPath.getPtr()[1] == B(':')))
         {
            valid = true;
         }
         else if (curPath.length() > 2)
         {  
            if (gotUNCServer)
               valid = true;

            gotUNCServer = true;
         }

         if (valid)
            _mkdir(curPath.getPtr());
      }

      BCHAR_T buf[2] = { c, 0 };
      curPath += BStringTemplate<BCHAR_T, StringAllocator>(buf);

      n++;
   } 

   return true;
}

template<class StringAllocator> inline 
bool strPathHasDirectory(const BStringTemplate<BCHAR_T, StringAllocator>& string)
{
   if (string.isEmpty())
      return(false);

   return(string.findLeft(B('\\'))!=-1);
}

template<class StringAllocator> inline 
bool strPathHasFilename(const BStringTemplate<BCHAR_T, StringAllocator>& string)
{
   if (string.isEmpty())
      return(false);

   long pos1 = string.findRight(B('\\'));
   long pos2 = string.findLeft(B('.'), pos1);

   // if I can't find either a directory, or an extension I have no idea what this is
   if (pos1 < 0 && pos2 < 0)
      return(false);

   if (pos1 >= string.length()-1)
      return(false);

   return(true);
}

template<class StringAllocator> inline 
bool strPathHasExtension(const BStringTemplate<BCHAR_T, StringAllocator>& string)
{
   if (string.isEmpty())
      return(false);

   // rg [6/30/07] - This is flawed, if the path has an extension, but the filename doesn't, this fails!      
   
   return(string.findRight(B('.')) >= 0);
}

template<class StringAllocator> inline 
bool strPathHasExtension(const BStringTemplate<BCHAR_T, StringAllocator>& string, const BCHAR_T* pExtension)
{
   if (string.isEmpty())
      return(false);

   // Find the dot.
   long pos = string.findRight(B('.'));

   // If no dot...
   if(pos<0)
   {
      // If no extension, but you asked if it had no extension ... then true.
      if(!pExtension || pExtension[0] == 0)
         return(true);

      // Otherwise we don't match extensions.
      return(false);
   }

   // No extension = no match.
   if(!pExtension)
      return(false);

   // Exclude dot from comparison.
   if(pExtension[0] == '.')
      pExtension++;

   // Actually compare.
   if(bcsicmp(string.getPtr() + pos + 1, pExtension) == 0)
      return(true);
   return(false);

   //return(string.findRight(pExtension) >= 0);
}

template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathGetDrive(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString)
{
   if (srcString.isEmpty())
      return(false);

   // need at least 2 chars (i.e. C:)
   if (srcString.length() < 2)
      return(false);

   const BCHAR_T* pBuffer = srcString.getPtr();

   // Make sure it is a valid drive letter (afaik anyway)
   BCHAR_T first = strToUpperCh(pBuffer[0]);
   if (first < B('A') && first > B('Z'))
      return(false);

   if (pBuffer[1] != B(':'))
      return(false);

   dstString.copy(srcString, 2);
   return(true);
}

template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathGetDirectory(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString, bool keepDrive)
{
   if (srcString.isEmpty())
      return(false);

   long start = 0;
   if (!keepDrive)
   {
      if (strPathHasDrive(srcString))
         start+= 2;
   }

   long end = srcString.findRight(B('\\'));
   if (end < start)
      return(false);

   dstString.copy(srcString, end-start+1, start);
   return(true);
}

template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathGetFilename(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString)
{
   if (srcString.isEmpty())
      return(false);

   long pos1 = srcString.findRight(B('\\'));
   long pos2 = srcString.findLeft(B('.'), pos1);
   if (pos1 < 0 && pos2 < 0)
      return(false);

   if (pos1 >= srcString.length()-1)
      return(false);

   // assume the whole thing is the filename
   if (pos1 < 0)
      dstString.copy(srcString);
   else
      dstString.copy(srcString, -1, pos1+1);
   return(true);
}

template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathGetExtension(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString)
{
   if (srcString.isEmpty())
      return(false);
      
   // rg [6/30/07] - This is flawed, if the path has an extension, but the filename doesn't, this fails!      

   long pos = srcString.findRight(B('.'));
   if (pos < 0 || pos >= srcString.length())
      return(false);

   dstString.copy(srcString, -1, pos);
   return(true);
}

template<class StringAllocator> inline 
void strPathAddExtension(BStringTemplate<BCHAR_T, StringAllocator>& srcString, const BCHAR_T* pExtension)
{
   if (!pExtension)
   {
      BFAIL("ERROR: strPathAddExtension -- Invalid parameter.");
      return;
   }

   if (pExtension[0] != B('.'))
      srcString.append(B("."));

   srcString.append(pExtension);
}

template<class StringAllocator> inline 
void strPathRemoveExtension(BStringTemplate<BCHAR_T, StringAllocator>& srcString)
{
   if (srcString.isEmpty())
      return;

   long pos = srcString.findRight(B('.'));
   if (pos < 0 || pos >= srcString.length())
      return;
   if(pos == 0)
   {
      srcString.empty();
      return;
   }

   srcString.crop(0, pos-1);
}

template<class StringAllocatorSrc, class StringAllocatorDst> inline 
void strPathSplit(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& path, BStringTemplate<BCHAR_T, StringAllocatorDst>& filename)
{
   int ofs = srcString.length() - 1;
   while (ofs >= 0)
   {
      const BCHAR_T c = srcString.getPtr()[ofs];
      if ((c == B(':')) || (c == B('\\')) || (c == B('/')))
         break;
      ofs--;
   }

   if (ofs < 0)
   {
      path.empty();
      filename = srcString;
   }
   else
   {
      path.set(srcString.getPtr(), ofs + 1);
      filename.set(&srcString.getPtr()[ofs + 1]);
   }
}

#if 0
// rg [12/10/05] - BFile doesn't exist in xcore yet
template<class StringAllocator> inline 
bool strPathLoadStringFromFile(BStringTemplate<BCHAR_T, StringAllocator>& dstString, long dirID, BStringTemplate<BCHAR_T, StringAllocator>& filename)
{
   dstString.empty();

   BYTE* pBuffer = NULL;

   //-- Open the file.
   BFile file;
   if (!file.openReadOnly(dirID, filename))
      return false;

   //-- Discover the size of the file.
   unsigned __int64 trueSize;
   if (!file.setOffset(0, BFILE_OFFSET_END))
      goto Error;
   if (file.getOffset(trueSize) != 0)
      goto Error;
   if (!file.setOffset(0, FILE_BEGIN))
      goto Error;
   if (trueSize < 1)
      goto Error;
#ifdef _BANG
   if (trueSize > (cMaximumDWORD - sizeof(BCHAR_T)))
#else
   if (trueSize > (0xFFFFFFFF - sizeof(BCHAR_T)))
#endif
      goto Error;

   //-- Allocate storage.
   DWORD size;
   size    = (DWORD)trueSize;
   pBuffer = new BYTE [size + sizeof(BCHAR_T)];

   //-- Read file.
   if (!file.read(pBuffer, size))
      goto Error;

   //-- Force a terminator on the end.
   FillMemory(pBuffer + size, sizeof(BCHAR_T), 0);

   //-- See if we have a Unicode text file.
   if (*(WCHAR*)pBuffer == UNICODE_TOKEN)
   {
      WCHAR* pString = ((WCHAR*)pBuffer) + 1;
      dstString.set(pString);
   }
   else
   {
      char* pString = (char*)pBuffer;
      dstString.set(pString);
   }

   //-- All done.
   delete[] pBuffer;
   return true;

Error:
   if (pBuffer)
      delete[] pBuffer;
   return false;   
}

template<class StringAllocator> inline 
bool strPathSaveStringToFile(const BStringTemplate<BCHAR_T, StringAllocator>& dstString, long dirID, BStringTemplate<BCHAR_T, StringAllocator>& filename)
{
   const BCHAR_T* pBuffer = dstString.getPtr();

   //-- Open the file.
   BFile file;
   if (!file.openWriteable(dirID, filename, BFILE_OPEN_ENABLE_BUFFERING))
      return false;

#ifdef UNICODE   
   //-- Write the Unicode Token.
   if (!file.write(&UNICODE_TOKEN, sizeof(UNICODE_TOKEN)))
   {
      return false;
   }
#endif   

   //-- Write the contents of the string.
   if (pBuffer)
   {
      if (!file.write(pBuffer, sizeof(BCHAR_T) * dstString.length()))
      {
         return false;
      }
   }

   //-- Close the file.
   if (!file.close())
      return false;

   //-- Success.
   return true;
}
#endif

template<class StringAllocator> inline 
bool strPathAddBackSlash(BStringTemplate<BCHAR_T, StringAllocator>& path, bool convertEmptyStringToPath)
{
   // rg [12/05/05] - I've changed the logic of this, hope nothing gets busted.

   if (path.isEmpty())
   {
      if (convertEmptyStringToPath)
         path.set(B("."));
      else
         return false;
   }         

#if 0   
   if(path.findRight(B("\\")) != (path.length()-1))
   {
      path.append(B("\\"));
      return(true);
   }
#endif

   const BCHAR_T lastChar = path.getPtr()[path.length() - 1];
   if ((lastChar != B(':')) && (lastChar != B('/')) && (lastChar != B('\\')))
   {
      path.append(B("\\"));
      return true;
   }

   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class StringAllocator> inline 
bool strPathAddForwardSlash(BStringTemplate<BCHAR_T, StringAllocator>& path)
{
   if(path.findRight(B("/")) != (path.length()-1))
   {
      path.append(B("/"));
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathMakeAbsolute(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& relativePath, BStringTemplate<BCHAR_T, StringAllocatorDst>& absolutePath)
{
   absolutePath = relativePath;
   
   // If no path given, return error
   if (relativePath.isEmpty())
      return false;

#if 0
   // If already absolute path, don't do anything
   if (strPathIsAbsolute(relativePath))
      return true;
#endif      

#ifdef XBOX
   absolutePath.set(relativePath);
   // _fullpath() and _wfullpath() don't appear to be available on Xbox in Release/Playtest builds ??
   return false;
#else

#if 0
   CHAR temp[MAX_PATH];
   _fullpath(temp, relativePath.getPtr(), MAX_PATH);
   absolutePath.set(temp);
#endif

   BCHAR_T temp[MAX_PATH];
#ifdef UNICODE   
   _wfullpath(temp, relativePath, MAX_PATH);
#else
   _fullpath(temp, relativePath, MAX_PATH);
#endif   
   absolutePath.set(temp);

   return true;
#endif //XBOX
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline 
bool strPathMakeRelative(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& absolutePath, const BStringTemplate<BCHAR_T, StringAllocatorSrc>& dirRelativeTo, BStringTemplate<BCHAR_T, StringAllocatorDst>& relativePath)
{
#ifdef XBOX
   // FIXME
   return false;
#else
   BCHAR_T temp[MAX_PATH];
   BOOL bSuccess;

   bSuccess = PathRelativePathTo(temp, dirRelativeTo, FILE_ATTRIBUTE_DIRECTORY, absolutePath, FILE_ATTRIBUTE_NORMAL);
   if (!bSuccess)
      return false;

   // PathRelativePathTo will add a backslash at the beginning of the path.  Move beyond that before copying.
   int nIndx = 0;
   while ((temp[nIndx] == B('\0')) || (temp[nIndx] == B('\\')) || (temp[nIndx] == B('/')))
      nIndx++;
   relativePath.set(temp + nIndx);

   return true;
#endif   
}



