//============================================================================
//
//  File: D3DTextureStats.h
//  
//  Copyright (c) 2003-2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "D3DTexture.h"

const char* getD3DTexFormatString(D3DFORMAT fmt);

//============================================================================
// class BD3DTextureDesc
//============================================================================
class BD3DTextureDesc
{
public:
   BD3DTextureDesc() { clear(); }
   BD3DTextureDesc(IDirect3DBaseTexture9* pBaseTex, DWORD membershipBits) { set(pBaseTex, membershipBits); }
   
   BD3DTextureDesc(const BD3DTextureDesc& other) { *this = other; }
   BD3DTextureDesc& operator= (const BD3DTextureDesc& rhs) { memcpy(this, &rhs, sizeof(*this)); return *this; }
      
   void clear() { Utils::ClearObj(*this); }
   void set(IDirect3DBaseTexture9* pBaseTex, DWORD membershipBits);

   bool operator== (const BD3DTextureDesc& rhs) const { return (mWidth == rhs.mWidth) && (mHeight == rhs.mHeight) && (mDepth == rhs.mDepth) && (mMipLevels == rhs.mMipLevels) && (mFormat == rhs.mFormat) && (mType == rhs.mType) && (mMembershipBits == rhs.mMembershipBits); }
   bool operator!= (const BD3DTextureDesc& rhs) const { return !(*this == rhs); }
   
   bool compareResolution(const BD3DTextureDesc& rhs) const { return (mWidth == rhs.mWidth) && (mHeight == rhs.mHeight) && (mDepth == rhs.mDepth) && (mType == rhs.mType); }
   bool compareFormat(const BD3DTextureDesc& rhs) const { return canonicalizeD3DFormat(mFormat) == canonicalizeD3DFormat(rhs.mFormat); }
   bool compareCategory(const BD3DTextureDesc& rhs) const { return (mMembershipBits == rhs.mMembershipBits); }
   
   int               mWidth;
   int               mHeight;
   int               mDepth;
   int               mMipLevels;
   D3DFORMAT         mFormat;
   BD3DTextureType   mType;
   DWORD             mMembershipBits;

private:
   static D3DFORMAT canonicalizeD3DFormat(D3DFORMAT fmt) { return (D3DFORMAT)MAKENOSRGBFMT(MAKELEFMT(fmt)); }   
};   

//============================================================================
// class BD3DTextureAllocationStats
//============================================================================
class BD3DTextureAllocationStats 
{
public:
   BD3DTextureAllocationStats() { clear(); }
   
   BD3DTextureAllocationStats(const BD3DTextureAllocationStats& other) : 
      mDesc(other.mDesc),
      mTotalTextures(other.mTotalTextures),
      mTotalBaseSize(other.mTotalBaseSize),
      mTotalMipSize(other.mTotalMipSize),
      mTotalRegionSize(other.mTotalRegionSize),
      mTotalAllocationSize(other.mTotalAllocationSize),
      mTotalWastedSize(other.mTotalWastedSize),
      mTotalAllocations(other.mTotalAllocations)
   {
   }
   
   BD3DTextureAllocationStats& operator= (const BD3DTextureAllocationStats& rhs) 
   {
      mDesc = rhs.mDesc;
      mTotalTextures = rhs.mTotalTextures;
      mTotalBaseSize = rhs.mTotalBaseSize;
      mTotalMipSize = rhs.mTotalMipSize;
      mTotalRegionSize = rhs.mTotalRegionSize;
      mTotalAllocationSize = rhs.mTotalAllocationSize;
      mTotalWastedSize = rhs.mTotalWastedSize;
      mTotalAllocations = rhs.mTotalAllocations;
      return *this;
   }
   
   BD3DTextureAllocationStats(IDirect3DBaseTexture9* pTex, int totalAllocations = 1, int totalAllocationSize = -1, DWORD membershipBits = 0) { clear(); set(pTex, totalAllocations, totalAllocationSize, membershipBits); }

   void clear(void) { Utils::ClearObj(*this); }
   void set(IDirect3DBaseTexture9* pBaseTex, int totalAllocations = 1, int totalAllocationSize = -1, DWORD membershipBits = 0);

