using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{
   public partial class CodeFileview : Form
   {
      int mCurrAllocLineIndex = -1;
      List<int> mLineIndexes = new List<int>();

      //=========================================
      // CodeFileview
      //=========================================
      public CodeFileview()
      {
         InitializeComponent();
      }

      //=========================================
      // load
      //=========================================
      public bool load(string filename, TopAllocators.FileAllocStats fas)
      {
         if (filename == "?")
            return false;

         if (!fileExistsCheck(ref filename))
            return false;



         //////////////////////////////////////////////////////
         //EVERYTHING IS OK! LOAD THE FILE!
         System.IO.StreamReader sr = null;
         try
         {
            sr = new StreamReader(File.Open(filename, FileMode.Open, FileAccess.Read));
            if (sr == null)
            {
               MessageBox.Show(null, "Error Loading file :" + filename, "error", MessageBoxButtons.OK);
               return false;
            }
         }
         catch (Exception e)
         {
            MessageBox.Show(e.InnerException.ToString());
         }


         //generate line statistics
         Hashtable lineStats = buildLineStats(fas);//KEY is LINENUM, VALUE is ALLOCAMOUNT
         List<Point> highlightStartLens = new List<Point>();
         //load the file to our list box  
         int lineCount = 0;
         while (!sr.EndOfStream)
         {
            string strFromFile = sr.ReadLine();
            if (!strFromFile.Contains("\n"))
               strFromFile += "\n";


            //if this line has memory allocations, mark it with the amount, and highlight it.
            if (lineStats.Contains(lineCount))
            {
               uint memAmt = (uint)lineStats[lineCount];
               string memoryAmount = MemoryNumber.convert(memAmt);

               int startCharIndex = richTextBox1.Text.Length;
               richTextBox1.Text += memoryAmount + "\t" + strFromFile;
               int selLength = richTextBox1.Text.Length - startCharIndex;

               highlightStartLens.Add(new Point(startCharIndex, selLength));
            }
            else
            {
               richTextBox1.Text += "\t" + strFromFile;   
            }
            lineCount++;
         }

         sr.Close();


         //we have to wait until our entire file is read in before we can highlight...
         for (int i = 0; i < highlightStartLens.Count;i++ )
         {

            richTextBox1.Select(highlightStartLens[i].X, highlightStartLens[i].Y);
            richTextBox1.SelectionBackColor = Color.Pink;
         }


         //move us back to the top
         jumpToLineIndex(0);


            this.Text = "Memory Allocation View : " + filename;
         return true;
      }

      //=========================================
      // fileExistsCheck
      //=========================================
      bool fileExistsCheck(ref string filename)
      {
         //the file doesn't exist...
         if (!File.Exists(filename))
         {
            ///////////////////////////////////
            //see if we can find it in our local setting..
            string targetString = "xbox\\code\\";
            int fid = filename.IndexOf(targetString);
            if (fid == -1)
               return false;
            string trimmedFName = filename.Substring(fid + targetString.Length);

            filename = GlobalSettings.LocalCodePath + trimmedFName;


            ///////////////////////////////////
            //WE COULDN"T FIND IT!!
            if (!File.Exists(filename))
            {
               MessageBox.Show("Error, file not found:" + filename + ". Please navigate to this file.");

               string sFilename = Path.GetFileName(filename);
               string sExtent = Path.GetExtension(filename);
               OpenFileDialog ofd = new OpenFileDialog();
               ofd.Filter = sFilename + "|*" + sExtent;
               if (ofd.ShowDialog() == DialogResult.Cancel)
                  return false;

               filename = ofd.FileName;

               ///////////////////////////////////
               //try to pass back up the chain?
               {
                  int fiid = filename.IndexOf(targetString);
                  if (fiid != -1)
                  {
                     string codePath = filename.Substring(0, fiid + targetString.Length);
                     GlobalSettings.LocalCodePath = codePath;
                     GlobalSettings.save();
                  }
               }
            }
         }

         return true;
      }

      //=========================================
      // buildLineStats
      //=========================================
      Hashtable buildLineStats(TopAllocators.FileAllocStats fas)
      {
         mLineIndexes.Clear();

         Hashtable stats = new Hashtable();  //KEY is LINENUM, VALUE is ALLOCAMOUNT

         
         //walk through our file allocation and highlight all the lines that currently have allocations
         IDictionaryEnumerator _enumerator = fas.mLineAllocs.GetEnumerator();
         while (_enumerator.MoveNext())
         {
            TopAllocators.LineAllocStats las = (TopAllocators.LineAllocStats)_enumerator.Value;
            int lineNum = las.mLineNumber;
            if (stats.Contains(lineNum))
            {
               uint memAmt = (uint)stats[lineNum];
               memAmt += las.mpBlockSize;
               stats[lineNum] = memAmt;
            }
            else
            {
               stats.Add(lineNum,las.mpBlockSize);
               mLineIndexes.Add(lineNum);
            }
         }

         //sort our lines so the up/down will work
         mLineIndexes.Sort(delegate(int p1, int p2) { return p1.CompareTo(p2); });
         return stats;
      }
      //=========================================
      // jumpToLineIndex
      //=========================================
      void jumpToLineIndex(int lineNum)
      {
         int startChar = richTextBox1.GetFirstCharIndexFromLine(lineNum);
         int endChar = richTextBox1.GetFirstCharIndexFromLine(lineNum + 1);
         richTextBox1.Select(startChar, endChar - 1);
         richTextBox1.DeselectAll();
      }
      //=========================================
      // toolStripButton1_Click
      //=========================================
      private void toolStripButton1_Click(object sender, EventArgs e)
      {
         if (mCurrAllocLineIndex + 1 >= mLineIndexes.Count)
         {
            return;
         }

         mCurrAllocLineIndex++;
         int targetLine = mLineIndexes[mCurrAllocLineIndex];

         jumpToLineIndex(targetLine);

         toolStripButton2.Enabled = true;
         if (mCurrAllocLineIndex + 1 >= mLineIndexes.Count)
            toolStripButton1.Enabled = false;

      }

      //=========================================
      // toolStripButton2_Click
      //=========================================
      private void toolStripButton2_Click(object sender, EventArgs e)
      {
         if (mCurrAllocLineIndex - 1 < 0)
            return;

         mCurrAllocLineIndex--;
         int targetLine = mLineIndexes[mCurrAllocLineIndex];

         jumpToLineIndex(targetLine);


         toolStripButton1.Enabled = true;
         if (mCurrAllocLineIndex - 1 < 0)
            toolStripButton2.Enabled = false;
          
         
      }

      private void richTextBox1_VScroll(object sender, EventArgs e)
      {
         //richTextBox2.scr
      }


   }
}
