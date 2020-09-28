// File: imageStitch.cpp
#include "xcore.h"
#include "xcoreLib.h"
#include "consoleOutput.h"
#include "containers\hashMap.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "stream\byteStream.h"
#include "RGBAImage.h"
#include "readTGA.h"
#include "writeTGA.h"

static void printHelp(void)
{
   gConsoleOutput.printf("Usage: imageStitch inputFilenamePrefix outputFilename.tga\n");
   gConsoleOutput.printf("\n");
   
   BConsoleAppHelper::printHelp();
   gConsoleOutput.printf(" -cols #           Number of macrotile columns\n"); 
   gConsoleOutput.printf(" -rows #           Number of macrotile rows\n");    

   gConsoleOutput.printf(" +jitter           Input is jittered data\n");    
   gConsoleOutput.printf(" -jitterAAQual #   Jittered AA Quality\n");    
}

static bool parseCommandLine(
   BCommandLineParser::BStringArray& args,
   BDynamicArray<BString>& filenames,
   uint& numCols, uint& numRows, bool &jitterMode, uint &jitterAAQual)
{
   if (args.size() == 1)
   {
      printHelp();
      return false;
   }
   
   numCols = 0;
   numRows = 0;
   jitterMode=false;
   jitterAAQual=0;
   filenames.clear();
            
   const BCLParam params[] = 
   {
      { "cols", cCLParamTypeIntPtr, &numCols },
      { "rows", cCLParamTypeIntPtr, &numRows },
      { "jitter", cCLParamTypeFlag, &jitterMode },
      { "jitterAAQual", cCLParamTypeIntPtr, &jitterAAQual },

   };
   const uint cNumCommandLineParams = sizeof(params)/sizeof(params[0]);

   BCommandLineParser parser(params, cNumCommandLineParams);
   const bool success = parser.parse(args, true, false);
   if (!success)
   {
      gConsoleOutput.error("%s\n", parser.getErrorString());
      return false;
   }

   if ((numRows == 0) || (numCols == 0))
   {
      gConsoleOutput.error("Must specify cols and rows!\n");
      return false;
   }

   const BDynamicArray<uint>& unparsedParams = parser.getUnparsedParams();
   
   for (uint j = 0; j < unparsedParams.getSize(); j++)
   {
      const uint i = unparsedParams[j];

      if (args[i].isEmpty())
         continue;

      if (filenames.getSize() >= 2)
      {
         gConsoleOutput.error("Too many filenames!\n");
         return false;  
      }

      filenames.pushBack(args[i]);
   }

   if (filenames.getSize() != 2) 
   {
      printHelp();
      return false;
   }
   
   if ((numRows < 1) || (numRows > 24) || (numCols < 1) || (numCols > 24))
   {
      gConsoleOutput.error("rows and cols must be between 1 and 24!\n");
      return false;
   }
   
   return true;
}

