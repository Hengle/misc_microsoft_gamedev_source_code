using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

using ModelSystem;
using EditorCore;

namespace VisualEditor.PropertyPages
{
   public partial class LogicNodeTechPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualLogicData mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;
      
      public LogicNodeTechPage()
      {
         InitializeComponent();

         // Read tech types
         string techsFileName = "techs.xml";
         string techsFileNameFull = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\" + techsFileName;

         XmlDocument doc = new XmlDocument();
         doc.Load(techsFileNameFull);

         XmlNodeList nodes = null;
         nodes = doc.SelectNodes("//Tech/@name");

         typeComboBox.Items.Clear();
         foreach (XmlNode n in nodes)
         {
            typeComboBox.Items.Add(n.InnerText);
         }
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualLogicData logicData, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = logicData;
         mNode = treeNode;


         // Move data to control data (DATA -> CONTROL DATA)
         //
         refreshComboBoxItems();

         bool found1 = false;
         for (int i = 0; i < typeComboBox.Items.Count; i++)
         {
            if (String.Compare(mData.value, (string)typeComboBox.Items[i], true) == 0)
            {
               typeComboBox.SelectedIndex = i;
               found1 = true;
               break;
            }
         }

         if (!found1)
         {
            typeComboBox.SelectedIndex = -1;
            typeComboBox.SelectedItem = null;
            typeComboBox.Text = null;
         }


         bool found2 = false;
         for (int i = 0; i < comboBox1.Items.Count; i++)
         {
            if (String.Compare(mData.modelref, (string)comboBox1.Items[i], true) == 0)
            {
               comboBox1.SelectedIndex = i;
               found2 = true;
               break;
            }
         }

         if (!found2)
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
         visualLogicData afterChanges = new visualLogicData();
         afterChanges.value = (string)typeComboBox.SelectedItem;
         afterChanges.modelref = (string)comboBox1.SelectedItem;

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }

      public void refreshComboBoxItems()
      {
         comboBox1.Items.Clear();

         foreach (visualModel model in mVisualEditorPage.visualFile.model)
         {
            if (!String.IsNullOrEmpty(model.name))
               comboBox1.Items.Add(model.name);
         }
      }

      // ---------------------------------
      // Even Handlers
      // ---------------------------------
      private void typeComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return; 
         
         updateData();
      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
