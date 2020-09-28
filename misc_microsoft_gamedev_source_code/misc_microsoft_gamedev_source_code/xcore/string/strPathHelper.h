//============================================================================
//
//  StrPathHelper.h
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================
#pragma once 

//----------------------------------------------------------------------------
// strPathHasDrive
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathHasDrive(const BStringTemplate<BCHAR_T, StringAllocator>& string);

//----------------------------------------------------------------------------
// strPathHasDirectory
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathHasDirectory(const BStringTemplate<BCHAR_T, StringAllocator>& string);

//----------------------------------------------------------------------------
// strPathHasFilename
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathHasFilename (const BStringTemplate<BCHAR_T, StringAllocator>& string);

//----------------------------------------------------------------------------
// strPathHasExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathHasExtension(const BStringTemplate<BCHAR_T, StringAllocator>& string);

//----------------------------------------------------------------------------
// strPathHasExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathHasExtension(const BStringTemplate<BCHAR_T, StringAllocator>& string, const BCHAR_T* pExtension);

//----------------------------------------------------------------------------
// strPathGetDrive
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathGetDrive    (const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcStringSrc, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString);

//----------------------------------------------------------------------------
// strPathGetDirectory
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathGetDirectory(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString, bool keepDrive = false);

//----------------------------------------------------------------------------
// strPathGetFilename
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathGetFilename (const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString);

//----------------------------------------------------------------------------
// strPathGetExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathGetExtension(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& dstString);

//----------------------------------------------------------------------------
// strPathAddExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline void strPathAddExtension   (BStringTemplate<BCHAR_T, StringAllocator>& srcString, const BCHAR_T* pExtension);

//----------------------------------------------------------------------------
// strPathRemoveExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//    Parameters:
//       string   - The string to check
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline void strPathRemoveExtension(BStringTemplate<BCHAR_T, StringAllocator>& srcString);

template<class StringAllocatorSrc, class StringAllocatorDst> inline void strPathSplit(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& srcString, BStringTemplate<BCHAR_T, StringAllocatorDst>& path, BStringTemplate<BCHAR_T, StringAllocatorDst>& filename);

#if 0
// rg [12/10/05] - BFile doesn't exist in xcore yet
template<class StringAllocator> inline bool strPathLoadStringFromFile(BStringTemplate<BCHAR_T, StringAllocator>& dstString, long dirID, BStringTemplate<BCHAR_T, StringAllocator>& filename);
template<class StringAllocator> inline bool strPathSaveStringToFile  (const BStringTemplate<BCHAR_T, StringAllocator>& dstString, long dirID, BStringTemplate<BCHAR_T, StringAllocator>& filename);
#endif

//----------------------------------------------------------------------------
// strPathAddBackSlash
//  NOTE: will append a backslash, if one isn't there already.  Returns false
//        if it didn't end up adding one. True otherwise.
//    Parameters:
//       string   - The string to modify
//    Return:
//       false/true
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathAddBackSlash   (BStringTemplate<BCHAR_T, StringAllocator>& path, bool convertEmptyStringToPath = false);

template<class StringAllocator> inline bool strPathAddForwardSlash(BStringTemplate<BCHAR_T, StringAllocator>& path);
bool strPathIsAbsolute     (const BCHAR_T *pFilename);

// Makes a relative path absolute.  Assumes the path is relative to the current
// working directory.  relativePath and absolutePath can be the same BStringTemplate<BCHAR_T, StringAllocator>.
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathMakeAbsolute(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& relativePath, BStringTemplate<BCHAR_T, StringAllocatorDst>& absolutePath);

// Makes an absolute path relative to a directory.  relativePath and absolutePath can be the same BStringTemplate<BCHAR_T, StringAllocator>.
// FIXME: Does not work on Xbox!
template<class StringAllocatorSrc, class StringAllocatorDst> inline bool strPathMakeRelative(const BStringTemplate<BCHAR_T, StringAllocatorSrc>& absolutePath, const BStringTemplate<BCHAR_T, StringAllocatorSrc>& dirRelativeTo, BStringTemplate<BCHAR_T, StringAllocatorDst>& relativePath);

//----------------------------------------------------------------------------
// Attempts to create the directories in the pathname fullPathName.
//----------------------------------------------------------------------------
template<class StringAllocator> inline bool strPathCreateFullPath(const BStringTemplate<BCHAR_T, StringAllocator>& fullPathName);

// This "standardizes" a path by forcing it to lower case, forcing forward slashes to backslashes and removing duplicate backslashes.
// dst should be as large as src
// src can be the same as dst.  This will modifies the string in place (the length of the resulting string will be <= the original length)
// Return is length of modified string (not counting null terminator)
DWORD strPathStandardizePath(const BCHAR_T *src, BCHAR_T *dst, DWORD dstLenWithoutNull);

// does windows-style wildcard matching (i.e. *.*, myfile?.*, etc.)
bool wildcmp(const BCHAR_T *wild, const BCHAR_T *string);

#include "strPathHelper.inl"
