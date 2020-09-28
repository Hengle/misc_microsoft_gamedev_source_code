// File: readHDR.cpp
#include "xcore.h"
#include "readHDR.h"

BHDRReader::BHDRReader() :
   mWidth(0),
   mHeight(0),
   mCurScanline(0),
   mpStream(NULL),
   mValid(false),
   mError(false),
   mXFlipped(false),
   mYFlipped(false)
{
}

BHDRReader::~BHDRReader()
{
}

bool BHDRReader::init(BStream& stream)
{
   mWidth = 0;
   mHeight = 0;
   mCurScanline = 0;
   mXFlipped = false;
   mYFlipped = false;
   
   mpStream = &stream;
      
   mValid = false;
   mError = true;
   
   if (!readHeader())
      return false;  
      
   mBuf.resize(mWidth);      
    
   mError = false;
          
   return true;
}

bool BHDRReader::readHeader(void)
{
   //sprintf_s(buf, sizeof(buf), "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %u +X %u\n", height, width);

   bool foundRadiance = false;
   bool foundFormat = false;
            
   uint numLines = 0;
   
   while ((!mpStream->errorStatus()) && (mpStream->bytesLeft() > 0))
   {
      BFixedString<512> buf;
      
      buf.readLine(*mpStream);
      if (mpStream->errorStatus())
         break;
      
      if (buf.getEmpty())         
         break;
                     
      if (!foundRadiance)
      {
         if ((buf.comparei("#?RADIANCE") == 0) || (buf.comparei("#?RGBE") == 0))
            foundRadiance = true;
         else
            return false;
      }
      else if (buf.comparei("FORMAT=32-bit_rle_rgbe") == 0)
      {
         foundFormat = true;
      }
           
      if (++numLines > 20)
         return false;
   }
   
   if ((!foundRadiance) || (!foundFormat))
      return false;
      
   BFixedString<512> buf;

   if ((mpStream->errorStatus()) || (mpStream->bytesLeft() == 0))
      return false;
      
   buf.readLine(*mpStream);
   if (mpStream->errorStatus())
      return false;
      
   int xIndex = buf.findLeft('X');
   int yIndex = buf.findLeft('Y');
   
   if ((xIndex == cInvalidIndex) || (yIndex == cInvalidIndex))
      return false;
      
   if ( ((xIndex + 2) >= (int)buf.getLen()) || ((yIndex + 2) >= (int)buf.getLen()) )
      return false;
      
   if ((buf[xIndex + 1] != ' ') || (buf[yIndex + 1] != ' '))
      return false;
         
   mXFlipped = false;
   mYFlipped = false;
   if (xIndex > 0) mXFlipped = (buf[xIndex - 1] == '-');
   if (yIndex > 0) mYFlipped = (buf[yIndex - 1] == '+');
   
   mWidth = atoi(buf.getPtr() + xIndex + 2);
   mHeight = atoi(buf.getPtr() + yIndex + 2);
   
   if ((mWidth < 1) || (mHeight < 1) || (mWidth > 8192) || (mHeight > 8192))
      return false;
   
   mValid = true;
   
   return true;
}   

void BHDRReader::deinit(void)
{
   mpStream = NULL;
   mError = false;
   mValid = false;   
   
   mBuf.clear();
}

const int MIN_ELEN = 8; /* minimum pScanLine length for encoding */
const int MIN_RUN = 4; /* minimum run length */
const int RED = 0;
const int GRN = 1;
const int BLU = 2;
const int EXP = 3;
#define copycolr(c1,c2)	(c1[0]=c2[0],c1[1]=c2[1], c1[2]=c2[2],c1[3]=c2[3])

bool BHDRReader::freadcolrs(COLR* pScanLine, int len)		/* read in an encoded colr pScanLine */
{
   int i, j;
   int code;
   
   /* determine pScanLine type */
   if (len < MIN_ELEN)
      return (oldreadcolrs(pScanLine, len));
   
   if ((i = mpStream->getch()) == EOF)
      return false;
      
   if (i != 2) 
   {
      return (oldreadcolrs(pScanLine, len, i));
   }
   
   pScanLine[0][GRN] = (uchar)mpStream->getch();
   pScanLine[0][BLU] = (uchar)mpStream->getch();
   if ((i = mpStream->getch()) == EOF)
      return false;
      
   if (pScanLine[0][GRN] != 2 || pScanLine[0][BLU] & 128) 
   {
      pScanLine[0][RED] = 2;
      pScanLine[0][EXP] = (uchar)i;
      return (oldreadcolrs(pScanLine+1, len-1));
   }
   
   if ((pScanLine[0][BLU]<<8 | i) != len)
      return false;		/* length mismatch! */
      
   /* read each component */
   for (i = 0; i < 4; i++)
   {
      for (j = 0; j < len; ) 
      {
         if ((code = mpStream->getch()) == EOF)
            return false;
            
         if (code > 128) 
         {	
            /* run */
            pScanLine[j++][i] = (uchar)mpStream->getch();
            for (code &= 127; --code; j++)
               pScanLine[j][i] = pScanLine[j-1][i];
         } 
         else			
         {
            /* non-run */
            while (code--)
               pScanLine[j++][i] = (uchar)mpStream->getch();
         }               
      }
   }      
      
   return !mpStream->errorStatus();
}

/* read in an old colr pScanLine */
bool BHDRReader::oldreadcolrs(COLR* pScanLine, int len, int firstChar)		
{
   int  rshift;
   register int  i;

   rshift = 0;

   while (len > 0) 
   {
      if (firstChar >= 0)
      {
         pScanLine[0][RED] = (uchar)firstChar;
         firstChar = -1;
      }
      else
         pScanLine[0][RED] = (uchar)mpStream->getch();
         
      pScanLine[0][GRN] = (uchar)mpStream->getch();
      pScanLine[0][BLU] = (uchar)mpStream->getch();
      pScanLine[0][EXP] = (uchar)mpStream->getch();
      
      if (mpStream->errorStatus())
         return false;
         
      if (pScanLine[0][RED] == 1 &&
         pScanLine[0][GRN] == 1 &&
         pScanLine[0][BLU] == 1) 
      {
         for (i = pScanLine[0][EXP] << rshift; i > 0; i--) 
         {
            copycolr(pScanLine[0], pScanLine[-1]);
            pScanLine++;
            len--;
         }
         rshift += 8;
      } 
      else 
      {
         pScanLine++;
         len--;
         rshift = 0;
      }
   }
   
   return true;
}

bool BHDRReader::decode(const BRGBAColor*& pScanOfs, uint& scanLen)
{
   pScanOfs = NULL;
   scanLen = 0;
   
   if ((mCurScanline >= mHeight) || (mError) || (!mValid))
      return false;
      
   if (!freadcolrs(reinterpret_cast<COLR*>(mBuf.getPtr()), mWidth))
   {
      mError = true;
      return false;
   }
      
   mCurScanline++;
   
   pScanOfs = mBuf.getPtr();
   scanLen = mWidth;

   return true;
}
