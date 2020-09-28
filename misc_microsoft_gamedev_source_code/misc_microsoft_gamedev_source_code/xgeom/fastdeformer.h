//============================================================================
//
//  fastdeformer.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once
#include "fastdeformerdata.h"

#define FAST_DEFORMER_CLASS_VERSION 0xDEADAB05

namespace BFastMeshDeformer
{
   class BDeformer;
  
   //==============================================================================
   // struct pngt3332_vertex
   // 44 bytes, 176 bytes, 11 vec128's
   //==============================================================================
   struct pngt3332_vertex
   {
      granny_real32 Position[3];
      granny_real32 Normal[3];
      granny_real32 Tangent[3];
      granny_real32 UV[2];
   };
   
   //==============================================================================
   // struct pnt332_vertex
   // 32 bytes, 128 bytes total, 8 vec128's
   //==============================================================================
   struct pnt332_vertex
   {
      granny_real32 Position[3];
      granny_real32 Normal[3];
      granny_real32 UV[2];
   };
   
   //==============================================================================
   // struct p3_vertex
   // 12 bytes, 48 bytes total, 3 vec128's
   //==============================================================================
   struct p3_vertex
   {
      granny_real32 Position[3];
   };
   
   enum BVertFormat
   {
      cAutoVertFormat,
      cFlatVertFormat,
      cBumpVertFormat,
      cPosOnlyVertFormat
   };
         
   struct BWorkQueueEntry
   {
      BWorkQueueEntry() { }

      BWorkQueueEntry(
         BDeformer* pDeformer,
         const granny_int32x* pMatrixIndices,
         const granny_real32* pMatrixBuffer4x4,
         void* pDestVertices,
         BVertFormat vertFormat) :
         mpDeformer(pDeformer),
         mpMatrixIndices(pMatrixIndices),
         mpMatrixBuffer4x4(pMatrixBuffer4x4),
         mpDestVertices(pDestVertices),
         mVertFormat(vertFormat)
      {
      }

