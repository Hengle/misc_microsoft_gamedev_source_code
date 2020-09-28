//============================================================================
//
//  fastdeformerc.cpp
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#include "xgeom.h"
#include "fastdeformer.h"

#ifdef __INTEL_COMPILER
   #pragma warning(disable:1572)       // warning #1572: floating-point equality and inequality comparisons are unreliable
   #pragma warning(disable:61)         // warning #61: integer operation result is out of range
   #pragma warning(disable:186)      // warning #186: pointless comparison of unsigned integer with zero
   #ifndef RESTRICT
      #define RESTRICT __restrict 
   #endif
#else
   #ifndef RESTRICT
      #define RESTRICT
   #endif  
#endif

namespace BFastMeshDeformer {
  
   static D3DXVECTOR3 unpackSingleNormal(DWORD n)
   {
      BASSERT((n & 0xFF) <= 0x7F);
      BASSERT(((n>>8) & 0xFF) <= 0x7F);
      BASSERT(((n>>16) & 0xFF) <= 0x7F);
      
      const float x = ((n      ) & 0xFF) * (2.0f / 127.0f) - 1.0f;
      const float y = ((n >>  8) & 0xFF) * (2.0f / 127.0f) - 1.0f;
      const float z = ((n >> 16) & 0xFF) * (2.0f / 127.0f) - 1.0f;
      
      return D3DXVECTOR3(x, y, z);
   }
      
