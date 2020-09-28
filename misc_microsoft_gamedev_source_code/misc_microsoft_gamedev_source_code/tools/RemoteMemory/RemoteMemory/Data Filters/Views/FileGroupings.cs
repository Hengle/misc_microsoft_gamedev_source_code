using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace RemoteMemory
{
   public partial class FileGroupings : UserControl, HaloWarsMemoryEventListener
   {
      //=========================================
      // FileGroupings
      //=========================================
      public FileGroupings()
      {
         InitializeComponent();
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
         for(int i = 0 ; i < tabControl1.TabPages.Count; i++)
         {
            for (int j = 0; j < tabControl1.TabPages[i].Controls.Count; j++)
            {
               if(tabControl1.TabPages[i].Controls[j] is FileGroupingElement)
               {
                  FileGroupingElement fge = (FileGroupingElement)tabControl1.TabPages[i].Controls[j];
                  fge.onConnect();
               }
            }
         }
      }
      //=========================================
      // onDisconnect
      //=========================================
      public void onDisconnect()
      {
         for (int i = 0; i < tabControl1.TabPages.Count; i++)
         {
            for (int j = 0; j < tabControl1.TabPages[i].Controls.Count; j++)
            {
               if (tabControl1.TabPages[i].Controls[j] is FileGroupingElement)
               {
                  FileGroupingElement fge = (FileGroupingElement)tabControl1.TabPages[i].Controls[j];
                  fge.onDisconnect();
               }
            }
         }
         
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
      private void button3_Click(object sender, EventArgs e)
      {
         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Filter = "Group File List .gfl|*.gfl";
         ofd.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;

         if (ofd.ShowDialog() == DialogResult.OK)
         {
            FileGroupingElement fge = new FileGroupingElement();
            if (!fge.loadGroupList(ofd.FileName))
               return;
            fge.onConnect();
            fge.Dock = DockStyle.Fill;


            TabPage tp = new TabPage();
            tp.Controls.Add(fge);
            tp.Text = Path.GetFileName(ofd.FileName);
            tp.BackColor = Color.FromArgb(63, 63, 63);

            tabControl1.TabPages.Add(tp);
            tabControl1.Visible = true;
         }
      }
      //=========================================
      // exportToNotepad
      //=========================================
      private void pictureBox2_Click(object sender, EventArgs e)
      {
         TabPage selTab = tabControl1.SelectedTab;
         if (selTab == null)
            return;


         for (int j = 0; j < selTab.Controls.Count; j++)
         {
            if (selTab.Controls[j] is FileGroupingElement)
            {
               FileGroupingElement fge = (FileGroupingElement)selTab.Controls[j];
               fge.exportToNotepad();
            }
         }
      }
      //=========================================
      // button1_Click
      //=========================================
      private void button1_Click(object sender, EventArgs e)
      {
         if (tabControl1.TabCount ==0)
            return;

         TabPage selTab = tabControl1.SelectedTab;
         

         SaveFileDialog sfd = new SaveFileDialog();
         sfd.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
         sfd.Filter = "CSV File *.csv|*.csv";
         if (sfd.ShowDialog() != DialogResult.OK)
            return;

         for (int j = 0; j < selTab.Controls.Count; j++)
         {
            if (selTab.Controls[j] is FileGroupingElement)
            {
               FileGroupingElement fge = (FileGroupingElement)selTab.Controls[j];
               fge.exportToCSV(sfd.FileName);

               Process.Start(sfd.InitialDirectory);
               return;
            }
         }
      }
   }
}
