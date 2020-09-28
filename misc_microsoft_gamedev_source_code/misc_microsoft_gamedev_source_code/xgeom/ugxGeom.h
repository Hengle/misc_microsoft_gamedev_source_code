//------------------------------------------------------------------------------------------------------------------------
//
//  File: ugxGeom.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#define UGX_MODEL_EXTENSION "ugx"

// xcore
#include "math\generalVector.h"
#include "math\hypersphere.h"
#include "resource\ecfUtils.h"

// local
#include "unigeom.h"
#include "univertPacker.h"

namespace BUGXGeom
{
   enum
   {
      cECFFileID              = 0xAAC93746,
      cECFCachedDataChunkID   = 0x00000700,
      cECFIBChunkID           = 0x00000701,
      cECFVBChunkID           = 0x00000702,
      cECFGRXChunkID          = 0x00000703,
      cECFMaterialChunkID     = 0x00000704,
      cECFTreeChunkID         = 0x00000705,
            
      cGeomHeaderSignature    = 0xC2340004,
      
      cMaxInstances           = 4
   };
   
   template<bool Packed, bool BigEndian>
   class BSection
   {
      friend class BSection<Packed, !BigEndian>;
      
   public:
      typedef UnivertPacker<Packed, BigEndian> UnivertPackerType;
      typedef PACKED_ARRAY_TYPE(PACKED_TYPE(int)) IntArrayType;
      
      enum { MaxTris = 8192 };
      
      BSection() :
         mMaterialIndex(0),
         mAccessoryIndex(0),
         mMaxBones(0),
         mRigidOnly(false),
         mRigidBoneIndex(INT_MAX),
         mIBOfs(0),
         mVBOfs(0),
         mVBBytes(0),
         mVertSize(0),
         mNumVerts(0),
         mNumTris(0)
      {
         BCOMPILETIMEASSERT(sizeof(BSection) == sizeof(BSection<!Packed, BigEndian>));
         BCOMPILETIMEASSERT((sizeof(BSection) & 3) == 0);
      }
           
