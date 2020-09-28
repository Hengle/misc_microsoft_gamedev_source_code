using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using System.Xml.Serialization;

namespace pxdb
{
   //==============================================================================
   // ProtoObjects
   //==============================================================================
   [XmlRoot("Objects")]
   public class ProtoObjects
   {
      [XmlElement("Object", typeof(ProtoObject))]
      public ProtoObjectList mList = new ProtoObjectList();
   }

   //==============================================================================
   // ProtoObject
   //==============================================================================
   public class ProtoObject
   {
      //---------------------------------------------------------------------------
      // Private members
      //---------------------------------------------------------------------------
      private int mID = -1;
      private string mName = @"";
      private bool mInProgress = false;
      private DataIDList mLinks = new DataIDList();
      private string mObjectClass = @"";
      private string mVisual = @"";
      private int mDisplayNameID = -1;
      private int mRolloverTextID = -1;
      private float mLOS = 0.0f;
      private float mHitpoints = 0.0f;
      private float mShieldPoints = 0.0f;
      private float mBuildPoints = 0.0f;
      private float mRepairPoints = 0.0f;
      private ProtoObjectCostList mCosts = new ProtoObjectCostList();
      private float mCostEscalation = 1.0f;
      private DataStringList mCostEscalationObjects = new DataStringList();
      private float mVelocity = 0.0f;
      private float mTurnRate = 0.0f;
      private string mMovementType = @"";
      private float mObstructionRadiusX = 0.0f;
      private float mObstructionRadiusZ = 0.0f;
      private float mPickRadius = 0.0f;
      private float mPickOffset = 0.0f;
      private string mPickPriority = @"";
      private string mSelectType = @"";
      private string mGotoType = @"";
      private float mSelectedRadiusX = 0.0f;
      private float mSelectedRadiusZ = 0.0f;
      private string mFormationType = @"Fixed";
      private string mPhysicsInfo = @"";
      private string mTactics = @"";
      private ProtoObjectColor mMinimapColor = new ProtoObjectColor();
      private ProtoObjectGatherLink mGatherLink = new ProtoObjectGatherLink();
      private int mGathererLimit = -1;
      private string mBlockMovementObject = @"";
      private float mFlightLevel = 10.0f;
      private float mLifespan = 0.0f;
      private float mBounty = 0.0f;
      private float mResourceAmount = 0.0f;
      private string mPlacementRules = @"";
      private string mTrainAnim = @"";
      private float mMaxProjectileHeight = 0.0f;
      private ProtoObjectPopList mPops = new ProtoObjectPopList();
      private ProtoObjectPopList mPopCapAdditions = new ProtoObjectPopList();
      private ProtoObjectCommandList mCommands = new ProtoObjectCommandList();
      private ProtoObjectTrainLimitList mTrainLimits = new ProtoObjectTrainLimitList();
      private DataStringList mContains = new DataStringList();
      private string mPower = @"";
      private string mPortraitIcon = @"";
      private string mMinimapIcon = @"";
      private ProtoObjectSoundList mSounds = new ProtoObjectSoundList();
      private DataStringList mObjectTypes = new DataStringList();
      private DataStringList mFlags = new DataStringList();

      //---------------------------------------------------------------------------
      // Properties
      //---------------------------------------------------------------------------
      [XmlAttribute("id")]
      public int ID
      {
         get { return mID; }
         set { mID = value; }
      }

      [XmlAttribute("name")]
      public string Name
      {
         get { return mName; }
         set { mName = value; }
      }

      [XmlElement("Link", typeof(DataID))]
      public DataIDList Links
      {
         get { return mLinks; }
         set { mLinks = value; }
      }

      [XmlElement("InProgress")]
      public bool InProgress
      {
         get { return mInProgress; }
         set { mInProgress = value; }
      }

      [XmlElement("ObjectClass")]
      public string ObjectClass
      {
         get { return mObjectClass; }
         set { mObjectClass = value; }
      }

