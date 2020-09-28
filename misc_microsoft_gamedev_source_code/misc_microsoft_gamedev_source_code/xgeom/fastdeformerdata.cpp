//============================================================================
//
//  fastdeformerdata.cpp
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#include "xgeom.h"

#include "fastdeformerdata.h"
#include "fastdeformer.h"
#include "math\generalmatrix.h"

#ifndef BUILD_FINAL
   #define DEBUG_CHECKS
#endif   

//==============================================================================
namespace BFastMeshDeformer  {
//==============================================================================

   //==============================================================================
   // BDataBuilder::BDataBuilder
   //==============================================================================
   BDataBuilder::BDataBuilder() :
      mpMesh(NULL),
      mNumRigidVerts(0),
      mNumBones(0),
      mSupported(false),
      mFlatMesh(false)
   {

   }

   //==============================================================================
   // BDataBuilder::~BDataBuilder
   //==============================================================================
   BDataBuilder::~BDataBuilder()
   {
   }

   //==============================================================================
   // BDataBuilder::clear
   //==============================================================================
   void BDataBuilder::clear(void)
   {
      mpMesh         = NULL;
      mNumRigidVerts = 0;
      mNumBones      = 0;
      mSupported     = false;
      mFlatMesh      = false;

      mData.clear();
   }

   //==============================================================================
   // BDataBuilder::convertGrannyVert
   //==============================================================================
   void BDataBuilder::convertGrannyVert(BGrannyBumpVert* pDst, int vertIndex)
   {
      BDEBUG_ASSERT(vertIndex < GrannyGetMeshVertexCount(mpMesh));
      
      if (mFlatMesh)
      {
         const BGrannyFlatVert* pSrc = &reinterpret_cast<const BGrannyFlatVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
         
         pDst->Position[0] = pSrc->Position[0];
         pDst->Position[1] = pSrc->Position[1];
         pDst->Position[2] = pSrc->Position[2];
         
         pDst->Normal[0] = pSrc->Normal[0];
         pDst->Normal[1] = pSrc->Normal[1];
         pDst->Normal[2] = pSrc->Normal[2];
         
         pDst->Tangent[0] = 0.0f;
         pDst->Tangent[1] = 0.0f;
         pDst->Tangent[2] = 0.0f;
       
         for (int i = 0; i < 4; i++)
         {
            pDst->BoneWeights[i] = pSrc->BoneWeights[i];
            pDst->BoneIndices[i] = pSrc->BoneIndices[i];
         }
         
         pDst->UV[0] = pSrc->UV[0];
         pDst->UV[1] = pSrc->UV[1];
      }
      else
      {
         const BGrannyBumpVert* pSrc = &reinterpret_cast<const BGrannyBumpVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
         memcpy(pDst, pSrc, sizeof(BGrannyBumpVert));
      }
   }

   //==============================================================================
   // BDataBuilder::isSupported
   //==============================================================================
   bool BDataBuilder::isSupported(granny_mesh* pMesh) 
   {
      BASSERT(pMesh);

      const int vertexCount = GrannyGetMeshVertexCount(pMesh);
      if (vertexCount > USHRT_MAX)
         return false;

      // Get a type definition for the vertices of the mesh
      granny_data_type_definition *VertexType = GrannyGetMeshVertexType(pMesh);

      return GrannyDataTypesAreEqual(GrannyPWNGT34332VertexType, VertexType) || 
             GrannyDataTypesAreEqual(GrannyPWNT3432VertexType, VertexType);
   }

