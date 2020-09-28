//-------------------------------------------------------------------------------------------------
// File: aabb_tree.h
//-------------------------------------------------------------------------------------------------
#pragma once

#include "math\generalVector.h"
#include "math\vectorInterval.h"

//-------------------------------------------------------------------------------------------------
// class BAABBTree
//-------------------------------------------------------------------------------------------------
class BAABBTree
{
public:
   enum { StreamVersion = 0x33440002 };

   // Max possible # of nodes that may be allocated
   enum { MaxNodes = 4000000 };
   
   //-------------------------------------------------------------------------------------------------
   // sub-class BNode
   //-------------------------------------------------------------------------------------------------
   class BNode
   {
   public:
      BNode()
      {
         clear();
      }

      void clear(void)
      {
         mpParent = NULL;
         mBounds.initExpand();
         Utils::ClearObj(mpChildren);
         mIndex = 0;
         mSplitPlane = 0;
      }

      const AABB& bounds(void) const   { return mBounds; }
            AABB& bounds(void)         { return mBounds; }
            
      const float splitPlane(void) const  { return mSplitPlane; }
            float& splitPlane(void)       { return mSplitPlane; }

      int numObjects(void) const { return static_cast<int>(mObjIndices.size()); }

      int& objIndex(int i)       { return mObjIndices[debugRangeCheck(i, mObjIndices.size())]; }
      int  objIndex(int i) const { return mObjIndices[debugRangeCheck(i, mObjIndices.size())]; }

      const IntVec& objIndices(void) const   { return mObjIndices; }
            IntVec& objIndices(void)         { return mObjIndices; }

      BNode* const & parent(void) const   { return mpParent; }
      BNode*       & parent(void)         { return mpParent; }
      
      BNode* const & child(int i) const   { return mpChildren[debugRangeCheck(i, 2)]; }
      BNode*       & child(int i)         { return mpChildren[debugRangeCheck(i, 2)]; }

      uint32  index(void) const  { return mIndex; }
      uint32& index(void)        { return mIndex; }

      bool leaf(void) const 
      {
         if (!child(0))
            return true;
         BDEBUG_ASSERT(child(0) && child(1));
         return false;
      }

      int maxDepth(void) const
      {
         if (leaf())
            return 1;
         return 1 + Math::Max(child(0)->maxDepth(), child(1)->maxDepth());
      }
      
      friend BStream& operator<< (BStream& dst, const BNode& src)
      {
         return dst << src.mBounds << (uint)src.mpParent << (uint)src.mpChildren[0] << (uint)src.mpChildren[1] << src.mIndex << src.mObjIndices << src.mSplitPlane;
      }
      
      friend BStream& operator>> (BStream& src, BNode& dst)
      {
         return src >> dst.mBounds >> (uint&)dst.mpParent >> (uint&)dst.mpChildren[0] >> (uint&)dst.mpChildren[1] >> dst.mIndex >> dst.mObjIndices >> dst.mSplitPlane;
      }
      
      void offsetize(void* pBase)
      {
         Utils::Offsetize(mpParent, pBase);
         Utils::Offsetize(mpChildren[0], pBase);
         Utils::Offsetize(mpChildren[1], pBase);
      }
      
      void pointerize(void* pBase)
      {
         Utils::Pointerize(mpParent, pBase);
         Utils::Pointerize(mpChildren[0], pBase);
         Utils::Pointerize(mpChildren[1], pBase);
      }

   private:
      AABB     mBounds;
      IntVec   mObjIndices;
      BNode*   mpParent;
      BNode*   mpChildren[2];
      uint32   mIndex;
      float    mSplitPlane;
   };
   
   //-------------------------------------------------------------------------------------------------
   // BAABBTree      
   //-------------------------------------------------------------------------------------------------
   BAABBTree()
   {
   }

   //-------------------------------------------------------------------------------------------------      
   // ~BAABBTree
   //-------------------------------------------------------------------------------------------------
   ~BAABBTree()
   {
   }
   
   //-------------------------------------------------------------------------------------------------      
   // clear
   //-------------------------------------------------------------------------------------------------      
   void clear(void)
   {
      mNodes.clear();
   }
   
