using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class PhysMemView : UserControl, HaloWarsMemoryEventListener
   {
      public PhysMemView()
      {
         InitializeComponent();
      }
      #region ON events
      //=========================================
      // onNew
      //=========================================
      public void onNew(uint mpHeap, uint mSize, uint mpBlock, uint mBlockSize, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpBlock);
         if (vai.mMemRegion != XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
            return;

         uint translatedAddress = XBOXMemView.convertVirtualAddrToPhysicalAddr(mpBlock);
         mPhysicalMemoryRange.onNew(mpHeap, mSize, translatedAddress, mBlockSize, context);
      }
      //=========================================
      // onResize
      //=========================================
      public void onResize(uint mpHeap, uint mpOrigBlock, uint mNewSize, uint mpNewBlock, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpOrigBlock);
         if (vai.mMemRegion != XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
            return;

         uint translatedAddress0 = XBOXMemView.convertVirtualAddrToPhysicalAddr(mpOrigBlock);
         uint translatedAddress = XBOXMemView.convertVirtualAddrToPhysicalAddr(mpNewBlock);
         mPhysicalMemoryRange.onResize(mpHeap, translatedAddress0, mNewSize, translatedAddress, context);
      }
      //=========================================
      // onDelete
      //=========================================
      public void onDelete(uint mpHeap, uint mpBlock, HaloWarsMem.BALContext context)
      {
         XBOXMemView.VirtualAddressInfo vai = XBOXMemView.getVirtualAddressInfo(mpBlock);
         if (vai.mMemRegion != XBOXMemView.VirtualAddressInfo.eMemRegion.ePhysical)
            return;

         uint translatedAddress = XBOXMemView.convertVirtualAddrToPhysicalAddr(mpBlock);
         mPhysicalMemoryRange.onDelete(mpHeap, translatedAddress, context);
      }

      //=========================================
      // onConnect
      //=========================================
      public void onConnect()
      {
         int labelHeight = 22;
         int cSpacing = 4;

         int maxHeight = Height - labelHeight - (cSpacing * 1);


         mPhysicalMemoryRange.Top = labelHeight; // take into account the label..
         mPhysicalMemoryRange.Height = maxHeight;
         mPhysicalMemoryRange.Width = this.Width;

         int mTotalVisibleArea = mPhysicalMemoryRange.Width * mPhysicalMemoryRange.Height;

         uint minMem = XBOXMemView.convertVirtualAddrToPhysicalAddr((uint)XBOXMemView.MM_PHYSICAL_64KB_BASE);
         uint maxMem = XBOXMemView.convertVirtualAddrToPhysicalAddr((uint)XBOXMemView.MM_PHYSICAL_4KB_END);
         uint rangeMem = maxMem - minMem;


         int mNumBytesPerPixel = (int)(rangeMem / (float)mTotalVisibleArea);

         headerLabel.Text = "Physical Memory View (1px = " + mNumBytesPerPixel + "bytes)";

         this.mPhysicalMemoryRange.init((ulong)minMem, (uint)rangeMem);

         mPhysicalMemoryRange.onConnect();

      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         mPhysicalMemoryRange.onDisconnect();
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, byte[] name)
      {

      }
      #endregion

   }
}