   //==============================================================================
   // BDataBuilder::fixGrannyIndices
   //==============================================================================
   void BDataBuilder::fixGrannyIndices(const BSortVertArray& sortVerts)
   {
      const uint vertexSize = mFlatMesh ? sizeof(BGrannyFlatVert) : sizeof(BGrannyBumpVert);
      const uint vertexCount = GrannyGetMeshVertexCount(mpMesh);

      // Copy the source vertices to a temporary buffer.
      // It's possible to do an in-place reorder (see Knuth Sorting and Searching) but this is a lot less tricky.
      BByteArray tempVerts(vertexSize * vertexCount);
      
      // Create a table which indicates where each source vertex has been placed in the new VB.
      BDynamicArray<uint> srcToDstRemap(vertexCount);

      uchar* pVerts = reinterpret_cast<uchar*>(GrannyGetMeshVertices(mpMesh));
      if (vertexCount)
         memcpy(&tempVerts[0], pVerts, vertexSize * vertexCount);
      
      uchar* pDstVert = pVerts;   
      for (uint i = 0; i < vertexCount; i++, pDstVert += vertexSize)
      {
         const uint srcVertIndex = sortVerts[i].mVertIndex;

         memcpy(pDstVert, &tempVerts[srcVertIndex * vertexSize], vertexSize);

         srcToDstRemap[srcVertIndex] = i;
      }

      // Remap the indices
      const int bytesPerIndex = GrannyGetMeshBytesPerIndex(mpMesh);
      const uint numIndices = GrannyGetMeshIndexCount(mpMesh);

      if (sizeof(ushort) == bytesPerIndex)
      {
         ushort* pIndices = reinterpret_cast<ushort*>(GrannyGetMeshIndices(mpMesh));

         for (uint i = 0; i < numIndices; i++)
            pIndices[i] = static_cast<ushort>(srcToDstRemap[pIndices[i]]);
      }
      else 
      {
         BASSERT(sizeof(uint32) == bytesPerIndex);
         uint32* pIndices = reinterpret_cast<uint32*>(GrannyGetMeshIndices(mpMesh));

         for (uint i = 0; i < numIndices; i++)
            pIndices[i] = srcToDstRemap[pIndices[i]];
      }
   }
   
