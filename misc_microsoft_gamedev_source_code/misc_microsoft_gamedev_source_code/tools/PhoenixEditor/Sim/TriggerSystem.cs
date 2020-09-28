using System;
using System.Collections.Generic;
using System.Text;
using Rendering;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Drawing;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Collections;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;


using ModelSystem;
using EditorCore;

namespace SimEditor
{
 

   public interface INamedVariable
   {
      string GetName();
      int GetID();
      void SetID(int id);
   }

   public interface ITypedValue
   {
      string GetTypeName();
      object GetValue();
      void SetValue(object val);

      int GetID();
   }

   public class VarValueBinder : INamedTypedProperty
   {
      private INamedVariable mVariable;
      private ITypedValue mValue;

      public VarValueBinder(INamedVariable variable, ITypedValue value)
      {
         mVariable = variable;
         mValue = value;
      }
      public void UpdateValue(ITypedValue value)
      {
         mValue = value;
      }
      public string GetName()
      {
         return mVariable.GetName();
      }
      public string GetTypeName()
      {
         return mValue.GetTypeName();
      }
      public object GetValue()
      {
         return mValue.GetValue();
      }
      public void SetValue(object val)
      {
         mValue.SetValue(val);
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

   [XmlRoot("NoteNodeXml")]
   public class NoteNodeXml
   {
      [System.Xml.Serialization.XmlAttribute]
      public int X = -1;
      [System.Xml.Serialization.XmlAttribute]
      public int Y = -1;
      [System.Xml.Serialization.XmlAttribute]
      public int Width = -1;
      [System.Xml.Serialization.XmlAttribute]
      public int Height = -1;
      [System.Xml.Serialization.XmlAttribute]
      public int GroupID = -1;
      [System.Xml.Serialization.XmlElement]
      public string Title = "";
      public string Description = "";
   }

   [XmlRoot("GroupUI")]
   public class GroupUI
   {
      [XmlAttribute]
      public int X = -1;
      [XmlAttribute]
      public int Y = -1;

      [XmlAttribute]
      public int iX = -1;
      [XmlAttribute]
      public int iY = -1;
      [XmlAttribute]
      public int oX = -1;
      [XmlAttribute]
      public int oY = -1;

      [XmlAttribute]
      public int Width = -1;
      [XmlAttribute]
      public int Height = -1;
      [XmlAttribute]
      public int GroupID = -1;
      [XmlAttribute]
      public int InternalGroupID = -1;
      [XmlElement]
      public string Title = "";


   }


   [XmlRoot("UIData")]
   public class TriggerUIData
   {
      [Browsable(false)]
      [System.Xml.Serialization.XmlArray("NoteNodes")]
      public List<NoteNodeXml> mNoteNodes = new List<NoteNodeXml>();


      [Browsable(false)]
      [System.Xml.Serialization.XmlArray("Groups")]
      public List<GroupUI> mGroups = new List<GroupUI>();


      //[Browsable(false)]
      //[System.Xml.Serialization.XmlArray("UserClasses")]
      //public List<UserClassDefinition> mUserClasses = new List<UserClassDefinition>();
   }


   [XmlRoot("EditorData")]
   public class TriggerEditorData
   {
      TriggerRoot mTriggerSystem = null;//new TriggerRoot();
      TriggerUIData mUIData = new TriggerUIData();
      List<TriggerTemplateMapping> mTriggerMappings = new List<TriggerTemplateMapping>();

      [Browsable(false)]
      [System.Xml.Serialization.XmlArray("TriggerMappings")]
      public List<TriggerTemplateMapping> TriggerMappings
      {
         get
         {
            return this.mTriggerMappings;
         }
         set
         {
            this.mTriggerMappings = value;
         }
      }

      [Browsable(false)]
      [System.Xml.Serialization.XmlElement("UIData")]
      public TriggerUIData UIData
      {
         get
         {
            return this.mUIData;
         }
         set
         {
            this.mUIData = value;
         }
      }

      [Browsable(false)]
      [System.Xml.Serialization.XmlElement("TriggerSystem")]
      public TriggerRoot TriggerSystem
      {
         get
         {
            return this.mTriggerSystem;
         }
         set
         {
            this.mTriggerSystem = value;
         }
      }



   }

   [XmlRoot("Group")]
   public class GameGroupInfo
   {
      [XmlAttribute("ID")]
      public int ID;
      [XmlAttribute("Name")]
      public string Name;
      [XmlText]
      public string Children ="";

   }
   [XmlRoot("UserClasses")]
   public class UserClassDefinitions
   {
      [Browsable(false)]
      //[System.Xml.Serialization.XmlArray("UserClasses")]
      [XmlElement("UserClass", typeof(UserClassDefinition))]
      public List<UserClassDefinition> mUserClasses = new List<UserClassDefinition>();
   }

   [XmlRoot("UserClass")]
   public class UserClassDefinition
   {
      int mID = -1;
      [XmlAttribute("DBID")]
      public int DBID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
         }
      }

      string mName = "";
      [XmlAttribute("Name")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }

      List<UserClassFieldDefinition> mFields = new List<UserClassFieldDefinition>();

      [XmlElement("Fields", typeof(UserClassFieldDefinition))]
      public List<UserClassFieldDefinition> Fields
      {
         get
         {
            return mFields;
         }
         set
         {
            mFields = value;
         }
      }
   }

