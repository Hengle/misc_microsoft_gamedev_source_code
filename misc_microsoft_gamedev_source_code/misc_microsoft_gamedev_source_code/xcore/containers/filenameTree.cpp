//==============================================================================
//
// File: filenameTree.cpp
//
// Copyright (c) 2007, Ensemble Studios
//
//==============================================================================
#include "xcore.h"
#include "filenameTree.h"

BFilenameTree::BFilenameTree() :
   mNodes(256, &gRenderHeap),
   mRootNodeIndex(cInvalidNodeIndex),
   mpRootNode(NULL),
   mNumFiles(0)
{
   initRoot();
}

BFilenameTree::~BFilenameTree()
{
}

void BFilenameTree::initRoot(void)
{
   BDEBUG_ASSERT(!mpRootNode);
   mpRootNode = mNodes.acquire((uint&)mRootNodeIndex, true);
   mpRootNode->mFileFlags = cDirFlag | cDirIsRoot;
}

void BFilenameTree::clear(void)
{
   mpRootNode = NULL;
   mNodes.clear();

   initRoot();   
         
   mNumFiles = 0;
}

BFilenameTree::eErrorCode BFilenameTree::add(const char* pName, uint fileFlags, uint64 data, bool replaceIfExisting, BNodeIndex* pNodeIndex)
{
   if (pNodeIndex)
      *pNodeIndex = cInvalidNodeIndex;
      
   BFixedStringMaxPath path;
   if (!canonicalizePathname(pName, path))
      return cErrorInvalidPath;
      
   const uint numComps = getNumFilenameComponents(path);
   if (!numComps)
      return cErrorInvalidPath;

   BNodeIndex curNode = mRootNodeIndex;

   for (uint compIndex = 0; compIndex < numComps; compIndex++)
   {
      BNode* pCurNode = &mNodes[curNode];
      const BOOL lastComp = (compIndex == (numComps - 1));
      
      BFixedStringMaxPath comp;
      getFilenameComponent(comp, path, compIndex);

      BNodeIndex childNode = findChildNode(curNode, comp);
      if (cInvalidNodeIndex != childNode)
      {
         BNode* pChildNode = &mNodes[childNode];
         
         if (lastComp)
         {
            if (!replaceIfExisting)
               return cErrorAlreadyExists;
               
            if ((fileFlags & cDirFlag) != (pChildNode->mFileFlags & cDirFlag))
               return cErrorCanNotReplace;
            
            pChildNode->mData = data;
            pChildNode->mFileFlags = fileFlags;
         }
         else
         {
            if (pChildNode->isFile())
               return cErrorInvalidPath;
         }
         
         curNode = childNode;
         continue;
      }
      
      uint newNode;
      BNode* pNewNode = mNodes.acquire(newNode, true);
      pNewNode->clear();
      
      pNewNode->mParentNodeIndex = curNode;
      pNewNode->mFileFlags = lastComp ? fileFlags : cDirFlag;
      pNewNode->mData = lastComp ? data : 0;
      pNewNode->mName.set(comp);
      
      if ((lastComp) && ((fileFlags & cDirFlag) == 0))
         mNumFiles++;
      
      const int insertionPoint = findChildInsertionPoint(pCurNode, comp);
      if (cInvalidIndex == insertionPoint)
         return cErrorAlreadyExists;
      
      BDEBUG_ASSERT(pCurNode->mFileFlags & cDirFlag);
      
      pCurNode->mChildren.insert(insertionPoint, newNode);         
      
      curNode = newNode;
   }
   
   if (pNodeIndex)
      *pNodeIndex = curNode;

   return cSucceeded;
}

BFilenameTree::eErrorCode BFilenameTree::remove(const char* pName, bool removeEmptyDirs)
{
   BNodeIndex curNode = findNode(pName);
   if (curNode == cInvalidNodeIndex)
      return cErrorPathNotFound;
   
//-- FIXING PREFIX BUG ID 667
   const BNode* pCurNode = &mNodes[curNode];
//--
   
   if (!pCurNode->isLeaf())
      return cErrorNotALeaf;

   if (curNode == mRootNodeIndex)      
      return cErrorCanNotDelete;
     
   for ( ; ; )
   {
      BDEBUG_ASSERT(curNode != mRootNodeIndex);
      BDEBUG_ASSERT(pCurNode->isLeaf());
      
      BNodeIndex parentNode = pCurNode->mParentNodeIndex;
      BDEBUG_ASSERT(cInvalidNodeIndex != parentNode);
                  
      BNode* pParentNode = &mNodes[parentNode];
      int index = pParentNode->mChildren.find(curNode);      
      BDEBUG_ASSERT(cInvalidIndex != index);
         
      pParentNode->mChildren.erase(index);
      
      if (pCurNode->isFile())
      {
         BDEBUG_ASSERT(mNumFiles);
         mNumFiles--;
      }
            
      mNodes.release(curNode);
      pCurNode = NULL;
                  
      if ((!removeEmptyDirs) || (parentNode == mRootNodeIndex) || (!pParentNode->isLeaf()))
         break;
            
      curNode = parentNode;
      pCurNode = pParentNode;
      
      BDEBUG_ASSERT((pCurNode->mFileFlags & cDirFlag) != 0);
   }
   
   return cSucceeded;
}

