using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;

namespace RemoteMemory
{
   public partial class HeapLines : UserControl, HaloWarsMemoryEventListener
   {
      public HeapLines()
      {
         InitializeComponent();


         //this.SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
         //  ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
      }

      static int cTimeBetweenUpdates = 1000; // in ms;

      DateTime mStartTime;

      

     
      bool mScrollerStuck = true;
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
         hScrollBar1.Minimum = 0;
         hScrollBar1.Maximum = 100;
         hScrollBar1.Value = 100;
         mScrollerStuck = true;
         mStartTime = DateTime.Now;
         fastTimeLine.start();
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         timer1_Tick(this, null);
         timer1.Enabled = false;
         fastTimeLine.stop();
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, string name)
      {
         HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(mPtr);

         fastTimeLine.addNewGraphLine(pHeap.getMemPtr(), pHeap.getName(), pHeap.ColorVal);
      }
      #endregion

     
      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         int numHeaps = AllocStats.getNumHeaps();

         //ask the manager what our heap sizes are.
         for (int i = 0; i < numHeaps; i++)
         {
            HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);

            uint allocSize = pHeap.getTotalAllocatedBytes();
            //covert the Y value to megabytes for graphing.
            int yMB = (int)(allocSize / (1024 * 1024));
            fastTimeLine.addPointToLine(pHeap.getMemPtr(), (int)yMB);
         }


         //elapsed time
         TimeSpan deltaTime = DateTime.Now - mStartTime;
         label2.Text = "Time Line [" + deltaTime.Hours + ":" + deltaTime.Minutes + ":" + deltaTime.Seconds + "]";

         if (mScrollerStuck)
            fastTimeLine.setScrollPercent(1);

         
         fastTimeLine.Refresh();
      }

      

      bool mShowMouseTimeSpot = false;
      Point mMousePoint = new Point(0, 0);
      //=========================================
      // hScrollBar1_ValueChanged
      //=========================================
      private void hScrollBar1_ValueChanged(object sender, EventArgs e)
      {
         mScrollerStuck = (hScrollBar1.Value >= (hScrollBar1.Maximum - 10 ));

         fastTimeLine.setScrollPercent(hScrollBar1.Value / (float)hScrollBar1.Maximum);
         fastTimeLine.Refresh();
      }

     
   }
}
