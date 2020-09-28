//============================================================================
//
//  File: textureStats.cpp
//  
//  Copyright (c) 2003-2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "D3DTextureStats.h"
#include "renderDraw.h"

//============================================================================
// BD3DTextureDesc::set
//============================================================================
void BD3DTextureDesc::set(IDirect3DBaseTexture9* pBaseTex, DWORD membershipBits)
{
   XGTEXTURE_DESC texDesc;
   XGGetTextureDesc(pBaseTex, 0, &texDesc);
   
   mWidth = texDesc.Width;
   mHeight = texDesc.Height;
   mDepth = texDesc.Depth;
   mFormat = texDesc.Format;
   mMipLevels = pBaseTex->GetLevelCount();;
   
   switch (texDesc.ResourceType)
   {
      case D3DRTYPE_LINETEXTURE:
      case D3DRTYPE_TEXTURE:
      {
         mType = cTT2D;
         break;
      }
      case D3DRTYPE_VOLUME:
      case D3DRTYPE_VOLUMETEXTURE:
      {
         mType = cTTVolume;
         break;
      }
      case D3DRTYPE_CUBETEXTURE:
      {
         mType = cTTCubemap;
         break;
      }
      case D3DRTYPE_ARRAYTEXTURE:
      {
         mType = cTTArray;
         break;
      }
      default:
      {
         mType = cTTInvalid;
         break;
      }
   }

   mMembershipBits = membershipBits;
}

//============================================================================
// BD3DTextureAllocationStats::set
//============================================================================
void BD3DTextureAllocationStats::set(IDirect3DBaseTexture9* pBaseTex, int totalAllocations, int totalAllocationSize, DWORD membershipBits)
{
   clear();
   
   if (!pBaseTex)
      return;

   mDesc.set(pBaseTex, membershipBits);
   
   mTotalTextures = 1;
   mTotalAllocations = totalAllocations;
   
   XGTEXTURE_DESC texDesc;
   XGGetTextureDesc(pBaseTex, 0, &texDesc);
   const uint numMipLevels = pBaseTex->GetLevelCount();
        
   const bool packedMips = (texDesc.Flags & XGTDESC_PACKED) != 0;
   const bool bordered = (texDesc.Flags & XGTDESC_BORDERED) != 0;
   const uint packedMipsBaseLevel = XGGetMipTailBaseLevel(texDesc.Width, texDesc.Height, bordered);
   for (uint i = 0; i < numMipLevels; i++)
   {
      if ((!packedMips) || (i < packedMipsBaseLevel))
      {
         XGTEXTURE_DESC mipDesc;
         XGGetTextureDesc(pBaseTex, i, &mipDesc);

         if (!i)
            mTotalBaseSize += mipDesc.SlicePitch;
         else
            mTotalMipSize += mipDesc.SlicePitch;
      }
      else
      {
         XGMIPTAIL_DESC mipTailDesc;
         XGGetMipTailDesc(pBaseTex, &mipTailDesc);

         if (!i)
            mTotalBaseSize += mipTailDesc.SlicePitch;
         else  
            mTotalMipSize += mipTailDesc.SlicePitch;

         break;
      }
   }

   UINT baseDataOfs = 0, baseAllocSize = 0, mipDataOfs = 0, mipAllocSize = 0;
   XGLAYOUT_REGION baseRegions[64];
   XGLAYOUT_REGION mipRegions[64];

   Utils::ClearObj(baseRegions);
   Utils::ClearObj(mipRegions);

   UINT baseRegionCount = 64, mipRegionCount = 64;

   BRenderDraw::getTextureLayout(
      pBaseTex,
      &baseDataOfs,
      &baseAllocSize,
      baseRegions,
      &baseRegionCount,
      4,//UINT BaseRegionAlignment,
      &mipDataOfs,
      &mipAllocSize,
      mipRegions,
      &mipRegionCount,
      4);      
      
   mTotalBaseSize = baseAllocSize;
   mTotalMipSize = mipAllocSize;      
   
   if (totalAllocationSize < 0)
      mTotalAllocationSize = Utils::AlignUpValue(mTotalBaseSize, 4096) + Utils::AlignUpValue(mTotalMipSize, 4096);
   else
   {
      mTotalAllocationSize = totalAllocationSize;
      BDEBUG_ASSERT((int)mTotalAllocationSize >= (int)(baseAllocSize + mipAllocSize));
   }

   uint totalBaseRegionSize = 0;
   for (uint i = 0; i < baseRegionCount; i++)
      totalBaseRegionSize += (baseRegions[i].EndOffset - baseRegions[i].StartOffset);

   uint totalMipRegionSize = 0;         
   for (uint i = 0; i < mipRegionCount; i++)
      totalMipRegionSize += (mipRegions[i].EndOffset - mipRegions[i].StartOffset);      
      
   const uint totalRegionSize = totalBaseRegionSize + totalMipRegionSize;      
   
   mTotalRegionSize = totalRegionSize;

   mTotalWastedSize = mTotalAllocationSize - totalRegionSize;
}

