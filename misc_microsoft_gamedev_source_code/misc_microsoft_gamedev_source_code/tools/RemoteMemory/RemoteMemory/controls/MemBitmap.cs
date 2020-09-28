using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class MemBitmap : UserControl, HaloWarsMemoryEventListener
   {
      float mBlockScalar = 0;
      FastBitmap mBitmap = new FastBitmap();

      ulong mBaseAddress = 0;
      uint mBlockSize = 0;

      public MemBitmap()
      {
         InitializeComponent();

         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);

         this.BackColor = GDIStatic.CommonBGColor;
      }

      //=========================================
      // setBaseAddress
      //=========================================
      public void init(ulong baseAddy, uint blockSize)
      {
         mBaseAddress = baseAddy;
         mBlockSize = blockSize;
      }

      //=========================================
      // onNew
      //=========================================
      void giveXYFromMemLoc(uint pBlock, ref uint x, ref uint y)
      {
         long shiftedBlock = (long)(pBlock - mBaseAddress);
         float scl = shiftedBlock / (float)mBlockSize;

         uint scaledPtr = (uint)(scl * (Width * Height));
     
         y = (uint)(scaledPtr / (Width));
         x = (uint)(scaledPtr  - (y*Width));
         
      }

      //=========================================
      // drawBlock
      //=========================================
      void drawBlock(uint pBlock, uint size, Color col)
      {
         uint startX = 0;
         uint startY = 0;
         uint endX = 0;
         uint endY = 0;

         giveXYFromMemLoc(pBlock, ref startX, ref startY);
         giveXYFromMemLoc((uint)(pBlock + size), ref endX, ref endY);


         if(startY == endY)
         {
            mBitmap.drawHorizline(startX, endX, startY, col);
         }
         else
         {
            //startline
            mBitmap.drawHorizline( startX, (uint)Width, startY, col);

            //full lines
            for (uint i = startY+1; i < endY; i++)
            {
               mBitmap.drawHorizline(0, (uint)Width, i, col);
            }

            //endline
            mBitmap.drawHorizline( 0, endX, endY, col);
         }
      }

      //=========================================
      // onNew
      //=========================================
      public void onNew(uint mpHeap, uint mSize, uint mpBlock, uint mBlockSize, HaloWarsMem.BALContext context)
      {
         HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(mpHeap);
         drawBlock(mpBlock, mBlockSize, pHeap.ColorVal);

      }
      //=========================================
      // onResize
      //=========================================
      public void onResize(uint mpHeap, uint mpOrigBlock, uint mNewSize, uint mpNewBlock, HaloWarsMem.BALContext context)
      {
         HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(mpHeap);
         uint blockSize = pHeap.getBlockSize(mpOrigBlock);

         drawBlock(mpOrigBlock, blockSize, GDIStatic.CommonBGColor);
         drawBlock(mpNewBlock, mNewSize, pHeap.ColorVal);

      }
      //=========================================
      // onDelete
      //=========================================
      public void onDelete(uint mpHeap, uint mpBlock, HaloWarsMem.BALContext context)
      {
         HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(mpHeap);
         uint blockSize = pHeap.getBlockSize(mpBlock);

         drawBlock(mpBlock, blockSize, GDIStatic.CommonBGColor);
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, string name)
      {

      }
      //=========================================
      // onConnect
      //=========================================
      public void onConnect()
      {
         int numAvailableBytes = Height * Width;
         mBlockScalar = numAvailableBytes / (float)XBOXMemView.TotalPhysicalMemory;
         mBitmap.init(Width, Height);

         timer1.Enabled = true;
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {

      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         this.Invalidate();
         this.BackgroundImage = mBitmap.flush();
      }
   }

}
