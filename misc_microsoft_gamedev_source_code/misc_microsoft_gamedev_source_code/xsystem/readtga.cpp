//==============================================================================
// Copyright (c) 1997-2002 Ensemble Studios
//
// Targa file reader
//==============================================================================

#include "xsystem.h"
#include "readtga.h"
#include "image.h"

// jce [12/9/2002] -- TODO: reenable this
#define logBuildError ((void)0)


//==============================================================================
// Internally used class
//==============================================================================
struct BTGAInfo
{
      WORD                    mWidth;
      WORD                    mHeight;
      BYTE                    mBpp;                  
      bool                    mRLE;
      bool                    mFlipHorizontal;
      bool                    mFlipVertical;
};


//==============================================================================
// Internally used function headers
//==============================================================================
bool        read32BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        read24BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        read16BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        readRLE32BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        readRLE24BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        readRLE16BPP(BFile &file, DWORD *data, WORD width, WORD height);
bool        getTGAInfo(BFile &file, long dirID, const BCHAR_T *filename, BTGAInfo &info);
bool        readTGAData(BFile &file, DWORD *data, const BTGAInfo &info);
bool        readTGABlockData(BFile &file, DWORD *data, const BTGAInfo &info, long xOffset, long yOffset, 
               long xSize, long ySize);


//==============================================================================
// getTGAInfo
//==============================================================================
bool getTGAInfo(BFile &file, long dirID, const BCHAR_T *filename, BTGAInfo &info)
{
   // open the file
   bool ok=file.openReadOnly(dirID, filename);

   // bail out if the open failed
   if(!ok)
   {
#ifdef _BANG      
      blog
#else
      logBuildError
#endif
         ("error - readTGA: unable to open file for read '%s'", filename);
      return(false);
   }
   
   // get the identification info length
   BYTE idLength;
   bool result = file.read(&idLength, 1);
   if(!result)
      goto error;

   // get the color map type
   BYTE colorMapType;
   result = file.read(&colorMapType, 1);
   if(!result)
      goto error;

   // get the image type
   BYTE imageType;
   result = file.read(&imageType, 1);
   if(!result)
      goto error;
   // check to see if this is a format we support
   switch(imageType)
   {
      case 2:
         info.mRLE = false;
         break;
      case 10:
         info.mRLE = true;
         break;
      default:
         
#ifdef _BANG      
         blog
#else
         logBuildError
#endif
            ("error - readTGA: unsupported image type %ld", imageType);
         BASSERT(0);
         goto error;
   }

   long colorMapSize;
   if(colorMapType == 0)   // if no color map
   {
      // ignore the color map info table
      file.setOffset(5, BFILE_OFFSET_CURRENT);
      colorMapSize = 0;
   }
   else
   {
      // skip color map origin
      file.setOffset(2, BFILE_OFFSET_CURRENT);
      WORD mapCount, mapSize;
      result = file.read(&mapCount, 2);
      if(!result)
         goto error;
      result = file.read(&mapSize, 2);
      if(!result)
         goto error;

      LittleEndianToNative(&mapCount, "s");
      LittleEndianToNative(&mapSize, "s");
      
      // compute the color map size
      colorMapSize = mapCount * (mapSize/8);
   }

   // skip the x and y origin values
   
   file.setOffset(4, BFILE_OFFSET_CURRENT);

   // get the width and height
   WORD width;
   result = file.read(&width, 2);
   if(!result)
      goto error;

   WORD height;
   result = file.read(&height, 2);
   if(!result)
      goto error;

   LittleEndianToNative(&width, "s");
   LittleEndianToNative(&height, "s");

   info.mWidth=width;
   info.mHeight=height;

   // get the bits per pixel
   BYTE bpp;
   result = file.read(&bpp, 1);
   if(!result)
      goto error;
   info.mBpp=bpp;

   // get the image descriptor byte
   BYTE descriptor;
   result = file.read(&descriptor, 1);
   if(!result)
      goto error;

   // look at the interleaving, fail if the image is interleaved -- not
   // supported for now
   if( (descriptor & 0xC0) != 0 )
      goto error;

   // Check for left to right flip.
   if(descriptor&0x10)
      info.mFlipHorizontal=true;
   else
      info.mFlipHorizontal=false;

   // Check for top to bottom flip.
   // jce [1/29/2003] -- reversed the sense of this so we read textures in the proper orientation since
   // tgas are actually bottom up by default and we want them top down.  This will break the universe until we fix 
   // everywhere that is compensating for this.
   if(descriptor&0x20)
      info.mFlipVertical=false;
   else
      info.mFlipVertical=true;

   // skip the identification string and the the color map -- some non-palettized
   // images have a color map, which we'll just ignore
   
   file.setOffset(idLength + colorMapSize, BFILE_OFFSET_CURRENT);

   // Success.
   return(true);

   error:
      
#ifdef _BANG      
      blog
#else
      logBuildError
#endif
         ("error - readTGA: attempt to read unsupported TGA format '%s'", filename);

      return(false);
}