static bool stitchImagesJittered(const BDynamicArray<BString>& filenames, uint numCols, uint numRows, uint jitterAAQuality)
{
   BCFileStream outFileStream;
   if (!outFileStream.open(filenames[1], cSFWritable | cSFSeekable))
   {
      gConsoleOutput.error("Unable to open file: %s\n", filenames[1].getPtr());
      return false;  
   }

   uint tileWidth = 0, tileHeight = 0;
   BTGAWriter tgaWriter;

   BDynamicArray<BRGBAImage> images(numRows*numCols);
   BRGBAFImage floatingImage;

   uint currImg =0;
   for (uint curRow = 0; curRow < numRows; curRow++)
   {
      gConsoleOutput.printf("Processing row %u of %u\n", curRow + 1, numRows);

      for (uint curCol = 0; curCol < numCols; curCol++)
      {
         //in jittered mode, we have X samples per real pixel
         floatingImage.clear();
         for(uint currAA = 0; currAA < jitterAAQuality; currAA++)
         {
            BString filename;
            filename.format("%s_%03u.tga", filenames[0].getPtr(), curCol + curRow * numCols + currAA);
            gConsoleOutput.printf("Reading image: %s\n", filename.getPtr());

            BCFileStream inFileStream;
            if (!inFileStream.open(filename))
            {
               gConsoleOutput.error("Unable to open file: %s\n", filename.getPtr());
               return false;  
            }

            BTGAReader tgaReader;
            if (!tgaReader.init(inFileStream))
            {  
               gConsoleOutput.error("Failed reading image: %s\n", filename.getPtr());
               return false;  
            }


            if (!tileWidth)
            {
               tileWidth = tgaReader.getWidth();
               tileHeight = tgaReader.getHeight();  

               floatingImage.setSize(tileWidth,tileHeight);
               gConsoleOutput.printf("Tile resolution is %ux%u\n", tileWidth, tileHeight);
            }
            else 
            {
               if ((tileWidth != tgaReader.getWidth()) || (tileHeight != tgaReader.getHeight()))
               {
                  gConsoleOutput.error("All images must be the same size: %s\n", filename.getPtr());
                  return false;  
               }
            }

            if (tgaReader.getFormat().getBytesPerPixel() != 4)
            {
               gConsoleOutput.error("TGA images must be 4BPP: %s\n", filename.getPtr());
               return false;  
            }

            //load our rep into a running floating point buffer
            for (uint y = 0; y < tileHeight; y++)
            {
               const void* pScanOfs;
               uint scanLen;
               if (!tgaReader.decode(pScanOfs, scanLen))
               {
                  gConsoleOutput.error("Failed decoding image: %s\n", filename.getPtr());
                  return false;  
               }

               const BRGBAColor* pSrcScanline = static_cast<const BRGBAColor*>(pScanOfs);
               BRGBAColorF* pDstScanline = floatingImage.getScanlinePtr(tgaReader.getYFlipped() ? (tileHeight - 1 - y) : y);
               for (uint x = 0; x < tileWidth; x++)
               {
                  uint dstIndx = tgaReader.getXFlipped() ? (tileWidth - 1 - x) : x;
                  pDstScanline[dstIndx].r  += pSrcScanline[x].b;
                  pDstScanline[dstIndx].g  += pSrcScanline[x].g;
                  pDstScanline[dstIndx].b  += pSrcScanline[x].r;
                  pDstScanline[dstIndx].a  += pSrcScanline[x].a;
               }
            }
         }

         //convert the floating point rep back to the image
         images[currImg].setSize(tileWidth, tileHeight);
         BRGBAColorF* pSrc = floatingImage.getPtr();
         BRGBAColor* pDst = images[currImg].getPtr();

         for (uint idex = 0; idex < tileWidth*tileHeight;idex++)
         {
            pDst[idex].r = Math::Clamp<unsigned char>(pSrc[idex].r / jitterAAQuality,0,255);
            pDst[idex].g = Math::Clamp<unsigned char>(pSrc[idex].g / jitterAAQuality,0,255);
            pDst[idex].b = Math::Clamp<unsigned char>(pSrc[idex].b / jitterAAQuality,0,255);
            pDst[idex].a = Math::Clamp<unsigned char>(pSrc[idex].a / jitterAAQuality,0,255);
         }

         currImg++;
      }
   }

   //weave our images into another temporary.
   BRGBAImage dstImage;
   dstImage.setSize(tileWidth * numCols,tileHeight * numRows);
   BRGBAColor* dstPtr = dstImage.getPtr();
   for(uint y = 0; y < tileHeight; y++)
   {
      for(uint x = 0; x < tileWidth; x++)   
      {
         const uint srcIndex = x + y * tileWidth;
         uint imageIdx = 0;
         for (uint curRow = 0; curRow < numRows; curRow++)
         {
            for (uint curCol = 0; curCol < numCols; curCol++)
            {
               const BRGBAColor* col =&images[imageIdx].getPtr()[srcIndex];
               const uint dstIndex = ((x*numCols)+curCol) + (tileWidth * numCols) * ((y*numRows)+curRow);
               dstPtr[dstIndex].r = col->r;
               dstPtr[dstIndex].g = col->g;
               dstPtr[dstIndex].b = col->b;
               dstPtr[dstIndex].a = col->a;

               imageIdx++;
            }
         }
      }
   }


   if (!tgaWriter.isOpened())
   {
      if (!tgaWriter.open(outFileStream, tileWidth * numCols, tileHeight * numRows, cTGAImageTypeBGR))
      {
         gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
         return false;  
      }
   }
   //weave our images back into the origional
  /* for(uint y =0; y<tileHeight;y++)
   {
      if (!tgaWriter.writeLine(dstImage.getScanlinePtr(y)))
      {
         gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
         return false;
      }
   }*/

   BDynamicArray<uchar> dstScanline(tileWidth * numCols * 3);
   for (uint y = 0; y < tileHeight * numRows; y++)
   {
      uchar* pDst = dstScanline.getPtr();
      const BRGBAColor* pSrc = dstImage.getScanlinePtr(y);

      for (uint x = 0; x< tileWidth * numCols; x++)
      {
         pDst[0] = pSrc->b;
         pDst[1] = pSrc->g;
         pDst[2] = pSrc->r;

         pSrc++;
         pDst += 3;
      }
      

      if (!tgaWriter.writeLine(dstScanline.getPtr()))
      {
         gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
         return false;
      }
   }      

   if (!tgaWriter.close())
   {  
      gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
      return false;
   }

   return true;
}

