using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class virtualMemLayout : UserControl, HaloWarsMemoryEventListener
   {


      int mTotalVisibleArea = 0;
      int mNumBytesPerPixel = 0;

      public virtualMemLayout()
      {
         InitializeComponent();

      }

      #region on events
      //=========================================
      // onNew
      //=========================================
      public void onNew(uint mpHeap, uint mSize, uint mpBlock, uint mBlockSize, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpBlock);

         if(vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eVirtual)
         {
            if(vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mVirtual4KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mVirtual64KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eImage)
         {
            if(vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mImage4KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mImage64KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eEncrypted)
         {
            mEncrypted64KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               Physical4KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               Physical64KRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e16m)
               mPhysical16MBRange.onNew(mpHeap, mSize, mpBlock, mBlockSize, context);
         }
      }
      //=========================================
      // onResize
      //=========================================
      public void onResize(uint mpHeap, uint mpOrigBlock, uint mNewSize, uint mpNewBlock, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpOrigBlock);

         if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eVirtual)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mVirtual4KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mVirtual64KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eImage)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mImage4KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mImage64KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eEncrypted)
         {
            mEncrypted64KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               Physical4KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               Physical64KRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e16m)
               mPhysical16MBRange.onResize(mpHeap, mpOrigBlock, mNewSize, mpNewBlock, context);
         }
      }
      //=========================================
      // onDelete
      //=========================================
      public void onDelete(uint mpHeap, uint mpBlock, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpBlock);

         if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eVirtual)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mVirtual4KRange.onDelete(mpHeap, mpBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mVirtual64KRange.onDelete(mpHeap, mpBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eImage)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               mImage4KRange.onDelete(mpHeap, mpBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               mImage64KRange.onDelete(mpHeap, mpBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.eEncrypted)
         {
            mEncrypted64KRange.onDelete(mpHeap, mpBlock, context);
         }
         else if (vai.mMemRegion == XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
         {
            if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e4k)
               Physical4KRange.onDelete(mpHeap, mpBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e64k)
               Physical64KRange.onDelete(mpHeap, mpBlock, context);
            else if (vai.mSizeOfContainingPage == XBOXMemView.VirtualAddressInfo.ePageSize.e16m)
               mPhysical16MBRange.onDelete(mpHeap, mpBlock, context);
         }
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, byte[] name)
      {

      }
      //=========================================
      // onConnect
      //=========================================
      public void onConnect()
      {
         int labelHeight = 22;
         int cSpacing = 4;

         int maxHeight = Height - labelHeight - (cSpacing * 8);



         mVirtual4KRange.Top = labelHeight; // take into account the label..
         mVirtual4KRange.Height = (int)(((XBOXMemView.MM_VIRTUAL_4KB_END - XBOXMemView.MM_VIRTUAL_4KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mVirtual4KRange.Width = this.Width;

         mVirtual64KRange.Top = mVirtual4KRange.Top + mVirtual4KRange.Height + cSpacing;
         mVirtual64KRange.Height = (int)(((XBOXMemView.MM_VIRTUAL_64KB_END - XBOXMemView.MM_VIRTUAL_64KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mVirtual64KRange.Width = this.Width;

         mImage64KRange.Top = mVirtual64KRange.Top + mVirtual64KRange.Height + cSpacing;
         mImage64KRange.Height = (int)(((XBOXMemView.MM_IMAGE_64KB_END - XBOXMemView.MM_IMAGE_64KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mImage64KRange.Width = this.Width;

         mEncrypted64KRange.Top = mImage64KRange.Top + mImage64KRange.Height + cSpacing;
         mEncrypted64KRange.Height = (int)(((XBOXMemView.MM_ENCRYPTED_64KB_END - XBOXMemView.MM_ENCRYPTED_64KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mEncrypted64KRange.Width = this.Width;

         mImage4KRange.Top = mEncrypted64KRange.Top + mEncrypted64KRange.Height + cSpacing;
         mImage4KRange.Height = (int)(((XBOXMemView.MM_IMAGE_4KB_END - XBOXMemView.MM_IMAGE_4KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mImage4KRange.Width = this.Width;

         Physical64KRange.Top = mImage4KRange.Top + mImage4KRange.Height + cSpacing;
         Physical64KRange.Height = (int)(((XBOXMemView.MM_PHYSICAL_64KB_END - XBOXMemView.MM_PHYSICAL_64KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         Physical64KRange.Width = this.Width;

         mPhysical16MBRange.Top = Physical64KRange.Top + Physical64KRange.Height + cSpacing;
         mPhysical16MBRange.Height = (int)(((XBOXMemView.MM_PHYSICAL_16MB_END - XBOXMemView.MM_PHYSICAL_16MB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         mPhysical16MBRange.Width = this.Width;

         Physical4KRange.Top = mPhysical16MBRange.Top + mPhysical16MBRange.Height + cSpacing;
         Physical4KRange.Height = (int)(((XBOXMemView.MM_PHYSICAL_4KB_END - XBOXMemView.MM_PHYSICAL_4KB_BASE) / (float)XBOXMemView.TotalVirtualMemory) * maxHeight);
         Physical4KRange.Width = this.Width;

         mTotalVisibleArea = 0;
         mTotalVisibleArea += mVirtual4KRange.Width * mVirtual4KRange.Height;
         mTotalVisibleArea += mVirtual64KRange.Width * mVirtual64KRange.Height;
         mTotalVisibleArea += mImage64KRange.Width * mImage64KRange.Height;
         mTotalVisibleArea += mEncrypted64KRange.Width * mEncrypted64KRange.Height;
         mTotalVisibleArea += mImage4KRange.Width * mImage4KRange.Height;
         mTotalVisibleArea += Physical64KRange.Width * Physical64KRange.Height;
         mTotalVisibleArea += mPhysical16MBRange.Width * mPhysical16MBRange.Height;
         mTotalVisibleArea += Physical4KRange.Width * Physical4KRange.Height;

         mNumBytesPerPixel = (int)(XBOXMemView.TotalVirtualMemory / (float)mTotalVisibleArea);

         headerLabel.Text = "Virtual Memory View (1px = " + mNumBytesPerPixel + "bytes)";

         this.mVirtual4KRange.init(XBOXMemView.MM_VIRTUAL_4KB_BASE,        (uint)(XBOXMemView.MM_VIRTUAL_4KB_END - XBOXMemView.MM_VIRTUAL_4KB_BASE));
         this.mVirtual64KRange.init(XBOXMemView.MM_VIRTUAL_64KB_BASE,      (uint)(XBOXMemView.MM_VIRTUAL_64KB_END - XBOXMemView.MM_VIRTUAL_64KB_BASE));
         this.mImage64KRange.init(XBOXMemView.MM_IMAGE_64KB_BASE,          (uint)(XBOXMemView.MM_IMAGE_64KB_END - XBOXMemView.MM_IMAGE_64KB_BASE));
         this.mEncrypted64KRange.init(XBOXMemView.MM_ENCRYPTED_64KB_BASE,  (uint)(XBOXMemView.MM_ENCRYPTED_64KB_END - XBOXMemView.MM_ENCRYPTED_64KB_BASE));
         this.mImage4KRange.init(XBOXMemView.MM_IMAGE_4KB_BASE,            (uint)(XBOXMemView.MM_IMAGE_4KB_END - XBOXMemView.MM_IMAGE_4KB_BASE));
         this.Physical64KRange.init(XBOXMemView.MM_PHYSICAL_64KB_BASE,     (uint)(XBOXMemView.MM_PHYSICAL_64KB_END - XBOXMemView.MM_PHYSICAL_64KB_BASE));
         this.mPhysical16MBRange.init(XBOXMemView.MM_PHYSICAL_16MB_BASE,   (uint)(XBOXMemView.MM_PHYSICAL_16MB_END - XBOXMemView.MM_PHYSICAL_16MB_BASE));
         this.Physical4KRange.init(XBOXMemView.MM_PHYSICAL_4KB_BASE,       (uint)(XBOXMemView.MM_PHYSICAL_4KB_END - XBOXMemView.MM_PHYSICAL_4KB_BASE));

         this.mVirtual4KRange.onConnect();
         this.mVirtual64KRange.onConnect();
         this.mImage64KRange.onConnect();
         this.mEncrypted64KRange.onConnect();
         this.mImage4KRange.onConnect();
         this.Physical64KRange.onConnect();
         this.mPhysical16MBRange.onConnect();
         this.Physical4KRange.onConnect();
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         this.mVirtual4KRange.onDisconnect();
         this.mVirtual64KRange.onDisconnect();
         this.mImage64KRange.onDisconnect();
         this.mEncrypted64KRange.onDisconnect();
         this.mImage4KRange.onDisconnect();
         this.Physical64KRange.onDisconnect();
         this.mPhysical16MBRange.onDisconnect();
         this.Physical4KRange.onDisconnect();
      }

     

      #endregion


   }
}