      template<bool OtherPacked, bool OtherBigEndian>
      BSection(const BSection<OtherPacked, OtherBigEndian>& other)
      {
         *this = other;
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      BSection& operator= (const BSection<OtherPacked, OtherBigEndian>& rhs)
      {  
         mMaterialIndex          = rhs.mMaterialIndex;
         mAccessoryIndex         = rhs.mAccessoryIndex;
         mMaxBones               = rhs.mMaxBones;
         mRigidBoneIndex         = rhs.mRigidBoneIndex;
         mIBOfs                  = rhs.mIBOfs;
         mNumTris                = rhs.mNumTris; 
         mVBOfs                  = rhs.mVBOfs;
         mVBBytes                = rhs.mVBBytes;
         mVertSize               = rhs.mVertSize;
         mNumVerts               = rhs.mNumVerts;
         mLocalToGlobalBoneRemap = rhs.mLocalToGlobalBoneRemap;
         mBaseVertPacker         = rhs.mBaseVertPacker;
         mRigidOnly              = rhs.mRigidOnly;

         return *this;
      }      
                        
      int materialIndex(void) const { return mMaterialIndex; }
      void setMaterialIndex(int materialIndex) { mMaterialIndex = materialIndex; }
      
      int accessoryIndex(void) const { return mAccessoryIndex; }
      void setAccessoryIndex(int accessoryIndex) { mAccessoryIndex = accessoryIndex; }
      
      int maxBones(void) const { return mMaxBones; }
      void setMaxBones(int maxBones) { mMaxBones = maxBones; }
      
      bool rigidOnly(void) const { return mRigidOnly; }
      void setRigidOnly(bool rigidOnly) { mRigidOnly = rigidOnly; }
      
      int rigidBoneIndex(void) const { return mRigidBoneIndex; }
      void setRigidBoneIndex(int rigidBoneIndex) { mRigidBoneIndex = rigidBoneIndex; }
                  
      int IBOfs(void) const { return mIBOfs; }
      void setIBOfs(int ofs) { mIBOfs = ofs; }
      
      int VBOfs(void) const { return mVBOfs; }
      void setVBOfs(int ofs) { mVBOfs = ofs; }
      
      int VBBytes(void) const { return mVBBytes; }
      void setVBBytes(int bytes) { mVBBytes = bytes; }
      
      int vertSize(void) const { return mVertSize; }
      void setVertSize(int size) { mVertSize = size; }
      
      int numVerts(void) const { return mNumVerts; }
      void setNumVerts(int numVerts) { mNumVerts = numVerts; }
                                
      int numTris(void) const { return mNumTris; }       
      void setNumTris(int numTris) { mNumTris = numTris; }
                        
      const UnivertPackerType& baseVertPacker(void) const { return mBaseVertPacker; }
      void setBaseVertPacker(const UnivertPackerType& baseVertPacker) { mBaseVertPacker = baseVertPacker; }
                  
      int numLocalBones(void) const { return static_cast<int>(mLocalToGlobalBoneRemap.size()); }
      int localToGlobalBone(int localBoneIndex) const { return mLocalToGlobalBoneRemap[debugRangeCheck(localBoneIndex, numLocalBones())]; }
      
            IntArrayType& localToGlobalBoneRemap(void)            { return mLocalToGlobalBoneRemap; }
      const IntArrayType& localToGlobalBoneRemap(void) const   { return mLocalToGlobalBoneRemap; }       
      
      void log(BTextDispatcher& l) const
      {
         l.printf("Material Index: %i\n", (int)mMaterialIndex);
         l.printf("Accessory Index: %i\n", (int)mAccessoryIndex);
         l.printf("Max Bones: %i\n", (int)mMaxBones);
         l.printf("Rigid only: %i\n", (int)mRigidOnly);
         l.printf("Rigid bone index: %i\n", (int)mRigidBoneIndex);
         l.printf("Index buffer offset: %i\n", (int)mIBOfs);
         l.printf("Num tris: %i\n", (int)mNumTris);
         l.printf("Vertex buffer offset: %i\n", (int)mVBOfs);
         l.printf("Vertex buffer bytes: %i\n", (int)mVBBytes);
         l.printf("Base vertex size: %i\n", (int)mVertSize);
         l.printf("Base vertex packer:\n");
         l.indent(1);
         mBaseVertPacker.log(l);
         l.indent(-1);
         l.printf("Local to global bone remap: \n");
         l.indent(1);
         for (uint i = 0; i < mLocalToGlobalBoneRemap.size(); i++)
            l.printf("%i ", (int)mLocalToGlobalBoneRemap[i]);
         l.printf("\n");
         l.indent(-1);
      }
      
      bool pack(BPackState& state)
      {
         if (!mLocalToGlobalBoneRemap.pack(state))
            return false;
         if (!mBaseVertPacker.pack(state))
            return false;
         return true;
      }
      
      bool unpack(const BDataBuffer& buf)
      {  
         if (!mLocalToGlobalBoneRemap.unpack(buf))
            return false;
         if (!mBaseVertPacker.unpack(buf))
            return false;
         return true;
      }
               
   private:
      PACKED_TYPE(int)        mMaterialIndex;
      PACKED_TYPE(int)        mAccessoryIndex;
      
      PACKED_TYPE(int)        mMaxBones;
      
      PACKED_TYPE(int)        mRigidBoneIndex;
      
      PACKED_TYPE(int)        mIBOfs;             // in indices, not bytes!
      PACKED_TYPE(int)        mNumTris;     
      
      PACKED_TYPE(int)        mVBOfs;
      PACKED_TYPE(int)        mVBBytes;
      PACKED_TYPE(int)        mVertSize;
      PACKED_TYPE(int)        mNumVerts;
      
      IntArrayType            mLocalToGlobalBoneRemap;
      
      UnivertPackerType       mBaseVertPacker;
      
      PACKED_TYPE(bool)       mRigidOnly;
      bool                    mPadding[3];
   };
      
