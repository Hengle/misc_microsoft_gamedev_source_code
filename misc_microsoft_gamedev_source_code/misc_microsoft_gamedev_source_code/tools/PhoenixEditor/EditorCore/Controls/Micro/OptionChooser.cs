using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class OptionChooser : UserControl
   {
      public OptionChooser()
      {
         InitializeComponent();
      }

      ICollection<string> mOptions = new List<string>();
      Dictionary<string, string> mOptionToValue = null;
      Dictionary<string, string> mValueToOption = null;

      public void SetOptions(ICollection<string> options)
      {
         this.OptionListBox.Items.Clear();

         mOptions = options;
         foreach (string s in options)
         {
            OptionListBox.Items.Add(s, false);
         }

         OptionListBox.Sorted = true;
      }
      public void SetOptions(ICollection<string> options, ICollection<string> valuesCodes)
      {
         SetOptions(options);
         mOptionToValue = new Dictionary<string, string>();
         mValueToOption = new Dictionary<string,string>();
         IEnumerator<string> op =  options.GetEnumerator();
         IEnumerator<string> val =  valuesCodes.GetEnumerator();
         while (op.MoveNext() && val.MoveNext())
         {
            mOptionToValue[op.Current] = val.Current;
            mValueToOption[val.Current] = op.Current;
         }
         
      }


      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      ICollection<string> mBoundSelectionList = new List<string>();
      public ICollection<string> BoundSelectionList
      {
         set
         {
            if (mOptionToValue == null)
            {

               mBoundSelectionList = value;
               foreach (string s in mBoundSelectionList)
               {
                  int indx = OptionListBox.Items.IndexOf(s);
                  if (indx != -1)
                  {
                     OptionListBox.SetItemChecked(indx, true);
                  }
               }
            }
            else 
            {
               mBoundSelectionList = value;
               foreach (string s in mBoundSelectionList)
               {
                  if (!mValueToOption.ContainsKey(s))
                  {
                     CoreGlobals.ShowMessage("Missing value for: " + s);
                     continue;
                     
                  }
                  int indx = OptionListBox.Items.IndexOf(mValueToOption[s]);
                  if (indx != -1)
                  {
                     OptionListBox.SetItemChecked(indx, true);
                  }
               }
            }
         }
      }

      bool mbPaused = false;
      private void SelcectAllButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;

         for (int i = 0; i < OptionListBox.Items.Count; i++)
         {
            OptionListBox.SetItemChecked(i, true);
         }

         mbPaused = false;

         foreach (object item in OptionListBox.Items)
         {
            mBoundSelectionList.Add(item.ToString());
         }
      }

      private void ClearButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;
         for (int i = 0; i < OptionListBox.Items.Count; i++)
         {
            OptionListBox.SetItemChecked(i, false);
         }
         mbPaused = false;

         mBoundSelectionList.Clear();
 
      }

      public event EventHandler OptionsChanged = null;

      private void OptionListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (mbPaused)
            return;
         if (mOptionToValue == null)
         {

            mBoundSelectionList.Clear();
            foreach (object item in OptionListBox.CheckedItems)
            {
               mBoundSelectionList.Add(item.ToString());
            }
         }
         else
         {

            mBoundSelectionList.Clear();
            foreach (object item in OptionListBox.CheckedItems)
            {
               mBoundSelectionList.Add(mOptionToValue[item.ToString()]);
            }
         }
         if(OptionsChanged != null)
         {
            OptionsChanged.Invoke(this, null);
         }

      }

   }
}