   //-------------------------------------------------------------------------------------------------      
   // prepareForBuild
   //-------------------------------------------------------------------------------------------------      
   void prepareForBuild(void)
   {
      mNodes.reserve(MaxNodes);
   }

   //-------------------------------------------------------------------------------------------------      
   // root
   //-------------------------------------------------------------------------------------------------
   const BNode* root(void) const
   { 
      if (mNodes.empty()) 
         return NULL;
      return &mNodes[0]; 
   }
   
   //-------------------------------------------------------------------------------------------------
   // root
   //-------------------------------------------------------------------------------------------------
   BNode* root(void) 
   { 
      if (mNodes.empty()) 
         return NULL;
      return &mNodes[0]; 
   }
   
   //-------------------------------------------------------------------------------------------------      
   // numNodes
   //-------------------------------------------------------------------------------------------------
   int numNodes(void) const
   {
      return static_cast<int>(mNodes.size());
   }

   //-------------------------------------------------------------------------------------------------      
   // index
   //-------------------------------------------------------------------------------------------------
   int index(const BNode* pNode) const
   {
      BDEBUG_ASSERT(!mNodes.empty());
      BDEBUG_ASSERT(pNode >= root());
      const int i = pNode - root();
      BDEBUG_ASSERT(i < numNodes());
      return i;
   }
   
   //-------------------------------------------------------------------------------------------------      
   // node
   //-------------------------------------------------------------------------------------------------
   const BNode& node(int i) const   { return mNodes[debugRangeCheck(i, numNodes())]; }
         BNode& node(int i)         { return mNodes[debugRangeCheck(i, numNodes())]; }
   
   //-------------------------------------------------------------------------------------------------                  
   // operator<<
   //-------------------------------------------------------------------------------------------------
   friend BStream& operator<< (BStream& dst, const BAABBTree& srcC)
   {
      //EVIL
      BAABBTree& src = const_cast<BAABBTree&>(srcC);
      
      src.offsetize();
      dst << static_cast<uint32>(StreamVersion) << src.mNodes << static_cast<uint32>(StreamVersion);
      src.pointerize();
      return dst;
   }
   
   friend BStream& operator>> (BStream& src, BAABBTree& dst)
   {
      uint32 streamVersion;
      
      src >> streamVersion;
      
      dst.mNodes.clear();
            
      if (StreamVersion != streamVersion)
      {
         trace("\nBAABBTree: File Version Mismatch!\nFile: \"%s\"\nUpdate GR2UGX.EXE and reexport.\n\nVersion Expected: %08X, Version Found: %08X",
            src.getName().getPtr(),
            StreamVersion,
            streamVersion);
         return src;
      }               
                  
      src >> dst.mNodes;
      
      src >> streamVersion;
      
      BVERIFY(StreamVersion == streamVersion);

      dst.pointerize();
      
      return src;
   }
   
   // should be protected!
   BNode* allocNode(void) 
   {
      BDEBUG_ASSERT(mNodes.capacity() == MaxNodes);
      BVERIFY(mNodes.size() != MaxNodes && "Increase MaxNodes!");

      BNode* pPrevRoot = root();
      pPrevRoot;

      mNodes.push_back(BNode());

      // make sure the vector doesn't decide to realloc the array, which would screw our pointers
      BDEBUG_ASSERT(!pPrevRoot || (pPrevRoot == root()));

      return &mNodes.back();         
   }

   //-------------------------------------------------------------------------------------------------
   // freeLastNode
   //-------------------------------------------------------------------------------------------------
   void freeLastNode(void)
   {
      BDEBUG_ASSERT(mNodes.size() > 0);
      mNodes.resize(mNodes.size() - 1);
   }
   
   //-------------------------------------------------------------------------------------------------      
   // checkTree
   //-------------------------------------------------------------------------------------------------
   void checkTree(void)
   {
#ifdef DEBUG      
      for (int i = 0; i < numNodes(); i++)
      {
         BNode* pNode = &mNodes[i];
         BNode* pParentNode = pNode->parent();
         if (pParentNode)
         {
            if (pParentNode->child(0) == pNode)
            {
            }
            else if (pParentNode->child(1) == pNode)
            {
            }
            else
               BDEBUG_ASSERT(false);
         }
      }
#endif         
   }
   