      [XmlElement("Visual")]
      public string Visual
      {
         get { return mVisual; }
         set { mVisual = value; }
      }

      [XmlElement("TrainAnim")]
      public string TrainAnim
      {
         get { return mTrainAnim; }
         set { mTrainAnim = value; }
      }

      [XmlElement("PortraitIcon")]
      public string PortraitIcon
      {
         get { return mPortraitIcon; }
         set { mPortraitIcon = value; }
      }

      [XmlElement("MinimapIcon")]
      public string MinimapIcon
      {
         get { return mMinimapIcon; }
         set { mMinimapIcon = value; }
      }

      [XmlElement("MinimapColor", typeof(ProtoObjectColor))]
      public ProtoObjectColor MinimapColor
      {
         get { return mMinimapColor; }
         set { mMinimapColor = value; }
      }

      [XmlIgnore]
      public float MinimapColorRed
      {
         get { return mMinimapColor.Red; }
         set { mMinimapColor.Red = value; }
      }

      [XmlIgnore]
      public float MinimapColorBlue
      {
         get { return mMinimapColor.Blue; }
         set { mMinimapColor.Blue = value; }
      }

      [XmlIgnore]
      public float MinimapColorGreen
      {
         get { return mMinimapColor.Green; }
         set { mMinimapColor.Green = value; }
      }

      [XmlElement("DisplayNameID")]
      public int DisplayNameID
      {
         get { return mDisplayNameID; }
         set { mDisplayNameID = value; }
      }

      [XmlElement("RolloverTextID")]
      public int RolloverTextID
      {
         get { return mRolloverTextID; }
         set { mRolloverTextID = value; }
      }

      [XmlElement("MovementType")]
      public string MovementType
      {
         get { return mMovementType; }
         set { mMovementType = value; }
      }

      [XmlElement("FlightLevel")]
      public float FlightLevel
      {
         get { return mFlightLevel; }
         set { mFlightLevel = value; }
      }

      [XmlElement("Velocity")]
      public float Velocity
      {
         get { return mVelocity; }
         set { mVelocity = value; }
      }

      [XmlElement("TurnRate")]
      public float TurnRate
      {
         get { return mTurnRate; }
         set { mTurnRate = value; }
      }

      [XmlElement("Hitpoints")]
      public float Hitpoints
      {
         get { return mHitpoints; }
         set { mHitpoints = value; }
      }

      [XmlElement("Shieldpoints")]
      public float ShieldPoints
      {
         get { return mShieldPoints; }
         set { mShieldPoints = value; }
      }

      [XmlElement("LOS")]
      public float LOS
      {
         get { return mLOS; }
         set { mLOS = value; }
      }

      [XmlElement("BuildPoints")]
      public float BuildPoints
      {
         get { return mBuildPoints; }
         set { mBuildPoints = value; }
      }

      [XmlElement("RepairPoints")]
      public float RepairPoints
      {
         get { return mRepairPoints; }
         set { mRepairPoints = value; }
      }

      [XmlElement("FormationType")]
      public string FormationType
      {
         get { return mFormationType; }
         set { mFormationType = value; }
      }

      [XmlElement("Cost", typeof(ProtoObjectCost))]
      public ProtoObjectCostList Costs
      {
         get { return mCosts; }
         set { mCosts = value; }
      }

      [XmlElement("CostEscalation")]
      public float CostEscalation
      {
         get { return mCostEscalation; }
         set { mCostEscalation = value; }
      }

      [XmlElement("CostEscalationObject", typeof(DataString))]
      public DataStringList CostEscalationObjects
      {
         get { return mCostEscalationObjects; }
         set { mCostEscalationObjects = value; }
      }

      [XmlElement("Pop", typeof(ProtoObjectPop))]
      public ProtoObjectPopList Pops
      {
         get { return mPops; }
         set { mPops = value; }
      }