   //==============================================================================
   // BDataBuilder::createSortVerts
   //==============================================================================
   void BDataBuilder::createSortVerts(BSortVertArray& sortVerts)
   {
      const int vertexSize = mFlatMesh ? sizeof(BGrannyFlatVert) : sizeof(BGrannyBumpVert);
      vertexSize;
      BASSERT(vertexSize == GrannyGetTotalObjectSize(GrannyGetMeshVertexType(mpMesh)));
      
      const uint vertexCount = GrannyGetMeshVertexCount(mpMesh);
      BASSERT(vertexCount <= USHRT_MAX);
      
      sortVerts.resize(0);
      sortVerts.reserve(vertexCount);

      BDynamicArray<uint> rigidBoneHist(256);

      uint numRigidVerts = 0;
      uint numRigidBones = 0;

      mNumBones = 0;

      for (ushort vertIndex = 0; vertIndex < vertexCount; vertIndex++)
      {
         BGrannyBumpVert vert;
         convertGrannyVert(&vert, vertIndex);
         
         BASSERT(vert.BoneWeights[0] >= vert.BoneWeights[1]);
         BASSERT(vert.BoneWeights[1] >= vert.BoneWeights[2]);
         BASSERT(vert.BoneWeights[2] >= vert.BoneWeights[3]);

         int numBones;
         for (numBones = 0; numBones < 4; numBones++)
            if (vert.BoneWeights[numBones] == 0)
               break;

         BASSERT(numBones > 0);

         if (1 == numBones)
         {
            numRigidVerts++;

            const uint boneIndex = vert.BoneIndices[0];

            BDEBUG_ASSERT(boneIndex < rigidBoneHist.size());
            rigidBoneHist[boneIndex]++;
            numRigidBones = Math::Max(boneIndex + 1, numRigidBones);
         }
         else if (numBones > 2)
         {
            const float total = (vert.BoneWeights[0] / 255.0f) + (vert.BoneWeights[1] / 255.0f);
            vert.BoneWeights[0] = static_cast<granny_uint8>(vert.BoneWeights[0] / total);
            vert.BoneWeights[1] = static_cast<granny_uint8>(vert.BoneWeights[1] / total);

            const int iTotal = vert.BoneWeights[0] + vert.BoneWeights[1];
            int weight0 = vert.BoneWeights[0] - (iTotal - 255);
            if (weight0 < 0) weight0 = 0; else if (weight0 > 255) weight0 = 255;
            vert.BoneWeights[0] = static_cast<granny_uint8>(weight0);

            BASSERT((vert.BoneWeights[0] + vert.BoneWeights[1]) == 255);

            vert.BoneWeights[2] = 0;
            vert.BoneWeights[3] = 0;
            vert.BoneIndices[2] = 0;
            vert.BoneIndices[3] = 0;

            numBones = 2;
         }

         sortVerts.pushBack(
            BSortVert(vert, static_cast<ushort>(vertIndex), static_cast<uchar>(vert.BoneIndices[0]), static_cast<uchar>(numBones))
            );

         for (int i = 0; i < numBones; i++)
            mNumBones = Math::Max<int>(mNumBones, vert.BoneIndices[i] + 1);
      }

      std::sort(sortVerts.begin(), sortVerts.end());

      // Now make sure incomplete packets are moved to the end of the rigid vert list
      // They will be processed by the blend loop (not the packet loop)
      BSortVertArray savedRigidVerts;

      uint srcSortVertIndex = 0;
      uint dstSortVertIndex = 0;

      for (uint boneIndex = 0; boneIndex < numRigidBones; boneIndex++)
      {
         const uint numVerts = rigidBoneHist[boneIndex];

         // Kills incomplete packets by forcing them into the blended array.
         const uint numVertsToSave = numVerts & 3;

         for (uint i = 0; i < numVerts - numVertsToSave; i++)
         {
            sortVerts[dstSortVertIndex] = sortVerts[srcSortVertIndex];
            srcSortVertIndex++;
            dstSortVertIndex++;

            mNumRigidVerts++;
         }

         for (uint i = 0; i < numVertsToSave; i++)
         {
            savedRigidVerts.pushBack(sortVerts[srcSortVertIndex]);
            srcSortVertIndex++;
         }
      }

      for (uint i = 0; i < savedRigidVerts.size(); i++)
      {
         sortVerts[dstSortVertIndex] = savedRigidVerts[i];
         dstSortVertIndex++;
      }

      BDEBUG_ASSERT(dstSortVertIndex == numRigidVerts);

   #ifdef _DEBUG
      {
         BByteArray vertPresentFlag(vertexCount);

         BASSERT(sortVerts.size() == vertexCount);
         for (uint i = 0; i < sortVerts.size(); i++)
            vertPresentFlag[sortVerts[i].mVertIndex]++;

         for (uint i = 0; i < vertexCount; i++)
         {
            BASSERT(1 == vertPresentFlag[i]);
         }
      }   
   #endif // _DEBUG   
   
      BASSERT((mNumRigidVerts & 3) == 0);
   }
   
   //==============================================================================
   // BDataBuilder::compressBlendVerts
   //==============================================================================
   void BDataBuilder::compressBlendVerts(BDataHeader& header, const BSortVertArray& sortVerts, uint numBlendVerts, uint blockDataSize, uint packetDataSize)
   {
      BBlendVert* pDst = reinterpret_cast<BBlendVert*>(reinterpret_cast<uchar*>(&header) + sizeof(BDataHeader) + blockDataSize + packetDataSize);
            
      for (uint blendVertIndex = 0; blendVertIndex < numBlendVerts; blendVertIndex++)
      {
         const BGrannyBumpVert* pSrc = &sortVerts[mNumRigidVerts + blendVertIndex].mVert; 
         pDst->Position[0] = pSrc->Position[0];
         pDst->Position[1] = pSrc->Position[1];
         pDst->Position[2] = pSrc->Position[2];
                           
         for (int i = 0; i < 2; i++)
         {
            pDst->BoneWeights[i] = pSrc->BoneWeights[i];
            pDst->BoneIndices[i] = pSrc->BoneIndices[i];
         }
         
         pDst->Normal = compressNormal(pSrc->Normal);
         pDst->Tangent = compressNormal(pSrc->Tangent);
                  
         pDst->UV[0] = pSrc->UV[0];
         pDst->UV[1] = pSrc->UV[1];
                  
         pSrc++;
         pDst++;
      }
   }