   //-------------------------------------------------------------------------------------------------      
   // deleteNode
   // deletes node by replacing it with the last node in the array
   // fixes parent/child pointers of the last node
   //-------------------------------------------------------------------------------------------------
   void deleteNode(int i)
   {
      debugRangeCheck(i, static_cast<int>(mNodes.size()));
                        
      const int lastNodeIndex = numNodes() - 1;
      
      // replace node i with the last node
      if (i != lastNodeIndex)
      {
         BDEBUG_ASSERT(lastNodeIndex >= 0);
         const BNode& lastNode = mNodes[lastNodeIndex];
         BNode* pParentNode = lastNode.parent();
                    
         // fix parent's child pointer
         if (pParentNode)
         {
            if (pParentNode->child(0) == &lastNode)
               pParentNode->child(0) = &mNodes[i];
            else if (pParentNode->child(1) == &lastNode)
               pParentNode->child(1) = &mNodes[i];
            else
               BVERIFY(false);
         }                  
         
         mNodes[i] = lastNode;
         // fix children's parent pointers
         if (mNodes[i].child(0)) mNodes[i].child(0)->parent() = &mNodes[i];
         if (mNodes[i].child(1)) mNodes[i].child(1)->parent() = &mNodes[i];
      }
                  
      freeLastNode();
   }

   //-------------------------------------------------------------------------------------------------            
   // deleteLeafNode
   // deletes leaf, collapses parent's tree
   //-------------------------------------------------------------------------------------------------      
   void deleteLeafNode(int i)
   {
      checkTree();
      
      debugRangeCheck(i, static_cast<int>(mNodes.size()));
      BNode* pLeaf = &mNodes[i];
      BDEBUG_ASSERT(pLeaf->leaf());
      
      // delete pLeaf from the tree
      BNode* pParent = pLeaf->parent();
      if (pParent->child(0) == pLeaf)
         pParent->child(0) = NULL;
      else
      {
         BVERIFY(pParent->child(1) == pLeaf);
         pParent->child(1) = NULL;
      }
      
      pLeaf->parent() = NULL;
      
      // if pParent is the last node, it's going to be moved into the leaf's place!
      
      if (pParent == &mNodes[numNodes() - 1])
         pParent = pLeaf;
      
      pLeaf = NULL;
         
      // now delete the unreferenced node
      deleteNode(i);
                        
      checkTree();
                                 
      // find the leaf node's sibling, and move it up
      BNode* pSiblingNode;
      if (pParent->child(0) == NULL)
         pSiblingNode = pParent->child(1);
      else
      {
         BVERIFY(pParent->child(1) == NULL);
         pSiblingNode = pParent->child(0);
      }
      BDEBUG_ASSERT(pSiblingNode);
               
      // move other  node up 1 level
      pParent->child(0) = pSiblingNode->child(0);
      pParent->child(1) = pSiblingNode->child(1);
      pParent->objIndices().swap(pSiblingNode->objIndices());
      pParent->bounds() = pSiblingNode->bounds();
      pParent->index() = pSiblingNode->index();
      
      // fix other leaf's parent pointer
      if (pSiblingNode->child(0)) pSiblingNode->child(0)->parent() = pParent;
      if (pSiblingNode->child(1)) pSiblingNode->child(1)->parent() = pParent;

#if 0      
      if (pParent->leaf())
         BDEBUG_ASSERT(pParent->numObjects() > 0);
      else
         BDEBUG_ASSERT(pParent->numObjects() == 0);

      BDEBUG_ASSERT(pParent->bounds().valid());                  
#endif      
      
      // now delete both child nodes
      deleteNode(pSiblingNode - &mNodes[0]);
      
      checkTree();
   }
               
protected:
   BDynamicArray<BNode> mNodes;

   //-------------------------------------------------------------------------------------------------            
   // offsetize
   //-------------------------------------------------------------------------------------------------            
   void offsetize(void) 
   {
      for (uint i = 0; i < mNodes.size(); i++)
         mNodes[i].offsetize(root());
   }

   //-------------------------------------------------------------------------------------------------            
   // pointerize
   //-------------------------------------------------------------------------------------------------            
   void pointerize(void)
   {
      for (uint i = 0; i < mNodes.size(); i++)
         mNodes[i].pointerize(root());
   }
};