//==============================================================================
// readTGA
//==============================================================================
BImage *readTGA(long dirID, const BCHAR_T *filename)
{
   BTGAInfo info;
   BFile file;
   bool ok=getTGAInfo(file, dirID, filename, info);
   if(!ok)
      return(NULL);

   BImage *image=NULL;
   bool result;

   // allocate the image struct
   image = new BImage;
   if(!image)
      return(NULL);

   // allocate image data
   if(!image->allocateData(info.mWidth, info.mHeight))
   {
      delete image;
      return(NULL);
   }

   // Read the data.
   result=readTGAData(file, image->getRawData(), info);

   // jce 11/15/2000 -- handle the flip flags.  We would be cooler if we did this as we read the stuff in, but what the hell...
   // p.s. who thought these were good flags to have in an image file?
   if(info.mFlipHorizontal)
      image->flipHorizontal();
   if(info.mFlipVertical)
      image->flipVertical();

   // If we failed, clean up and return null.
   if(!result)
   {
      delete image;
      return(NULL);
   }

   // We succeeded, return the image.
   return(image);
}

//==============================================================================
// readTGAData
//==============================================================================
bool readTGAData(BFile &file, DWORD *data, const BTGAInfo &info)
{
   // Check params
   if(!data || !file.isOpen())
   {
      BASSERT(0);
      return(false);
   }

   // read in the image data as appropriate
   bool result=false;
   switch(info.mBpp)
   {
      case 32:
         if(info.mRLE)
            result = readRLE32BPP(file, data, info.mWidth, info.mHeight);
         else
            result = read32BPP(file, data, info.mWidth, info.mHeight);
         break;

      case 24:
         if(info.mRLE)
            result = readRLE24BPP(file, data, info.mWidth, info.mHeight);
         else
            result = read24BPP(file, data, info.mWidth, info.mHeight);
         break;

      case 16:
         if(info.mRLE)
            result = readRLE16BPP(file, data, info.mWidth, info.mHeight);
         else
            result = read16BPP(file, data, info.mWidth, info.mHeight);
         break;

      default:
         
#ifdef _BANG      
      blog
#else
      logBuildError
#endif
            ("error - readTGA: unsupported image depth %ld", info.mBpp);
         result=false;
   }

   return(result);
}


