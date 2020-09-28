// File: grannyVertexConverter.cpp
#include "xgeom.h"
#include "grannyVertexConverter.h"
#include "granny.h"

//-------------------------------------------------------------------------------------------------------
// BGrannyVertexConverter::BGrannyVertexConverter
//-------------------------------------------------------------------------------------------------------
BGrannyVertexConverter::BGrannyVertexConverter() 
{
   clear();
}

//-------------------------------------------------------------------------------------------------------
// BGrannyVertexConverter::clear
//-------------------------------------------------------------------------------------------------------
void BGrannyVertexConverter::clear(void)
{
   mInitialized = false;

   mPosOfs = -1;
   mNormOfs = -1;
   mTangentOfs = -1;
   mBinormalOfs = -1;
   mNumUVCoords = 0;
   mIndicesOfs = -1;
   mWeightsOfs = -1;
   mNumBoneIndices = 0;
   mNumUVCoords = 0;
   mSize = 0;    
   for (int i = 0; i < Univert::MaxUVCoords; i++)
      mUVOfs[i] = -1;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyVertexConverter::BGrannyVertexConverter
//-------------------------------------------------------------------------------------------------------
BGrannyVertexConverter::BGrannyVertexConverter(const granny_data_type_definition* pDataType, bool allowSkinning, bool allowTangentNormal) :
   mInitialized(false)
{
   init(pDataType, allowSkinning, allowTangentNormal);
}

//-------------------------------------------------------------------------------------------------------
// BGrannyVertexConverter::init
//-------------------------------------------------------------------------------------------------------
bool BGrannyVertexConverter::init(const granny_data_type_definition* pDataType, bool allowSkinning, bool allowTangentNormal)
{
   clear();

   const int grannyObjSize = GrannyGetTotalObjectSize(pDataType);
      
   int curOfs = 0;

   while (GrannyEndMember != pDataType->Type)
   {  
      if ((_stricmp(pDataType->Name, GrannyVertexPositionName) == 0) && (mPosOfs == -1))
      {
         if (!(pDataType->Type == GrannyReal32Member && pDataType->ArrayWidth == 3))
            return false;

         mPosOfs = curOfs;
      }
      else if ((_stricmp(pDataType->Name, GrannyVertexNormalName) == 0) && (mNormOfs == -1))
      {
         if (!(pDataType->Type == GrannyReal32Member && pDataType->ArrayWidth == 3))
            return false;

         mNormOfs = curOfs;
      }
      else if ((_stricmp(pDataType->Name, GrannyVertexTangentName) == 0) && (mTangentOfs == -1))
      {
         if (allowTangentNormal)
         {
            if (!(pDataType->Type == GrannyReal32Member && pDataType->ArrayWidth == 3))
               return false;

            mTangentOfs = curOfs;
         }
      }
      else if ((_stricmp(pDataType->Name, GrannyVertexBinormalName) == 0) && (mBinormalOfs == -1))
      {
         if (allowTangentNormal)
         {
            if (!(pDataType->Type == GrannyReal32Member && pDataType->ArrayWidth == 3))
               return false;

            mBinormalOfs = curOfs;
         }
      }
      else if ((_stricmp(pDataType->Name, GrannyVertexBoneIndicesName) == 0) && (mIndicesOfs == -1))
      {
         if (!(pDataType->Type == GrannyUInt8Member && ((pDataType->ArrayWidth == 1) || (pDataType->ArrayWidth == 4))))
            return false;

         if (allowSkinning)
         {
            mNumBoneIndices = pDataType->ArrayWidth;
            if (mNumBoneIndices > Univert::MaxInfluences)
               return false;

            mIndicesOfs = curOfs;
         }
      }
      else if ((_stricmp(pDataType->Name, GrannyVertexBoneWeightsName) == 0) && (mWeightsOfs == -1))
      {
         if (!(
            ((pDataType->Type == GrannyUInt8Member) || (pDataType->Type == GrannyNormalUInt8Member)) && 
            ((pDataType->ArrayWidth == 1) || (pDataType->ArrayWidth == 4))
            ))
            return false;

         if (mNumBoneIndices)
         {
            if (!(mNumBoneIndices == pDataType->ArrayWidth))
               return false;
         }

         if (allowSkinning)
         {
            mNumBoneIndices = pDataType->ArrayWidth;
            if (mNumBoneIndices > Univert::MaxInfluences)
               return false;

            mWeightsOfs = curOfs;
         }
      }
      else if (_strnicmp(pDataType->Name, GrannyVertexTextureCoordinatesName, strlen(GrannyVertexTextureCoordinatesName)) == 0)
      {
         if (!(pDataType->Type == GrannyReal32Member && pDataType->ArrayWidth == 2))
            return false;

         const int uvChannel = pDataType->Name[strlen(pDataType->Name) - 1] - '0';
         if (uvChannel < Univert::MaxUVCoords)
         {
            mUVOfs[uvChannel] = curOfs;
            mNumUVCoords = Math::Max(mNumUVCoords, uvChannel + 1);
         }
      }

      curOfs += GrannyGetMemberTypeSize(pDataType);

      pDataType++;
   }

   mSize = curOfs;
   
   if (curOfs != grannyObjSize)
      return false;

   if (-1 == mPosOfs)
      return false;

   if (mNumBoneIndices)
   {
      if ((-1 == mWeightsOfs) || (-1 == mIndicesOfs))
         return false;           
   }

   for (int i = 0; i < mNumUVCoords; i++)
   {
      if (-1 == mUVOfs[i])
         return false;
   }

   mInitialized = true;
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyVertexConverter::getVert
//-------------------------------------------------------------------------------------------------------
Univert BGrannyVertexConverter::getVert(const void* p, int vertexIndex) const
{
   BDEBUG_ASSERT(mInitialized);
   
   const uint8* pData = reinterpret_cast<const uint8*>(p) + vertexIndex * getSize();

   Univert ret;
   ret.clear();

   if (!mInitialized)
      return ret;

   ret.p = readObj< BVec3 >(pData, mPosOfs, 0);
   ret.n = readObj< BVec3 >(pData, mNormOfs, 0);

   if (!ret.n.isValid())
      ret.n = BVec3(0, 0, 1);
   else
   {
      ret.n.tryNormalize();
      if (ret.n.len() <= .99f)
         ret.n = BVec3(0, 0, 1);
   }         

   if (mNumBoneIndices)
   {
      int weightSum = 0;
      for (int i = 0; i < mNumBoneIndices; i++)
      {
         ret.indices[i] = readObj<uint8>(pData, mIndicesOfs, i);

         int scaledWeight = readObj<uint8>(pData, mWeightsOfs, i);
         weightSum += scaledWeight;
         ret.weights[i] = scaledWeight / 255.0f;

         if (0 == ret.weights[i])
            ret.indices[i] = ret.indices[0];
      }

      BASSERT(255 == weightSum);
   }

   if (mNumUVCoords)               
   {
      for (int i = 0; i < mNumUVCoords; i++)
      {
         ret.uv[i] = readObj< BVecN<2> >(pData, mUVOfs[i], 0);

         if (!Math::IsValidFloat(ret.uv[i][0])) ret.uv[i][0] = 0.0f;
         if (!Math::IsValidFloat(ret.uv[i][1])) ret.uv[i][1] = 0.0f;
      }
   }

   if (-1 != mTangentOfs)
   {
      ret.t[0] = readObj< BVec3 >(pData, mTangentOfs, 0);

      if (!ret.t[0].isValid())
         ret.t[0] = BVec3(1, 0, 0);
      else
      {
         ret.t[0].tryNormalize();
         if (ret.t[0].len() <= .99f)
            ret.t[0] = BVec3(1, 0, 0);
      }
   }

   if (-1 != mBinormalOfs)
   {
      ret.s[0] = readObj< BVec3 >(pData, mBinormalOfs, 0);

      if (!ret.s[0].isValid())
         ret.s[0] = BVec3(0, 1, 0);
      else
      {
         ret.s[0].tryNormalize();
         if (ret.s[0].len() <= .99f)
            ret.s[0] = BVec3(0, 1, 0);
      }
   }

   return ret;
}
