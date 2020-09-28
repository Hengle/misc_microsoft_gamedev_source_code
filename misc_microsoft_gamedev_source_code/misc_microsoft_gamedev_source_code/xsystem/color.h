//==============================================================================
// color.h
//
// Copyright (c) 1999-2001 Ensemble Studios
//==============================================================================


#ifndef _COLOR_H_
#define _COLOR_H_

//==============================================================================
// lint directives
//==============================================================================
//lint -e1927 -e1401 -e777 -e1706

//==============================================================================
// Forward declarations.
class BChunkWriter;
class BChunkReader;

//==============================================================================
// class BColor
//
// WARNING: This class can never have ANY virtual functions.  Don't add any
// member variables to it either.
//==============================================================================
class BColor 
{
   public:
                              BColor(void)
                              {
                                 // don't EVER initialize anything here
                              }

                              BColor(float initR, float initG, float initB) 
                              {
                                 r=initR; g=initG; b=initB;
                              }

                              BColor(const BColor& c) 
                              {
                                 r=c.r; g=c.g; b=c.b;
                              }

      explicit                BColor(DWORD c) 
                              {
                                 *this = c;
                              }

      explicit                BColor(const char *colorName);

      long                    operator==(const BColor &c) const {return(r==c.r && g==c.g && b==c.b);}
      long                    operator!=(const BColor &c) const {return(r!=c.r || g!=c.g || b!=c.b);}
      BColor                  operator-(const BColor& v) const { return(BColor(r-v.r, g-v.g, b-v.b)); }
      BColor                  operator+(const BColor& v) const { return(BColor(r+v.r, g+v.g, b+v.b)); }
      BColor                  operator*(const float& scale) const { return(BColor(r*scale, g*scale, b*scale)); }
                              
      void                    clamp(void) 
                              {
                                 if(r>1.0f) 
                                    r=1.0f; 
                              
                                 if(g>1.0f) 
                                    g=1.0f; 
                              
                                 if(b>1.0f) 
                                    b=1.0f;

                                 if(r<0.0f) 
                                    r=0.0f; 
                              
                                 if(g<0.0f) 
                                    g=0.0f; 
                              
                                 if(b<0.0f) 
                                    b=0.0f;
                              }
                              
                              //lint -e1539 -e419 -e420
      BColor                  &operator=(const BColor & c)
                              {
#ifdef _DEBUG
                                 if (this == &c)
                                 {
                                    // jce [6/24/2002] please don't assert
                                    //BASSERT(0);
                                    return *this;
                                 }
#endif
                                 memcpy(&r, &c.r, 3 * sizeof(float));
                              
                                 return *this;
                              }

      BColor                  &operator=(DWORD dwcolor)
                              {
                                 // multiply by 1/255 rather than dividing by 255

                                 r = (float) ((dwcolor & cRedMask) >> cRedShift) * cOneOver255;
                                 g = (float) ((dwcolor & cGreenMask) >> cGreenShift) * cOneOver255;
                                 b = (float) (dwcolor & cBlueMask) * cOneOver255;

                                 return *this;

                              } // BColor::operator=

      BColor                  &operator=(float *pcolor)
                              {
                                 r = pcolor[0];
                                 g = pcolor[1];
                                 b = pcolor[2];
      
                                 return *this;
                              }

      DWORD                   asDWORD() const
                              {
                                 DWORD color = 0;
                                 color |= (DWORD)255 << cAlphaShift;
                                 color |= (DWORD)(r * 255.0f) << cRedShift;
                                 color |= (DWORD)(g * 255.0f) << cGreenShift;
                                 color |= (DWORD)(b * 255.0f);

                                 return (color);
                              }
                              //lint +e1539 +e419 +e420

      void                    set(float newR, float newG, float newB) {r=newR; g=newG; b=newB;}
      void                    setLong(long newR, long newG, long newB) {r=float(newR)*cOneOver255; g=float(newG)*cOneOver255; b=float(newB)*cOneOver255;}
                              
      
      void                    addIntensity(const float red, const float green, const float blue)
                              {
                                 r += red;
                                 g += green;
                                 b += blue;
                                 // don't bother to clamp here, since we'll clamp as a final step
                              }

      const BColor            &BColor::operator*=( const BColor &col )
                              {
                                 r = r * col.r;
                                 g = g * col.g;
                                 b = b * col.b;
                                 return *this;
                              }

      void                    BColor::operator*=( float scale )
                              {
                                 r *= scale;
                                 g *= scale;
                                 b *= scale;
                              }

      void                    lerp(const BColor& color0, const BColor& color1, float alpha);

      static bool             writeVersion(BChunkWriter *chunkWriter);
      static bool             readVersion(BChunkReader *chunkReader);
      bool                    save(BChunkWriter *chunkWriter);
      bool                    load(BChunkReader *chunkReader);
      
      float                   r, g, b;

   protected:
      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
}; // BColor

//==============================================================================
// lint directives
//==============================================================================
//lint +e1927 +e1401 +e777 +e1706


//==============================================================================\
// conversion functions
//==============================================================================
DWORD packRGB(const BColor &color);
DWORD packRGB(const float *pcolor);
DWORD packRGBA(const BColor &color);
DWORD packRGBA(const BColor &color, float falpha);
DWORD packRGBA(DWORD color, float falpha);
DWORD packRGB(int r, int g, int b);

void srgbToLinear(DWORD color, float &resultR, float &resultG, float &resultB);
void srgbToLinear(float r, float g, float b, float &resultR, float &resultG, float &resultB);


//==============================================================================
// Some handy color consts
//==============================================================================
#define COLOR_CONST(x, red, green, blue) \
   __declspec(selectany) extern const BColor cColor##x(red, green, blue); \
   __declspec(selectany) extern const DWORD cDWORD##x = DWORD( 0xFF000000 | (BYTE(red*255.0f)<<16) | (BYTE(green*255.0f)<<8) | BYTE(blue*255.0f) ); \
   __declspec(selectany) extern const DWORD cDWORD##x##NoAlpha = DWORD( (BYTE(red*255.0f)<<16) | (BYTE(green*255.0f)<<8) | BYTE(blue*255.0f) ); \

COLOR_CONST(White, 1.0f, 1.0f, 1.0f)
COLOR_CONST(Grey, 0.5f, 0.5f, 0.5f)
COLOR_CONST(DarkGrey, 0.25f, 0.25f, 0.25f)
COLOR_CONST(LightGrey, 0.75f, 0.75f, 0.75f)
COLOR_CONST(Black, 0.0f, 0.0f, 0.0f)
COLOR_CONST(Red, 1.0f, 0.0f, 0.0f)
COLOR_CONST(Green, 0.0f, 1.0f, 0.0f)
COLOR_CONST(Blue, 0.0f, 0.0f, 1.0f)
COLOR_CONST(Yellow, 1.0f, 1.0f, 0.0f)
COLOR_CONST(Magenta, 1.0f, 0.0f, 1.0f)
COLOR_CONST(Cyan, 0.0f, 1.0f, 1.0f)
COLOR_CONST(Orange, 1.0f, 0.5f, 0.0f)
COLOR_CONST(DarkOrange, 0.5f, 0.25f, 0.0f)
COLOR_CONST(Purple, 0.5f, 0.0f, 0.5f)
COLOR_CONST(Gold, 0.905882f, 0.788235f, 0.156862f)

#endif // _COLOR_H_

//==============================================================================
// eof: color.h
//==============================================================================

