using System;
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Reflection;
using System.Xml;
using System.Xml.XPath;

namespace EditorCore
{

   public partial class BetterPropertyGrid : ObjectEditorControl
   {
      public BetterPropertyGrid()
      {
         InitializeComponent();
      }

      override protected void ClearUI()
      {
         mTableControlCount = 0;

         this.SuspendLayout();
         MainTableLayoutPanel.Visible = false;
         MainTableLayoutPanel.SuspendLayout();
         MainTableLayoutPanel.Controls.Clear();
         MainTableLayoutPanel.RowStyles.Clear();
         //mControls.Clear();
         //      foreach (Control c in temp)
         //      {
         //         AddRow(c.Name, c);
         //      }
         MainTableLayoutPanel.ResumeLayout();
         MainTableLayoutPanel.Visible = true;
         this.ResumeLayout();
      }
      override protected void AddControls(List<Control> controls)
      {
         mTableControlCount = 0;
         this.SuspendLayout();
         MainTableLayoutPanel.Visible = false;
         MainTableLayoutPanel.SuspendLayout();
         MainTableLayoutPanel.Controls.Clear();
         MainTableLayoutPanel.RowStyles.Clear();

         foreach (Control c in controls)
         {
            AddRow(c.Name, c);
         }
         if (mbLastRowHack)
         {
            AddBlankRow();
         }

         MainTableLayoutPanel.ResumeLayout();
         MainTableLayoutPanel.Visible = true;
         this.ResumeLayout();
      }
      bool mbLastRowHack = false;
      public bool LastRowHack
      {
         set
         {
            mbLastRowHack = value;
         }
         get
         {
            return mbLastRowHack;
         }

      }
      void AddBlankRow()
      {
         AddRow("", new Label());//for better formatting of the crappy tables

      }

      int mTableControlCount = 0;
      private void AddRow(string name, Control c)
      {
         MainTableLayoutPanel.RowStyles.Add(new RowStyle(SizeType.AutoSize));
         Label l = new Label();
         l.Text = name;
         l.AutoSize = true;
         l.Name = name + "Label";

         l.Margin = new Padding(0);
         c.Margin = new Padding(0);

         c.Dock = DockStyle.Fill;

         //mControls.Add(c);
         //MainTableLayoutPanel.Controls.Add(l, 0, mControls.Count);
         //MainTableLayoutPanel.Controls.Add(c, 1, mControls.Count);
         mTableControlCount++;
         MainTableLayoutPanel.Controls.Add(l, 0, mTableControlCount);
         MainTableLayoutPanel.Controls.Add(c, 1, mTableControlCount);
      }

   }



   //public partial class BetterPropertyGrid : UserControl
   //{
   //   public BetterPropertyGrid()
   //   {
   //      InitializeComponent();

   //      InitBasicTypes();

   //      //this.DoubleBuffered = true;
   //   }

   //   private object mSelectedObject = null;

   //   [Browsable(false)]
   //   [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
   //   public object SelectedObject
   //   {
   //      set
   //      {
   //         mSelectedObject = value;
   //         LoadSettings(mSelectedObject);
   //      }
   //      get
   //      {
   //         return mSelectedObject;
   //      }

   //   }
   //   object[] mSelectedObjects = null;

   //   [Browsable(false)]
   //   [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
   //   public object[] SelectedObjects
   //   {
   //      set
   //      {
   //         mSelectedObjects = value;
   //         LoadSettings(mSelectedObjects);
   //      }
   //      get
   //      {
   //         return mSelectedObjects;
   //      }

   //   }


   //   //This loads metadata from a file
   //   public void LoadSettingsFromStream(Stream s)
   //   {
   //      XmlDocument d = new XmlDocument();
   //      d.Load(s);
   //      s.Close();

   //      XmlNodeList properties = d.SelectNodes("//Property");
   //      string className = d.FirstChild.Attributes["Name"].InnerXml;
   //      foreach (XmlNode n in properties)
   //      {
   //         string propertyName = (string)n.Attributes["Name"].InnerXml;

   //         XmlNodeList settings = n.ChildNodes;
   //         foreach (XmlNode setting in settings)
   //         {
   //            string settingName = (string)setting.Attributes["Name"].InnerXml;
   //            this.AddMetaDataForProp(className, propertyName, settingName, setting.InnerText);
   //         }
   //      }
   //   }

