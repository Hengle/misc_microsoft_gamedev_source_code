using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using System.Windows.Forms;
using System.Drawing;
using EditorCore.Controls.Micro;
using EditorCore.Controls;

namespace EditorCore
{
   public interface INamedTypedProperty
   {
      string GetName();
      string GetTypeName();

      Dictionary<string, object> MetaData
      {
         get;
         set;
      }

      object GetValue();
      void SetValue(object val);
   }

   public interface IPropertyArray
   {
      ICollection<INamedTypedProperty> GetProperties();
      

      
   }



   public interface IEnumProvider
   {
      ICollection<string> GetEnumStrings();
   }


   public class HighLevelProperty
   {
      public HighLevelProperty(INamedTypedProperty prop)
      {
         mProp = prop;
         mLastValue = DataValue;// mProp.GetValue();

         if (mProp.MetaData.ContainsKey("ReadOnly"))
         {
            //mbReadOnly = true;
            bool.TryParse(mProp.MetaData["ReadOnly"].ToString(), out mbReadOnly);
         }
      }

      protected bool mbReadOnly = false;

      private INamedTypedProperty mProp = null;

      public string Name
      {
         get
         {
            return mProp.GetName();
         }
      }
      public object PresentationValue
      {
         get
         {
            return GetPresentationValue();
         }
         set
         {
            SetPresentationValue(value);
         }
      }
      public object DataValue
      {
         get
         {
            return GetDataValue();
         }
         set
         {
            SetDataValue(value);
            Changing();
            mLastValue = value;

         }
      }
      object mLastValue = null;
      public object LastValue
      {
         get
         {
            return mLastValue;
         }

      }

      protected Dictionary<string, object> GetMetadata()
      {
         return mProp.MetaData;
      }

      virtual public object GetPresentationValue()
      {
         return DataValue;
      }
      virtual protected void SetPresentationValue(object val)
      {
         DataValue = val;
      }
      virtual public object GetDataValue()
      {
         return mProp.GetValue();
      }
      private void SetDataValue(object val)
      {
         mProp.SetValue(val);

      }
      virtual public Control GetViewer()
      {
         return null;
      }

      virtual public Control GetEditor(out string bindPropName)
      {
         string text = "";
         if (PresentationValue != null)
            text = PresentationValue.ToString();
         else
            text = "";

         if (mbReadOnly)//mProp.MetaData.ContainsKey("ReadOnly"))
         {
            Label l = new Label();
            l.Text = text;
            //l.ForeColor = Color.Red;
            bindPropName = "Text";
            l.AutoSize = true;
            l.Margin = new Padding(0);
            return l;
         }
         else
         {



            TextBox box = new TextBox();
            box.Text = text;
            mLastValidText = box.Text;  
            bindPropName = "Text";

            if (GetMetadata().ContainsKey("Multiline"))
            {
               box.Multiline = true;
               box.ScrollBars = ScrollBars.Vertical;
               box.Height = box.Height * 3;
               box.Width += 200;
            }


            box.TextChanged += new EventHandler(box_TextChanged);
            //box.BorderStyle = BorderStyle.None;
            //box.Margin = new Padding(0);
            return box;
         }
         
      }
      protected string mLastValidText = "";
      void box_TextChanged(object sender, EventArgs e)
      {
         try
         {
            PresentationValue = ((TextBox)sender).Text;
         }
         catch(System.Exception ex)
         {
            ((TextBox)sender).Text = mLastValidText;
            return;
         }
         if (PresentationValue != null)
         {
            mLastValidText = PresentationValue.ToString();
            ((TextBox)sender).Text = PresentationValue.ToString();
         }
      }
      virtual public void ResetToDefault()
      {

      }

      virtual protected void Changing()
      {
         if (Changed != null)
            Changed.Invoke(this);
      }

      public delegate void HighLevelPropertyEvent(HighLevelProperty sender);

      public event HighLevelPropertyEvent Changed;



   }

   public class NoEditProperty : HighLevelProperty
   {
      public NoEditProperty(INamedTypedProperty prop)
         : base(prop)
      {

      }


      public override Control GetEditor(out string bindPropName)
      {
         bindPropName = "";
         Label l = new Label();
         l.Text = "Not editable";
         return l;
      }

   }


   public class Pair<K, V>
   {
      public K Key;
      public V Value;

      public Pair(K a, V b)
      {
         Key = a;
         Value = b;
      }
      public Pair() { }

      

   }

   public class EnumUtils
   {
      public static Pair<List<int>, List<string>>  EnumToPairList(Type t)
      {
         Pair<List<int>, List<string>> entries = new Pair<List<int>, List<string>>();
         entries.Key = new List<int>();
         entries.Value = new List<string>();
         entries.Value.AddRange(Enum.GetNames(t));
         entries.Key.AddRange( (int[])Enum.GetValues(t));
         return entries;
      }

   }

   public class EnumeratedProperty : HighLevelProperty
   {
      protected List<string> mEnumValues = new List<string>();
      protected List<string> mEnumAlias = new List<string>();

      protected int[] mRealEnumvalues = null;

      protected string mDefaultValue = "";

      protected bool mbIsRealEnum = false;

