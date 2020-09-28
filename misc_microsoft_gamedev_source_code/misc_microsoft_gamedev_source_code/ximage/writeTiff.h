//============================================================================
//
// File: writeTiff.h
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "colorUtils.h"

class BTIFFWriter
{
public:
   BTIFFWriter();
   ~BTIFFWriter();
      
   // numComponents may be 3 or 4
   // bitsPerComponent may be 8, 16, or 32
   bool open(BStream& stream, uint width, uint height, uint numComponents, uint bitsPerComponent);
   
   bool close(void);
   
   // pScanLine must be in RGBA order (R first).
   bool writeLine(const void* pScanLine);
   
private:
   enum eByteOrder
   {
      cINTEL_BYTE_ORDER,
      cMOTOROLA_BYTE_ORDER 
   };

   enum eTagType
   {
      cIMAGEWIDTH            = 256, /* data is a short */
      cIMAGEHEIGHT           = 257, /* data is a short */
      cBITSPERSAMPLE         = 258, /* data is a short */
      cCOMPRESSION           = 259, /* data is a short */
      cPHOTOMETRIC           = 262, /* data is short */
      cSTRIPOFFSETS          = 273, /* data is set of short or long */
      cSAMPLESPERPIXEL       = 277, /* data is short */
      cROWSPERSTRIP          = 278, /* data is short or long */
      cSTRIPBYTECOUNTS       = 279, /* data is set of short or long */
      cPLANARCONFIG          = 284, /* data is short */
      cCOLORMAP              = 320, /* data is 3 sets of shorts */
      cSAMPLEFORMAT          = 339
   };

   enum { cENC_NONE          = 1 };

   enum eTagPhoto
   {      
      cPHOTO_BIT0            = 0,
      cPHOTO_BIT1            = 1,
      cPHOTO_RGB             = 2,
      cPHOTO_PAL             = 3
   };

   enum eDataType 
   {
      cBYTE                  = 1,  /* data is unsigned 8 bit */
      cASCII                 = 2,  /* data is ASCIIZ string */
      cSHORT                 = 3,  /* data is unsigned 16 bit */
      cLONG                  = 4   /* data is unsigned 32 bit */
   };      

#pragma pack(push, 1)         
   struct BTAG
   {
      WORD mType;
      WORD mDataType;
      DWORD mLength;
      DWORD mValue;  
   };
#pragma pack(pop)   

   BStream* mpStream;
   uint mWidth;
   uint mHeight;
   uint mNumComponents;
   uint mBitsPerComponent;
   uint mBytesPerPixel;
      
   BDynamicArray<BTAG> mTags;
   BDynamicArray<BByteArray> mTagData;
         
   uint mRowsPerStrip;
   uint mTotalStrips;
   
   uint mCurRow;
   uint mCurStrip;
   
   int64 mStreamStartOfs;
   
   UIntArray mStripOffsets;
   UIntArray mStripByteCount;
   
   static uint getSizeofDataType(eDataType dataType);

   void defineTag(eTagType type, eDataType dataType, uint num, const void* pValue);

   void defineWORDTag(eTagType type, WORD val) { defineTag(type, cSHORT, 1, &val); }
   void defineDWORDTag(eTagType type, DWORD val) { defineTag(type, cLONG, 1, &val); }

   bool writeIFHIFD(void);
};