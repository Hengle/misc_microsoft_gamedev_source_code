using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace RemoteMemory
{
   public partial class HeapKey : UserControl, HaloWarsMemoryEventListener
   {


      static int cTimeBetweenUpdates = 1000; // in ms;

      public HeapKey()
      {
         InitializeComponent();

         this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
          ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);

         this.BackColor = GDIStatic.CommonBGColor;
      }
      #region ON events
      //=========================================
      // onNew
      //=========================================
      public void onNew(uint mpHeap, uint mSize, uint mpBlock, uint mBlockSize, HaloWarsMem.BALContext context)
      {

      }
      //=========================================
      // onResize
      //=========================================
      public void onResize(uint mpHeap, uint mpOrigBlock, uint mNewSize, uint mpNewBlock, HaloWarsMem.BALContext context)
      {

      }
      //=========================================
      // onDelete
      //=========================================
      public void onDelete(uint mpHeap, uint mpBlock, HaloWarsMem.BALContext context)
      {

      }

      //=========================================
      // onConnect
      //=========================================
      public void onConnect()
      {
         timer1.Enabled = true;
         timer1.Interval = cTimeBetweenUpdates;
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         timer1_Tick(this, null);
         timer1.Enabled = false;
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, string name)
      {

      }
      #endregion

      //=========================================
      // onHeapRegister
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         this.Invalidate();
      }

      //=========================================
      // OnPaint
      //=========================================
      protected override void OnPaint(PaintEventArgs e)
      {
         Graphics g = e.Graphics;

         int[] xTabs = { 5, 100, 170, 250, 330, 420, 510 };

         int x = 0;
         int y = 0;
         int ySpacing = 12;

         
         g.DrawString("allocated", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[1], y);
         g.DrawString("blocks", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[2], y);
         g.DrawString("N", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[3], y);
         g.DrawString("D", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[4], y);
         g.DrawString("R", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[5], y);

         y += ySpacing + 1;

         g.DrawLine(GDIStatic.Pen_DimGray, 0, y, Width, y);

         y+=2;

         //SORT THE LIST!
         List<int> sortOrder = new List<int>();
         for (int i = 0; i < AllocStats.getNumHeaps(); i++)
            sortOrder.Add(i);

         //SORT!
         for (int i = 0; i < AllocStats.getNumHeaps(); i++)
         {
            for (int j = 0; j < AllocStats.getNumHeaps(); j++)
            {
               uint target = AllocStats.getHeapFromIndex(sortOrder[i]).getTotalAllocatedBytes();
               uint next = AllocStats.getHeapFromIndex(sortOrder[j]).getTotalAllocatedBytes();
               if (next < target)
               {
                  int tmp = sortOrder[i];
                  sortOrder[i] = sortOrder[j];
                  sortOrder[j] = tmp;
               }
            }
         }



         uint totalActiveBytes = 0;
         uint totalActiveBlocks = 0;
         for (int i = 0; i < AllocStats.getNumHeaps(); i++)
         {
            HeapAlloc pHeap = AllocStats.getHeapFromIndex(sortOrder[i]);
            Brush brush = new SolidBrush(pHeap.ColorVal);

            uint totalNumBlocks = pHeap.getTotalNumAllocations();
            uint totalNumBytes = pHeap.getTotalAllocatedBytes();

            
            g.DrawString(pHeap.getName() + " :",   GDIStatic.Font_Console_10, brush, xTabs[0], y);
            g.DrawString(MemoryNumber.convert(totalNumBytes), GDIStatic.Font_Console_10, brush, xTabs[1], y);
            g.DrawString(totalNumBlocks.ToString(),  GDIStatic.Font_Console_10, brush, xTabs[2], y);
            g.DrawString(pHeap.getNumNews().ToString(), GDIStatic.Font_Console_10, brush, xTabs[3], y);
            g.DrawString(pHeap.getNumDeletes().ToString(), GDIStatic.Font_Console_10, brush, xTabs[4], y);
            g.DrawString(pHeap.getNumResizes().ToString(), GDIStatic.Font_Console_10, brush, xTabs[5], y);
            
            g.FillRectangle(brush, x + 0, y + 2, 4, 8);

            y += ySpacing;

            totalActiveBlocks += totalNumBlocks;
            totalActiveBytes += totalNumBytes;
         }

         
         g.DrawString("Total :", GDIStatic.Font_Console_10, GDIStatic.SolidBrush_White, xTabs[0], y + ySpacing);
         g.DrawString(MemoryNumber.convert(totalActiveBytes), GDIStatic.Font_Console_10, GDIStatic.SolidBrush_White, xTabs[1], y + ySpacing);
         g.DrawString(totalActiveBlocks.ToString(), GDIStatic.Font_Console_10, GDIStatic.SolidBrush_White, xTabs[2], y + ySpacing);

      }

   }
}
