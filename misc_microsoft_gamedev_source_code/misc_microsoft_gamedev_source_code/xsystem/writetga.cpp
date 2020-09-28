//==============================================================================
// Copyright (c) 1997-2001 Ensemble Studios
//
// Targa output
//==============================================================================

#include "xsystem.h"
#include "writetga.h"
#include "image.h"

//==============================================================================
// writeTGA(long dirID, char *filename, BImage *image)
// 
// Writes out a 32 bit targa of the given image into a file with the given name.
// Returns true on success, false on failure.
//==============================================================================
bool writeTGA(long dirID, const BCHAR_T *filename, BImage *image)
{
   if(!image || !filename)
   {
#ifdef _BANG
      {setBlogError(4124); blogerror("error - writeTGA: no image and/or filename supplied");}
#endif
      return false;
   }

   // open the file
   BFile file;
   bool ok=file.openWriteable(dirID, filename);

   // bail out if the open failed
   if(!ok)
   {
#ifdef _BANG
      {setBlogError(4125); blogerror("error - writeTGA: cannot open file %s", BStrConv::toA(filename));}
#endif
      return false;
   }

   BYTE tempByte;
   BYTE tempBytes5[5];
   WORD tempWord;
   bool result;

   // write desription length of 0
   tempByte = 0;
   result = file.write(&tempByte,1);
   if(!result)
      goto error;

   // no color map
   tempByte = 0;
   result = file.write(&tempByte,1);
   if(!result)
      goto error;

   // uncompressed RGB   
   tempByte = 2;
   result = file.write(&tempByte,1);
   if(!result)
      goto error;

   // color map specification -- ignored for RGB
   memset(tempBytes5,0,5);
   result = file.write(tempBytes5,5);
   if(!result)
      goto error;

   // x origin of image
   tempWord = 0;
   result = file.write(&tempWord,2);
   if(!result)
      goto error;

   // y origin of image
   tempWord = 0;
   result = file.write(&tempWord,2);
   if(!result)
      goto error;

   // width of image
   tempWord = static_cast<WORD>(image->getWidth());
   NativeToLittleEndian(&tempWord, "s");
   result = file.write(&tempWord,2);
   if(!result)
      goto error;

   // height of image
   tempWord = static_cast<WORD>(image->getHeight());
   NativeToLittleEndian(&tempWord, "s");
   result = file.write(&tempWord,2);
   if(!result)
      goto error;

   // bpp
   tempByte = 32;
   result = file.write(&tempByte,1);
   if(!result)
      goto error;

   // image descriptor
   tempByte = 8;   // eight bits of alpha, non-interleaved, origin in upper-left
   result = file.write(&tempByte,1);
   if(!result)
      goto error;

   // the image getData
   ok=true;
   long y;
   for(y=image->getHeight()-1; y>=0; y--)
   {
      result = file.write(image->getRawData()+y*image->getWidth(), image->getWidth()*sizeof(DWORD));
      if(!result)
      {
         ok=false;
         break;
      }
   }
   if(ok)
      return true;

   error:
#ifdef _BANG
      {setBlogError(4126); blogerror("error - writeTGA: unable to write data to file");}
#endif

      return false;

} // writeTGA