   //   List<HighLevelProperty> mProps = new List<HighLevelProperty>();
   //   private void LoadSettings(object obj)
   //   {
   //      List<ReflectedPropertyBinder> mBinders = new List<ReflectedPropertyBinder>();
   //      if(obj == null)
   //      {
   //         this.Visible = false;
   //         this.SuspendLayout();
            
           
   //         MainTableLayoutPanel.SuspendLayout();
   //         foreach (Control c in MainTableLayoutPanel.Controls)
   //         {
   //            c.SuspendLayout();
   //         }
   //         mControls.Clear();
   //         MainTableLayoutPanel.Controls.Clear();
   //         MainTableLayoutPanel.RowStyles.Clear();
   //         MainTableLayoutPanel.ResumeLayout();
   //         this.ResumeLayout();
   //         return;
   //      }
   //      this.Visible = true;


   //      List<Control> temp = new List<Control>(); 

   //      Type t = obj.GetType();
   //      if (mTypeDescriptions.ContainsKey(t) == false)
   //      {
   //         mTypeDescriptions[t] = new TypeDescription(t);
   //      }

   //      mBinders = new List<ReflectedPropertyBinder>();
   //      foreach (PropertyInfo p in mTypeDescriptions[t].mPropertyInfoList)
   //      {
   //         ReflectedPropertyBinder b = new ReflectedPropertyBinder(obj, p);
   //         mBinders.Add(b);
   //      }
   //      mProps = new List<HighLevelProperty>();
   //      foreach (ReflectedPropertyBinder b in mBinders)
   //      {
   //         ApplyMetadata(b,t.Name);
   //         HighLevelProperty p = GetHighLevelProperty(b);//,t.Name);
   //         mProps.Add(p);
   //         string bindName;
   //         Control c = p.GetEditor(out bindName);

   //         c.Name = b.GetName();
   //         //AddRow(b.GetName(), c);

   //         temp.Add(c);

   //      }

   //      //saving the ui changes for the end.  why is this so slow?
   //      this.SuspendLayout();
   //      MainTableLayoutPanel.Visible = false;
   //      MainTableLayoutPanel.SuspendLayout();
   //      MainTableLayoutPanel.Controls.Clear();
   //      MainTableLayoutPanel.RowStyles.Clear();
   //      mControls.Clear();
   //      foreach (Control c in temp)
   //      {
   //         AddRow(c.Name, c);
   //      }
   //      MainTableLayoutPanel.ResumeLayout();
   //      MainTableLayoutPanel.Visible = true;
   //      this.ResumeLayout();

   //   }
   //   private void LoadSettings(object[] objects)
   //   {
   //      if (objects == null || objects.Length == 0)
   //      {
   //         this.Visible = false;
   //         this.SuspendLayout();
   //         MainTableLayoutPanel.SuspendLayout();
   //         MainTableLayoutPanel.Controls.Clear();
   //         MainTableLayoutPanel.RowStyles.Clear();
   //         mControls.Clear();
   //         MainTableLayoutPanel.ResumeLayout();
   //         this.ResumeLayout();
 

   //         //MainTableLayoutPanel.Controls.Clear();
   //         //MainTableLayoutPanel.RowStyles.Clear();
   //         //mControls.Clear();
   //         return;
   //      }
   //      if(objects.Length == 1)
   //      {
   //         LoadSettings(objects[0]);
   //         return;
   //      }         
         
   //      this.Visible = true;


   //      List<Control> temp = new List<Control>();
       
   //      Dictionary<Type, TypeDescription> representedTypes = new Dictionary<Type, TypeDescription>();

   //      foreach (object obj in objects)
   //      {
   //         Type t = obj.GetType();
   //         if (mTypeDescriptions.ContainsKey(t) == false)
   //         {
   //            mTypeDescriptions[t] = new TypeDescription(t);
   //         }
   //         representedTypes[t] = mTypeDescriptions[t];
   //      }

   //      Dictionary<KeyValuePair<string, Type>, int> mergedPropertyList = new Dictionary<KeyValuePair<string, Type>, int>();         
   //      Dictionary<Type, TypeDescription>.Enumerator it = representedTypes.GetEnumerator();
   //      while(it.MoveNext())
   //      {
            
   //         foreach (PropertyInfo p in it.Current.Value.mPropertyInfoList)
   //         {
   //            KeyValuePair<string, Type> info = new KeyValuePair<string, Type>(p.Name, p.PropertyType);
   //            if(!mergedPropertyList.ContainsKey(info))
   //            {
   //               mergedPropertyList[info] = 0;
   //            }
   //            mergedPropertyList[info]++;
   //         }
   //         //it.Current.
   //      }

