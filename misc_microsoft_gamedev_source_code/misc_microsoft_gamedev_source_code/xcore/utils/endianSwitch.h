//-----------------------------------------------------------------------------
// File: EndianSwitch.h
//
// Desc: Functions for changing the endianness (byte ordering) of data. This
//       code should generally be run on the development PC at authoring time,
//       but the same code can also run on the console.
//
// Hist: 04.10.04 - Created
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef ENDIANSWITCH_H
#define ENDIANSWITCH_H

// Disable warnings about using constant conditionals - they are used
// intentionally.
// Disable PREfast warnings
#pragma warning( push )
#pragma warning( disable : 4127 25042)

// Use a const bool to specify what the native endianness is. This will be
// evaluated at compile time.
#if defined(_M_PPCBE) || defined(XBOX) || defined(_XBOX)
   // PowerPC/Xbox 2 is big-endian
   const bool cLittleEndianNative = false;
   const bool cBigEndianNative = true;
#else
   // Other platforms are assumed to be little-endian.
   const bool cLittleEndianNative = true;
   const bool cBigEndianNative = false;
#endif


//-----------------------------------------------------------------------------
// Name: EndianSwitchWorker
// Desc: Worker function for byte swapping of complex data structures. It is
//       usually better to call the higher level functions such as
//       LittleEndianToNative.
//       For full documentation of the format string see the EndianSwitchWorker
//       implementation.
//-----------------------------------------------------------------------------
void* EndianSwitchWorker( void* pData, void* pEnd, const char* format,
            int blockRepeatCount = 1, const char** updatedFormat = 0 );


//-----------------------------------------------------------------------------
// Name: EndianSwitchWords
// Desc: Byte swap the specified number of two-byte items. The updated
//       address is returned.
//-----------------------------------------------------------------------------
void* EndianSwitchWords( WORD* pData, int count );


//-----------------------------------------------------------------------------
// Name: EndianSwitchDWords
// Desc: Byte swap the specified number of four-byte items. The updated
//       address is returned.
//-----------------------------------------------------------------------------
void* EndianSwitchDWords( DWORD* pData, int count );



//-----------------------------------------------------------------------------
// Name: EndianSwitchQWords
// Desc: Byte swap the specified number of eight-byte items. The updated
//       address is returned.
//-----------------------------------------------------------------------------
void* EndianSwitchQWords( unsigned __int64* pData, int count );


//--------------------------------------------------------------------------------------
// Name: LittleEndianToNative
// Desc: This helper function uses a template to grab the size of the structure being
//       converted, and also checks to see whether conversion is needed. This makes it
//       easy to write endian-safe code that runs on any platform - converting as
//       needed.
//       Use this function when you have little endian data that you want to use,
//       regardless of what endianness your current platform is.
//--------------------------------------------------------------------------------------
template <typename T>
void LittleEndianToNative( T* pData, const char* format )
{
    if( !cLittleEndianNative )
        EndianSwitchWorker( pData, pData + 1, format );
}


//--------------------------------------------------------------------------------------
// Name: NativeToLittleEndian
// Desc: Use this function when you want to convert native data to little endian format,
//       regardless of what endianness your current platform is.
//       The same code goes from little endian to native format
//       and vice-versa. The different functions exist so that you
//       can easily show your intent.
//--------------------------------------------------------------------------------------
template <typename T>
void NativeToLittleEndian( T* pData, const char* format )
{
    if( !cLittleEndianNative )
        EndianSwitchWorker( pData, pData + 1, format );
}


//--------------------------------------------------------------------------------------
// Name: BigEndianToNative
// Desc: This helper function uses a template to grab the size of the structure being
//       converted, and also checks to see whether conversion is needed. This makes it
//       easy to write endian-safe code that runs on any platform - converting as
//       needed.
//       Use this function when you have big endian data that you want to use,
//       regardless of what endianness your current platform is.
//--------------------------------------------------------------------------------------
template <typename T>
void BigEndianToNative( T* pData, const char* format )
{
    if( cLittleEndianNative )
        EndianSwitchWorker( pData, pData + 1, format );
}


//--------------------------------------------------------------------------------------
// Name: NativeToBigEndian
// Desc: Use this function when you want to convert native data to big endian format,
//       regardless of what endianness your current platform is.
//       The same code goes from big endian to native format
//       and vice-versa. The different functions exist so that you
//       can easily show your intent.
//--------------------------------------------------------------------------------------
template <typename T>
void NativeToBigEndian( T* pData, const char* format )
{
    if( cLittleEndianNative )
        EndianSwitchWorker( pData, pData + 1, format );
}