      public EnumeratedProperty(INamedTypedProperty prop)
         : base(prop)
      {


         //is it a "real" enum if the internal type is set to "Enum"
         if (prop.MetaData.ContainsKey("InternalType") && prop.MetaData["InternalType"].ToString() == "Enum")
         {
            string[] enumNames = Enum.GetNames(prop.GetValue().GetType());
            mEnumAlias.AddRange(enumNames);
            mRealEnumvalues = (int[])Enum.GetValues(prop.GetValue().GetType()); 
            mbIsRealEnum = true;
         }

         if (prop.MetaData.ContainsKey("SimpleEnumeration"))
         {
            string[] entryArray = prop.MetaData["SimpleEnumeration"] as string[];
            ICollection<string> entryList = prop.MetaData["SimpleEnumeration"] as ICollection<string>;
            string commaSeparatedString = prop.MetaData["SimpleEnumeration"] as string;

            if(entryArray != null)
            {
               foreach(string enumval in entryArray)
               {
                  AddEnum(enumval, enumval);
               }
               //normalize the metadata:
               prop.MetaData["SimpleEnumeration"] = new List<string>(entryArray);

            }            
            else if(entryList != null)
            {
               if (entryList != null)
               {                  
                  foreach (string enumval in entryList)
                  {
                     AddEnum(enumval, enumval);
                  }
               }
               

            }
            else if (commaSeparatedString != null)
            {
               string[] e = commaSeparatedString.Split(',');
               foreach (string enumval in e)
               {
                  AddEnum(enumval, enumval);
               }
               //normalize the metadata:
               prop.MetaData["SimpleEnumeration"] = new List<string>(e);
            }

         }
         if (prop.MetaData.ContainsKey("StringIntEnumeration"))
         {
            //string[] entries = prop.MetaData["SimpleEnumeration"] as string[];
            Pair<List<int>, List<string>> entries = prop.MetaData["StringIntEnumeration"] as Pair<List<int>, List<string>>;
            if (entries != null)
            {
               mEnumAlias.AddRange(entries.Value.ToArray());
               mRealEnumvalues = entries.Key.ToArray();
               mbIsRealEnum = true;
            }
         }

         if (prop.MetaData.ContainsKey("StringIntSource"))
         {
            Pair<List<int>, List<string>> entries = prop.MetaData["StringIntSource"] as Pair<List<int>, List<string>>;
            if (entries != null)
            {
               mEnumAlias.AddRange(entries.Value.ToArray());
               foreach (int i in entries.Key)
               {
                  mEnumValues.Add(i.ToString());
               }
            }
         }
         if (prop.MetaData.ContainsKey("EnumSimpleSource"))
         {
            if (prop.MetaData["EnumSimpleSource"].ToString() == "Default")
            {
               IEnumProvider ip = prop as IEnumProvider;
               if(ip != null)
               {
                  ICollection<string> enums = ip.GetEnumStrings();
                  foreach (string enumval in enums)
                  {
                     AddEnum(enumval, enumval);
                  }
               }
            }
         }

      }

      protected object UnboxValue(object boxedValue)
      {
         if (boxedValue == null)
            return null;
         
         if (mbIsRealEnum)
         {
            if(boxedValue is string)
            {
               if(((string)boxedValue) == "")
               {
                  return "";
               }
            }
            int val = (int)boxedValue;
            int index = Array.FindIndex<int>(mRealEnumvalues, delegate(int compareTo) { return val == compareTo; });
            if (index >= 0)
               return mEnumAlias[index];
            else
               return "";
         }
         else
         {
            int index = mEnumValues.IndexOf(boxedValue.ToString());
            if (index >= 0)
               return mEnumAlias[index];
            else
               return "";
         }

      }
      protected object BoxValue(object unboxedValue)
      {
         if (unboxedValue is string)
         {
            int index = mEnumAlias.IndexOf((string)unboxedValue);
            if (mbIsRealEnum && index >= 0)
            {
               return mRealEnumvalues[index];
            }
            else if (index >= 0)
            {
               return mEnumValues[index];
            }
            else
               return "";
         }
         return null;
      }

      public override object GetPresentationValue()
      {
         return UnboxValue(DataValue);
      }
      protected override void SetPresentationValue(object val)
      {
         object v = BoxValue(val);
         if (v != null)
            DataValue = v;
      }

      //public override Control GetEditor(out string bindPropName)
      //{
      //   ComboBox combo = new ComboBox();
      //   combo.Items.AddRange(mEnumAlias.ToArray());
      //   combo.DropDownStyle = ComboBoxStyle.DropDownList;
      //   //combo.Text = (string)PresentationValue;
      //   if (PresentationValue != null)
      //      combo.SelectedIndex = mEnumAlias.IndexOf((string)PresentationValue);
         

      //   combo.SelectedIndexChanged += new EventHandler(combo_SelectedIndexChanged);
      //   bindPropName = "Text";

      //   return combo;
      //}

      //void combo_SelectedIndexChanged(object sender, EventArgs e)
      //{
      //   this.PresentationValue = ((ComboBox)sender).Text;

