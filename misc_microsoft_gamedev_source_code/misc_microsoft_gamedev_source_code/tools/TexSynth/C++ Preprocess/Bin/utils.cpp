



/* -------------------------------------------------------- */
#include "utils.h"


#include "xcore.h"
#include "xcoreLib.h"
#include "RGBAImage.h"
#include "readTGA.h"
#include "writeTGA.h"

#include "stream\cfileStream.h"
#include "imageUtils.h"
#include "ImageResample.h"

#include "hdrUtils.h"
#include "pixelFormat.h"

/* -------------------------------------------------------- */




bool resizeImage(char *srcName, char *dstName)
{
   BCFileStream srcFile;
   if (!srcFile.open(srcName))
      return false;

   BCFileStream dstFile;
   if (!dstFile.open(dstName, cSFWritable | cSFSeekable))
      return false;

   BRGBAImage srcImage;
   BRGBAImage dstImage;

   BPixelFormat pixelFormat;
   if (!BImageUtils::readTGA(srcFile, srcImage, &pixelFormat))
      return false;

   BImageResampler resampler;
   const bool resampleStatus = resampler.resample(srcImage, dstImage, 128, 128, 4, true, true);
   if (!resampleStatus)
      return false;

   if (!BImageUtils::writeTGA(dstFile, dstImage, cTGAImageTypeBGR))
      return false;

}