//--------------------------------------------------------------------------------------
// Name: WriteWordAsLittleEndian
// Desc: Use this function if you want to con
//--------------------------------------------------------------------------------------
inline void WriteWordAsLittleEndian(void* pData, WORD value)
{
   if( cLittleEndianNative )
   {
      *reinterpret_cast<WORD*>(pData)= value;
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      pDestData[0]= static_cast<BYTE>(value & 0xFF);
      pDestData[1]= static_cast<BYTE>((value>>8) & 0xFF);
   }
}

//--------------------------------------------------------------------------------------
// Name: WriteWordAsBigEndian
//--------------------------------------------------------------------------------------
inline void WriteWordAsBigEndian(void* pData, WORD value)
{
   if( !cLittleEndianNative )
   {
      *reinterpret_cast<WORD*>(pData)= value;
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      pDestData[1]= static_cast<BYTE>(value & 0xFF);
      pDestData[0]= static_cast<BYTE>((value>>8) & 0xFF);
   }
}

//--------------------------------------------------------------------------------------
// Name: WriteDWordAsLittleEndian
//--------------------------------------------------------------------------------------
inline void WriteDWordAsLittleEndian(void* pData, DWORD value)
{
   if( cLittleEndianNative )
   {
      *reinterpret_cast<DWORD*>(pData) = value;
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      pDestData[0]= static_cast<BYTE>(value & 0xFF);
      pDestData[1]= static_cast<BYTE>((value>>8)  & 0xFF);
      pDestData[2]= static_cast<BYTE>((value>>16) & 0xFF);
      pDestData[3]= static_cast<BYTE>((value>>24) & 0xFF);
   }
}

//--------------------------------------------------------------------------------------
// Name: WriteDWordAsBigEndian
//--------------------------------------------------------------------------------------
inline void WriteDWordAsBigEndian(void* pData, DWORD value)
{
   if( !cLittleEndianNative )
   {
      *reinterpret_cast<DWORD*>(pData) = value;
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      pDestData[3]= static_cast<BYTE>(value & 0xFF);
      pDestData[2]= static_cast<BYTE>((value>>8)  & 0xFF);
      pDestData[1]= static_cast<BYTE>((value>>16) & 0xFF);
      pDestData[0]= static_cast<BYTE>((value>>24) & 0xFF);
   }
}

//--------------------------------------------------------------------------------------
// Name: ReadDWordAsLittleEndian
//--------------------------------------------------------------------------------------
inline WORD ReadWordAsLittleEndian(void* pData)
{
   if( cLittleEndianNative )
   {
      return *reinterpret_cast<WORD*>(pData);
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);

      return pDestData[0] | (pDestData[1]<<8);
   }
}

//--------------------------------------------------------------------------------------
// Name: ReadDWordAsBigEndian
//--------------------------------------------------------------------------------------
inline WORD ReadWordAsBigEndian(void* pData)
{
   if( !cLittleEndianNative )
   {
      return *reinterpret_cast<WORD*>(pData);
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);

      return pDestData[1] | (pDestData[0]<<8);
   }
}

//--------------------------------------------------------------------------------------
// Name: ReadDWordAsLittleEndian
//--------------------------------------------------------------------------------------
inline DWORD ReadDWordAsLittleEndian(void* pData)
{
   if( cLittleEndianNative )
   {
      return *reinterpret_cast<DWORD*>(pData);
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      return pDestData[0] | (pDestData[1]<<8) | (pDestData[2]<<16) | (pDestData[3]<<24);
   }
}

//--------------------------------------------------------------------------------------
// Name: ReadDWordAsBigEndian
//--------------------------------------------------------------------------------------
inline DWORD ReadDWordAsBigEndian(void* pData)
{
   if( !cLittleEndianNative )
   {
      return *reinterpret_cast<DWORD*>(pData);
   }
   else
   {
      BYTE* pDestData = reinterpret_cast<BYTE*>(pData);
      return pDestData[3] | (pDestData[2]<<8) | (pDestData[1]<<16) | (pDestData[0]<<24);
   }
}

//--------------------------------------------------------------------------------------
// Lint-free helper function for stack-allocated integer types
//--------------------------------------------------------------------------------------
void EndianSwitch(int16 & value);
void EndianSwitch(uint16 & value);
void EndianSwitch(int32 & value);
void EndianSwitch(uint32 & value);
void EndianSwitch(int64 & value);
void EndianSwitch(uint64 & value);

//--------------------------------------------------------------------------------------
// Lint-free helper function for arrays of integer types
//--------------------------------------------------------------------------------------
void EndianSwitch(int16 arrayValue[], int32 count);
void EndianSwitch(uint16 arrayValue[], int32 count);
void EndianSwitch(int32 arrayValue[], int32 count);
void EndianSwitch(uint32 arrayValue[], int32 count);
void EndianSwitch(int64 arrayValue[], int32 count);
void EndianSwitch(uint64 arrayValue[], int32 count);

#pragma warning( pop )

#endif
