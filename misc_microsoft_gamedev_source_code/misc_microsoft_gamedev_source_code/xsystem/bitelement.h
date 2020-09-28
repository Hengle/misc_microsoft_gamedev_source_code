//==============================================================================
// bitelement.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "bitvector.h"

#pragma warning(push)
#pragma warning(disable:4244)
template <uint8 ElementBitSize, uint32 NumElements> class BBitElement16 : public UTBitVector<(ElementBitSize * NumElements)>
{
public:

   long getElementBitSize() const { return ElementBitSize; }

   long getNumElements() const { return NumElements; }

   uint16 getElement(uint32 index) const
   {
      BDEBUG_ASSERT((index >= 0) && (index < NumElements));
      uint32 firstBitIndex = index * ElementBitSize;
      uint32 byteIndex = firstBitIndex >> 3;
      uint16 mask = (1 << ElementBitSize) - 1;
      uint16 shift = (uint16) (firstBitIndex - (byteIndex << 3));
      return (__loadshortbytereverse(byteIndex, mValue) >> shift) & mask;
   }

   void setElement(uint32 index, uint16 element)
   {
      BDEBUG_ASSERT((index >= 0) && (index < NumElements));
      uint32 firstBitIndex = index * ElementBitSize;
      uint32 byteIndex = firstBitIndex >> 3;
      uint16 mask = (1 << ElementBitSize) - 1;
      uint16 shift = (uint16) (firstBitIndex - (byteIndex << 3));
      __storeshortbytereverse((__loadshortbytereverse(byteIndex, mValue) & ~(mask << shift)) | ((element & mask) << shift), byteIndex, mValue);
   }
}; // BBitElement16
#pragma warning(pop)
