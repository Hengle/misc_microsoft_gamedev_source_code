//============================================================================
// File: textureMetadata.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//============================================================================
#include "xcore.h"
#include "textureMetadata.h"
#include "hash\crc.h"

namespace BTextureMetadata
{
   const char* gpTexTypeSuffixes[cNumTexTypes] =
   {
      "???",
      "_df",
      "_nm",
      "_gl",
      "_sp",
      "_op",
      "_xf",
      "_em",
      "_ao",
      "_en",
      "_rm",
      "_ui",
      "_ef",
      "_fx",
      "_ds"
   };

   const char* gpTexTypeDescriptions[cNumTexTypes] =
   {
      "Unknown",
      "Diffuse",
      "Normal",
      "Gloss",
      "Specular",
      "Opacity",
      "PixelXForm",
      "Emissive",
      "AmbientOcclusion",
      "Environment",
      "EnvironmentMask",
      "UI",
      "EmissivePixelXForm",
      "Effect",
      "Distortion"
   };   

   BTexType determineTextureType(const char* pSrcFilename)
   {
      BString filename(pSrcFilename);

      strPathRemoveExtension(filename);

      const uint len = filename.length();

      for (uint i = 0; i < cNumTexTypes; i++)
      {
         if (i == cTexTypeUnknown)
            continue;

         const uint suffixLen = strlen(gpTexTypeSuffixes[i]);

         if (len < suffixLen)
            continue;

         if (_stricmp(filename.getPtr() + len - suffixLen, gpTexTypeSuffixes[i]) == 0)
            return static_cast<BTexType>(i);
      }
      return cTexTypeUnknown;
   }

   //-------------------------------------------------

   static const uchar gTGAMetaSig[16] = { 0x45, 0x00, 0x00, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00 };
   static const uint cTGAMetaSigSize = sizeof(gTGAMetaSig);               

   //============================================================================
   // BMetadata::BMetadata
   //============================================================================
   BMetadata::BMetadata() 
   {  
      clear();
   }

   //============================================================================
   // BMetadata::clear
   //============================================================================
   void BMetadata::clear(void)
   {
      mID = 0;
      mFileSize = 0;
      mFileCRC = 0;
      mFileDigest.clear();
      mConvertToHDRScale = 1.0f;
      mValid = false;
      mMipmaps = false;
      mAlpha = false;
      mTilable = false;
      mConvertToHDR = false;
   }         

   //============================================================================
   // BMetadata::read
   //============================================================================
   bool BMetadata::read(const char* pFilename)
   {
      clear();

      FILE* pFile = NULL;
      fopen_s(&pFile, pFilename, "rb");
      if (!pFile)    
         return false;

      fseek(pFile, 0, SEEK_END);

      const uint fileSize = ftell(pFile);
      
      fseek(pFile, 0, SEEK_SET);
      
      mFileSize = fileSize;
      
      const uint cBufSize = 512;
      uchar buf[cBufSize];                       
                  
      BSHA1Gen sha1Gen;
      mFileCRC = cInitCRC32;
            
      uint bytesLeft = fileSize;
      while (bytesLeft)
      {
         const uint bytesToRead = Math::Min(cBufSize, bytesLeft);
         
         if (fread(buf, bytesToRead, 1, pFile) != 1)
            return false;
         
         mFileCRC = calcCRC32(buf, bytesToRead, mFileCRC);
         sha1Gen.update(buf, bytesToRead);
         
         bytesLeft -= bytesToRead;
      }
      
      mFileDigest = sha1Gen.finalize();
      
      const uint bufLen = Math::Min(fileSize, sizeof(buf));
      if (bufLen < 17)
      {
         fclose(pFile);
         return false;
      }

      fseek(pFile, fileSize - bufLen, SEEK_SET);      

      if (fread(buf, bufLen, 1, pFile) != 1)
      {
         fclose(pFile);
         return false;
      }

      fclose(pFile);
      pFile = NULL;

      uint bufOfs = 0;
      
      while (bufOfs <= (bufLen - cTGAMetaSigSize))
      {
         uint i;
         for (i = 0; i < cTGAMetaSigSize; i++)
         {
            BDEBUG_ASSERT((bufOfs + i) < bufLen);
            if (buf[bufOfs + i] != gTGAMetaSig[i])
               break;
         }
         if (i == cTGAMetaSigSize)
            break;

         bufOfs++;
      }

      if (bufOfs > (bufLen - cTGAMetaSigSize))
         return false;

      BFixedString<512> string;

      while ((bufOfs < bufLen) && (buf[bufOfs] != '!'))
      {
         string.appendChar(buf[bufOfs]);
         bufOfs += 4;
      }

      mValid = true;
      mMipmaps = (string.find("MIPS=1") >= 0);
      mAlpha = (string.find("ALPHA=1") >= 0);
      mTilable = (string.find("TILABLE=1") >= 0);
      mConvertToHDR = (string.find("CONVERTHDR=1") >= 0);
      
      mConvertToHDRScale = 1.0f;
      int hdrScaleOfs = string.find("HDRSCALE=");
      if (hdrScaleOfs >= 0)
      {
         mConvertToHDRScale = static_cast<float>(atof(string.getPtr() + hdrScaleOfs + 9));
         mConvertToHDRScale = Math::Clamp(mConvertToHDRScale, .00125f, 128.0f);
      }
      
      mID = 0;
      int idOfs = string.find("ID=");
      if (idOfs >= 0)
      {
         mID = atoi(string.getPtr() + idOfs + 3);
         if (mID < 0) 
            mID = 0;
      }
      
      return true;
   }

   //============================================================================
   // BMetadata::write
   //============================================================================
   bool BMetadata::write(BStream& stream)
   {
      if (stream.writeBytes(gTGAMetaSig, cTGAMetaSigSize) < cTGAMetaSigSize)
         return false;

      BFixedString256 metaString(cVarArg, "ID=%u MIPS=%i ALPHA=%i TILABLE=%i CONVERTHDR=%i HDRSCALE=%2.3f!", mID, mMipmaps, mAlpha, mTilable, mConvertToHDR, mConvertToHDRScale);

      const uint stringLen = metaString.getLen();
      for (uint i = 0; i < stringLen; i++)
      {
         if (!stream.putch((uchar)metaString.get(i)))
            return false;

         if (!stream.putch(0))
            return false;

         if (!stream.putch(0))
            return false;

         if (!stream.putch(0))
            return false;
      }

      return true;
   }

} // namespace BTextureMetadata
    