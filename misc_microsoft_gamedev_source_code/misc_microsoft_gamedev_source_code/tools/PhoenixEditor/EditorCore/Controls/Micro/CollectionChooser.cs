using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace EditorCore.Controls.Micro
{
   public partial class CollectionChooser : UserControl
   {
      public CollectionChooser()
      {
         InitializeComponent();

         EnumListBox.Sorted = true;
         EnumListBox.DoubleClick += new EventHandler(AddButton_Click);
         CollectionListBox.DoubleClick += new EventHandler(RemoveButton_Click);


         CollectionListBox.MouseMove += new MouseEventHandler(CollectionListBox_MouseMove);

         this.KeyDown += new KeyEventHandler(CollectionChooser_KeyDown);
         CollectionListBox.KeyDown += new KeyEventHandler(CollectionChooser_KeyDown);
      }




      ICollection<string> mOptions = new List<string>();
      Dictionary<string, string> mOptionToValue = null;
      Dictionary<string, string> mValueToOption = null;
      public event EventHandler OptionsChanged = null;

      bool mAutoSort = false;
      bool mAllowRepeats = true;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool AutoSort
      {
         set
         {
            mAutoSort = value;
            CollectionListBox.Sorted = mAutoSort;

            this.UpButton.Visible = !mAutoSort;
            this.DownButton.Visible = !mAutoSort;
            
         }
         get
         {
            return mAutoSort;
         }
      }

      public bool AllowRepeats
      {
         set
         {
            mAllowRepeats = value;
         }
         get
         {
            return mAllowRepeats;
         }
      }

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
                  if (s != "")
                  {
                     CollectionListBox.Items.Add(s);
                  }
               }
            }
            else
            {
               mBoundSelectionList = value;
               foreach (string s in mBoundSelectionList)
               {
                  if (s != "")
                  {
                     string option;
                     if (mValueToOption.TryGetValue(s, out option))
                     {
                        CollectionListBox.Items.Add(option);
                     }
                  }
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
               if (mAllowRepeats == false)
               {
                  if (mBoundSelectionList.Contains(item.ToString()))
                     continue;
               }

               mBoundSelectionList.Add(item.ToString());
               CollectionListBox.Items.Add(item.ToString());
            }
         }
         else
         {
            foreach (object item in EnumListBox.SelectedItems)
            {
               if (mAllowRepeats == false)
               {
                  if (mBoundSelectionList.Contains(mOptionToValue[item.ToString()]))
                     continue;
               }

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

         List<int> hitlist2 = new List<int>();
         foreach (int index in CollectionListBox.SelectedIndices)
         {
            hitlist2.Add(index);
         }
         hitlist2.Reverse();
         foreach (int index in hitlist2)
         {
            CollectionListBox.Items.RemoveAt(index);
         }
         UpdateData();
      }

      void UpdateData()
      {
         mBoundSelectionList.Clear();

         if (mOptionToValue == null)
         {
            foreach (object item in CollectionListBox.Items)
            {
               mBoundSelectionList.Add(item.ToString());
            }
         }
         else
         {
            foreach (object item in CollectionListBox.Items)
            {
               mBoundSelectionList.Add(mOptionToValue[item.ToString()]);
            }
         }
         if (OptionsChanged != null)
         {
            OptionsChanged.Invoke(this, null);
         }
      }

      private void UpButton_Click(object sender, EventArgs e)
      {
         if (CollectionListBox.SelectedItems.Count == 1)
         {
            object item = CollectionListBox.SelectedItem;
            int index = CollectionListBox.SelectedIndex;
            if (index > 0)
            {
               CollectionListBox.Items.RemoveAt(index);
               CollectionListBox.Items.Insert(index - 1, item);
               CollectionListBox.SelectedIndex = index - 1;

               UpdateData();
            }

         }
      }

      private void DownButton_Click(object sender, EventArgs e)
      {
         if (CollectionListBox.SelectedItems.Count == 1)
         {
            object item = CollectionListBox.SelectedItem;
            int index = CollectionListBox.SelectedIndex;
            if (index < (CollectionListBox.Items.Count - 1))
            {
               CollectionListBox.Items.RemoveAt(index);
               CollectionListBox.Items.Insert(index + 1, item);
               CollectionListBox.SelectedIndex = index + 1;

               UpdateData();
            }

         }
      }

      void CollectionListBox_MouseMove(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Left)
         {
            DoDragDrop(GetCollectionText(), DragDropEffects.Copy);
         }        
      }

      string GetCollectionText()
      {
         string textToDrag = "";
         foreach (string s in mBoundSelectionList)
         {
            textToDrag += s;
            textToDrag += Environment.NewLine;
         }
         return textToDrag;
      }

      void CollectionChooser_KeyDown(object sender, KeyEventArgs e)
      {
         if ((e.KeyData == (Keys.Control | Keys.C)) ||
             (e.KeyData == (Keys.Control | Keys.Insert)))
         {        
            Clipboard.SetDataObject(new DataObject(DataFormats.Text,GetCollectionText()));
         }
         //if( ( e.KeyData == ( Keys.Control | Keys.V ) ) || 
         //    ( e.KeyData == ( Keys.Shift | Keys.Insert ) ) )
         //  buttonPaste_Click( sender, e );    
      }
   }
}