   //==============================================================================
   // BDataBuilder::createDataBlock
   //==============================================================================
   void BDataBuilder::createDataBlock(const BBlockArray& blocks, BPacketVertArray& packetVerts, const BSortVertArray& sortVerts)
   {
      const uint numBlendVerts = GrannyGetMeshVertexCount(mpMesh) - mNumRigidVerts;
      
      const uint blockDataSize = Utils::AlignUpValue(blocks.size() * sizeof(blocks[0]), 16);
      const uint packetDataSize = Utils::AlignUpValue(packetVerts.size() * sizeof(packetVerts[0]), 16);
      const uint blendVertDataSize = Utils::AlignUpValue(numBlendVerts * sizeof(BBlendVert), 16);
      
      BASSERT((sizeof(BDataHeader) % 16) == 0);
      
      const uint totalSize = sizeof(BDataHeader) + blockDataSize + packetDataSize + blendVertDataSize;
      
      BASSERT((totalSize % 16) == 0);
         
      mData.resize(totalSize);
      memset(&mData[0], 0, totalSize);
      
      BDataHeader& header = *reinterpret_cast<BDataHeader*>(&mData[0]);
      
      header.mNumVerts           = GrannyGetMeshVertexCount(mpMesh);
      
      header.mNumBlocks          = blocks.size();
      header.mBlockDataSize      = blockDataSize;

      header.mNumPackets         = packetVerts.size();
      header.mPacketDataSize     = packetDataSize;

      header.mNumBlendVerts      = numBlendVerts;
      header.mBlendVertDataSize  = blendVertDataSize;

      header.mNumRigidVerts      = mNumRigidVerts;
      header.mNumBones           = mNumBones;
                  
      const DWORD dataSize = totalSize - sizeof(DWORD) * BDataHeader::NumPrefixDWORDs;
      header.mDataSize = dataSize;
      
      if (mFlatMesh)
         header.mFlags = static_cast<DWORD>(BDataHeader::cAllFlat);
            
      BBlock* pBlocks = reinterpret_cast<BBlock*>(&mData[0] + sizeof(BDataHeader));
      BPacketVert* pPackets = reinterpret_cast<BPacketVert*>(&mData[0] + sizeof(BDataHeader) + blockDataSize);

      if (!blocks.empty())
         Utils::Copy(blocks.begin(), blocks.end(), pBlocks);
      
      if (!packetVerts.empty())
         Utils::Copy(packetVerts.begin(), packetVerts.end(), pPackets);
                     
      if (numBlendVerts)
         compressBlendVerts(header, sortVerts, numBlendVerts, blockDataSize, packetDataSize);

      header.mDataVersion = FAST_DEFORMER_DATA_VERSION;
      
#ifdef XBOX
      header.endianSwap();
#endif      
      
      DWORD crc = 0;
      crc = calcCRC32SmallTable(reinterpret_cast<const BYTE*>(&header) + sizeof(DWORD) * BDataHeader::NumPrefixDWORDs, dataSize, false);

#ifdef XBOX
      header.endianSwap(true);
#endif
      
      header.mDataCRC = crc;

#ifdef XBOX
      header.endianSwap(true);
#endif      
   }
   
   //==============================================================================
   // BDataBuilder::BDataBuilder::compressNormal
   //==============================================================================
   DWORD BDataBuilder::compressNormal(const float* pNormal)
   {
      DWORD t = 0xFF000000;
      t |= static_cast<DWORD>(Math::Clamp(Math::FloatToIntRound(127.0f * (pNormal[0] * .5f + .5f)), 0, 127));
      t |= static_cast<DWORD>(Math::Clamp(Math::FloatToIntRound(127.0f * (pNormal[1] * .5f + .5f)), 0, 127)) << 8;
      t |= static_cast<DWORD>(Math::Clamp(Math::FloatToIntRound(127.0f * (pNormal[2] * .5f + .5f)), 0, 127)) << 16;
      return t;
   }

