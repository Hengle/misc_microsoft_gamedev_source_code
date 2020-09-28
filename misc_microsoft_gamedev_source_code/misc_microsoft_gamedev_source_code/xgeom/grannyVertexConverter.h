// File: grannyVertexConverter.h
#pragma once

#include "unigeom.h"

struct granny_data_type_definition;

//-------------------------------------------------------------------------------------------------------
// Converts Granny verts to Univert's
//-------------------------------------------------------------------------------------------------------
class BGrannyVertexConverter
{
public:
   BGrannyVertexConverter();
   
   BGrannyVertexConverter(const granny_data_type_definition* pDataType, bool allowSkinning = true, bool allowTangentNormal = true);

   bool init(const granny_data_type_definition* pDataType, bool allowSkinning = true, bool allowTangentNormal = true);

   bool getInitialized(void) const { return mInitialized; }

   int getSize(void) const { BDEBUG_ASSERT(mInitialized); return mSize; }
   bool getHasIndices(void) const { BDEBUG_ASSERT(mInitialized); return mNumBoneIndices > 0; }
   
   Univert getVert(const void* p, int vertexIndex) const;

private: 
   bool mInitialized;
   
   int mSize;
   int mPosOfs;
   int mNormOfs;
   int mTangentOfs;
   int mBinormalOfs;
   int mIndicesOfs;
   int mWeightsOfs;
   int mNumBoneIndices;
   int mUVOfs[Univert::MaxUVCoords];
   int mNumUVCoords;

   template<typename T> static T readObj(const uint8* pData, int ofs, int index) { return *reinterpret_cast<const T*>(pData + ofs + index * sizeof(T)); }

   void clear(void);

}; // class BGrannyVertexConverter