   //==============================================================================
   // DeformerRigidToStackAligned
   //==============================================================================
   void DeformerRigidToStackAlignedC(const BDataHeader* pHeader, const BWorkQueueEntry& work)
   {
      pngt3332_vertex*           RESTRICT pDstVerts = reinterpret_cast<pngt3332_vertex*>(work.mpDestVertices);
      const D3DXMATRIX*          RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      const BPacketVert*         RESTRICT pPacketVert = pHeader->getPackets();
      const BBlock*              RESTRICT pBlock = pHeader->getBlocks();

      for (uint blockIndex = 0; blockIndex < pHeader->mNumBlocks; blockIndex++)
      {
         const int boneIndex = work.mpMatrixIndices[pBlock->mBoneIndex];
         const D3DXMATRIX* RESTRICT pMatrix = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[boneIndex]);

         pngt3332_vertex* RESTRICT pDstVert = pDstVerts + pBlock->mDstVert;
         
         for (uint packetIndex = 0; packetIndex < pBlock->mNumPackets; packetIndex++)
         {
            const float* RESTRICT pX = reinterpret_cast<const float*>(&pPacketVert->x);
            const float* RESTRICT pY = reinterpret_cast<const float*>(&pPacketVert->y);
            const float* RESTRICT pZ = reinterpret_cast<const float*>(&pPacketVert->z);
            const DWORD* RESTRICT pN = reinterpret_cast<const DWORD*>(&pPacketVert->pn);
            const DWORD* RESTRICT pT = reinterpret_cast<const DWORD*>(&pPacketVert->pt);
            const float* RESTRICT pU = reinterpret_cast<const float*>(&pPacketVert->u);
            const float* RESTRICT pV = reinterpret_cast<const float*>(&pPacketVert->v);

            const D3DXVECTOR3 a(pX[0], pY[0], pZ[0]);
            const D3DXVECTOR3 b(pX[1], pY[1], pZ[1]);
            const D3DXVECTOR3 c(pX[2], pY[2], pZ[2]);
            const D3DXVECTOR3 d(pX[3], pY[3], pZ[3]);

            D3DXVECTOR4 ta, tb, tc, td;
            D3DXVec3Transform(&ta, &a, pMatrix);
            D3DXVec3Transform(&tb, &b, pMatrix);
            D3DXVec3Transform(&tc, &c, pMatrix);
            D3DXVec3Transform(&td, &d, pMatrix);

            pDstVert[0].Position[0] = ta.x;
            pDstVert[0].Position[1] = ta.y;
            pDstVert[0].Position[2] = ta.z;
            pDstVert[0].UV[0] = pU[0];
            pDstVert[0].UV[1] = pU[1];

            pDstVert[1].Position[0] = tb.x;
            pDstVert[1].Position[1] = tb.y;
            pDstVert[1].Position[2] = tb.z;
            pDstVert[1].UV[0] = pU[2];
            pDstVert[1].UV[1] = pU[3];

            pDstVert[2].Position[0] = tc.x;
            pDstVert[2].Position[1] = tc.y;
            pDstVert[2].Position[2] = tc.z;
            pDstVert[2].UV[0] = pV[0];
            pDstVert[2].UV[1] = pV[1];

            pDstVert[3].Position[0] = td.x;
            pDstVert[3].Position[1] = td.y;
            pDstVert[3].Position[2] = td.z;
            pDstVert[3].UV[0] = pV[2];
            pDstVert[3].UV[1] = pV[3];

            const D3DXVECTOR3 na(unpackSingleNormal(pN[0])), nb(unpackSingleNormal(pN[1])), nc(unpackSingleNormal(pN[2])), nd(unpackSingleNormal(pN[3]));
            const D3DXVECTOR3 ga(unpackSingleNormal(pT[0])), gb(unpackSingleNormal(pT[1])), gc(unpackSingleNormal(pT[2])), gd(unpackSingleNormal(pT[3]));

            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[0].Normal), &na, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[1].Normal), &nb, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[2].Normal), &nc, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[3].Normal), &nd, pMatrix);

            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[0].Tangent), &ga, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[1].Tangent), &gb, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[2].Tangent), &gc, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[3].Tangent), &gd, pMatrix);

            pPacketVert++;
            pDstVert += 4;
         }
         
         pBlock++;
      }
   }

   //==============================================================================
   // DeformerRigidToStackAlignedNoTanSSE
   //==============================================================================
   void DeformerRigidToStackAlignedNoTanC(const BDataHeader* pHeader, const BWorkQueueEntry& work)
   {
      pnt332_vertex*             RESTRICT pDstVerts = reinterpret_cast<pnt332_vertex*>(work.mpDestVertices);
      const D3DXMATRIX*          RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      const BPacketVert*         RESTRICT pPacketVert = pHeader->getPackets();
      const BBlock*              RESTRICT pBlock = pHeader->getBlocks();

      for (uint blockIndex = 0; blockIndex < pHeader->mNumBlocks; blockIndex++)
      {
         const int boneIndex = work.mpMatrixIndices[pBlock->mBoneIndex];
         const D3DXMATRIX* RESTRICT pMatrix = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[boneIndex]);
         
         pnt332_vertex* RESTRICT pDstVert = pDstVerts + pBlock->mDstVert;
        
         for (uint packetIndex = 0; packetIndex < pBlock->mNumPackets; packetIndex++)
         {
            const float* RESTRICT pX = reinterpret_cast<const float*>(&pPacketVert->x);
            const float* RESTRICT pY = reinterpret_cast<const float*>(&pPacketVert->y);
            const float* RESTRICT pZ = reinterpret_cast<const float*>(&pPacketVert->z);
            const DWORD* RESTRICT pN = reinterpret_cast<const DWORD*>(&pPacketVert->pn);
            
            const float* RESTRICT pU = reinterpret_cast<const float*>(&pPacketVert->u);
            const float* RESTRICT pV = reinterpret_cast<const float*>(&pPacketVert->v);
            
            const D3DXVECTOR3 a(pX[0], pY[0], pZ[0]);
            const D3DXVECTOR3 b(pX[1], pY[1], pZ[1]);
            const D3DXVECTOR3 c(pX[2], pY[2], pZ[2]);
            const D3DXVECTOR3 d(pX[3], pY[3], pZ[3]);

            D3DXVECTOR4 ta, tb, tc, td;
            D3DXVec3Transform(&ta, &a, pMatrix);
            D3DXVec3Transform(&tb, &b, pMatrix);
            D3DXVec3Transform(&tc, &c, pMatrix);
            D3DXVec3Transform(&td, &d, pMatrix);
                                              
            pDstVert[0].Position[0] = ta.x;
            pDstVert[0].Position[1] = ta.y;
            pDstVert[0].Position[2] = ta.z;
            pDstVert[0].UV[0] = pU[0];
            pDstVert[0].UV[1] = pU[1];

            pDstVert[1].Position[0] = tb.x;
            pDstVert[1].Position[1] = tb.y;
            pDstVert[1].Position[2] = tb.z;
            pDstVert[1].UV[0] = pU[2];
            pDstVert[1].UV[1] = pU[3];

            pDstVert[2].Position[0] = tc.x;
            pDstVert[2].Position[1] = tc.y;
            pDstVert[2].Position[2] = tc.z;
            pDstVert[2].UV[0] = pV[0];
            pDstVert[2].UV[1] = pV[1];

            pDstVert[3].Position[0] = td.x;
            pDstVert[3].Position[1] = td.y;
            pDstVert[3].Position[2] = td.z;
            pDstVert[3].UV[0] = pV[2];
            pDstVert[3].UV[1] = pV[3];
            
            const D3DXVECTOR3 na(unpackSingleNormal(pN[0])), nb(unpackSingleNormal(pN[1])), nc(unpackSingleNormal(pN[2])), nd(unpackSingleNormal(pN[3]));

            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[0].Normal), &na, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[1].Normal), &nb, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[2].Normal), &nc, pMatrix);
            D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert[3].Normal), &nd, pMatrix);

            pPacketVert++;
            pDstVert += 4;
         }
         
         pBlock++;
      }
   }

   //==============================================================================
   // DeformerRigidToStackAlignedPosOnlySSE
   //==============================================================================
   void DeformerRigidToStackAlignedPosOnlyC(const BDataHeader* pHeader, const BWorkQueueEntry& work)
   {
      p3_vertex*                 RESTRICT pDstVerts = reinterpret_cast<p3_vertex*>(work.mpDestVertices);
      const D3DXMATRIX*          RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      const BPacketVert*         RESTRICT pPacketVert = pHeader->getPackets();
      const BBlock*              RESTRICT pBlock = pHeader->getBlocks();

      for (uint blockIndex = 0; blockIndex < pHeader->mNumBlocks; blockIndex++)
      {
         const int boneIndex = work.mpMatrixIndices[pBlock->mBoneIndex];
         const D3DXMATRIX* RESTRICT pMatrix = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[boneIndex]);
         
         D3DXVECTOR3* RESTRICT pDstVert = reinterpret_cast<D3DXVECTOR3*>(pDstVerts + pBlock->mDstVert);

         for (uint packetIndex = 0; packetIndex < pBlock->mNumPackets; packetIndex++)
         {
            const float* RESTRICT pX = reinterpret_cast<const float*>(&pPacketVert->x);
            const float* RESTRICT pY = reinterpret_cast<const float*>(&pPacketVert->y);
            const float* RESTRICT pZ = reinterpret_cast<const float*>(&pPacketVert->z);
            
            const D3DXVECTOR3 a(pX[0], pY[0], pZ[0]);
            const D3DXVECTOR3 b(pX[1], pY[1], pZ[1]);
            const D3DXVECTOR3 c(pX[2], pY[2], pZ[2]);
            const D3DXVECTOR3 d(pX[3], pY[3], pZ[3]);

            D3DXVECTOR4 ta, tb, tc, td;
            D3DXVec3Transform(&ta, &a, pMatrix);
            D3DXVec3Transform(&tb, &b, pMatrix);
            D3DXVec3Transform(&tc, &c, pMatrix);
            D3DXVec3Transform(&td, &d, pMatrix);
                        
            pDstVert[0].x = ta.x;
            pDstVert[0].y = ta.y;
            pDstVert[0].z = ta.z;
                        
            pDstVert[1].x = tb.x;
            pDstVert[1].y = tb.y;
            pDstVert[1].z = tb.z;
            
            pDstVert[2].x = tc.x;
            pDstVert[2].y = tc.y;
            pDstVert[2].z = tc.z;
            
            pDstVert[3].x = td.x;
            pDstVert[3].y = td.y;
            pDstVert[3].z = td.z;
            
            pPacketVert++;
            pDstVert += 4;
         }

         pBlock++;
      }
   }

   //==============================================================================
   // DeformerBlendedSSE
   //==============================================================================
   void DeformerBlendedC(
      const BWorkQueueEntry& work,
      const BBlendVert* pSrcVertA,
      pngt3332_vertex* pDstVertA,
      const uint numBlendVerts)
   {
      const BBlendVert* RESTRICT pSrcVert = pSrcVertA;
      pngt3332_vertex* RESTRICT pDstVert = pDstVertA;
      const D3DXMATRIX* RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      for (uint i = 0; i < numBlendVerts; i++, pSrcVert++, pDstVert++)
      {
         const int bone0 = work.mpMatrixIndices[pSrcVert->BoneIndices[0]];
         const D3DXMATRIX* RESTRICT pBoneMatrix0 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone0]);

         const int bone1 = work.mpMatrixIndices[pSrcVert->BoneIndices[1]];
         const D3DXMATRIX* RESTRICT pBoneMatrix1 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone1]);

         const float weight1 = pSrcVert->BoneWeights[1] * (1.0f / 255.0f);

         const D3DXMATRIX matrix(*pBoneMatrix0 + (*pBoneMatrix1 - *pBoneMatrix0) * weight1);

         const D3DXVECTOR3* RESTRICT pSrcPos = reinterpret_cast<const D3DXVECTOR3*>(pSrcVert->Position);

         D3DXVECTOR4 result;
         D3DXVec3Transform(&result, pSrcPos, &matrix);

         pDstVert->Position[0] = result.x;
         pDstVert->Position[1] = result.y;
         pDstVert->Position[2] = result.z;

         const D3DXVECTOR3 srcNorm(unpackSingleNormal(pSrcVert->Normal));
         D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert->Normal), &srcNorm, &matrix);
         
         const D3DXVECTOR3 srcTan(unpackSingleNormal(pSrcVert->Tangent));
         D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert->Tangent), &srcTan, &matrix);

         pDstVert->UV[0] = pSrcVert->UV[0];
         pDstVert->UV[1] = pSrcVert->UV[1];
      }   
   }

   //==============================================================================
   // DeformerBlendedNoTanSSE
   //==============================================================================
   void DeformerBlendedNoTanC(
      const BWorkQueueEntry& work,
      const BBlendVert* pSrcVertA,
      pnt332_vertex* pDstVertA,
      const uint numBlendVerts)
   {
      const BBlendVert* RESTRICT pSrcVert = pSrcVertA;
      pnt332_vertex* RESTRICT pDstVert = pDstVertA;
      const D3DXMATRIX* RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      for (uint i = 0; i < numBlendVerts; i++, pSrcVert++, pDstVert++)
      {
         const int bone0 = work.mpMatrixIndices[pSrcVert->BoneIndices[0]];
         const D3DXMATRIX* RESTRICT pBoneMatrix0 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone0]);

         const int bone1 = work.mpMatrixIndices[pSrcVert->BoneIndices[1]];
         const D3DXMATRIX* RESTRICT pBoneMatrix1 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone1]);

         const float weight1 = pSrcVert->BoneWeights[1] * (1.0f / 255.0f);

         const D3DXMATRIX matrix(*pBoneMatrix0 + (*pBoneMatrix1 - *pBoneMatrix0) * weight1);

         const D3DXVECTOR3* RESTRICT pSrcPos = reinterpret_cast<const D3DXVECTOR3*>(pSrcVert->Position);
         
         D3DXVECTOR4 result;
         D3DXVec3Transform(&result, pSrcPos, &matrix);

         pDstVert->Position[0] = result.x;
         pDstVert->Position[1] = result.y;
         pDstVert->Position[2] = result.z;
         
         const D3DXVECTOR3 srcNorm(unpackSingleNormal(pSrcVert->Normal));
         D3DXVec3TransformNormal(reinterpret_cast<D3DXVECTOR3*>(pDstVert->Normal), &srcNorm, &matrix);
         
         pDstVert->UV[0] = pSrcVert->UV[0];
         pDstVert->UV[1] = pSrcVert->UV[1];
      }   
   }

   //==============================================================================
   // DeformerDeformBlendedPosOnlySSE
   //==============================================================================
   void DeformerBlendedPosOnlyC(
      const BWorkQueueEntry& work,
      const BBlendVert* pSrcVertA,
      p3_vertex* pDstVertA,
      const uint numBlendVerts)
   {
      const BBlendVert* RESTRICT pSrcVert = pSrcVertA;
      p3_vertex* RESTRICT pDstVert = pDstVertA;
      const D3DXMATRIX* RESTRICT pMatrices = reinterpret_cast<const D3DXMATRIX*>(work.mpMatrixBuffer4x4);

      for (uint i = 0; i < numBlendVerts; i++, pSrcVert++, pDstVert++)
      {
         const int bone0 = work.mpMatrixIndices[pSrcVert->BoneIndices[0]];
         const D3DXMATRIX* RESTRICT pBoneMatrix0 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone0]);
         
         const int bone1 = work.mpMatrixIndices[pSrcVert->BoneIndices[1]];
         const D3DXMATRIX* RESTRICT pBoneMatrix1 = reinterpret_cast<const D3DXMATRIX*>(&pMatrices[bone1]);
         
         const float weight1 = pSrcVert->BoneWeights[1] * (1.0f / 255.0f);
         
         const D3DXMATRIX matrix(*pBoneMatrix0 + (*pBoneMatrix1 - *pBoneMatrix0) * weight1);
                  
         const D3DXVECTOR3* RESTRICT pSrcPos = reinterpret_cast<const D3DXVECTOR3*>(pSrcVert->Position);
         
         D3DXVECTOR4 result;
         D3DXVec3Transform(&result, pSrcPos, &matrix);
                  
         pDstVert->Position[0] = result.x;
         pDstVert->Position[1] = result.y;
         pDstVert->Position[2] = result.z;
      }   
   }

   //==============================================================================
   // CopyMatricesSSE
   //==============================================================================
   void DeformerCopyMatricesC(
      const BDataHeader* pHeader,
      VecType* pDstN,
      const VecType* pSrcN,
      const granny_int32x* MatrixIndices)
   {
      VecType* RESTRICT pDst = pDstN;
      const VecType* RESTRICT pSrc = pSrcN;

      int numBonesLeft = pHeader->mNumBones;
      int boneIndex = 0;
      while (numBonesLeft >= 4)
      {
         const int b0 = MatrixIndices[boneIndex + 0] * 4;
         const int b1 = MatrixIndices[boneIndex + 1] * 4;
         const int b2 = MatrixIndices[boneIndex + 2] * 4;
         const int b3 = MatrixIndices[boneIndex + 3] * 4;

         pDst[0*4+0] = pSrc[b0 + 0];
         pDst[0*4+1] = pSrc[b0 + 1];
         pDst[0*4+2] = pSrc[b0 + 2];
         pDst[0*4+3] = pSrc[b0 + 3];

         pDst[1*4+0] = pSrc[b1 + 0];
         pDst[1*4+1] = pSrc[b1 + 1];
         pDst[1*4+2] = pSrc[b1 + 2];
         pDst[1*4+3] = pSrc[b1 + 3];

         pDst[2*4+0] = pSrc[b2 + 0];
         pDst[2*4+1] = pSrc[b2 + 1];
         pDst[2*4+2] = pSrc[b2 + 2];
         pDst[2*4+3] = pSrc[b2 + 3];

         pDst[3*4+0] = pSrc[b3 + 0];
         pDst[3*4+1] = pSrc[b3 + 1];
         pDst[3*4+2] = pSrc[b3 + 2];
         pDst[3*4+3] = pSrc[b3 + 3];

         pDst += 4*4;
         numBonesLeft -= 4;
         boneIndex += 4;
      }

      while (numBonesLeft)
      {
         pDst[0] = pSrc[MatrixIndices[boneIndex] * 4 + 0];
         pDst[1] = pSrc[MatrixIndices[boneIndex] * 4 + 1];
         pDst[2] = pSrc[MatrixIndices[boneIndex] * 4 + 2];
         pDst[3] = pSrc[MatrixIndices[boneIndex] * 4 + 3];      

         pDst += 4;

         numBonesLeft--;
         boneIndex++;
      }
   }
   
   //==============================================================================
   // DeformerGetBumpVertexC
   //==============================================================================
   void DeformerGetBumpVertexC(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyBumpVert& dst, uint vertIndex)
   {
      BASSERT(vertIndex < pHeader->mNumVerts);

      if (vertIndex < pHeader->mNumRigidVerts)
      {
         const uint packetIndex = vertIndex >> 2;
         const uint packetOfs = vertIndex & 3;

         const BPacketVert& src = pHeader->getPackets()[packetIndex];

         dst.Position[0] = GetVecComp(src.x, packetOfs);
         dst.Position[1] = GetVecComp(src.y, packetOfs);
         dst.Position[2] = GetVecComp(src.z, packetOfs);

         dst.BoneIndices[0] = pPacketBoneIndices[packetIndex];
         dst.BoneIndices[1] = 0;
         dst.BoneIndices[2] = 0;
         dst.BoneIndices[3] = 0;

         dst.BoneWeights[0] = 255;
         dst.BoneWeights[1] = 0;
         dst.BoneWeights[2] = 0;
         dst.BoneWeights[3] = 0;

         const D3DXVECTOR3 n(unpackSingleNormal(reinterpret_cast<const DWORD*>(&src.pn)[packetOfs]));
         const D3DXVECTOR3 t(unpackSingleNormal(reinterpret_cast<const DWORD*>(&src.pt)[packetOfs]));

         dst.Normal[0] = n.x;
         dst.Normal[1] = n.y;
         dst.Normal[2] = n.z;

         dst.Tangent[0] = t.x;
         dst.Tangent[1] = t.y;
         dst.Tangent[2] = t.z;

         if (packetOfs < 2)
         {
            dst.UV[0] = GetVecComp(src.u, packetOfs*2+0);
            dst.UV[1] = GetVecComp(src.u, packetOfs*2+1);
         }
         else
         {
            dst.UV[0] = GetVecComp(src.v, (packetOfs-2)*2+0);
            dst.UV[1] = GetVecComp(src.v, (packetOfs-2)*2+1);
         }
      }
      else
      {
         const BBlendVert& src = pHeader->getBlendVerts()[vertIndex - pHeader->mNumRigidVerts];

         dst.Position[0] = src.Position[0];
         dst.Position[1] = src.Position[1];
         dst.Position[2] = src.Position[2];

         dst.BoneWeights[0] = src.BoneWeights[0];
         dst.BoneWeights[1] = src.BoneWeights[1];
         dst.BoneWeights[2] = 0;
         dst.BoneWeights[3] = 0;
         dst.BoneIndices[0] = src.BoneIndices[0];
         dst.BoneIndices[1] = src.BoneIndices[1];
         dst.BoneIndices[2] = 0;
         dst.BoneIndices[3] = 0;

         const D3DXVECTOR3 n(unpackSingleNormal(src.Normal));

         dst.Normal[0] = n.x;
         dst.Normal[1] = n.y;
         dst.Normal[2] = n.z;

         const D3DXVECTOR3 t(unpackSingleNormal(src.Tangent));
         dst.Tangent[0] = t.x;
         dst.Tangent[1] = t.y;
         dst.Tangent[2] = t.z;

         dst.UV[0] = src.UV[0];
         dst.UV[1] = src.UV[1];
      }
   }

   //==============================================================================
   // DeformerGetFlatVertexSSE
   //==============================================================================
   void DeformerGetFlatVertexC(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyFlatVert& dst, uint vertIndex)
   {
      BASSERT(vertIndex < pHeader->mNumVerts);

      if (vertIndex < pHeader->mNumRigidVerts)
      {
         const uint packetIndex = vertIndex >> 2;
         const uint packetOfs = vertIndex & 3;

         const BPacketVert& src = pHeader->getPackets()[packetIndex];

         dst.Position[0] = GetVecComp(src.x, packetOfs);
         dst.Position[1] = GetVecComp(src.y, packetOfs);
         dst.Position[2] = GetVecComp(src.z, packetOfs);

         dst.BoneIndices[0] = pPacketBoneIndices[packetIndex];
         dst.BoneIndices[1] = 0;
         dst.BoneIndices[2] = 0;
         dst.BoneIndices[3] = 0;

         dst.BoneWeights[0] = 255;
         dst.BoneWeights[1] = 0;
         dst.BoneWeights[2] = 0;
         dst.BoneWeights[3] = 0;

         const D3DXVECTOR3 n(unpackSingleNormal(reinterpret_cast<const DWORD*>(&src.pn)[packetOfs]));
         
         dst.Normal[0] = n.x;
         dst.Normal[1] = n.y;
         dst.Normal[2] = n.z;

         if (packetOfs < 2)
         {
            dst.UV[0] = GetVecComp(src.u, packetOfs*2+0);
            dst.UV[1] = GetVecComp(src.u, packetOfs*2+1);
         }
         else
         {
            dst.UV[0] = GetVecComp(src.v, (packetOfs-2)*2+0);
            dst.UV[1] = GetVecComp(src.v, (packetOfs-2)*2+1);
         }
      }
      else
      {
         const BBlendVert& src = pHeader->getBlendVerts()[vertIndex - pHeader->mNumRigidVerts];

         dst.Position[0] = src.Position[0];
         dst.Position[1] = src.Position[1];
         dst.Position[2] = src.Position[2];

         dst.BoneWeights[0] = src.BoneWeights[0];
         dst.BoneWeights[1] = src.BoneWeights[1];
         dst.BoneWeights[2] = 0;
         dst.BoneWeights[3] = 0;
         dst.BoneIndices[0] = src.BoneIndices[0];
         dst.BoneIndices[1] = src.BoneIndices[1];
         dst.BoneIndices[2] = 0;
         dst.BoneIndices[3] = 0;

         const D3DXVECTOR3 n(unpackSingleNormal(src.Normal));

         dst.Normal[0] = n.x;
         dst.Normal[1] = n.y;
         dst.Normal[2] = n.z;

         dst.UV[0] = src.UV[0];
         dst.UV[1] = src.UV[1];
      }
   }

} // namespace BFastMeshDeformer