uint BFilenameTree::getNumFiles(void) const
{
   return mNumFiles;
}

BFilenameTree::BNodeIndex BFilenameTree::getRootNodeIndex(void) const
{
   return mRootNodeIndex;
}

BFilenameTree::BNodeIndex BFilenameTree::findNode(const char* pName) const
{
   BFixedStringMaxPath path;
   if (!canonicalizePathname(pName, path))
      return cInvalidNodeIndex;
   
   BNodeIndex curNode = mRootNodeIndex;
   
   const uint numComps = getNumFilenameComponents(path);
   
   for (uint compIndex = 0; compIndex < numComps; compIndex++)
   {
      BFixedStringMaxPath comp;
      getFilenameComponent(comp, path, compIndex);
      
      BNodeIndex childNode = findChildNode(curNode, comp);
      if (cInvalidNodeIndex == childNode)
         return cInvalidNodeIndex;
      
      curNode = childNode;
   }
   
   return curNode;
}

int BFilenameTree::findChildInsertionPoint(BNode* pNode, const char* pName) const
{
   BDEBUG_ASSERT(pNode && pName && pName[0]);

   const uint numChildren = pNode->mChildren.getSize();
   if (!numChildren)
      return 0;
   
   int l = 0;
   int r = numChildren - 1;
   int m = 0;
   int compResult = 0;
   while (r >= l)
   {
      m = (l + r) >> 1;
      const BNodeIndex childNodeIndex = pNode->mChildren[m];
      const BString& childNodeName = mNodes[childNodeIndex].mName;

      compResult = childNodeName.compare(pName);

      if (!compResult)
         return cInvalidIndex;
      else if (compResult > 0)
         r = m - 1;
      else
         l = m + 1;
   }
   
   int result;
   if (compResult > 0)
      result = m;
   else
      result = m + 1;
      
   BDEBUG_ASSERT((result >= 0) && (result <= (int)numChildren));
   
   if (result > 0)
   {
      const BNode* pPrevNode = &mNodes[pNode->mChildren[result - 1]];
      pPrevNode;
      BDEBUG_ASSERT(pPrevNode->mName.compare(pName) < 0);
   }
   
   if (result < (int)numChildren)
   {
      const BNode* pResultNode = &mNodes[pNode->mChildren[result]];
      pResultNode;
      BDEBUG_ASSERT(pResultNode->mName.compare(pName) > 0);
   }
        
   return result;
}

BFilenameTree::BNodeIndex BFilenameTree::findChildNode(BNodeIndex nodeIndex, const char* pName) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   BDEBUG_ASSERT(pName);
   
   if ((!pName[0]) || (strchr(pName, '\\')))
      return cInvalidNodeIndex;
      
   const BNode* pNode = &mNodes[nodeIndex];
   
   if ((pNode->isFile()) || (pNode->isLeaf()))
      return cInvalidNodeIndex;
            
   int l = 0;
   int r = pNode->mChildren.getSize() - 1;

   while (r >= l)
   {
      const int m = (l + r) >> 1;
      const BNodeIndex childNodeIndex = pNode->mChildren[m];
      const BString& childNodeName = mNodes[childNodeIndex].mName;

      const long compResult = childNodeName.compare(pName);
      
      if (compResult == 0)
         return childNodeIndex;
      else if (compResult > 0)
         r = m - 1;
      else
         l = m + 1;
   }

   return cInvalidNodeIndex;
}

bool BFilenameTree::isValidNodeIndex(BNodeIndex nodeIndex) const
{
   if ((nodeIndex < 0) || (nodeIndex >= (int)mNodes.getHighWaterMark()))
      return false;
   
   if (!mNodes.isInUse(nodeIndex))
      return false;
   
   return true;
}

BFilenameTree::BNodeIndex BFilenameTree::getParentNodeIndex(BNodeIndex nodeIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   return mNodes[nodeIndex].mParentNodeIndex;
}

