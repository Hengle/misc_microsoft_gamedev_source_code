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
   public partial class LogicPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualLogic mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public LogicPage()
      {
         InitializeComponent();
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualLogic logicData, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = logicData;
         mNode = treeNode;

         // Disable all controls if not variation logic type
         //
         if(mData.type != visualLogic.LogicType.Variation)
         {
            labelComment.Visible = false;
            label1.Visible = false;
            comboBox1.Visible = false;
         }
         else
         {
            labelComment.Visible = true;
            label1.Visible = true;
            comboBox1.Visible = true;
         }


         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();

         if(comboBox1.Items.Count > mData.selectedItem)
            comboBox1.SelectedIndex = mData.selectedItem;
         else
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
         visualLogic afterChanges = new visualLogic();
         afterChanges.selectedItem = comboBox1.SelectedIndex;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }

      public void refreshComboBoxItems()
      {
         comboBox1.Items.Clear();

         foreach (visualLogicData curLogicData in mData.logicdata)
         {
            if (!String.IsNullOrEmpty(curLogicData.value))
               comboBox1.Items.Add(curLogicData.value);
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

         // This is a bit of a hack here.  But we must load the newly selected node
         EditorCore.BBoundingBox bb = new EditorCore.BBoundingBox();
         mVisualEditorPage.preLoadLogicData(mData.logicdata[comboBox1.SelectedIndex], ref bb);
      }
   }
}