static bool stitchImagesTiled(const BDynamicArray<BString>& filenames, uint numCols, uint numRows)
{
   BCFileStream outFileStream;
   if (!outFileStream.open(filenames[1], cSFWritable | cSFSeekable))
   {
      gConsoleOutput.error("Unable to open file: %s\n", filenames[1].getPtr());
      return false;  
   }
   
   uint tileWidth = 0, tileHeight = 0;
   BTGAWriter tgaWriter;
         
   BDynamicArray<BRGBAImage> images(numCols);
   
   for (uint curRow = 0; curRow < numRows; curRow++)
   {
      gConsoleOutput.printf("Processing row %u of %u\n", curRow + 1, numRows);
      
      for (uint curCol = 0; curCol < numCols; curCol++)
      {
         BString filename;
         filename.format("%s_%03u.tga", filenames[0].getPtr(), curCol + curRow * numCols);
         
         gConsoleOutput.printf("Reading image: %s\n", filename.getPtr());
         
         BCFileStream inFileStream;
         if (!inFileStream.open(filename))
         {
            gConsoleOutput.error("Unable to open file: %s\n", filename.getPtr());
            return false;  
         }
         
         BTGAReader tgaReader;
         if (!tgaReader.init(inFileStream))
         {  
            gConsoleOutput.error("Failed reading image: %s\n", filename.getPtr());
            return false;  
         }
         
         if (!tileWidth)
         {
            tileWidth = tgaReader.getWidth();
            tileHeight = tgaReader.getHeight();  
            
            gConsoleOutput.printf("Tile resolution is %ux%u\n", tileWidth, tileHeight);
         }
         else 
         {
            if ((tileWidth != tgaReader.getWidth()) || (tileHeight != tgaReader.getHeight()))
            {
               gConsoleOutput.error("All images must be the same size: %s\n", filename.getPtr());
               return false;  
            }
         }
         
         if (tgaReader.getFormat().getBytesPerPixel() != 4)
         {
            gConsoleOutput.error("TGA images must be 4BPP: %s\n", filename.getPtr());
            return false;  
         }
         
         images[curCol].setSize(tileWidth, tileHeight);
                           
         for (uint y = 0; y < tileHeight; y++)
         {
            const void* pScanOfs;
            uint scanLen;
            if (!tgaReader.decode(pScanOfs, scanLen))
            {
               gConsoleOutput.error("Failed decoding image: %s\n", filename.getPtr());
               return false;  
            }
            
            const uchar* pSrcScanline = static_cast<const uchar*>(pScanOfs);
            BRGBAColor* pDstScanline = images[curCol].getScanlinePtr(tgaReader.getYFlipped() ? (tileHeight - 1 - y) : y);
            for (uint x = 0; x < tileWidth; x++)
            {
               BRGBAColor c;
               c.r = pSrcScanline[2];
               c.g = pSrcScanline[1];
               c.b = pSrcScanline[0];
               c.a = pSrcScanline[3];
                              
               pDstScanline[tgaReader.getXFlipped() ? (tileWidth - 1 - x) : x]  = c;
               
               pSrcScanline += 4;
            }
         }
      }
      
      if (!tgaWriter.isOpened())
      {
         if (!tgaWriter.open(outFileStream, tileWidth * numCols, tileHeight * numRows, cTGAImageTypeBGR))
         {
            gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
            return false;  
         }
      }
      
      BDynamicArray<uchar> dstScanline(tileWidth * numCols * 3);
      for (uint y = 0; y < tileHeight; y++)
      {
         for (uint col = 0; col < numCols; col++)
         {
            uchar* pDst = dstScanline.getPtr() + col * tileWidth * 3;
            const BRGBAColor* pSrc = images[col].getScanlinePtr(y);
            
            for (uint x = 0; x < tileWidth; x++)
            {
               pDst[0] = pSrc->b;
               pDst[1] = pSrc->g;
               pDst[2] = pSrc->r;
               
               pSrc++;
               pDst += 3;
            }
         }
         
         if (!tgaWriter.writeLine(dstScanline.getPtr()))
         {
            gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
            return false;
         }
      }      
   }
   
   if (!tgaWriter.close())
   {  
      gConsoleOutput.error("Unable to write to file: %s\n", filenames[1].getPtr());
      return false;
   }
      
   return true;
}

int main(int argc, const char *argv[])
{
   XCoreCreate();
   
   BConsoleAppHelper::setup();
   
   gConsoleOutput.printf("ImageStitch Compiled %s %s\n", __DATE__, __TIME__);      
   
   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argc, argv))
   {
      BConsoleAppHelper::deinit();
      XCoreRelease();
      return EXIT_FAILURE;
   }
      
   uint numCols = 0, numRows = 0;
   bool jitterMode = false;
   uint jitterAAQual=0;
   BDynamicArray<BString> filenames;
   if (!parseCommandLine(args, filenames, numCols, numRows, jitterMode, jitterAAQual))
   {
      BConsoleAppHelper::deinit();
      XCoreRelease();
      return EXIT_FAILURE;
   }

   bool OK =false;
   if(jitterMode)
      OK=stitchImagesJittered(filenames, numCols, numRows,jitterAAQual );
   else
      OK=stitchImagesTiled(filenames, numCols, numRows);

   if (OK)
   {
      BConsoleAppHelper::deinit();
      XCoreRelease();
      return EXIT_FAILURE;
   }
      
   BConsoleAppHelper::deinit();
   
   XCoreRelease();
   
   return EXIT_SUCCESS;
}