   public class UserClassFieldDefinition
   {
      string mName = "";
      [XmlAttribute("Name")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }
      string mType = "";
      [XmlAttribute("Type")]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }
   }





   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   //[System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   [XmlRoot("TriggerSystem")]
   public partial class TriggerRoot
   {
      [XmlAttribute("Name")]
      public string Name = "Script";

      [XmlAttribute("Type")]
      public string Type = "Scenario";

      private TriggerVars triggerVars = new TriggerVars();

      private List<Trigger> triggerField = new List<Trigger>();

      private int nextTriggerVarID = 0;

      private int nextTriggerIDField = 0;

      private int nextConditionIDField = 0;

      private int nextEffectIDField = 0;

      //private int nextTemplateIDField = 0;

      private TriggerEditorData triggerEditorData = new TriggerEditorData();

      [Browsable(false)]
      [System.Xml.Serialization.XmlArray("TriggerGroups")]
      [XmlArrayItem(ElementName = "Group", Type = typeof(GameGroupInfo))]
      public List<GameGroupInfo> GroupInfo = new List<GameGroupInfo>();

      /// <remarks/>
      [Browsable(false)]
      [System.Xml.Serialization.XmlElementAttribute("TriggerVars")]
      public TriggerVars TriggerVars
      {
         get
         {
            return this.triggerVars;
         }
         set
         {
            this.triggerVars = value;
         }
      }

      /// <remarks/>
      [Browsable(false)]
      //[System.Xml.Serialization.XmlElementAttribute("Triggers")]
      [System.Xml.Serialization.XmlArray("Triggers")]
      public List<Trigger> Trigger
      {
         get
         {
            return this.triggerField;
         }
         set
         {
            this.triggerField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int NextTriggerVarID
      {
         get
         {
            return this.nextTriggerVarID;
         }
         set
         {
            this.nextTriggerVarID = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int NextTriggerID
      {
         get
         {
            return this.nextTriggerIDField;
         }
         set
         {
            this.nextTriggerIDField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int NextConditionID
      {
         get
         {
            return this.nextConditionIDField;
         }
         set
         {
            this.nextConditionIDField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int NextEffectID
      {
         get
         {
            return this.nextEffectIDField;
         }
         set
         {
            this.nextEffectIDField = value;
         }
      }

      bool mExternal = false;
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool External
      {
         get
         {
            return this.mExternal;
         }
         set
         {
            this.mExternal = value;
         }
      }
      ///// <remarks/>
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int NextTemplateID
      //{
      //   get
      //   {
      //      return this.nextTemplateIDField;
      //   }
      //   set
      //   {
      //      this.nextTemplateIDField = value;
      //   }
      //}

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("EditorData")]
      public TriggerEditorData TriggerEditorData
      {
         get
         {
            return this.triggerEditorData;
         }
         set
         {
            this.triggerEditorData = value;
         }
      }   
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [XmlRoot("Trigger")]
   public partial class Trigger
   {

      private TriggerConditionsContainer triggerConditionsField = new TriggerConditionsContainer();

      private TriggerEffectsContainer triggerEffectsField = new TriggerEffectsContainer();
      private TriggerEffectsContainer triggerEffectsFalseField = new TriggerEffectsContainer();

      private int idField;

      private string nameField;

      private bool activeField;

      //private bool stayActiveOnFireField;

      private bool conditionalTriggerField;

      private int evaluateFrequency = 0;

      private int x = -1;
      private int y = -1;
      private int groupID = -1;
      private int templateID = -1;

      private int evalLimit = 0;

      
      private bool commentOut = false;

      public void ShallowCopyTo(Trigger other)
      {
         other.ID = this.ID;
         other.Name = this.Name;
         other.Active = this.Active;
         //other.StayActiveOnFire = this.StayActiveOnFire;
         other.ConditionalTrigger = this.ConditionalTrigger;
         other.EvaluateFrequency = this.EvaluateFrequency;
         other.X = this.X;
         other.Y = this.Y;
         other.GroupID = this.GroupID;
         other.EvalLimit = this.EvalLimit;
         other.CommentOut = this.CommentOut;

      }
      public void DeepCopyTo(Trigger other)
      {
         this.ShallowCopyTo(other);

         TriggerConditions.CopyTo(other.TriggerConditions);
         TriggerEffects.CopyTo(other.TriggerEffects);
         TriggerEffectsFalse.CopyTo(other.TriggerEffectsFalse);

      }

      /// <remarks/>
      [Browsable(false)]
      [System.Xml.Serialization.XmlElement("TriggerConditions")]
      public TriggerConditionsContainer TriggerConditions
      {
         get
         {
            return this.triggerConditionsField;
         }
         set
         {
            this.triggerConditionsField = value;
         }
      }


      /// <remarks/>
      [Browsable(false)]
      [System.Xml.Serialization.XmlElement("TriggerEffectsOnTrue")]
      public TriggerEffectsContainer TriggerEffects
      {
         get
         {
            return this.triggerEffectsField;
         }
         set
         {
            this.triggerEffectsField = value;
         }
      }

      /// <remarks/>
      [Browsable(false)]
      [System.Xml.Serialization.XmlElement("TriggerEffectsOnFalse")]
      public TriggerEffectsContainer TriggerEffectsFalse
      {
         get
         {
            return this.triggerEffectsFalseField;
         }
         set
         {
            this.triggerEffectsFalseField = value;
         }
      }


      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool Active
      {
         get
         {
            return this.activeField;
         }
         set
         {
            this.activeField = value;
         }
      }

      ///// <remarks/>
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public bool StayActiveOnFire
      //{
      //   get
      //   {
      //      return this.stayActiveOnFireField;
      //   }
      //   set
      //   {
      //      this.stayActiveOnFireField = value;
      //   }
      //}

      [System.Xml.Serialization.XmlAttribute()]
      public int EvaluateFrequency
      {
         get
         {
            return this.evaluateFrequency;
         }
         set
         {
            this.evaluateFrequency = value;
         }
      }
      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int EvalLimit
      {
         get
         {
            return this.evalLimit;
         }
         set
         {
            this.evalLimit = value;
         }
      }
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool CommentOut
      {
         get
         {
            return this.commentOut;
         }
         set
         {
            this.commentOut = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool ConditionalTrigger
      {
         get
         {
            return this.conditionalTriggerField;
         }
         set
         {
            this.conditionalTriggerField = value;
         }
      }

      [System.Xml.Serialization.XmlAttribute]
      public int X
      {
         get
         {
            return this.x;
         }
         set
         {
            this.x = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int Y
      {
         get
         {
            return this.y;
         }
         set
         {
            this.y = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int GroupID
      {
         get
         {
            return this.groupID;
         }
         set
         {
            this.groupID = value;
         }
      }      
      
      [System.Xml.Serialization.XmlAttribute]
      public int TemplateID
      {
         get
         {
            return this.templateID;
         }
         set
         {
            this.templateID = value;
         }
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRoot("Condition")]
   public partial class TriggerConditionsContainer
   {
      private object child = new TriggersTriggerAnd();//new object;


      public void CopyTo(TriggerConditionsContainer other)
      {
         if (Child != null)
         {

            TriggersTriggerOR or = Child as TriggersTriggerOR;
            if (or != null)
            {
               TriggersTriggerOR newOr = new TriggersTriggerOR();
               or.CopyTo(newOr);
               other.Child = (newOr);
            }
            TriggersTriggerAnd and = Child as TriggersTriggerAnd;
            if (and != null)
            {
               TriggersTriggerAnd newAnd = new TriggersTriggerAnd();
               and.CopyTo(newAnd);
               other.Child = (newAnd);

            }
            TriggerCondition cond = Child as TriggerCondition;
            if (cond != null)
            {
               TriggerCondition newCond = new TriggerCondition();
               cond.CopyTo(newCond);
               other.Child = (newCond);
            }

            
         }
      }


      /// <remarks/>
      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTriggerOR), ElementName = "Or"),
       XmlElement(Type = typeof(TriggersTriggerAnd), ElementName = "And"),
       XmlElement(Type = typeof(TriggerCondition), ElementName = "Condition")]

      public object Child
      {
         get
         {
            return this.child;
         }
         set
         {
            this.child = value;
         }
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRoot("Or")]
   public partial class TriggersTriggerOR
   {

      private List<object> children = new List<object>();


  

      /// <remarks/>
      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTriggerOR), ElementName = "Or"),
       XmlElement(Type = typeof(TriggersTriggerAnd), ElementName = "And"),
       XmlElement(Type = typeof(TriggerCondition), ElementName = "Condition")]
      public List<object> Children
      {
         get
         {
            return this.children;
         }
         set
         {
            this.children = value;
         }
      }

      public void CopyTo(TriggersTriggerOR other)
      {

         foreach (object o in Children)
         {
            TriggersTriggerOR or = o as TriggersTriggerOR;
            if (or != null)
            {
               TriggersTriggerOR newOr = new TriggersTriggerOR();
               or.CopyTo(newOr);
               other.children.Add(newOr);
            }
            TriggersTriggerAnd and = o as TriggersTriggerAnd;
            if (and != null)
            {
               TriggersTriggerAnd newAnd = new TriggersTriggerAnd();
               and.CopyTo(newAnd);
               other.children.Add(newAnd);

            }
            TriggerCondition cond = o as TriggerCondition;
            if (cond != null)
            {
               TriggerCondition newCond = new TriggerCondition();
               cond.CopyTo(newCond);
               other.children.Add(newCond);
            }

         }

      }

   }
   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRoot("And")]
   public partial class TriggersTriggerAnd
   {

      private List<object> children = new List<object>();

      /// <remarks/>
      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTriggerOR), ElementName = "Or"),
       XmlElement(Type = typeof(TriggersTriggerAnd), ElementName = "And"),
       XmlElement(Type = typeof(TriggerCondition), ElementName = "Condition")]
      public List<object> Children
      {
         get
         {
            return this.children;
         }
         set
         {
            this.children = value;
         }
      }


      public void CopyTo(TriggersTriggerAnd other)
      {

         foreach (object o in Children)
         {
            TriggersTriggerOR or = o as TriggersTriggerOR;
            if (or != null)
            {
               TriggersTriggerOR newOr = new TriggersTriggerOR();
               or.CopyTo(newOr);
               other.children.Add(newOr);
            }
            TriggersTriggerAnd and = o as TriggersTriggerAnd;
            if (and != null)
            {
               TriggersTriggerAnd newAnd = new TriggersTriggerAnd();
               and.CopyTo(newAnd);
               other.children.Add(newAnd);

            }
            TriggerCondition cond = o as TriggerCondition;
            if (cond != null)
            {
               TriggerCondition newCond = new TriggerCondition();
               cond.CopyTo(newCond);
               other.children.Add(newCond);
            }

         }

      }



   }

   public class TriggerComponent
   {
      protected List<TriggerVariable> parameterField = new List<TriggerVariable>();
      protected int idField;
      protected string typeField;
      private bool commentOut = false;

      [XmlIgnore]
      public string mLastSchema = null;

      public virtual void CopyTo(TriggerComponent other)
      {
         other.ID = this.ID;
         other.Type = this.Type;
         other.DBID = this.DBID;
         other.Version = this.Version;
         other.CommentOut = this.CommentOut;
         foreach (TriggerVariable v in parameterField)
         {
            TriggersInputVariable inVar = v as TriggersInputVariable;
            if (inVar != null)
            {
               TriggersInputVariable newVar = new TriggersInputVariable();
               inVar.CopyTo(newVar);
               other.Parameter.Add(newVar);
            }
            TriggersOutputVariable outVar = v as TriggersOutputVariable;
            if (outVar != null)
            {
               TriggersOutputVariable newVar = new TriggersOutputVariable();
               outVar.CopyTo(newVar);
               other.Parameter.Add(newVar);
            }
         }
      }

      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersInputVariable), ElementName = "Input"),
      XmlElement(Type = typeof(TriggersOutputVariable), ElementName = "Output")]
      public List<TriggerVariable> Parameter
      {
         get
         {
            return this.parameterField;
         }
         set
         {
            this.parameterField = value;
         }
      }


      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Type
      {
         get
         {
            return this.typeField;
         }
         set
         {
            this.typeField = value;
         }
      }

      int mDBID = -1;
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int DBID
      {
         get
         {
            return this.mDBID;
         }
         set
         {
            this.mDBID = value;
         }
      }
      int mVer = -1;
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int Version
      {
         get
         {
            return this.mVer;
         }
         set
         {
            this.mVer = value;
         }
      }

      List<TriggerSystemDebugInfo> mDebugInfo = new List<TriggerSystemDebugInfo>();
      [Browsable(false)]
      [XmlIgnore]
      public List<TriggerSystemDebugInfo> DebugInfo
      {
         get
         {
            return mDebugInfo;
         }
         set
         {
            mDebugInfo = value;
         }

      }
      [XmlIgnore]
      public bool HasErrors
      {
         get
         {
            foreach(TriggerSystemDebugInfo d in mDebugInfo)
            {
               if (d.mLevel == TriggerSystemDebugLevel.Error)
                  return true;
            }
            return false;
         }
      }
      [XmlIgnore]
      public bool HasWarnings
      {
         get
         {
            foreach (TriggerSystemDebugInfo d in mDebugInfo)
            {
               if (d.mLevel == TriggerSystemDebugLevel.Warning)
                  return true;
            }
            return false;
         }
      }

      [XmlIgnore]
      public bool NeedsUpgrade
      {
         get
         {
            foreach (TriggerSystemDebugInfo d in mDebugInfo)
            {
               if (d.mType == TriggerSystemDebugType.OldVersion || d.mType == TriggerSystemDebugType.ObsoleteVersion)
                  return true;
            }
            return false;
         }
      }

      bool mbJustUpgraded = false;
      [XmlIgnore]
      public bool JustUpgraded
      {
         get
         {
            return mbJustUpgraded;
         }
         set
         {
            mbJustUpgraded = value;
         }

      }


      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool CommentOut
      {
         get
         {
            return this.commentOut;
         }
         set
         {
            this.commentOut = value;
         }
      }
   }



   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [XmlRoot("Condition")]
   public partial class TriggerCondition : TriggerComponent
   {
      bool mInvert = false;
      [XmlAttribute("Invert")]
      public bool Invert
      {
         get
         {
            return mInvert;
         }
         set
         {
            mInvert = value;
         }
      }



      bool mAsync = false;
      [XmlAttribute("Async")]
      public bool Async
      {
         get
         {
            return mAsync;
         }
         set
         {
            mAsync = value;
         }
      }
      int mAsyncParameterKey = 0;
      [XmlAttribute("AsyncParameterKey")]
      public int AsyncParameterKey
      {
         get
         {
            return mAsyncParameterKey;
         }
         set
         {
            mAsyncParameterKey = value;
         }
      }

      public override void CopyTo(TriggerComponent other)
      {
         base.CopyTo(other); //base copy
         TriggerCondition otherCond = other as TriggerCondition;
         if(otherCond != null)
         {
            otherCond.Async = this.Async;
            otherCond.AsyncParameterKey = this.AsyncParameterKey;
            otherCond.Invert = this.Invert;
         }
      }


      //private List<TriggerVariable> parameterField = new List<TriggerVariable>();

      //private int idField;

      //private string typeField;


      //public void CopyTo(TriggerCondition other)
      //{
      //   other.ID = this.ID;
      //   other.Type = this.Type;
      //   other.DBID = this.DBID;
      //   other.Ver = this.Ver;
      //   foreach (TriggerVariable v in parameterField)
      //   {
      //      TriggersInputVariable inVar = v as TriggersInputVariable;
      //      if(inVar != null)
      //      {
      //         TriggersInputVariable newVar = new TriggersInputVariable();
      //         inVar.CopyTo(newVar);
      //         other.Parameter.Add(newVar);
      //      }
      //      TriggersOutputVariable outVar = v as TriggersOutputVariable;
      //      if (outVar != null)
      //      {
      //         TriggersOutputVariable newVar = new TriggersOutputVariable();
      //         outVar.CopyTo(newVar);
      //         other.Parameter.Add(newVar);
      //      }
      //   }        
      //}
 
      //[Browsable(false)]
      //[XmlElement(Type = typeof(TriggersInputVariable), ElementName = "Input"),
      //XmlElement(Type = typeof(TriggersOutputVariable), ElementName = "Output")]
      //public List<TriggerVariable> Parameter
      //{
      //   get
      //   {
      //      return this.parameterField;
      //   }
      //   set
      //   {
      //      this.parameterField = value;
      //   }
      //}


      ///// <remarks/>
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int ID
      //{
      //   get
      //   {
      //      return this.idField;
      //   }
      //   set
      //   {
      //      this.idField = value;
      //   }
      //}

      ///// <remarks/>
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public string Type
      //{
      //   get
      //   {
      //      return this.typeField;
      //   }
      //   set
      //   {
      //      this.typeField = value;
      //   }
      //}

      //int mDBID = -1;
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int DBID
      //{
      //   get
      //   {
      //      return this.mDBID;
      //   }
      //   set
      //   {
      //      this.mDBID = value;
      //   }
      //}
      //int mVer = -1;
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int Ver
      //{
      //   get
      //   {
      //      return this.mVer;
      //   }
      //   set
      //   {
      //      this.mVer = value;
      //   }
      //}
   }

   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class TriggerVariable : INamedVariable
   {
      private string nameField;

      private int idField = -1;

      [XmlIgnore]
      public bool mbIsDynamicVar = false;

      public void CopyTo(TriggerVariable v)
      {
         v.Name = this.Name;
         v.ID = this.ID;
         v.SigID = this.SigID;
         v.Optional = this.Optional;
      }
      virtual public TriggerVariable GetCopy()         
      {
         TriggerVariable v = new TriggerVariable();
         CopyTo(v);
         return v;
      }

      public string GetName()
      {
         return Name;
      }

      public int GetID()
      {
         return ID;
      }

      public void SetID(int id)
      {
         ID = id;
      }

      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlTextAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      int mSigID = -1;
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int SigID
      {
         get
         {
            return this.mSigID;
         }
         set
         {
            this.mSigID = value;
         }
      }

      bool mOptional = false;
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool Optional
      {
         get
         {
            return mOptional;
         }
         set
         {
            mOptional = value;
         }
      }

   }
   public class TriggersOutputVariable  : TriggerVariable
   {
      public override TriggerVariable GetCopy()
      {
         TriggerVariable v = new TriggersOutputVariable();
         CopyTo(v);
         return v;
      }

   }
   public class TriggersInputVariable : TriggerVariable
   {
      public override TriggerVariable GetCopy()
      {
         TriggerVariable v = new TriggersInputVariable();
         CopyTo(v);
         return v;
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class TriggerEffectsContainer
   {

      private List<TriggerEffect> effectsField = new List<TriggerEffect>();

      /// <remarks/>
      [Browsable(false)]
      [System.Xml.Serialization.XmlElementAttribute("Effect")]
      public List<TriggerEffect> Effects
      {
         get
         {
            return this.effectsField;
         }
         set
         {
            this.effectsField = value;
         }
      }

      public void CopyTo(TriggerEffectsContainer other)
      {
         foreach(TriggerEffect e in Effects)
         {
            TriggerEffect newE = new TriggerEffect();
            e.CopyTo(newE);
            other.Effects.Add(newE);
         }

      }



   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [XmlRoot("Effect")]
   public partial class TriggerEffect : TriggerComponent
   {

      public override void CopyTo(TriggerComponent other)
      {
         base.CopyTo(other); //base copy
         TriggerEffect otherEffect = other as TriggerEffect;
         if (otherEffect != null)
         {
            //insert specific copy code here.

         }
      }


      //private List<TriggerVariable> parameterField = new List<TriggerVariable>();

      //private int idField;

      //private string typeField;


      //public void CopyTo(TriggerEffect other)
      //{
      //   other.ID = this.ID;
      //   other.Type = this.Type;
      //   other.DBID = this.DBID;
      //   other.Ver = this.Ver;
      //   foreach(TriggerVariable v in parameterField)
      //   {
      //      TriggersInputVariable inVar = v as TriggersInputVariable;
      //      if (inVar != null)
      //      {
      //         TriggersInputVariable newVar = new TriggersInputVariable();
      //         inVar.CopyTo(newVar);
      //         other.Parameter.Add(newVar);
      //      }
      //      TriggersOutputVariable outVar = v as TriggersOutputVariable;
      //      if (outVar != null)
      //      {
      //         TriggersOutputVariable newVar = new TriggersOutputVariable();
      //         outVar.CopyTo(newVar);
      //         other.Parameter.Add(newVar);
      //      }
      //   }

      //}

      //[Browsable(false)]
      //[XmlElement(Type = typeof(TriggersInputVariable), ElementName = "Input"),
      //XmlElement(Type = typeof(TriggersOutputVariable), ElementName = "Output")]
      //public List<TriggerVariable> Parameter
      //{
      //   get
      //   {
      //      return this.parameterField;
      //   }
      //   set
      //   {
      //      this.parameterField = value;
      //   }
      //}


      ///// <remarks/>
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int ID
      //{
      //   get
      //   {
      //      return this.idField;
      //   }
      //   set
      //   {
      //      this.idField = value;
      //   }
      //}

      ///// <remarks/>
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public string Type
      //{
      //   get
      //   {
      //      return this.typeField;
      //   }
      //   set
      //   {
      //      this.typeField = value;
      //   }
      //}


      //int mDBID = -1;
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int DBID
      //{
      //   get
      //   {
      //      return this.mDBID;
      //   }
      //   set
      //   {
      //      this.mDBID = value;
      //   }
      //}
      //int mVer = -1;
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public int Ver
      //{
      //   get
      //   {
      //      return this.mVer;
      //   }
      //   set
      //   {
      //      this.mVer = value;
      //   }
      //}
   }

   ///// <remarks/>
   //[System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   //[System.SerializableAttribute()]
   //[System.Diagnostics.DebuggerStepThroughAttribute()]
   //[System.ComponentModel.DesignerCategoryAttribute("code")]
   //[System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   //[XmlRoot("Parameter")]
   //public partial class TriggerEffectParameter : INamedTypedProperty
   //{

   //   private string nameField;

   //   private string typeField;

   //   private string valueField;


   //   //INamedTypedProperty
   //   public string GetName()
   //   {
   //      return Name;
   //   }
   //   public string GetTypeName()
   //   {
   //      return Type;
   //   }
   //   public object GetValue()
   //   {
   //      return Value;
   //   }
   //   public void SetValue(object val)
   //   {
   //      Value = val.ToString();
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
   //   /// <remarks/>
   //   [ReadOnly(true)]
   //   [System.Xml.Serialization.XmlAttributeAttribute()]
   //   public string Name
   //   {
   //      get
   //      {
   //         return this.nameField;
   //      }
   //      set
   //      {
   //         this.nameField = value;
   //      }
   //   }

   //   /// <remarks/>
   //   [ReadOnly(true)] 
   //   [System.Xml.Serialization.XmlAttributeAttribute()]
   //   public string Type
   //   {
   //      get
   //      {
   //         return this.typeField;
   //      }
   //      set
   //      {
   //         this.typeField = value;
   //      }
   //   }

   //   /// <remarks/>
   //   [System.Xml.Serialization.XmlTextAttribute()]
   //   public string Value
   //   {
   //      get
   //      {
   //         return this.valueField;
   //      }
   //      set
   //      {
   //         this.valueField = value;
   //      }
   //   }
   //}


   /////////Trigger Vars


   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   //[System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   [XmlRoot("TriggerVars")]
   public partial class TriggerVars
   {
      private List<TriggerValue> triggerVarField = new List<TriggerValue>();

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("TriggerVar")]
      public List<TriggerValue> TriggerVar
      {
         get
         {
            return this.triggerVarField;
         }
         set
         {
            this.triggerVarField = value;
         }
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   //[System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [XmlRoot("TriggerVar")]
   public partial class TriggerValue : ITypedValue, INamedVariable
   {

      private int idField;

      private string typeField;

      private string valueField;

      private string nameField = "";

      

      public void CopyTo(TriggerValue v)
      {
         v.ID = this.ID;
         v.Type = this.Type;
         v.Value = this.Value;
         v.Name = this.Name;
         v.IsNull = this.IsNull;

      }
      public TriggerValue GetCopy()         
      {
         TriggerValue v = new TriggerValue();
         CopyTo(v);
         return v;
      }

      public string GetTypeName()
      {
         return Type;
      }
      public object GetValue()
      {
         return Value;
      }
      public void SetValue(object val)
      {
         Value = val.ToString();
      }

      public int GetID()
      {
         return ID;
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Type
      {
         get
         { /* marchack */
            if (this.typeField == "Count")
               return ("Integer");
            else if (this.typeField == "Hitpoints")
               return ("Float");
            else if (this.typeField == "Distance")
               return ("Float");
            else if (this.typeField == "Percent")
               return ("Float");
            else if (this.typeField == "Location")
               return ("Vector");
            else if (this.typeField == "Direction")
               return ("Vector");
            else if (this.typeField == "LocationList")
               return ("VectorList");
            else
               return this.typeField;
         }
         set
         { /* marchack */
            if (value == "Count")
               this.typeField = "Integer";
            else if (value == "Hitpoints")
               this.typeField = "Float";
            else if (value == "Distance")
               this.typeField = "Float";
            else if (value == "Percent")
               this.typeField = "Float";
            else if (value == "Location")
               this.typeField = "Vector";
            else if (value == "Direction")
               this.typeField = "Vector";
            else if (value == "LocationList")
               this.typeField = "VectorList";
            else
               this.typeField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlTextAttribute()]
      public string Value
      {
         get
         {
            return this.valueField;
         }
         set
         {
            this.valueField = value;
         }
      }

      //[System.Xml.Serialization.XmlTextAttribute()]
      [System.Xml.Serialization.XmlAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      bool mIsNull = false;
      [System.Xml.Serialization.XmlAttribute()]
      public bool IsNull
      {
         set
         {
            mIsNull = value;
         }
         get
         {
            return mIsNull;
         }


      }

      #region INamedVariable Members

      public string GetName()
      {
         //throw new Exception("The method or operation is not implemented.");
         return this.Name;
      }

      public void SetID(int id)
      {
         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion
   }

   ////////////////////Templates

   public class TriggerTemplateMapping
   {
      //public TriggerTemplateInstance(TriggerTemplateDefinition parent)
      //{
      //   mDefinition = parent;
      //}
      //public TriggerTemplateDefinition mDefinition;


      //namespace contained by it
      //TriggerNamespace mContainedNamespace;

      //public void LoadFromFile(string filename);
      //Dictionary<int,int> mValueMapping

      //<input id = "0" name="count">10<>

      //TriggerTemplateInstance createNewInstance()
      //{
      //   TriggerTemplateInstance instance = new TriggerTemplateInstance(this);
      //   return instance;
      //}
      private int idField;
      private string mName;

      private int x = -1;
      private int y = -1;

      private int mSizeX = 150;
      private int mSizeY = 150;

      private int groupID = -1;

      private string mImage = "";

      private bool commentOut = false;

      private List<TriggersTemplateInputVariable> mInputMappings = new List<TriggersTemplateInputVariable>();
      private List<TriggersTemplateOutputVariable> mOutputMappings = new List<TriggersTemplateOutputVariable>();

      private List<TriggersTemplateInputActionBinder> mTriggerInputs = new List<TriggersTemplateInputActionBinder>();
      private List<TriggersTemplateOutputActionBinder> mTriggerOutputs = new List<TriggersTemplateOutputActionBinder>();


      public Dictionary<int, bool> GetInitializedVars()
      {
         Dictionary<int, bool> map = new Dictionary<int, bool>();

         foreach (TriggersTemplateInputVariable v in mInputMappings)
         {
            map[v.BindID] = true;
         }
         return map;
      }
      public Dictionary<int, bool> GetActivatedTriggers()
      {
         Dictionary<int, bool> map = new Dictionary<int, bool>();

         foreach (TriggersTemplateInputActionBinder v in mTriggerInputs)
         {
            map[v.BindID.ID] = true;
         }
         return map;
      }


      public void FinalizeForSave()
      {
         foreach(TriggersTemplateInputVariable v in mInputMappings)
         {
            v.DeInitID();
         }
         foreach (TriggersTemplateOutputVariable v in mOutputMappings)
         {
            v.DeInitID();
         }


      }


      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTemplateInputVariable), ElementName = "InputMapping")]
      public List<TriggersTemplateInputVariable> InputMappings
      {
         get
         {
            return this.mInputMappings;
         }
         set
         {
            this.mInputMappings = value;
         }
      }

      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTemplateOutputVariable), ElementName = "OutputMapping")]
      public List<TriggersTemplateOutputVariable> OutputMappings
      {
         get
         {
            return this.mOutputMappings;
         }
         set
         {
            this.mOutputMappings = value;
         }
      }

      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTemplateInputActionBinder), ElementName = "TriggerInput")]
      public List<TriggersTemplateInputActionBinder> TriggerInputs
      {
         get
         {
            return this.mTriggerInputs;
         }
         set
         {
            this.mTriggerInputs = value;
         }
      }
      [Browsable(false)]
      [XmlElement(Type = typeof(TriggersTemplateOutputActionBinder), ElementName = "TriggerOutput")]
      public List<TriggersTemplateOutputActionBinder> TriggerOutputs
      {
         get
         {
            return this.mTriggerOutputs;
         }
         set
         {
            this.mTriggerOutputs = value;
         }
      }

      [XmlAttribute("Name")]
      public string Name
      {
         get
         {
            
            return mName;
         }
         set
         {
            mName = value;
         }
      }

      [XmlAttribute("Image")]
      public string Image
      {
         get
         {
            return mImage;
         }
         set
         {
            mImage = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int X
      {
         get
         {
            return this.x;
         }
         set
         {
            this.x = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int Y
      {
         get
         {
            return this.y;
         }
         set
         {
            this.y = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int SizeX
      {
         get
         {
            return this.mSizeX;
         }
         set
         {
            this.mSizeX = value;
         }
      }
      [System.Xml.Serialization.XmlAttribute]
      public int SizeY
      {
         get
         {
            return this.mSizeY;
         }
         set
         {
            this.mSizeY = value;
         }
      }

      [System.Xml.Serialization.XmlAttribute]
      public int GroupID
      {
         get
         {
            return this.groupID;
         }
         set
         {
            this.groupID = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool CommentOut
      {
         get
         {
            return this.commentOut;
         }
         set
         {
            this.commentOut = value;
         }
      }





      bool mObsolete = false;
      [XmlAttribute("Obsolete")]
      public bool Obsolete
      {
         get
         {
            return mObsolete;
         }
         set
         {
            mObsolete = value;
         }
      }

      bool mDoNotUse = false;
      [XmlAttribute("DoNotUse")]
      public bool DoNotUse
      {
         get
         {
            return mDoNotUse;
         }
         set
         {
            mDoNotUse = value;
         }
      }

   }


   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [XmlRoot("TriggerTemplateDefinition")]
   public class TriggerTemplateDefinition
   {

      private TriggerRoot mTriggerSystemRoot = new TriggerRoot();
      private TriggerTemplateMapping mTriggerTemplateMapping = new TriggerTemplateMapping();

      [Browsable(false)]
      [System.Xml.Serialization.XmlElementAttribute("TriggerSystem")]
      public TriggerRoot TriggerSystemRoot
      {
         get
         {
            return this.mTriggerSystemRoot;
         }
         set
         {
            this.mTriggerSystemRoot = value;
         }
      }

      [Browsable(false)]
      [System.Xml.Serialization.XmlElementAttribute("TemplateMapping")]
      public TriggerTemplateMapping TriggerTemplateMapping
      {
         get
         {
            return this.mTriggerTemplateMapping;
         }
         set
         {
            this.mTriggerTemplateMapping = value;
         }
      }

   }

   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class TriggersTemplateVariableBinder : TriggerVariable//: INamedVariable
   {
      //private string nameField;
      private string typeField;
      private int bindIDField = -1;
      //private int targetIDField;


      public TriggersTemplateVariableBinder()
      {
         DeInitID();
      }

      public void DeInitID()
      {
         ID = -1;


      }

      //public void CopyTo(TriggerVariable v)
      //{
      //   v.Name = this.Name;
      //   v.ID = this.ID;
      //}
      //virtual public TriggerVariable GetCopy()
      //{
      //   TriggerVariable v = new TriggerVariable();
      //   CopyTo(v);
      //   return v;
      //}

      //public string GetName()
      //{
      //   return Name;
      //}

      //public int GetID()
      //{
      //   return ID;
      //}

      /// <remarks/>
      //[ReadOnly(true)]
      //[System.Xml.Serialization.XmlAttributeAttribute()]
      //public string Name
      //{
      //   get
      //   {
      //      return this.nameField;
      //   }
      //   set
      //   {
      //      this.nameField = value;
      //   }
      //}

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Type
      {
         get
         {
            return this.typeField;
         }
         set
         {
            this.typeField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int BindID
      {
         get
         {
            return this.bindIDField;
         }
         set
         {
            this.bindIDField = value;
         }
      }

      ///// <remarks/>
      //[System.Xml.Serialization.XmlTextAttribute()]
      //public int TargetID
      //{
      //   get
      //   {
      //      return this.targetIDField;
      //   }
      //   set
      //   {
      //      this.targetIDField = value;
      //   }
      //}



   }


   public class TriggersTemplateInputVariable : TriggersTemplateVariableBinder
   {
      public override TriggerVariable GetCopy()
      {
         TriggerVariable v = new TriggersTemplateInputVariable();
         CopyTo(v);
         return v;
      }
   }
   public class TriggersTemplateOutputVariable : TriggersTemplateVariableBinder
   {
      public override TriggerVariable GetCopy()
      {
         TriggerVariable v = new TriggersTemplateOutputVariable();
         CopyTo(v);
         return v;
      }
   }



   
   public class TriggerBindInfo 
   {
      private int mID;
      private string mLinkName;

      private string mLine = "";

      private bool mbHasTarget = false;
      public bool HasTarget()
      {
         return mbHasTarget;
      }
      public TriggerBindInfo()
      {
         mbHasTarget = false;
      }   
      public void FromString(string fromString)
      {
         if(fromString == "")
         {
            mbHasTarget = false;
            return;
         }
         try
         {
            string[] input = fromString.Split(',');
            mID = System.Convert.ToInt32(input[0]);
            mLinkName = input[1];
            mbHasTarget = true;
         }
         catch(System.Exception ex)
         {            
            mbHasTarget = false;
         }
      }
      public void SetTarget(int id, string linkName)
      {
         mID = id;
         mLinkName = linkName;
         mbHasTarget = true;

      }
      public int ID
      {
         get
         {
            return mID;            
         }
      }
      public string LinkName
      {
         get
         {
            return mLinkName;
         }
      }
      public bool IsLinkToTemplate()
      {
         if (mLinkName == "Activate")   return false;
         if (mLinkName == "Deactivate") return false;
         if (mLinkName == "Effect.False") return false;
         if (mLinkName == "Effect.True") return false;

         return true;

      }
      public override string ToString()
      {
         if (HasTarget() == false)
            return "";
         string ret =  string.Format("{0},{1}",  mID, mLinkName);
                  
         return ret;
      }
      public void CopyTo(TriggerBindInfo other)
      {
         other.FromString(this.ToString());
      }
   }


   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public class TriggersTemplateActionBinder //: INamedVariable
   {
      private string nameField;

   

      private TriggerBindInfo mTarget = new TriggerBindInfo();
      private TriggerBindInfo mBindTo = new TriggerBindInfo();
      private List<TriggerBindInfo> mTargetList = new List<TriggerBindInfo>();

      /// <remarks/>
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }
      [XmlIgnore]
      public TriggerBindInfo BindID
      {
         get
         {
            return mBindTo;
         }  
      }
      [XmlIgnore] 
      public List<TriggerBindInfo> TargetIDs
      {
         get
         {
            return mTargetList;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute("BindID")]
      public string _BindID
      {
         get
         {
            return mBindTo.ToString();
         }
         set
         {
            mBindTo.FromString(value);
         }
      }

      /// <remarks/>
     
      [System.Xml.Serialization.XmlTextAttribute()]
      public string _TargetID
      {
         get
         {
            //return mTarget.ToString() ;
            string outstring = "";
            foreach(TriggerBindInfo t in mTargetList )
            {
               outstring += (t.ToString() + ";" );
            }
            return outstring;
      
         }
         set
         {
            //mTarget.FromString(value);
            mTargetList.Clear();
            string[] instring = value.Split(';');
            foreach (string s in instring)
            {
               if(s != "")
               {
                  TriggerBindInfo t = new TriggerBindInfo();
                  t.FromString(s);
                  mTargetList.Add(t);
               }
            }
         }
      }

      string baseColor;
      [ReadOnly(true)]
      [System.Xml.Serialization.XmlAttributeAttribute("Color")]
      public string Color
      {
         get
         {
            return this.baseColor;
         }
         set
         {
            this.baseColor = value;

            string[] values = baseColor.Split(',');
            mBaseColor = System.Drawing.Color.FromArgb(System.Convert.ToInt32(values[0]) , System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
         }
      }

      Color mBaseColor = System.Drawing.Color.Blue;
      public Color GetColor()
      {        
         return mBaseColor;
      }
      [XmlIgnore]
      public Color _Color
      {
         get
         {
            return mBaseColor;
         }
         set
         {
            Color = string.Format("{0},{1},{2}", value.R, value.G, value.B);
         }
      }

   }

   // Target -> BindID
   public class TriggersTemplateInputActionBinder : TriggersTemplateActionBinder
   {

   }
   //BindID -> Target
   public class TriggersTemplateOutputActionBinder : TriggersTemplateActionBinder
   {

   }

#region TriggerDebugInfo


   public enum TriggerSystemDebugLevel
   {
      Info,
      Warning,
      Error
   }
   public enum TriggerSystemDebugType
   {
      None,
      //components
      InvalidParameters,
      ObsoleteVersion,
      OldVersion,
      DoNotUse,
      InvalidArgument,
      MissingDefinition,
      //variables
      UnassignedVariable,
      MissingValue,
      WriteToConstant,
      //Values
      ValueError,
      //Triggers
      NeverActiveTrigger,
      //General
      GeneralError


   }
   public enum TriggerSystemDebugSubject
   {
      None,
      Effect,
      Condition,
      Value,
      Variable,
      Template,
      Trigger
   }

   public class TriggerSystemDebugInfo
   {
      public TriggerSystemDebugType mType;
      public string mErrorText = "";
      public TriggerComponent mErrorComponent = null;
      public TriggerSystemDebugLevel mLevel;
      public object mArgs = null;
      public object mSubject = null;

      public TriggerSystemDebugInfo(TriggerSystemDebugLevel level, TriggerSystemDebugType type, object args, string errortext, TriggerComponent errorComponent)
      {
         mLevel = level;
         mType = type;
         mArgs = args;
         mErrorText = errortext;
         mErrorComponent = errorComponent;
         mSubject = errorComponent;
      }

      public TriggerSystemDebugInfo(TriggerSystemDebugLevel level, TriggerSystemDebugType type, object args, string errortext, object subject)
      {
         mLevel = level;
         mType = type;
         mArgs = args;
         mErrorText = errortext;
         mSubject = subject;
      }

      public string GetComponentString()
      {
         if (mErrorComponent != null)
         {
            return string.Format("{0}",mErrorComponent.Type);
         }
         else if (mSubject is TriggerVariable)
         {

            return string.Format("{0}", ((TriggerVariable)mSubject).Name);
         }
         else if (mSubject is TriggerValue)
         {

            return string.Format("{0}", ((TriggerValue)mSubject).Name);
         }
         else if (mSubject is Trigger)
         {
            return string.Format("{0}", ((Trigger)mSubject).Name);
         }
         else if (mSubject is TriggerTemplateMapping)
         {
            return string.Format("{0}", ((TriggerTemplateMapping)mSubject).Name);
         }
         else
         {
            return "";// string.Format("{0}", mErrorText);
         } 
      }


      public override string ToString()
      {
         if (mErrorComponent != null)
         {
            return string.Format("{0} {1} {2}", mLevel.ToString(), mErrorText, mErrorComponent.Type);
         }
         else if (mSubject is TriggerVariable)
         {

            return string.Format("{0} {1} {2}", mLevel.ToString(), mErrorText, ((TriggerVariable)mSubject).Name);
         }
         else if (mSubject is TriggerValue)
         {

            return string.Format("{0} {1} {2}", mLevel.ToString(), mErrorText, ((TriggerValue)mSubject).Name);
         }
         else if (mSubject is Trigger)
         {
            return string.Format("{0} {1} {2}", mLevel.ToString(), mErrorText, ((Trigger)mSubject).Name);
         }
         else if (mSubject is TriggerTemplateMapping)
         {
            return string.Format("{0} {1} {2}", mLevel.ToString(), mErrorText, ((TriggerTemplateMapping)mSubject).Name);
         }
         else 
         {
            return string.Format("{0} {1}", mLevel.ToString(), mErrorText);
         }

         //return base.ToString();
      }
   }


#endregion





   //////////////Armies

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   public partial class TriggerArmiesRoot
   {

      private List<TriggerArmy> trigArmyField = new List<TriggerArmy>();

      private int nextIDField;

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("TrigArmy")]
      public List<TriggerArmy> TrigArmy
      {
         get
         {
            return this.trigArmyField;
         }
         set
         {
            this.trigArmyField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int NextID
      {
         get
         {
            return this.nextIDField;
         }
         set
         {
            this.nextIDField = value;
         }
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class TriggerArmy
   {
      public TriggerArmy() { }

      private List<int> objectField = new List<int>();

      private int idField;

      private string nameField = "";


      public override string ToString()
      {
         string text = Name;
         if (Name == "")
         {
            text = "unnamed";
         }


         return text + " : " + objectField.Count;
         //return base.ToString();
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("Object")]
      public List<int> Object
      {
         get
         {
            return this.objectField;
         }
         set
         {
            this.objectField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int ID
      {
         get
         {
            return this.idField;
         }
         set
         {
            this.idField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }
   }


  



}