      //   //throw new Exception("The method or operation is not implemented.");
      //}
      ComboBox mComboBox;
      public override Control GetEditor(out string bindPropName)
      {
         mbOnce = true;
         mComboBox = new ComboBox();
         mComboBox.DropDownHeight = 400;
         mComboBox.Items.AddRange(mEnumAlias.ToArray());
         //mComboBox.DropDownStyle = ComboBoxStyle.DropDown;
         mComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
         //combo.Text = (string)PresentationValue;
         if (PresentationValue != null)
            mComboBox.SelectedIndex = mEnumAlias.IndexOf((string)PresentationValue);


         mComboBox.SelectedIndexChanged += new EventHandler(combo_SelectedIndexChanged);
         mComboBox.KeyPress += new KeyPressEventHandler(combo_KeyPress);
         mComboBox.TextChanged += new EventHandler(combo_TextChanged);
         bindPropName = "Text";

         if (mbReadOnly)
            mComboBox.Enabled = false;

         return mComboBox;
      }
      bool bSuspendInput = false;
      void combo_TextChanged(object sender, EventArgs e)
      {
         if (bSuspendInput == true)
         {

         }
         else
         {
            string searchText = mComboBox.Text;
            int match = mComboBox.FindString(searchText);
 
            if (match == -1 && searchText.Length > 3)
            {
               string bestMatch = "";
               int score = int.MaxValue;
               foreach (object o in mComboBox.Items)
               {
                  string text = o.ToString();
                  int res = text.ToLower().IndexOf(searchText.ToLower());
                  if (res != -1 && res < score)
                  {
                     score = res;
                     bestMatch = text;
                  }
               }
               if (bestMatch != "")
               {
                  searchText = bestMatch.Substring(0, score) + searchText;
                  match = mComboBox.FindString(searchText);
               }               
            }

            if (match != -1)
            {
               mComboBox.SelectedIndex = match;
               mComboBox.SelectionStart = searchText.Length;
               mComboBox.SelectionLength = mComboBox.Text.Length - mComboBox.SelectionStart;
            }

         }
      }
      bool mbOnce = true;
      void combo_KeyPress(object sender, KeyPressEventArgs e)
      {
         if (mbOnce)
         {
            mbOnce = false;
            mComboBox.DropDownStyle = ComboBoxStyle.DropDown;
         }

         if(e.KeyChar == (int)Keys.Escape)
         {
            e.Handled = true;
         }
         else if (Char.IsControl(e.KeyChar))
         {
            bSuspendInput = true;
         }
         else
         {
            bSuspendInput = false;
         }
      }


      void combo_SelectedIndexChanged(object sender, EventArgs e)
      {
         this.PresentationValue = ((ComboBox)sender).Text;

         //throw new Exception("The method or operation is not implemented.");
      }

      protected void AddEnum(string val, string alias)
      {
         mEnumValues.Add(val);
         mEnumAlias.Add(alias);
      }

      protected void AddEnum(string valAndAlias)
      {
         mEnumValues.Add(valAndAlias);
         mEnumAlias.Add(valAndAlias);
      }
   }

   
   public class EnumeratedSetProperty : EnumeratedProperty
   {
      public EnumeratedSetProperty(INamedTypedProperty prop)
         : base(prop)
      {
         if(this.mbIsRealEnum == true)
         {
            throw new System.Exception("Real enums in EnumeratedSetProperty not supported");
         }

         if (prop.MetaData.ContainsKey("NotCommaSeparatedString"))
         {
            mbStringFormat = false;
         }
      }
      public override object GetPresentationValue()
      {

         return DataValue;
      }
      protected override void SetPresentationValue(object val)
      {
         DataValue = val;
      }
      //public override object GetPresentationValue()
      //{
      //   //if (DataValue != null)
      //   //{
      //   //   return StringListHelper.FromString(DataValue.ToString());
      //   //}
      //   //else
      //   //   return null;
      //   if (DataValue != null)
      //   {
      //      if(mbStringFormat)
      //      {
      //         return DataValue.ToString();
      //      }
      //      else
      //      {
      //         return DataValue;
      //      }
      //   }
      //   return null;
      //}
      //public override void SetPresentationValue(object val)
      //{
      //   if (val != null)
      //   {
      //      if (mbStringFormat)
      //      {
      //         DataValue = val.ToString();
      //      }
      //      else
      //      {
      //         DataValue = val;
      //      }
      //   }
      //   //ICollection<string> list = val as ICollection<string>;
      //   //if (list != null)
      //   //{
      //   //   DataValue = StringListHelper.ToString(list);

      //   //}
      //}
      protected Size mSize = new Size(300, 500);
      bool mbStringFormat = true;
      OptionChooser mOptionChooser = null;
      ICollection<string> mSelectedOptions = null;
      public override Control GetEditor(out string bindPropName)
      {
         mOptionChooser = new OptionChooser();

         mOptionChooser.Size = mSize;

         mOptionChooser.SetOptions(this.mEnumAlias, this.mEnumValues);

         //if (DataValue != null)
         //{
         //   if(DataValue is 
         //}
         //if (PresentationValue is string)
         //{
         //   mbStringFormat = true;
         //}
         if (PresentationValue == null)
         {
            mSelectedOptions = new List<string>();
         }
         else if (PresentationValue is ICollection<string>)//mbStringFormat == false)
         {
            mbStringFormat = false;
            mSelectedOptions = (ICollection<string>)PresentationValue;
         }
         else
         {
            mbStringFormat = true;
            if (PresentationValue != "")
            {
               mSelectedOptions = StringListHelper.FromString(PresentationValue.ToString());
            }
            else
            {
               mSelectedOptions = new List<string>();
            }
         }
         mOptionChooser.BoundSelectionList = mSelectedOptions;

         mOptionChooser.OptionsChanged += new EventHandler(mOptionChooser_OptionsChanged);

         bindPropName = "BoundSelectionList";

         return mOptionChooser;

      }

