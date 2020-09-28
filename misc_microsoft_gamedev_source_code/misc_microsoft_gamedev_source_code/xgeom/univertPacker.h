// File: univert_packer.h
// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once

#include "vertexElement.h"
#include "univert.h"
#include "utils\packedString.h"
#include "memory\memStack.h"

template<bool Packed, bool BigEndian>
class UnivertPacker
{
   friend class UnivertPacker<Packed, !BigEndian>;
   friend class UnivertPacker<!Packed, BigEndian>;
   friend class UnivertPacker<!Packed, !BigEndian>;
   
public:
   enum { cMaxUV = 8 };
   
   UnivertPacker();
  
   template<bool OtherPacked, bool OtherBigEndian>
   UnivertPacker(const UnivertPacker<OtherPacked, OtherBigEndian>& other)
   {
      *this = other;
   }
   
   template<bool OtherPacked, bool OtherBigEndian>  
   UnivertPacker& operator= (const UnivertPacker<OtherPacked, OtherBigEndian>& rhs)
   {
      mPos = rhs.mPos;             
      mBasis = rhs.mBasis;           
      mBasisScales = rhs.mBasisScales;     
      mTangent = rhs.mTangent;         
      mNorm = rhs.mNorm;            
      for (int i = 0; i < cMaxUV; i++)
         mUV[i] = rhs.mUV[i];
      mIndices = rhs.mIndices;         
      mWeights = rhs.mWeights;         
      mDiffuse = rhs.mDiffuse;         
      mIndex = rhs.mIndex;           

      mPackOrder = rhs.mPackOrder;   
      mDeclOrder = rhs.mDeclOrder; 

      return *this;  
   }
         
   void clear(void);
      
   bool operator== (const UnivertPacker& rhs) const;
   bool operator!= (const UnivertPacker& rhs) const { return !(*this == rhs); }
   
   VertexElement::EType pos(void) const            { return mPos; }
   VertexElement::EType basis(void) const          { return mBasis; }
   VertexElement::EType basisScales(void) const    { return mBasisScales; }
   VertexElement::EType tangent(void) const        { return mTangent; }   
   VertexElement::EType norm(void) const           { return mNorm; }
   VertexElement::EType UV(uint index = 0) const   { return mUV[debugRangeCheck<uint, uint>(index, cMaxUV)]; }
   VertexElement::EType indices(void) const        { return mIndices; }
   VertexElement::EType weights(void) const        { return mWeights; }
   VertexElement::EType diffuse(void) const        { return mDiffuse; }
   VertexElement::EType index(void) const          { return mIndex; }
   const char* packOrder(void) const               { return mPackOrder; }
   const char* declOrder(void) const               { return mDeclOrder; }
   
   void setPos(VertexElement::EType pos)                 { mPos = pos; }
   void setBasis(VertexElement::EType basis)             { mBasis = basis; }
   void setBasisScales(VertexElement::EType basisScales) { mBasisScales = basisScales; }
   void setTangent(VertexElement::EType tangent)         { mTangent = tangent; }   
   void setNorm(VertexElement::EType norm)               { mNorm = norm; }
   void setUV(VertexElement::EType UV)                   { for (uint i = 0; i < cMaxUV; i++) mUV[i] = UV; }
   void setUV(VertexElement::EType UV, uint index)       { mUV[debugRangeCheck<uint, uint>(index, cMaxUV)] = UV; }
   void setIndices(VertexElement::EType indices)         { mIndices = indices; }
   void setWeights(VertexElement::EType weights)         { mWeights = weights; }
   void setDiffuse(VertexElement::EType diffuse)         { mDiffuse = diffuse; }
   void setIndex(VertexElement::EType index)             { mIndex = index; }
   
   bool empty(void) const;
      
   void log(BTextDispatcher& log) const;
   
   void* pack(void* pDst, const Univert& vert, bool bigEndian = true) const;
      
   struct ElementStats
   {
      int numPos;
      int numBasis;
      int numTangent;
      int numBasisScales;
      int numNorm;
      int numTexCoords;
      int numSkin;
      int numDiffuse;
      int numIndex;
      
      ElementStats()
      {
         Utils::ClearObj(*this);
      }
   };
   
   int countVertexElements(EVertElementSpec spec) const;
      
   ElementStats getStats(void) const;
      
   int size(void) const;
      
   void setPackOrder(const char* pPackOrder);
      
   void setDeclOrder(const char* pDeclOrder);
   
   // The decl order string is canonicalized to contain indices after each specification, to simplify vertex decl creation.
   // This must be called before attempting to create a decl through the vertex decl manager.               
   void setDeclOrder(
      int firstPos = 0,
      const IntVec& basisMap = IntVec(),
      const IntVec& basisScalesMap = IntVec(),
      int firstNorm = 0,
      const IntVec& uvMap = IntVec(),
      int firstSkin = 0,
      int firstDiffuse = 0,
      int firstIndex = 0);
      
   void setPackOrder(const UnivertAttributes& usedAttr, const char* pPackOrder = NULL);
      
   bool pack(BPackState& state)
   {
      if (!mPackOrder.pack(state))
         return false;
      if (!mDeclOrder.pack(state))
         return false;
      return true;
   }
   
   bool unpack(const BDataBuffer& buf)
   {
      if (!mPackOrder.unpack(buf))
         return false;
      if (!mDeclOrder.unpack(buf))
         return false;
      return true;
   }
   
protected:
   PACKED_STRING                       mPackOrder;   
   PACKED_STRING                       mDeclOrder;   
   
   PACKED_TYPE(VertexElement::EType)   mPos;             // written as (x,y,z,1)
   PACKED_TYPE(VertexElement::EType)   mBasis;           // written as (x,y,z,s),(x,y,z,s) where s is scale
   PACKED_TYPE(VertexElement::EType)   mBasisScales;     // written as (ts,bs,0,0)
   PACKED_TYPE(VertexElement::EType)   mTangent;         // written as (x,y,z,s) where s is scale   
   
   PACKED_TYPE(VertexElement::EType)   mNorm;            // written as (x,y,z,0)
   PACKED_TYPE(VertexElement::EType)   mUV[cMaxUV];      // written as (x,y,z,w)
   PACKED_TYPE(VertexElement::EType)   mIndices;         // written as (a,b,c,d)
   PACKED_TYPE(VertexElement::EType)   mWeights;         // written as (a,b,c,d)
   
   PACKED_TYPE(VertexElement::EType)   mDiffuse;         // written as (r,g,b,a)
   PACKED_TYPE(VertexElement::EType)   mIndex;           // written as (i,0,0,0)
   
   //uchar                               mPadding[2];
   
}; // struct UnivertPacker

DEFINE_PACKABLE_TYPE(UnivertPacker);

typedef UnivertPacker<false, cBigEndianNative>  BUnpackedUnivertPackerType;
typedef UnivertPacker<true, cBigEndianNative>   BNativeUnivertPackerType;

#include "univertPacker.inl"
