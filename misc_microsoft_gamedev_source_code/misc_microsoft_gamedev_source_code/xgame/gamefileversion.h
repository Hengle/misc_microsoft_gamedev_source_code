//==============================================================================
// gamefileversion.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// BGameFileVersion
//
// Allows us to pack in more information about the contents of the game file
// Do not exceed 4 bytes
// The header will not be compressed to maintain a fixed length
//==============================================================================
union BGameFileVersion
{
   struct
   {
      uint32 mReserved : 13;
      uint32 mEncryptHeader : 1;
      uint32 mEncryptData : 1;
      uint32 mCompressData : 1;
      uint32 mVersion : 16;
   };
   uint32 mValue;
};