      [XmlElement("PopCapAddition", typeof(ProtoObjectPop))]
      public ProtoObjectPopList PopCapAdditions
      {
         get { return mPopCapAdditions; }
         set { mPopCapAdditions = value; }
      }

      [XmlElement("Bounty")]
      public float Bounty
      {
         get { return mBounty; }
         set { mBounty = value; }
      }

      [XmlElement("ResourceAmount")]
      public float ResourceAmount
      {
         get { return mResourceAmount; }
         set { mResourceAmount = value; }
      }

      [XmlElement("Lifespan")]
      public float Lifespan
      {
         get { return mLifespan; }
         set { mLifespan = value; }
      }

      [XmlElement("Tactics")]
      public string Tactics
      {
         get { return mTactics; }
         set { mTactics = value; }
      }

      [XmlElement("GathererLimit")]
      public int GathererLimit
      {
         get { return mGathererLimit; }
         set { mGathererLimit = value; }
      }

      [XmlElement("ObstructionRadiusX")]
      public float ObstructionRadiusX
      {
         get { return mObstructionRadiusX; }
         set { mObstructionRadiusX = value; }
      }

      [XmlElement("ObstructionRadiusZ")]
      public float ObstructionRadiusZ
      {
         get { return mObstructionRadiusZ; }
         set { mObstructionRadiusZ = value; }
      }

      [XmlElement("PlacementRules")]
      public string PlacementRules
      {
         get { return mPlacementRules; }
         set { mPlacementRules = value; }
      }

      [XmlElement("MaxProjectileHeight")]
      public float MaxProjectileHeight
      {
         get { return mMaxProjectileHeight; }
         set { mMaxProjectileHeight = value; }
      }

      [XmlElement("PickPriority")]
      public string PickPriority
      {
         get { return mPickPriority; }
         set { mPickPriority = value; }
      }

      [XmlElement("PickRadius")]
      public float PickRadius
      {
         get { return mPickRadius; }
         set { mPickRadius = value; }
      }

      [XmlElement("PickOffset")]
      public float PickOffset
      {
         get { return mPickOffset; }
         set { mPickOffset = value; }
      }

      [XmlElement("SelectType")]
      public string SelectType
      {
         get { return mSelectType; }
         set { mSelectType = value; }
      }

      [XmlElement("GotoType")]
      public string GotoType
      {
         get { return mGotoType; }
         set { mGotoType = value; }
      }

      [XmlElement("SelectedRadiusX")]
      public float SelectedRadiusX
      {
         get { return mSelectedRadiusX; }
         set { mSelectedRadiusX = value; }
      }

      [XmlElement("SelectedRadiusZ")]
      public float SelectedRadiusZ
      {
         get { return mSelectedRadiusZ; }
         set { mSelectedRadiusZ = value; }
      }

      [XmlElement("PhysicsInfo")]
      public string PhysicsInfo
      {
         get { return mPhysicsInfo; }
         set { mPhysicsInfo = value; }
      }

      [XmlElement("GatherLink", typeof(ProtoObjectGatherLink))]
      public ProtoObjectGatherLink GatherLink
      {
         get { return mGatherLink; }
         set { mGatherLink = value; }
      }

      [XmlIgnore]
      public float GatherLinkRadius
      {
         get { return mGatherLink.Radius; }
         set { mGatherLink.Radius = value; }
      }

      [XmlIgnore]
      public float GatherLinkRate
      {
         get { return mGatherLink.Rate; }
         set { mGatherLink.Rate = value; }
      }

      [XmlIgnore]
      public bool GatherLinkSelf
      {
         get { return mGatherLink.Self; }
         set { mGatherLink.Self = value; }
      }

      [XmlIgnore]
      public string GatherLinkUnit
      {
         get { return mGatherLink.Unit; }
         set { mGatherLink.Unit = value; }
      }

      [XmlIgnore]
      public string GatherLinkTarget
      {
         get { return mGatherLink.Target; }
         set { mGatherLink.Target = value; }
      }

