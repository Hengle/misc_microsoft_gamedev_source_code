//-----------------------------------------------------------------------------
// File: uniGeom.h
// Universal Geometry Format
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

// xcore
#include "containers\unifier.h"
#include "utils\packedType.h"
#include "utils\packedString.h"
#include "memory\memStack.h"

// local
#include "univert.h"
#include "indexedTri.h"
#include "unigeomMaterial.h"
#include "tri.h"

namespace Unigeom
{
   enum 
   {
      MaxBoneInfluences = Univert::MaxInfluences
   };

   #define SHADOW_MESH_NORMALS 0

   typedef int MaterialIndex;
   typedef int BoneIndex;
   typedef int AccessoryIndex;
   typedef int TriIndex;
   
   typedef int KeyFrameIndex;
   typedef int TriVertIndex;
   typedef int VertIndex;
   typedef int MorphTargetIndex;
              
   //---------------------------------------------------------------------------------
   // class Tri
   //---------------------------------------------------------------------------------
   class Tri
   {
   public:
      Tri()
      {
         clear();
      }
      
      Tri(MaterialIndex materialIndex) : mMaterialIndex(materialIndex)
      {
         Utils::ClearObj(mVertIndex);
      }
      
      Tri(MaterialIndex index, const IndexedTri& tri) : mMaterialIndex(index)
      {
         mVertIndex[0] = tri[0];
         mVertIndex[1] = tri[1];
         mVertIndex[2] = tri[2];
      }
      
      Tri(MaterialIndex index, VertIndex a, VertIndex b, VertIndex c) : mMaterialIndex(index)
      {
         mVertIndex[0] = a;
         mVertIndex[1] = b;
         mVertIndex[2] = c;
      }
                        
      void clear(void)
      {
         mMaterialIndex = cInvalidIndex;
         Utils::ClearObj(mVertIndex);
      }

      MaterialIndex materialIndex(void) const 
      { 
         return mMaterialIndex; 
      }

      void setMaterialIndex(MaterialIndex index)            
      { 
         mMaterialIndex = index; 
      }
      
      VertIndex  operator[] (TriVertIndex i) const    { return mVertIndex[debugRangeCheck(i, 3)]; }
      VertIndex& operator[] (TriVertIndex i)          { return mVertIndex[debugRangeCheck(i, 3)]; }
      
      void debugCheck(void) const
      {
      }
      
      BStream& write(BStream& dst) const
      {
         return dst << mMaterialIndex << mVertIndex[0] << mVertIndex[1] << mVertIndex[2];
      }
      
      BStream& read(BStream& src) 
      {
         return src >> mMaterialIndex >> mVertIndex[0] >> mVertIndex[1] >> mVertIndex[2];
      }

      friend BStream& operator<< (BStream& dst, const Tri& src)
      {
         return src.write(dst);
      }

      friend BStream& operator>> (BStream& src, Tri& dst)
      {
         return dst.read(src);
      }
      
      static bool MaterialIndexComp(const Tri& lhs, const Tri& rhs)
      {
         return lhs.mMaterialIndex < rhs.mMaterialIndex;
      }
      
      IndexedTri indexedTri(void) const
      {
         return IndexedTri(mVertIndex[0], mVertIndex[1], mVertIndex[2]);
      }
      
      Tri offsetted(VertIndex vertOffset) const
      {
         return Tri(mMaterialIndex, vertOffset + mVertIndex[0], vertOffset + mVertIndex[1], vertOffset + mVertIndex[2]);
      }

   protected:
      MaterialIndex mMaterialIndex;
      VertIndex mVertIndex[3];
   };
         
   //---------------------------------------------------------------------------------
   // class Transform
   //---------------------------------------------------------------------------------
   template<bool Packed, bool BigEndian>
   class Transform
   {
      friend class Transform<Packed, !BigEndian>;
      
   public:
      typedef BVecN<4, PACKED_TYPE(float)> BPackedVec4Type;
      
      enum { NumRows = 4 };
      
      Transform()
      {
         BCOMPILETIMEASSERT(sizeof(Transform) == sizeof(Transform<!Packed, BigEndian>));
      }

