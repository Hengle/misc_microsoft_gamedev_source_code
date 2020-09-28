//============================================================================
//
//  fastdeformerdata.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once
#include "granny.h"
#include "hash\crc.h"

#define FAST_DEFORMER_DATA_VERSION  0xFDABCD01
#define FAST_DEFORMER_DATA_VERSION_SWAPPED  0x01CDABFD

namespace BFastMeshDeformer
{
#ifdef XBOX
   typedef D3DXVECTOR4 VecType;
#else
   typedef __m128 VecType;
#endif   

   inline float GetVecComp(VecType x, uint o)
   {
      BASSERT(o < 4);
      return reinterpret_cast<const float*>(&x)[o];
   }

   inline void SetVecComp(VecType& v, uint o, float f) 
   { 
      BASSERT(o < 4);
      reinterpret_cast<float*>(&v)[o] = f; 
   }
         
   inline void WriteDWordsLittleEndian(void* pDst, const void* pSrc, int n = 1)
   {
      memcpy(pDst, pSrc, n * sizeof(DWORD));
#ifdef XBOX
      EndianSwitchDWords(reinterpret_cast<DWORD*>(pDst), n);
#endif
   }
   
   inline void WriteDWordLittleEndian(void* pDst, DWORD d)
   {
      WriteDWordsLittleEndian(pDst, &d);
   }
   
   //==============================================================================
   // struct BGrannyBumpVert
   //==============================================================================
   struct BGrannyBumpVert
   {
      granny_real32 Position[3];
      granny_uint8 BoneWeights[4];
      granny_uint8 BoneIndices[4];
      granny_real32 Normal[3];
      granny_real32 Tangent[3];
      granny_real32 UV[2];
   };
   
   //==============================================================================
   // struct BGrannyFlatVert
   //==============================================================================
   struct BGrannyFlatVert
   {
      granny_real32 Position[3];
      granny_uint8 BoneWeights[4];
      granny_uint8 BoneIndices[4];
      granny_real32 Normal[3];
      granny_real32 UV[2];
   };
    
   //==============================================================================
   // struct BBlendVert
   // 32 bytes per vertex.
   //==============================================================================
   struct BBlendVert
   {
      granny_real32 Position[3];
      granny_uint8 BoneWeights[2];
      granny_uint8 BoneIndices[2];
      DWORD Normal;
      DWORD Tangent;
      granny_real32 UV[2];
      
#ifdef XBOX            
      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "fffcccciiff");
      }
#endif   
   };
         
   //==============================================================================
   // struct BPacketVert
   // 112 bytes, or 28 bytes per vertex.
   //==============================================================================
   struct BPacketVert
   {
      // positions
      VecType x;
      VecType y;
      VecType z;

      // norms/tangents (packed as abcFF abcFF abcFF abcFF)
      VecType pn;
      VecType pt;

      // uv's
      VecType u;
      VecType v;
      
#ifdef XBOX            
      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "28(f)");
      }
