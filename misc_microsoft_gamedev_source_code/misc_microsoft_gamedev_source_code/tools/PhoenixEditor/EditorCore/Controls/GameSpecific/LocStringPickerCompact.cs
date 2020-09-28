using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore.Controls.Micro;

namespace EditorCore.Controls
{
   public interface ILocStringPicker
   {
      int LocStringID
      {
         get;
         set;
      }
      event LocStringPicker.LocEvent IDChanged;

   }
   //public delegate void LocEvent(object sender, LocString locString);

   public partial class LocStringPickerCompact : UserControl, ILocStringPicker
   {
      public LocStringPickerCompact()
      {
         InitializeComponent();
         SearchButton.Image = SharedResources.GetImage(Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "Search.bmp"));

         IDTextBox.TextChanged+=new EventHandler(IDTextBox_TextChanged);
      }

      private void SearchButton_Click(object sender, EventArgs e)
      {
         LocStringPicker p = new LocStringPicker();
         p.LocStringID = this.LocStringID;
         p.Height = mPopupHeight;
         if (ContentMessageBox.ShowMessage(this, p, "") == DialogResult.OK)
         {
            this.LocStringID = p.LocStringID;
         }

      }
      public int mPopupHeight = 600;

      public const int cInvalidID = int.MinValue;

      static private string mLastScenario = "";

//      public delegate void LocEvent(object sender, LocString locString);
      public event LocStringPicker.LocEvent IDChanged = null;

      private LocString mLocString = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public LocString LocString
      {
         get
         {
            return mLocString;
         }
         set
         {
            mLocString = value;
            SetUI(mLocString);
         }
      }
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public int LocStringID
      {
         get
         {
            if (mLocString == null)
               return cInvalidID;
            return mLocString.mLocID;
         }
         set
         {
            LocString val;
            if (CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(value.ToString(), out val))
            {
               mLocString = val;

               SetUI(mLocString);
            }
         }
      }

      private void SetUI(LocString value)
      {
         StringTextLabel.Text = value.mString;
         IDTextBox.Text = value.mLocID.ToString();
         StringTextLabel.ForeColor = System.Drawing.Color.Black;
      }

      private void IDTextBox_TextChanged(object sender, EventArgs e)
      {
         int id = 0;
         if (int.TryParse(IDTextBox.Text, out id))
         {
            LocString value;
            if (CoreGlobals.getGameResources().mStringTable.mStringsByID.TryGetValue(id.ToString(), out value))
            {
               mLocString = value;
               StringTextLabel.Text = value.mString;
               if (IDChanged != null)
               {
                  IDChanged.Invoke(this, mLocString);
               }
               StringTextLabel.ForeColor = System.Drawing.Color.Black;
               return;
            }
            else
            {
               StringTextLabel.Text = "###ID not found";
               StringTextLabel.ForeColor = System.Drawing.Color.Red;
            }
         }
         else
         {
            StringTextLabel.Text = "###Please type an id number";
            StringTextLabel.ForeColor = System.Drawing.Color.Red;
         }
      }

   }




}
