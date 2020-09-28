using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{
   public partial class TopAllocators : UserControl, HaloWarsMemoryEventListener
   {

      class SortedFileStats
      {
         public string mFilenameOnly = "?";
         public uint mTotalAllocatedBytes = 0;
      };
      List<SortedFileStats> mLastSortedKeys = new List<SortedFileStats>();
      static int cIgnoreBelowValue = 10;   //in bytes
      static int cMaxNumBars = 32;

      #region on events
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
         timer1.Enabled = true;
         timer1.Interval = 1500;
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         timer1_Tick(this, null);
         timer1.Enabled = false;
      }

      #endregion

      
      //=========================================
      // TopAllocators
      //=========================================
      public TopAllocators()
      {
         InitializeComponent();

         this.BackColor = GDIStatic.CommonBGColor;
      }
      //=========================================
      // TopAllocators_DoubleClick
      //=========================================
      private void TopAllocators_MouseDoubleClick(object sender, MouseEventArgs e)
      {
 
      }

      //=========================================
      // dumpEntireList
      //=========================================
      class tHashVal
      {
         public string tFname = "";
         public uint memAmt = 0;
         public string heapsUsed = "";
      };

      public void dumpEntireList(string filename)
      {
         

         

         Dictionary<string, tHashVal> fileHashes = new Dictionary<string, tHashVal>();

         int numHeaps = AllocStats.getNumHeaps();
         for (int i = 0; i < numHeaps; i++)
         {
            HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);
            Hashtable pFiles = pHeap.getFileAllocations();

            IDictionaryEnumerator _enumerator = pFiles.GetEnumerator();
            while (_enumerator.MoveNext())
            {
               FileAlloc fa = ((FileAlloc)_enumerator.Value);
               string fnameOnly = Path.GetFileName(fa.getFilename());
               uint memAmt = fa.getTotalAllocatedBytes(false);

               if (fileHashes.ContainsKey(fnameOnly))
               {
                  tHashVal thv = fileHashes[fnameOnly];
                  thv.memAmt += memAmt;
                  thv.heapsUsed += "," + pHeap.getName();
               }
               else
               {
                  tHashVal th = new tHashVal();
                  th.heapsUsed = pHeap.getName();
                  th.memAmt = memAmt;
                  th.tFname = fa.getFilename();
                  fileHashes.Add(fnameOnly, th);
               }
            }
         }


         StreamWriter sw = new StreamWriter(filename, true);
         IDictionaryEnumerator enumerator = fileHashes.GetEnumerator();
         while (enumerator.MoveNext())
         {
            tHashVal thv = ((tHashVal)enumerator.Value);
            sw.WriteLine(thv.tFname + "," + thv.memAmt + "," + thv.heapsUsed);
         }
         

         sw.Close();
         sw = null;
      }

      //=========================================
      // onUpdate
      //=========================================
      void onUpdate()
      {
      
         /////////////////////////////////////
         //find our sorting order..
         mLastSortedKeys.Clear();

         //this has changed a bit..
         //walk each heap, and get all the files per heap
         //get each file's memory, and use insertion sort to handle it.

         int numHeaps = AllocStats.getNumHeaps();
         for (int i = 0; i < numHeaps; i ++ )
         {
            HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);
            Hashtable pFiles = pHeap.getFileAllocations();

            IDictionaryEnumerator _enumerator = pFiles.GetEnumerator();
            while (_enumerator.MoveNext())
            {
               FileAlloc fa = ((FileAlloc)_enumerator.Value);

               uint memAmt = fa.getTotalAllocatedBytes(false);

               //search the other heaps for this same file..
               for (int j = 0; j < numHeaps; j++)
               {
                  if(i==j)
                     continue;

                  HeapAlloc pHeap2 = AllocStats.getHeapFromIndex(j);
                  Hashtable pFiles2 = pHeap2.getFileAllocations();
                  if(pFiles2.Contains(fa.getFilename()))
                  {
                     FileAlloc fa2 = ((FileAlloc)pFiles2[fa.getFilename()]);
                     memAmt += fa2.getTotalAllocatedBytes(false);
                  }
               }



               //use insertion sort
               bool inserted = false;
               for(int j = 0 ; j < mLastSortedKeys.Count; j++)
               {
                  if (mLastSortedKeys[j].mFilenameOnly == Path.GetFileName(fa.getFilename()))
                  {
                     inserted = true;
                     break;
                  }

   
                  if(memAmt < mLastSortedKeys[j].mTotalAllocatedBytes)
                  {
                     SortedFileStats sfs = new SortedFileStats();
                     sfs.mTotalAllocatedBytes = memAmt;
                     sfs.mFilenameOnly = Path.GetFileName(fa.getFilename());
                     mLastSortedKeys.Insert(j,sfs);

                     if (mLastSortedKeys.Count >= cMaxNumBars)
                        mLastSortedKeys.RemoveAt(0);
                     inserted = true;

                     break;
                  }
               }

               if (!inserted)
               {
                  SortedFileStats sfs = new SortedFileStats();
                  sfs.mTotalAllocatedBytes = memAmt;
                  sfs.mFilenameOnly = Path.GetFileName(fa.getFilename());
                  mLastSortedKeys.Add(sfs);
               }


            }
         }

       }
      //=========================================
      // render
      //=========================================
      protected override void OnPaint(PaintEventArgs e)
      {
          


         ///////////////////////////////
         Graphics g = e.Graphics;

         int[] xTabs = { 5, 270, 360, 450 };

         int x = 0;
         int y = 0;
         int ySpacing = 12;

         /////////////////////
         //print header..
         g.DrawString("name",       GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[0], y);
         g.DrawString("exclusive",  GDIStatic.Font_Console_10, GDIStatic.SolidBrush_DimGray, xTabs[1], y);
         y += ySpacing;
         g.DrawLine(GDIStatic.Pen_DimGray, 0, y, Width, y);
         y += 2;


         ////////////////////
         //Draw BG Bars
         for (int i = 0; i < mLastSortedKeys.Count; i++)
         {
            Brush BGBrush = i % 2 == 0 ? GDIStatic.SolidBrush_CommonBGColor : GDIStatic.SolidBrush_DimGray;
            g.FillRectangle(BGBrush, 0, y + (i*ySpacing+2), Width, ySpacing);
         }

         /////////////////////
         //print sorted list..
         for (int i = mLastSortedKeys.Count - 1; i >= 0; i--)
         {
            string filename = mLastSortedKeys[i].mFilenameOnly;
            UInt64 exclusiveTotal = mLastSortedKeys[i].mTotalAllocatedBytes;
            int list_index = mLastSortedKeys.Count - 1 - i;
            g.DrawString(list_index + ": " + filename,               GDIStatic.Font_Console_10, GDIStatic.SolidBrush_Black, xTabs[0], y);
            g.DrawString(MemoryNumber.convert((uint)exclusiveTotal), GDIStatic.Font_Console_10, GDIStatic.SolidBrush_Black, xTabs[1], y);
            y += ySpacing;
         }

      }

      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         onUpdate();
         this.Invalidate();
      }

   }
}
