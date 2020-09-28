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
   public partial class AnimPage : UserControl
   {
      private VisualEditorPage mVisualEditorPage = null;
      private visualModelAnim mData = null;
      private TreeNode mNode = null;
      private bool mIsBindingData = false;

      public AnimPage()
      {
         InitializeComponent();

         // Initialize combo box
         Type enumtype = typeof(visualModelAnim.AnimType);
         foreach (string s in Enum.GetNames(enumtype))
            comboBox1.Items.Add(s);

         comboBox1.SelectedIndex = 0;
      }

      public void setVisualEditorPage(VisualEditorPage page)
      {
         mVisualEditorPage = page;
      }

      public void bindData(visualModelAnim anim, TreeNode treeNode)
      {
         mIsBindingData = true;

         mData = anim;
         mNode = treeNode;

         // Move data to control data (DATA -> CONTROL DATA)
         //
         comboBox1.Text = mData.type;
         tweenTimeNumericUpDown.Value = mData.tweenTime;
         tweenToAnimation.Text = mData.tweenToAnimation;
         loopRadioButton.Checked = false;
         freezeRadioButton.Checked = false;
         transitionRadioButton.Checked = false;
         if (mData.exitAction == visualModelAnim.AnimExitAction.Transition)
         {
            tweenToAnimation.Enabled = true;

            transitionRadioButton.Checked = true;
         }
         else
         {
            tweenToAnimation.Enabled = false;

            if (mData.exitAction == visualModelAnim.AnimExitAction.Loop)
               loopRadioButton.Checked = true;
            else if (mData.exitAction == visualModelAnim.AnimExitAction.Freeze)
               freezeRadioButton.Checked = true;
         }

         mIsBindingData = false;
      }

      public void updateData()
      {
         if ((mData == null) || (mNode == null))
            return;

         // Move control data to data (CONTROL DATA -> DATA)
         //
         visualModelAnim afterChanges = new visualModelAnim();
         afterChanges.type = comboBox1.Text;
         afterChanges.tweenTime = (int)tweenTimeNumericUpDown.Value;
         afterChanges.tweenToAnimation = tweenToAnimation.Text;
         if (transitionRadioButton.Checked)
         {
            tweenToAnimation.Enabled = true;

            afterChanges.exitAction = visualModelAnim.AnimExitAction.Transition;
         }
         else
         {
            tweenToAnimation.Enabled = false;

            if (loopRadioButton.Checked)
               afterChanges.exitAction = visualModelAnim.AnimExitAction.Loop;
            else if (freezeRadioButton.Checked)
               afterChanges.exitAction = visualModelAnim.AnimExitAction.Freeze;
         }

         // Add/Execute undo action
         UndoRedoChangeDataAction undoAction = new UndoRedoChangeDataAction(mData, afterChanges);
         mVisualEditorPage.mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
      }

      // ---------------------------------
      // Even Handlers
      // ---------------------------------

      private void comboBox1_TextChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void tweenTimeNumericUpDown_ValueChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void transitionRadioButton_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void freezeRadioButton_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void loopRadioButton_CheckedChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }

      private void tweenToAnimation_TextChanged(object sender, EventArgs e)
      {
         // early out if currently binding data
         if (mIsBindingData)
            return;

         updateData();
      }
   }
}
