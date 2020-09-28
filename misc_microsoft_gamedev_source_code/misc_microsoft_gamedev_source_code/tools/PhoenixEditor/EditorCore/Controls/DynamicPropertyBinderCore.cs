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


   public class TypeDescription
   {
      public TypeDescription(Type t)
      {
         PropertyInfo[] propinfo = t.GetProperties();
         foreach (PropertyInfo p in propinfo)
         {
            mCustomAttributes[p] = new List<object>(p.GetCustomAttributes(false));

            mPropertyInfoList.Add(p);

            mPropsByName[p.Name] = p;

            #region attempt at faster stuff
            //This would have been cool, but I was foiled by a few details.
            //MethodInfo[] accessors = p.GetAccessors();
            //foreach (MethodInfo m in accessors)
            //{
            //   if (m.Name.StartsWith("get"))
            //   {
            //      //mInvoke = Delegate.CreateDelegate(typeof(BasicSig), bindingContext, mMethod);                    
            //      //Delegate d = Delegate.CreateDelegate(typeof(GetSig), m);
            //      //object asdf = d.DynamicInvoke(new object[] { });
            //   }
            //   else if (m.Name.StartsWith("set"))
            //   {
            //   }
            //}
            #endregion
         }
      }
      public Dictionary<string, PropertyInfo> mPropsByName = new Dictionary<string, PropertyInfo>();
      public List<PropertyInfo> mPropertyInfoList = new List<PropertyInfo>();
      public Dictionary<PropertyInfo, List<object>> mCustomAttributes = new Dictionary<PropertyInfo, List<object>>();

   }

   public class LambdaObject : IPropertyArray  , ICloneTemplate
   {
      public LambdaObject(string parentName, string parentType, INamedTypedProperty[] properties)
      {
         mParentName = parentName;
         mParentType = parentType;
         mProperties = properties;
      }

      private LambdaObject() { }

      public string mParentName;
      public string mParentType;
      public INamedTypedProperty[] mProperties;

      public override string ToString()
      {
         return mParentName;
      }

      #region IPropertyArray Members

      public ICollection<INamedTypedProperty> GetProperties()
      {
         return mProperties;
         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion

      #region ICloneTemplate Members

      virtual public bool TryCloneEntry(out object clone)
      {
         LambdaObject copy = new LambdaObject(mParentName, mParentType, null);

         List<INamedTypedProperty> copyProps = new List<INamedTypedProperty>();
         foreach (INamedTypedProperty prop in mProperties)
         {
            ICloneTemplate cl = prop as ICloneTemplate;
            object newChild;
            if (cl.TryCloneEntry(out newChild))
            {
               if(newChild is INamedTypedProperty)
               {
                  copyProps.Add((INamedTypedProperty)newChild);
               }
            }
         }
         copy.mProperties = copyProps.ToArray();
         clone = copy;
         return true;
      }

      //public bool TryAddChild(object child)
      //{
      //   throw new Exception("The method or operation is not implemented.");
      //}

      #endregion
   }

   public interface ICollectionObserver
   {
      void Added(object obj);
      void Removed(object obj);
   }

   public class ObjectArrayProperty : INamedTypedProperty, IPropertyArray, ICollectionObserver
   {
      //LambdaObject mObject = null;
      public ObjectArrayProperty(string name, string type)//LambdaObject obj)
      {
         //TODO fix me

         //mObject = obj;
         //mName = name;
         //mType = type;
      }
      public string mName = "";
      string mType = "";
      public IList<INamedTypedProperty> mArray = new List<INamedTypedProperty>();

      #region INamedTypedProperty Members

      public string GetName()
      {
         return mName;
      }

      public string GetTypeName()
      {
         return mType;
      }

      public object GetValue()
      {
         return mArray;
      }

      public void SetValue(object val)
      {
         if (val is IList<INamedTypedProperty>)
         {
            //mObject = (LambdaObject)val;
            mArray = (IList<INamedTypedProperty>)val;
         }
      }

      Dictionary<string, object> mMetaData = new Dictionary<string, object>();
      public Dictionary<string, object> MetaData
      {
         get
         {
            return mMetaData;
         }
         set
         {
            mMetaData = value;
         }
      }

      #endregion

      #region IPropertyArray Members

      public ICollection<INamedTypedProperty> GetProperties()
      {
         return mArray;
         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion

      #region ICollectionObserver Members

      virtual public void Added(object obj)
      {

      }

      virtual public void Removed(object obj)
      {

      }

      #endregion
   }

   //Like cloneable, but not g
   public interface ICloneTemplate
   {
      bool TryCloneEntry(out object clone);
      //bool TryAddChild(object child);
      //object CloneFromTemplate();
   }

   public class LamdaObjectProperty : INamedTypedProperty , ICloneTemplate
   {
      protected LambdaObject mObject = null;
      public LamdaObjectProperty(LambdaObject obj)
      {
         mObject = obj;
      }


      #region INamedTypedProperty Members

      public string GetName()
      {
         return mObject.mParentName;
      }

      public string GetTypeName()
      {
         return mObject.mParentType;
      }

     

      public object GetValue()
      {
         return mObject;
      }

      public void SetValue(object val)
      {
         if (val is LambdaObject)
         {
            mObject = (LambdaObject)val;
         }
      }

      Dictionary<string, object> mMetaData = new Dictionary<string, object>();
      public Dictionary<string, object> MetaData
      {
         get
         {
            return mMetaData;
         }
         set
         {
            mMetaData = value;
         }
      }

      #endregion





      #region ICloneTemplate Members

      virtual public bool TryCloneEntry(out object clone)
      {
         clone = null;
         object subClone;
         if (mObject.TryCloneEntry(out subClone))
         {
            LamdaObjectProperty lprop = new LamdaObjectProperty((LambdaObject)subClone);
            lprop.MetaData = this.MetaData;
            
            clone = lprop;
            return true;
         }
         return false;
         //throw new Exception("The method or operation is not implemented.");
      }

      //public bool TryAddChild(object child)
      //{
      //   throw new Exception("The method or operation is not implemented.");
      //}

      #endregion
   }

   public class ReflectedPropertyBinder : INamedTypedProperty
   {
      PropertyInfo mPropInfo = null;

      bool mbReadOnly = false;
      public ReflectedPropertyBinder(object target, PropertyInfo p)
      {
         mTarget = target;
         mPropInfo = p;
         if (p.CanWrite == false)
         {
            MetaData["ReadOnly"] = true;
            mbReadOnly = true;
         }
         if (p.PropertyType.BaseType != null && p.PropertyType.BaseType.Name == "Enum")
         {
            MetaData["InternalType"] = "Enum";
         }
      }
      object mTarget = null;
      public string GetName()
      {
         return mPropInfo.Name;
      }
      public string GetTypeName()
      {
         return mPropInfo.PropertyType.Name;
      }

      public object GetValue()
      {
         if (mTarget == null)
            return null;
         return mPropInfo.GetValue(mTarget, null);
      }
      public void SetValue(object val)
      {
         if (mbReadOnly || mTarget == null)
            return;

         object convertedValue = val;
         if (val.GetType() != mPropInfo.PropertyType)
         {
            convertedValue = Convert.ChangeType(val, mPropInfo.PropertyType);
         }
         mPropInfo.SetValue(mTarget, convertedValue, null);
         //mPropInfo.SetValue(mTarget, val, BindingFlags.Default,null, null, null);
      }

      Dictionary<string, object> mMetaData = new Dictionary<string, object>();
      public Dictionary<string, object> MetaData
      {
         get
         {
            return mMetaData;
         }
         set
         {
            mMetaData = value;
         }
      }

   }

   public class ReflectedPropertyMultiBinder : INamedTypedProperty
   {
      Type mPropType = null;
      bool mbReadOnly = false;
      string mPropName = "";
      Dictionary<Type, TypeDescription> mRepresentedTypes = new Dictionary<Type, TypeDescription>();
      public ReflectedPropertyMultiBinder(object[] targets, string propName, Type propType, Dictionary<Type, TypeDescription> representedTypes)
      {
         mTargets = new List<object>();
         mTargets.AddRange(targets);

         mPropName = propName;
         mPropType = propType;
         mRepresentedTypes = representedTypes;

         Dictionary<Type, TypeDescription>.Enumerator it = mRepresentedTypes.GetEnumerator();
         while (it.MoveNext())
         {
            if (it.Current.Value.mPropsByName[propName].CanWrite == false)
            {
               MetaData["ReadOnly"] = true;
               mbReadOnly = true;
               break;
            }
            if (it.Current.Value.mPropsByName[propName].PropertyType.BaseType.Name == "Enum")
            {
               MetaData["InternalType"] = "Enum";
            }

         }
      }
      List<object> mTargets = new List<object>();
      public string GetName()
      {
         return mPropName;
      }
      public string GetTypeName()
      {
         return mPropType.Name;
      }

      //If all properties are equal return value, else return null
      public object GetValue()
      {
         if (mTargets.Count == 0)
            return null;

         IComparable value = null;
         foreach (object target in mTargets)
         {
            object res = mRepresentedTypes[target.GetType()].mPropsByName[mPropName].GetValue(target, null);

            IComparable value2 = res as IComparable;
            if (value2 == null)
            {
               return null;  //oops cant compare.
            }
            if (value == null)
            {
               value = value2;
               continue;
            }
            if (value.CompareTo(value2) != 0)
            {
               return null;
            }
         }
         return value;
      }
      public void SetValue(object val)
      {
         if (mbReadOnly || mTargets.Count == 0)
            return;
         object convertedValue = val;
         if (val.GetType() != mPropType)
         {
            convertedValue = Convert.ChangeType(val, mPropType);
         }
         foreach (object target in mTargets)
         {
            mRepresentedTypes[target.GetType()].mPropsByName[mPropName].SetValue(target, convertedValue, null);
         }
      }

      Dictionary<string, object> mMetaData = new Dictionary<string, object>();
      public Dictionary<string, object> MetaData
      {
         get
         {
            return mMetaData;
         }
         set
         {
            mMetaData = value;
         }
      }

   }


   public class ObjectEditorControl : UserControl
   {
      public ObjectEditorControl()
      {
         //InitializeComponent();
         InitBasicTypes();

      }

      private object mSelectedObject = null;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public object SelectedObject
      {
         set
         {
            mSelectedObject = value;
            LoadSettings(mSelectedObject);
         }
         get
         {
            return mSelectedObject;
         }

      }
      object[] mSelectedObjects = null;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public object[] SelectedObjects
      {
         set
         {
            mSelectedObjects = value;
            LoadSettings(mSelectedObjects);
         }
         get
         {
            return mSelectedObjects;
         }

      }


      //This loads metadata from a file
      public void LoadSettingsFromStream(Stream s)
      {
         XmlDocument d = new XmlDocument();
         d.Load(s);
         s.Close();

         XmlNodeList entries = d.SelectNodes("//MetaData");
         foreach (XmlNode entry in entries)
         {
            
            string className = entry.Attributes["Name"].InnerXml;
            
            //Load included settings
            XmlNodeList includes = entry.SelectNodes("Include");
            foreach (XmlNode include in includes)
            {
               XmlNode toinclude = d.SelectSingleNode("//MetaData[@Name='" + include.InnerText + "']");
               if (toinclude != null)
               {
                  LoadSettings(toinclude, className);

               }
            }
            
            //Load our settings
            LoadSettings(entry, className);
         }
      }

      private void LoadSettings(XmlNode entry, string className)
      {
         XmlNodeList properties = entry.SelectNodes("Property");
         //string className = d.FirstChild.Attributes["Name"].InnerXml;
         //string className = entry.Attributes["Name"].InnerXml;
         foreach (XmlNode n in properties)
         {
            string propertyName = (string)n.Attributes["Name"].InnerXml;

            XmlNodeList settings = n.ChildNodes;
            foreach (XmlNode setting in settings)
            {
               if (setting is XmlComment)
                  continue;
               string settingName = (string)setting.Attributes["Name"].InnerXml;
               this.AddMetaDataForProp(className, propertyName, settingName, setting.InnerText);
            }
         }
      }
      


      List<HighLevelProperty> mProps = new List<HighLevelProperty>();
      private void LoadSettings(object obj)
      {
         List<ReflectedPropertyBinder> mBinders = new List<ReflectedPropertyBinder>();
         if (obj == null)
         {
            this.Visible = false;
            this.SuspendLayout();

            //ClearUI();
            ClearUI();

            mControls.Clear();
            this.ResumeLayout();
            return;
         }

         if (obj is LambdaObject)
         {
            LoadSettings((LambdaObject)obj);
            return; 
         }
         if (obj is LamdaObjectProperty)
         {
            LamdaObjectProperty lambdaprop = (LamdaObjectProperty)obj;
            this.ApplyMetaDataToType(lambdaprop.GetName(), lambdaprop.GetName(), lambdaprop.MetaData);
            LoadSettings((LambdaObject)lambdaprop.GetValue());
            return;

         }

         this.Visible = true;


         List<Control> temp = new List<Control>();

         Type t = obj.GetType();
         if (mTypeDescriptions.ContainsKey(t) == false)
         {
            mTypeDescriptions[t] = new TypeDescription(t);
         }

         mBinders = new List<ReflectedPropertyBinder>();
         foreach (PropertyInfo p in mTypeDescriptions[t].mPropertyInfoList)
         {
            ReflectedPropertyBinder b = new ReflectedPropertyBinder(obj, p);
            mBinders.Add(b);
         }
         mProps = new List<HighLevelProperty>();
         foreach (ReflectedPropertyBinder b in mBinders)
         {
            ApplyMetadata(b, t.Name);
            HighLevelProperty p = GetHighLevelProperty(b, t.Name);

            if (b.MetaData.ContainsKey("UpdateEvent"))
               p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_Changed);

            p.Changed+=new HighLevelProperty.HighLevelPropertyEvent(p_AnyChanged);

            if (b.MetaData.ContainsKey("Ignore") && (bool)(b.MetaData["Ignore"]) == true)
               continue;

            mProps.Add(p);
            string bindName;
            Control c = p.GetEditor(out bindName);

            if (b.MetaData.ContainsKey("Alias"))
            {
               c.Name = b.MetaData["Alias"].ToString();
            }
            else
            {
               c.Name = b.GetName();
            }

            //Set editor properties:
            Type edtiorType = c.GetType();
            foreach (string s in b.MetaData.Keys)
            {
               if (s.Contains("EditorProperty."))
               {
                  string editorProp = s.Replace("EditorProperty.", "");
                  PropertyInfo editorPropertyInfo = edtiorType.GetProperty(editorProp);
                  object convertedValue = b.MetaData[s];
                  convertedValue = Convert.ChangeType(convertedValue, editorPropertyInfo.PropertyType);
                  editorPropertyInfo.SetValue(c, convertedValue, null);
               }
            }


            temp.Add(c);

         }

         //saving the ui changes for the end.  why is this so slow?
         this.SuspendLayout();
         mControls.Clear();

         this.AddControls(temp);

         this.ResumeLayout();

      }


      public Control GetSingleControl(object obj, string field)
      {
         INamedTypedProperty b = GetPropertyFromObject(obj, field);
         if (b != null)
            return GetSingleControl(b, obj.GetType().Name);
         return null;
      }
      public INamedTypedProperty GetPropertyFromObject(object obj, string field)
      {
         Type t = obj.GetType();
         if (mTypeDescriptions.ContainsKey(t) == false)
         {
            mTypeDescriptions[t] = new TypeDescription(t);
         }         
         foreach (PropertyInfo p in mTypeDescriptions[t].mPropertyInfoList)
         {
            if (p.Name == field)
            {
               ReflectedPropertyBinder b = new ReflectedPropertyBinder(obj, p);
               return b;
            }
         }
         return null;
      }      
      public Control GetSingleControl(INamedTypedProperty b, string typeName )
      {
         ApplyMetadata(b, typeName);
         HighLevelProperty p = GetHighLevelProperty(b, typeName);

         if (b.MetaData.ContainsKey("UpdateEvent"))
            p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_Changed);

         p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_AnyChanged);

         if (b.MetaData.ContainsKey("Ignore") && (bool)(b.MetaData["Ignore"]) == true)
            return null;// continue;
         
         //mProps.Add(p);
         string bindName;
         Control c = p.GetEditor(out bindName);

         if (b.MetaData.ContainsKey("Alias"))
         {
            c.Name = b.MetaData["Alias"].ToString();
         }
         else
         {
            c.Name = b.GetName();
         }

         //Set editor properties:
         Type edtiorType = c.GetType();
         foreach (string s in b.MetaData.Keys)
         {
            if (s.Contains("EditorProperty."))
            {
               string editorProp = s.Replace("EditorProperty.", "");
               PropertyInfo editorPropertyInfo = edtiorType.GetProperty(editorProp);
               object convertedValue = b.MetaData[s];
               convertedValue = Convert.ChangeType(convertedValue, editorPropertyInfo.PropertyType);
               editorPropertyInfo.SetValue(c, convertedValue, null);
            }
         }
         return c;
      }


      public int ControlCount
      {
         get
         {
            return mControls.Count;
         }
      }

      public event PropertyChanged SelectedObjectPropertyChanged;
      public delegate void PropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop);
      void p_Changed(HighLevelProperty sender)
      {

         if (SelectedObjectPropertyChanged != null)
            SelectedObjectPropertyChanged.Invoke(this, SelectedObject, sender);

         ////??? does this refresh belong here?
         //object temp = SelectedObject;
         //this.SelectedObject = null;
         //this.SelectedObject = temp;

      }

      public event PropertyChanged AnyPropertyChanged;
      void p_AnyChanged(HighLevelProperty sender)
      {

         if (AnyPropertyChanged != null)
            AnyPropertyChanged.Invoke(this, SelectedObject, sender);
      }
      private void LoadSettings(object[] objects)
      {
         if (objects == null || objects.Length == 0)
         {
            this.Visible = false;
            this.SuspendLayout();
            mControls.Clear();
            this.ClearUI();
            this.ResumeLayout();
            return;
         }
         if (objects.Length == 1)
         {
            LoadSettings(objects[0]);
            return;
         }

         this.Visible = true;
         string parentType = "";

         List<Control> temp = new List<Control>();
         Dictionary<Type, TypeDescription> representedTypes = new Dictionary<Type, TypeDescription>();

         foreach (object obj in objects)
         {
            Type t = obj.GetType();
            if (mTypeDescriptions.ContainsKey(t) == false)
            {
               mTypeDescriptions[t] = new TypeDescription(t);
            }
            representedTypes[t] = mTypeDescriptions[t];
         }

         Dictionary<KeyValuePair<string, Type>, int> mergedPropertyList = new Dictionary<KeyValuePair<string, Type>, int>();
         Dictionary<Type, TypeDescription>.Enumerator it = representedTypes.GetEnumerator();
         while (it.MoveNext())
         {
            if (representedTypes.Count == 1)
               parentType = it.Current.Key.Name;

            foreach (PropertyInfo p in it.Current.Value.mPropertyInfoList)
            {
               KeyValuePair<string, Type> info = new KeyValuePair<string, Type>(p.Name, p.PropertyType);
               if (!mergedPropertyList.ContainsKey(info))
               {
                  mergedPropertyList[info] = 0;
               }
               mergedPropertyList[info]++;
            }
            //it.Current.
         }

         List<ReflectedPropertyMultiBinder> mMultiBinders = new List<ReflectedPropertyMultiBinder>();
         Dictionary<KeyValuePair<string, Type>, int>.Enumerator itProps = mergedPropertyList.GetEnumerator();
         while (itProps.MoveNext())
         {
            if (itProps.Current.Value == representedTypes.Count)
            {
               //mPropsByName
               ReflectedPropertyMultiBinder b = new ReflectedPropertyMultiBinder(objects, itProps.Current.Key.Key, itProps.Current.Key.Value, representedTypes);
               mMultiBinders.Add(b);
            }
         }


         mProps = new List<HighLevelProperty>();
         foreach (ReflectedPropertyMultiBinder b in mMultiBinders)
         {

            Dictionary<Type, TypeDescription>.Enumerator repTypes = representedTypes.GetEnumerator();
            while (repTypes.MoveNext())
            {
               ApplyMetadata(b, repTypes.Current.Key.Name);
            }

            if (b.MetaData.ContainsKey("Ignore") && (bool)(b.MetaData["Ignore"]) == true)
               continue;




            HighLevelProperty p = GetHighLevelProperty(b, parentType);//, t.Name);
            mProps.Add(p);
            string bindName;
            Control c = p.GetEditor(out bindName);

            if (b.MetaData.ContainsKey("Alias"))
            {
               c.Name = b.MetaData["Alias"].ToString();
            }
            else
            {
               c.Name = b.GetName();
            }

            //Set editor properties:
            Type edtiorType = c.GetType();
            foreach (string s in b.MetaData.Keys)
            {
               if (s.Contains("EditorProperty."))
               {
                  string editorProp = s.Replace("EditorProperty.","");
                  PropertyInfo editorPropertyInfo = edtiorType.GetProperty(editorProp);
                  object convertedValue = b.MetaData[s];
                  convertedValue = Convert.ChangeType(convertedValue, editorPropertyInfo.PropertyType);         
                  editorPropertyInfo.SetValue(c, convertedValue, null);                  
               }
            }

            //c.Name = b.GetName();
            temp.Add(c);
         }
        
         mControls.Clear();
         this.SuspendLayout();
         this.AddControls(temp);
         this.ResumeLayout();

      }

      private void LoadSettings(LambdaObject lambdaObject)
      {
         string parentName ="";
         string parentType ="";
         INamedTypedProperty[] properties = null;

         if (lambdaObject != null)
         {
            parentName = lambdaObject.mParentName;
            parentType = lambdaObject.mParentType;
            properties = lambdaObject.mProperties;
         }

         if (properties == null || properties.Length == 0)
         {
            this.Visible = false;
            this.SuspendLayout();
            mControls.Clear();
            this.ClearUI();
            this.ResumeLayout();
            return;
         }

         this.Visible = true;         

         List<Control> temp = new List<Control>();
  
         //could we refactor this section:....

         mProps = new List<HighLevelProperty>();
         foreach (INamedTypedProperty ip in properties)
         {
            ApplyMetadata(ip, parentType);
            if (ip.MetaData.ContainsKey("Ignore") && (bool)(ip.MetaData["Ignore"]) == true)
               continue;

            //This code is for supporting arrays:
            IPropertyArray ar2 = ip as IPropertyArray;
            if (ar2 != null)
            {
               foreach (INamedTypedProperty inner in ar2.GetProperties())
               {
                  if (inner.GetValue() == null)
                     continue;
                  IPropertyArray propertyArray = inner.GetValue() as IPropertyArray;
                  if (propertyArray != null)
                  {
                     foreach (INamedTypedProperty inner2 in ((IPropertyArray)inner.GetValue()).GetProperties())
                     {
                        Dictionary<string, object> innerData;
                        if (mSpecificPropertyMetadata[ip.MetaData["ArrayType"].ToString()].TryGetValue(inner2.GetName(), out innerData))
                        {
                           inner2.MetaData = innerData;
                        }
                     }
                  }
               }
            }            
            //Nested logic:  This pushes complext type metadata inwards
            else if (ip.MetaData.ContainsKey("ComplexType") == true)
            {
               IPropertyArray ar = ip.GetValue() as IPropertyArray;
               if(ar != null)
               {
                  foreach (INamedTypedProperty inner in ar.GetProperties())
                  {
                     Dictionary<string,object> innerData;
                     if(mSpecificPropertyMetadata[ip.MetaData["ComplexType"].ToString()].TryGetValue(inner.GetName(), out innerData))
                     {
                        inner.MetaData = innerData;
                     }
                  }
               }
            }


            HighLevelProperty p = GetHighLevelProperty(ip, parentType);
            

            //////events
            //if (ip.MetaData.ContainsKey("UpdateEvent"))
            //   p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_Changed);

            p.Changed += new HighLevelProperty.HighLevelPropertyEvent(p_AnyChanged);
            /////events

            if (ip.MetaData.ContainsKey("Ignore") && (bool)(ip.MetaData["Ignore"]) == true)
               continue;


            mProps.Add(p);
            string bindName;
            Control c = p.GetEditor(out bindName);

            if (ip.MetaData.ContainsKey("Alias"))
            {
               c.Name = ip.MetaData["Alias"].ToString();
            }
            else
            {
               c.Name = ip.GetName();
            }

            //Set editor properties:
            Type edtiorType = c.GetType();
            foreach (string s in ip.MetaData.Keys)
            {
               if (s.Contains("EditorProperty."))
               {
                  string editorProp = s.Replace("EditorProperty.", "");
                  PropertyInfo editorPropertyInfo = edtiorType.GetProperty(editorProp);
                  object convertedValue = ip.MetaData[s];
                  convertedValue = Convert.ChangeType(convertedValue, editorPropertyInfo.PropertyType);
                  editorPropertyInfo.SetValue(c, convertedValue, null);
               }
            }

            temp.Add(c);
         }

         mControls.Clear();
         this.SuspendLayout();
         this.AddControls(temp);
         this.ResumeLayout();

      }



      //A few more maps need to be implemented as needed.

      Dictionary<string, Type> mInternalTypeEditors = new Dictionary<string, Type>();
      Dictionary<string, Type> mBaseTypeEditors = new Dictionary<string, Type>();
      Dictionary<string, Dictionary<string, object>> mBasicTypeMetadata = new Dictionary<string, Dictionary<string, object>>();
      Dictionary<string, Dictionary<string, Dictionary<string, object>>> mSpecificPropertyMetadata = new Dictionary<string, Dictionary<string, Dictionary<string, object>>>();

      public void AddMetaDataForProps(string parentTypeName, string[] properties, string metadataKey, object metadataValue)
      {
         foreach (string s in properties)
         {
            AddMetaDataForProp(parentTypeName, s, metadataKey, metadataValue);
         }
      }

      public void IgnoreProperties(string parentTypeName, string[] properties)
      {
         foreach(string s in properties)
         {
            AddMetaDataForProp(parentTypeName, s, "Ignore", true);
         }
      }

      public void AddMetaDataForType(string propertyType, string metadataKey, object metadataValue)
      {

      }
      public void AddMetaDataForProp(string parentTypeName, string propertyName, string metadataKey, object metadataValue)
      {
         if (mSpecificPropertyMetadata.ContainsKey(parentTypeName) == false)
         {
            mSpecificPropertyMetadata[parentTypeName] = new Dictionary<string, Dictionary<string, object>>();
         }
         if (mSpecificPropertyMetadata[parentTypeName].ContainsKey(propertyName) == false)
         {
            mSpecificPropertyMetadata[parentTypeName][propertyName] = new Dictionary<string, object>();
         }
         mSpecificPropertyMetadata[parentTypeName][propertyName][metadataKey] = metadataValue;
      }

      public void ApplyMetaDataToType(string parentTypeName, string propertyName, Dictionary<string, object> metadata)
      {
         if (mSpecificPropertyMetadata.ContainsKey(parentTypeName) == false)
         {
            mSpecificPropertyMetadata[parentTypeName] = new Dictionary<string, Dictionary<string, object>>();
         }
         if (mSpecificPropertyMetadata[parentTypeName].ContainsKey(propertyName) == false)
         {
            mSpecificPropertyMetadata[parentTypeName][propertyName] = new Dictionary<string, object>();
         }
         Dictionary<string, object>.Enumerator it = metadata.GetEnumerator();
         while (it.MoveNext())
         {
            mSpecificPropertyMetadata[parentTypeName][propertyName][it.Current.Key] = it.Current.Value;
         }      
      }

      public Dictionary <string, Dictionary<string, object>> GetMetadataForType(string typeName) 
      {
         Dictionary<string, Dictionary<string, object>> data = new Dictionary<string, Dictionary<string, object>>();
         mSpecificPropertyMetadata.TryGetValue(typeName, out data);           
         return data;
      }

      Dictionary<string, Dictionary<string, Type>> mSpecificTypeEditor = new Dictionary<string, Dictionary<string, Type>>();

      public void SetTypeEditor(string parentTypeName, string propertyName, Type toEdit)
      {
         if (mSpecificTypeEditor.ContainsKey(parentTypeName) == false)
         {
            mSpecificTypeEditor[parentTypeName] = new Dictionary<string, Type>();
         }
         mSpecificTypeEditor[parentTypeName][propertyName] = toEdit;
      }

      private bool mbOverwriteMetaData = false;
      private void ApplyMetadata(INamedTypedProperty b, string parentTypeName)
      {
         if (mBasicTypeMetadata.ContainsKey(b.GetTypeName()))
         {


         }
         else if (mSpecificPropertyMetadata.ContainsKey(parentTypeName) && mSpecificPropertyMetadata[parentTypeName].ContainsKey(b.GetName()))
         {
            Dictionary<string, object> metadata = mSpecificPropertyMetadata[parentTypeName][b.GetName()];
            Dictionary<string, object>.Enumerator it = metadata.GetEnumerator();
            if (b.MetaData == null)
               b.MetaData = new Dictionary<string, object>();
            while (it.MoveNext())
            {
               if (mbOverwriteMetaData || b.MetaData.ContainsKey(it.Current.Key) == false)
               {
                  b.MetaData[it.Current.Key] = it.Current.Value;
               }
            }

         }
      }

      //?? how can we promote based off of available metadata?
      private bool mbAllowPromoteFromMetadata = false; //add this to hl?


      private HighLevelProperty GetHighLevelProperty(INamedTypedProperty b, string parentTypeName)
      {
         HighLevelProperty p = null;
         if (false)
         {

         }
         else if (b.MetaData.ContainsKey("ComplexType") == true)
         {
            p = new ComplexTypeProperty(b);
         }
         else if (mbAllowPromoteFromMetadata == true /* && has promotable metadata??? */)
         {

         }
         else if (mSpecificTypeEditor.ContainsKey(parentTypeName) && mSpecificTypeEditor[parentTypeName].ContainsKey(b.GetName()))
         {
            p = (HighLevelProperty)(mSpecificTypeEditor[parentTypeName][b.GetName()].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));

         }
         else if (mInternalTypeEditors.ContainsKey(b.GetTypeName()) == true)
         {
            p = (HighLevelProperty)(mInternalTypeEditors[b.GetTypeName()].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));
         }
         else if (b.MetaData.ContainsKey("BaseType") && mBaseTypeEditors.ContainsKey(b.MetaData["BaseType"].ToString()))
         {
            string baseTypeName = b.MetaData["BaseType"] as string;
            p = (HighLevelProperty)(mBaseTypeEditors[baseTypeName].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));

         }
         //   //is this good???
         //else if (b.MetaData.ContainsKey("BaseType") && mInternalTypeEditors.ContainsKey(b.MetaData["BaseType"].ToString()))
         //{
         //   string baseTypeName = b.MetaData["BaseType"] as string;
         //   p = (HighLevelProperty)(mInternalTypeEditors[baseTypeName].GetConstructor(new Type[] { typeof(INamedTypedProperty) }).Invoke(new object[] { b }));
         //}
         else  //default
         {
            p = new HighLevelProperty(b);
         }
         return p;
      }

      private void InitBasicTypes()
      {
         //Internal C# types
         mInternalTypeEditors["Single"] = typeof(NumericProperty);
         mInternalTypeEditors["Int32"] = typeof(NumericProperty);
         mInternalTypeEditors["Boolean"] = typeof(BoolProperty);
         mInternalTypeEditors["Color"] = typeof(ColorProperty);


         //Overrideable types:
         mBaseTypeEditors["Single"] = typeof(NumericProperty);
         mBaseTypeEditors["Int32"] = typeof(NumericProperty);
         mBaseTypeEditors["Boolean"] = typeof(BoolProperty);
         mBaseTypeEditors["Color"] = typeof(ColorProperty);

         //special cases
         mBaseTypeEditors["Enum"] = typeof(EnumeratedProperty);

         //Common
         mBaseTypeEditors["FileName"] = typeof(FileNameProperty);
         mBaseTypeEditors["EnumeratedSetProperty"] = typeof(EnumeratedSetProperty);

         mBaseTypeEditors["LocStringIDProperty"] = typeof(LocStringIDProperty);

         //numeric basic types
         mBaseTypeEditors["Float"] = typeof(NumericProperty);
         mBaseTypeEditors["Integer"] = typeof(NumericProperty);

         //Array
         mBaseTypeEditors["Array"] = typeof(ArrayTypeProperty);

      }

      List<Control> mControls = new List<Control>();


      public void UpdateData()
      {
         if(SelectedObject != null)
         {
            //SelectedObject = SelectedObject;
            object temp = SelectedObject;
            SelectedObject = null;
            SelectedObject = temp;
         }

      }


      virtual protected void ClearUI()
      {
      }
      virtual protected void AddControls(List<Control> controls)
      {

      }

      static Dictionary<Type, TypeDescription> mTypeDescriptions = new Dictionary<Type, TypeDescription>();
   }
}