//============================================================================
// BD3DTextureAllocationStatsTracker::BD3DTextureAllocationStatsTracker
//============================================================================
BD3DTextureAllocationStatsTracker::BD3DTextureAllocationStatsTracker() :
   mMergePolicy(cMPFull),
   mMergePolicyCategory(0)
{
}

//============================================================================
// BD3DTextureAllocationStatsTracker::BD3DTextureAllocationStatsTracker
//============================================================================
BD3DTextureAllocationStatsTracker::BD3DTextureAllocationStatsTracker(const BD3DTextureAllocationStatsTracker& other) :
   mMergePolicy(other.mMergePolicy),
   mMergePolicyCategory(other.mMergePolicyCategory),
   mStats(other.mStats)
{
}

//============================================================================
// BD3DTextureAllocationStatsTracker::operator=
//============================================================================
BD3DTextureAllocationStatsTracker& BD3DTextureAllocationStatsTracker::operator= (const BD3DTextureAllocationStatsTracker& other)
{
   mMergePolicy = other.mMergePolicy;
   mMergePolicyCategory = other.mMergePolicyCategory;
   mStats = other.mStats;
   return *this;
}

//============================================================================
// BD3DTextureAllocationStatsTracker::clear
//============================================================================
void BD3DTextureAllocationStatsTracker::clear()
{
   BScopedLightWeightMutex lock(mMutex);

   mStats.clear();   
}

//============================================================================
// BD3DTextureAllocationStatsTracker::add
//============================================================================
void BD3DTextureAllocationStatsTracker::add(const BD3DTextureAllocationStats& stats)
{
   BScopedLightWeightMutex lock(mMutex);

   if (mMergePolicyCategory > 0)
      if (mMergePolicyCategory != stats.mDesc.mMembershipBits)
         return;

   for (uint i = 0; i < mStats.getSize(); i++)
   {
      bool equal = false;      
      switch (mMergePolicy)
      {
         case cMPNone: equal = true; break;      
         case cMPFull: equal = (mStats[i].mDesc == stats.mDesc); break;
         case cMPRes: equal = mStats[i].mDesc.compareResolution(stats.mDesc); break;
         case cMPFmt: equal = mStats[i].mDesc.compareFormat(stats.mDesc); break;           
      }
      
      if (equal)
      {
         mStats[i] += stats;
         return;
      }
   }   
   
   mStats.pushBack(stats);
}

//============================================================================
// BD3DTextureAllocationStatsTracker::remove
//============================================================================
bool BD3DTextureAllocationStatsTracker::remove(const BD3DTextureAllocationStats& stats)
{
   BScopedLightWeightMutex lock(mMutex);

   if (mMergePolicyCategory > 0)
      if (mMergePolicyCategory != stats.mDesc.mMembershipBits)
         return false;

   for (uint i = 0; i < mStats.getSize(); i++)
   {      
      bool equal = false;
      switch (mMergePolicy)
      {
         case cMPNone: equal = true; break;
         case cMPFull: equal = (mStats[i].mDesc == stats.mDesc); break;
         case cMPRes: equal = mStats[i].mDesc.compareResolution(stats.mDesc); break;
         case cMPFmt: equal = mStats[i].mDesc.compareFormat(stats.mDesc); break;
      }

      if (equal)
      {
         mStats[i] -= stats;
         return true;
      }
   }   
   
   return false;
}