      BDeformer*           mpDeformer;
      const granny_int32x* mpMatrixIndices;
      const granny_real32* mpMatrixBuffer4x4;
      void*                mpDestVertices;
      BVertFormat          mVertFormat;
   };

#ifndef XBOX   
   void DeformerRigidToStackAlignedSSE(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerRigidToStackAlignedNoTanSSE(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerRigidToStackAlignedPosOnlySSE(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerBlendedSSE(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, pngt3332_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerBlendedNoTanSSE(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, pnt332_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerBlendedPosOnlySSE(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, p3_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerCopyMatricesSSE(const BDataHeader* pHeader, __m128* pDst, const __m128* pSrc, const granny_int32x* MatrixIndices);
   void DeformerGetFlatVertexSSE(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyFlatVert& dst, uint vertIndex);
   void DeformerGetBumpVertexSSE(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyBumpVert& dst, uint vertIndex);
#endif   
   
   void DeformerRigidToStackAlignedC(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerRigidToStackAlignedNoTanC(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerRigidToStackAlignedPosOnlyC(const BDataHeader* pHeader, const BWorkQueueEntry& work);
   void DeformerBlendedC(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, pngt3332_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerBlendedNoTanC(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, pnt332_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerBlendedPosOnlyC(const BWorkQueueEntry& work, const BBlendVert* pSrcVertA, p3_vertex* pDstVertA, const uint numBlendVerts);
   void DeformerCopyMatricesC(const BDataHeader* pHeader, VecType* pDst, const VecType* pSrc, const granny_int32x* MatrixIndices);
   void DeformerGetFlatVertexC(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyFlatVert& dst, uint vertIndex);
   void DeformerGetBumpVertexC(const BDataHeader* pHeader, uchar* pPacketBoneIndices, BGrannyBumpVert& dst, uint vertIndex);
   
   //==============================================================================
   // class BFastMeshDeformer::BDeformer
   //==============================================================================   
   class BDeformer
   {
   public:
      BDeformer();
      ~BDeformer();
            
      // Data block must be 16-byte aligned. 
      // False on failure -- either block isn't aligned, dataLen is too small, or CRC-32 of block is bad.
      // Requires native endianness.
      bool setData(const uchar* pData, uint dataLen);
      
      bool crcCheck(const uchar* pData, uint dataLen) const;
      
#ifdef XBOX      
      bool endianSwap(uchar* pData, uint dataLen);
#endif      
                  
      // True if the deform was queued for later -- in this case the source data must be persistent until sync() is called!
      // Set copyMatrices to true if the MatrixBuffer4x4 data will not be persistent.
      bool deform(
         const granny_int32x * MatrixIndices,
         const granny_real32 * MatrixBuffer4x4,
         void* DestVertices,
         BVertFormat vertFormat,
         bool multithreaded,
         bool copyMatrices,
         bool finalBuild);
           
      const BDataHeader* getHeader(void) const { return mpHeader; }
      
      // Vertex accessors
      
      void getVertex(BGrannyBumpVert& dst, uint vertIndex);
      void getVertex(BGrannyFlatVert& dst, uint vertIndex);
      
      // Multithreading support
                        
      static void enableMultithreading(void);
      static bool isMultithreadingEnabled(void) { return INVALID_HANDLE_VALUE != mHelperThread; }
      static void sync(void);      
      static void wakeupWorkerThread(void);
      
      static void initHelperThread(void);   
      static void destroyHelperThread(void);
      
      struct BStats
      {
         DWORD mNumDeformedInMainThread;
         DWORD mNumDeformedInHelperThread;
         DWORD mNumDeformedByGranny;
         
         BStats() : mNumDeformedInMainThread(0), mNumDeformedInHelperThread(0), mNumDeformedByGranny(0)
         {
         }
      };
      
      static void getStats(BStats& stats);
      static void clearStats(void);
      
   private:
      const uchar*         mpData;
      uint                 mDataLen;
      const BDataHeader*   mpHeader;
      uchar*               mpPacketBoneIndices;
                                    
      // mVersion is used to detect mismatches between the VCC and ICL compiled modules (the ICL compiled stuff is in a lib that could go out of sync)
      DWORD                mVersion; 
      
      void clear(void);   
                              
      void deformRigidToStackAligned(const BWorkQueueEntry& work);
      
      void deformBlended(
         const BWorkQueueEntry& work,
         const BBlendVert* pSrcVertA,
         pngt3332_vertex* pDstVertA,
         const uint numBlendVerts);
         
      void deformRigidToStackAlignedNoTan(const BWorkQueueEntry& work);
      
      void deformRigidToStackAlignedPosOnly(const BWorkQueueEntry& work);
      
      void deformBlendedNoTan(
         const BWorkQueueEntry& work,
         const BBlendVert* pSrcVertA,
         pnt332_vertex* pDstVertA,
         const uint numBlendVerts);

      void deformBlendedPosOnly(
         const BWorkQueueEntry& work,
         const BBlendVert* pSrcVertA,
         p3_vertex* pDstVertA,
         const uint numBlendVerts);
         
      void processWorkEntry(const BWorkQueueEntry& work);
      
      void createPacketBoneIndices(void);
                           
      // rg [8/23/06] - Hack to save memory on Xbox
#ifdef XBOX
      enum { cMatrixBufEntries = 1 };
#else      
      enum { cMatrixBufEntries = 256 * 20 * 4 };
#endif      
      static VecType mMatrixBuf[cMatrixBufEntries];
      static int mMatrixBufSize;

#ifdef XBOX
      enum { cWorkQueueEntries = 1 };
#else      
      enum { cWorkQueueEntries = 256 };
#endif      
      static BWorkQueueEntry mWorkQueue[cWorkQueueEntries];
      static int mWorkQueueSize;
      static int mWorkQueueNextAvail;
      static CRITICAL_SECTION mWorkQueueCriticalSection;
      static BWorkQueueEntry* getNextWorkQueueEntry(void);
      
      static HANDLE mHelperThread;
      static bool mHelperThreadExitFlag;
      static HANDLE mWakeupEvent;   
      static int volatile mNumWorkEntriesProcessed;
      static bool volatile mWorkerThreadProcessingEnabled;
      
      enum 
      {
         cNumDeformedMainThreadIndex     = 0,
         cNumDeformedHelperThreadIndex   = 1,
         cNumDeformedGrannyIndex         = 2,
         cNumDeformedTotalIndices
      };

      static DWORD mNumDeformed[cNumDeformedTotalIndices];
              
      static unsigned int __stdcall helperThreadProc(void* pArguments);
      static void setMultithreadingStatics(void);
      static void defaultPoolCallback(bool create, void* pData1, void* pData2);
      static void initDefaultPoolCallback(void);
      static void deinitDefaultPoolCallback(void);
   };

} // namespace BFastMeshDeformer

