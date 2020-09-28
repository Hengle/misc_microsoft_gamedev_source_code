using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class ListBuilder : UserControl
   {
      public ListBuilder()
      {
         InitializeComponent();

         EnumListBox.Sorted = true;
         EnumListBox.DoubleClick += new EventHandler(AddButton_Click);
         CollectionListBox.DoubleClick += new EventHandler(RemoveButton_Click);

      }
      ICollection<string> mOptions = new List<string>();
      Dictionary<string, string> mOptionToValue = null;
      Dictionary<string, string> mValueToOption = null;
      public event EventHandler OptionsChanged = null;


      public void SetOptions(ICollection<string> options)
      {

         this.EnumListBox.Items.Clear();

         mOptions = options;
         foreach (string s in options)
         {
            EnumListBox.Items.Add(s);
         }

      }
      public void SetOptions(ICollection<string> options, ICollection<string> valuesCodes)
      {
         SetOptions(options);
         mOptionToValue = new Dictionary<string, string>();
         mValueToOption = new Dictionary<string, string>();
         IEnumerator<string> op = options.GetEnumerator();
         IEnumerator<string> val = valuesCodes.GetEnumerator();
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
               CollectionListBox.Items.Clear();
               foreach (string s in mBoundSelectionList)
               {
                  CollectionListBox.Items.Add(s);
               }
            }
            else
            {
               mBoundSelectionList = value;
               foreach (string s in mBoundSelectionList)
               {
                  CollectionListBox.Items.Add(mValueToOption[s]);
               }
            }
         }
      }

      private void AddButton_Click(object sender, EventArgs e)
      {
         if (mOptionToValue == null)
         {
            foreach (object item in EnumListBox.SelectedItems)
            {
               mBoundSelectionList.Add(item.ToString());
               CollectionListBox.Items.Add(item.ToString());
            }
         }
         else
         {
            foreach (object item in EnumListBox.SelectedItems)
            {
               mBoundSelectionList.Add(mOptionToValue[item.ToString()]);
               CollectionListBox.Items.Add(item.ToString());

            }
         }
         if (OptionsChanged != null)
         {
            OptionsChanged.Invoke(this, null);
         }
      }

      private void RemoveButton_Click(object sender, EventArgs e)
      {
         List<object> hitlist = new List<object>();
         if (mOptionToValue == null)
         {
            foreach (object item in CollectionListBox.SelectedItems)
            {
               mBoundSelectionList.Remove(item.ToString());
               hitlist.Add(item);
            }
         }
         else
         {
            foreach (object item in CollectionListBox.SelectedItems)
            {
               mBoundSelectionList.Remove(mOptionToValue[item.ToString()]);
               hitlist.Add(item);
            }
         }
         foreach (object o in hitlist)
         {
            CollectionListBox.Items.Remove(o);
         }
         if (OptionsChanged != null)
         {
            OptionsChanged.Invoke(this, null);
         }
      }
   }
}