   template<bool Packed, bool BigEndian>         
   class BHeader
   {
      friend class BHeader<Packed, !BigEndian>;
      
   public:
      typedef BVecN<3, PACKED_TYPE(float) > BPackedVec3Type;
      
      BHeader() 
      {
         BCOMPILETIMEASSERT(sizeof(BHeader) == sizeof(BHeader<!Packed, BigEndian>));
         BCOMPILETIMEASSERT((sizeof(BHeader) & 3) == 0);
         clear();
      }
          
      template<bool OtherPacked, bool OtherBigEndian>
      BHeader(const BHeader<OtherPacked, OtherBigEndian>& other)
      {
         *this = other;
      }
      
      void clear(void)
      {
         mSignature = (DWORD)cGeomHeaderSignature;
         mAllSectionsRigid = false;
         mAllSectionsSkinned = false;
         mGlobalBones = false;
         //mLevelGeom = false;            
         //mShadowGeom = false;
         mRigidOnly = false;
         mRigidBoneIndex = 0;
         mBoundingSphereCenter.clear();
         mBoundingSphereRadius = 0.0f;
         mBounds[0].clear();
         mBounds[1].clear();
         mMaxInstances = 1;
         mInstanceIndexMultiplier  = 0;
         mLargeGeomBoneIndex = INT16_MAX;
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      BHeader& operator= (const BHeader<OtherPacked, OtherBigEndian>& rhs)
      {
         mSignature               = rhs.mSignature;            
         mAllSectionsRigid        = rhs.mAllSectionsRigid;
         mAllSectionsSkinned      = rhs.mAllSectionsSkinned;
         mGlobalBones             = rhs.mGlobalBones;
         mRigidOnly               = rhs.mRigidOnly;
         mRigidBoneIndex          = rhs.mRigidBoneIndex;
         mBoundingSphereCenter    = rhs.mBoundingSphereCenter;
         mBoundingSphereRadius    = rhs.mBoundingSphereRadius;
         mBounds[0]               = rhs.mBounds[0];
         mBounds[1]               = rhs.mBounds[1];
         mMaxInstances            = rhs.mMaxInstances;
         mInstanceIndexMultiplier = rhs.mInstanceIndexMultiplier;
         mLargeGeomBoneIndex      = rhs.mLargeGeomBoneIndex;

         return *this;
      }
      
      DWORD getSignature(void) const { return mSignature; }
      
      // Not clearly named, should be something suggesting "rigid only multiple bone".
      bool allSectionsRigid(void) const { return mAllSectionsRigid; }
      void setAllSectionsRigid(bool allSectionsRigid) { mAllSectionsRigid = allSectionsRigid; }
      
      bool allSectionsSkinned(void) const { return mAllSectionsRigid; }
      void setAllSectionsSkinned(bool allSectionsSkinned) { mAllSectionsSkinned = allSectionsSkinned; }
      
      bool globalBones(void) const { return mGlobalBones; }
      void setGlobalBones(bool globalBones) { mGlobalBones = globalBones; }
                  
      // Not clearly named, should be something suggesting "rigid only single bone".
      bool rigidOnly(void) const { return mRigidOnly; }
      void setRigidOnly(bool rigidOnly) { mRigidOnly = rigidOnly; }
      
      // If rigidOnly() is true, this is the bone index used by all sections.
      int rigidBoneIndex(void) const { return mRigidBoneIndex; }
      void setRigidBoneIndex(int rigidBoneIndex) { mRigidBoneIndex = rigidBoneIndex; }
      
      const BPackedVec3Type& getBoundingSphereCenter(void) const { return mBoundingSphereCenter; }
      float getBoundingSphereRadius(void) const { return mBoundingSphereRadius; }
      void setBoundingSphere(const Sphere& sphere) 
      { 
         mBoundingSphereCenter = sphere.origin();
         mBoundingSphereRadius = sphere.radius();
      }

      const BPackedVec3Type& getBounds(uint i) const { return mBounds[i]; }
      void setBounds(const AABB& bounds) 
      { 
         mBounds[0] = bounds.low(); 
         mBounds[1] = bounds.high();
      }
      
      uint getMaxInstances(void) const { return mMaxInstances; }
      void setMaxInstances(uint maxInstances) { mMaxInstances = static_cast<uint16>(maxInstances); }
      
      uint getInstanceIndexMultiplier(void) const { return mInstanceIndexMultiplier; }
      void setInstanceIndexMultiplier(uint instanceIndexMultiplier) { mInstanceIndexMultiplier = static_cast<uint16>(instanceIndexMultiplier); }
      
      bool hasLargeGeomBoneIndex(void) const { return INT16_MAX != mLargeGeomBoneIndex; }
      int largeGeomBoneIndex(void) const { return mLargeGeomBoneIndex; }
      void setLargeGeomBoneIndex(int largeGeomBoneIndex) { mLargeGeomBoneIndex = static_cast<int16>(largeGeomBoneIndex); }
      
      void log(BTextDispatcher& l) const
      {
         l.printf("RigidOnly: %i RigidBoneIndex: %i AllSectionsRigid: %i, AllSectionsSkinned: %i\n", (int)mRigidOnly, (int)mRigidBoneIndex, (int)mAllSectionsRigid, (int)mAllSectionsSkinned);
         l.printf("MaxInstances: %u, InstanceIndexMultiplier: %u", (uint)mMaxInstances, (uint)mInstanceIndexMultiplier);
         l.printf("GlobalBones: %i\n", (int)mGlobalBones);
         l.printf("BoundingSphere: (%f %f %f) %f\n", (float)mBoundingSphereCenter[0], (float)mBoundingSphereCenter[1], (float)mBoundingSphereCenter[2], (float)mBoundingSphereRadius);
         l.printf("AABB: (%f %f %f) (%f %f %f)\n",
            (float)mBounds[0][0], (float)mBounds[0][1], (float)mBounds[0][2],
            (float)mBounds[1][0], (float)mBounds[1][1], (float)mBounds[1][2]);
      }
                              
   private:
      PACKED_TYPE(DWORD)      mSignature;
      PACKED_TYPE(int)        mRigidBoneIndex;
            
      BPackedVec3Type         mBoundingSphereCenter;
      PACKED_TYPE(float)      mBoundingSphereRadius;
      
      BPackedVec3Type         mBounds[2];
      
      PACKED_TYPE(int16)      mMaxInstances;
      PACKED_TYPE(int16)      mInstanceIndexMultiplier;
      
      PACKED_TYPE(int16)      mLargeGeomBoneIndex;
            
      PACKED_TYPE(bool)       mAllSectionsRigid;
      PACKED_TYPE(bool)       mGlobalBones;
      PACKED_TYPE(bool)       mAllSectionsSkinned;

      PACKED_TYPE(bool)       mRigidOnly;
   };
   
