//------------------------------------------------------------------------------------------------------------------------
//  File: grannyDataEndianSwapper.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "granny\win32\granny.h"

class BGrannyDataEndianSwapper
{
public:   
   BGrannyDataEndianSwapper(void* pSrc, void* pDst, uint bufSize, granny_data_type_definition* pDataTypeDef);

private:
   void processArray(
      granny_pointer_hash *ObjectHash, 
      char const *Name, int ArrayCount,
      granny_data_type_definition const *Type,
      void *ObjectInit);

   void processPointers(void** p, uint numPointers = 1);

   void processTransform(granny_transform &Transform);
   
   void processDWORDs(const void* p, uint num = 1);

   void* computeDst(const void* pSrc);
   
   template <class scalar_type>
   void processScalarMember(
      granny_data_type_definition const* Type,
      void const* Data,
      char const* FormatCode)
   {
      FormatCode;
      
      void* pDst = computeDst(Data);
      
      const uint sizeOfType = sizeof(scalar_type);
      const uint num = Math::Max(1, Type->ArrayWidth);
      
      if (sizeOfType == 1)
         return;
      else if (sizeOfType == 2)
      {
         EndianSwitchWords((WORD*)pDst, num);
      }
      else if (sizeOfType == 4)
      {
         EndianSwitchDWords((DWORD*)pDst, num);
      }
      else if (sizeOfType == 8)
      {
         EndianSwitchQWords((uint64*)pDst, num);
      }
      else
      {
         BVERIFY(0);
      }
   }
   
   void processVariant(const granny_data_type_definition* VariantType, granny_pointer_hash *ObjectHash);

   void processObject(
      granny_pointer_hash *ObjectHash,
      void const *Object,
      granny_data_type_definition const *ObjectType,
      bool IgnoreHashing);

   void* mpSrc;
   void* mpDst;
   uint mBufSize;
   granny_data_type_definition* mpDataTypeDef;
   granny_pointer_hash* mpPointerHash;
   
   BGrannyDataEndianSwapper(BGrannyDataEndianSwapper&);
   BGrannyDataEndianSwapper& operator= (BGrannyDataEndianSwapper&);
};
