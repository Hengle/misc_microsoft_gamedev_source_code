using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class OpenStringList : UserControl
   {
      public OpenStringList()
      {
         InitializeComponent();


         this.listBox1.AllowDrop = true;
         this.listBox1.DragEnter += new DragEventHandler(listBox1_DragEnter);
         this.listBox1.DragDrop += new DragEventHandler(listBox1_DragDrop);

         this.KeyDown += new KeyEventHandler(OpenStringList_KeyDown);
         listBox1.KeyDown += new KeyEventHandler(OpenStringList_KeyDown);
      }

      void OpenStringList_KeyDown(object sender, KeyEventArgs e)
      {
         if ((e.KeyData == (Keys.Control | Keys.V)) ||
             (e.KeyData == (Keys.Shift | Keys.Insert)))
         {
            PasteButton_Click(sender, e);
         }
      }

      void listBox1_DragDrop(object sender, DragEventArgs e)
      {

      }

      void listBox1_DragEnter(object sender, DragEventArgs e)
      {

      }
      private void UpdateUI()
      {
         this.listBox1.Items.Clear();
         this.listBox1.Items.AddRange(mStrings.ToArray());
      }

      public event EventHandler ListChanged = null;

      private void ValuesChanged()
      {
         if (ListChanged != null)
         {
            ListChanged.Invoke(this, null);
         }
      }

      private void PasteButton_Click(object sender, EventArgs e)
      {
         string data = (string)Clipboard.GetDataObject().GetData(DataFormats.Text);
         if (data != string.Empty)
         {
            ImportList(data);
            UpdateUI();
            ValuesChanged();
         }
      }
      public void ImportList(string data)
      {
         mStrings.Clear();
         if (data == "")
            return;

         string[] byComma = data.Split(',');
         string[] byTab = data.Split('\t');
         string[] byNewLine = data.Split(new string[] { Environment.NewLine }, StringSplitOptions.None);

         if (byComma.Length > 1 && byComma.Length > byTab.Length)
         {
            mStrings.AddRange(byComma);
         }
         else if (byTab.Length > 1 && byTab.Length > byComma.Length)
         {
            mStrings.AddRange(byTab);
         }
         else if (byNewLine.Length > 1)
         {
            mStrings.AddRange(byNewLine);
         }
         else
         {
            mStrings.Add(data);
         }
         //?try xml parse?
      }

      List<string> mStrings = new List<string>();
      public ICollection<string> Strings
      {
         set
         {
            mStrings.Clear();
            mStrings.AddRange(value);
            UpdateUI();
         }
         get
         {
            return mStrings;
         }
      }

   }
}