//==============================================================================
// readTGABlockData
//==============================================================================
bool readTGABlockData(BFile &file, DWORD *data, const BTGAInfo &info, long xOffset, long yOffset, 
   long xSize, long ySize)
{
   // Check params
   if(!data || !file.isOpen())
   {
      BASSERT(0);
      return(false);
   }
   
   long xEnd = xOffset+xSize;
   long yEnd = yOffset+ySize;
   
   // Overflow check.
   if((xEnd < xOffset) || (xEnd < xSize) || (yEnd < yOffset) || (yEnd < ySize))
   {
      BFAIL("int overflow in readTGABlockData");
      return(false);
   }

   // Check that the block we've asked for is valid.
   if(xOffset<0 || yOffset<0 || xSize<0 || ySize<0 || xSize>=info.mWidth || yEnd>=info.mHeight)
   {
#ifdef _BANG      
      blog
#else
      logBuildError
#endif
         ("error -- readTGABlockData: invalid block!");
      BASSERT(0);
      return(false);
   }

   // Check that the TGA is not compressed.
   if(info.mRLE)
   {
#ifdef _BANG      
      blog
#else
      logBuildError
#endif
         ("error -- readTGABlockData: can't read RLE block!");
      BASSERT(0);
      return(false);
   }

   // Get the current file pointer position.  This is the start of the actual
   // image data.
   DWORD startPos;
   bool ok=file.getOffset(startPos);
   if(!ok)
      return(false);

   long tgaBytesPerPixel;
   switch(info.mBpp)
   {
      case 32:
         tgaBytesPerPixel=4;
         break;

      case 24:
         tgaBytesPerPixel=3;
         break;

      case 16:
         tgaBytesPerPixel=2;
         break;

      default:
         
#ifdef _BANG      
         blog
#else
         logBuildError
#endif
            ("error - readTGABlockData: unsupported image depth %ld", info.mBpp);
         return(false);
   }

   // Read in each row of pixels.
   DWORD *currLine=data;
   for(long y=yOffset; y<yOffset+ySize; y++, currLine+=xSize)
   {
      // Jump to the right spot in the image data.
      long offset=tgaBytesPerPixel*(y*info.mWidth + xOffset);
      file.setOffset(startPos+offset, BFILE_OFFSET_CURRENT);

      bool result=false;
      switch(info.mBpp)
      {
         case 32:
            result = read32BPP(file, currLine, (WORD)xSize, 1);
            break;

         case 24:
            result = read24BPP(file, currLine, (WORD)xSize, 1);
            break;

         case 16:
            result = read16BPP(file, currLine, (WORD)xSize, 1);
            break;
      }
      if(!result)
      {
#ifdef _BANG      
         blog
#else
         logBuildError
#endif
            ("error - readTGABlockData: error reading data");
         return(false);
      }
   }

   return(true);
}

//==============================================================================
// read32BPP
//==============================================================================
bool read32BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   bool result = file.read(data, width*height*sizeof(DWORD));
   return result;
}


//==============================================================================
// read24BPP
//==============================================================================
bool read24BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   long widthBytes = width*3;
   bool result;

   // get a buffer for a single line
   unsigned char *line = new unsigned char[widthBytes];
   if(!line)
      return false;

//   DWORD *ptr = data;
   for(long y=0; y<height; y++)
   {
      result = file.read(line, widthBytes);
      if(!result)
      {
         delete []line;
         return false;
      }

      // warning - the following lines may create a spurious (and incorrect) BC error
      // since texture data can contain the BC fill character as normal data

      for(long x=0; x<widthBytes; x+=3)
         *(data++) = line[x] | (line[x+1] << 8) | (line[x+2] << 16);
   }

   delete []line;
   return true;
}


//==============================================================================
// read16BPP
//==============================================================================
bool read16BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   long widthBytes = width*2;
   bool result;

   // get a buffer for a single line
   WORD *line = new WORD[width];
   if(!line)
      return false;

   DWORD *ptr = data;
   for(long y=0; y<height; y++)
   {
      result = file.read(line, widthBytes);
      if(!result)
      {
         delete []line;
         return false;
      }
      for(long x=0; x<width; x++)
      {
         *ptr = (((DWORD)(line[x] & 0x8000)) << 16) | (((DWORD)(line[x] & 0x7c00)) << 9) | (((DWORD)(line[x] & 0x03e0)) << 6) | (((DWORD)(line[x] & 0x001f)) << 3);
         ptr++;
      }
   }

   delete []line;
   return true;
}