      void mOptionChooser_OptionsChanged(object sender, EventArgs e)
      {
         //PresentationValue = mSelectedOptions;

         if (mbStringFormat == true)
         {
            PresentationValue = StringListHelper.ToString(mSelectedOptions);
         }
         else
         {
            PresentationValue = mSelectedOptions;
         }
         

      }



   }


   public class EnumeratedCollectionProperty : EnumeratedProperty
   {
      public EnumeratedCollectionProperty(INamedTypedProperty prop)
         : base(prop)
      {
         if (this.mbIsRealEnum == true)
         {
            throw new System.Exception("Real enums in EnumeratedSetProperty not supported");
         }
         if (prop.MetaData.ContainsKey("Concise"))
         {
            if (bool.TryParse(prop.MetaData["Concise"].ToString(), out mbConciseFormat) == false)
               mbConciseFormat = false;
         }

         if (prop.MetaData.ContainsKey("AllowRepeats"))
         {
            if(bool.TryParse(prop.MetaData["AllowRepeats"].ToString(), out mbAllowRepeats) == false)
               mbAllowRepeats = true;
         }
         if (prop.MetaData.ContainsKey("AutoSort"))
         {
            if(bool.TryParse(prop.MetaData["AutoSort"].ToString(), out mbAutoSort) == false)
               mbAutoSort = false;
         }
      }

      //defaults match child control... could match these to more general data properties.
      bool mbAllowRepeats = true;
      bool mbAutoSort = false;

      public override object GetPresentationValue()
      {
         //if (DataValue != null)
         //{
         //   return StringListHelper.FromString(DataValue.ToString());
         //}
         //else
         //   return null;
         if (DataValue != null)
         {
            return DataValue.ToString();
         }
         return null;
      }
      protected override void SetPresentationValue(object val)
      {
         if (val != null)
         {

            DataValue = val.ToString();
         }
         //ICollection<string> list = val as ICollection<string>;
         //if (list != null)
         //{
         //   DataValue = StringListHelper.ToString(list);

         //}
      }
      bool mbConciseFormat = false;
      protected Size mSize = new Size(300, 500);
      CollectionChooser mOptionChooser = null;
      ICollection<string> mSelectedOptions = null;
      public override Control GetEditor(out string bindPropName)
      {
         mOptionChooser = new CollectionChooser();

         mOptionChooser.AllowRepeats = mbAllowRepeats;
         mOptionChooser.AutoSort = mbAutoSort;

         mOptionChooser.Size = mSize;

         mOptionChooser.SetOptions(this.mEnumAlias, this.mEnumValues);

         if (PresentationValue == null)
         {
            mSelectedOptions = new List<string>();
         }
         else
         {
            mSelectedOptions = StringListHelper.FromString(PresentationValue.ToString());
         }
         mOptionChooser.BoundSelectionList = mSelectedOptions;

         mOptionChooser.OptionsChanged += new EventHandler(mOptionChooser_OptionsChanged);

         bindPropName = "BoundSelectionList";

         return mOptionChooser;

      }

      void mOptionChooser_OptionsChanged(object sender, EventArgs e)
      {
         //PresentationValue = mSelectedOptions;


         PresentationValue = StringListHelper.ToString(mSelectedOptions);



      }



   }


   public class ResourceProperty : HighLevelProperty
   {
      public ResourceProperty(INamedTypedProperty prop)
         : base(prop)
      {
      }

      protected string mStartingDir = "";
      protected bool mbAllowSubDir = true;

      //mask / extension

      //protected bool mbAllow

   }

   public class FileNameProperty : HighLevelProperty
   {

      public FileNameProperty(INamedTypedProperty prop)
         : base(prop)
      {
         if (prop.MetaData.ContainsKey("RootDirectory"))
         {
            string value = prop.MetaData["RootDirectory"].ToString();
            if (value.Contains("$") == true)
            {
               try
               {
                  value = value.Replace("$", "");
                  Type t = CoreGlobals.getWorkPaths().GetType();
                  mRootDir = t.GetField(value).GetValue(CoreGlobals.getWorkPaths()).ToString();
               }
               catch(System.Exception ex)
               {
                  CoreGlobals.getErrorManager().SendToErrorWarningViewer(ex.ToString());
               }
            }
            else
            {
               mRootDir = value.ToLower();
            }
         }

         if (prop.MetaData.ContainsKey("StartingDirectory"))
         {
            //mStartingDir = prop.MetaData["StartingDirectory"].ToString().ToLower();
            string value = prop.MetaData["StartingDirectory"].ToString();
            if (value.Contains("$") == true)
            {
               try
               {
                  value = value.Replace("$", "");
                  Type t = CoreGlobals.getWorkPaths().GetType();
                  mStartingDir = t.GetField(value).GetValue(CoreGlobals.getWorkPaths()).ToString();
               }
               catch(System.Exception ex)
               {
                  CoreGlobals.getErrorManager().SendToErrorWarningViewer(ex.ToString());
               }
            }
            else
            {
               mStartingDir = value.ToLower();
            }

         }
         if (prop.MetaData.ContainsKey("FileFilter"))
         {
            mFileFilter = prop.MetaData["FileFilter"].ToString();
         }
         if (prop.MetaData.ContainsKey("NoExtension"))
         {
            string value = prop.MetaData["NoExtension"].ToString();
            bool.TryParse(value, out mbNoExtension);                       
         }
         if (prop.MetaData.ContainsKey("Clearable"))
         {
            string value = prop.MetaData["Clearable"].ToString();
            bool.TryParse(value, out mbClearable);
         }
         if (prop.MetaData.ContainsKey("FilePrefix"))
         {
            mFilePrefix = prop.MetaData["FilePrefix"].ToString();
         }

      }
      public bool mbClearable = true;
      public bool mbNoExtension = false;
      protected string mRootDir = "";
      protected string mStartingDir = "";
      protected string mFileFilter = "File (*.*)|*.*";
      protected string mFilePrefix = "";
      
