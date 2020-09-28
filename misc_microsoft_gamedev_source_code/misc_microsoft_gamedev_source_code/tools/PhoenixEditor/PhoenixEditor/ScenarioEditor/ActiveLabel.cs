using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ActiveLabel : UserControl
   {
      public ActiveLabel()
      {
         InitializeComponent();
         this.Size = TextLabel.Size;
      }

      string mTextValue = "no value";
      public string TextValue
      {
         set
         {
            TextLabel.Text = value;
            mTextValue = value;
            this.Size = TextLabel.Size;
         }
         get
         {
            return mTextValue;
         }
      }

      private void ActiveDragLablel_MouseEnter(object sender, EventArgs e)
      {
         this.BorderStyle = BorderStyle.FixedSingle;
      }

      private void ActiveDragLablel_MouseLeave(object sender, EventArgs e)
      {
         this.BorderStyle = BorderStyle.None;
      }

      private void TextLabel_Click(object sender, EventArgs e)
      {
         this.OnClick(e);
      }

   }
}
