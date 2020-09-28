// =============================================================================
// palette.cpp
//
// Copyright (c) 2002 Ensemble Studios
//
// created by Matt Pritchard 7/2002
// =============================================================================

#include "xsystem.h"
#include "palette.h"








//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// BPalette - Class that handles the Palette attached to an image
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



// =============================================================================
// BPalette::Constructor
// =============================================================================
BPalette::BPalette(void) :
   mPaletteSize(0)
{
	reset();
}


// =============================================================================
// BPalette::Destructor
// =============================================================================
BPalette::~BPalette(void)
{
}


// =============================================================================
// BPalette::reset - Clears the palette to empty
// =============================================================================
void BPalette::reset(void)
{

   mPaletteSize = 256;

   for (int z =0; z < 256; z++)
   {
      setColor(z, 0, 0, 0, false);
   }

   mPaletteSize = 0;

}


// =============================================================================
// BPalette::initGreyScale - Creates a grey scale (ramp) palette
// =============================================================================
void BPalette::initGreyScale(void)
{
   mPaletteSize = 256;

   for (int z =0; z < 256; z++)
   {
      setColor(z, z, z, z, true);
   }

}


// =============================================================================
// BPalette::setSize - sets number of suported colors
// =============================================================================
void BPalette::setSize(long newSize)
{
   if (newSize < 0 || newSize > 256)
   {
      BASSERT(0);
      return;
   }

   mPaletteSize = newSize;
}





// =============================================================================
// BPalette::setColor - sets a palette color
// =============================================================================
void BPalette::setColor(long Index, long Red, long Green, long Blue, long Alpha)
{
	if ((Index < 0) || (Index >= mPaletteSize)) return;

   if (Index >= cMaxPaletteSize)
   {
      //ErrorMessage("help! PalSize");
      return;
   }

 	mPalette[Index].Red   = (BYTE) (Red & 0x00FF);
  	mPalette[Index].Green = (BYTE) (Green & 0x00FF);
  	mPalette[Index].Blue  = (BYTE) (Blue & 0x00FF);
  	mPalette[Index].Alpha = (BYTE) (Alpha & 0x00FF);

   mPaletteRGBAValue[Index] = (DWORD) ( ((Blue & 0x00FF) << cBlueShift) | ((Green & 0x00FF) << cGreenShift) |
                              ((Red & 0x00FF) << cRedShift) | ((Alpha & 0x00FF) << cAlphaShift) );

   mPalette565Value[Index]  = (WORD) ( (equalize5BitColor(Blue) << cBlue565Shift) | (equalize6BitColor(Green) << cGreen565Shift) |
                              (equalize5BitColor(Red) << cRed565Shift) );

   mPalette555Value[Index]  = (WORD) ( (equalize5BitColor(Blue) << cBlue555Shift) | (equalize5BitColor(Green) << cGreen555Shift) |
                              (equalize5BitColor(Red) << cRed555Shift) );

   mPalette1555Value[Index] =  (WORD) ( mPalette555Value[Index] | (convertAlphaToOneBit(Alpha) << cAlpha555Shift) );

   mPalette444Value[Index]  = (WORD) ( (equalize4BitColor(Blue) << cBlue444Shift) | (equalize4BitColor(Green) << cGreen444Shift) |
                              (equalize4BitColor(Red) << cRed444Shift) );
}


// =============================================================================
// BPalette::findNearestColor - locates the closest distance match
// =============================================================================
bool BPalette::findNearestColor(BYTE &color, BYTE red, BYTE green, BYTE blue)
{
	bool 	success = false;
   long	Distance;
   long 	ClosestDistance = 0x7FFFFFFF;
   long	Rd, Gd, Bd;


   for (long n = 0; n < mPaletteSize; n++)
   {
      Rd = mPalette[n].Red - red;
      Gd = mPalette[n].Green - green;
      Bd = mPalette[n].Blue - blue;
      Distance = (Rd*Rd) + (Gd*Gd) + (Bd*Bd);
      if (Distance < ClosestDistance)
      {
         ClosestDistance = Distance;
         color = (BYTE)n;
         success = true;
      }
   }

	return(success);
}



// =============================================================================
// BPalette::Operator= - copies a palette object
// =============================================================================
BPalette& BPalette::operator=(BPalette &Source)
{
   mPaletteSize = Source.mPaletteSize;

   for (int z =0; z < 256; z++)
   {
   	mPalette[z]      = Source.mPalette[z];
   }


   return (*this);
}



//=============================================================================
// BPalette::equalize6BitColor
//=============================================================================
int BPalette::equalize6BitColor(int color)
{
   int eColor = color;

   //if (mAdjustForColorTruncation)
   {
       eColor = min( (color & 0x00FF) + 2, 255);    // 2 = 1/2 of rounded off amount (2 bits = 4)
   }

   return (eColor >> 2);
}


//=============================================================================
// BPalette::equalize5BitColor
//=============================================================================
int BPalette::equalize5BitColor(int color)
{
   int eColor = color;

   //if (mAdjustForColorTruncation)
   {
      eColor = min( (color & 0x00FF) + 4, 255);    // 4 = 1/2 of rounded off amount (3 bits = 8)
   }

   return (eColor >> 3);
}


//=============================================================================
// BPalette::equalize4BitColor
//=============================================================================
int BPalette::equalize4BitColor(int color)
{
   int eColor = color;

   //if (mAdjustForColorTruncation)
   {
      eColor = min( (color & 0x00FF) + 8, 255);    // 8 = 1/2 of rounded off amount (4 bits = 16)
   }

   return (eColor >> 4);
}

//=============================================================================
// BPalette::convertAlphaToOneBit
//=============================================================================
int BPalette::convertAlphaToOneBit(int Alpha)
{
   int a = Alpha & 0x00FF;

   /*
   if (mRaiseAnyAlphaTo1)
   {
      return (Alpha > 0);
   }
   */

   return (a >> 7);
}


//=============================================================================
// BPalette::dupeWithAlpha255
//=============================================================================
bool BPalette::dupeWithAlpha255(void)
{
   // Can't dupe >128 colors.
   if(mPaletteSize>128)
      return(false);

   // Save original size.
   long origSize=mPaletteSize;

   // Double our size.
   mPaletteSize*=2;

   // Set the new entries
   for(long i=0; i<origSize; i++)
   {
      setColor(origSize+i, mPalette[i].Red, mPalette[i].Green, mPalette[i].Blue, 255);
   }
   
   // Success.
   return(true);
}