   //      List<ReflectedPropertyMultiBinder>  mMultiBinders = new List<ReflectedPropertyMultiBinder>();
   //      Dictionary<KeyValuePair<string, Type>, int>.Enumerator itProps = mergedPropertyList.GetEnumerator();
   //      while(itProps.MoveNext())
   //      {
   //         if(itProps.Current.Value == representedTypes.Count)
   //         {
   //            //mPropsByName
   //            ReflectedPropertyMultiBinder b = new ReflectedPropertyMultiBinder(objects, itProps.Current.Key.Key, itProps.Current.Key.Value, representedTypes);
   //            mMultiBinders.Add(b);
   //         }
   //      }

   //      mProps = new List<HighLevelProperty>();
   //      foreach (ReflectedPropertyMultiBinder b in mMultiBinders)
   //      {

   //         Dictionary<Type, TypeDescription>.Enumerator repTypes = representedTypes.GetEnumerator();
   //         while (repTypes.MoveNext())
   //         {
   //            ApplyMetadata(b, repTypes.Current.Key.Name);
   //         }


   //         HighLevelProperty p = GetHighLevelProperty(b);//, t.Name);
   //         mProps.Add(p);
   //         string bindName;
   //         Control c = p.GetEditor(out bindName);
   //         c.Name = b.GetName();
   //         temp.Add(c);
   //      }

   //      //mBinders = new List<ReflectedPropertyBinder>();
   //      //foreach (PropertyInfo p in mTypeDescriptions[t].mPropertyInfoList)
   //      //{
   //      //   ReflectedPropertyBinder b = new ReflectedPropertyBinder(obj, p);
   //      //   mBinders.Add(b);
   //      //}

   //      //mProps = new List<HighLevelProperty>();
   //      //foreach (ReflectedPropertyBinder b in mBinders)
   //      //{
   //      //   ApplyMetadata(b, t.Name);
   //      //   HighLevelProperty p = GetHighLevelProperty(b, t.Name);
   //      //   mProps.Add(p);
   //      //   string bindName;
   //      //   Control c = p.GetEditor(out bindName);
   //      //   c.Name = b.GetName();
   //      //   temp.Add(c);
   //      //}


   //      //saving the ui changes for the end.  why is this so slow?
   //      this.SuspendLayout();
   //      MainTableLayoutPanel.Visible = false;
   //      MainTableLayoutPanel.SuspendLayout();
   //      MainTableLayoutPanel.Controls.Clear();
   //      MainTableLayoutPanel.RowStyles.Clear();
   //      mControls.Clear();
   //      foreach (Control c in temp)
   //      {
   //         AddRow(c.Name, c);
   //      }
   //      MainTableLayoutPanel.ResumeLayout();
   //      MainTableLayoutPanel.Visible = true;
   //      this.ResumeLayout();

   //   }
   //   //A few more maps need to be implemented as needed.
      
   //   Dictionary<string, Type> mBasicTypeEditors = new Dictionary<string, Type>();
   //   Dictionary<string, Dictionary<string, object>> mBasicTypeMetadata = new Dictionary<string, Dictionary<string, object>>();
   //   Dictionary<string, Dictionary<string, Dictionary<string, object>>> mSpecificPropertyMetadata = new Dictionary<string, Dictionary<string, Dictionary<string, object>>>();

   //   public void AddMetaDataForType(string propertyType, string metadataKey, object metadataValue)
   //   {

   //   }
   //   public void AddMetaDataForProp(string parentTypeName, string propertyName, string metadataKey, object metadataValue)
   //   {
   //      if (mSpecificPropertyMetadata.ContainsKey(parentTypeName) == false)
   //      {
   //         mSpecificPropertyMetadata[parentTypeName] = new Dictionary<string, Dictionary<string, object>>();
   //      }
   //      if(mSpecificPropertyMetadata[parentTypeName].ContainsKey(propertyName) == false)
   //      {
   //         mSpecificPropertyMetadata[parentTypeName][propertyName] = new Dictionary<string, object>();
   //      }         
   //      mSpecificPropertyMetadata[parentTypeName][propertyName][metadataKey] = metadataValue;
   //   }

   //   private bool mbOverwriteMetaData = false;
   //   private void ApplyMetadata(INamedTypedProperty b, string parentTypeName)
   //   {         
   //      if(mBasicTypeMetadata.ContainsKey(b.GetTypeName()))
   //      {


