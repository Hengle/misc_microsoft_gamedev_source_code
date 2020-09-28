using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace EditorCore
{
   public partial class RefSetPanel : UserControl
   {
      public RefSetPanel()
      {
         InitializeComponent();

         this.SetRefButton.Image = SharedResources.GetImage(Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "VisualEditor_Refresh.png"));
         this.ClearRefButton.Image = SharedResources.GetImage(Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "DeleteHS.png"));
      }

      public event EventHandler RefButtonClicked;
      public event EventHandler ClearRefButtonClicked;

      Control mGuestControl = null;

      public Control GuestControl
      {
         set
         {
            mGuestControl = value;

            this.guestControlPanel.SuspendLayout();
            this.guestControlPanel.Controls.Clear();
            this.guestControlPanel.Controls.Add(mGuestControl);
            mGuestControl.Dock = DockStyle.Fill;
            this.guestControlPanel.ResumeLayout();

         }
         get
         {
            return mGuestControl;
         }

      }
      override public string Text
      {
         get
         {
            if (mGuestControl != null)
               return mGuestControl.Text;
            return "";
         }
         set
         {
            if (mGuestControl != null)
               mGuestControl.Text = value;
         }
      }

      private void SetRefButton_Click(object sender, EventArgs e)
      {
         if(RefButtonClicked != null)
         {
            RefButtonClicked.Invoke(this, null);
         }
      }
      private void ClearRefButton_Click(object sender, EventArgs e)
      {
         if (ClearRefButtonClicked != null)
         {
            ClearRefButtonClicked.Invoke(this, null);
         }
      }
      protected override void OnResize(EventArgs e)
      {
         //SetRefButton.Width = SetRefButton.Height;
         base.OnResize(e);
      }

   }
}
