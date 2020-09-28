using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class CategoryTreeEditor : UserControl
   {
      public CategoryTreeEditor()
      {
         InitializeComponent();

         this.collectionChooser1.AutoSort = true;
         this.collectionChooser1.AllowRepeats = false;
      }

      Category mCat = null;
      List<string> mTempEntries = new List<string>();

      public void Setup(CategoryTreeNode settings, Category cat)
      {
         mCat = cat;
         this.textBox1.Text = mCat.Name;

         this.collectionChooser1.SetOptions(settings.mAllEntries);

         mTempEntries.Clear();
         mTempEntries.AddRange(mCat.Entries);
         this.collectionChooser1.BoundSelectionList = mTempEntries;

      }

      protected void SaveSettings()
      {
         mCat.Name = this.textBox1.Text;
         mCat.Entries.Clear();
         mCat.Entries.AddRange(mTempEntries);
      }

      private void OkButton_Click(object sender, EventArgs e)
      {
         SaveSettings();

         this.ParentForm.DialogResult = DialogResult.OK;
         this.ParentForm.Close();

      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.Cancel;
         this.ParentForm.Close();

      }
   }

   
}
