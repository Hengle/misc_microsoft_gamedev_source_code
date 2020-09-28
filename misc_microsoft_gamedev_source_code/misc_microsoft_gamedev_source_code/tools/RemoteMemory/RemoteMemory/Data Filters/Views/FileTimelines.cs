using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using GDIControls;

namespace RemoteMemory
{
   public partial class FileTimelines : UserControl, HaloWarsMemoryEventListener
   {

      Hashtable mFileControlHash = new Hashtable(); // KEY is filename, VALUE is FilenameControlBridge

      //=========================================
      // FilenameControlBridge
      //=========================================
      class FilenameControlBridge
      {
         public string mFilename;
         public Control mAssociatedControl = null;
      };
      //=========================================
      // FileTimelines
      //=========================================
      public FileTimelines()
      {
         InitializeComponent();
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

         for (int i = 0; i < this.flowLayoutPanel1.Controls.Count; i++)
         {
            FileTimelineElement pEle = (FileTimelineElement)this.flowLayoutPanel1.Controls[i];
            if (pEle == null)
               continue;

            pEle.onConnect();
         }
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         timer1.Enabled = false;
         for (int i = 0; i < this.flowLayoutPanel1.Controls.Count; i++)
         {
            FileTimelineElement pEle = (FileTimelineElement)this.flowLayoutPanel1.Controls[i];
            if (pEle == null)
               continue;

            pEle.onDisconnect();
         }
      }
      //=========================================
      // onHeapRegister
      //=========================================
      public void onHeapRegister(uint mPtr, int flags, string name)
      {

      }
      #endregion

      #region SAVE LOAD
      //=========================================
      // saveCurrentWatchList
      //=========================================
      [XmlRoot("WatchList")]
      public class WatchListXML
      {
         [XmlArrayItem(ElementName = "WatchFiles", Type = typeof(WatchFileXML))]
         [XmlArray("WatchFile")]
         public List<WatchFileXML> mFiles = new List<WatchFileXML>();
      };

      //=========================================
      // saveCurrentWatchList
      //=========================================
      public class WatchFileXML
      {
         [XmlText]
         public string Filename = "?";
      };

      //=========================================
      // saveCurrentWatchList
      //=========================================
      bool saveCurrentWatchList(string filename)
      {
         try
         {
            if (File.Exists(filename))
               File.Delete(filename);

            WatchListXML wlXML = new WatchListXML();
            for(int i = 0 ; i< this.flowLayoutPanel1.Controls.Count; i++)
            {
               FileTimelineElement pEle = (FileTimelineElement)this.flowLayoutPanel1.Controls[i];
               if (pEle == null)
                  continue;

               
               WatchFileXML wfXML = new WatchFileXML();
               wfXML.Filename = pEle.getFilename();
               wlXML.mFiles.Add(wfXML);
            }

            XmlSerializer s = new XmlSerializer(typeof(WatchListXML), new Type[] { });
            Stream st = File.Open(filename, FileMode.OpenOrCreate);
            s.Serialize(st, wlXML);
            st.Close();
         }
         catch (Exception e)
         {
            MessageBox.Show(e.InnerException.ToString());
            return false;
         }

         return true;
      }

      //=========================================
      // loadWatchList
      //=========================================
      bool loadWatchList(string filename)
      {
         Stream st = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(WatchListXML), new Type[] { });
            st = File.OpenRead(filename);
            WatchListXML wlXML = (WatchListXML)s.Deserialize(st);
            st.Close();

            for(int i = 0 ; i < wlXML.mFiles.Count;i++)
            {
               addElement(wlXML.mFiles[i].Filename);
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

         return true;
      }

      #endregion

      //=========================================
      // addElement
      //=========================================
      private void addElement(string strFName)
      {
         FileTimelineElement fte = new FileTimelineElement(strFName, this);
         fte.onConnect();

         flowLayoutPanel1.Controls.Add(fte);
         flowLayoutPanel1.Invalidate();

         FilenameControlBridge fcb = new FilenameControlBridge();
         fcb.mFilename = strFName;
         fcb.mAssociatedControl = fte;
         mFileControlHash.Add(strFName, fcb);
      }
      //=========================================
      // button1_Click
      //=========================================
      private void button1_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Source .cpp|*.cpp|Header .h|*.h|Inline .inl|*.inl";
         ofd.InitialDirectory = GlobalSettings.LocalCodePath;

         if(ofd.ShowDialog() == DialogResult.OK)
         {
            string strFName = giveTrimmedString(ofd.FileName);
            addElement(strFName);
         }
      }

      //=========================================
      // button1_Click
      //=========================================
      public void removeControl(string filename, Control ctrl)
      {
         flowLayoutPanel1.Controls.Remove(ctrl);
         mFileControlHash.Remove(filename);
      }

      //=========================================
      // timer1_Tick
      //=========================================
      private void timer1_Tick(object sender, EventArgs e)
      {

      }

      //=========================================
      // button2_Click
      //=========================================
      private void button2_Click(object sender, EventArgs e)
      {
         SaveFileDialog ofd = new SaveFileDialog();
         ofd.Filter = "Watch Lists .xml|*.xml";
         ofd.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;

         if (ofd.ShowDialog() == DialogResult.OK)
         {
            saveCurrentWatchList(ofd.FileName);
         }
      }

      //=========================================
      // button3_Click
      //=========================================
      private void button3_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Watch Lists .xml|*.xml";
         ofd.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;

         if (ofd.ShowDialog() == DialogResult.OK)
         {
            loadWatchList(ofd.FileName);
         }
      }



   }
}
