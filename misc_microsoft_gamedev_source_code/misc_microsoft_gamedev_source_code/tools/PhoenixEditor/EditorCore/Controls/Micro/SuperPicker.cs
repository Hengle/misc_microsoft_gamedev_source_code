using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public interface IContentProvider
   {
      Control ProvideContent(string key, Control lastContent);

   }

   public partial class SuperPicker : UserControl
   {
      public SuperPicker()
      {
         InitializeComponent();

         categoryTree1.SelectedCategoryChanged += new EventHandler(categoryTree1_SelectedCategoryChanged);
         autoTree1.SelectedItemChanged += new EventHandler(autoTree1_SelectedItemChanged);
      }

      void autoTree1_SelectedItemChanged(object sender, EventArgs e)
      {
         string key = autoTree1.SelectedItem;
         this.SelectionLabel.Text = key;

         if (mContentProfivider != null)
         {
            SetContent(mContentProfivider.ProvideContent(key, mLastContent));
         }
      }

      public IContentProvider mContentProfivider = null;

      Control mContent = null;
      Control mLastContent = null;

      void SetContent(Control content)
      {
         mContent = content;
         if (mContent != mLastContent && mContent != null)
         {
            ClearContent();
            panel1.Controls.Add(content);
         }
         mLastContent = mContent;
      }
      void ClearContent()
      {
         panel1.Controls.Clear();
      }


      void categoryTree1_SelectedCategoryChanged(object sender, EventArgs e)
      {
         SetCategory(categoryTree1.SelectedCategory);
      }

      CategoryTreeNode mSettings = null;
      public void Setup(CategoryTreeNode settings)//ICollection<string> entries)
      {
         mSettings = settings;
         this.categoryTree1.Setup(mSettings);

      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.OK;
         this.ParentForm.Close();        
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.Cancel;
         this.ParentForm.Close();
      }

      
      private void SetCategory(Category category)
      {
         autoTree1.Clear();
         if (category == null)
         {

         }
         else
         {
            ICollection<string> content = CategoryTreeLogic.GetContent(mSettings, category);
            autoTree1.SetContent(content);
            
         }


      }


   }



}