const char* BFilenameTree::getNodeName(BNodeIndex nodeIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   return mNodes[nodeIndex].mName.getPtr();
}

bool BFilenameTree::getNodeFullName(BNodeIndex nodeIndex, BFixedStringMaxPath& fullName) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   fullName.empty();
   
   BNodeIndex nodeStack[cMaxDirectoryDepth];
   uint nodeStackSize = 0;
   
   BNodeIndex curNode = nodeIndex;
   for ( ; ; )
   {
      if (curNode == mRootNodeIndex)
         break;
      
      if (cMaxDirectoryDepth == nodeStackSize)
         return false;

      nodeStack[nodeStackSize++] = curNode;
               
      curNode = getParentNodeIndex(curNode);
      BDEBUG_ASSERT(cInvalidNodeIndex != curNode);
   }
   
   uint fullNameLen = 0;
   for (int i = nodeStackSize - 1; i >= 0; i--)
   {
      curNode = nodeStack[i];
      const BNode* pNode = &mNodes[curNode];
      
      const uint nameLen = pNode->mName.length();
      BDEBUG_ASSERT(nameLen);
      
      if ((fullNameLen + nameLen) >= MAX_PATH)
      {
         fullName.empty();
         return false;
      }
               
      memcpy(fullName.getPtr() + fullNameLen, pNode->mName.getPtr(), nameLen);
      fullNameLen += nameLen;
      
      if (i)
      {
         if ((fullNameLen + 1) >= MAX_PATH)
         {
            fullName.empty();
            return false;
         }
            
         fullName.getPtr()[fullNameLen] = '\\';
         fullNameLen++;
      }
   }
   
   BDEBUG_ASSERT(fullNameLen < MAX_PATH);
   
   fullName.getPtr()[fullNameLen] = '\0';
   
   return true;
}

uint BFilenameTree::getFileFlags(BNodeIndex nodeIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   return mNodes[nodeIndex].mFileFlags;
}

uint64 BFilenameTree::getFileData(BNodeIndex nodeIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   return mNodes[nodeIndex].mData;
}

bool BFilenameTree::setFileData(BNodeIndex nodeIndex, uint64 data)
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   mNodes[nodeIndex].mData = data;
   
   return true;
}

uint BFilenameTree::getNumChildren(BNodeIndex nodeIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(nodeIndex));
   
   return mNodes[nodeIndex].mChildren.getSize();
}

BFilenameTree::BNodeIndex BFilenameTree::getChildNodeIndex(BNodeIndex parentNodeIndex, uint childIndex) const
{
   BDEBUG_ASSERT(isValidNodeIndex(parentNodeIndex));
   
   const BNode* pNode = &mNodes[parentNodeIndex];
   
   if (childIndex >= pNode->mChildren.getSize())
      return cInvalidNodeIndex;
   
   const BNodeIndex childNodeIndex = pNode->mChildren[childIndex];
   
   BDEBUG_ASSERT(isValidNodeIndex(childNodeIndex));
   
   return childNodeIndex;
}

// blah.xxx\blah.xxx\blah.xxx
// \\machine\\blah.xxx
// game:\blah.xxx\blah.xxx
// c:\blah.xxx\blah.xxx
// In all cases, returns "blah.xxx\blah.xxx", and ensures each path doesn't start with a "."
//
// Examples of proper paths:
// ""
// "filename.ext"
// "path1.ext\path2.ext\filename.ext"
// "path1.ext\path2.ext\filename.ext.ext2"
// "path1.ext"
// "path1.ext\path2.ext"
 bool BFilenameTree::canonicalizePathname(const char* pSrc, BFixedStringMaxPath& path)
{
   BDEBUG_ASSERT(pSrc && (strlen(pSrc) < MAX_PATH));
   
   path.set(pSrc);
   path.trim();      
   path.standardizePath();
      
   int l = path.length();
   
   // An empty string is valid - it points to the root node.
   if (!l)
      return true;
      
   if ((l >= 2) && (path.get(0) == '\\') && (path.get(1) == '\\'))
   {
      if (l <= 2)
         return false;
         
      int i = path.findLeft('\\', 2);
      if (cInvalidIndex != i)
         path.substring(i + 1, l);
   }
   
   int firstSep = path.findLeft('\\');
   int firstColon = path.findLeft(':');
   if (cInvalidIndex != firstColon)
   {
      if ((cInvalidIndex == firstSep) || (firstColon < firstSep))
         path.right(firstColon + 1);
      else
         return false;
   }
      
   if (path.findLeft(':') != cInvalidIndex)
      return false;      
   
   if (path.isEmpty())
      return true;
      
   if (path.get(0) == '\\')
      path.right(1);
      
   if (path.isEmpty())
      return true;
   
   if (path.get(path.length() - 1) == '\\')
      path.truncate(path.length() - 1);
         
   uint startPos = 0;
   uint len = path.length();
   if (!len)
      return true;
   
   for ( ; ; )
   {
      if (startPos >= len)
         break;
               
      const int sepPos = path.findLeft('\\', startPos);
      if (cInvalidIndex == sepPos)
         break;
      
      const uint nameLen = sepPos - startPos;
      if (!nameLen)
         return false;
            
      char c = path.get(startPos);
      if (c == '.')
      {
         if (nameLen == 1)
            return false;
         else if ((nameLen == 2) && (path.get(startPos + 1) == '.'))
            return false;
      }
      
      startPos = sepPos + 1;
   }
      
   return true;
}

