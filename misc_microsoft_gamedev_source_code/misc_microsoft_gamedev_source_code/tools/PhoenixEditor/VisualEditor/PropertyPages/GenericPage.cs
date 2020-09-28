using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace VisualEditor.PropertyPages
{
   public partial class GenericPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private object mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public GenericPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(object data, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = data;
         mNode = treeNode;

         propertyGrid.SelectedObject = data;

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Update text in TreeView node
         mVisualEditorPage.UpdateTreeNodeText(mNode);
      }



      // ---------------------------------
      // Even Handlers
      // ---------------------------------

      private void VisualPropertyGrid_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return; 
         
         updateData();
      }
   }
}
