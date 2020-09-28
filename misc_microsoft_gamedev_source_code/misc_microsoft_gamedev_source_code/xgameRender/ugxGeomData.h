//============================================================================
//
//  ugxGeomData.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// local
#include "vertexDeclManager.h"

// xgeom
#include "ugxGeom.h"
#include "aabbTree.h"

class BECFFileData;

//============================================================================
// class BUGXGeomData
//============================================================================
class BUGXGeomData
{
public:
   BUGXGeomData();
   ~BUGXGeomData();
   
   void clear(void);
      
   // Takes ownership. 
   bool setFileData(BECFFileData* pECFFileData, const BUnpackedUnivertPackerType* pBonePacker = NULL);
   
   const BECFFileData* getECFFileData(void) const { return mpECFFileData; }   
   const BUGXGeom::BNativeCachedDataType* getCachedData(void) const { return mpCachedData; }
         
   IDirect3DVertexBuffer9* getVB(void) { return &mVB; }
   IDirect3DIndexBuffer9* getIB(void) { return &mIB; }

   uint calcSectionIndices(ushort* pSectionIndices, const ushort* pAccessoryIndices = NULL, uint numAccessoryIndices = 0);

   void setIndexBuffer(void);
   
   void drawSectionInit(uint sectionIndex);
   void drawSection(uint sectionIndex, uint numInstances);
   
   IDirect3DVertexDeclaration9* getVertexDecl(uint sectionIndex) const { return gVertexDeclManager.get(mVertexDeclHandles[sectionIndex]).getDecl(); }
   uint getStream0Stride(uint sectionIndex) const { const BUGXGeom::BNativeSectionType& section = mpCachedData->section(sectionIndex); return section.vertSize(); }
   
   BVertexDeclManager::Handle getSectionVertexDeclHandle(uint sectionIndex) const { return mVertexDeclHandles[sectionIndex]; }
   
   float getInstanceIndexMultiplier(void) const { return mInstanceIndexMultiplier; }
   float getInstanceIndexDivider(void) const { return mInstanceIndexDivider; }
   
   void* getIndexBufferData(void) const { return mpIBData; }
   
   typedef BDynamicArray<Unigeom::BMaterial, ALIGN_OF(Unigeom::BMaterial), BDynamicArrayRenderHeapAllocator> BUnigeomMaterialArray;
   bool getMaterials(BUnigeomMaterialArray& materials);
   
   bool getAABBTree(BAABBTree& tree);
               
protected:
   BECFFileData*                             mpECFFileData;
   const BUGXGeom::BNativeCachedDataType*    mpCachedData;
     
   float                                     mInstanceIndexMultiplier;
   float                                     mInstanceIndexDivider;
   
   typedef BDynamicArray<BVertexDeclManager::Handle, ALIGN_OF(BVertexDeclManager::Handle), BDynamicArrayRenderHeapAllocator> BVertexDeclHandleArray;
   BVertexDeclHandleArray                    mVertexDeclHandles;
   
   void*                                     mpIBData;
   
   IDirect3DVertexBuffer9                    mVB;
   IDirect3DIndexBuffer9                     mIB;
   
   void clearVB(void);
   void clearIB(void);
   
   bool init(const BUnpackedUnivertPackerType* pBonePacker);
   bool createIB(void);
   bool createVB(void);
      
   BVertexDeclManager::Handle createVertexDecl(const BUGXGeom::BNativeSectionType& section, const BUnpackedUnivertPackerType* pBonePacker);
   
}; // class BUGXGeomData
