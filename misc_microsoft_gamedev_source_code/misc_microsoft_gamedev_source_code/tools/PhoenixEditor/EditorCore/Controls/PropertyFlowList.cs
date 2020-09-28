using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.XPath;

namespace EditorCore
{

   public partial class PropertyFlowList : ObjectEditorControl
   {
      public PropertyFlowList()
      {
         InitializeComponent();        
      }
      public bool mbUseLabels = true;

      public bool WrapContents
      {
         get
         {
            return flowLayoutPanel1.WrapContents;
         }
         set
         {
            flowLayoutPanel1.WrapContents = value;
         }
      }

      override protected void ClearUI()
      {
         flowLayoutPanel1.Controls.Clear();
      }
      override protected void AddControls(List<Control> controls)
      {
         
         this.SuspendLayout();
         int newHeight = 20;
         foreach(Control c in controls)
         {
            //if (mbUseLabels == true)
            //{
            //   Label l = new Label();
            //   l.AutoSize = true;
            //   l.Text = c.Name;
            //   l.Padding = new Padding(0);
            //   flowLayoutPanel1.Controls.Add(l);
            //}
            //flowLayoutPanel1.Controls.Add(c);
            //newHeight = c.Bounds.Bottom + 10; ;// c.Top + c.Height + 5;

            if (mbUseLabels == true)
            {
               Panel p = new Panel();
               Label l = new Label();
               l.AutoSize = true;
               l.Text = c.Name;
               l.Padding = new Padding(0);

               p.Controls.Add(l);
               l.Top = 0; l.Left = 0;
               p.Controls.Add(c);
               c.Top = l.Bottom; c.Left = 0;

               p.Width = (l.Width > c.Width) ? l.Width : c.Width;

               flowLayoutPanel1.Controls.Add(p);
               p.Height = c.Bottom;
               newHeight = p.Height + 10;
               c.Resize += new EventHandler(c_Resize);
            }
            else
            {
               flowLayoutPanel1.Controls.Add(c);
               newHeight = c.Bounds.Bottom + 10;
            }
         }
         this.ResumeLayout();
         if (this.Height < newHeight)
            this.Height = newHeight;      
      }

      void c_Resize(object sender, EventArgs e)
      {
         Control control = sender as Control;
         if (control != null && control.Parent != null)
         {
            int width = 0;
            foreach(Control subControl in control.Parent.Controls)
            {
               if(subControl.Width > width)
               {
                  width = subControl.Width;
               }
            }
            control.Parent.Width = width;
         }
      }
   }

   //public class ObjectEditorControl : UserControl
   //{
   //   public ObjectEditorControl()
   //   {
   //      //InitializeComponent();
   //      InitBasicTypes();

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
   //      if (obj == null)
   //      {
   //         this.Visible = false;
   //         this.SuspendLayout();

   //         ClearUI();

   //         mControls.Clear();
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
   //         ApplyMetadata(b, t.Name);
   //         HighLevelProperty p = GetHighLevelProperty(b,t.Name);
            
   //         if (b.MetaData.ContainsKey("UpdateEvent"))
   //            p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_Changed);

   //         if (b.MetaData.ContainsKey("Ignore"))
   //            continue;

   //         mProps.Add(p);
   //         string bindName;
   //         Control c = p.GetEditor(out bindName);

   //         c.Name = b.GetName();
   //         //AddRow(b.GetName(), c);

   //         temp.Add(c);

   //      }

   //      //saving the ui changes for the end.  why is this so slow?
   //      this.SuspendLayout();
   //      mControls.Clear();

   //      this.AddControls(temp);
   //      //MainTableLayoutPanel.Visible = false;
   //      //MainTableLayoutPanel.SuspendLayout();
   //      //MainTableLayoutPanel.Controls.Clear();
   //      //MainTableLayoutPanel.RowStyles.Clear();
   //      //mControls.Clear();
   //      //foreach (Control c in temp)
   //      //{
   //      //   AddRow(c.Name, c);
   //      //}
   //      //MainTableLayoutPanel.ResumeLayout();
   //      //MainTableLayoutPanel.Visible = true;
   //      this.ResumeLayout();

   //   }

   //   public int ControlCount
   //   {
   //      get 
   //      {
   //         return mControls.Count;
   //      }
   //   }

   //   public event PropertyChanged SelectedObjectPropertyChanged;
   //   public delegate void PropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop);
   //   void p_Changed(HighLevelProperty sender)
   //   {
   //      object temp = SelectedObject;

   //      if (SelectedObjectPropertyChanged != null)
   //         SelectedObjectPropertyChanged.Invoke(this, SelectedObject, sender);

   //      this.SelectedObject = null;
   //      this.SelectedObject = temp;

   //   }
   //   private void LoadSettings(object[] objects)
   //   {
   //      if (objects == null || objects.Length == 0)
   //      {
   //         this.Visible = false;
   //         this.SuspendLayout();
   //         mControls.Clear();

   //         //MainTableLayoutPanel.SuspendLayout();
   //         //MainTableLayoutPanel.Controls.Clear();
   //         //MainTableLayoutPanel.RowStyles.Clear();
   //         //MainTableLayoutPanel.ResumeLayout();
   //         this.ClearUI();

   //         this.ResumeLayout();


   //         //MainTableLayoutPanel.Controls.Clear();
   //         //MainTableLayoutPanel.RowStyles.Clear();
   //         //mControls.Clear();
   //         return;
   //      }
   //      if (objects.Length == 1)
   //      {
   //         LoadSettings(objects[0]);
   //         return;
   //      }

   //      this.Visible = true;
   //      string parentType = "";


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
   //      while (it.MoveNext())
   //      {
   //         if (representedTypes.Count == 1)
   //            parentType = it.Current.Key.Name;