   template<bool Packed, bool BigEndian>
   class BCachedData
   {
      friend class BCachedData<Packed, !BigEndian>;
      
   public:
      typedef BVecN<3, PACKED_TYPE(float) >                 BPackedVec3Type;
      typedef BHeader<Packed, BigEndian>                    GeomHeaderType;
      typedef BSection<Packed, BigEndian>                   SectionType;
      typedef Unigeom::Bone<Packed, BigEndian>              BoneType;
      typedef Unigeom::Accessory<Packed, BigEndian>         AccessoryType;
            
      typedef PACKED_ARRAY_TYPE(PACKED_TYPE(int))           IntArrayType;
      typedef PACKED_ARRAY_TYPE(SectionType)                SectionArrayType;
      typedef PACKED_ARRAY_TYPE(BYTE)                       MaterialArrayType;
      typedef PACKED_ARRAY_TYPE(BoneType)                   BoneArrayType;
      typedef PACKED_ARRAY_TYPE(AccessoryType)              AccessoryArrayType;
      typedef PACKED_ARRAY_TYPE(BPackedVec3Type)            BPackedVec3ArrayType;
      
      BCachedData() 
      {
         BCOMPILETIMEASSERT(sizeof(BCachedData) == sizeof(BCachedData<!Packed, BigEndian>));
         BCOMPILETIMEASSERT((sizeof(BCachedData) & 3) == 0);
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      BCachedData(const BCachedData<OtherPacked, OtherBigEndian>& geom)
      {
         *this = geom;
      }
                  
      template<bool OtherPacked, bool OtherBigEndian>
      BCachedData& operator= (const BCachedData<OtherPacked, OtherBigEndian>& rhs)
      {
         mHeader           = rhs.mHeader;
         mSections         = rhs.mSections;

         mBones            = rhs.mBones;
         mAccessories      = rhs.mAccessories;
         mValidAccessories = rhs.mValidAccessories;
         mBoneBoundsLow    = rhs.mBoneBoundsLow;
         mBoneBoundsHigh   = rhs.mBoneBoundsHigh;  

         return *this;
      }
                
      const GeomHeaderType& header(void) const { return mHeader; }
            GeomHeaderType& header(void) { return mHeader; }
                                 
      int numSections(void) const { return static_cast<int>(mSections.size()); }
      const SectionType& section(int i) const { return mSections[debugRangeCheck(i, numSections())]; }
            SectionType& section(int i)       { return mSections[debugRangeCheck(i, numSections())]; }
      const SectionArrayType& sections(void) const { return mSections; }                 
            SectionArrayType& sections(void)       { return mSections; }                 
                              
      int numBones(void) const { return static_cast<int>(mBones.size()); }
      const BoneType& bone(int i) const { return mBones[debugRangeCheck(i, numBones())]; }
      const BoneArrayType& bones(void) const { return mBones; }
            BoneArrayType& bones(void)       { return mBones; }
      
      int numAccessories(void) const                           { return static_cast<int>(mAccessories.size()); }
      const AccessoryType& accessory(int i) const         { return mAccessories[debugRangeCheck(i, numAccessories())]; }
            AccessoryType& accessory(int i)               { return mAccessories[debugRangeCheck(i, numAccessories())]; }
      const AccessoryArrayType& accessories(void) const  { return mAccessories; }
            AccessoryArrayType& accessories(void)        { return mAccessories; }
      
      int numValidAccessories(void) const          { return static_cast<int>(mValidAccessories.size()); }
      int validAccessory(int i) const             { return mValidAccessories[debugRangeCheck(i, numValidAccessories())]; }
      const IntArrayType& validAccessories(void) const   { return mValidAccessories; }
            IntArrayType& validAccessories(void)         { return mValidAccessories; }

      int numBoneBounds(void) const { return static_cast<int>(mBoneBoundsLow.size()); }
      const BPackedVec3Type& boneBoundsLow(int i) const { return mBoneBoundsLow[i]; }
            BPackedVec3Type& boneBoundsLow(int i)       { return mBoneBoundsLow[i]; }
      const BPackedVec3Type& boneBoundsHigh(int i) const { return mBoneBoundsHigh[i]; }
            BPackedVec3Type& boneBoundsHigh(int i)       { return mBoneBoundsHigh[i]; }
            
      const BPackedVec3ArrayType& boneBoundsLowArray(void) const  { return mBoneBoundsLow; }
            BPackedVec3ArrayType& boneBoundsLowArray(void)        { return mBoneBoundsLow; }
            
      const BPackedVec3ArrayType& boneBoundsHighArray(void) const  { return mBoneBoundsHigh; }
            BPackedVec3ArrayType& boneBoundsHighArray(void)        { return mBoneBoundsHigh; }
                 
      void log(BTextDispatcher& l) const
      {
         l.printf("********************* BUGXGeom::BCachedData:\n");
                  
         l.indent(1);
         
         mHeader.log(l);
         
         l.printf("---- Bones: %i\n", numBones());
         l.indent(1);
         for (int i = 0; i < numBones(); i++)
         {
            l.printf("Bone: %i\n", i);
            l.indent(1);
            bone(i).log(l);

            l.printf("Bounds:\n");
            l.indent(1);
            boneBoundsLow(i).log(l);
            boneBoundsHigh(i).log(l);
            l.indent(-1);

            l.indent(-1);
         }
         l.indent(-1);

         l.printf("---- Accessories: %i\n", mAccessories.size());
         l.indent(1);
         for (int i = 0; i < numAccessories(); i++)
         {
            l.printf("Accessory: %i\n", i);
            l.indent(1);
            accessory(i).log(l);
            l.indent(-1);
         }
         l.indent(-1);

         l.printf("---- Valid Accessories: %i\n", mValidAccessories.size());
         l.indent(1);
         for (int i = 0; i < numValidAccessories(); i++)
            l.printf("%i ", validAccessory(i));
         l.printf("\n");
         l.indent(-1); 
              
                                 
         l.printf("---- Sections: %i\n", numSections());
         l.indent(1);
         for (int i = 0; i < numSections(); i++)
         {
            l.printf("Section: %i\n", i);
            l.indent(1);
            section(i).log(l);
            l.indent(-1);
         }
         l.indent(-1);
                     
         l.indent(-1);
      }
      
      void clear(void)
      {
         mHeader.clear();
         mSections.clear();
         mBones.clear();
         mAccessories.clear();
         mBoneBoundsLow.clear();
         mBoneBoundsHigh.clear();
      }

      bool pack(BPackState& state)
      {  
         if (!mSections.pack(state))
            return false;
         if (!mBones.pack(state))
            return false;
         if (!mAccessories.pack(state))
            return false;
         if (!mValidAccessories.pack(state))
            return false;
         if (!mBoneBoundsLow.pack(state))
            return false;
         if (!mBoneBoundsHigh.pack(state))
            return false;
         return true;
      }  
                  
      bool unpack(const BDataBuffer& buf)
      {  
         if (!checkHeader())
            return false;
         
         if (!mSections.unpack(buf))
            return false;
         
         if (!mBones.unpack(buf))
            return false;
         if (!mAccessories.unpack(buf))
            return false;
         if (!mValidAccessories.unpack(buf))
            return false;
         if (!mBoneBoundsLow.unpack(buf))
            return false;
         if (!mBoneBoundsHigh.unpack(buf))
            return false;
         
         return true;
      }
      
      bool checkHeader(void)
      {
         return (mHeader.getSignature() == (DWORD)cGeomHeaderSignature);
      }
                
   private:
      GeomHeaderType          mHeader;
      SectionArrayType        mSections;
      BoneArrayType           mBones;
      AccessoryArrayType      mAccessories;                                          
      IntArrayType            mValidAccessories;
      BPackedVec3ArrayType    mBoneBoundsLow;
      BPackedVec3ArrayType    mBoneBoundsHigh;
   };
   
