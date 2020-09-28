using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;

namespace RemoteMemory
{
   //=========================================
   // FastBitmap
   //=========================================
   class FastBitmap
   {
      public struct PixelData24
      {
         public byte red;
         public byte green;
         public byte blue; 
      }

      Bitmap mBmp = null;
      BitmapData sourceData;

      //=========================================
      // init
      //=========================================
      public unsafe void init(int width, int height)
      {
         mBmp = new Bitmap(width, height, PixelFormat.Format24bppRgb);
         for (int x = 0; x < mBmp.Width; x++)
            for (int y = 0; y < mBmp.Height; y++)
               mBmp.SetPixel(x, y, GDIStatic.CommonBGColor);


         //Rectangle r = new Rectangle(0, 0, width, height);
         //sourceData = mBmp.LockBits(r, ImageLockMode.WriteOnly, mBmp.PixelFormat);

         //PixelData24* mpSrcDat = (PixelData24*)sourceData.Scan0.ToPointer();
         //for (int i = 0; i < mBmp.Width * mBmp.Height; i++)
         //{
         //   mpSrcDat[i].red = GDIStatic.CommonBGColor.R;
         //   mpSrcDat[i].green = GDIStatic.CommonBGColor.G;
         //   mpSrcDat[i].blue = GDIStatic.CommonBGColor.B;
         //}
      }

      ~FastBitmap()
      {
       //  mBmp.UnlockBits(sourceData);
       //  mBmp = null;
      }

      //=========================================
      // drawHorizline
      //=========================================
      public unsafe void drawHorizline(uint startX, uint endX, uint y, Color col)
      {

         for (uint i = startX; i < endX; i++)
            mBmp.SetPixel((int)i,(int) y, col);

         //if (y == 2 && endX > mBmp.Width - 3)
         //   return;
         //PixelData24* mpSrcDat = (PixelData24*)sourceData.Scan0;

         //int startIndex = (int)((startX + (mBmp.Width) * y));

         //PixelData24* pSourcePixel = mpSrcDat + startIndex;// (y * mBmp.Width + x);


         //for (uint x = startX; x < endX; x++, pSourcePixel++)
         //{
            
         //   pSourcePixel->red = col.R;
         //   pSourcePixel->green = col.G;
         //   pSourcePixel->blue = col.B;
         //}
      }

      //=========================================
      // flush
      //=========================================
      public Bitmap flush()
      {
      //   mBmp.UnlockBits(sourceData);

         Bitmap bmp = (Bitmap)mBmp.Clone();


      //   Rectangle r = new Rectangle(0, 0, mBmp.Width, mBmp.Height);
      //   sourceData = mBmp.LockBits(r, ImageLockMode.WriteOnly, mBmp.PixelFormat);

         return bmp;
      }


   };
}
