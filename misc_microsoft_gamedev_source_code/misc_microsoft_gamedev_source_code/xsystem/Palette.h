// =============================================================================
// palette.h
//
// Copyright (c) 2002 Ensemble Studios
//
// created by Matt Pritchard 7/2002
// =============================================================================

#pragma once 

#ifndef _PALETTE_H_
#define _PALETTE_H_

// =============================================================================
// Forward declarations
// =============================================================================


// =============================================================================
// supporting Structures
// =============================================================================

struct BRGBQuad
{
   BYTE	Blue;
   BYTE	Green;
	BYTE	Red;
   BYTE	Alpha;
};


// =============================================================================
// BPalette Class
//
// Provedes services for managine up to 256-entry RGB palettes
// =============================================================================


class BPalette
{
	public:           
      explicit                BPalette(void);
                              ~BPalette(void);
                              
      BPalette&               operator= (BPalette &SourcePal);
                              
      void                    setSize(long newSize);
      long                    getSize(void) const {return(mPaletteSize);}
                              
     	void	                  reset(void);
      void                    initGreyScale(void);

      // Take a 128 color palette and copy it to the upper 128 with alpha=255
      bool                    dupeWithAlpha255(void);
                              
      void                    setColor(long Index, long Red, long Green, long Blue, long Alpha);
                              
      bool	                  findNearestColor(BYTE &color, BYTE red, BYTE green, BYTE blue);
                              
      long                    getSize(void)              {return (mPaletteSize);}
      BRGBQuad*               getPaletteData(void)       {return (mPalette);}
      DWORD*                  getPaletteRGBA(void)     {return (mPaletteRGBAValue);};
      WORD*                   getPalette565(void)      {return (mPalette565Value);};
      WORD*                   getPalette555(void)      {return (mPalette555Value);};
      WORD*                   getPalette1555(void)     {return (mPalette1555Value);};
      WORD*                   getPalette444(void)      {return (mPalette444Value);};
                    
 	protected:
      enum
      {
         cMaxPaletteSize = 256
      };
      enum
      {
         cBlueShift = 0, 
         cGreenShift = 8, 
         cRedShift = 16, 
         cAlphaShift = 24
      };
      enum
      {
         cBlue565Shift = 0, 
         cGreen565Shift = 5, 
         cRed565Shift = 11
      };
      enum
      {
         cBlue555Shift = 0, 
         cGreen555Shift = 5, 
         cRed555Shift = 10, 
         cAlpha555Shift = 15
      };
      enum
      {
         cBlue444Shift = 0, 
         cGreen444Shift = 4, 
         cRed444Shift = 8
      };

      long			   	      mPaletteSize;				// Number of entires that will be valid
  		BRGBQuad		   	      mPalette[cMaxPaletteSize];				// R, G, B, & Used values

      DWORD				         mPaletteRGBAValue[cMaxPaletteSize];  // 32-bit value for use in TIntImage Surface

      WORD                    mPalette565Value[cMaxPaletteSize];   // 16-bit x565 values
      WORD                    mPalette555Value[cMaxPaletteSize];   // 16-bit x555 values

      WORD                    mPalette1555Value[cMaxPaletteSize];  // 16-bit 1555 values
      WORD                    mPalette444Value[cMaxPaletteSize];   // 16-bit x444 values


      int                  equalize6BitColor(int color);
      int                  equalize5BitColor(int color);
      int                  equalize4BitColor(int color);
      int                  convertAlphaToOneBit(int Alpha);
};                   



#endif