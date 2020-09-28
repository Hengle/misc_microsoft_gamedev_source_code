using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using graphapp;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ApplyMaskPropFrm : UserControl
   {
      MaskGenTreeNode mOwnerMask = null;
      ActionTreeNode mOwnerAction = null;

      public ApplyMaskPropFrm()
      {
         InitializeComponent();
      }

      public void setOwner(MaskGenTreeNode ownerMask, ActionTreeNode ownerAction)
      {
         mOwnerMask = ownerMask;
         mOwnerAction = ownerAction;
         textBox1.Text = mOwnerAction.Text;
      }

      private void button1_Click(object sender, EventArgs e)
      {
         if (mOwnerMask == null)
            return;

         GraphBasedMask gbp = mOwnerMask.Mask;
         mOwnerMask.Mask.GraphMemStream.Position = 0;

         MaskGenFrm mgf = new MaskGenFrm();
         mgf.loadCanvasFromMemoryStream(mOwnerMask.Mask.GraphMemStream);

         PopupEditor pe = new PopupEditor();
         Form frm = pe.ShowPopup(this, mgf, FormBorderStyle.FixedToolWindow, true);
         frm.ShowDialog();


         mOwnerMask.Mask.clearMemStream();
         mgf.saveCanvasToMemoryStream(mOwnerMask.Mask.GraphMemStream);
      }

      private void textBox1_TextChanged(object sender, EventArgs e)
      {
         mOwnerAction.Text = textBox1.Text;
      }
   }
}