   //==============================================================================
   // BDataBuilder::init
   //==============================================================================
   bool BDataBuilder::init(granny_mesh* pMesh)
   {
      clear();

      mSupported = isSupported(pMesh);
      if (!mSupported)
         return false;

      mpMesh = pMesh;
      
      mFlatMesh = GrannyDataTypesAreEqual(GrannyPWNT3432VertexType, GrannyGetMeshVertexType(mpMesh));
                  
      BBlockArray blocks;
      BPacketVertArray packetVerts;
      
      BSortVertArray sortVerts;
      createSortVerts(sortVerts);
      fixGrannyIndices(sortVerts);

      const uint vertexCount = GrannyGetMeshVertexCount(mpMesh);
      vertexCount;
      BASSERT(vertexCount <= USHRT_MAX);
      
      // Create the packets
      BPacketArray packets;
      
      uint curVertIndex = 0;
      while (curVertIndex < mNumRigidVerts)
      {
         const uint boneIndex = sortVerts[curVertIndex].mBoneIndex;

         //for (uint i = curVertIndex + 1; i < sortVerts.size(); i++)
         uint i;
         for (i = curVertIndex + 1; i < mNumRigidVerts; i++)
         {
            if (boneIndex != sortVerts[i].mBoneIndex)
               break;
         }

         const uint numVerts = i - curVertIndex;
         const uint lastVertIndex = i - 1;

         const uint numPackets = (numVerts + 3) >> 2;
         //const uint numPackets = numVerts >> 2;

         BASSERT((curVertIndex + (numPackets * 4)) <= vertexCount);

         blocks.pushBack(BBlock(static_cast<ushort>(numPackets), static_cast<ushort>(curVertIndex), static_cast<uchar>(boneIndex)));

         uint curPacketIndex = packets.size();
         uint curPacketVerts = 0;
         packets.resize(packets.size() + numPackets);

         for (uint i = curVertIndex; i <= lastVertIndex; i++)
         {
            packets[curPacketIndex].mVertIndex[curPacketVerts] = static_cast<ushort>(i);

            curPacketVerts++;
            if (curPacketVerts == 4)
            {
               curPacketVerts = 0;
               curPacketIndex++;
            }
         }

         if (curPacketVerts > 0)
         {
            for (uint i = curPacketVerts; i < 4; i++)
               packets[curPacketIndex].mVertIndex[i] = packets[curPacketIndex].mVertIndex[curPacketVerts - 1];
         }

         curVertIndex = lastVertIndex + 1;
      }

      BASSERT(curVertIndex == mNumRigidVerts);

      // Initialize the rigid packet vertex data
      packetVerts.resize(packets.size());
      if (packets.size())
      {
         memset(&packetVerts[0], 0, packets.size() * sizeof(packetVerts[0]));
      }

      for (uint packetIndex = 0; packetIndex < packets.size(); packetIndex++)
      {
         bool allFlat = true;
         
         for (int i = 0; i < 4; i++)
         {
            const int vertIndex = packets[packetIndex].mVertIndex[i];
            const BGrannyBumpVert* pVert = &sortVerts[vertIndex].mVert; 

            SetVecComp(packetVerts[packetIndex].x, i, pVert->Position[0]);
            SetVecComp(packetVerts[packetIndex].y, i, pVert->Position[1]);
            SetVecComp(packetVerts[packetIndex].z, i, pVert->Position[2]);

            DWORD* pN = reinterpret_cast<DWORD*>(&packetVerts[packetIndex].pn);
            DWORD* pT = reinterpret_cast<DWORD*>(&packetVerts[packetIndex].pt);

            pN[i] = compressNormal(pVert->Normal);
            pT[i] = compressNormal(pVert->Tangent);
            
            if ((pVert->Tangent[0] != 0.0f) || (pVert->Tangent[1] != 0.0f) || (pVert->Tangent[2] != 0.0f))
               allFlat = false;

            switch (i)
            {
            case 0:
               SetVecComp(packetVerts[packetIndex].u, 0, pVert->UV[0]);
               SetVecComp(packetVerts[packetIndex].u, 1, pVert->UV[1]);
               break;
            case 1:
               SetVecComp(packetVerts[packetIndex].u, 2, pVert->UV[0]);
               SetVecComp(packetVerts[packetIndex].u, 3, pVert->UV[1]);
               break;
            case 2:
               SetVecComp(packetVerts[packetIndex].v, 0, pVert->UV[0]);
               SetVecComp(packetVerts[packetIndex].v, 1, pVert->UV[1]);
               break;
            case 3:
               SetVecComp(packetVerts[packetIndex].v, 2, pVert->UV[0]);
               SetVecComp(packetVerts[packetIndex].v, 3, pVert->UV[1]);
               break;
            }
         }
                  
         if (allFlat)
         {
            uchar* pT = reinterpret_cast<uchar*>(&packetVerts[packetIndex].pt) + 3;
            // Turn off the MSB of the first DWORD of the packet's compressed tangent if the packet is all-flat.
            *pT &= 0x7F;
         }
      }
      
      createDataBlock(blocks, packetVerts, sortVerts);
      
#ifndef BUILD_FINAL
      debugCheck(sortVerts);
#endif      
      
      return true;
   }
   
