// File colorutils.h
#pragma once

template <typename CompType> struct BColorTypeLimits  { enum { cMinValue = 0,          cMaxValue = 0xFF        }; };
template <> struct BColorTypeLimits<uchar>            { enum { cMinValue = 0,          cMaxValue = UCHAR_MAX   }; };
template <> struct BColorTypeLimits<short>            { enum { cMinValue = SHRT_MIN,   cMaxValue = SHRT_MAX    }; };
template <> struct BColorTypeLimits<ushort>           { enum { cMinValue = 0,          cMaxValue = USHRT_MAX   }; };
template <> struct BColorTypeLimits<int>              { enum { cMinValue = INT_MIN,    cMaxValue = INT_MAX     }; };
template <> struct BColorTypeLimits<uint>             { enum { cMinValue = 0,          cMaxValue = UINT_MAX    }; };
template <> struct BColorTypeLimits<float>            { enum { cMinValue = INT_MIN,    cMaxValue = INT_MAX     }; };

template<typename CompType, typename ParamType = int>
struct BRGBAColorTemplate
{
   CompType r;
   CompType g;
   CompType b;
   CompType a;
   
   enum { R = 0, G = 0, B = 0, A = 3, NumComponents = 4 };
  
   BRGBAColorTemplate() { }
   
   template<typename R> BRGBAColorTemplate(const BRGBAColorTemplate<R>& c) : 
      r(clamp(c.r)), g(clamp(c.g)), b(clamp(c.b)), a(clamp(c.a)) 
   {
   }
   
   BRGBAColorTemplate(ParamType cg, ParamType ca = 0)  
   {
      r = clamp(cg);
      g = r;
      b = r;
      a = clamp(ca);
   }
   
   BRGBAColorTemplate(ParamType cr, ParamType cg, ParamType cb, ParamType ca = getMaxRepresentableValue())  
   { 
      r = clamp(cr);
      g = clamp(cg);
      b = clamp(cb);
      a = clamp(ca);
   }
   
   void clear(void)
   {
      r = 0;
      g = 0;
      b = 0;
      a = 0;
   }
      
   CompType  operator() (uint i) const   { BDEBUG_ASSERT(i < 4); return (&r)[i]; }
   CompType& operator() (uint i)         { BDEBUG_ASSERT(i < 4); return (&r)[i]; }
   
   CompType  operator[] (uint i) const   { BDEBUG_ASSERT(i < 4); return (&r)[i]; }
   CompType& operator[] (uint i)         { BDEBUG_ASSERT(i < 4); return (&r)[i]; }
   
   ParamType getComponent(uint i) const { return (*this)[i]; }
   void setComponent(uint i, ParamType val) { (*this)[i] = clamp(val); }
         
   static ParamType getMinValue(void) 
   {
#if 0   
      if (sizeof(CompType) == sizeof(uchar))
         return 0;
      else if (sizeof(CompType) == sizeof(short))
         return static_cast<ParamType>(SHRT_MIN);
      else 
         return static_cast<ParamType>(INT_MIN);
#endif
      return static_cast<ParamType>(BColorTypeLimits<CompType>::cMinValue);
   }
   
   static ParamType getMaxRepresentableValue(void) 
   {
#if 0   
      if (sizeof(CompType) == sizeof(uchar))
         return static_cast<ParamType>(UCHAR_MAX);
      else if (sizeof(CompType) == sizeof(short))
         return static_cast<ParamType>(SHRT_MAX);
      else 
         return static_cast<ParamType>(INT_MAX);
#endif
      return static_cast<ParamType>(BColorTypeLimits<CompType>::cMaxValue);         
   }
            
   void set(ParamType cg, ParamType ca = 0)
   {
      r = clamp(cg);
      g = r;
      b = r;
      a = clamp(ca);
   }

   void set(ParamType cr, ParamType cg, ParamType cb, ParamType ca)
   {
      r = clamp(cr);
      g = clamp(cg);
      b = clamp(cb);
      a = clamp(ca);
   }
   
   BRGBAColorTemplate<uchar> getRGBAColor(void) const 
   { 
      return BRGBAColorTemplate<uchar>(static_cast<uchar>(r), static_cast<uchar>(g), static_cast<uchar>(b), static_cast<uchar>(a)); 
   }
   