      //protected string mFilter
      
      protected bool mbAllowSubDir = false;
      //protected bool 
      Button mButton = null;
      public override Control GetEditor(out string bindPropName)
      {
         string text = "";
         if (PresentationValue != null)
            text = PresentationValue.ToString();
         else
            text = "";
         mLastValidText = text;

         if (mbReadOnly)//;GetMetadata().ContainsKey("ReadOnly"))
         {
            return base.GetEditor(out bindPropName);
         }
         //else if (mbAllowSubDir == false)
         //{
         //}
         //else if(mbAllowSubDir == true)
         //{
         //}

         bindPropName = "";

         
         Button b = new Button();
         b.AutoSize = true;
         b.Text = text;
         b.Click += new EventHandler(b_Click);

         if (mbClearable == true)
         {
            b.MouseUp += new MouseEventHandler(b_MouseUp);

         }
         mButton = b;

         return b;
      }

      void b_MouseUp(object sender, MouseEventArgs e)
      {
         ContextMenu m = new ContextMenu();
         MenuItem clear = new MenuItem("Clear value");
         clear.Click += new EventHandler(clear_Click);
         m.MenuItems.Add(clear);
         m.Show(((Control)sender).FindForm(), Cursor.Position);
      }

      void clear_Click(object sender, EventArgs e)
      {
         SafeUpdateUI(mButton, "");

      }
      
      void b_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.Filter = mFileFilter;
         d.InitialDirectory = mStartingDir;
         if (d.ShowDialog() == DialogResult.OK)
         {
            SafeUpdateUI((Button)sender, d.FileName);
         }
      }

      private void SafeUpdateUI(Button control, string value)
      {
         try
         {
            PresentationValue = value;
            control.Text = PresentationValue.ToString();
         }
         catch (System.Exception ex)
         {
            control.Text = mLastValidText;
            return;
         }
         mLastValidText = PresentationValue.ToString();

      }


      protected override void SetPresentationValue(object val)
      {
         string manipulatedValue;

         if (mbClearable == true && val.ToString() == "")
         {
            DataValue = "";
         }

         if (ValidateValue(val.ToString(), out manipulatedValue) == true)
         {
            DataValue = manipulatedValue;
         }

      }