//==============================================================================
// readRLE32BPP
//==============================================================================
bool readRLE32BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   BYTE header;
   long count;
   bool ok;
   bool runPacket;
   
   // Run through each scanline.
   for(long y=0; y<height; y++)
   {
      // Get start of scanline.
      DWORD *curr=data+y*width;
      DWORD *endOfLine=curr+width;

      while(curr<endOfLine)
      {
         // Read header.
         ok=file.read(&header, 1);
         if(!ok)
         {
#ifdef _BANG      
            blog
#else
            logBuildError
#endif
               ("readTGA: readRLE32Bpp: failed reading data.");
            return(false);
         }

         // See if this is an run packet or a raw packet by checking bit 7.
         if(header&0x80)
            runPacket=true;
         else
            runPacket=false;
         
         // Get count -- which is stored in the lower 7 bits of header and offset by one.
         count=(header&0x7F)+1;

         // Make sure count won't overrun array.
         if(curr+count>endOfLine)
         {
            count=endOfLine-curr;
#ifdef _BANG      
            blog
#else
            logBuildError
#endif
               ("readTGA: readRLE32Bpp: ignoring extraneous info in scanline %d.", y);
         }
   
         // If a run packet read the one pixel and copy, otherwise read the data.
         if(runPacket)
         {
            // Read the pixel into the current pixel in the image.
            ok=file.read(curr, 4);         
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE32Bpp: failed reading data.");
               return(false);
            }

            // Now copy the pixel.
            for(long i=1; i<count; i++)
               curr[i]=curr[0];
         }
         else
         {
            ok=file.read(curr, 4*count);
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE32Bpp: failed reading data.");
               return(false);
            }
         }

         // Move curr to new starting point.
         curr+=count;
      }
   }

   return(true);
}


//==============================================================================
// readRLE24BPP
//==============================================================================
bool readRLE24BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   const unsigned long cPixelBytes=3;
   BYTE header;
   long count;
   bool ok;
   bool runPacket;
   
   // get a buffer for a single line
   unsigned char *line = new unsigned char[cPixelBytes*width];
   if(!line)
      return false;
   unsigned char *endOfLine=line+cPixelBytes*width;

   // Run through each scanline.
   for(long y=0; y<height; y++)
   {
      // Get start of scanline.
      unsigned char *curr=line;

      while(curr<endOfLine)
      {
         // Read header.
         ok=file.read(&header, 1);
         if(!ok)
         {
#ifdef _BANG      
            blog
#else
            logBuildError
#endif
               ("readTGA: readRLE24Bpp: failed reading data.");
            delete []line;
            return(false);
         }

         // See if this is a run packet or a raw packet by checking bit 7.
         if(header&0x80)
            runPacket=true;
         else
            runPacket=false;
         
         // Get count -- which is stored in the lower 7 bits of header and offset by one.
         count=(header&0x7F)+1;

         // Make sure count won't overrun array.
         if(curr+count*cPixelBytes>endOfLine)
         {
            count=(endOfLine-curr)/cPixelBytes;
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
               ("readTGA: readRLE24Bpp: ignoring extraneous info in scanline %d.", y);
         }
   
         // If a run packet read the one pixel and copy, otherwise read the data.
         if(runPacket)
         {
            // Read the pixel into the current pixel in the image.
            ok=file.read(curr, cPixelBytes);         
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE24Bpp: failed reading data.");
               delete []line;
               return(false);
            }

            // Now copy the pixel.
            for(long i=1; i<count; i++)
            {
               curr[i*3]=curr[0];
               curr[i*3+1]=curr[1];
               curr[i*3+2]=curr[2];
            }
         }
         else
         {
            ok=file.read(curr, cPixelBytes*count);
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE24Bpp: failed reading data.");
               delete []line;
               return(false);
            }
         }

         // Move curr to new starting point.
         curr+=count*cPixelBytes;
      }

      // Expand line into image.
      DWORD *ptr=data+y*width;
      for(long x=0; x<width*(long)cPixelBytes; x+=3)
      {
         *ptr = line[x] | (line[x+1] << 8) | (line[x+2] << 16);
         ptr++;
      }
   }

   delete []line;
   return(true);
}


