using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;


namespace DependencyUtils
{
   public abstract class XMLVisualNode
   {
      public abstract bool isNodeValid();
      public abstract bool isBranchValid();
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   public partial class visual : XMLVisualNode
   {

      private List<visualModel> modelField = new List<visualModel>();

      private string defaultmodelField = "Default";

      private visualLogic logicField;

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("model")]
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public List<visualModel> model
      {
         get { return this.modelField; }
         set { this.modelField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string defaultmodel
      {
         get { return this.defaultmodelField; }
         set { this.defaultmodelField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualLogic logic
      {
         get { return this.logicField; }
         set { this.logicField = value; }
      }


      public override bool isNodeValid()
      {
         if (modelField.Count == 0)
            return false;

         // Check name
         bool foundDefaultModel = false;
         foreach (visualModel curModel in modelField)
         {
            if (String.Compare(defaultmodelField, curModel.name, true) == 0)
            {
               foundDefaultModel = true;
               break;
            }
         }

         if (!foundDefaultModel)
         {
            return false;
         }

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check all models
         foreach (visualModel curModel in modelField)
         {
            if (!curModel.isBranchValid())
               return (false);
         }

         // check logic if any
         if (logicField != null)
         {
            if (!logicField.isBranchValid())
               return (false);
         }

         return (true);
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModel : XMLVisualNode
   {
      private visualModelComponent componentField = new visualModelComponent();

      private List<visualModelAnim> animField = new List<visualModelAnim>();

      private string textField = "Default";

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.textField; }
         set { this.textField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualModelComponent component
      {
         get { return this.componentField; }
         set { this.componentField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      [System.Xml.Serialization.XmlElementAttribute("anim")]
      public List<visualModelAnim> anim
      {
         get { return this.animField; }
         set { this.animField = value; }
      }

      public override bool isNodeValid()
      {
         if (componentField == null)
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check component
         if (!componentField.isBranchValid())
            return (false);

         // check all anims
         foreach (visualModelAnim curAnim in animField)
         {
            if (!curAnim.isBranchValid())
               return (false);
         }

         return (true);
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelComponent : XMLVisualNode
   {
      private visualModelComponentAsset assetField;

      private visualLogic logicField; 
      
      private List<visualModelComponentOrAnimAttach> attachField = new List<visualModelComponentOrAnimAttach>();

      private List<visualModelComponentPoint> pointField = new List<visualModelComponentPoint>();

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualLogic logic
      {
         get { return this.logicField; }
         set { this.logicField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualModelComponentAsset asset
      {
         get { return this.assetField; }
         set { this.assetField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("attach")]
      public List<visualModelComponentOrAnimAttach> attach
      {
         get { return this.attachField; }
         set { this.attachField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("point")]
      public List<visualModelComponentPoint> point
      {
         get { return this.pointField; }
         set { this.pointField = value; }
      }


      public override bool isNodeValid()
      {
         // Must have an asset or logic
         if ((assetField == null) && (logicField == null))
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check component asset or logic
         if (assetField != null)
         {
            if (!assetField.isBranchValid())
            {
               return (false);
            }
         }
         else if (logicField != null)
         {
            if (!logicField.isBranchValid())
            {
               return (false);
            }
         }

         // check all attachments
         foreach (visualModelComponentOrAnimAttach curAttach in attachField)
         {
            if (!curAttach.isBranchValid())
               return (false);
         }

         // check all points
         foreach (visualModelComponentPoint curPoint in pointField)
         {
            if (!curPoint.isBranchValid())
               return (false);
         }

         return (true);
      }

   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelAnim : XMLVisualNode
   {
      public enum AnimType
      {
         None = 0,
         Idle,
         Walk,
         Jog,
         Run,
         RangedAttack,
         Attack,
         Limber,
         Unlimber,
         Death,
         Gather,
         Research,
         Train,
         Bored,
         Incoming,
         Landing,
         Takeoff,
         Outgoing
      }

      public enum AnimExitAction
      {
         Loop = 0,
         Freeze,
         Transition
      }

      private string animName;

      private AnimExitAction exitActionField = AnimExitAction.Loop;

      private int tweenTimeField = 0;

      private string tweenToAnimationField = "";

      private List<visualModelAnimAsset> assetField = new List<visualModelAnimAsset>();

      private List<visualModelComponentOrAnimAttach> attachField = new List<visualModelComponentOrAnimAttach>();

      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string type
      {
         get { return this.animName; }
         set { this.animName = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public AnimExitAction exitAction
      {
         get { return this.exitActionField; }
         set { this.exitActionField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public int tweenTime
      {
         get { return this.tweenTimeField; }
         set { this.tweenTimeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tweenToAnimation
      {
         get { return this.tweenToAnimationField; }
         set { this.tweenToAnimationField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("asset")]
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public List<visualModelAnimAsset> asset
      {
         get { return this.assetField; }
         set { this.assetField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("attach")]
      public List<visualModelComponentOrAnimAttach> attach
      {
         get { return this.attachField; }
         set { this.attachField = value; }
      }



      public override bool isNodeValid()
      {
         // Must have an asset
         if (assetField.Count == 0)
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check all anims assets
         foreach (visualModelAnimAsset curAnimAsset in assetField)
         {
            if (!curAnimAsset.isBranchValid())
               return (false);
         }

         // check all attachments
         foreach (visualModelComponentOrAnimAttach curAttach in attachField)
         {
            if (!curAttach.isBranchValid())
               return (false);
         }

         return (true);
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelComponentAsset : XMLVisualNode
   {
      public enum ComponentAssetType
      {
         Model = 0,
         Particle,
         Light
      }

      private ComponentAssetType typeField;

      private string fileField;

      private string damagefileField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ComponentAssetType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /// <remarks/>
      public string file
      {
         get { return this.fileField; }
         set { this.fileField = value; }
      }

      /// <remarks/>
      public string damagefile
      {
         get { return this.damagefileField; }
         set { this.damagefileField = value; }
      }

      public override bool isNodeValid()
      {
         // Must have an asset
         switch (typeField)
         {
            case ComponentAssetType.Model:
            case ComponentAssetType.Particle:
            case ComponentAssetType.Light:
               if (String.IsNullOrEmpty(fileField))
                  return false;
               break;
         }

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node only since this is leaf node
         return (isNodeValid());
      }
      
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelComponentPoint : XMLVisualNode
   {
      public enum ComponentPointType
      {
         Impact = 0,
         Launch,
         HitpointBar,
         Reflect,
         Cover,
         Carry,
         Pickup,
         Board
      }

      public enum ComponentPointData
      {
         Metal = 0,
      }

      private ComponentPointType pointTypeField;

      private string boneField;

      private ComponentPointData pointDataField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ComponentPointType pointType
      {
         get { return this.pointTypeField; }
         set { this.pointTypeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string bone
      {
         get { return this.boneField; }
         set { this.boneField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ComponentPointData pointData
      {
         get { return this.pointDataField; }
         set { this.pointDataField = value; }
      }

      public override bool isNodeValid()
      {
         // TODO:  Add validation here

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node only since this is leaf node
         return (isNodeValid());
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelAnimAsset : XMLVisualNode
   {
      public enum AnimAssetType
      {
         Anim = 0
      }

      private AnimAssetType typeField;

      private string fileField = "";

      private int weightField = 1;

      private List<visualModelAnimAssetTag> tagField = new List<visualModelAnimAssetTag>();

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public AnimAssetType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /// <remarks/>
      public string file
      {
         get { return this.fileField; }
         set { this.fileField = value; }
      }

      /// <remarks/>
      public int weight
      {
         get { return this.weightField; }
         set { this.weightField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      [System.Xml.Serialization.XmlElementAttribute("tag")]
      public List<visualModelAnimAssetTag> tag
      {
         get { return this.tagField; }
         set { this.tagField = value; }
      }


      public override bool isNodeValid()
      {
         if (String.IsNullOrEmpty(fileField))
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check all tags
         foreach (visualModelAnimAssetTag curTag in tagField)
         {
            if (!curTag.isBranchValid())
               return (false);
         }

         return (true);
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelComponentOrAnimAttach : XMLVisualNode
   {
      public enum AttachType
      {
         ModelFile = 0,
         ParticleFile,
         LightFile,
         ModelRef,
         TerrainEffect
      }

      private AttachType typeField;

      private string nameField;

      private string toboneField;

      private string fromboneField;

      private bool syncanimsField;

      private bool disregardOrientField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public AttachType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.nameField; }
         set { this.nameField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tobone
      {
         get { return this.toboneField; }
         set { this.toboneField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string frombone
      {
         get { return this.fromboneField; }
         set { this.fromboneField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool syncanims
      {
         get { return this.syncanimsField; }
         set { this.syncanimsField = value; }
      }
      
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool disregardorient
      {
         get { return this.disregardOrientField; }
         set { this.disregardOrientField = value; }
      }


      public override bool isNodeValid()
      {
         switch (typeField)
         {
            case AttachType.ModelFile:
            case AttachType.ModelRef:
            case AttachType.ParticleFile:
            case AttachType.LightFile:
               if (String.IsNullOrEmpty(nameField))
                  return false;
               break;
         }

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node only since this is leaf node
         return (isNodeValid());
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualModelAnimAssetTag : XMLVisualNode
   {
      public enum TagType
      {
         Attack = 0,
         Sound,
         Particle,
         TerrainEffect,
         Light,
         CameraShake,
         GroundIK,
         AttachTarget,
         SweetSpot,
         TerrainAlpha,
         Rumble,
         BuildingDecal,
         UVOffset,
         KillAndThrow,
         PhysicsImpulse,
      }

      private TagType typeField;

      /*
      private decimal positionField = 0.0M;
      */

      private string nameField;

      /*
      private string toboneField; 
      
      private bool checkvisibleField;

      private bool disregardOrientField;

      private decimal lifespanField = 0.25M;
      */
      
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public TagType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }

      /*
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public decimal position
      {
         get { return this.positionField; }
         set { this.positionField = value; }
      }
      */

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string name
      {
         get { return this.nameField; }
         set { this.nameField = value; }
      }

      /*
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string tobone
      {
         get { return this.toboneField; }
         set { this.toboneField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool checkvisible
      {
         get { return this.checkvisibleField; }
         set { this.checkvisibleField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool disregardorient
      {
         get { return this.disregardOrientField; }
         set { this.disregardOrientField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public decimal lifespan
      {
         get { return this.lifespanField; }
         set { this.lifespanField = value; }
      }
       */

      public override bool isNodeValid()
      {
         switch (typeField)
         {
            case TagType.Attack:
               break;
            case TagType.Particle:
               if (String.IsNullOrEmpty(nameField))
                  return false;
               break;
            case TagType.Sound:
               if (String.IsNullOrEmpty(nameField))
                  return false;
               break;
            case TagType.Light:
               if (String.IsNullOrEmpty(nameField))
                  return false;
               break;
            case TagType.CameraShake:
               break;
         }

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node only since this is leaf node
         return (isNodeValid());
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualLogic : XMLVisualNode
   {
      public enum LogicType
      {
         Variation = 0,
         BuildingCompletion,
         Tech,
         SquadMode,
         ImpactSize,
         Destruction
      }

      private LogicType typeField;

      private List<visualLogicData> dataField = new List<visualLogicData>();

      private int curSelectedItem = 0;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public LogicType type
      {
         get { return this.typeField; }
         set { this.typeField = value; }
      }
      
      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("logicdata")]
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public List<visualLogicData> logicdata
      {
         get { return this.dataField; }
         set { this.dataField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlIgnore]
      public int selectedItem
      {
         get { return this.curSelectedItem; }
         set { this.curSelectedItem = value; }
      }

      public override bool isNodeValid()
      {
         if (dataField.Count == 0)
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check all logic data
         foreach (visualLogicData curLogicData in dataField)
         {
            if (!curLogicData.isBranchValid())
               return (false);
         }

         return (true);
      }
   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class visualLogicData : XMLVisualNode
   {
      private string valueField = "";

      private visualModelComponentAsset assetField;

      private visualLogic logicField;

      private string modelRefField = "";

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string value
      {
         get { return this.valueField; }
         set { this.valueField = value; }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string modelref
      {
         get { return this.modelRefField; }
         set { this.modelRefField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualModelComponentAsset asset
      {
         get { return this.assetField; }
         set { this.assetField = value; }
      }

      /// <remarks/>
      [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
      public visualLogic logic
      {
         get { return this.logicField; }
         set { this.logicField = value; }
      }

      public override bool isNodeValid()
      {
         if ((assetField == null) && (logicField == null) && !string.IsNullOrEmpty(modelref))
            return true;

         if ((assetField == null) && (logicField == null))
            return false;

         if ((assetField != null) && (logicField != null))
            return false;

         return true;
      }

      public override bool isBranchValid()
      {
         // check this node
         if (!isNodeValid())
            return (false);

         // check asset
         if (assetField != null)
         {
            if (!assetField.isBranchValid())
               return (false);
         }
         else if (logicField != null)
         {
            if (!logicField.isBranchValid())
               return (false);
         }


         return (true);
      }
   }

}