   BD3DTextureAllocationStats& operator+= (const BD3DTextureAllocationStats & rhs)
   {
      mTotalTextures       += rhs.mTotalTextures;
      mTotalBaseSize       += rhs.mTotalBaseSize;
      mTotalMipSize        += rhs.mTotalMipSize;
      mTotalRegionSize     += rhs.mTotalRegionSize;
      mTotalAllocationSize += rhs.mTotalAllocationSize;
      mTotalWastedSize     += rhs.mTotalWastedSize;
      mTotalAllocations    += rhs.mTotalAllocations;      
      
      return *this;
   }

   BD3DTextureAllocationStats& operator-= (const BD3DTextureAllocationStats & rhs)
   {
      mTotalTextures       -= rhs.mTotalTextures;
      mTotalBaseSize       -= rhs.mTotalBaseSize;
      mTotalMipSize        -= rhs.mTotalMipSize;
      mTotalRegionSize     -= rhs.mTotalRegionSize;
      mTotalAllocationSize -= rhs.mTotalAllocationSize;
      mTotalWastedSize     -= rhs.mTotalWastedSize;
      mTotalAllocations    -= rhs.mTotalAllocations;

      
      return *this;
   }
   
   BD3DTextureDesc   mDesc;
   
   int               mTotalTextures;

   int               mTotalBaseSize;
   int               mTotalMipSize;
   int               mTotalRegionSize;
         
   int               mTotalAllocationSize;
   int               mTotalWastedSize;
   int               mTotalAllocations;   
};

//============================================================================
// class BD3DTextureAllocationStatsPayload
//============================================================================
class BD3DTextureAllocationStatsPayload : public BEventPayload
{
public:
   BD3DTextureAllocationStatsPayload() { }
   BD3DTextureAllocationStatsPayload(const BD3DTextureAllocationStats& stats) : mStats(stats) { }

   virtual ~BD3DTextureAllocationStatsPayload() { }

   virtual void deleteThis(bool delivered) { delete this; }

   BD3DTextureAllocationStats mStats;
};

//============================================================================
// class BD3DTextureAllocationStatsTracker
//============================================================================
class BD3DTextureAllocationStatsTracker
{
public:
   BD3DTextureAllocationStatsTracker();
   BD3DTextureAllocationStatsTracker(const BD3DTextureAllocationStatsTracker& other);
   BD3DTextureAllocationStatsTracker& operator= (const BD3DTextureAllocationStatsTracker& other);

   enum eMergePolicy
   {
      cMPNone,
      cMPFull,
      cMPRes,
      cMPFmt,      
   };

   void setMergePolicy(eMergePolicy mergePolicy) { mMergePolicy = mergePolicy; }   
   eMergePolicy getMergePolicy() const { return mMergePolicy; }

   void setMergePolicyCategory(uint category) { mMergePolicyCategory = category; }
   uint getMergePolicyCategory() const { return mMergePolicyCategory; }

   // clear(), add() and remove() are thread safe.
   void clear();
   
   void add(const BD3DTextureAllocationStats& stats);
   bool remove(const BD3DTextureAllocationStats& stats);

   typedef BDynamicArray<BD3DTextureAllocationStats> BD3DTextureAllocationStatsArray;

   const BD3DTextureAllocationStatsArray& getStatsArray() const   { return mStats; }
         BD3DTextureAllocationStatsArray& getStatsArray()         { return mStats; }

   uint getNumStats() const { return mStats.getSize(); }
   const BD3DTextureAllocationStats& getStats(uint index) const { return mStats[index]; } 
      
   uint getNumDumpPages() const { return cNumDumpPages; }
   void dump(BConsoleOutput& consoleOutput, uint page, uint category) const;

private:
   BLightWeightMutex                mMutex;
   BD3DTextureAllocationStatsArray  mStats;
   eMergePolicy                     mMergePolicy;
   uint                             mMergePolicyCategory;
   
   enum { cNumDumpPages = 5 };
};