   ParamType getGrayscale(void) const
   {
      return (299 * r + 587 * g + 114 * b + 500) / 1000;
   }
   
   BRGBAColorTemplate& clamp(const BRGBAColorTemplate& lo, const BRGBAColorTemplate& hi)
   {
      for (uint i = 0; i < 4; i++)
      {
         if ((*this)(i) < lo(i)) 
            (*this)(i) = lo(i);
         else if ((*this)(i) > hi(i)) 
            (*this)(i) = hi(i);
      }
      return *this;
   }
   
   BRGBAColorTemplate& clamp(CompType lo, CompType hi)
   {
      for (uint i = 0; i < 4; i++)
      {
         if ((*this)(i) < lo) 
            (*this)(i) = lo;
         else if ((*this)(i) > hi) 
            (*this)(i) = hi;
      }
      return *this;
   }
   
   uint distSquared(const BRGBAColorTemplate& c, bool includeAlpha = true) const
   {
      return Math::Sqr(r - c.r) + Math::Sqr(g - c.g) + Math::Sqr(b - c.b) + (includeAlpha ? Math::Sqr(a - c.a) : 0);
   }
         
   bool compareRGB(const BRGBAColorTemplate& rhs) const
   {
      return ((r == rhs.r) && (g == rhs.g) && (b == rhs.b));
   }
   
   ParamType getMaxComponent(void) const { return Math::Max4(r, g, b, a); }
   ParamType getMaxComponent3(void) const { return Math::Max3(r, g, b); }
   ParamType getMinComponent(void) const { return Math::Min4(r, g, b, a); }
   ParamType getMinComponent3(void) const { return Math::Min3(r, g, b); }
            
   bool operator== (const BRGBAColorTemplate& rhs) const
   {
      return ((r == rhs.r) && (g == rhs.g) && (b == rhs.b) && (a == rhs.a));
   }
   
   bool operator!= (const BRGBAColorTemplate& rhs) const
   {
      return !(*this == rhs);
   }
      
   BRGBAColorTemplate& operator+= (const BRGBAColorTemplate& rhs)
   {
      r = static_cast<CompType>(r + rhs.r);
      g = static_cast<CompType>(g + rhs.g);
      b = static_cast<CompType>(b + rhs.b);
      a = static_cast<CompType>(a + rhs.a);
      return *this;
   }
         
   BRGBAColorTemplate& operator-= (const BRGBAColorTemplate& rhs)
   {
      r = static_cast<CompType>(r - rhs.r);
      g = static_cast<CompType>(g - rhs.g);
      b = static_cast<CompType>(b - rhs.b);
      a = static_cast<CompType>(a - rhs.a);
      return *this;
   }
   
   BRGBAColorTemplate& operator*= (ParamType i)
   {
      r = static_cast<CompType>(r * i);
      g = static_cast<CompType>(g * i);
      b = static_cast<CompType>(b * i);
      a = static_cast<CompType>(a * a);
      return *this;
   }
   
   BRGBAColorTemplate& operator/= (ParamType i)
   {
      r = static_cast<CompType>(r / i);
      g = static_cast<CompType>(g / i);
      b = static_cast<CompType>(b / i);
      a = static_cast<CompType>(a / i);
      return *this;
   }
   
   friend BRGBAColorTemplate operator+ (const BRGBAColorTemplate& lhs, const BRGBAColorTemplate& rhs)
   {
      return BRGBAColorTemplate(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a);
   }
   
   friend BRGBAColorTemplate operator- (const BRGBAColorTemplate& lhs, const BRGBAColorTemplate& rhs)
   {
      return BRGBAColorTemplate(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a);
   }
   
   friend BRGBAColorTemplate operator/ (const BRGBAColorTemplate& lhs, ParamType i)
   {
      return BRGBAColorTemplate(lhs.r / i, lhs.g / i, lhs.b / i, lhs.a / i);
   }
   
   friend BRGBAColorTemplate operator* (const BRGBAColorTemplate& lhs, const BRGBAColorTemplate& rhs)
   {
      return BRGBAColorTemplate(lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a);
   }
   
   friend BRGBAColorTemplate operator/ (const BRGBAColorTemplate& lhs, const BRGBAColorTemplate& rhs)
   {
      return BRGBAColorTemplate(lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a);
   }
   
