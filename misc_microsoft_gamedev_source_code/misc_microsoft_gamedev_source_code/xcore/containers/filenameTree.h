//==============================================================================
//
// File: filenameTree.h
//
// Copyright (c) 2007, Ensemble Studios
//
//==============================================================================
#pragma once
#include "freelist.h"
#include "containers\hashMap.h"

//==============================================================================
// class BFilenameTree
//==============================================================================
class BFilenameTree
{
   BFilenameTree(const BFilenameTree&);
   BFilenameTree& operator= (const BFilenameTree&);
   
public:
   enum { cMaxDirectoryDepth = 24 };
   
   typedef int BNodeIndex;
   enum { cInvalidNodeIndex = -1 };
   
   BFilenameTree();
   ~BFilenameTree();
   
   void              clear(void);
   
   uint              getNumFiles(void) const;
   
   enum
   {
      cDirFlag    = 1,
      cDirIsRoot  = 2
   };
   
   enum eErrorCode
   {
      cSucceeded                 = 0,
      cErrorFailed               = -1,
      cErrorInvalidPath          = -2,
      cErrorAlreadyExists        = -3,
      cErrorFileNotFound         = -4,
      cErrorPathNotFound         = -5,
      cErrorCanNotReplace        = -6,
      cErrorNotALeaf             = -7,
      cErrorCanNotDelete         = -8,
   };
   
   eErrorCode        add(const char* pName, uint fileFlags, uint64 data, bool replaceIfExisting = true, BNodeIndex* pNodeIndex = NULL);
   
   // Only leaf nodes can be removed!
   eErrorCode        remove(const char* pName, bool removeEmptyDirs);
            
   struct BFindFilesData
   {
      BFindFilesData() : mFileFlags(0), mNodeIndex(0) { }
      
      BString     mFullname;            
      BString     mRelPath;
      BString     mName;
            
      uint        mFileFlags;
      BNodeIndex  mNodeIndex;
   };
   
   typedef BDynamicArray<BFindFilesData> BFindFilesResults;
   
   enum
   {
      cFindFilesWantFiles = 1,
      cFindFilesWantDirs = 2,
      cFindFilesRecurse = 4
   };
               
   eErrorCode        findFiles(const char* pPath, const char* pFilename, uint findFilesFlags, BFindFilesResults& results);
   
   eErrorCode        findFilesNonRecursive(const char* pSpec, uint findFileFlags, BFindFilesResults& results) const;
      
   BNodeIndex        getRootNodeIndex(void) const; 
   bool              isValidNodeIndex(BNodeIndex nodeIndex) const;
   uint              getNumChildren(BNodeIndex nodeIndex) const;
   BNodeIndex        getChildNodeIndex(BNodeIndex parentNodeIndex, uint childIndex) const;
   
   BNodeIndex        getParentNodeIndex(BNodeIndex nodeIndex) const;
         
   BNodeIndex        findNode(const char* pName) const;
   BNodeIndex        findChildNode(BNodeIndex nodeIndex, const char* pName) const;
   
   const char*       getNodeName(BNodeIndex nodeIndex) const;
   bool              getNodeFullName(BNodeIndex nodeIndex, BFixedStringMaxPath& fullName) const;
   uint              getFileFlags(BNodeIndex nodeIndex) const;
   uint64            getFileData(BNodeIndex nodeIndex) const;
   bool              setFileData(BNodeIndex nodeIndex, uint64 data);
   
   bool              check(void) const;
         
   static bool canonicalizePathname(const char* pSrc, BFixedStringMaxPath& path);
      
private:
   struct BNode
   {
      BNode() :
         mParentNodeIndex(cInvalidNodeIndex),
         mFileFlags(0),
         mData(0)
      {
      }
      
      void clear(void)
      {
         mParentNodeIndex = cInvalidNodeIndex;
         mFileFlags = 0;
         mData = 0;
         mName.empty();
         mChildren.clear();
      }
      
      BNodeIndex              mParentNodeIndex;
      
      uint                    mFileFlags;
      uint64                  mData;
      
      BString                 mName;
      
      BDynamicArray<uint>     mChildren;       
      
      BOOL isFile(void) const { return 0 == (mFileFlags & cDirFlag); }
      BOOL isDir(void) const { return 0 != (mFileFlags & cDirFlag); }
      BOOL isLeaf(void) const { return mChildren.isEmpty(); }
      BOOL isRoot(void) const { return cInvalidNodeIndex == mParentNodeIndex; }
   };
   
   BFreeList<BNode>  mNodes;
   
   BNodeIndex        mRootNodeIndex;
   BNode*            mpRootNode;
   
   uint              mNumFiles;

   void        initRoot(void);
   int         findChildInsertionPoint(BNode* pNode, const char* pName) const;   
   
   typedef BHashMap<uint> UIntSet;
   void        floodFill(UIntSet& nodesFound, uint nodeIndex) const;
   
   eErrorCode  findFilesInternal(const char* pStartPath, const char* pRelPath, const char* pFilename, uint findFilesFlags, BFindFilesResults& results);
   
   static uint getNumFilenameComponents(const char* pName);
   static void getFilenameComponent(BFixedStringMaxPath& comp, const char* pName, uint compIndex);
};