      Transform(const BVec4& r0, const BVec4& r1, const BVec4& r2, const BVec4& r3)
      {
         mRow[0] = r0;
         mRow[1] = r1;
         mRow[2] = r2;
         mRow[3] = r3;
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      Transform(const Transform<OtherPacked, OtherBigEndian>& other)
      {
         for (uint i = 0; i < 4; i++)
            mRow[i] = other.mRow[i];
      }
           
      template<bool OtherPacked, bool OtherBigEndian>
      Transform& operator= (const Transform<OtherPacked, OtherBigEndian>& rhs)
      {
         for (uint i = 0; i < 4; i++)
            mRow[i] = rhs.mRow[i];
         return *this;
      }
      
      bool operator== (const Transform& rhs) const
      {
         for (int i = 0; i < NumRows; i++)
            if (mRow[i] != rhs.mRow[i])
               return false;
         return true;
      }
      
      bool operator!= (const Transform& rhs) const
      {
         return !(*this == rhs);
      }

      void clear(void)
      {
         for (int i = 0; i < NumRows; i++)
            mRow[i].setZero();
      }

      const BPackedVec4Type& getRow(int i) const { return mRow[debugRangeCheck(i, NumRows)]; }

      void setRow(const BVec4& r0, const BVec4& r1, const BVec4& r2, const BVec4& r3) 
      {
         mRow[0] = r0;
         mRow[1] = r1;
         mRow[2] = r2;
         mRow[3] = r3;
      }

      BMatrix44 getMatrix(void) const
      {
         BMatrix44 m;
         m.setRow(0, mRow[0]);
         m.setRow(1, mRow[1]);
         m.setRow(2, mRow[2]);
         m.setRow(3, mRow[3]);
         return m;
      }
      
      void log(BTextDispatcher& l) const
      {
         l.printf("Matrix:\n");
         for (int i = 0; i < NumRows; i++)
         {
            mRow[i].log(l);
         }
      }

   protected:
      BPackedVec4Type mRow[NumRows];
   };
   
   //---------------------------------------------------------------------------------
   // class BUnpackedBoneType
   //---------------------------------------------------------------------------------
   template<bool Packed, bool BigEndian>
   class Bone
   {
      friend class Bone<Packed, !BigEndian>;
      
   public:
      typedef Transform<Packed, BigEndian>   TransformType;
      typedef PACKED_STRING                  StringType;
      
      Bone() : mParentIndex(cInvalidIndex)
      {
         BCOMPILETIMEASSERT(sizeof(Bone) == sizeof(Bone<!Packed, BigEndian>));
         
         mModelToBone.clear();
         mName.clear();
      }