      public bool ValidateValue(string text, out string manipulatedValue)
      {
         manipulatedValue = text;

         text = text.ToLower();
         mRootDir = mRootDir.ToLower();

         if (mRootDir != "")
         {
            if (text.IndexOf(mRootDir) == -1)
               return false;

            manipulatedValue = text.Replace(mRootDir, "");

            //kill leading \\
            manipulatedValue = manipulatedValue.TrimStart(new char[] { '\\' });
         }
         if (mbNoExtension == true)
         {
            int ext = manipulatedValue.LastIndexOf(".");
            if(ext != -1)
            {
               manipulatedValue = manipulatedValue.Substring(0, ext);
            }

         }

         manipulatedValue = mFilePrefix + manipulatedValue;

         return true;
         //doubleVal = 1;
         //if (text.EndsWith("."))
         //{
         //   return false;
         //}
         //if (double.TryParse(text, out doubleVal) == false)
         //{
         //   return false;
         //}
         //if (doubleVal > mMax)
         //   return false; //doubleVal = mMax;
         //if (doubleVal < mMin)
         //   return false; //doubleVal = mMin;
         //if (mbInteger)
         //   doubleVal = (double)((int)(doubleVal));
         //if (mbOnlyPositive && doubleVal < 0)
         //   return false; //doubleVal = Math.Abs(doubleVal);
         //if (mbNonZero && doubleVal == 0)
         //{
         //   return false;
         //}
         //return true;
      }

   }

   //public class ReadOnlyProperty : HighLevelProperty
   //{
   //   public ReadOnlyProperty(INamedTypedProperty prop)
   //      : base(prop)
   //   {
   //   }

   //   public override Control GetEditor(out string bindPropName)
   //   {
   //      //ComboBox combo = new ComboBox();
   //      //combo.Items.AddRange(mEnumAlias.ToArray());
   //      //combo.DropDownStyle = ComboBoxStyle.DropDownList;
   //      ////combo.Text = (string)PresentationValue;
   //      //combo.SelectedIndexChanged += new EventHandler(combo_SelectedIndexChanged);
   //      //bindPropName = "Text";
   //      //return combo;
   //      //   combo.SelectedIndex = mEnumAlias.IndexOf((string)PresentationValue);


   //      string text = ".";
   //      if (PresentationValue != null)
   //      {
   //         text = PresentationValue.ToString();
   //      }



   //      Label l = new Label();
   //      l.Text = text;
   //      //l.ForeColor = Color.Red;
   //      l.AutoSize = true;
   //      l.Margin = new Padding(0);

   //   }

   //}



   public class StringListProperty : HighLevelProperty
   {
      public StringListProperty(INamedTypedProperty prop)
         : base(prop)
      {
         
      
      }


      //OpenStringList mOpenStringList =
      public override Control GetEditor(out string bindPropName)
      {
         OpenStringList openStringList = new OpenStringList();
         bindPropName = "Strings";
         openStringList.ListChanged += new EventHandler(openStringList_ListChanged);
         if (PresentationValue == null)
         {

         }
         else if (PresentationValue is ICollection<string>)
         {
            openStringList.Strings = PresentationValue as ICollection<string>;
            //mStringPicker.LocStringID = (int)PresentationValue;
         }

         return openStringList;
      }

      void openStringList_ListChanged(object sender, EventArgs e)
      {
         OpenStringList openStringList = sender as OpenStringList;
         if (openStringList != null)
         {
            PresentationValue = openStringList.Strings;

         }

      }


      //public override object GetPresentationValue()
      //{
      //   return DataValue;
      //}
      //protected override void SetPresentationValue(object val)
      //{
      //   //double doubleVal;
      //   //if (ValidateValue(val.ToString(), out doubleVal) == true)
      //   //{
      //   //   DataValue = doubleVal.ToString();
      //   //}
      //}

      //void mStringPicker_IDChanged(object sender, LocString locString)
      //{
      //   PresentationValue = locString.mLocID.ToString();
      //}

   }

   public class NumericProperty : HighLevelProperty
   {
      //min,max, floating?

      protected bool mbOnlyPositive = false;
      protected bool mbNonZero = false;
      protected bool mbInteger = false;

      protected double mMin = double.MinValue;
      protected double mMax = double.MaxValue;
      protected bool mbLogrithmic = false;
      protected bool mbHasSettings = false;

     
      public NumericProperty(INamedTypedProperty prop)
         : base(prop)
      {
         mbHasSettings = CheckMetaDataSettings(prop);
      }

      bool CheckMetaDataSettings(INamedTypedProperty prop)
      {
         if (prop.MetaData.ContainsKey("Default"))
         {
            double defaultValue;
            if (double.TryParse(prop.MetaData["Default"].ToString(), out defaultValue))
            {
               if (PresentationValue == null || PresentationValue.ToString() == "")
               {
                  SetPresentationValue(defaultValue);
               }
            }
         }


         if (prop.MetaData.ContainsKey("Logrithmic"))
         {
            bool value = false;
            bool.TryParse(prop.MetaData["Logrithmic"].ToString(), out value);
            mbLogrithmic = value;
         }
         string tname = prop.GetTypeName();
         if (prop.MetaData.ContainsKey("Integer") || prop.GetTypeName() == "Int32" || prop.GetTypeName() == "Int16")
         {
            mbInteger = true;
         }

         if (prop.MetaData.ContainsKey("Max") && prop.MetaData.ContainsKey("Min")
            && double.TryParse(prop.MetaData["Max"].ToString(), out mMax)
            && double.TryParse(prop.MetaData["Min"].ToString(), out mMin))
         {
            return true;
         }
         return false;
      }

      //cParmTypeLong,
      //cParmTypeFloat,	 
      //protected double mValue;

      public override object GetPresentationValue()
      {
         return DataValue;
      }
      //may want to add the option to throw errors later
      protected override void SetPresentationValue(object val)
      {
         double doubleVal;

         if(ValidateValue(val.ToString(), out doubleVal) == true)
         {
            DataValue = doubleVal.ToString();

         }


      }

      public bool ValidateValue(string text, out double doubleVal)
      {
         doubleVal = 1;
         if (text.EndsWith("."))
         {
            return false;
         }
         if (double.TryParse(text, out doubleVal) == false)
         {
            return false;
         }

         if (doubleVal > mMax)
            return false; //doubleVal = mMax;
         if (doubleVal < mMin)
            return false; //doubleVal = mMin;
         if (mbInteger)
            doubleVal = (double)((int)(doubleVal));
         if (mbOnlyPositive &&  doubleVal < 0)
            return false; //doubleVal = Math.Abs(doubleVal);
         if (mbNonZero && doubleVal == 0)
         {
            return false;
         }

         return true;
      }

 

      public override Control GetEditor(out string bindPropName)
      {
         if (mbReadOnly)//GetMetadata().ContainsKey("ReadOnly"))
         {
            string text = "";
            if (PresentationValue != null)
               text = PresentationValue.ToString();
            else
               text = ""; 
            Label l = new Label();
            l.Text = text;
            //l.ForeColor = Color.Red;
            bindPropName = "Text";
            l.AutoSize = true;
            l.Margin = new Padding(0);
            return l;
         }
         else if (mbHasSettings == false)
         {

            TextBox box = new TextBox();
            if (PresentationValue != null)
               box.Text = PresentationValue.ToString();
            else
               box.Text = "";

            bindPropName = "Text";
            box.TextChanged += new EventHandler(box_TextChanged);
 
            return box;

        
         }
         else
         {
            NumericSliderControl slider = new NumericSliderControl();
            slider.Logrithmic = mbLogrithmic;
            slider.Setup(mMin, mMax, !mbInteger);

            double doubleVal = 0;

            StringDecorator dec;
            if(PresentationValue == null || PresentationValue.ToString() == "") 
            {
               slider.NumericValue = mMin;
               slider.StartGray = true;
            }
            else if ( double.TryParse(PresentationValue.ToString(), out doubleVal) == true)
            {
               slider.NumericValue = doubleVal;
            }
            else if(StringDecorator.TryParse(PresentationValue.ToString(), out dec) )
            {
               if (double.TryParse(dec.mDecorator, out doubleVal) == true)
               {
                  slider.NumericValue = LoadValue(doubleVal);
               }            
            }
            else
            {
               return base.GetEditor(out bindPropName);
            }
   
            //slider.NumericValue = doubleVal;
            slider.ValueChanged += new EventHandler(slider_ValueChanged);
            bindPropName = "NumericValue";
            return slider;
         }
      }
      void box_TextChanged(object sender, EventArgs e)
      {
         double res = 0;

         string text = ((TextBox)sender).Text;

         if(ValidateValue(text, out res) == false)
         {
            ((TextBox)sender).ForeColor = Color.Red;
            return;
         }
         ((TextBox)sender).ForeColor = Color.Black;
         try
         {
            PresentationValue = ((TextBox)sender).Text;
         }
         catch (System.Exception ex)
         {
         }

         ((TextBox)sender).Text = PresentationValue.ToString();
      }
      void slider_ValueChanged(object sender, EventArgs e)
      {
         double slidervalue = ((NumericSliderControl)sender).NumericValue;
         double res = 0;

         if (ValidateValue(slidervalue.ToString(), out res) == false)
         {
            ((NumericSliderControl)sender).SetInvalid(true);
            return;
         }
         //PresentationValue = slidervalue.ToString();
         //PresentationValue = StoreValue(res).ToString();
         double finalValue = StoreValue(res);
         SetPresentationValue(finalValue);
      }

      protected virtual double LoadValue(double input )
      {
         return input;
      }
      protected virtual double StoreValue(double input)
      {
         return input;
      }
      protected virtual void SetPresentationValue(double value)
      {
         PresentationValue = value.ToString();
      }
   }




   public class BoolProperty : HighLevelProperty
   {
  
      public BoolProperty(INamedTypedProperty prop)
         : base(prop)
      {
      }

      protected bool mbShowTextValue = false;

      //Label mValueTextLabel = null;
      public override Control GetEditor(out string bindPropName)
      {         
         CheckBox checkBox = new CheckBox();
         checkBox.Text = "";

         checkBox.Width = 35;

         bool result = false;
         if (PresentationValue == null)
         {
            //checkBox.Enabled = false;
            checkBox.CheckState = CheckState.Indeterminate;

         }
         else if (this.PresentationValue is bool)
         {
            checkBox.Checked = (bool)PresentationValue;
         }
         else if (bool.TryParse(PresentationValue.ToString(), out result))
         {
            checkBox.Checked = result;
         }
         else if (PresentationValue.ToString() == "")
         {
            checkBox.CheckState = CheckState.Indeterminate;
         }

         bindPropName = "Checked";
         checkBox.CheckedChanged += new EventHandler(checkBox_CheckedChanged);

         
         //if (mbShowTextValue)
         //{
         //   mValueTextLabel = new Label();
         //   checkBox.Height = 24; 
         //   Control c = DynamicUIHelpers.MakePair(checkBox, mValueTextLabel);
            
         //   mValueTextLabel.Text = PresentationValue.ToString();
         //   return c;
         //}

         return checkBox;
      }

      void checkBox_CheckedChanged(object sender, EventArgs e)
      {

         PresentationValue = ((CheckBox)sender).Checked;

         if(PresentationValue is string)
         {
            bool res;
            if(bool.TryParse(PresentationValue.ToString(), out res))
            {
               ((CheckBox)sender).Checked = res;
            }
         }
         else
         {
            ((CheckBox)sender).Checked = (bool)PresentationValue;
         }

         if(mbShowTextValue)
         {
            ((CheckBox)sender).Text = PresentationValue.ToString();
         }
         
      }
   }

   public class LocStringIDProperty : HighLevelProperty
   {

      public LocStringIDProperty(INamedTypedProperty prop)
         : base(prop)
      {
         if (GetMetadata().ContainsKey("Compact"))
         {
            bool compact;
            if (bool.TryParse(GetMetadata()["Compact"].ToString(), out compact))
            {
               mbCompact = compact;
            }
         }

      }
      bool mbCompact = false;
      ILocStringPicker mStringPicker = null;
      protected Nullable<int> mPickerHeight = null;

      public override Control GetEditor(out string bindPropName)
      {
   
         if (mbCompact == false)
         {
            mStringPicker = new LocStringPicker();
            if (mPickerHeight != null)
            {
               ((Control)mStringPicker).Height = mPickerHeight.Value;
            }
         }
         else if (mbCompact == true)
         {
            mStringPicker = new LocStringPickerCompact();
         }
         int id;
         if (PresentationValue == null)
         {

         }
         else if (PresentationValue is int)
         {
            mStringPicker.LocStringID = (int)PresentationValue;
         }
         //else if (PresentationValue is long)
         //{
         //   mStringPicker.LocStringID = (int)(PresentationValue);
         //}
         else if (PresentationValue is string)
         {
            if (int.TryParse(PresentationValue.ToString(), out id))
            {
               mStringPicker.LocStringID = id;
            }
         }

         bindPropName = "LocStringID";

         mStringPicker.IDChanged += new LocStringPicker.LocEvent(mStringPicker_IDChanged);
         return mStringPicker as Control ;
      }

      void mStringPicker_IDChanged(object sender, LocString locString)
      {
         PresentationValue = locString.mLocID.ToString();
      }

   }

   public class ComplexTypeProperty : HighLevelProperty
   {
      public ComplexTypeProperty(INamedTypedProperty prop)
         : base(prop)
      {
         mPropertyList = new PropertyList();

         //IPropertyArray subProps = prop.GetValue() as IPropertyArray;
         //if (subProps != null)
         //{
         //   foreach (INamedTypedProperty subProp in subProps.GetProperties())
         //   {
         //      mPropertyList.ApplyMetaDataToType(subProp.GetTypeName(), subProp.GetTypeName(), subProp.MetaData);
         //   }
         //}
      }
      ObjectEditorControl mPropertyList = null;

      public override Control GetEditor(out string bindPropName)
      {
         
         mPropertyList.SelectedObject = this.PresentationValue;

         bindPropName = "SelectedObject";

         return mPropertyList;
      }
   }

   public class ArrayTypeProperty : HighLevelProperty
   {
      public ArrayTypeProperty(INamedTypedProperty prop)
         : base(prop)
      {
         mPropertyList = new DynamicContainerList();

         //IPropertyArray subProps = prop.GetValue() as IPropertyArray;
         //if (subProps != null)
         //{
         //   foreach (INamedTypedProperty subProp in subProps.GetProperties())
         //   {
         //      mPropertyList.ApplyMetaDataToType(subProp.GetTypeName(), subProp.GetTypeName(), subProp.MetaData);
         //   }
         //}

         //mPropertyList.SetTypeEditor(prop.GetTypeName, prop.GetName(), typeof(LamdaObjectProperty));
         object objThis;
         if(prop.MetaData.TryGetValue("this", out objThis))
         {
            if (objThis is ICollectionObserver)
            {
               mCollectionObserver = (ICollectionObserver)objThis;
            }

         }


      }
      ICollectionObserver mCollectionObserver = null;
      DynamicContainerList mPropertyList = null;

      public override Control GetEditor(out string bindPropName)
      {
         mPropertyList.ObjectList = (IList)this.PresentationValue;

         mPropertyList.ObjectDeleted += new DynamicContainerList.ObjectChanged(mPropertyList_ObjectDeleted);
         mPropertyList.NewObjectAdded += new DynamicContainerList.ObjectChanged(mPropertyList_NewObjectAdded);
         //mPropertyList.SetTypeEditor(
            //= this.PresentationValue;

         bindPropName = "SelectedObject";

         return mPropertyList;
      }

      void mPropertyList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {

         this.DataValue = mPropertyList.ObjectList;
         if (mCollectionObserver != null)
         {
            mCollectionObserver.Added(selectedObject);
         }
         //throw new Exception("The method or operation is not implemented.");
      }

      void mPropertyList_ObjectDeleted(ObjectEditorControl sender, object selectedObject)
      {

         this.DataValue = mPropertyList.ObjectList;
         if (mCollectionObserver != null)
         {
            mCollectionObserver.Removed(selectedObject);
         }

         //throw new Exception("The method or operation is not implemented.");
      }
   }


   public class ColorProperty : HighLevelProperty
   {
      public ColorProperty(INamedTypedProperty prop)
         : base(prop)
      {
      }

      protected bool mbUseTextFormat = false;

      public override Control GetEditor(out string bindPropName)
      {

         ColorPickerLauncher colorpickerLauncher = new ColorPickerLauncher();

         colorpickerLauncher.ColorPropertyName = this.Name;
         Color selectedColor = Color.Wheat;

         if( PresentationValue is Color )
         {
            selectedColor = (Color)PresentationValue;
         }
         else if( ( PresentationValue != null ) && ( mbUseTextFormat || ( PresentationValue is string ) ) )
         {
            string blah = PresentationValue.ToString();
            selectedColor = TextColorHelper.FromString( PresentationValue.ToString() );
         }
         colorpickerLauncher.SelectedColor = selectedColor;

         colorpickerLauncher.SelectedColorChanged += new EventHandler(colorpickerLauncher_SelectedColorChanged);

         bindPropName = "SelectedColor";

         return colorpickerLauncher;
      }

      void colorpickerLauncher_SelectedColorChanged(object sender, EventArgs e)
      {
         if( mbUseTextFormat || ( PresentationValue is string ) )
         {
            PresentationValue = TextColorHelper.ToString( ( (ColorPickerLauncher)sender ).SelectedColor );
         }
         else
         {
            PresentationValue = ( (ColorPickerLauncher)sender ).SelectedColor;
         }         
      }

   
   }

   public class FunctionProperty : HighLevelProperty
   {
      string mTextValue = "Button0";

      public FunctionProperty(INamedTypedProperty prop)
         : base(prop)
      {
         object objThis;
         if (prop.MetaData.TryGetValue("Text", out objThis))
         {
            if (objThis is string)
            {
               mTextValue = (string)objThis;
            }
         }
      }

      protected bool mbShowTextValue = false;

      //Label mValueTextLabel = null;
      public override Control GetEditor(out string bindPropName)
      {
         Button butt = new Button();
         butt.Text = mTextValue;

         butt.Width = 35;

         bindPropName = "Text";

         butt.Click += new EventHandler(butt_Click);
         return butt;
      }

      void butt_Click(object sender, EventArgs e)
      {
         PresentationValue = true;
      }


   }

}
