//============================================================================
//
//  ugxGeomData.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "resource\ecfFileData.h"
#include "stream\byteStream.h"
#include "renderThread.h"

// local
#include "ugxGeomData.h"
#include "ugxGeomRenderTypes.h"
#include "BD3D.h"

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::BUGXGeomData
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomData::BUGXGeomData() : 
   mpECFFileData(NULL),
   mpCachedData(NULL),
   mInstanceIndexMultiplier(0),
   mInstanceIndexDivider(0),
   mpIBData(NULL)
{
   Utils::ClearObj(mVB);
   Utils::ClearObj(mIB);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::~BUGXGeomData
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomData::~BUGXGeomData()
{
   clear();
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::clearIB
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::clearIB(void)
{
   if (gRenderThread.getHasD3DOwnership())
   {
      D3DIndexBuffer* pIB;
      BD3D::mpDev->GetIndices(&pIB);
      if (pIB)
      {
         pIB->Release();
         if (pIB == &mIB)
            BD3D::mpDev->SetIndices(NULL);
      }
   }      

   if (mIB.Address)
   {
      BDEBUG_ASSERT(mIB.ReferenceCount == 1);
      Utils::ClearObj(mIB);
   }
   
   mpIBData = NULL;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::clearVB
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::clearVB(void)
{
   if (gRenderThread.getHasD3DOwnership())
   {
      for (uint streamIndex = 0; streamIndex < 4; streamIndex++)
      {
         D3DVertexBuffer* pVB;
         UINT stride, offset;
         BD3D::mpDev->GetStreamSource(streamIndex, &pVB, &offset, &stride);
         if (pVB)
         {
            pVB->Release();
            if (pVB == &mVB)
               BD3D::mpDev->SetStreamSource(streamIndex, NULL, 0, 0);
         }
      }
   }      

   if ((mVB.Format.dword[0]) || (mVB.Format.dword[1]))
   {
      BDEBUG_ASSERT(mVB.ReferenceCount == 1);
      Utils::ClearObj(mVB);
   }
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::clear
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::clear(void)
{
   clearVB();
   clearIB();
   
   if (mpECFFileData)
   {
      HEAP_DELETE(mpECFFileData, gRenderHeap);
      mpECFFileData = NULL;
   }

   mInstanceIndexMultiplier = 0;
   mInstanceIndexDivider = 0;

   mVertexDeclHandles.resize(0);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::setFileData
//------------------------------------------------------------------------------------------------------------------------------
bool BUGXGeomData::setFileData(BECFFileData* pECFFileData, const BUnpackedUnivertPackerType* pBonePacker)
{
   clear();
   
   mpECFFileData = pECFFileData;
      
   if (!init(pBonePacker))
   {
      clear();
      return false;
   }
   
   return true;
}
//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::calcSectionIndices
//------------------------------------------------------------------------------------------------------------------------------
uint BUGXGeomData::calcSectionIndices(ushort* pSectionIndices, const ushort* pAccessoryIndices, uint numAccessoryIndices)
{
   int numSectionsToRender = 0;

   if ((!pAccessoryIndices) || (numAccessoryIndices == mpCachedData->validAccessories().size()))
   {
      numSectionsToRender = mpCachedData->numSections();
      for (int i = 0; i < numSectionsToRender; i++)
         pSectionIndices[i] = static_cast<ushort>(i);
   }
   else
   {
      for (uint activeAccessoryIndex = 0; activeAccessoryIndex < numAccessoryIndices; activeAccessoryIndex++)
      {
         const int accessoryIndex = pAccessoryIndices[activeAccessoryIndex];
         const Unigeom::BNativeAccessoryType& accessory = mpCachedData->accessory(accessoryIndex);

         for (int modelSectionIndex = 0; modelSectionIndex < accessory.numObjectIndices(); modelSectionIndex++)
         {
            const int sectionIndex = accessory.objectIndex(modelSectionIndex);

            pSectionIndices[numSectionsToRender++] = static_cast<ushort>(sectionIndex);
         }
      }

      std::sort(pSectionIndices, pSectionIndices + numSectionsToRender);
   }

   return numSectionsToRender;      
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::setIndexBuffer
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::setIndexBuffer(void)
{
   gpUGXD3DDev->SetIndices(&mIB);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::drawSectionInit
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::drawSectionInit(uint sectionIndex)
{
   gVertexDeclManager.setToDevice(mVertexDeclHandles[sectionIndex], gpUGXD3DDev);            

   const BUGXGeom::BNativeSectionType& section = mpCachedData->section(sectionIndex);

   gpUGXD3DDev->SetStreamSource(0, &mVB, section.VBOfs(), section.vertSize());
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::drawSection
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomData::drawSection(uint sectionIndex, uint numInstances)
{
   BDEBUG_ASSERT((numInstances > 0) && (numInstances <= mpCachedData->header().getMaxInstances()));

   const BUGXGeom::BNativeSectionType& section = mpCachedData->section(sectionIndex);
   
   gpUGXD3DDev->DrawIndexedVertices(D3DPT_TRIANGLELIST, 0, section.IBOfs(), section.numTris() * 3 * numInstances);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::init
//------------------------------------------------------------------------------------------------------------------------------
bool BUGXGeomData::init(const BUnpackedUnivertPackerType* pBonePacker)
{
   int chunkIndex = mpECFFileData->findChunkByID((uint64)BUGXGeom::cECFCachedDataChunkID);
   if (cInvalidIndex == chunkIndex)
      return false;

   BUGXGeom::BNativeCachedDataType* pCachedData = static_cast<BUGXGeom::BNativeCachedDataType*>(mpECFFileData->getChunkDataPtr(chunkIndex));
      
   if ((!pCachedData) || (!pCachedData->checkHeader()))
      return false;

   if (!pCachedData->unpack(BDataBuffer(pCachedData, mpECFFileData->getChunkDataLen(chunkIndex))))
      return false;
      
   mpCachedData = pCachedData;      
              
   mVertexDeclHandles.resize(mpCachedData->numSections());
   
   for (int sectionIndex = 0; sectionIndex < mpCachedData->numSections(); sectionIndex++)
      mVertexDeclHandles[sectionIndex] = createVertexDecl(mpCachedData->section(sectionIndex), pBonePacker);

   if (!createIB())
      return false;
      
   if (!createVB())
      return false;
      
   const BUGXGeom::BNativeCachedDataType::GeomHeaderType& header = mpCachedData->header();

   mInstanceIndexMultiplier = 1.0f / header.getInstanceIndexMultiplier();
   mInstanceIndexDivider = (float)header.getInstanceIndexMultiplier();
   
   return true;      
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::createIB
//------------------------------------------------------------------------------------------------------------------------------
bool BUGXGeomData::createIB(void)
{
   int chunkIndex = mpECFFileData->findChunkByID((uint64)BUGXGeom::cECFIBChunkID);
   if (chunkIndex < 0)
      return false;

   void* pIndices = mpECFFileData->getChunkDataPtr(chunkIndex);
   if (!pIndices)
      return false;
   
   const int indicesSize = mpECFFileData->getChunkDataLen(chunkIndex);
   if (indicesSize < 0)
      return false;

   if (mpECFFileData->getChunkDataMemProtect(chunkIndex) == BECFFileData::cPhysicalCached)
      XGSetIndexBufferHeader(indicesSize, D3DUSAGE_CPU_CACHED_MEMORY, D3DFMT_INDEX16, 0, 0, &mIB);
   else
      XGSetIndexBufferHeader(indicesSize, 0, D3DFMT_INDEX16, 0, 0, &mIB);
      
   XGOffsetResourceAddress(&mIB, pIndices);
   
   mpIBData = pIndices;
   
   return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::createVB
//------------------------------------------------------------------------------------------------------------------------------
bool BUGXGeomData::createVB(void)
{
   int chunkIndex = mpECFFileData->findChunkByID((uint64)BUGXGeom::cECFVBChunkID);
   if (chunkIndex < 0)
      return false;
      
   void* pVertices = mpECFFileData->getChunkDataPtr(chunkIndex);
   if (!pVertices)
      return false;

   const int verticesSize = mpECFFileData->getChunkDataLen(chunkIndex);
   if (verticesSize < 0)
      return false;

   if (mpECFFileData->getChunkDataMemProtect(chunkIndex) == BECFFileData::cPhysicalCached)
      XGSetVertexBufferHeader(verticesSize, D3DUSAGE_CPU_CACHED_MEMORY, 0, 0, &mVB);
   else
      XGSetVertexBufferHeader(verticesSize, 0, 0, 0, &mVB);
      
   XGOffsetResourceAddress(&mVB, pVertices);

   return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomData::createVertexDecls
//------------------------------------------------------------------------------------------------------------------------------
BVertexDeclManager::Handle BUGXGeomData::createVertexDecl(const BUGXGeom::BNativeSectionType& section, const BUnpackedUnivertPackerType* pBonePacker)
{
   BUnivertPackerArray packers;
   packers.reserve(2);

   BUnpackedUnivertPackerType basePacker(section.baseVertPacker());         
   
   {
      const BNativeUnivertPackerType::ElementStats baseStats(section.baseVertPacker().getStats());
      const uint MAX_PASS0_TEXCOORDS = 4;
      
      BASSERT(1 == baseStats.numPos);
            
      IntVec basisMap;
      IntVec basisScalesMap;
      IntVec uvMap;

      BASSERT(baseStats.numTexCoords <= MAX_PASS0_TEXCOORDS);
      for (int i = 0; i < MAX_PASS0_TEXCOORDS; i++)
         uvMap.push_back(i);

      BASSERT(baseStats.numBasis <= 2);
      BASSERT(baseStats.numBasisScales <= 2);
      for (int i = 0; i < 2; i++)
      {
         basisMap.push_back(i);
         basisScalesMap.push_back(i);
      }

      basePacker.setDeclOrder(0, basisMap, basisScalesMap, 0, uvMap);

      packers.push_back(basePacker);
   }
   
   if (pBonePacker)
      packers.pushBack(*pBonePacker);

   return gVertexDeclManager.create(packers);     
}

//============================================================================
// BUGXGeomData::getMaterials
//============================================================================
bool BUGXGeomData::getMaterials(BUnigeomMaterialArray& materials)
{
   materials.resize(0);
   
   const int chunkIndex = mpECFFileData->findChunkByID((uint64)BUGXGeom::cECFMaterialChunkID);
   if (cInvalidIndex == chunkIndex)
   {
      gConsoleOutput.error("BUGXGeomData::getMaterials: Missing material chunk\n");
      return false;
   }
   
   const void* pChunkData = mpECFFileData->getChunkDataPtr(chunkIndex);
   const uint chunkDataLen = mpECFFileData->getChunkDataLen(chunkIndex);
   if (!pChunkData)
   {
      gConsoleOutput.error("BUGXGeomData::getMaterials: Material data chunk already discarded!\n");
      return false;
   }

   BBinaryDataTree::BPackedDocumentReader docReader;
   if (!docReader.set(BConstDataBuffer(pChunkData, chunkDataLen)))
   {
      gConsoleOutput.error("BUGXGeomData::getMaterials: BBinaryDataTree::BPackedDocumentReader failed reading UGX material data!");
      return false;
   }

   BBinaryDataTree::BPackedDocumentReader::BNode rootNode(docReader.getRoot());

   materials.reserve(rootNode.getNumChildren());

   for (uint childIndex = 0; childIndex < rootNode.getNumChildren(); childIndex++)
   {
      BBinaryDataTree::BPackedDocumentReader::BNode childNode(rootNode.getChild(childIndex));

      if (childNode.compareName("Material") != 0)
         continue;

      Unigeom::BMaterial& material = materials.grow();
      if (!material.read(childNode))
      {
         gConsoleOutput.error("BUGXGeomData::getMaterials: Failed reading UGX material!\n");
         return false;
      }
   }
   
   // Shouldn't ever need to read the material chunk again.
   mpECFFileData->discardChunkData(chunkIndex);

   return true;
}

//============================================================================
// BUGXGeomData::getAABBTree
//============================================================================
bool BUGXGeomData::getAABBTree(BAABBTree& tree)
{
   tree.clear();
   
   const int chunkIndex = mpECFFileData->findChunkByID((uint64)BUGXGeom::cECFTreeChunkID);
   if (cInvalidIndex == chunkIndex)
      return false;

   const void* pChunkData = mpECFFileData->getChunkDataPtr(chunkIndex);
   const uint chunkDataLen = mpECFFileData->getChunkDataLen(chunkIndex);
   if (!pChunkData)
   {
      gConsoleOutput.error("BUGXGeomData::getAABBTree: AABB tree data chunk already discarded!\n");
      return false;
   }   
   
   BByteStream byteStream(pChunkData, chunkDataLen);
   
   byteStream >> tree;
   
   if (!tree.numNodes())     
   {
      gConsoleOutput.error("BUGXGeomData::getAABBTree: Invalid AABB tree data chunk!\n");
      return false;
   }
   
   mpECFFileData->discardChunkData(chunkIndex);

   return true;
}
















