using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace graphapp
{
   public partial class Device_MaskFromListDlg : Form
   {
      public Device_MaskFromList mOwningDevice = null;
      public Device_MaskFromListDlg(Device_MaskFromList owningDev)
      {
         mOwningDevice = owningDev;
         InitializeComponent();
      }

      private void Device_InputMaskFromListDlg_Load(object sender, EventArgs e)
      {
         List<string> maskNames = CoreGlobals.getEditorMain().mIMaskPickerUI.GetMaskNames();
         if(maskNames.Count ==0)
         {
            MessageBox.Show("This device requires you to have masks listed in the masks tab.");
            this.Close();
            return;
         }
         maskListComboBox.Items.AddRange(maskNames.ToArray());
         maskNames.Clear();
         maskNames = null;

         maskListComboBox.SelectedIndex = 0;

         if (!(mOwningDevice.SelectedMaskName == "" || mOwningDevice.SelectedMaskName == null))
         {
            int idx = maskListComboBox.Items.IndexOf(mOwningDevice.SelectedMaskName);
            if(idx != -1)
               maskListComboBox.SelectedIndex = idx;
         }

      }

      private void maskListComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {

      }

      private void button1_Click(object sender, EventArgs e)
      {
         mOwningDevice.SelectedMaskName = maskListComboBox.SelectedItem as string;
         mOwningDevice.generatePreview();
      }
   }
}