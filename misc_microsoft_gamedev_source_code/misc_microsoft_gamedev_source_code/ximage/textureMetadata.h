//============================================================================
// File: textureMetadata.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//============================================================================
#pragma once
#include "hash\bsha1.h"

namespace BTextureMetadata
{
   enum BTexType
   {
      cTexTypeUnknown,
      cTexTypeDiffuse,
      cTexTypeNormal,
      cTexTypeGloss,
      cTexTypeSpecular,
      cTexTypeOpacity,
      cTexTypePixelXForm,
      cTexTypeEmmissive,
      cTexTypeAmbientOcclusion,
      cTexTypeEnvironment,
      cTexTypeEnvironmentMask,
      cTexTypeUI,
      cTexTypeEmissivePixelXForm,
      cTexTypeEffect,
      cTexTypeDistortion,

      cNumTexTypes
   };

   extern const char* gpTexTypeSuffixes[cNumTexTypes];
   extern const char* gpTexTypeDescriptions[cNumTexTypes];

   BTexType determineTextureType(const char* pSrcFilename);

   class BMetadata
   {
   public:
      BMetadata();

      void clear(void);

      bool read(const char* pFilename);
      
      bool write(BStream& stream);

      uint mID;
      uint mFileSize;
      uint mFileCRC;
      BSHA1 mFileDigest;
      
      float mConvertToHDRScale;
            
      bool mValid          : 1;
      bool mMipmaps        : 1;
      bool mAlpha          : 1;
      bool mTilable        : 1;
      bool mConvertToHDR   : 1;
   };

} // namespace BTextureMetadata
