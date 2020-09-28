using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.IO;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;
using GDIControls;

namespace RemoteMemory
{
   public partial class FileGroupingElement : UserControl
   {
      //=========================================
      // FileGroupingElement
      //=========================================
      public FileGroupingElement()
      {
         InitializeComponent();
      }


      #region TREE Update

      

      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {
         for (int i = 0; i < mTreeView.Nodes.Count; i++)
         {
            if (mTreeView.Nodes[i] is GroupNode)
            {
               GroupNode pNode = (GroupNode)mTreeView.Nodes[i];
               pNode.update();
            }
         }

         mTreeView.Refresh();
      }

      #endregion

      #region ON EVENTS
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
      #endregion

      #region SAVE LOAD
      //=========================================
      // GroupListXML
      //=========================================
      [XmlRoot("GroupList")]
      public class GroupListXML
      {
         [XmlArrayItem(ElementName = "Group", Type = typeof(GroupFilesXML))]
         [XmlArray("Groups")]
         public List<GroupFilesXML> mGroups = new List<GroupFilesXML>();
      };

      //=========================================
      // GroupFilesXML
      //=========================================
      public class GroupFilesXML
      {
         [XmlAttribute]
         public string GroupName = "?";

         [XmlArrayItem(ElementName = "File", Type = typeof(GroupFileXML))]
         [XmlArray("Files")]
         public List<GroupFileXML> mFiles = new List<GroupFileXML>();
      };

      //=========================================
      // GroupFileXML
      //=========================================
      public class GroupFileXML
      {
         [XmlText]
         public string Filename = "?";
      };

      //=========================================
      // saveCurrentWatchList
      //=========================================
      bool saveCurrentWatchList(string filename)
      {
         //try
         //{
         //   if (File.Exists(filename))
         //      File.Delete(filename);

         //   WatchListXML wlXML = new WatchListXML();
         //   for (int i = 0; i < this.flowLayoutPanel1.Controls.Count; i++)
         //   {
         //      FileTimelineElement pEle = (FileTimelineElement)this.flowLayoutPanel1.Controls[i];
         //      if (pEle == null)
         //         continue;


         //      WatchFileXML wfXML = new WatchFileXML();
         //      wfXML.Filename = pEle.getFilename();
         //      wlXML.mFiles.Add(wfXML);
         //   }

         //   XmlSerializer s = new XmlSerializer(typeof(WatchListXML), new Type[] { });
         //   Stream st = File.Open(filename, FileMode.OpenOrCreate);
         //   s.Serialize(st, wlXML);
         //   st.Close();
         //}
         //catch (Exception e)
         //{
         //   MessageBox.Show(e.InnerException.ToString());
         //   return false;
         //}

         return true;
      }

      //=========================================
      // loadGroupList
      //=========================================
      public bool loadGroupList(string filename)
      {
         Stream st = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(GroupListXML), new Type[] { });
            st = File.OpenRead(filename);
            GroupListXML wlXML = (GroupListXML)s.Deserialize(st);
            st.Close();



            //do loading here..
            for(int i =0 ; i < wlXML.mGroups.Count; i++)
            {
               GroupNode gn = new GroupNode(wlXML.mGroups[i].GroupName);
               for (int j = 0; j < wlXML.mGroups[i].mFiles.Count; j++)
               {
                  GroupFileNode fn = new GroupFileNode(wlXML.mGroups[i].mFiles[j].Filename);
                  gn.addNode(fn);
               }
               mTreeView.addNode(gn);
            }

         }
         catch (Exception e)
         {
            MessageBox.Show("Error Loading " + filename + ":\n" + e.InnerException.ToString());

            if (st != null)
               st.Close();

            if (File.Exists(filename))
               File.Delete(filename);

            return false;
         }

         timer1_Tick(this, null);