   template<bool BigEndian>
   class BECFDataBuilder
   {
      friend class BECFDataBuilder<!BigEndian>;
      
   public:
      typedef uint16                                              IndexType;
      typedef BDynamicArray<uchar>                                VBArrayType;
      typedef BDynamicArray< BPackedType<IndexType, BigEndian> >  IBArrayType;      
      typedef BDynamicArray<uchar>                                MaterialDataType;
      typedef BCachedData<false, BigEndian>                       CachedDataType;
      
      BECFDataBuilder()
      {
      }

      template<bool OtherBigEndian>      
      BECFDataBuilder(const BECFDataBuilder<OtherBigEndian>& other)
      {
         *this = other;
      }
      
      template<bool OtherBigEndian>      
      BECFDataBuilder& operator= (const BECFDataBuilder<OtherBigEndian>& rhs)
      {
         mCachedData    = rhs.mCachedData;
         mVB            = rhs.mVB;
         mIB            = rhs.mIB;
         mMaterialData  = rhs.mMaterialData;
         return *this;
      }
      
      ~BECFDataBuilder()
      {
      }
      
      void clear(void)
      {
         mCachedData.clear();
         mIB.clear();
         mVB.clear();
      }
      
      bool createPackedData(BECFFileBuilder& ecfBuilder) const
      {
         ecfBuilder.setID(0);
         
         BMemStack alloc(&gPrimaryHeap, 64 * 1024 * 1024);
         
         CachedDataType* pPackedCachedData = ALLOCATOR_NEW_INIT(CachedDataType, alloc, mCachedData);
         if (!pPackedCachedData)
            return false;
         
         BPackState packState(alloc);
         if (!pPackedCachedData->pack(packState))
            return false;

         BByteArray cachedDataBytes(alloc.getAllocatedBytes());
         memcpy(cachedDataBytes.getPtr(), alloc.getBasePtr(), alloc.getAllocatedBytes());
         
         BECFChunkData& cachedChunk = ecfBuilder.addChunk((uint64)cECFCachedDataChunkID, cachedDataBytes);
         cachedChunk.setAlignmentLog2(4);
         
         BECFChunkData& vbChunk = ecfBuilder.addChunk((uint64)cECFVBChunkID, mVB.getPtr(), mVB.getSizeInBytes());
         // rg [1/26/08] - HACK HACK- change gr2ugx so it doesn't request unnecessarily large alignments for indices and vertices!      
         vbChunk.setAlignmentLog2(5);
         vbChunk.setResourceBitFlag(cECFChunkResFlagContiguous, true);
         
         BECFChunkData& ibChunk = ecfBuilder.addChunk((uint64)cECFIBChunkID, (const BYTE*)mIB.getPtr(), mIB.getSizeInBytes());
         // rg [1/26/08] - HACK HACK- change gr2ugx so it doesn't request unnecessarily large alignments for indices and vertices!      
         ibChunk.setAlignmentLog2(5);
         ibChunk.setResourceBitFlag(cECFChunkResFlagContiguous, true);
         
         BECFChunkData& materialDataChunk = ecfBuilder.addChunk((uint64)cECFMaterialChunkID, (const BYTE*)mMaterialData.getPtr(), mMaterialData.getSizeInBytes());
         materialDataChunk.setAlignmentLog2(2);
                  
         ecfBuilder.setID((uint)cECFFileID);
         
         return true;
      }
      
