using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using GDIControls;

namespace RemoteMemory
{
   public partial class HeapFileView : UserControl, HaloWarsMemoryEventListener
   {
      //=========================================
      // HeapFileView
      //=========================================
      public HeapFileView()
      {
         InitializeComponent();
      }

      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         for(int i = 0 ; i < mTreeView.Nodes.Count; i++)
         {
            if(mTreeView.Nodes[i] is HeapAllocNode)
            {
               HeapAllocNode han = (HeapAllocNode)mTreeView.Nodes[i];
               han.update();
            }
         }

         //////////////////
         mTreeView.updatePositions();
         mTreeView.Refresh();
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
         timer1.Interval = 1000;

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
         HeapAlloc pHeap = AllocStats.getHeapFromBasePtr(mPtr);
         HeapAllocNode has = new HeapAllocNode(pHeap);

         mTreeView.addNode(has);
         mTreeView.updatePositions();
         mTreeView.Refresh();
      }
      #endregion


      #region ALLOC STAT CLASSES


      //=========================================
      // LineAllocStats
      //=========================================
      public class LineAllocNode : GDITreeViewNode
      {
         public LineAllocNode(LineAlloc ownerAlloc)
         {
            mpOwnerAlloc = ownerAlloc;
            this.Text = MemoryNumber.convert(mpOwnerAlloc.getTotalAllocatedBytes()) + " from " + mpOwnerAlloc.getTotalNumAllocations() + " allocations at Line " + mpOwnerAlloc.getLinenum();
         }

         //=========================================
         // OnMouseDoubleClick
         //=========================================
         public override void OnMouseDoubleClick(MouseEventArgs e)
         {
            LineCallstacksView lcs = new LineCallstacksView();
            lcs.load(mpOwnerAlloc.getLinenum(), mpOwnerAlloc.getBlockAllocations());
            lcs.Show();
         }

         public LineAlloc mpOwnerAlloc = null;
      };

      //=========================================
      // FileAllocStats
      //=========================================
      public class FileAllocNode : GDITreeViewNode
      {
         public FileAlloc mpOwnerAlloc = null;

         //=========================================
         // FileAllocStats
         //=========================================
         public FileAllocNode(FileAlloc ownerAlloc)
         {
            mpOwnerAlloc = ownerAlloc;

            this.Text = MemoryNumber.convert(mpOwnerAlloc.getTotalAllocatedBytes(false)) + "\t" + mpOwnerAlloc.getFilename();
         }
       
         public void update()
         {
            Hashtable pLines = mpOwnerAlloc.getLineAllocations();
            if (!Expanded)
            {
               this.Nodes.Clear();
               if (pLines.Count > 0)
                  Nodes.Add(new GDITreeViewNode());
            }
            else
            {
               this.Nodes.Clear();

               ////////////////// //per line
               IDictionaryEnumerator line_enumerator = pLines.GetEnumerator();
               while (line_enumerator.MoveNext())
               {
                  LineAlloc la = ((LineAlloc)line_enumerator.Value);

                  if (la != null)
                  {
                     LineAllocNode lan = new LineAllocNode(la);

                     addNode(lan);
                  }
               }
            }
            this.Text = MemoryNumber.convert(mpOwnerAlloc.getTotalAllocatedBytes(false)) + "\t" + mpOwnerAlloc.getFilename();
         }
      };

      //=========================================
      // HeapAllocStats
      //=========================================
      public class HeapAllocNode : GDITreeViewNode
      {
         public HeapAlloc mpOwnerAlloc = null;
         //=========================================
         // HeapAllocStats
         //=========================================
         public HeapAllocNode(HeapAlloc ownerAlloc)
         {
            mpOwnerAlloc = ownerAlloc;
            this.Text = mpOwnerAlloc.getName() + " : " + MemoryNumber.convert(mpOwnerAlloc.getTotalAllocatedBytes());
         }

         //=========================================
         // update
         //=========================================
         public void update()
         {
            Hashtable pFiles = mpOwnerAlloc.getFileAllocations();

            if (Expanded)
            {
               //do a quick pass to determine if we need to remove stuff..
               for (int i = 0; i < Nodes.Count; i++)
               {
                  if(Nodes[i] is FileAllocNode)
                  {
                     FileAllocNode fan = (FileAllocNode)Nodes[i];
                     if (!pFiles.Contains(fan.mpOwnerAlloc.getFilename()))
                     {
                        Nodes.RemoveAt(i);
                        i--;
                     }
                  }
                  else
                  {
                     Nodes.RemoveAt(i);
                     i--;
                  }
                  
               }

               //our local node list should be completly present in the parent container.
               IDictionaryEnumerator file_enumerator = pFiles.GetEnumerator();
               while (file_enumerator.MoveNext())
               {
                  FileAlloc fa = ((FileAlloc)file_enumerator.Value);

                  bool found = false;
                  for (int i = 0; i < Nodes.Count; i++)
                  {
                     FileAllocNode fan = (FileAllocNode)Nodes[i];
                     if (fan.mpOwnerAlloc == fa)
                     {
                        fan.update();
                        found = true;
                        break;
                     }
                  }
                  if (!found)
                  {
                     if (fa.getTotalAllocatedBytes(false) == 0)
                        continue;
                     FileAllocNode fan = new FileAllocNode(fa);
                     addNode(fan);

                  }
               }
            }
            else
            {
               Nodes.Clear();
               if (pFiles.Count > 0)
                  Nodes.Add(new GDITreeViewNode());
            }

            this.Text = mpOwnerAlloc.getName() + " : " + MemoryNumber.convert(mpOwnerAlloc.getTotalAllocatedBytes());
         }

      };


      #endregion

      private void button1_Click(object sender, EventArgs e)
      {
         SaveFileDialog sfd = new SaveFileDialog();
         sfd.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
         sfd.Filter = "CSV File *.csv|*.csv";
         if (sfd.ShowDialog() == DialogResult.OK)
         {
            
              StreamWriter sw = new StreamWriter(sfd.FileName, true);


              int numHeaps = AllocStats.getNumHeaps();
              for (int i = 0; i < numHeaps; i++)
              {
                 HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);
                 Hashtable pFiles = pHeap.getFileAllocations();

                 sw.WriteLine(pHeap.getName() + ", ," + pHeap.getMaxAllocatedBytes());

                 IDictionaryEnumerator _enumerator = pFiles.GetEnumerator();
                 while (_enumerator.MoveNext())
                 {
                    FileAlloc fa = ((FileAlloc)_enumerator.Value);
                    string fnameOnly = Path.GetFileName(fa.getFilename());
                    uint memAmt = fa.getTotalAllocatedBytes(false);

                   sw.WriteLine(" ," + fa.getFilename() + "," + memAmt);
                 }
              }


               
               
               sw.Close();
               sw = null;
             
         }  
      }



      //=========================================
      //=========================================
      


      

   }
}