         return true;
      }

      //=========================================
      // exportToNotepad
      //=========================================
      public bool exportToNotepad()
      {
         return true;
      }

      //=========================================
      // exportToCSV
      //=========================================
      public bool exportToCSV(string filename)
      {
         
         
         using (StreamWriter sw = new StreamWriter(filename, true))
         {
            sw.WriteLine(" , currentAlloc (MB), maxAlloc (MB)");
            for (int j = 0; j < mTreeView.Nodes.Count; j++)
            {
               if (!(mTreeView.Nodes[j] is GroupNode))
                  continue;

               
               GroupNode gn = mTreeView.Nodes[j] as GroupNode;
               sw.WriteLine(gn.getOutputString());
               for (int i = 0; i < gn.Nodes.Count; i++)
               {
                  if (!(gn.Nodes[i] is GroupFileNode))
                     continue;

                  GroupFileNode gfn = gn.Nodes[i] as GroupFileNode;
                  sw.WriteLine("   " + gfn.getOutputString());
               }
            }
         }

         return true;
      }

      #endregion

      #region statsRegion
      //=========================================
      // FileAllocStats
      //=========================================
      public class GroupFileNode : GDITreeViewNode
      {
         string mFilename = "?";
         public FileAlloc mpOwnerAlloc = null;
         uint mCurrAllocatedBytes = 0;
         uint mMaxAllocatedBytes = 0;
         //=========================================
         // FileAllocStats
         //=========================================
         public GroupFileNode(string filename)
         {
            mFilename = filename;
         }

         //=========================================
         // update
         //=========================================
         public uint update()
         {
            uint totalBytes = 0;

            int numheaps = AllocStats.getNumHeaps();
            for (int i = 0; i < numheaps; i++)
            {
               HeapAlloc pHeap = AllocStats.getHeapFromIndex(i);

               IDictionaryEnumerator _enumerator = pHeap.getFileAllocations().GetEnumerator();
               while (_enumerator.MoveNext())
               {
                  FileAlloc fa = ((FileAlloc)_enumerator.Value);
                  string longFName = fa.getFilename();

                  if (longFName.Contains("xbox\\code"))
                  {
                     string trimedFName = giveTrimmedString(longFName);
                     if (trimedFName.ToLower() == mFilename.ToLower())
                        totalBytes += fa.getTotalAllocatedBytes(false);
                  }
                  else
                  {
                     if (longFName.ToLower() == mFilename.ToLower())
                        totalBytes += fa.getTotalAllocatedBytes(false);
                  }
                  
               }
            }

            if (totalBytes > mMaxAllocatedBytes)
               mMaxAllocatedBytes = totalBytes;

            mCurrAllocatedBytes = totalBytes;

            this.Text = Path.GetFileName(mFilename) + ", curr :  " + MemoryNumber.convert(totalBytes) + " , max : " + MemoryNumber.convert(mMaxAllocatedBytes);

            
            return totalBytes;
         }
         //=========================================
         // getOutputString
         //=========================================
         public string getOutputString()
         {
            uint kb = 1024;
            uint mb = kb * kb;

            float CurrAllocatedBytes = ((uint)(mCurrAllocatedBytes / (float)mb * 1000) / 1000.0f);
            float MaxAllocatedBytes = ((uint)(mMaxAllocatedBytes / (float)mb * 1000) / 1000.0f);

            return Path.GetFileName(mFilename) + "," + CurrAllocatedBytes + "," + MaxAllocatedBytes;
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
      };

      //=========================================
      // HeapAllocStats
      //=========================================
      public class GroupNode : GDITreeViewNode
      {
         string mGroupName;
         uint mMaxAllocatedBytes = 0;
         uint mCurrAllocatedBytes = 0;
         //=========================================
         // GroupNode
         //=========================================
         public GroupNode(string groupname)
         {
            mGroupName = groupname;
         }

         //=========================================
         // GroupNode
         //=========================================
         void addWatchedFile(string filename)
         {

            GroupFileNode gfn = new GroupFileNode(filename);
            this.addNode(gfn);
         }

         //=========================================
         // update
         //=========================================
         public void update()
         {
            uint totalBytes = 0;

            for (int i = 0; i < this.Nodes.Count; i++)
            {
               if(this.Nodes[i] is GroupFileNode)
               {
                  GroupFileNode pNode = (GroupFileNode)this.Nodes[i];
                  totalBytes += pNode.update();
               }
            }

            if (totalBytes > mMaxAllocatedBytes)
               mMaxAllocatedBytes = totalBytes;

            mCurrAllocatedBytes = totalBytes;

            this.Text = mGroupName + ", curr :  " + MemoryNumber.convert(totalBytes) + " , max : " + MemoryNumber.convert(mMaxAllocatedBytes);
         }

         //=========================================
         // getOutputString
         //=========================================
         public string getOutputString()
         {
            uint kb = 1024;
            uint mb = kb*kb;

            float CurrAllocatedBytes = ((uint)(mCurrAllocatedBytes / (float)mb * 1000) / 1000.0f);
            float MaxAllocatedBytes = ((uint)(mMaxAllocatedBytes / (float)mb * 1000) / 1000.0f);

            return mGroupName + "," + CurrAllocatedBytes + "," + MaxAllocatedBytes;
         }

      };
      #endregion

   }
}
