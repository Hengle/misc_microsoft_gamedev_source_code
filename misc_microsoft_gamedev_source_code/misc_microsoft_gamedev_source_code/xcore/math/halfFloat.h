//============================================================================
//
// File: halfFloat.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

namespace HalfFloat
{
   inline float IntToFloat(int i)
   {
      return *reinterpret_cast<const float*>(&i);
   }

   inline uint FloatToUInt(float f)
   {
      return *reinterpret_cast<const uint*>(&f);
   }
         
   inline ushort FloatToHalf(float f, bool noAbnormalValues = false)
   {
      const uint i = FloatToUInt(f);
      
      const uint sign = 0x8000 & (i >> 16);
      int exp = (255 & (i >> 23)) - 112;
      uint man = 0x7FFFFF & i;
      
      // max. possible exponent value indicates Inf/NaN
      if (143 == exp)   
      {
         if (noAbnormalValues)
            return 0;
            
         if (0 == man)
         {
            // output infinity
            return static_cast<ushort>(sign | 0x7C00);
         }
         else 
         {
            // output NaN
            return static_cast<ushort>(sign | (man >> 13) | 0x7C00);
         }
      }
      else if (exp <= 0)
      {
         if (noAbnormalValues)
            return 0;
            
         // too small
         if (exp < -10)
            return static_cast<ushort>(sign);
                        
         // output denormal
         man = (man | 0x800000) >> (1 - exp);
         
         // Round 
         if (man & 0x1000)
            man += 0x2000;
            
         return static_cast<ushort>(sign | (man >> 13));
      }
               
      // round
      if (man & 0x1000)
      {
         man += 0x2000;
         if (man > 0x7FFFFF)
         {
            exp++;
            man = 0;
         }
      }
      
      // rg [12/30/05] - Was 29, oops!
      if (exp > 30)  
         return static_cast<ushort>(sign | 0x7C00);
         
      return static_cast<ushort>(sign | (exp << 10) | (man >> 13));
   }

   inline float HalfToFloat (ushort h, bool noAbnormalValues = false)
   {
      const uint sign = 0x80000000 & ((uint)h << 16);
      uint exp = 31 & (h >> 10);
      uint man = 1023 & h;

      if (exp == 0)
      {
         if (man == 0)  
         {
            // +-0
            return IntToFloat(sign);
         }
         else  
         {
            // normalize denormal
            while (0 == (man & 0x400)) 
            {
               exp--;
               man <<= 1;
            }
            
            exp++;
            man &= ~0x400;
         }
      }
      else if (exp == 31)
      {
         if (noAbnormalValues)
            return 0;
            
         if (man)
         {
            // NaN
            return IntToFloat(sign | (man << 13) | 0x7F800000); 
         }
         else
         {
            // +-inf
            return IntToFloat(sign | 0x7F800000);
         }
      }
                              
      return IntToFloat(sign | ((exp + 112) << 23) | (man << 13));
   }
   
   inline ushort FloatToHalfLittleEndian(float f, bool noAbnormalValues = false)
   {
      ushort val = FloatToHalf(f, noAbnormalValues);;
      if (cBigEndianNative) EndianSwitchWords(&val, 1);
      return val;
   }
   
   inline ushort FloatToHalfBigEndian(float f, bool noAbnormalValues = false)
   {
      ushort val = FloatToHalf(f, noAbnormalValues);;
      if (cLittleEndianNative) EndianSwitchWords(&val, 1);
      return val;
   }

   inline float LittleEndianHalfToFloat(ushort h, bool noAbnormalValues = false)      
   {
      if (cBigEndianNative) EndianSwitchWords(&h, 1);
      return HalfToFloat(h, noAbnormalValues);
   }
   
   inline float BigEndianHalfToFloat(ushort h, bool noAbnormalValues = false)      
   {
      if (cLittleEndianNative) EndianSwitchWords(&h, 1);
      return HalfToFloat(h, noAbnormalValues);
   }
         
} // namespace HalfFloat   

// cMaxRelativeError is used to help us catch when we are storing a float in a BHalfFloat and it is losing too much precision.
// Precision loss is calculated as a relative error not an absolute error.
#ifdef BUILD_DEBUG
   __declspec(selectany) extern const double cHalfFloatMaxRelativeError = 0.001; // 99.9% accurate or better.
#endif

//==============================================================================
// class BHalfFloat
//==============================================================================
class BHalfFloat
{
public:
   BHalfFloat() { }

   BHalfFloat(ushort h) { mHalf = h; }   
   
   BHalfFloat(const BHalfFloat& h) { mHalf = h.mHalf; }

   void clear(void) { mHalf = 0; }
   
   BHalfFloat& operator= (BHalfFloat& h) { mHalf = h.mHalf; return *this; }
   
   ushort getBits(void) const { return mHalf; }
   void setBits(uint bits) { mHalf = static_cast<ushort>(bits); }
   
#ifdef XBOX
   BHalfFloat(float f) { mHalf = XMConvertFloatToHalf(f); BDEBUG_ASSERT(Math::EqualRelTol<double>(f, XMConvertHalfToFloat(mHalf), cHalfFloatMaxRelativeError)); }
   operator float() const { return XMConvertHalfToFloat(mHalf); }
   BHalfFloat& operator= (float f) { mHalf = XMConvertFloatToHalf(f); BDEBUG_ASSERT(Math::EqualRelTol<double>(f, XMConvertHalfToFloat(mHalf), cHalfFloatMaxRelativeError)); return *this; }
   bool operator== (float f) { return (mHalf == XMConvertFloatToHalf(f)); }
   bool operator!= (float f) { return (mHalf != XMConvertFloatToHalf(f)); }
   bool operator< (float f) { return (mHalf < XMConvertFloatToHalf(f)); }
   bool operator> (float f) { return (mHalf > XMConvertFloatToHalf(f)); }
   bool operator<= (float f) { return (mHalf <= XMConvertFloatToHalf(f)); }
   bool operator>= (float f) { return (mHalf >= XMConvertFloatToHalf(f)); }
#else   
   BHalfFloat(float f) { mHalf = HalfFloat::FloatToHalf(f); }
   operator float() const { return HalfFloat::HalfToFloat(mHalf); }
   BHalfFloat& operator= (float f) { mHalf = HalfFloat::FloatToHalf(f); return *this; }
   bool operator== (float f) { return (mHalf == HalfFloat::FloatToHalf(f)); }
   bool operator!= (float f) { return (mHalf != HalfFloat::FloatToHalf(f)); }
   bool operator< (float f) { return (mHalf < HalfFloat::FloatToHalf(f)); }
   bool operator> (float f) { return (mHalf > HalfFloat::FloatToHalf(f)); }
   bool operator<= (float f) { return (mHalf <= HalfFloat::FloatToHalf(f)); }
   bool operator>= (float f) { return (mHalf >= HalfFloat::FloatToHalf(f)); }
#endif   
      
private:

   ushort mHalf;
};