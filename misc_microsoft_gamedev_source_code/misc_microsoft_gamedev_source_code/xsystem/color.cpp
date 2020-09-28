//==============================================================================
// color.cpp
//
// Copyright (c) 1999-2002 Ensemble Studios
//==============================================================================

#include "xsystem.h"

#include "color.h"
#include "chunker.h"


//==============================================================================
// BColor::msSaveVersion
//==============================================================================
const DWORD BColor::msSaveVersion=0;

//==============================================================================
// BColor::msLoadVersion
//==============================================================================
DWORD BColor::msLoadVersion=0xFFFF;

//==============================================================================
// 
//==============================================================================
// 22Oct00 - ham - made this data directed rather than hard coded
//                 I'm also unsure whether to move this class decl to the header 
//                 since this is only a local helper class *grimace*
//
//               - maybe this shouldn't even be here? hmmmm ...

class BLocalColor
{
   public:
      const char *mpLocalColorName;
      float       mfColor[3];
};

//==============================================================================
// 
//==============================================================================
BLocalColor localColorArray[] = 
{  { "colorBlack",      {0, 0, 0} },  // default, please don't move black around :}
   { "colorWhite",      {1, 1, 1} },
   { "colorGrey",       {0.5, 0.5, 0.5} },
   { "colorLightGrey",  {0.75, 0.75, 0.75} },
   { "colorDarkGrey",   {0.25, 0.25, 0.25} },
   { "colorRed",        {1, 0, 0} },
   { "colorGreen",      {0, 1, 0} },
   { "colorBlue",       {0, 0, 1} },
   { "colorYellow",     {1, 1, 0} },
   { "colorMagenta",    {1, 0, 1} },
   { "colorCyan",       {0, 1, 1} },
   { "colorBrown",      {0.60f, 0.40f, 0.20f} },
};

//==============================================================================
// BColor::BColor
// construct a BColor based on a character string
//==============================================================================
BColor::BColor(const char *colorName)
{
   long max = sizeof(localColorArray)/sizeof(localColorArray[0]);

   long i;
   for (i = 0; i < max; i++)
      if (!stricmp(colorName, localColorArray[i].mpLocalColorName))
      {
         *this = localColorArray[i].mfColor;
         break;
      }

   // if not found, set to black

   if (i >= max)
      *this = localColorArray[0].mfColor; // black


} //end BColor::BColor

//==============================================================================
// BColor::lerp
//==============================================================================
void BColor::lerp(const BColor& color0, const BColor& color1, float alpha)
{
   r = color0.r + alpha*(color1.r - color0.r);
   g = color0.g + alpha*(color1.g - color0.g);
   b = color0.b + alpha*(color1.b - color0.b);
}

//==============================================================================
// BColor::save
//==============================================================================
bool BColor::save(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      return(false);
   }

   long result;
   CHUNKWRITESAFE(chunkWriter,Float,r);
   CHUNKWRITESAFE(chunkWriter,Float,g);
   CHUNKWRITESAFE(chunkWriter,Float,b);

   return(true);
}


//==============================================================================
// BColor::load
//==============================================================================
bool BColor::load(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   long result;
   CHUNKREADSAFE(chunkReader, Float,r);
   CHUNKREADSAFE(chunkReader, Float,g);
   CHUNKREADSAFE(chunkReader, Float,b);

   return(true);
}


//=============================================================================
// BColor::writeVersion
//=============================================================================
bool BColor::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4021); blogerror("BColor::writeVersion -- bad chunkWriter");}
      return(false);
   }

   long result;
   CHUNKWRITETAGGEDSAFE(chunkWriter,DWORD,BCHUNKTAG("VV"), msSaveVersion);

   return(true);
}


//=============================================================================
// BColor::readVersion
//=============================================================================
bool BColor::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4022); blogerror("BColor::readVersion -- bad chunkReader");}
      return(false);
   }

   long result;
   CHUNKREADTAGGEDSAFE(chunkReader,DWORD,BCHUNKTAG("VV"), msLoadVersion);

   return(true);
}