   //      }
   //      else if (mSpecificPropertyMetadata.ContainsKey(parentTypeName) && mSpecificPropertyMetadata[parentTypeName].ContainsKey(b.GetName()))
   //      {
   //         Dictionary<string, object> metadata = mSpecificPropertyMetadata[parentTypeName][b.GetName()];
   //         Dictionary<string, object>.Enumerator it = metadata.GetEnumerator();
   //         if (b.MetaData == null)
   //            b.MetaData = new Dictionary<string, object>();
   //         while (it.MoveNext())
   //         {
   //            if (mbOverwriteMetaData || b.MetaData.ContainsKey(it.Current.Key) == false)
   //            {
   //               b.MetaData[it.Current.Key] = it.Current.Value;
   //            }
   //         }

   //      }
   //   }

   //   //?? how can we promote based off of available metadata?
   //   private bool mbAllowPromoteFromMetadata = false; //add this to hl?

     
   //   private HighLevelProperty GetHighLevelProperty(INamedTypedProperty b)//, string parentTypeName)
   //   {
   //      HighLevelProperty p = null;
   //      if (false)
   //      {

   //      }
   //      else if (mbAllowPromoteFromMetadata == true /* && has promotable metadata??? */)
   //      {

   //      }
   //      else if (mBasicTypeEditors.ContainsKey(b.GetTypeName()) == true)
   //      {
   //         p = (HighLevelProperty)(mBasicTypeEditors[b.GetTypeName()].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));
   //      }
   //      else  //default
   //      {
   //         p = new HighLevelProperty(b);
   //      }
   //      return p;
   //   }

   //   private void InitBasicTypes()
   //   {
   //      mBasicTypeEditors["Single"] = typeof(NumericProperty);
   //      mBasicTypeEditors["Boolean"] = typeof(BoolProperty);
   //      mBasicTypeEditors["Color"] = typeof(ColorProperty);       
   //   }

   //   List<Control> mControls = new List<Control>();
   //   private void AddRow(string name, Control c)
   //   {        
   //      MainTableLayoutPanel.RowStyles.Add( new RowStyle(SizeType.AutoSize));
   //      Label l = new Label();
   //      l.Text = name;
   //      l.AutoSize = true;
   //      l.Name = name + "Label";

   //      l.Margin = new Padding(0);
   //      c.Margin = new Padding(0);

   //      c.Dock = DockStyle.Fill;

   //      mControls.Add(c);  
   //      MainTableLayoutPanel.Controls.Add(l, 0, mControls.Count);
   //      MainTableLayoutPanel.Controls.Add(c, 1, mControls.Count);
   //   }
   //   static Dictionary<Type, TypeDescription> mTypeDescriptions = new Dictionary<Type, TypeDescription>();
   //}


   //public class TypeDescription
   //{
   //   public TypeDescription(Type t)
   //   {
   //      PropertyInfo[] propinfo = t.GetProperties();
   //      foreach (PropertyInfo p in propinfo)
   //      {
   //         mCustomAttributes[p] = new List<object>(p.GetCustomAttributes(false));

   //         mPropertyInfoList.Add(p);

   //         mPropsByName[p.Name] = p;

   //         #region attempt at faster stuff
   //         //This would have been cool, but I was foiled by a few details.
   //         //MethodInfo[] accessors = p.GetAccessors();
   //         //foreach (MethodInfo m in accessors)
   //         //{
   //         //   if (m.Name.StartsWith("get"))
   //         //   {
   //         //      //mInvoke = Delegate.CreateDelegate(typeof(BasicSig), bindingContext, mMethod);                    
   //         //      //Delegate d = Delegate.CreateDelegate(typeof(GetSig), m);
   //         //      //object asdf = d.DynamicInvoke(new object[] { });
   //         //   }
   //         //   else if (m.Name.StartsWith("set"))
   //         //   {
   //         //   }
   //         //}
   //         #endregion
   //      }
   //   }
   //   public Dictionary<string, PropertyInfo> mPropsByName = new Dictionary<string, PropertyInfo>();
   //   public List<PropertyInfo> mPropertyInfoList = new List<PropertyInfo>();
   //   public Dictionary<PropertyInfo, List<object>> mCustomAttributes = new Dictionary<PropertyInfo, List<object>>();

   //}


