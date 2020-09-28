using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{
   public partial class FileTimelineElement : UserControl
   {
      string mTruncatedFilePath;
      string mFilenameOnly;

      FileTimelines mOwner = null;
      fastTimeline mTimeLine = new fastTimeline();
      
      bool mScrollerStuck = true;
      //=========================================
      // compareFilename
      //=========================================
      public bool compareFilename(string rhs)
      {
         if (mTruncatedFilePath.ToLower() == rhs.ToLower())
            return true;

         return false;
      }
      //=========================================
      // getFilename
      //=========================================
      public string getFilename()
      {
         return mTruncatedFilePath;
      }
      //=========================================
      // giveTrimmedString
      //=========================================
      string giveTrimmedString(string filename)
      {
         string targetString = "xbox\\code\\";
         int fid = filename.IndexOf(targetString);
         if (fid == -1)
            return filename;
         string trimmedFName = filename.Substring(fid + targetString.Length);

         return trimmedFName;
      }

      //=========================================
      // FileTimelineElement
      //=========================================
      public FileTimelineElement( string truncatedfilepath, FileTimelines pOwner)
      {
         mTruncatedFilePath = truncatedfilepath;
         mFilenameOnly = Path.GetFileName(truncatedfilepath);
         mOwner = pOwner;

        
         InitializeComponent();

         FilenameLabel.Text = mFilenameOnly;
      }

      //=========================================
      // onConnect
      //=========================================
      public void onConnect()
      {
         timer1.Enabled = true;
         timer1.Interval = 1000;
         hScrollBar1.Minimum = 0;
         hScrollBar1.Maximum = 100;
         hScrollBar1.Value = 100;
         mScrollerStuck = true;

         
         mTimeline.start();

         mTimeline.addNewGraphLine(0xBEEFCAD0, mFilenameOnly, GDIStatic.getNextDesiredColor());
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         timer1_Tick(this, null);

         timer1.Enabled = false;
         mTimeline.stop();

      }

      //=========================================
      // pictureBox1_Click
      //=========================================
      private void pictureBox1_Click(object sender, EventArgs e)
      {
         onDisconnect();
         mOwner.removeControl(mTruncatedFilePath,this);
      }

      

      //=========================================
      // hScrollBar1_ValueChanged
      //=========================================
      private void hScrollBar1_ValueChanged(object sender, EventArgs e)
      {
         mScrollerStuck = (hScrollBar1.Value >= (hScrollBar1.Maximum - 10));

         mTimeline.setScrollPercent(hScrollBar1.Value / (float)hScrollBar1.Maximum);
         mTimeline.Refresh();
      }

      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         uint mTotalAllocatedBytesI = 0;
         uint mTotalAllocatedBytesE = 0;

         int numHeaps = AllocStats.getNumHeaps();
         for(int i = 0 ; i < numHeaps; i ++)
         {
            HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);

            Hashtable pHeapFiles = pHeap.getFileAllocations();
            IDictionaryEnumerator file_enumerator = pHeapFiles.GetEnumerator();
            while (file_enumerator.MoveNext())
            {

               FileAlloc fa = ((FileAlloc)file_enumerator.Value);
               string trFName = giveTrimmedString(fa.getFilename());
               if (trFName.ToLower() == mTruncatedFilePath.ToLower())
               {
                  mTotalAllocatedBytesE += fa.getTotalAllocatedBytes(false);
                  mTotalAllocatedBytesI += fa.getTotalAllocatedBytes(true);
               }
            }
         }

         //covert the Y value to megabytes for graphing.
         uint allocSize = mTotalAllocatedBytesE;
         int yMB = (int)(allocSize / (1024 * 1024));

         mTimeline.addPointToLine(0xBEEFCAD0, (int)yMB);

         if (mScrollerStuck)
            mTimeline.setScrollPercent(1);

         
         mTimeline.Refresh();

         FilenameLabel.Text = mFilenameOnly + " i(" + MemoryNumber.convert(mTotalAllocatedBytesI) + ")" + " e(" + MemoryNumber.convert(mTotalAllocatedBytesE) + ")";
      }
   }
}
