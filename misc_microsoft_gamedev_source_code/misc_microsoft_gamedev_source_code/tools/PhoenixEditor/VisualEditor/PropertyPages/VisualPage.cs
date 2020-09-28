using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using ModelSystem;


namespace VisualEditor.PropertyPages
{
   public partial class VisualPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visual mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public VisualPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visual visual, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = visual;
         mNode = treeNode;


         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();

         bool found = false;
         for (int i = 0; i < comboBox1.Items.Count; i++ )
         {
            if(String.Compare(mData.defaultmodel, (string) comboBox1.Items[i]) == 0)
            {
               comboBox1.SelectedItem = comboBox1.Items[i];
               found = true;
               break;
            }
         }

         if (!found)
         {
            comboBox1.SelectedIndex = -1;
            comboBox1.SelectedItem = null;
            comboBox1.Text = null;
         }

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visual afterChanges = new visual();
         afterChanges.defaultmodel = (string)comboBox1.SelectedItem;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }


      public void refreshComboBoxItems()
      {
         comboBox1.Items.Clear();

         foreach(visualModel model in mData.model)
         {
            if(!String.IsNullOrEmpty(model.name))
               comboBox1.Items.Add(model.name);
         }
      }

      // ---------------------------------
      // Even Handlers
      // ---------------------------------
      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return; 
         
         updateData();
      }
   }
}