   //public class ReflectedPropertyBinder : INamedTypedProperty
   //{
   //   PropertyInfo mPropInfo = null;

   //   bool mbReadOnly = false;
   //   public ReflectedPropertyBinder(object target, PropertyInfo p)
   //   {
   //      mTarget = target;
   //      mPropInfo = p;
   //      if (p.CanWrite == false)
   //      {
   //         MetaData["ReadOnly"] = true;
   //         mbReadOnly = true;
   //      }
   //   }
   //   object mTarget = null;
   //   public string GetName()
   //   {
   //      return mPropInfo.Name;
   //   }
   //   public string GetTypeName()
   //   {
   //      return mPropInfo.PropertyType.Name;
   //   }

   //   public object GetValue()
   //   {
   //      if (mTarget == null)
   //         return null;
   //      return mPropInfo.GetValue(mTarget, null);
   //   }
   //   public void SetValue(object val)
   //   {
   //      if (mbReadOnly || mTarget == null)
   //         return;

   //      object convertedValue = val;
   //      if (val.GetType() != mPropInfo.PropertyType)
   //      {
   //         convertedValue = Convert.ChangeType(val, mPropInfo.PropertyType);
   //      }
   //      mPropInfo.SetValue(mTarget, convertedValue, null);
   //      //mPropInfo.SetValue(mTarget, val, BindingFlags.Default,null, null, null);
   //   }

   //   Dictionary<string, object> mMetaData = new Dictionary<string, object>();
   //   public Dictionary<string, object> MetaData
   //   {
   //      get
   //      {
   //         return mMetaData;
   //      }
   //      set
   //      {
   //         mMetaData = value;
   //      }
   //   }

   //}

   //public class ReflectedPropertyMultiBinder : INamedTypedProperty
   //{
   //   Type mPropType = null;
   //   bool mbReadOnly = false;
   //   string mPropName = "";
   //   Dictionary<Type, TypeDescription> mRepresentedTypes = new Dictionary<Type, TypeDescription>();
   //   public ReflectedPropertyMultiBinder(object[] targets, string propName, Type propType, Dictionary<Type, TypeDescription> representedTypes)
   //   {
   //      mTargets = new List<object>();
   //      mTargets.AddRange(targets);

   //      mPropName = propName;
   //      mPropType = propType;
   //      mRepresentedTypes = representedTypes;

   //      Dictionary<Type, TypeDescription>.Enumerator it = mRepresentedTypes.GetEnumerator();
   //      while (it.MoveNext())
   //      {
   //         if (it.Current.Value.mPropsByName[propName].CanWrite == false)
   //         {
   //            MetaData["ReadOnly"] = true;
   //            mbReadOnly = true;
   //            break;
   //         }
   //      }
   //   }
   //   List<object> mTargets = new List<object>();
   //   public string GetName()
   //   {
   //      return mPropName;
   //   }
   //   public string GetTypeName()
   //   {
   //      return mPropType.Name;
   //   }

   //   //If all properties are equal return value, else return null
   //   public object GetValue()
   //   {
   //      if (mTargets.Count == 0)
   //         return null;

   //      IComparable value = null;
   //      foreach (object target in mTargets)
   //      {
   //         object res = mRepresentedTypes[target.GetType()].mPropsByName[mPropName].GetValue(target, null);

   //         IComparable value2 = res as IComparable;
   //         if (value2 == null)
   //         {
   //            return null;  //oops cant compare.
   //         }
   //         if (value == null)
   //         {
   //            value = value2;
   //            continue;
   //         }
   //         if (value.CompareTo(value2) != 0)
   //         {
   //            return null;
   //         }
   //      }
   //      return value;
   //   }
   //   public void SetValue(object val)
   //   {
   //      if (mbReadOnly || mTargets.Count == 0)
   //         return;
   //      object convertedValue = val;
   //      if (val.GetType() != mPropType)
   //      {
   //         convertedValue = Convert.ChangeType(val, mPropType);
   //      }
   //      foreach (object target in mTargets)
   //      {
   //         mRepresentedTypes[target.GetType()].mPropsByName[mPropName].SetValue(target, convertedValue, null);
   //      }
   //   }

   //   Dictionary<string, object> mMetaData = new Dictionary<string, object>();
   //   public Dictionary<string, object> MetaData
   //   {
   //      get
   //      {
   //         return mMetaData;
   //      }
   //      set
   //      {
   //         mMetaData = value;
   //      }
   //   }

   //}


}