//==============================================================================
// getD3DTexFormatString
//==============================================================================
const char* getD3DTexFormatString(D3DFORMAT fmt)
{
   switch (MAKENOSRGBFMT(MAKELEFMT(fmt)))
   {
      case MAKELEFMT(D3DFMT_A8):                return "A8";
      case MAKELEFMT(D3DFMT_A8R8G8B8):          return "A8R8G8B8";
      case MAKELEFMT(D3DFMT_A8B8G8R8):          return "A8B8G8R8";
      case MAKELEFMT(D3DFMT_R5G6B5):            return "R5G6B5";
      case MAKELEFMT(D3DFMT_A16B16G16R16F):     return "A16B16G16R16F";
      case MAKELEFMT(D3DFMT_DXT1):              return "DXT1";
      case MAKELEFMT(D3DFMT_DXT3):              return "DXT3";
      case MAKELEFMT(D3DFMT_DXT5):              return "DXT5";
      case MAKELEFMT(D3DFMT_DXN):               return "DXN";
      case MAKELEFMT(D3DFMT_LIN_A8):            return "LINA8";
      case MAKELEFMT(D3DFMT_LIN_A8R8G8B8):      return "LINA8R8G8B8";
      case MAKELEFMT(D3DFMT_LIN_R5G6B5):        return "LINR5G6B5";
      case MAKELEFMT(D3DFMT_LIN_A16B16G16R16F): return "LINA16B16G16R16F";
      case MAKELEFMT(D3DFMT_LIN_DXT1):          return "LINDXT1";
      case MAKELEFMT(D3DFMT_LIN_DXT3):          return "LINDXT3";
      case MAKELEFMT(D3DFMT_LIN_DXT5):          return "LINDXT5";
      case MAKELEFMT(D3DFMT_LIN_DXN):           return "LINDXN";
      default:                                  return "?";
   }
}

namespace
{
     
   //==============================================================================
   // getTexTypeString
   //==============================================================================
   static const char* getTexTypeString(BD3DTextureType type)
   {
      switch (type)
      {
         case cTT2D:       return "2D";
         case cTTCubemap:  return "CUBE";
         case cTTVolume:   return "VOLUME";
         case cTTArray:    return "ARRAY";
         default:          return "?";         
      }         
   }
   
   //==============================================================================
   // sortByRes
   //==============================================================================
   static bool sortByRes(const BD3DTextureAllocationStats& lhs, const BD3DTextureAllocationStats& rhs)
   {
      const uint lhsSize = lhs.mDesc.mWidth * lhs.mDesc.mHeight;
      const uint rhsSize = rhs.mDesc.mWidth * rhs.mDesc.mHeight;
      return lhsSize < rhsSize;
   }
}