   //         foreach (PropertyInfo p in it.Current.Value.mPropertyInfoList)
   //         {
   //            KeyValuePair<string, Type> info = new KeyValuePair<string, Type>(p.Name, p.PropertyType);
   //            if (!mergedPropertyList.ContainsKey(info))
   //            {
   //               mergedPropertyList[info] = 0;
   //            }
   //            mergedPropertyList[info]++;
   //         }
   //         //it.Current.
   //      }

   //      List<ReflectedPropertyMultiBinder> mMultiBinders = new List<ReflectedPropertyMultiBinder>();
   //      Dictionary<KeyValuePair<string, Type>, int>.Enumerator itProps = mergedPropertyList.GetEnumerator();
   //      while (itProps.MoveNext())
   //      {
   //         if (itProps.Current.Value == representedTypes.Count)
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

   //         if (b.MetaData.ContainsKey("Ignore"))
   //            continue;

   //         HighLevelProperty p = GetHighLevelProperty(b, parentType);//, t.Name);
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
   //      mControls.Clear();
   //      this.SuspendLayout();
   //      this.AddControls(temp);
   //      //MainTableLayoutPanel.Visible = false;
   //      //MainTableLayoutPanel.SuspendLayout();
   //      //MainTableLayoutPanel.Controls.Clear();
   //      //MainTableLayoutPanel.RowStyles.Clear();
   //      //foreach (Control c in temp)
   //      //{
   //      //   AddRow(c.Name, c);
   //      //}
   //      //MainTableLayoutPanel.ResumeLayout();
   //      //MainTableLayoutPanel.Visible = true;
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
   //      if (mSpecificPropertyMetadata[parentTypeName].ContainsKey(propertyName) == false)
   //      {
   //         mSpecificPropertyMetadata[parentTypeName][propertyName] = new Dictionary<string, object>();
   //      }
   //      mSpecificPropertyMetadata[parentTypeName][propertyName][metadataKey] = metadataValue;
   //   }


   //   Dictionary<string, Dictionary<string, Type>> mSpecificTypeEditor = new Dictionary<string, Dictionary<string, Type>>();

   //   public void SetTypeEditor(string parentTypeName, string propertyName, Type toEdit)
   //   {
   //      if (mSpecificTypeEditor.ContainsKey(parentTypeName) == false)
   //      {
   //         mSpecificTypeEditor[parentTypeName] = new Dictionary<string, Type>();
   //      }
   //      mSpecificTypeEditor[parentTypeName][propertyName] = toEdit;         
   //   }

   //   private bool mbOverwriteMetaData = false;
   //   private void ApplyMetadata(INamedTypedProperty b, string parentTypeName)
   //   {
   //      if (mBasicTypeMetadata.ContainsKey(b.GetTypeName()))
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


   //   private HighLevelProperty GetHighLevelProperty(INamedTypedProperty b, string parentTypeName)
   //   {
   //      HighLevelProperty p = null;
   //      if (false)
   //      {

   //      }
   //      else if (mbAllowPromoteFromMetadata == true /* && has promotable metadata??? */)
   //      {

   //      }
   //      else if(mSpecificTypeEditor.ContainsKey(parentTypeName) && mSpecificTypeEditor[parentTypeName].ContainsKey(b.GetName())) 
   //      {
   //         p = (HighLevelProperty)(mSpecificTypeEditor[parentTypeName][b.GetName()].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));

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
   //      mBasicTypeEditors["Int32"] = typeof(NumericProperty);
   //      mBasicTypeEditors["Boolean"] = typeof(BoolProperty);
   //      mBasicTypeEditors["Color"] = typeof(ColorProperty);
   //   }

   //   List<Control> mControls = new List<Control>();



   //   virtual protected void ClearUI()
   //   {
   //      //MainTableLayoutPanel.SuspendLayout();
   //      //foreach (Control c in MainTableLayoutPanel.Controls)
   //      //{
   //      //   c.SuspendLayout();
   //      //}
   //      //MainTableLayoutPanel.Controls.Clear();
   //      //MainTableLayoutPanel.RowStyles.Clear();
   //      //MainTableLayoutPanel.ResumeLayout();
   //   }
   //   virtual protected void AddControls(List<Control> controls)
   //   {
   //      //MainTableLayoutPanel.Visible = false;
   //      //MainTableLayoutPanel.SuspendLayout();
   //      //MainTableLayoutPanel.Controls.Clear();
   //      //MainTableLayoutPanel.RowStyles.Clear();

   //      //foreach (Control c in controls)
   //      //{
   //      //   AddRow(c.Name, c);
   //      //}
   //      //MainTableLayoutPanel.ResumeLayout();
   //      //MainTableLayoutPanel.Visible = true;

   //   }

   //   //private void AddRow(string name, Control c)
   //   //{
   //   //   MainTableLayoutPanel.RowStyles.Add(new RowStyle(SizeType.AutoSize));
   //   //   Label l = new Label();
   //   //   l.Text = name;
   //   //   l.AutoSize = true;
   //   //   l.Name = name + "Label";

   //   //   l.Margin = new Padding(0);
   //   //   c.Margin = new Padding(0);

   //   //   c.Dock = DockStyle.Fill;

   //   //   mControls.Add(c);
   //   //   MainTableLayoutPanel.Controls.Add(l, 0, mControls.Count);
   //   //   MainTableLayoutPanel.Controls.Add(c, 1, mControls.Count);
   //   //}
   //   static Dictionary<Type, TypeDescription> mTypeDescriptions = new Dictionary<Type, TypeDescription>();
   //}
}