      Bone(const char* pName, const TransformType& modelToBone, BoneIndex parentIndex = cInvalidIndex) :
         mName(pName),
         mParentIndex(parentIndex), 
         mModelToBone(modelToBone)
      {
         mName = pName;
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      Bone(const Bone<OtherPacked, OtherBigEndian>& other) :
         mName(other.mName),
         mParentIndex(other.mParentIndex),
         mModelToBone(other.mModelToBone)
      {
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      Bone& operator= (const Bone<OtherPacked, OtherBigEndian>& rhs)
      {
         mName = rhs.mName;
         mModelToBone = rhs.mModelToBone;
         mParentIndex = rhs.mParentIndex;
         return *this;         
      }  

      const StringType& name(void) const                       { return mName; }
      BoneIndex parentBoneIndex(void) const                    { return mParentIndex; }
      const TransformType& modelToBone(void) const             { return mModelToBone; }

      void setName(const StringType& name)                     { mName = name; }
      void setParentBoneIndex(BoneIndex index)                 { mParentIndex = index; }
      void setModelToBone(const TransformType& modelToBone)    { mModelToBone = modelToBone; }
      
      bool operator== (const Bone& b) const
      {
         if (mName != b.mName)
            return false;
         else if (mParentIndex != b.mParentIndex)
            return false;
         else if (mModelToBone != b.mModelToBone)
            return false;
         return true;
      }

      bool operator< (const Bone& b) const
      {
         if (mParentIndex < b.mParentIndex)
            return true;
         else if (mParentIndex == b.mParentIndex)
         {
            if (mName < b.mName)
               return true;
         }
         return false;
      }
     
      void log(BTextDispatcher& l) const
      {
         l.printf("Name: \"%s\"\n", mName.c_str());
         l.printf("Parent index: %i\n", (int)mParentIndex);
         l.printf("Model To bone Transform:\n");
         l.indent(1);
         mModelToBone.log(l);
         l.indent(-1);
      }
      
      bool pack(BPackState& state)
      {
         return mName.pack(state);
      }
      
      bool unpack(const BDataBuffer& buf)
      {
         return mName.unpack(buf);
      }

   protected:
      StringType              mName;
      TransformType           mModelToBone;
      PACKED_TYPE(BoneIndex)  mParentIndex;
   };
            
   //---------------------------------------------------------------------------------
   // TraverseBoneList
   // FIXME: Move
   //---------------------------------------------------------------------------------       
   template<class Type>
   inline void TraverseBoneList(
         const Type& bones, 
         IntVec& newToOldRemap, 
         BoneIndex curBoneIndex)
   {
      IntVec children;
      for (int i = 0; i < bones.size(); i++)
         if (bones[i].parentBoneIndex() == curBoneIndex)
            children.push_back(i);

      int numChildren = children.size();

      bool swapFlag;
      do
      {
         swapFlag = false;
         for (int i = 0; i < numChildren - 1; i++)
         {
            int& a = children.at(i);
            int& b = children.at(i + 1);
            if (bones[b] < bones[a])
            {
               std::swap(a, b);
               swapFlag = true;
               BASSERT(!(bones[b] < bones[a]));
            }
         }
      } while (swapFlag);

      for (int i = 0; i < numChildren; i++)
      {
         newToOldRemap.push_back(children[i]);
      }

      for (int i = 0; i < numChildren; i++)
         TraverseBoneList(bones, newToOldRemap, children[i]);
   }
   
   //---------------------------------------------------------------------------------
   // CanonicalizeBoneList
   // FIXME: Move
   //---------------------------------------------------------------------------------
   template<class Type>
   inline void CanonicalizeBoneList(
         const Type& bones, 
         IntVec& newToOldRemap,
         IntVec& oldToNewRemap)
   {
      const int numBones = static_cast<int>(bones.size());

      newToOldRemap.clear();
      
      TraverseBoneList(bones, newToOldRemap, -1);

      BVERIFY(newToOldRemap.size() == numBones);
      
      BDynamicArray<bool> accountedFor(numBones);
      for (int i = 0; i < numBones; i++)
      {
         const int oldIndex = newToOldRemap.at(i);
         BVERIFY(!accountedFor.at(oldIndex));
         accountedFor.at(oldIndex) = true;
      }

      oldToNewRemap.resize(numBones);
      for (int i = 0; i < numBones; i++)
         oldToNewRemap[newToOldRemap[i]] = i;
   }
   
   //---------------------------------------------------------------------------------
   // struct MorphVert
   //---------------------------------------------------------------------------------
   struct MorphVert
   {
      BVec3 p;
      BVec3 n;
      
      MorphVert() { }
      MorphVert(const BVec3& pos, const BVec3& norm) : p(pos), n(norm) { }

      friend BStream& operator<< (BStream& dst, const MorphVert& src)
      {
         return dst << src.p << src.n;
      }

      friend BStream& operator>> (BStream& src, MorphVert& dst)
      {
         return src >> dst.p >> dst.n;
      }
   };
   
   typedef BDynamicArray<MorphVert> MorphVertVec;
   
   //---------------------------------------------------------------------------------
   // class MorphTarget
   //---------------------------------------------------------------------------------
   class MorphTarget
   {
   public:
      MorphTarget(float time = 0.0f, int numVerts = 0) :
         mTime(time),
         mVerts(numVerts)
      {
      }

      void clear(void)
      {
         mTime = 0;
         mVerts.clear();
      }

      float time(void) const
      {
         return mTime;
      }

      void setTime(float time) 
      {
         mTime = time;
      }

      void resize(int numVerts)
      {
         mVerts.resize(numVerts);
      }

      void insertVert(const MorphVert& vert)
      {
         mVerts.push_back(vert);
      }

      int size(void) const
      {
         return static_cast<int>(mVerts.size());
      }

      const MorphVert& operator[] (int i) const { return mVerts[debugRangeCheck(i, size())]; }
            MorphVert& operator[] (int i)          { return mVerts[debugRangeCheck(i, size())]; }
            
      const MorphVert& vert (int i) const    { return (*this)[i]; }
            MorphVert& vert (int i)          { return (*this)[i]; }
   
      const MorphVertVec& getMorphVertVec(void) const
      {
         return mVerts;
      }

      friend BStream& operator<< (BStream& dst, const MorphTarget& src)
      {
         dst << src.mTime;
         dst.writeVec(src.mVerts);
         return dst;
      }

      friend BStream& operator>> (BStream& src, MorphTarget& dst)
      {
         src >> dst.mTime;
         src.readVec(dst.mVerts);
         return src;
      }

      // FIXME
      void debugCheck(void)
      {
      }

   private:
      float mTime;
      MorphVertVec mVerts; 
   };
         
   //---------------------------------------------------------------------------------
   // class Accessory
   // Accessories correspond to Granny models
   //---------------------------------------------------------------------------------
   template<bool Packed, bool BigEndian>
   class Accessory
   {
      friend class Accessory<Packed, !BigEndian>;
      
   public:
      typedef PACKED_ARRAY_TYPE( PACKED_TYPE(int) ) IntArrayType;
      
      Accessory()
      {
         BCOMPILETIMEASSERT(sizeof(Accessory) == sizeof(Accessory<!Packed, BigEndian>));
         clear();
      }

      ~Accessory()
      {
      }
      
      template<bool OtherPacked, bool OtherBigEndian>
      Accessory(const Accessory<OtherPacked, OtherBigEndian>& other) :
         mFirstBone(other.mFirstBone),
         mNumBones(other.mNumBones),
         mObjectIndices(other.mObjectIndices)
      {         
      }

      void clear(void)
      {
         mFirstBone = 0;
         mNumBones = 0;
         mObjectIndices.clear();
      }
           
      template<bool OtherPacked, bool OtherBigEndian>
      Accessory& operator= (const Accessory<OtherPacked, OtherBigEndian>& rhs)
      {
         mFirstBone = rhs.mFirstBone;
         mNumBones = rhs.mNumBones;
         mObjectIndices = rhs.mObjectIndices;
         return *this;
      }

      int firstBone(void) const { return mFirstBone; }
      int numBones(void) const { return mNumBones; }

      void setFirstBone(int firstBone) { mFirstBone = firstBone; }
      void setNumBones(int numBones)   { mNumBones = numBones; }

      int numObjectIndices(void) const    { return static_cast<int>(mObjectIndices.size()); }
      void insertObjectIndex(int j)       { mObjectIndices.push_back(j); }
      int setNumObjectIndices(int num)    { mObjectIndices.resize(num); }
      int objectIndex(int i) const        { return mObjectIndices[debugRangeCheck(i, numObjectIndices())]; }
      void setObjectIndex(int i, int j)   { mObjectIndices[debugRangeCheck(i, numObjectIndices())] = j; }

            IntArrayType& objectIndices(void)         { return mObjectIndices; }
      const IntArrayType& objectIndices(void) const   { return mObjectIndices; }
    
      void log(BTextDispatcher& log) const
      {
         log.printf("FirstBone: %i, NumBones: %i, ObjIndices: %i [", (int)mFirstBone, (int)mNumBones, mObjectIndices.size());
         for (uint i = 0; i < mObjectIndices.size(); i++)
            log.printf("%03i ", (int)mObjectIndices[i]);
         log.printf("]\n");
      }
      
      bool pack(BPackState& state)
      {
         return mObjectIndices.pack(state);
      }
      
      bool unpack(const BDataBuffer& buf)
      {
         return mObjectIndices.unpack(buf);
      }

   private:
      PACKED_TYPE(int)        mFirstBone;
      PACKED_TYPE(int)        mNumBones;
      
      // In a UGX file, object indices are actually section indices (i.e. sections that use this accessory/model).
      IntArrayType            mObjectIndices;
   };
         
   typedef Transform<false, cBigEndianNative>         BUnpackedTransformType;
   typedef Bone<false, cBigEndianNative>              BUnpackedBoneType;
   typedef Accessory<false, cBigEndianNative>         BUnpackedAccessoryType;
   
   typedef Transform<true, cBigEndianNative>          BNativeTransformType;
   typedef Bone<true, cBigEndianNative>               BNativeBoneType;
   typedef Accessory<true, cBigEndianNative>          BNativeAccessoryType;
      
   typedef BDynamicArray<Tri>                         TriVec;
   typedef BDynamicArray<BUnpackedTransformType>      TransformVec;
   typedef BDynamicArray<BUnpackedBoneType>           BoneVec;
   typedef BDynamicArray<BUnpackedAccessoryType>      AccessoryVec;
   typedef BDynamicArray<MorphTarget>                 MorphTargetVec;
   
   //---------------------------------------------------------------------------------
   // class Geom
   //---------------------------------------------------------------------------------      
   class Geom
   {
   public:
      typedef BArrayUnifier<Unigeom::BMaterial>          MaterialUnifier;
               
      Geom();
      ~Geom();
      
      // Construction
      void clear(void);
      
      MaterialIndex insertMaterial(const Unigeom::BMaterial& m);
      TriIndex insertTri(const Tri& t);
      VertIndex insertVert(const Univert& v);
      MorphTargetIndex insertMorphTarget(const MorphTarget& m);
      BoneIndex insertBone(const BUnpackedBoneType& b);
      AccessoryIndex insertAccessory(const BUnpackedAccessoryType& a);
                                 
      const BFixedString256& name(void) const   { return mName; }
            BFixedString256& name(void)         { return mName; }
      
      // Materials      
      int numMaterials(void) const { return mMaterials.size(); }
      const Unigeom::BMaterial& material(MaterialIndex i) const { return mMaterials[i]; }

      // Triangles
      int numTris(void) const { return static_cast<int>(mTris.size()); }
      const Tri& tri(TriIndex i) const { return mTris[i]; }
            Tri& tri(TriIndex i)       { return mTris[i]; }

      Univert triVert(TriIndex i, TriVertIndex j) const { return vert( mTris[debugRangeCheck(i, numTris())][debugRangeCheck(j, 3)] ); }
      Unitri unitri(TriIndex i) const { return Unitri(triVert(i, 0), triVert(i, 1), triVert(i, 2)); }
      BTri3 tri3(TriIndex i) const { return BTri3(triVert(i, 0).pos(), triVert(i, 1).pos(), triVert(i, 2).pos()); }
      
      void setVert(VertIndex vertIndex, const Univert& v);
                     
      void sortTrisByMaterial(void)
      {
         std::sort(mTris.begin(), mTris.end(), Tri::MaterialIndexComp);
      }
                     
      // Vertices
      uint numVerts(void) const { return mNumVerts; }
                        
      Univert vert(VertIndex i) const;
               
      BVec3 vertPos(VertIndex i) const;
      BVec3 vertNorm(VertIndex i) const;
      BVec3 vertS(VertIndex i, uint set) const;
      BVec3 vertT(VertIndex i, uint set) const;
      BVec2 vertUV(VertIndex i, uint set) const;
      void vertIndices(uint8* pIndices, VertIndex i) const;
      BVecN<Univert::MaxInfluences> vertWeights(VertIndex i) const;
      BVec4 vertDiffuse(VertIndex i) const;
      int vertIndex(VertIndex i) const;
               
      Univert& interpolatedVert(Univert& vert, uint triIndex, const BVec3& barycentric) const
      {
         vert = (triVert(triIndex, 0) * barycentric[0]) + 
                (triVert(triIndex, 1) * barycentric[1]) + 
                (triVert(triIndex, 2) * barycentric[2]);
         return vert.normalize(vertAttributes());
      }

      BVec3& interpolatedVertNorm(BVec3& norm, uint triIndex, const BVec3& barycentric) const
      {
         norm = (vertNorm(tri(triIndex)[0]) * barycentric[0]) + 
                (vertNorm(tri(triIndex)[1]) * barycentric[1]) + 
                (vertNorm(tri(triIndex)[2]) * barycentric[2]);
         return norm;
      }

      BVec3& interpolatedVertT(BVec3& t, uint triIndex, uint set, const BVec3& barycentric) const
      {
         t = (vertT(tri(triIndex)[0], set) * barycentric[0]) + 
             (vertT(tri(triIndex)[1], set) * barycentric[1]) + 
             (vertT(tri(triIndex)[2], set) * barycentric[2]);
         return t;
      }

      BVec3& interpolatedVertS(BVec3& s, uint triIndex, uint set, const BVec3& barycentric) const
      {
         s = (vertS(tri(triIndex)[0], set) * barycentric[0]) + 
             (vertS(tri(triIndex)[1], set) * barycentric[1]) + 
             (vertS(tri(triIndex)[2], set) * barycentric[2]);
         return s;
      }

      BVec2& interpolatedVertUV(BVec2& uv, uint triIndex, uint set, const BVec3& barycentric) const
      {
         uv = (vertUV(tri(triIndex)[0], set) * barycentric[0]) + 
              (vertUV(tri(triIndex)[1], set) * barycentric[1]) + 
              (vertUV(tri(triIndex)[2], set) * barycentric[2]);
         return uv;
      }                  
      // Vertex attributes
      bool lockVertAttributes(bool locked) { mVertAttributesLocked = locked; }

      const UnivertAttributes& vertAttributes(void) const { return mVertAttributes; }
      const UnivertStreamer& vertStreamer(void) const { return mVertStreamer; }

      // returns size of new vertex format
      int changeVertAttributes(const UnivertAttributes& newAttributes);
                                                  
      // Morph targets
      int numMorphTargets(void) const { return static_cast<int>(mMorphTargets.size()); }
      const MorphTarget& morphTarget(int i) const { return mMorphTargets[debugRangeCheck(i, numMorphTargets())]; }
            MorphTarget& morphTarget(int i)       { return mMorphTargets[debugRangeCheck(i, numMorphTargets())]; }

      // Triangle vector
      const TriVec& triVec(void) const { return mTris; }
            TriVec& triVec(void)       { return mTris; }
      
      // Packed vertex vector
      const UCharVec& packedVertVec(void) const  { return mPackedVerts; }
            UCharVec& packedVertVec(void)        { return mPackedVerts; }
      
      const uchar* packedVertPtr(VertIndex i) const;
            uchar* packedVertPtr(VertIndex i);
      
      // Morph target vector
      const MorphTargetVec& morphTargetVec(void) const   { return mMorphTargets; }
            MorphTargetVec& morphTargetVec(void)         { return mMorphTargets; }
      
      // Bones      
      int numBones(void) const { return static_cast<int>(mBones.size()); }
      const BUnpackedBoneType& bone(BoneIndex i) const { return mBones[i]; }

      const BoneVec& bones(void) const { return mBones; }
            BoneVec& bones(void)       { return mBones; }
      
      // Accessories
      int numAccessories(void) const { return static_cast<int>(mAccessories.size()); }
      const BUnpackedAccessoryType& accessory(int i) const   { return mAccessories[debugRangeCheck(i, numAccessories())]; }
            BUnpackedAccessoryType& accessory(int i)         { return mAccessories[debugRangeCheck(i, numAccessories())]; }

      const AccessoryVec& accessories(void) const  { return mAccessories; }
            AccessoryVec& accessories(void)        { return mAccessories; }

      enum eModelType
      { 
         cMTNormal         = 0,
         cMTDestructable   = 1,
         cMTLarge          = 2
      };
      
      eModelType getModelType(void) const { return mModelType; }
      void setModelType(eModelType modelType) { mModelType = modelType; }
            
      void log(BTextDispatcher& log) const;
                     
   private:
      BFixedString256         mName;
      eModelType              mModelType;
      
      MaterialUnifier         mMaterials;
      TriVec                  mTris;

      uint                    mNumVerts;                  
      UCharVec                mPackedVerts;
      UnivertAttributes       mVertAttributes;
      bool                    mVertAttributesLocked;
      UnivertStreamer         mVertStreamer;
      int                     mVertSize;
                        
      MorphTargetVec          mMorphTargets;
      BoneVec                 mBones;
      AccessoryVec            mAccessories;
   };
   
   typedef BDynamicArray<Geom> GeomVec;
   
} // namespace Unigeom

DEFINE_PACKABLE_TYPE(Unigeom::Bone)
DEFINE_PACKABLE_TYPE(Unigeom::Accessory)