uint BFilenameTree::getNumFilenameComponents(const char* pName)
{
   BDEBUG_ASSERT(pName);
   
   if (!(*pName))
      return 0;
      
   uint num = 1;
   
   while (*pName)
   {
      if (*pName++ == '\\')
         num++;
   }
   
   return num;
}

void BFilenameTree::getFilenameComponent(BFixedStringMaxPath& comp, const char* pName, uint compIndex)
{
   BDEBUG_ASSERT(pName);
   
   uint startOfs = 0;
   
   uint curCompIndex = 0;

   int ofs = 0; 
   for ( ; ; )
   {
      char c = pName[ofs];
      
      if (!c)
         break;
      else if (c == '\\')
      {
         if (curCompIndex == compIndex)
            break;
            
         curCompIndex++;
         
         if (curCompIndex == compIndex)
            startOfs = ofs + 1;
      }
      
      ofs++;
   }
   
   comp.setBuf(pName + startOfs, ofs - startOfs);
}

BFilenameTree::eErrorCode BFilenameTree::findFilesNonRecursive(const char* pSpec, uint findFileFlags, BFindFilesResults& results) const
{
   if (!findFileFlags)
      return cSucceeded;

   BFixedStringMaxPath path;
   if (!canonicalizePathname(pSpec, path))
      return cErrorInvalidPath;

   if (path.isEmpty())      
      return cErrorInvalidPath;

   BNodeIndex curNode = mRootNodeIndex;      
   const BNode* pCurNode = &mNodes[curNode];

   const uint numComps = getNumFilenameComponents(path);
   if (!numComps)
      return cErrorFailed;

   BFixedStringMaxPath pathComp;

   if (numComps > 1)
   {
      for (uint compIndex = 0; compIndex < numComps - 1; compIndex++)
      {
         getFilenameComponent(pathComp, path, compIndex);

         BNodeIndex childNode = findChildNode(curNode, pathComp);
         if (cInvalidNodeIndex == childNode)
            return cErrorPathNotFound;

         curNode = childNode;
         pCurNode = &mNodes[curNode];
      }         
   }      

   getFilenameComponent(pathComp, path, numComps - 1);

   const uint numChildren = pCurNode->mChildren.getSize();
   for (uint childIndex = 0; childIndex < numChildren; childIndex++)
   {
      BNodeIndex childNode = pCurNode->mChildren[childIndex];
      const BNode* pChildNode = &mNodes[childNode];

      if (pChildNode->isFile())
      {
         if ((findFileFlags & cFindFilesWantFiles) == 0)
            continue;
      }
      else
      {
         if ((findFileFlags & cFindFilesWantDirs) == 0)
            continue;
      }

      if (!wildcmp(pathComp, pChildNode->mName))
         continue;

      BFindFilesData& resultObj = *results.enlarge(1);

      resultObj.mNodeIndex = childNode;
      resultObj.mFileFlags = pChildNode->mFileFlags;

      resultObj.mName = pChildNode->mName;
   }

   return cSucceeded;
}

static void appendBackSlash(BFixedStringMaxPath& path)
{
   uint len = path.length();
   if (len)
   {
      if ((path.get(len - 1) != '\\') && (path.get(len - 1) != '/'))
         path.appendChar('\\');
   }
}