      [XmlElement("BlockMovementObject")]
      public string BlockMovementObject
      {
         get { return mBlockMovementObject; }
         set { mBlockMovementObject = value; }
      }

      [XmlElement("Contain", typeof(DataString))]
      public DataStringList Contains
      {
         get { return mContains; }
         set { mContains = value; }
      }

      [XmlElement("Command", typeof(ProtoObjectCommand))]
      public ProtoObjectCommandList Commands
      {
         get { return mCommands; }
         set { mCommands = value; }
      }

      [XmlElement("TrainLimit", typeof(ProtoObjectTrainLimit))]
      public ProtoObjectTrainLimitList TrainLimits
      {
         get { return mTrainLimits; }
         set { mTrainLimits = value; }
      }

      [XmlElement("Power")]
      public string Power
      {
         get { return mPower; }
         set { mPower = value; }
      }

      [XmlElement("Sound", typeof(ProtoObjectSound))]
      public ProtoObjectSoundList Sounds
      {
         get { return mSounds; }
         set { mSounds = value; }
      }

      [XmlElement("ObjectType", typeof(DataString))]
      public DataStringList ObjectTypes
      {
         get { return mObjectTypes; }
         set { mObjectTypes = value; }
      }

      [XmlElement("Flag", typeof(DataString))]
      public DataStringList Flags
      {
         get { return mFlags; }
         set { mFlags = value; }
      }
   }

   //==============================================================================
   // ProtoObjectCost
   //==============================================================================
   public class ProtoObjectCost
   {
      //---------------------------------------------------------------------------
      // Private members
      //---------------------------------------------------------------------------
      private string mResourceType = @"";
      private float mAmount = 0.0f;

      //---------------------------------------------------------------------------
      // Properties
      //---------------------------------------------------------------------------
      [XmlAttribute("ResourceType")]
      public string ResourceType
      {
         get { return mResourceType; }
         set { mResourceType = value; }
      }

      [XmlText]
      public float Amount
      {
         get { return mAmount; }
         set { mAmount = value; }
      }
   }

   //==============================================================================
   // ProtoObjectPop
   //==============================================================================
   public class ProtoObjectPop
   {
      private string mPopType = @"";
      private float mAmount = 0;

      [XmlAttribute("Type")]
      public string PopType
      {
         get { return mPopType; }
         set { mPopType = value; }
      }

      [XmlText]
      public float Amount
      {
         get { return mAmount; }
         set { mAmount = value; }
      }
   }

   //==============================================================================
   // ProtoObjectCommand
   //==============================================================================
   public class ProtoObjectCommand
   {
      private string mCommandType = @"";
      private int mPosition = -1;
      private string mItem = @"";

      [XmlAttribute("Type")]
      public string CommandType
      {
         get { return mCommandType; }
         set { mCommandType = value; }
      }

      [XmlAttribute("Position")]
      public int Position
      {
         get { return mPosition; }
         set { mPosition = value; }
      }

      [XmlText]
      public string Item
      {
         get { return mItem; }
         set { mItem = value; }
      }
   }

   //==============================================================================
   // ProtoObjectTrainLimit
   //==============================================================================
   public class ProtoObjectTrainLimit
   {
      private string mLimitType = @"";
      private string mItem = @"";
      private int mCount = 0;

      [XmlAttribute("Type")]
      public string LimitType
      {
         get { return mLimitType; }
         set { mLimitType = value; }
      }

      [XmlAttribute("Count")]
      public int Count
      {
         get { return mCount; }
         set { mCount = value; }
      }

      [XmlText]
      public string Item
      {
         get { return mItem; }
         set { mItem = value; }
      }
   }

   //==============================================================================
   // ProtoObjectColor
   //==============================================================================
   public class ProtoObjectColor
   {
      private float mRed = 1.0f;
      private float mGreen = 1.0f;
      private float mBlue = 1.0f;