      void log(BTextDispatcher& l) const
      {
         mCachedData.log(l);
         
         l.printf("Vertex buffer size: %u bytes\n", mVB.size());
         l.printf(" Index buffer size: %u bytes\n", mIB.size() * sizeof(mIB[0]));
         l.printf("Material data size: %u bytes\n", mMaterialData.getSize());
      }
      
      const CachedDataType& getCachedData(void) const   { return mCachedData; }
            CachedDataType& getCachedData(void)         { return mCachedData; }
            
      const VBArrayType& getVB(void) const { return mVB; }            
            VBArrayType& getVB(void)       { return mVB; }            
            
      const IBArrayType& getIB(void) const { return mIB; }            
            IBArrayType& getIB(void)       { return mIB; }            
            
      const MaterialDataType& getMaterialData(void) const { return mMaterialData; }            
            MaterialDataType& getMaterialData(void)       { return mMaterialData; }            
   
   private:
      CachedDataType       mCachedData;
      
      VBArrayType          mVB;
      IBArrayType          mIB;
      MaterialDataType     mMaterialData;
   };   
   
   typedef BSection<false, cBigEndianNative>    BUnpackedSection;
   typedef BHeader<false, cBigEndianNative>     BUnpackedHeader;
   typedef BCachedData<false, cBigEndianNative> BUnpackedCachedData;
   
   typedef BSection<true, cBigEndianNative>     BNativeSectionType;
   typedef BHeader<true, cBigEndianNative>      BNativeHeaderType;
   typedef BCachedData<true, cBigEndianNative>  BNativeCachedDataType;
   
} // namespace BUGXGeom

DEFINE_PACKABLE_TYPE(BUGXGeom::BSection)
DEFINE_PACKABLE_TYPE(BUGXGeom::BCachedData)