BFilenameTree::eErrorCode BFilenameTree::findFilesInternal(const char* pStartPath, const char* pRelPath, const char* pFilename, uint findFilesFlags, BFindFilesResults& userResults)
{
   BFixedStringMaxPath fullPath(pStartPath);
         
   appendBackSlash(fullPath);
   if (pRelPath[0])
   {
      fullPath.append(pRelPath);
      appendBackSlash(fullPath);
   }
   
   BFindFilesResults findResults;
   BFixedStringMaxPath findPath(fullPath);
   
   findPath += pFilename;
   eErrorCode errorCode = findFilesNonRecursive(findPath, findFilesFlags, findResults);
   if (cSucceeded != errorCode)
      return errorCode;
   
   for (uint i = 0; i < findResults.getSize(); i++)
   {
      BFindFilesData& resultObj = *userResults.enlarge(1);
      
      BFixedStringMaxPath fullName(fullPath);
      fullName += findResults[i].mName;
      resultObj.mFullname.set(fullName);
            
      resultObj.mRelPath.set(pRelPath);
      resultObj.mName.set(findResults[i].mName);
      
      resultObj.mFileFlags = findResults[i].mFileFlags;
      resultObj.mNodeIndex = findResults[i].mNodeIndex;
   }

   if (findFilesFlags & cFindFilesRecurse)      
   {
      findResults.resize(0);
      errorCode = findFilesNonRecursive(fullPath + "*", BFilenameTree::cFindFilesWantDirs, findResults);
      if (cSucceeded != errorCode)
         return errorCode;
         
      for (uint i = 0; i < findResults.getSize(); i++)
      {
         findPath.set(pRelPath);
         appendBackSlash(findPath);         
         findPath += findResults[i].mName;
         
         errorCode = findFilesInternal(pStartPath, findPath, pFilename, findFilesFlags, userResults);
         if (errorCode != cSucceeded)
            return errorCode;
      }
   }      
   
   return cSucceeded;
}

BFilenameTree::eErrorCode BFilenameTree::findFiles(const char* pPath, const char* pFilename, uint findFilesFlags, BFindFilesResults& userResults)
{
   return findFilesInternal(pPath, "", pFilename, findFilesFlags, userResults);
}

void BFilenameTree::floodFill(UIntSet& nodesFound, uint nodeIndex) const
{
   nodesFound.insert(nodeIndex);
   
   const BNode& node = mNodes[nodeIndex];
   for (uint i = 0; i < node.mChildren.getSize(); i++)
      floodFill(nodesFound, node.mChildren[i]);
}

bool BFilenameTree::check(void) const
{
   if (mRootNodeIndex == cInvalidNodeIndex)
      return false;
      
   const BNode& rootNode = mNodes[mRootNodeIndex];
   if (!rootNode.isRoot())
      return false;
      
   if ((rootNode.mFileFlags & cDirIsRoot) == 0)
      return false;
      
   if (mNumFiles > mNodes.getNumberAllocated())
      return false;
      
   uint numFilesFound = 0;
   
   for (int i = 0; i < (int)mNodes.getHighWaterMark(); i++)
   {
      if (!mNodes.isInUse(i))
         continue;
         
      const BNode& node = mNodes[i];
            
      if (node.isRoot())
      {
         if ((node.mFileFlags & cDirIsRoot) == 0)
            return false;
         
         if (mRootNodeIndex != i)
            return false;
         
         if (node.mParentNodeIndex != cInvalidNodeIndex)
            return false;
      }
      else
      {
         if (!isValidNodeIndex(node.mParentNodeIndex))
            return false;
            
         if (node.mName.isEmpty())
            return false;
      }
      
      if (node.isFile())
      {
         numFilesFound++;
         
         if (!node.mChildren.isEmpty())
            return false;
      }
      else
      {
         for (int j = 0; j < (int)node.mChildren.getSize(); j++)
         {
            if (!isValidNodeIndex(node.mChildren[j]))
               return false;
               
            if (i == (int)node.mChildren[j])
               return false;
               
            if (j < ((int)node.mChildren.getSize() - 1))
            {
               const BString& a = mNodes[node.mChildren[j]].mName;
               const BString& b = mNodes[node.mChildren[j + 1]].mName;
               
               if (a >= b)
                  return false;
            }
                                    
            if (mNodes[node.mChildren[j]].mParentNodeIndex != i)
               return false;
         }
      }
   }
   
   if (numFilesFound != mNumFiles)
      return false;
   
   UIntSet nodesFound;      
   floodFill(nodesFound, mRootNodeIndex);
   
   for (int i = 0; i < (int)mNodes.getHighWaterMark(); i++)
   {
      if (!mNodes.isInUse(i))
      {
         if (nodesFound.find(i) != nodesFound.end())
            return false;
            
         continue;
      }

      if (nodesFound.find(i) == nodesFound.end())
         return false;
   }      

   return true;
}