//=============================================================================
// packRGB
//=============================================================================
DWORD packRGB(const BColor &color)
{
   // Convert to 0 to 255
   DWORD r = (DWORD)(255.0f*color.r);
   DWORD g = (DWORD)(255.0f*color.g);
   DWORD b = (DWORD)(255.0f*color.b);

   // Clamp.
   if(r>255)
      r = 255;
   if(g>255)
      g = 255;
   if(b>255)
      b = 255;

   // Return packed value.

   return((r<<cRedShift)|(g<<cGreenShift)|b);
}

//=============================================================================
// packRGB
//=============================================================================
DWORD packRGB(const float *pcolor)
{
   // Convert to 0 to 255
   DWORD r = (DWORD)(255.0f* pcolor[0]);
   DWORD g = (DWORD)(255.0f* pcolor[1]);
   DWORD b = (DWORD)(255.0f* pcolor[2]);

   // Clamp.
   if(r>255)
      r = 255;
   if(g>255)
      g = 255;
   if(b>255)
      b = 255;

   // Return packed value.

   return((r<<cRedShift)|(g<<cGreenShift)|b);
}

//=============================================================================
// packRGBA
//=============================================================================
DWORD packRGBA(const BColor &color)
{
   // Convert to 0 to 255
   DWORD r = (DWORD)(255.0f*color.r);
   DWORD g = (DWORD)(255.0f*color.g);
   DWORD b = (DWORD)(255.0f*color.b);

   // Clamp.
   if(r>255)
      r = 255;
   if(g>255)
      g = 255;
   if(b>255)
      b = 255;

   // Return packed value.

   return((255 << cAlphaShift)|(r<<cRedShift)|(g<<cGreenShift)|b);
}

//=============================================================================
// packRGBA
//=============================================================================
DWORD packRGBA(const BColor &color, float falpha)
{
   // Convert to 0 to 255
   DWORD r = (DWORD)(255.0f*color.r);
   DWORD g = (DWORD)(255.0f*color.g);
   DWORD b = (DWORD)(255.0f*color.b);
   DWORD a = (DWORD)(255.0f*falpha);

   // Clamp.
   if(r>255)
      r = 255;
   if(g>255)
      g = 255;
   if(b>255)
      b = 255;
   if(a>255)
      a = 255;

   // Return packed value.

   return((a << cAlphaShift)|(r<<cRedShift)|(g<<cGreenShift)|b);
}


//=============================================================================
//=============================================================================
DWORD packRGBA(DWORD color, float falpha)
{
   color &= !cAlphaMask;
   DWORD a = static_cast<DWORD>(255.0f * falpha);
   if (a > 255)
      a = 255;
   return (color | (a << cAlphaShift));
}


//==============================================================================
// packRGB
//==============================================================================
DWORD packRGB(int r, int g, int b)
{
   return((255 << cAlphaShift)|(r<<cRedShift)|(g<<cGreenShift)|b);
}


//==============================================================================
// srgbToLinear
//==============================================================================
void srgbToLinear(DWORD color, float &resultR, float &resultG, float &resultB)
{
   BColor temp(color);
   srgbToLinear(temp.r, temp.g, temp.b, resultR, resultG, resultB);
}


//==============================================================================
// srgbToLinear
//==============================================================================
void srgbToLinear(float r, float g, float b, float &resultR, float &resultG, float &resultB)
{
   // sRGB to linear mojo, per the spec.
   if(r <= 0.04045f)
      resultR = r/12.92f;
   else
      resultR = powf((r+0.55f)/1.055f, 2.4f);
   if(g <= 0.04045f)
      resultG = g/12.92f;
   else
      resultG = powf((g+0.55f)/1.055f, 2.4f);
   if(b <= 0.04045f)
      resultB = b/12.92f;
   else
      resultB = powf((b+0.55f)/1.055f, 2.4f);
}



//==============================================================================
// eof: color.cpp
//==============================================================================