//============================================================================
// BD3DTextureAllocationStatsTracker::dump
//============================================================================
void BD3DTextureAllocationStatsTracker::dump(BConsoleOutput& consoleOutput, uint page, uint category) const
{
   if (mStats.isEmpty())
      return;
   
   BD3DTextureAllocationStatsTracker allStats;
   allStats.setMergePolicy(cMPNone);
   allStats.setMergePolicyCategory(category);

   for (uint i = 0; i < mStats.getSize(); i++)
   {
      const BD3DTextureAllocationStats& texStats = mStats[i];
      allStats.add(texStats);
   }  
   
   const BD3DTextureAllocationStats& totalStats = allStats.getStats(0);       
   
   consoleOutput.printf("Totals: Textures: %u, Allocations: %u, Allocation Size: %u", totalStats.mTotalTextures, totalStats.mTotalAllocations, totalStats.mTotalAllocationSize);
   consoleOutput.printf("        Base Size: %u, Mip Size: %u, Region Size: %u, Wasted Size: %u", totalStats.mTotalBaseSize, totalStats.mTotalMipSize, totalStats.mTotalRegionSize, totalStats.mTotalWastedSize);
            
   if ((page == 6) || (page >= 7))
   {
      BD3DTextureAllocationStatsTracker resStats;
      resStats.setMergePolicy(cMPRes);
      resStats.setMergePolicyCategory(category);
      
      BD3DTextureAllocationStatsTracker fmtStats;
      fmtStats.setMergePolicy(cMPFmt);
      fmtStats.setMergePolicyCategory(category);
      
      for (uint i = 0; i < mStats.getSize(); i++)
      {
         const BD3DTextureAllocationStats& texStats = mStats[i];
         
         resStats.add(texStats);
         fmtStats.add(texStats);
      }         
            
      if (!resStats.getStatsArray().isEmpty())
         std::sort(resStats.getStatsArray().getPtr(), resStats.getStatsArray().getPtr() + resStats.getStatsArray().getSize(), sortByRes);
      
      if (page == 6)
      {
         consoleOutput.printf("Statistics by resolution:");
         
         for (uint i = 0; i < resStats.getNumStats(); i++)
         {
            const BD3DTextureAllocationStats& texStats = resStats.getStats(i);

            consoleOutput.printf("%u. %ux%ux%u, Type: %s, Total Textures: %u, Allocations: %u (%2.1f%%), Allocation Size: %u (%2.1f%%)",
               i,
               texStats.mDesc.mWidth,
               texStats.mDesc.mHeight,
               texStats.mDesc.mDepth,
               getTexTypeString(texStats.mDesc.mType),
               
               texStats.mTotalTextures,
               texStats.mTotalAllocations, texStats.mTotalAllocations*100.0f/totalStats.mTotalAllocations,
               texStats.mTotalAllocationSize,
               texStats.mTotalAllocationSize*100.0f/totalStats.mTotalAllocationSize);

            if (texStats.mTotalAllocationSize)
            {
               consoleOutput.printf("   Base Size: %u (%2.1f%%), Mip Size: %u (%2.1f%%), Wasted Size: %u (%2.1f%%)",
                  texStats.mTotalBaseSize, texStats.mTotalBaseSize*100.0f/texStats.mTotalAllocationSize,
                  texStats.mTotalMipSize, texStats.mTotalMipSize*100.0f/texStats.mTotalAllocationSize,
                  texStats.mTotalWastedSize, texStats.mTotalWastedSize*100.0f/texStats.mTotalAllocationSize);
            }            
         }      
      }         

      if (page == 7)
      {
         consoleOutput.printf("Statistics by format:");

         for (uint i = 0; i < fmtStats.getNumStats(); i++)
         {
            const BD3DTextureAllocationStats& texStats = fmtStats.getStats(i);

            consoleOutput.printf("%u. Fmt: %s, BPP: %u, Total Textures: %u, Allocations: %u (%2.1f%%), Allocation Size: %u (%2.1f%%)",
               i,
               getD3DTexFormatString(texStats.mDesc.mFormat), 
               XGBitsPerPixelFromFormat(texStats.mDesc.mFormat),
               texStats.mTotalTextures,
               texStats.mTotalAllocations, texStats.mTotalAllocations*100.0f/totalStats.mTotalAllocations,
               texStats.mTotalAllocationSize,
               texStats.mTotalAllocationSize*100.0f/totalStats.mTotalAllocationSize);

            if (texStats.mTotalAllocationSize)
            {
               consoleOutput.printf("   Base Size: %u (%2.1f%%), Mip Size: %u (%2.1f%%), Wasted Size: %u (%2.1f%%)",
                  texStats.mTotalBaseSize, texStats.mTotalBaseSize*100.0f/texStats.mTotalAllocationSize,
                  texStats.mTotalMipSize, texStats.mTotalMipSize*100.0f/texStats.mTotalAllocationSize,
                  texStats.mTotalWastedSize, texStats.mTotalWastedSize*100.0f/texStats.mTotalAllocationSize);
            }            
         }      
      }         
   }
   else
   {
      uint firstIndex = (mStats.getSize()*page)/6;
      uint lastIndex = (mStats.getSize()*(page+1))/6;
      
      for (uint i = firstIndex; i < lastIndex; i++)
      {
         const BD3DTextureAllocationStats& texStats = mStats[i];

         if (category > 0 && texStats.mDesc.mMembershipBits != category)
            continue;

         consoleOutput.printf("%u. %ux%ux%u, Mips: %u, Fmt: %s (0x%X), BPP: %u, Type: %s",
            i,
            texStats.mDesc.mWidth,
            texStats.mDesc.mHeight,
            texStats.mDesc.mDepth,
            texStats.mDesc.mMipLevels,
            getD3DTexFormatString(texStats.mDesc.mFormat), texStats.mDesc.mFormat,
            XGBitsPerPixelFromFormat(texStats.mDesc.mFormat),
            getTexTypeString(texStats.mDesc.mType));

         if (totalStats.mTotalAllocations)
         {
            consoleOutput.printf("  Total Textures: %u, Allocations: %u (%2.1f%%), Allocation Size: %u (%2.1f%%)",
               texStats.mTotalTextures,
               texStats.mTotalAllocations, texStats.mTotalAllocations*100.0f/totalStats.mTotalAllocations,
               texStats.mTotalAllocationSize,
               texStats.mTotalAllocationSize*100.0f/totalStats.mTotalAllocationSize);
         }            

         if (texStats.mTotalAllocationSize)
         {
            consoleOutput.printf("   Base Size: %u (%2.1f%%), Mip Size: %u (%2.1f%%), Wasted Size: %u (%2.1f%%)",
               texStats.mTotalBaseSize, texStats.mTotalBaseSize*100.0f/texStats.mTotalAllocationSize,
               texStats.mTotalMipSize, texStats.mTotalMipSize*100.0f/texStats.mTotalAllocationSize,
               texStats.mTotalWastedSize, texStats.mTotalWastedSize*100.0f/texStats.mTotalAllocationSize);
         }            
      }      
   }      
}

