   //============================================================================
   // BDataBuilder::debugCheck
   //============================================================================
   void BDataBuilder::debugCheck(const BSortVertArray& sortVerts)
   {
      sortVerts;
      
#ifdef DEBUG_CHECKS    
      uchar* pRawData = new uchar[getDataLen() + 15];
      uchar* pData = Utils::AlignUp(pRawData, 16);
      memcpy(pData, getData(), getDataLen());
            
      BDeformer deformer;
      
      if (!deformer.crcCheck(pData, getDataLen()))
      {
         BFAIL("BDataBuilder::debugCheck: deformer.crcCheck() failed!");
      }

#ifdef XBOX
      deformer.endianSwap(pData, getDataLen());
#endif                     
      
      const bool success = deformer.setData(pData, getDataLen());
      if (!success)
      {
         BFAIL("BDataBuilder::debugCheck: deformer.setData() failed!");
      }
      
      const BDataHeader* pHeader = reinterpret_cast<const BDataHeader*>(pData);
      
      BASSERT(pHeader->mNumVerts == sortVerts.size());
      
      for (uint vertIndex = 0; vertIndex < sortVerts.size(); vertIndex++)
      {
         BGrannyBumpVert bumpVert;
         
         deformer.getVertex(bumpVert, vertIndex);

         if (mFlatMesh)         
         {
            const BGrannyFlatVert* pSrcVert = &reinterpret_cast<const BGrannyFlatVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
            pSrcVert;
            
            BASSERT(pSrcVert->Position[0] == bumpVert.Position[0]);
            BASSERT(pSrcVert->Position[1] == bumpVert.Position[1]);
            BASSERT(pSrcVert->Position[2] == bumpVert.Position[2]);
            
            BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
            BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
            BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);
            
            if (bumpVert.BoneWeights[0] > 0)            
            {
               BASSERT(pSrcVert->BoneIndices[0] == bumpVert.BoneIndices[0]);
            }
            
            if (bumpVert.BoneWeights[1] > 0)            
            {
               BASSERT(pSrcVert->BoneIndices[1] == bumpVert.BoneIndices[1]);
            }
            
            BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
            BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
         }
         else
         {
            const BGrannyBumpVert* pSrcVert = &reinterpret_cast<const BGrannyBumpVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
            pSrcVert;
            
            BASSERT(pSrcVert->Position[0] == bumpVert.Position[0]);
            BASSERT(pSrcVert->Position[1] == bumpVert.Position[1]);
            BASSERT(pSrcVert->Position[2] == bumpVert.Position[2]);

            BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
            BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
            BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);
            
            BASSERT(fabs(pSrcVert->Tangent[0] - bumpVert.Tangent[0]) < .025f);
            BASSERT(fabs(pSrcVert->Tangent[1] - bumpVert.Tangent[1]) < .025f);
            BASSERT(fabs(pSrcVert->Tangent[2] - bumpVert.Tangent[2]) < .025f);

            if (bumpVert.BoneWeights[0] > 0)            
            {
               BASSERT(pSrcVert->BoneIndices[0] == bumpVert.BoneIndices[0]);
            }
            
            if (bumpVert.BoneWeights[1] > 0)            
            {
               BASSERT(pSrcVert->BoneIndices[1] == bumpVert.BoneIndices[1]);
            }

            BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
            BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
         }
      }
      
      //for (int alignFlag = 0; alignFlag < 2; alignFlag++)
      int alignFlag = 1;
      {
         // Pos only deform      
         {
            uchar* pRawTemp = new uchar[pHeader->mNumVerts * sizeof(p3_vertex) + 48];
            uchar* pTemp = Utils::AlignUp(pRawTemp, 16);
            if (!alignFlag)
               pTemp++;
            
            memset(pTemp, 0x5A, pHeader->mNumVerts * sizeof(p3_vertex) + 16);      
            
            bool finalBuild = false;
      #ifdef BUILD_FINAL
            finalBuild = true;
      #endif      
            
            granny_int32x indices[128];
            for (int i = 0; i < 128; i++)
               indices[i] = i;
            
            BMatrix44 __declspec(align(16)) matrices[128];
            for (int i = 0; i < 128; i++)
               matrices[i].setIdentity();
               
            deformer.deform(indices, (float*)matrices, pTemp, cPosOnlyVertFormat, false, false, finalBuild);
            
            for (int i = 0; i < 16; i++)
            {  
               BASSERT((pTemp + pHeader->mNumVerts * sizeof(p3_vertex))[i] == 0x5A);
            }
            
            for (uint vertIndex = 0; vertIndex < pHeader->mNumVerts; vertIndex++)
            {
               p3_vertex* pVert = &((p3_vertex*)pTemp)[vertIndex];
               pVert;

               if (mFlatMesh)         
               {
                  const BGrannyFlatVert* pSrcVert = &reinterpret_cast<const BGrannyFlatVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;
                  
                  BASSERT(pVert->Position[0] == pSrcVert->Position[0]);
                  BASSERT(pVert->Position[1] == pSrcVert->Position[1]);
                  BASSERT(pVert->Position[2] == pSrcVert->Position[2]);
               }
               else
               {                     
                  const BGrannyBumpVert* pSrcVert = &reinterpret_cast<const BGrannyBumpVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;
                  
                  BASSERT(pVert->Position[0] == pSrcVert->Position[0]);
                  BASSERT(pVert->Position[1] == pSrcVert->Position[1]);
                  BASSERT(pVert->Position[2] == pSrcVert->Position[2]);
               }
            }
            
            delete [] pRawTemp;
         }
         

         // Bump deform
         {
            uchar* pRawTemp = new uchar[pHeader->mNumVerts * sizeof(pngt3332_vertex) + 48];
            uchar* pTemp = Utils::AlignUp(pRawTemp, 16);
            if (!alignFlag)
               pTemp++;
            
            memset(pTemp, 0x5A, pHeader->mNumVerts * sizeof(pngt3332_vertex) + 16);      

            bool finalBuild = false;
      #ifdef BUILD_FINAL
            finalBuild = true;
      #endif      

            granny_int32x indices[128];
            for (int i = 0; i < 128; i++)
               indices[i] = i;

            BMatrix44 __declspec(align(16)) matrices[128];
            for (int i = 0; i < 128; i++)
               matrices[i].setIdentity();

            deformer.deform(indices, (float*)matrices, pTemp, cBumpVertFormat, false, false, finalBuild);

            for (int i = 0; i < 16; i++)
            {  
               BASSERT((pTemp + pHeader->mNumVerts * sizeof(pngt3332_vertex))[i] == 0x5A);
            }

            for (uint vertIndex = 0; vertIndex < pHeader->mNumVerts; vertIndex++)
            {
               const pngt3332_vertex& bumpVert = ((const pngt3332_vertex*)pTemp)[vertIndex];
               bumpVert;
               
               if (mFlatMesh)         
               {
                  const BGrannyFlatVert* pSrcVert = &reinterpret_cast<const BGrannyFlatVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;

                  BASSERT(bumpVert.Position[0] == pSrcVert->Position[0]);
                  BASSERT(bumpVert.Position[1] == pSrcVert->Position[1]);
                  BASSERT(bumpVert.Position[2] == pSrcVert->Position[2]);
                  
                  BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);
                                 
                  BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
                  BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
               }
               else
               {                     
                  const BGrannyBumpVert* pSrcVert = &reinterpret_cast<const BGrannyBumpVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;

                  BASSERT(bumpVert.Position[0] == pSrcVert->Position[0]);
                  BASSERT(bumpVert.Position[1] == pSrcVert->Position[1]);
                  BASSERT(bumpVert.Position[2] == pSrcVert->Position[2]);

                  BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);

                  BASSERT(fabs(pSrcVert->Tangent[0] - bumpVert.Tangent[0]) < .025f);
                  BASSERT(fabs(pSrcVert->Tangent[1] - bumpVert.Tangent[1]) < .025f);
                  BASSERT(fabs(pSrcVert->Tangent[2] - bumpVert.Tangent[2]) < .025f);
                  
                  BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
                  BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
               }
            }

            delete [] pRawTemp;
         }


         // Flat deform
         {
            uchar* pRawTemp = new uchar[pHeader->mNumVerts * sizeof(pnt332_vertex) + 48];
            uchar* pTemp = Utils::AlignUp(pRawTemp, 16);
            if (!alignFlag)
               pTemp++;
               
            memset(pTemp, 0x5A, pHeader->mNumVerts * sizeof(pnt332_vertex) + 16);      

            bool finalBuild = false;
      #ifdef BUILD_FINAL
            finalBuild = true;
      #endif      

            granny_int32x indices[128];
            for (int i = 0; i < 128; i++)
               indices[i] = i;

            BMatrix44 __declspec(align(16)) matrices[128];
            for (int i = 0; i < 128; i++)
               matrices[i].setIdentity();

            deformer.deform(indices, (float*)matrices, pTemp, cFlatVertFormat, false, false, finalBuild);

            for (int i = 0; i < 16; i++)
            {  
               BASSERT((pTemp + pHeader->mNumVerts * sizeof(pnt332_vertex))[i] == 0x5A);
            }

            for (uint vertIndex = 0; vertIndex < pHeader->mNumVerts; vertIndex++)
            {
               const pnt332_vertex& bumpVert = ((const pnt332_vertex*)pTemp)[vertIndex];
               bumpVert;

               if (mFlatMesh)         
               {
                  const BGrannyFlatVert* pSrcVert = &reinterpret_cast<const BGrannyFlatVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;

                  BASSERT(bumpVert.Position[0] == pSrcVert->Position[0]);
                  BASSERT(bumpVert.Position[1] == pSrcVert->Position[1]);
                  BASSERT(bumpVert.Position[2] == pSrcVert->Position[2]);

                  BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);
                                 
                  BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
                  BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
               }
               else
               {                     
                  const BGrannyBumpVert* pSrcVert = &reinterpret_cast<const BGrannyBumpVert*>(GrannyGetMeshVertices(mpMesh))[vertIndex];
                  pSrcVert;

                  BASSERT(bumpVert.Position[0] == pSrcVert->Position[0]);
                  BASSERT(bumpVert.Position[1] == pSrcVert->Position[1]);
                  BASSERT(bumpVert.Position[2] == pSrcVert->Position[2]);

                  BASSERT(fabs(pSrcVert->Normal[0] - bumpVert.Normal[0]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[1] - bumpVert.Normal[1]) < .025f);
                  BASSERT(fabs(pSrcVert->Normal[2] - bumpVert.Normal[2]) < .025f);
                                 
                  BASSERT(pSrcVert->UV[0] == bumpVert.UV[0]);
                  BASSERT(pSrcVert->UV[1] == bumpVert.UV[1]);
               }
            }

            delete [] pRawTemp;
         }
         
         delete[] pRawData;
      }               
      
#endif  // DEBUG_CHECKS      
   }

//============================================================================
} // namespace BFastMeshDeformer
//============================================================================