//==============================================================================
// readRLE16BPP
//==============================================================================
bool readRLE16BPP(BFile &file, DWORD *data, WORD width, WORD height)
{
   const int cPixelBytes=2;
   BYTE header;
   long count;
   bool ok;
   bool runPacket;

   // get a buffer for a single line
   WORD *line = new WORD[cPixelBytes*width];
   if(!line)
      return(false);
   WORD *endOfLine=line+width;
   
   // Run through each scanline.
   for(long y=0; y<height; y++)
   {
      // Get start of scanline.
      WORD *curr=line;

      while(curr<endOfLine)
      {
         // Read header.
         ok=file.read(&header, 1);
         if(!ok)
         {
#ifdef _BANG      
            blog
#else
            logBuildError
#endif
               ("readTGA: readRLE16Bpp: failed reading data.");
            delete []line;
            return(false);
         }

         // See if this is a run packet or a raw packet by checking bit 7.
         if(header&0x80)
            runPacket=true;
         else
            runPacket=false;
         
         // Get count -- which is stored in the lower 7 bits of header and offset by one.
         count=(header&0x7F)+1;

         // Make sure count won't overrun array.
         if(curr+count>endOfLine)
         {
            count=(endOfLine-curr)/cPixelBytes;
#ifdef _BANG      
            blog
#else
            logBuildError
#endif
               ("readTGA: readRLE16Bpp: ignoring extraneous info in scanline %d.", y);
         }
   
         // If a run packet read the one pixel and copy, otherwise read the data.
         if(runPacket)
         {
            // Read the pixel into the current pixel in the image.
            ok=file.read(curr, cPixelBytes);         
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE16Bpp: failed reading data.");
               delete []line;
               return(false);
            }

            // Now copy the pixel.
            for(long i=1; i<count; i++)
               curr[i]=curr[0];
         }
         else
         {
            ok=file.read(curr, cPixelBytes*count);
            if(!ok)
            {
#ifdef _BANG      
               blog
#else
               logBuildError
#endif
                  ("readTGA: readRLE16Bpp: failed reading data.");
               delete []line;
               return(false);
            }
         }

         // Move curr to new starting point.
         curr+=count;
      }

      // Expand data.
      DWORD *ptr=data+width*y;
      for(long x=0; x<width; x++)
      {
         *ptr = (((DWORD)(line[x] & 0x8000)) << 16) | (((DWORD)(line[x] & 0x7c00)) << 9) | (((DWORD)(line[x] & 0x03e0)) << 6) | (((DWORD)(line[x] & 0x001f)) << 3);
         ptr++;
      }
   }

   delete []line;
   return(true);
}


//==============================================================================
// getTGADimensions
//==============================================================================
bool getTGADimensions(long dirID, const BCHAR_T *filename, long &width, long &height)
{
   // Open file and get info.
   BTGAInfo info;
   BFile file;
   bool ok=getTGAInfo(file, dirID, filename, info);
   if(!ok)
      return(false);
   
   // Copy width and height.
   width=info.mWidth;
   height=info.mHeight;

   // Success.
   return(true);
}

//==============================================================================
// readTGA
//==============================================================================
bool readTGA(long dirID, const BCHAR_T *filename, DWORD *data)
{
   // Check params.
   if(!filename || !data)
      return(false);

   // Open file and get info.
   BTGAInfo info;
   BFile file;
   bool ok = getTGAInfo(file, dirID, filename, info);
   if(!ok)
      return(false);

   // Read in the data.
   bool result=readTGAData(file, data, info);

   // Return the result.
   return(result);
}


//==============================================================================
// readTGABlock
//==============================================================================
bool readTGABlock(long dirID, const BCHAR_T *filename, DWORD *data, long xOffset, long yOffset, 
   long xSize, long ySize)
{
   // Check params.
   if(!filename || !data)
      return(false);

   // Open file and get info.
   BTGAInfo info;
   BFile file;
   bool ok = getTGAInfo(file, dirID, filename, info);
   if(!ok)
      return(false);

   // Read the block in.
   bool result=readTGABlockData(file, data, info, xOffset, yOffset, xSize, ySize);

   // Return the result.
   return(result);
}

//==============================================================================
// eof: readtga.cpp
//==============================================================================