#endif              
   };

   typedef BDynamicArray<BPacketVert, 64> BPacketVertArray;
   
   //==============================================================================
   // struct BBlock
   // A block describes a run of rigidly bound vertex packets all using the same bone matrix.
   //==============================================================================
   struct BBlock
   {
      ushort mNumPackets;
      ushort mDstVert;

      uchar mBoneIndex;
      uchar mPad0;
      uchar mPad1;
      uchar mPad2;

      BBlock() { }

      BBlock(ushort numPackets, ushort dstVert, uchar boneIndex) :
         mNumPackets(numPackets),
         mDstVert(dstVert),
         mBoneIndex(boneIndex),
         mPad0(0),
         mPad1(0),
         mPad2(0)
      {
      }         
      
#ifdef XBOX            
      void endianSwap(void)
      {
         EndianSwitchWorker(this, this + 1, "sscccc");
      }
#endif              
   };

   typedef BDynamicArray<BBlock> BBlockArray;

   //==============================================================================
   // struct BDataHeader
   //==============================================================================
   // sizeof(BDataHeader) must be a multiple of 16.
   struct BDataHeader
   {
      // Number of DWORD's to skip before calculating the CRC.
      enum { NumPrefixDWORDs = 3 };
      
      // Should be FAST_DEFORMER_DATA_VERSION  
      DWORD mDataVersion;
      // Size of data following the CRC-32
      DWORD mDataSize;
      // CRC-32 of subsequent data
      DWORD mDataCRC;
      
      // Padding, for alignment and future expansion.
      DWORD mNumVerts;

      DWORD mNumBlocks;
      DWORD mBlockDataSize;

      DWORD mNumPackets;
      DWORD mPacketDataSize;

      DWORD mNumBlendVerts;
      DWORD mBlendVertDataSize;

      DWORD mNumRigidVerts;
      DWORD mNumBones;
                        
      enum BFlags
      {
         cAllFlat = 1,
         
         cForceDWORD = 0xFFFFFFFF
      };
      
      DWORD mFlags;
      DWORD mPad1;
      DWORD mPad2;
      DWORD mPad3;

      // Blocks, packets, and blend verts appear after header sequentially.

      BDataHeader() 
      {
         clear();
      }
      
      void clear(void)
      {
         Utils::ClearObj(*this);
      }

      const uchar* getData(void) const { return reinterpret_cast<const uchar*>(this) + sizeof(*this); }
            uchar* getData(void)       { return reinterpret_cast<      uchar*>(this) + sizeof(*this); }

      const BBlock* getBlocks(void) const { return reinterpret_cast<const BBlock*>(getData()); }
            BBlock* getBlocks(void)       { return reinterpret_cast<      BBlock*>(getData()); }

      const BPacketVert* getPackets(void) const { return reinterpret_cast<const BPacketVert*>(getData() + mBlockDataSize); } 
            BPacketVert* getPackets(void)       { return reinterpret_cast<      BPacketVert*>(getData() + mBlockDataSize); } 

      const BBlendVert* getBlendVerts(void) const  { return reinterpret_cast<const BBlendVert*>(getData() + mBlockDataSize + mPacketDataSize); }
            BBlendVert* getBlendVerts(void)        { return reinterpret_cast<      BBlendVert*>(getData() + mBlockDataSize + mPacketDataSize); }

      bool isEndianSwapped(void) const
      {
         return (FAST_DEFORMER_DATA_VERSION_SWAPPED == mDataVersion);
      }

      bool crcCheck(void) const
      {
         DWORD dataVersion = mDataVersion;
         DWORD dataSize = mDataSize;
         DWORD dataCRC = mDataCRC;

#ifdef XBOX         
         if (isEndianSwapped())
         {
            EndianSwitchDWords(&dataVersion, 1);
            EndianSwitchDWords(&dataSize, 1);
            EndianSwitchDWords(&dataCRC, 1);
         }
#endif            

         if (FAST_DEFORMER_DATA_VERSION != dataVersion)
            return false;
         
         const uchar* pData = reinterpret_cast<const uchar*>(this);
         const DWORD crc = calcCRC32SmallTable(reinterpret_cast<const BYTE*>(pData) + sizeof(DWORD) * BDataHeader::NumPrefixDWORDs, dataSize, 0, false);

         if (crc != dataCRC)
            return false;
            
         return true;
      }

#ifdef XBOX            
      bool endianSwap(bool headerOnly = false)
      {
         BDataHeader nativeHeader(*this);
         
         EndianSwitchWorker(this, this + 1, "iiiiiiiiiiiiiiii");
                  
         if (!headerOnly)                  
         {
            if (nativeHeader.isEndianSwapped())
               EndianSwitchWorker(&nativeHeader, &nativeHeader + 1, "iiiiiiiiiiiiiiii");
               
            BBlock* pBlocks = reinterpret_cast<BBlock*>(reinterpret_cast<uchar*>(this) + sizeof(BDataHeader));
            for (uint i = 0; i < nativeHeader.mNumBlocks; i++)
               pBlocks[i].endianSwap();

            BPacketVert* pPackets = reinterpret_cast<BPacketVert*>(reinterpret_cast<uchar*>(this) + sizeof(BDataHeader) + nativeHeader.mBlockDataSize);
            for (uint i = 0; i < nativeHeader.mNumPackets; i++)
               pPackets[i].endianSwap();

            BBlendVert* pBlendVerts = reinterpret_cast<BBlendVert*>(reinterpret_cast<uchar*>(this) + sizeof(BDataHeader) + nativeHeader.mBlockDataSize + nativeHeader.mPacketDataSize);
            for (uint i = 0; i < nativeHeader.mNumBlendVerts; i++)
               pBlendVerts[i].endianSwap();
         }               
            
         return true;            
      }
#endif              
   };

   //==============================================================================
   // class BDataBuilder
   // class BFastMeshDeformer::BDataBuilder
   //==============================================================================
   class BDataBuilder
   {
   public:
      BDataBuilder();
      ~BDataBuilder();

      // True if the fast deformer can handle this mesh.
      static bool isSupported(granny_mesh* pMesh);

      // False on failure (unsupported mesh).
      // Important: This will modify the granny mesh!
      // The vertices will be sorted and the mesh indices will be translated to index into the sorted VB.
      bool init(granny_mesh* pMesh);

      // Returns a 16-byte aligned pointer to the fast deformer data.
      const uchar* getData(void) { return mData.size() ? &mData[0] : NULL; }
      
      // Returns the size of the fast deformer data.
      const DWORD getDataLen(void) { return mData.size(); }
            
   private:
      granny_mesh* mpMesh;
      uint mNumRigidVerts;
      int mNumBones;
          
      BDynamicArray<uchar, 16> mData;

      bool mSupported;
      bool mFlatMesh;
      
      struct BPacket
      {
         ushort mVertIndex[4];
      };

      typedef BDynamicArray<BPacket> BPacketArray;

      struct BSortVert
      {
         BGrannyBumpVert mVert;
         ushort mVertIndex;
         uchar mBoneIndex;
         uchar mNumBones;

         BSortVert() { }
         
         BSortVert(const BGrannyBumpVert& vert, ushort vertIndex, uchar boneIndex, uchar numBones) : 
            mVert(vert), 
            mVertIndex(vertIndex), 
            mBoneIndex(boneIndex), 
            mNumBones(numBones) 
         { 
         }

         friend bool operator< (const BSortVert& a, const BSortVert& b)
         {
            if (a.mNumBones < b.mNumBones)
               return true;
            else if (a.mNumBones == b.mNumBones)
            {
               if (a.mBoneIndex < b.mBoneIndex)
               {
                  return true;
               }
               else if (a.mBoneIndex == b.mBoneIndex)
               {
                  return a.mVertIndex < b.mVertIndex;
               }
            }

            return false;
         }
      };

      typedef BDynamicArray<BSortVert> BSortVertArray;

      static DWORD compressNormal(const float* pNormal);
      void convertGrannyVert(BGrannyBumpVert* pDst, int vertIndex);
      void fixGrannyIndices(const BSortVertArray& sortVerts);
      void createSortVerts(BSortVertArray& sortVerts);
      void createDataBlock(const BBlockArray& blocks, BPacketVertArray& packetVerts, const BSortVertArray& sortVerts);
      void compressBlendVerts(BDataHeader& header, const BSortVertArray& sortVerts, uint numBlendVerts, uint blockDataSize, uint packetDataSize);
      void clear(void);
      void debugCheck(const BSortVertArray& sortVerts);
   };
      
} // namespace BFastMeshDeformer