      [XmlAttribute("Red")]
      public float Red
      {
         get { return mRed; }
         set { mRed = value; }
      }

      [XmlAttribute("Green")]
      public float Green
      {
         get { return mGreen; }
         set { mGreen = value; }
      }

      [XmlAttribute("Blue")]
      public float Blue
      {
         get { return mBlue; }
         set { mBlue = value; }
      }
   }

   //==============================================================================
   // ProtoObjectGatherLink
   //==============================================================================
   public class ProtoObjectGatherLink
   {
      private string mTarget = @"";
      private float mRadius = 0.0f;
      private float mRate = 0.0f;
      private bool mSelf = false;
      private string mUnit = @"";

      [XmlAttribute("Target")]
      public string Target
      {
         get { return mTarget; }
         set { mTarget = value; }
      }

      [XmlAttribute("Radius")]
      public float Radius
      {
         get { return mRadius; }
         set { mRadius = value; }
      }

      [XmlAttribute("Rate")]
      public float Rate
      {
         get { return mRate; }
         set { mRate = value; }
      }

      [XmlAttribute("Self")]
      public bool Self
      {
         get { return mSelf; }
         set { mSelf = value; }
      }

      [XmlText]
      public string Unit
      {
         get { return mUnit; }
         set { mUnit = value; }
      }
   }

   //==============================================================================
   // ProtoObjectSound
   //==============================================================================
   public class ProtoObjectSound
   {
      private string mSoundType = @"";
      private string mName = @"";

      [XmlAttribute("Type")]
      public string SoundType
      {
         get { return mSoundType; }
         set { mSoundType = value; }
      }

      [XmlText]
      public string Name
      {
         get { return mName; }
         set { mName = value; }
      }
   }

   //==============================================================================
   // ProtoObjectList
   //==============================================================================
   public class ProtoObjectList : BindingListView<ProtoObject>
   {
      private int mNextID = 0;

      public int NextID
      {
         get { return mNextID; }
         set { mNextID = value; }
      }

      //---------------------------------------------------------------------------
      // AddNewCore
      //---------------------------------------------------------------------------
      protected override object AddNewCore()
      {
         int id = NextID;
         NextID = id + 1;
         ProtoObject c = new ProtoObject();
         c.ID = id;
         this.Add(c);
         return c;
      }

      //---------------------------------------------------------------------------
      // EndNew
      //---------------------------------------------------------------------------
      public override void EndNew(int itemIndex)
      {
         base.EndNew(itemIndex);
      }

      //---------------------------------------------------------------------------
      // CancelNew
      //---------------------------------------------------------------------------
      public override void CancelNew(int itemIndex)
      {
         if (this.Items[itemIndex].ID == NextID - 1)
            NextID--;

         base.CancelNew(itemIndex);
      }

      //---------------------------------------------------------------------------
      // calcNextID
      //---------------------------------------------------------------------------
      public void calcNextID()
      {
         int maxId = -1;
         foreach (ProtoObject c in this)
         {
            if (c.ID > maxId)
               maxId = c.ID;
         }
         mNextID = maxId + 1;
      }
   }

   //==============================================================================
   // ProtoObjectCostList
   //==============================================================================
   public class ProtoObjectCostList : List<ProtoObjectCost>
   {
   }

   //==============================================================================
   // ProtoObjectPopList
   //==============================================================================
   public class ProtoObjectPopList : List<ProtoObjectPop>
   {
   }

   //==============================================================================
   // ProtoObjectCommandList
   //==============================================================================
   public class ProtoObjectCommandList : List<ProtoObjectCommand>
   {
   }

   //==============================================================================
   // ProtoObjectTrainLimitList
   //==============================================================================
   public class ProtoObjectTrainLimitList : List<ProtoObjectTrainLimit>
   {
   }

   //==============================================================================
   // ProtoObjectSoundList
   //==============================================================================
   public class ProtoObjectSoundList : List<ProtoObjectSound>
   {
   }
}
