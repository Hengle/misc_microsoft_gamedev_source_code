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
   public partial class SearchPanel : UserControl
   {
 
      public SearchPanel()
      {
         InitializeComponent();

         SearchButton.Image = SharedResources.GetImage(Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "Search.bmp"));
      }

      public event EventHandler SearchButtonClicked;

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

      protected override void OnResize(EventArgs e)
      {
         base.OnResize(e);
      }

      private void SearchButton_Click_1(object sender, EventArgs e)
      {
         if (SearchButtonClicked != null)
         {
            SearchButtonClicked.Invoke(this, null);
         }     
      }

   }

   
}