   friend BRGBAColorTemplate operator* (const BRGBAColorTemplate& lhs, ParamType i)
   {
      return BRGBAColorTemplate(lhs.r * i, lhs.g * i, lhs.b * i, lhs.a * i);
   }

private:
   static CompType clamp(ParamType i)
   {
      if (i < getMinValue())
         i = getMinValue();
      else if (i > getMaxRepresentableValue())
         i = getMaxRepresentableValue();
      return static_cast<CompType>(i);
   }   
};

typedef BRGBAColorTemplate<uchar,   int> BRGBAColor;
typedef BRGBAColorTemplate<short,   int> BRGBAColor16;
typedef BRGBAColorTemplate<ushort,  uint> BRGBAColor16U;
typedef BRGBAColorTemplate<int,     int> BRGBAColor32;
typedef BRGBAColorTemplate<uint,    uint> BRGBAColor32U;
typedef BRGBAColorTemplate<float,   float> BRGBAColorF;

extern const BRGBAColor gWhiteColor;
extern const BRGBAColor gBlackColor;

class BColorUtils
{
public:
   // returns y[0,255], co[-255,255],cg[-255,255]
   static void RGBToYCoCg(int r, int g, int b, int& y, int& co, int& cg);
   static void YCoCgToRGB(int y, int co, int cg, int& r, int& g, int& b);

   static void RGBToYCoCgR(int r, int g, int b, int& y, int& co, int& cg);
   static void YCoCgRToRGB(int y, int co, int cg, int& r, int& g, int& b);

   // returns YCgCo in RGB (Cg in G)
   static void RGBToYCoCgR(const BRGBAColor& rgb, BRGBAColor16& yCoCg);
   static void YCoCgRToRGB(const BRGBAColor16& yCoCg, BRGBAColor& rgb);
   
   static void RGBToYCoCg(const BRGBAColor& rgb, BRGBAColor16& yCoCg);
   static void YCoCgToRGB(const BRGBAColor16& yCoCg, BRGBAColor& rgb);
      
   static void RGBToYCbCr(const BRGBAColor& rgb, BRGBAColor16& yCbCr);
   static void YCbCrToRGB(const BRGBAColor16& YCbCr, BRGBAColor& rgb);
   
   static uint RGBToY(const BRGBAColor& rgb);

   static void unpackColor(WORD packed, int& r, int& g, int& b, bool scaled);
   
   enum eRoundFlags
   {
      cRCeil = 1,
      cGCeil = 2,
      cBCeil = 4,
      
      cRFloor = 8,
      cGFloor = 16,
      cBFloor = 32
   };
   
   static WORD packColor(int r, int g, int b, bool scaled, DWORD roundFlags = 0);

   static void unpackColor(WORD packed, BRGBAColor& color, bool scaled);
   static WORD packColor(const BRGBAColor& color, bool scaled, DWORD roundFlags = 0);

   static int colorDistancePerceptual(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b);
   static int colorDistancePerceptual(int e1r, int e1g, int e1b, int e1a, int e2r, int e2g, int e2b, int e2a);
   static int colorDistancePerceptual(const BRGBAColor& a, const BRGBAColor& b, bool includeAlpha = false);

   static int colorDistanceElucidian(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b);
   static int colorDistanceElucidian(int e1r, int e1g, int e1b, int e1a, int e2r, int e2g, int e2b, int e2a);
   static int colorDistanceElucidian(const BRGBAColor& a, const BRGBAColor& b, bool includeAlpha = false);
      
   static int colorDistanceWeighted(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b);
   static int colorDistanceWeighted(const BRGBAColor& a, const BRGBAColor& b);
};


class BHDRColorUtils
{
public:
   // Pack/unpack radiance RGBE format
   static BRGBAColor& packRGBE(const BRGBAColorF& src, BRGBAColor& dst);
   static BRGBAColorF& unpackRGBE(const BRGBAColor& src, BRGBAColorF& dst);

   static void unpackRGBE(const BRGBAColor* pSrc, BRGBAColorF* pDst, uint numPixels);
   static void packRGBE(const BRGBAColorF* pSrc, BRGBAColor* pDst, uint numPixels);
};
