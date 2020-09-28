#pragma once

//-----------------------------------------------------------------------------
// Utility constants for compression/decompression (don't use these directly)
//-----------------------------------------------------------------------------
#define _COMPRESS_F32_I16     ((float) 32767.0f)      // Constant for compressing an f32 from -1.0f to 1.0f into an i16
#define _COMPRESS_F32_U16     ((float) 65535.0f)      // Constant for compressing an f32 from 0.0f to 1.0f into a u16
#define _COMPRESS_F32_I8      ((float) 127.0f)        // Constant for compressing an f32 from -1.0f to 1.0f into an i8
#define _COMPRESS_F32_U8      ((float) 255.0f)        // Constant for compressing an f32 from 0.0f to 1.0f into a u8

#define _DECOMPRESS_F32_I16   ((float) 1.0f / _COMPRESS_F32_I16)  // Constant for decompressing an i16 into an f32 from -1.0f to 1.0f
#define _DECOMPRESS_F32_U16   ((float) 1.0f / _COMPRESS_F32_U16)  // Constant for decompressing a u16 into an f32 from 0.0f to 1.0f
#define _DECOMPRESS_F32_I8    ((float) 1.0f / _COMPRESS_F32_I8)   // Constant for decompressing an i8 into an f32 from -1.0f to 1.0f
#define _DECOMPRESS_F32_U8    ((float) 1.0f / _COMPRESS_F32_U8)   // Constant for decompressing a u8 into an f32 from 0.0f to 1.0f
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Utility functions for compression/decompression (don't use these directly)
//-----------------------------------------------------------------------------
inline unsigned char _compressF32ToU8(float data, const float multiplier)
{
   data *= multiplier;
   return (unsigned char) data;
}

inline char _compressF32ToI8(float data, const float multiplier)
{
   data *= multiplier;
   return (char) data;
}

inline unsigned short _compressF32ToU16(float data, const float multiplier)
{
   data *= multiplier;
   return (unsigned short) data;
}

inline short _compressF32ToI16(float data, const float multiplier)
{
   data *= multiplier;
   return (short) data;
}

inline float _decompressF32FromU8(const unsigned char data, const float multiplier)
{
   float f = (float) data;
   f *= multiplier;
   return f;
}

inline float _decompressF32FromI8(const char data, const float multiplier)
{
   float f = (float) data;
   f *= multiplier;
   return f;
}

inline float _decompressF32FromU16(const unsigned short data, const float multiplier)
{
   float f = (float) data;
   f *= multiplier;
   return f;
}

inline float _decompressF32FromI16(const short data, const float multiplier)
{
   float f = (float) data;
   f *= multiplier;
   return f;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Compression/Decompression macros
//-----------------------------------------------------------------------------
// Macro for compressing an f32 from minValue to maxValue into a u16
#define COMPRESS_F32_TO_U16(data, minValue, maxValue)                \
   (_compressF32ToU16(((float) data - (float) minValue),             \
   (_COMPRESS_F32_U16 / ((float) maxValue - (float) minValue))))

// Macro for compressing an f32 from minValue to maxValue into a u8
#define COMPRESS_F32_TO_U8(data, minValue, maxValue)                 \
   (_compressF32ToU8(((float) data - (float) minValue),              \
   (_COMPRESS_F32_U8 / ((float) maxValue - (float) minValue))))

// Macro for decompressing a u16 into an f32 from minValue to maxValue
#define DECOMPRESS_F32_FROM_U16(data, minValue, maxValue)            \
   (_decompressF32FromU16(data,                                      \
   (_DECOMPRESS_F32_U16 * ((float) maxValue - (float) minValue)))    \
   + (float) minValue)

// Macro for decompressing a u8 into an f32 from minValue to maxValue
#define DECOMPRESS_F32_FROM_U8(data, minValue, maxValue)             \
   (_decompressF32FromU8(data,                                       \
   (_DECOMPRESS_F32_U8 * ((float) maxValue - (float) minValue)))     \
   + (float) minValue)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline char compand_RngSqrt(const float val,const float range,const char maxVal=127)
{
   const float rangeVal = 1.f / range;
   const char cVal = maxVal;

   float aVal = abs(val/rangeVal);
   float sVal = sqrt(aVal);
   float sign = (val<0)?-1:1;
   char comp = ((cVal*(sVal*sign)));
   return comp;
}
//-----------------------------------------------------------------------------
inline float uncompand_RngSqrt(const char val,const float range,const float maxVal=127.f)
{
   const float rangeVal = 1.f / range;

   const float fVal = maxVal;

   float sVal = float((val)/fVal);
   float sign = (sVal<0)?-1:1;
   float aVal = abs(sVal);
   float pVal = aVal ;
   float tVal = pow(pVal,2)*sign*rangeVal;
   return tVal;
}