using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class ContentMessageBox : Form
   {
      public ContentMessageBox()
      {
         InitializeComponent();
         this.MessageLabel.Text = "";
      }

      static public DialogResult ShowMessage(Control parent, Control content, string message)
      {
         ContentMessageBox mb = new ContentMessageBox();
         mb.Content = content;
         mb.Message = message;

         return mb.ShowDialog(parent);
      }

      Control mContent;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public Control Content
      {
         set
         {
            mContent = value;

            int xOffset = this.Width - this.panel1.Width;
            int yOffset = this.Height - this.panel1.Height;

            this.Size = new Size(mContent.Size.Width + xOffset, mContent.Size.Height + yOffset);

            this.panel1.Controls.Add(mContent);
            mContent.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Bottom | AnchorStyles.Right;
         }
         get
         {
            return mContent;
         }
      }

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public string Message
      {
         set
         {
            this.MessageLabel.Text = value;
         }
      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.Cancel;
         this.Close();
      }
   }
}