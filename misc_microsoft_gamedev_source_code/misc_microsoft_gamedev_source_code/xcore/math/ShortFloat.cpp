//==============================================================================
// File: ShortFloat.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#include "xcore.h"
#include "shortfloat.h"


int  nBits = 11; 
int  expBits = 16-nBits-1;
int  expRange = 0x01 << expBits;
int  biasFloor = 127 - (expRange / 2);
 
#define MANTISSA_MASK_11   0x000007FF
#define EXPONENT_MASK_11   0x00007800
 
#define MANTISSA_MASK_10   0x000003FF
#define EXPONENT_MASK_10   0x00007C00
 
#define MANTISSA_MASK_9    0x000001FF
#define EXPONENT_MASK_9    0x00007E00
 
#define MANTISSA_MASK_HERB   0x0000007F
#define EXPONENT_MASK_HERB   0x00007F80
 
DWORD  MANTISSA_MASK = MANTISSA_MASK_11;
DWORD  EXPONENT_MASK = EXPONENT_MASK_11;
 
// rg [12/10/05] - FIXME: This takes 256k!
static float staticDatabase[65536];
static bool initData = false;

//=============================================================================
// shortToFloat
//=============================================================================
float shortToFloat(WORD packedValue)
{
   if (!initData)
   {
      // rg [12/31/05] - This is not thread safe, but no harm should be done if two threads execute this fill loop simultaneously.
      for(long i=0; i<65536; i++)
      {
         float  result;
         DWORD  *pResult = (DWORD*) &result;
 
         DWORD  XpackedValue = (i & 0x0000FFFF);
 
         DWORD  M     =   (XpackedValue & MANTISSA_MASK) << (23-nBits);
         DWORD  EXP   = (((XpackedValue & EXPONENT_MASK) >> nBits) + biasFloor) << 23;;
         DWORD  SIGN  =   (XpackedValue & 0x00008000) << 16;
 
         *pResult  =  (DWORD) SIGN | EXP | M;

         staticDatabase[i] = result;
      }
      initData = true;
   }

   return (staticDatabase[packedValue]);
} // shortToFloat

//=============================================================================
// floatToShort
//=============================================================================
static WORD floatToShort(float value)
{
   DWORD *p = (DWORD*) &value;
   if (*p == 0) return(0);
 
   DWORD  M    =  ((*p) & 0x007FFFFF) >> (23-nBits);
   DWORD  EXP  = (((*p) & 0x7F800000) >> 23) - biasFloor;
   DWORD  SIGN =  ((*p) & 0x80000000) >> 16;
 
   // We don't shift the Exponent << nBits in the line above so we can
   // Check the intermediate result for a value out of bounds
 
   // If EXP < 0, the number is too small (too close to 0)
   if ((int) EXP < 0 )
   {
      return 0;            // the packed value of 0
   }
   else if ((int) EXP >= expRange)
   {
      // Issue Warning Than the value was out of the compressable range
      // If EXP > MaxRange, the number is too big (too far from 0)
      BFAIL("Unable to Pack Exponent.");
      return (0);
   }
 
   WORD  PackedValue = (WORD) ( (DWORD) (SIGN | (EXP << nBits) | M) );
 
   return (PackedValue);

} // floatToShort

//=============================================================================
// floatToShortFloat
//=============================================================================
static BShortFloat floatToShortFloat(float value)
{
   return (BShortFloat(value));
} // floatToShortFloat

//==============================================================================
// 
//==============================================================================
BShortFloat::BShortFloat(float f)
{
   mValue = floatToShort(f);
}

//==============================================================================
// 
//==============================================================================
float BShortFloat::asFloat(void) const
{
   return(shortToFloat(mValue));
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator=(BShortFloat sf)
{
   mValue = sf.mValue;

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator=(float f)
{
   mValue = floatToShort(f);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator*(float a) const
{
   float f = shortToFloat(mValue);

   return floatToShortFloat(f * a);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator*(BShortFloat a) const
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return floatToShortFloat(f1 * f2);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator/(float a) const
{
   float f = shortToFloat(mValue);

   return floatToShortFloat(f / a);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator/(BShortFloat a) const
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return floatToShortFloat(f1 / f2);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator-(float a) const
{
   float f = shortToFloat(mValue);

   return floatToShortFloat(f - a);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator-(BShortFloat a) const
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return floatToShortFloat(f1 - f2);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator+(float a) const
{
   float f = shortToFloat(mValue);

   return floatToShortFloat(f + a);
}

//==============================================================================
// 
//==============================================================================
BShortFloat BShortFloat::operator+(BShortFloat a) const
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return floatToShortFloat(f1 + f2);
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator*=(float a)
{
   float f = shortToFloat(mValue);

   mValue = floatToShort(f * a);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator*=(BShortFloat a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   mValue = floatToShort(f1 * f2);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator/=(float a)
{
   float f = shortToFloat(mValue);

   mValue = floatToShort(f / a);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator/=(BShortFloat a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   mValue = floatToShort(f1 / f2);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator-=(float a)
{
   float f = shortToFloat(mValue);

   mValue = floatToShort(f - a);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator-=(BShortFloat a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   mValue = floatToShort(f1 - f2);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator+=(float a)
{
   float f = shortToFloat(mValue);

   mValue = floatToShort(f + a);

   return *this;
}

//==============================================================================
// 
//==============================================================================
BShortFloat &BShortFloat::operator+=(BShortFloat a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   mValue = floatToShort(f1 + f2);

   return *this;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator==(const BShortFloat &a)
{
   return mValue == a.mValue;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator==(float a)
{
   float f1 = shortToFloat(mValue);

   return a == f1;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator>=(const BShortFloat &a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return f1 >= f2;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator>=(float a)
{
   float f1 = shortToFloat(mValue);

   return f1 >= a;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator>(const BShortFloat &a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return f1 > f2;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator>(float a)
{
   float f1 = shortToFloat(mValue);

   return f1 > a;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator<=(const BShortFloat &a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return f1 <= f2;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator<=(float a)
{
   float f1 = shortToFloat(mValue);

   return f1 <= a;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator<(const BShortFloat &a)
{
   float f1 = shortToFloat(mValue);
   float f2 = shortToFloat(a.mValue);

   return f1 < f2;
}

//==============================================================================
// 
//==============================================================================
bool BShortFloat::operator<(float a)
{
   float f1 = shortToFloat(mValue);

   return f1 < a;
}

//==============================================================================
// eof: ShortFloat.cpp
//==============================================================================
