using System;
using System.Collections.Generic;
using System.Text;
using Rendering;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.IO;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;

using ModelSystem;
using EditorCore;

namespace SimEditor
{
   public interface IHasID
   {
      int ID
      {         
         get;
      }
   }
   public interface IHasGroup
   {
      int Group
      {
         get;
         set;
      }
   }

   public interface ISupportsScenarioOwner
   {
      int Department
      {
         get;
         set;
      }
   }

      //const int cDesign = 1;
      //const int cArt = 2;
      //const int cSound = 3;

   [Flags]
   public enum eDepartment
   {
      Design = 0x01,
      Art = 0x02,
      Sound = 0x04,
   }



   public class EditorObject
   {
      public EditorObject()
      {

      }

      ~EditorObject()
      {

      }

      public virtual void postAddCB() { }
      public virtual void postSelectCB() { }
      
      public virtual void initVisuals()
      {

      }
      public virtual void clearVisuals()
      {

      }

      public virtual void render()
      {

      }
      public virtual void renderSelected()
      {


      }


      public virtual EditorObject Copy()
      {
         return null;
      }
      public virtual string ToString()
      {
         return GetTypeName();
      }
      public virtual object GetProperties()
      {
         return null;

      }


      public virtual int GetDepartment()
      {
         return (int)(eDepartment.Design);
      }
      protected int mDepartmentVisPermission = (int)(eDepartment.Art | eDepartment.Design | eDepartment.Sound);
      public virtual int GetDeptVisPermission()
      {
         return mDepartmentVisPermission;
      }


      public virtual void propertiesChanged() { }
      public virtual void childComponentChanged(VisualControlObject obj) { }
      public virtual List<EditorObject> getChildComponents() { return null; }
      public virtual bool AutoSelectChildComponents() { return true; }

      public virtual void updateBB(){ }

      public virtual bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         return false;
      }
      public virtual bool testForBoxIntersection(Vector3[] points)
      {
         return false;
      }

      public Vector3 getPosition()
      {
         return new Vector3(mMatrix.M41, mMatrix.M42, mMatrix.M43);
      }
      public void setPosition(Vector3 pos)
      {
         mMatrix.M41 = pos.X;
         mMatrix.M42 = pos.Y;
         mMatrix.M43 = pos.Z;
         propertiesChanged();
      }

      public Vector3 getDirection()
      {
         return new Vector3(mMatrix.M31, mMatrix.M32, mMatrix.M33);
      }
      public void setDirection(Vector3 dir)
      {
         mMatrix.M31 = dir.X;
         mMatrix.M32 = dir.Y;
         mMatrix.M33 = dir.Z;
         mMatrix.M11 = mMatrix.M33;
         mMatrix.M13 = -mMatrix.M31;
         propertiesChanged();
      }

      public void setRotation(Vector3 rot)
      {
         mRotation = rot;
      }
      public Vector3 getRotation()
      {
         return (mRotation);
      }
      public Matrix getRotationOnly()
      {
         Matrix k = mMatrix;
         k.M41 = 0;
         k.M42 = 0;
         k.M43 = 0;
         k.M44 = 1;
         return k;
      }

      public void setMatrix(Matrix m)
      {
         mMatrix = m;
         propertiesChanged();

         //Set mRotation.Y
         Vector3 fwd = new Vector3(m.M31, m.M32, m.M33);
         fwd.Y = 0;
         fwd = BMathLib.Normalize(fwd);
         double angle = Math.Acos(Vector3.Dot(new Vector3(0, 0, 1), fwd));
         double finalAngle = angle;
         if (fwd.X < 0)
            finalAngle = -angle;
         mRotation.Y = (float)finalAngle;  

      }
      public Matrix getMatrix()
      {
         return mMatrix;
      }


      public virtual void rotated() { }
      public virtual void moved() { }
      public virtual void removed() { }

      public virtual string GetTypeName()
      {
         return "EditorObject";
      }

      //public string TypeName
      //{
      //   get
      //   {
      //      return GetTypeName();
      //   }
      //}

      public virtual string Name
      {
         get
         {
            return GetName();
         }
      }

      public virtual string GetName()
      {
         return GetTypeName();
      }

      public virtual void OnPlaced()
      {

      }
      //public virtual void OnPlaced()
      //{

      //}

      public virtual bool IsVisible()
      {
         return true;
      }
      public virtual bool IsSelectable()
      {
         return true;
      }
      public virtual bool IsInPublicList()
      {
         return true;
      }

      bool mbBound = false;
      public virtual void SetBound(bool bound)
      {
         mbBound = bound;
      }
      public virtual bool IsBound()
      {
         return mbBound;
      }

      [XmlIgnore]
      public BBoundingBox mAABB = new BBoundingBox();

      //values we can adjust through the main panel
      [XmlIgnore]
      public Vector3 mRotation = Vector3.Empty;

      [XmlIgnore]
      public Vector3 mTempGroupOffset = Vector3.Empty;

      protected Matrix mMatrix = Matrix.Identity;

      [XmlIgnore]
      public bool mLockToTerrain = true;
      [XmlIgnore]
      public bool mAlignToTerrain = false;

      [XmlIgnore]
      public bool mRenderedThisFrame = false;
   }

   public class VisualControlObject : EditorObject
   {
      public VisualControlObject(EditorObject owner, float radius, int color)
      {
         init(owner, radius, color, null);
      }
      public VisualControlObject(EditorObject owner, float radius, int color, string name)
      {
         init(owner, radius, color, name);
      }
      ~VisualControlObject()
      {
         clearVisuals();
      }

      private void init(EditorObject owner, float radius, int color, string name)
      {
         mColor = color;
         mRadius = radius;
         mOwner = owner;

         mLockToTerrain = false;
         mAlignToTerrain = false;

         if (name == null && !mName.Contains("target"))
            mName += "target";
         else if(!mName.Contains(name))
            mName += name;
      }

      public override void postSelectCB()
      {
         if(SimGlobals.getSimMain().giveSelectedObjectIndex(mOwner)!=-1)
            SimGlobals.getSimMain().unselectObject(this);
      }

      public override void initVisuals()
      {
         updateBB();
         mVisSphere = new BRenderDebugSphere(mRadius, mColor);
         mSelCube = new BRenderDebugCube(new Vector3(-mRadius, -mRadius, -mRadius), new Vector3(mRadius, mRadius, mRadius), System.Drawing.Color.Yellow.ToArgb(), false);
      }
      public override void clearVisuals()
      {
         if(mVisSphere!=null)mVisSphere.destroy();
         if (mSelCube != null) mSelCube.destroy();
      }

      public override void render()
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mVisSphere.render(false,false);
      }
      public override void renderSelected()
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mSelCube.render();
      }

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mRadius * 2;
         mAABB.min.Y = -mRadius * 2;
         mAABB.min.Z = -mRadius * 2;

         mAABB.max.X = mRadius * 2;
         mAABB.max.Y = mRadius * 2;
         mAABB.max.Z = mRadius * 2;

      }

      public override void rotated() 
      {
         if (mOwner != null)
            mOwner.childComponentChanged(this);
      }
      public override void moved()
      {
         if (mOwner != null)
            mOwner.childComponentChanged(this);
      }


      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(getPosition(), mRadius * 2, r0, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect((mAABB.min) + pos, (mAABB.max) + pos,
                     points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }

      public override string Name
      {
         get
         {
            return "--" + mOwner.Name + " " + mName;
         }
      }

      public override string ToString()
      {
         string kName = "--" + mOwner.ToString() + " " + mName;


         return kName;
      }
      public override string GetTypeName()
      {
         return "VisualControlObject";
      }


      BRenderDebugSphere mVisSphere;
      BRenderDebugCube mSelCube;

      public bool mAddToWorldList = true;
      float mRadius=1.0f;
      int mColor = 0;
      EditorObject mOwner;
      string mName = "";

      public void Bind(EditorObject newOwner)
      {
         mOwner = newOwner;
      }

      public VisualControlObject Copy()
      {
         this.updateBB();
         VisualControlObject newCopy = (VisualControlObject)this.MemberwiseClone();
         newCopy.init(mOwner, mRadius, mColor, mName);
         newCopy.initVisuals();

         return newCopy;
      }

 
      public override int GetDepartment()
      {
         return (int)eDepartment.Art;
      }

   }


   //------------------------------------------
   //------------------------------------------
   //------------------------------------------

   public class SimObjectData
   {
      public SimObjectData(SimObject par)
      {
         mParent = par;
      }

      eDepartment mDepartment = eDepartment.Design;
      bool mDepartmentSet = false;
      public int Department
      {
         get
         {
            return (int)mDepartment;
         }
         set
         {

            if (mbIsSound == true)
            {
               mDepartment = eDepartment.Sound;
               return;
            }

            eDepartment newDepartment = (eDepartment)value;

            if (newDepartment != eDepartment.Design && mDepartment == eDepartment.Design && mParent.IsBound() == true)
            {
               return; ///!Cant change the department of a bound object!!
            }

            if (mDepartmentSet && (newDepartment != mDepartment))
            {
               SimGlobals.getSimMain().TransferID(this, mDepartment, newDepartment);
            }

            mDepartment = newDepartment;
            mDepartmentSet = true;
         }
      }
      public bool mbIsSound = false;


      public string mEditorName = "";
      public string Name
      {
         set
         {
            mEditorName = value;
         }
         get
         {
            //always make a name
            if(mEditorName == "" && ID != -1)
            {
               if (IsSquad == true)
               {
                  mEditorName = mParent.GetProtoName() + "_Squad_" + ID.ToString();

               }
               else
               {
                  mEditorName = mParent.GetProtoName() + "_" + ID.ToString();

               }
            }

            return mEditorName;
         }
      }

      int mID = -1;
      [Description("The unit's unique id."), Category("ID")]
      public int ID
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


      [XmlAttribute("Group")]
      int mGroup = -1;
      public int Group
      {
         get
         {
            return mGroup;
         }
         set
         {
            mGroup = value;
         }
      }

      public bool mLockToTerrain = false;
      [Description("Determines if an object can float above the terrain"), Category("Movement")]
      public bool LockToTerrain
      {
         get
         {
            return mLockToTerrain;
         }
         set
         {
            mLockToTerrain = value;
            mParent.mLockToTerrain = mLockToTerrain;
         }
      }

      public bool mAlignToTerrain = false;
      [Description("Determines if an object will align to the terrain"), Category("Movement")]
      public bool AlignToTerrain
      {
         get
         {
            return mAlignToTerrain;
         }
         set
         {
            mAlignToTerrain = value;
         }
      }

      public bool mIgnoreToAO = false;
      [Description("Determines if an object will cast AO to the terrain"), Category("Data")]
      public bool IgnoreToAO
      {
         get
         {
            return mIgnoreToAO;
         }
         set
         {
            mIgnoreToAO = value;
         }
      }

      public float mTintValue = 1.0f;
      [Description("The tint value of an object"), Category("Data")]
      public float TintValue
      {
         get
         {
            return mTintValue;
         }
         set
         {
            mTintValue = BMathLib.Clamp(value, 0, 1.0f); ;
         }
      }

      public bool mNoCull = false;
      [Description("Tell an object not to cull"), Category("Data")]
      public bool NoCull
      {
         get
         {
            return mNoCull;
         }
         set
         {
            mNoCull = value;
         }
      }


      public bool mIncludeInSimRep = false;
      [Description("Include this object in sim rep"), Category("Data")]
      public bool IncludeInSimRep
      {
         get
         {
            return mIncludeInSimRep;
         }
         set
         {
            mIncludeInSimRep = value;
         }
      }


      public Vector3 mPosition = Vector3.Empty;
      [Category("Movement")]
      public string Position
      {
         set
         {
            mPosition = TextVectorHelper.FromString(value);
            mParent.setPosition(mPosition);
         }
         get
         {
            return TextVectorHelper.ToString(mPosition);
         }
      }

      public Vector3 mRotation = Vector3.Empty;
      [Category("Rotation")]
      public string Rotation
      {
         set
         {

            mRotation = TextVectorHelper.FromString(value);
            mParent.setRotation(mRotation);
         }
         get
         {
            return TextVectorHelper.ToString(mRotation);
         }
      }


      public int mOwningPlayer = 0;
      [Description("Player number that owns this item"), Category("Data")]
      public int OwningPlayer
      {
         get
         {
            return mOwningPlayer;
         }
         set
         {
            if (mbForcePlayerZero == false)
            {

               mOwningPlayer = value;
            }
            else
            {
               mOwningPlayer = 0;
            }
         }
      }

      public bool mbForcePlayerZero = false;
      [Description("This object must belong to player zero"), Category("Data")]
      public bool ForceToGaiaPlayer
      {
         get
         {
            return mbForcePlayerZero;
         }
      }

      public bool mbIsSquad = false;
      public bool IsSquad
      {
         get
         {
            return mbIsSquad;
         }
      }

      
      int mVisualVariationIndex = 0;
      public int VisualVariationIndex
      {
         get
         {
            return mVisualVariationIndex;
         }
         set
         {
            mVisualVariationIndex = value;
         }
      }

      private SimObject mParent;
   }

   //public class SimSquad : EditorObject, IHasID
   //{
   //   DataSquadXml mProtoSquad = null;
   //   public DataSquadXml ProtoObject
   //   {
   //      set
   //      {
   //         mProtoSquad = value;

   //         if (mProtoSquad != null)
   //         {
   //            //if (mProtoSquad.hasFlag("NoTieToGround"))
   //            //{
   //            //   mLockToTerrain = false;
   //            //   mAbstractData.LockToTerrain = false;
   //            //}
   //            //else
   //            //{
   //            //   mLockToTerrain = true;
   //            //   mAbstractData.LockToTerrain = true;
   //            //}
   //            //if (mProtoSquad.hasFlag("ForceToGaiaPlayer"))
   //            //{
   //            //   mAbstractData.mbForcePlayerZero = true;
   //            //}
   //            //else
   //            //{
   //            //   mAbstractData.mbForcePlayerZero = false;
   //            //}
   //            //if (mProtoSquad.hasFlag("OrientUnitWithGround"))
   //            //{
   //            //   mAbstractData.mAlignToTerrain = true;
   //            //}
   //            //else
   //            //{
   //            //   mAbstractData.mAlignToTerrain = false;
   //            //}

   //            initVisuals();
   //         }
   //      }
   //      get
   //      {
   //         return mProtoSquad;
   //      }
   //   }
   //   public int ID
   //   {
   //      get
   //      {
   //         return mAbstractData.ID;
   //      }
   //   }

   //   public override string Name
   //   {
   //      get
   //      {
   //         return mAbstractData.Name;
   //      }
   //   }

   //   private SimObjectData mAbstractData = null;


   //}


   public class SimObject : EditorObject, IHasID, IHasGroup, ISupportsScenarioOwner
   {
      public SimObject()
      {
         ProtoObject = null;
      }

      public SimObject(SimUnitXML protoObj, ScenarioSimUnitXML instanceObj, bool bLoading)
      {
         mAbstractData = new SimObjectData(this);
         fromXMLData(protoObj,null, instanceObj, bLoading);
         ProtoObject = protoObj;
      }

      public SimObject(ProtoSquadXml squadObj, ScenarioSimUnitXML instanceObj, bool bLoading)
      {
         mAbstractData = new SimObjectData(this);
         fromXMLData(null,squadObj, instanceObj, bLoading);
         ProtoSquad = squadObj;         

      }
      
      public int Department
      {
         get
         {           
            return mAbstractData.Department;
         }
         set
         {
            //Lock sound objects to sound department
            //if (mbIsSoundObject == true)
            //{
            //   mAbstractData.Department = (int)(eDepartment.Sound);
            //   return;
            //}

            mAbstractData.Department = value;
         }
      }

      public override int GetDepartment()
      {
         return Department;
      }

      public int Group
      {
         get
         {
            return mAbstractData.Group;
         }
         set
         {
            mAbstractData.Group = value;
         }
      }

      public SimObject(ScenarioSimUnitXML instanceObj, bool bLoading)
      {
         if (instanceObj.mIsSquad == true)
         {
            ProtoSquadXml protoSquad = SimGlobals.getSimMain().mSimFileData.mProtoSquadsByName[instanceObj.mProtoUnit];
            mAbstractData = new SimObjectData(this);
            mAbstractData.mbIsSquad = true;
            ProtoSquad = protoSquad;             
            fromXMLData(null, protoSquad, instanceObj, bLoading);

         }
         else
         {
            SimUnitXML protoObject = SimGlobals.getSimMain().mSimFileData.mProtoObjectsByName[instanceObj.mProtoUnit];
            mAbstractData = new SimObjectData(this);
            ProtoObject = protoObject; 
            fromXMLData(protoObject, null, instanceObj, bLoading);
         }

      }
      public SimObject(string protoname, bool isSquad)
      {
         if (isSquad == true)
         {
            ProtoSquadXml protoSquad = SimGlobals.getSimMain().mSimFileData.mProtoSquadsByName[protoname];
            mAbstractData = new SimObjectData(this);
            mAbstractData.mbIsSquad = true;
            ProtoSquad = protoSquad;
            fromXMLData(null, protoSquad, null, false);

         }
         else
         {
            SimUnitXML protoObject = SimGlobals.getSimMain().mSimFileData.mProtoObjectsByName[protoname];
            mAbstractData = new SimObjectData(this);
            ProtoObject = protoObject;
            fromXMLData(protoObject, null, null, false);
         }
      }

      public override void postAddCB()
      {
         mAbstractData.OwningPlayer = SimGlobals.getSimMain().SelectedPlayerID;
      }
      public int ID
      {
         get
         {
            return mAbstractData.ID;
         }
      }

      public override string Name
      {
         get
         {
            return mAbstractData.Name;
         }
      }

      public override void initVisuals()
      {
         SimObject obj = this;

         int boxColor = (int)System.Drawing.Color.Yellow.ToArgb();
         if (this.mAbstractData.mbIsSquad == true)
         {
            boxColor = (int)System.Drawing.Color.BlueViolet.ToArgb();
            foreach (DataSquadUnitXml unit in ProtoSquad.mUnits)
            {
               if (mVisual != null)
                  continue;               
               SimUnitXML protoObject;

               if (SimGlobals.getSimMain().mSimFileData.mProtoObjectsByNameLower.TryGetValue(unit.Name.ToLower(), out protoObject))
               {
                  if (protoObject.mAnimFile != null)
                  {
                     string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, protoObject.mAnimFile);
                     fileName = Path.ChangeExtension(fileName, ".vis");

                     mVisual = BVisualManager.getOrLoadVisual(fileName);

                     if (mVisual == null || mVisual.mBoundingBox.isEmpty())
                     {
                        boxColor = (int)System.Drawing.Color.Red.ToArgb();
                     }
                  }
               }
            }
            updateBB();

         }
         else
         {

            if (obj.ProtoObject.mAnimFile != null)
            {
               string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, obj.ProtoObject.mAnimFile);
               fileName = Path.ChangeExtension(fileName, ".vis");

               mVisual = BVisualManager.getOrLoadVisual(fileName);

               if (mVisual == null || mVisual.mBoundingBox.isEmpty())
               {
                  boxColor = (int)System.Drawing.Color.Red.ToArgb();
               }
            }
            else
            {
               boxColor = (int)System.Drawing.Color.Red.ToArgb();
            }

            updateBB();

            //int color = !mVisual.mBoundingBox.isEmpty() ? (int)System.Drawing.Color.Yellow.ToArgb() : (int)System.Drawing.Color.Red.ToArgb();

         }
         mAABBVis = new BRenderDebugCube(mAABB.min, mAABB.max, boxColor, false);
         mOOBBVis = new BRenderDebugOOBB(mOBB.mCenter, mOBB.mExtents, mOBB.mOrientation, (int)System.Drawing.Color.Yellow.ToArgb(), false);
       }
      public override void clearVisuals()
      {
         /*
         for (int i = 0; i < mMeshes.Count; i++)
            mMeshes[i].destroy();
         */
         if (mAABBVis!=null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if (mOOBBVis != null)
         {
            mOOBBVis.destroy();
            mOOBBVis = null;
         }
      }

      public override void render()
      {
         //BRenderDevice.getDevice().Transform.World = mMatrix;
         /*
         for(int i=0;i<mMeshes.Count;i++)
         {
            mMeshes[i].render();
         }
          */

         Vector3 color;
         switch(mAbstractData.OwningPlayer)
         {
            case 0:
               // white
               color = new Vector3(1.0f, 1.0f, 1.0f);
               break;
            case 1:
               // blue
               color = new Vector3(0.0f, 0.0f, 1.0f);
               break;
            case 2:
               // red
               color = new Vector3(1.0f, 0.0f, 0.0f);
               break;
            case 3:
               // yellow
               color = new Vector3(1.0f, 1.0f, 0.0f);
               break;
            case 4:
               // green
               color = new Vector3(0.0f, 1.0f, 0.0f);
               break;
            case 5:
               //orange
               color = new Vector3(1.0f, 0.5f, 0.0f);
               break;
            case 6:
               //cyan
               color = new Vector3(0.0f, 1.0f, 1.0f);
               break;
            case 7:
            default:
               //purple
               color = new Vector3(1.0f, 0.0f, 1.0f);
               break;
         }


         

         if (this.mAbstractData.mbIsSquad == true)
         {

            if (mVisual != null)
            {
               mVisual.setSelectedVisualVariation(mAbstractData.VisualVariationIndex);
               mVisual.render(color, true);
            }
            if ((mVisual == null) || (mVisual.mBoundingBox.isEmpty()))
            {
               BRenderDevice.getDevice().Transform.World = mMatrix;
               mAABBVis.render();
            }

         }
         else
         {

            if (mVisual != null)
            {
               mVisual.setSelectedVisualVariation(mAbstractData.VisualVariationIndex);
               mVisual.render(color, true);
            }
            if ((mVisual == null) || (mVisual.mBoundingBox.isEmpty()))
            {
               BRenderDevice.getDevice().Transform.World = mMatrix;
               mAABBVis.render();
            }
         }

      }


      public override void renderSelected()
      {
        BRenderDevice.getDevice().Transform.World = mMatrix;
        mOOBBVis.render();
     }

     public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         if (BMathLib.rayOOBBIntersect(mOBB.mCenter, mOBB.mExtents, mOBB.mOrientation, kOrg, rD, ref tVal))
         {
            if (mVisual != null)
               return mVisual.testRayIntersection(kOrg, rD, getRotationOnly());

            return true;
         }
         return false;
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect(mAABB.min + pos, mAABB.max + pos,
            points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }
      public override void updateBB()
      {
         if ((mVisual == null) || (mVisual.mBoundingBox.isEmpty()))
         {
            mAABB.min.X = -1;
            mAABB.min.Y = -1;
            mAABB.min.Z = -1;

            mAABB.max.X = 1;
            mAABB.max.Y = 1;
            mAABB.max.Z = 1;
         }
         else
         {
            mAABB.min.X = 0;
            mAABB.min.Y = 0;
            mAABB.min.Z = 0;

            mAABB.max.X = 0;
            mAABB.max.Y = 0;
            mAABB.max.Z = 0;

            mAABB.addBox(mVisual.mBoundingBox);
         }

         mOBB.constructFromAABB(mAABB.min, mAABB.max, mMatrix);
         

         if(mAbstractData==null)
            mAbstractData = new SimObjectData(this);
         mAbstractData.mPosition = getPosition();
      }

      public override EditorObject Copy()
      {
         SimObject obj = new SimObject( toXMLData(true), false);
   
         return obj;
      }
      public override string ToString()
      {
         return GetProtoName();
      }
      public override string GetTypeName()
      {
         return "SimObject"; //mProtoObject.mName;// "SimObject";
      }

      public override object GetProperties()
      {
         return mAbstractData;

      }
      public SimObjectData GetTypedProperties()
      {
         return mAbstractData;
      }
      public void setPlayerID(int id)
      {
         mAbstractData.OwningPlayer = id;
      }
      public int getPlayerID()
      {
         return mAbstractData.OwningPlayer;
      }

      public string GetProtoName()
      {
         if (ProtoObject != null)
         {
            return ProtoObject.mName;
         }
         else if (ProtoSquad != null)
         {
            return ProtoSquad.Name;
         }
         else
         {
            return "error";
         }

      }

      public ScenarioSimUnitXML toXMLData(bool bMakeNewID)
      {
         ScenarioSimUnitXML simUnitXml = new ScenarioSimUnitXML();


         simUnitXml.mProtoUnit = GetProtoName();// ProtoObject.mName;


         simUnitXml.Position = TextVectorHelper.ToString(getPosition());

         Vector3 forward = new Vector3(0, 0, 1);
         //forward.TransformNormal(Matrix.RotationY(mRotation.Y));
         Vector4 v = Vector3.Transform(forward, this.getRotationOnly());
         forward.X = v.X;
         forward.Y = v.Y;
         forward.Z = v.Z;

         simUnitXml.Forward = TextVectorHelper.ToString(forward);

         Vector3 right = new Vector3(1, 0, 0);
         //right.TransformNormal(Matrix.RotationY(mRotation.Y));
         v = Vector3.Transform(right, this.getRotationOnly());
         right.X = v.X;
         right.Y = v.Y;
         right.Z = v.Z;

         simUnitXml.Right = TextVectorHelper.ToString(right);

         simUnitXml.mPlayer = mAbstractData.mOwningPlayer;
         simUnitXml.mTintValue = mAbstractData.mTintValue;

         if (mAbstractData.mAlignToTerrain)
            simUnitXml.mFlags.Add("OrientUnitWithGround");
         else
            simUnitXml.mFlags.Remove("OrientUnitWithGround");

         if (mAbstractData.mLockToTerrain)
            simUnitXml.mFlags.Remove("NoTieToGround");
         else
            simUnitXml.mFlags.Add("NoTieToGround");

         if (mAbstractData.IgnoreToAO)
            simUnitXml.mFlags.Add("IgnoreToAO");
         else
            simUnitXml.mFlags.Remove("IgnoreToAO");

         if (mAbstractData.NoCull)
            simUnitXml.mFlags.Add("NoCull");
         else
            simUnitXml.mFlags.Remove("NoCull");

         if (mAbstractData.IncludeInSimRep)
            simUnitXml.mFlags.Add("IncludeInSimRep");
         else
            simUnitXml.mFlags.Remove("IncludeInSimRep");

         if (mAbstractData.mbForcePlayerZero == true)
         {
            simUnitXml.mPlayer = 0;
         }
      

         if (bMakeNewID)
         {
            simUnitXml.mID = SimGlobals.getSimMain().GetUniqueID();
            //mAbstractData.ID = simUnitXml.mID;
         }
         else
         {
            simUnitXml.mID = mAbstractData.ID;
         }

         simUnitXml.Name = mAbstractData.Name;
         simUnitXml.Group = mAbstractData.Group;
         simUnitXml.VisualVariationIndex = mAbstractData.VisualVariationIndex;

         simUnitXml.mIsSquad = mAbstractData.IsSquad;

         return simUnitXml;
      }

      public bool IsSoundObject
      {
         get
         {
            return mAbstractData.mbIsSound;
         } 
      }

      private void fromXMLData(SimUnitXML protoObj,ProtoSquadXml squadObj, ScenarioSimUnitXML instanceObj, bool bLoading)
      {
         //mAbstractData = new SimObjectData(this);

         string protoName = "";

         if (protoObj != null)
         {
            mAbstractData.mbIsSquad = false;
            protoName = protoObj.mName;
         }
         else if (squadObj != null)
         {
            mAbstractData.mbIsSquad = true;
         }
         else
         {

         }
        


         if(instanceObj ==null)
         {
            //we don't have an instance, use proto as our defaults..

            if (protoObj != null)
            {
               mAbstractData.LockToTerrain = mLockToTerrain = protoObj.hasFlag("NoTieToGround");
               mAbstractData.AlignToTerrain = protoObj.hasFlag("OrientUnitWithGround");
               mAbstractData.IgnoreToAO = protoObj.hasFlag("IgnoreToAO");
               mAbstractData.IncludeInSimRep = protoObj.hasFlag("IncludeInSimRep");
               mAbstractData.NoCull = protoObj.hasFlag("NoCull");

            }
            mAbstractData.OwningPlayer = 1;
            mAbstractData.Position = TextVectorHelper.ToString(Vector3.Empty);
            mAbstractData.Rotation = TextVectorHelper.ToString(Vector3.Empty);
            mAbstractData.VisualVariationIndex = 0;
            //mAbstractData.mID = SimGlobals.getSimMain().GetUniqueID();
         }
         else
         {
            if (instanceObj.mID == -1)
            {

               mAbstractData.ID = SimGlobals.getSimMain().GetUniqueID();
            }
            else
            {
               mAbstractData.ID = instanceObj.mID;
               if (bLoading)
               {
                  SimGlobals.getSimMain().RegisterID(instanceObj.mID);
               }
            }


            //use our instance info as our defaults
            mAbstractData.LockToTerrain = mLockToTerrain = !instanceObj.hasFlag("NoTieToGround");
            mAbstractData.AlignToTerrain = instanceObj.hasFlag("OrientUnitWithGround");
            mAbstractData.IgnoreToAO = instanceObj.hasFlag("IgnoreToAO");
            mAbstractData.NoCull = instanceObj.hasFlag("NoCull");
            mAbstractData.IncludeInSimRep = instanceObj.hasFlag("IncludeInSimRep");
            mAbstractData.OwningPlayer = instanceObj.mPlayer;
            mAbstractData.TintValue = instanceObj.mTintValue;

            mAbstractData.Position = instanceObj.Position;

            Matrix rotationMatrix = BMathLib.makeRotateMatrix2(instanceObj.mForward, instanceObj.mRight);       

            setMatrix(rotationMatrix * Matrix.Translation(mAbstractData.mPosition));
            updateBB();

            mAbstractData.Name = instanceObj.Name;
            mAbstractData.Group = instanceObj.Group;
            mAbstractData.VisualVariationIndex = instanceObj.VisualVariationIndex;

            if (mAbstractData.Name.ToLower().StartsWith("snd_") == true)
            {
               protoName = mAbstractData.Name;
            }
         }
         if (protoName.ToLower().StartsWith("snd_") == true)
         {
     
            mAbstractData.mbIsSound = true;
            mDepartmentVisPermission = (int)(eDepartment.Sound);
            Department = (int)(eDepartment.Sound);
         }      
      }

      public void cycleVariationVisual(bool useRandomWeights)
      {
         mVisual.cycleSelectedVisualVariation(useRandomWeights);
         mAbstractData.VisualVariationIndex = mVisual.getSelectedVisualVariation();
      }



      public BVisual mVisual = null;

      public List<BRenderGrannyMesh> mMeshes = new List<BRenderGrannyMesh>();
      SimUnitXML mProtoObject = null;

      public bool mbSusbendProtoUpdate = false;

      public SimUnitXML ProtoObject
      {
         set
         {
            mProtoObject = value;

            if (mProtoObject != null)
            {
               if (mbSusbendProtoUpdate == false)
               {
                  if (mProtoObject.hasFlag("NoTieToGround"))
                  {
                     mLockToTerrain = false;
                     mAbstractData.LockToTerrain = false;
                  }
                  else
                  {
                     mLockToTerrain = true;
                     mAbstractData.LockToTerrain = true;
                  }

                  if (mProtoObject.hasFlag("ForceToGaiaPlayer"))
                  {
                     mAbstractData.mbForcePlayerZero = true;
                  }
                  else
                  {
                     mAbstractData.mbForcePlayerZero = false;
                  }

                  if (mProtoObject.hasFlag("OrientUnitWithGround"))
                  {
                     mAbstractData.mAlignToTerrain = true;
                  }
                  else
                  {
                     mAbstractData.mAlignToTerrain = false;
                  }
               }
               initVisuals();
            }
         }
         get
         {
            return mProtoObject;
         }
      }

      ProtoSquadXml mProtoSquad = null;
      public ProtoSquadXml ProtoSquad
      {
         set
         {
            mProtoSquad = value;

            if (mProtoSquad != null)
            {
               //if (mProtoSquad.hasFlag("NoTieToGround"))
               //{
               //   mLockToTerrain = false;
               //   mAbstractData.LockToTerrain = false;
               //}
               //else
               //{
               mLockToTerrain = true;
               mAbstractData.LockToTerrain = true;
               
               //if (mProtoSquad.hasFlag("OrientUnitWithGround"))
               //{
               //   mAbstractData.mAlignToTerrain = true;
               //}
               //else
               //{
               //   mAbstractData.mAlignToTerrain = false;
               //}

               initVisuals();
            }
         }
         get
         {
            return mProtoSquad;
         }
      }


      public bool mbForcePlayerZero = false;

      private SimObjectData mAbstractData=null;

      protected BRenderDebugCube mAABBVis = null;
      BRenderDebugOOBB mOOBBVis = null;
      public BOrientedBoundingBox mOBB = new BOrientedBoundingBox();
 
 
      public bool IgnoreToAO
      {
         get
         {
            return mAbstractData.IgnoreToAO;
         }
      }
      public bool IncludeInSimRep
      {
         get
         {
            return mAbstractData.IncludeInSimRep;
         }
      }
      public int VisualVariationIndex
      {
         get { return mAbstractData.VisualVariationIndex; }
      }

   }

   public class PlayerPosition : SimObject, ISupportsScenarioOwner
   {

      public PlayerPosition()
      {
         PlayerPositionXML = new PlayerPositionXML();
         PlayerID = 0;
      }

      void mPlayerPositionXML_NumberChanged(object sender, EventArgs e)
      {
         initVisuals();
      }

      public PlayerPosition(int id)
      {
         PlayerPositionXML = new PlayerPositionXML();
         PlayerID = id;

      }

      public override void initVisuals()
      {
         try
         {
            mMeshes.Clear();
            string resource = string.Format(@"models\playerstart\playerstart_0{0}.gr2", mPlayerPositionXML.mNumber);
            resource = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mBaseDirectory, resource);
            if (File.Exists(resource))
            {
               /*
               BRenderGrannyMesh mesh = GrannyManager.getOrLoadGR2(resource);
               if (mesh != null)
                  mMeshes.Add(mesh);
               */
               mVisual = BVisualManager.getOrLoadVisualGR2Only(resource);
            }
            updateBB();
            mAABBVis = new BRenderDebugCube(mAABB.min, mAABB.max, (int)System.Drawing.Color.Yellow.ToArgb(), false);
         }
         catch
         {
         }
      }
      /*
      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 coord = Vector3.Empty;
         float t = 0;
         Vector3 tOrg = r0 - getPosition();

         return BMathLib.ray3AABB(ref coord, ref t, ref tOrg, ref rD, ref mAABB.min, ref mAABB.max);
      }
      */
      BRenderDebugCube mFacingRect = null;
      public override void render()
      {
         if (mFacingRect == null)
            mFacingRect = new BRenderDebugCube(new Vector3(0, 12, 0), new Vector3(6, 12.1f, 0.1f), (int)System.Drawing.Color.Blue.ToArgb(), false);

         BRenderDevice.getDevice().Transform.World = mMatrix;// this.getMatrix();
         mVisual.render(Vector3.Empty, false);

         mFacingRect.render();
      }

      public override void renderSelected()
      {
         if (mAABBVis == null)
            mAABBVis = new BRenderDebugCube(new Vector3(-1, -1, -1), new Vector3(1, 1, 1), (int)System.Drawing.Color.Yellow.ToArgb(), false);
         BRenderDevice.getDevice().Transform.World = mMatrix;
         
         mAABBVis.render();


      }

      public override string ToString()
      {
         return "Playerstart";// +PlayerID.ToString();
      }
      public override string GetTypeName()
      {
         return "Playerstart";
      }
      public override string GetName()
      {
         return "Playerstart";
      }
      public override string Name
      {
         get
         {
            return "Playerstart";
         }
      }
      public override EditorObject Copy()
      {
         PlayerPosition obj = new PlayerPosition(mPlayerPositionXML.mNumber);
         return obj;

      }

      public override object GetProperties()
      {
         return mPlayerPositionXML;
      }

      //work\art\tool\scenario\playerstart
      //playerstart_01.gr2
      private PlayerPositionXML mPlayerPositionXML;
      public PlayerPositionXML PlayerPositionXML 
      {
         set
         {
            mPlayerPositionXML = value;
            mPlayerPositionXML.NumberChanged += new EventHandler(mPlayerPositionXML_NumberChanged);

         }
         get
         {
            return mPlayerPositionXML;
         }
      }

      public int PlayerID
      {
         set
         {
            mPlayerPositionXML.mNumber = value;
            initVisuals();
         }
         get
         {
            return mPlayerPositionXML.mNumber;
         }
      }




      public override int GetDepartment()      
      {
         return (int)(eDepartment.Design);
      }


      #region ISupportsScenarioOwner Members

      int ISupportsScenarioOwner.Department
      {
         get
         {
            return GetDepartment(); 
         }
         set
         {
            //Const
         }
      }

      #endregion
   }
   
   public class SimHelperObject : EditorObject, IHasID, IHasGroup
   {
      public SimHelperObject()
      {
         mDepartmentVisPermission = (int)(eDepartment.Design);
      }


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
      protected string mName = ""; 
      
      int mID = -1;
      public int ID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
            if(mName == "")
            {
               mName = mTypeName + ID.ToString();
            }
         }
      }
      int mGroup = -1;
      public int Group
      {
         get
         {
            return mGroup;
         }
         set
         {
            mGroup = value;
         }
      }
      //public override string EditorName
      //{
      //   get
      //   {
      //      return Name;
      //   }
      //}

      protected string mTypeName = "SimHelperObject";
      //[XmlIgnore]
      //public string TypeName
      //{
      //   get
      //   {
      //      return mTypeName;
      //   }
      //   set
      //   {
      //      mTypeName = value;
      //   }
      //}




      public override int GetDepartment()
      {
         return (int)eDepartment.Design;
      }

      
   }

   
   public class HelperAreaObject : SimHelperObject
   {
      public HelperAreaObject() 
      {
         AreaColor = System.Drawing.Color.Blue;
         initVisuals();
         mTypeName = "Area";
      }

      public override string ToString()
      {
         return GetTypeName();
      }
      public override string GetName()
      {
         return Name;//ugh
      }
      public override string GetTypeName()
      {
         return mTypeName;
      }

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

      protected int mColor = 0;//(0x00FFFFFF);
      System.Drawing.Color mAreaColor;
      [XmlIgnore]
      public System.Drawing.Color AreaColor
      {
         get
         {
            return System.Drawing.Color.FromArgb((int)(mColor | 0xFF000000));// & 0x00FFFFFF);
         }
         set
         {
            mAreaColor = value;
            mColor = mAreaColor.ToArgb() + 0x44000000;

            mbChanged = true;
         }
      }
      int mRadius = 3;
      public int Radius
      {
         get
         {
            return mRadius;
         }
         set
         {
            mRadius = value;
            mbChanged = true;
         }

      }

      //for xml i/o
      public string Position
      {
         get
         {            
            return TextVectorHelper.ToString(this.getPosition());
         }
         set
         {
            this.setPosition( TextVectorHelper.FromString(value) );
            mbChanged = true;
         }
      }

      public string Direction
      {
         get
         {
            Vector3 direction = this.getDirection();
            direction.Y = 0.0f;
            direction = BMathLib.Normalize(direction);            
            return (TextVectorHelper.ToString(direction));
         }
         set
         {
            Vector3 direction = TextVectorHelper.FromString(value);
            direction.Y = 0.0f;
            direction = BMathLib.Normalize(direction);
            this.setDirection(direction);
            mbChanged = true;
         }
      }

      public string Color
      {
         get
         {
            return TextColorHelper.ToString(this.AreaColor);
         }
         set
         {
            this.AreaColor = TextColorHelper.FromString(value);
            mbChanged = true;
         }
      }


      bool mbChanged = true;

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mRadius;
         mAABB.min.Y = -mRadius;
         mAABB.min.Z = -mRadius;

         mAABB.max.X = mRadius;
         mAABB.max.Y = mRadius;
         mAABB.max.Z = mRadius;

      }
      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();

         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(getPosition(), mRadius, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }


      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(mRadius, 2, mColor);

         int red = (int)System.Drawing.Color.Red.ToArgb();
         red &= 0x00FFFFFF;
         red += 0x44000000;
         mFacingRect = new BRenderDebugCube(new Vector3(-0.05f, 0.0f, 0.0f), new Vector3(0.05f, 0.1f, mRadius + 3.0f), red, false);

         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();
         mAxisCircle = new BRenderDebug2DCircle(mRadius, mColor);
         Vector3 top = new Vector3(0, mRadius, 0);// getPosition();
         Vector3 bottom = new Vector3(0, -mRadius, 0);// getPosition();
         mCenterMarker = new BRenderDebugCube(top, bottom, mColor, false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if(mAxisCircle != null)
            mAxisCircle.destroy();
         if(mLocSphere != null)
            mLocSphere.destroy();
         if (mCenterMarker != null)
            mCenterMarker.destroy();

         if (mFacingRect != null)
         {
            mFacingRect.destroy();
            mFacingRect = null;
         }
      }
      
      public override void render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }

         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mCenterMarker.render();


         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();         

         BRenderDevice.getDevice().Transform.World = mMatrix;
         mFacingRect.mSolid = false;
         mFacingRect.render();
      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }

         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mLocSphere.render(false,true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();

         BRenderDevice.getDevice().Transform.World = mMatrix;         
         mFacingRect.mSolid = true;
         mFacingRect.render(true);

         //if (mAABBVis == null)
         //   mAABBVis = new BRenderDebugCube(new Vector3(-1, -1, -1), new Vector3(1, 1, 1), (int)System.Drawing.Color.Yellow.ToArgb(), false);
         //BRenderDevice.getDevice().Transform.World = mMatrix;
         //mAABBVis.render();
      }


      BRenderDebugCube mCenterMarker = null;
      BRenderDebugCube mAABBVis = null;
      BRenderDebugSphere mLocSphere = null;
      BRenderDebug2DCircle mAxisCircle = null;
      BRenderDebugCube mFacingRect = null;

      public override EditorObject Copy()
      {
         HelperAreaObject beacon = new HelperAreaObject();
         beacon.Name = this.Name;
         beacon.Radius = this.Radius;

         beacon.ID = SimGlobals.getSimMain().GetUniqueID();


         return beacon;
         //return (EditorObject)this.MemberwiseClone();
      }
      public override object GetProperties()
      {
         return this;
      }


   }

   public class HelperAreaBoxObject : SimHelperObject
   {
      public HelperAreaBoxObject()
      {
         AreaColor = System.Drawing.Color.Red;
         initVisuals();
         mTypeName = "AreaBox";
      }

      public override string ToString()
      {
         return GetTypeName();
      }

      public override string GetName()
      {
         return Name;//ugh
      }

      public override string GetTypeName()
      {
         return mTypeName;
      }

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

      protected int mColor = 0;
      System.Drawing.Color mAreaColor;
      [XmlIgnore]
      public System.Drawing.Color AreaColor
      {
         get
         {
            return System.Drawing.Color.FromArgb((int)(mColor | 0xFF000000));
         }

         set
         {
            mAreaColor = value;
            mColor = mAreaColor.ToArgb() + 0x44000000;
            mbChanged = true;
         }
      }

      int mHalfLengthX = 3;
      public int HalfLengthX
      {
         get
         {
            return (mHalfLengthX);
         }

         set
         {
            mHalfLengthX = value;
            mLengthX = mHalfLengthX * 2;
            mbChanged = true;
         }
      }

      int mHalfLengthY = 3;
      public int HalfLengthY
      {
         get
         {
            return (mHalfLengthY);
         }

         set
         {
            mHalfLengthY = value;
            mLengthY = mHalfLengthY * 2;
            mbChanged = true;
         }
      }

      int mHalfLengthZ = 3;
      public int HalfLengthZ
      {
         get
         {
            return (mHalfLengthZ);
         }

         set
         {
            mHalfLengthZ = value;
            mLengthZ = mHalfLengthZ * 2;
            mbChanged = true;
         }
      }

      int mLengthX = 6;
      public int LengthX
      {
         get
         {
            return (mLengthX);
         }

         set
         {
            mLengthX = value;
            mHalfLengthX = mLengthX / 2;
            mbChanged = true;
         }
      }

      int mLengthY = 6;
      public int LengthY
      {
         get
         {
            return (mLengthY);
         }

         set
         {
            mLengthY = value;
            mHalfLengthY = mLengthY / 2;
            mbChanged = true;
         }
      }

      int mLengthZ = 3;
      public int LengthZ
      {
         get
         {
            return (mLengthZ);
         }

         set
         {
            mLengthZ = value;
            mHalfLengthZ = mLengthZ / 2;
            mbChanged = true;
         }
      }

      //for xml i/o
      public string Position
      {
         get
         {
            return TextVectorHelper.ToString(this.getPosition());
         }

         set
         {
            this.setPosition(TextVectorHelper.FromString(value));
            mbChanged = true;
         }
      }

      public string Direction
      {
         get
         {
            Vector3 direction = this.getDirection();
            direction.Y = 0.0f;
            direction = BMathLib.Normalize(direction);
            return (TextVectorHelper.ToString(direction));
         }

         set
         {
            Vector3 direction = TextVectorHelper.FromString(value);
            direction.Y = 0.0f;
            direction = BMathLib.Normalize(direction);
            this.setDirection(direction);
            mbChanged = true;
         }
      }

      public string Color
      {
         get
         {
            return (TextColorHelper.ToString(this.AreaColor));
         }

         set
         {
            this.AreaColor = TextColorHelper.FromString(value);
            mbChanged = true;
         }
      }

      public string Extent1
      {
         get
         {
            Microsoft.DirectX.Vector3 min = this.getPosition();
            min.X -= (float)mHalfLengthX;
            min.Y -= (float)mHalfLengthY;
            min.Z -= (float)mHalfLengthZ;
            return TextVectorHelper.ToString(min);
         }
      }

      public string Extent2
      {
         get
         {
            Microsoft.DirectX.Vector3 max = this.getPosition();
            max.X += (float)mHalfLengthX;
            max.Y += (float)mHalfLengthY;
            max.Z += (float)mHalfLengthZ;
            return TextVectorHelper.ToString(max);
         }
      }

      bool mbChanged = true;

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mHalfLengthX;
         mAABB.min.Y = -mHalfLengthY;
         mAABB.min.Z = -mHalfLengthZ;

         mAABB.max.X = mHalfLengthX;
         mAABB.max.Y = mHalfLengthY;
         mAABB.max.Z = mHalfLengthZ;
      }

      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         Quaternion quat = Quaternion.RotationMatrix(this.getRotationOnly());         
         return (BMathLib.rayOOBBIntersect(new Vector3(0.0f, 0.0f, 0.0f), new Vector3(mHalfLengthX, mHalfLengthY, mHalfLengthZ), quat, kOrg, rD, ref tVal));
      }

      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect(mAABB.min + pos, mAABB.max + pos, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }

      public override void initVisuals()
      {
         mAreaBox = new BRenderDebugCube(new Vector3(-mHalfLengthX, -mHalfLengthY, -mHalfLengthZ), new Vector3(mHalfLengthX, mHalfLengthY, mHalfLengthZ), mColor, false);

         int color = (int)System.Drawing.Color.Green.ToArgb();
         color &= 0x00FFFFFF;
         color += 0x44000000;
         mFacingRect = new BRenderDebugCube(new Vector3(-0.05f, 0.0f, 0.0f), new Vector3(0.05f, 0.1f, mHalfLengthZ + 3.0f), color, false);

         initCube();
      }

      public override void clearVisuals()
      {
         clearCube();
      }

      public void initCube()
      {
         updateBB();
      }

      public void clearCube()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }

         if (mAreaBox != null)
         {
            mAreaBox.destroy();
            mAreaBox = null;
         }

         if (mFacingRect != null)
         {
            mFacingRect.destroy();
            mFacingRect = null;
         }
      }
      
      public override void render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = mMatrix;
         mAreaBox.mSolid = false;
         mAreaBox.render();


         BRenderDevice.getDevice().Transform.World = mMatrix;
         mFacingRect.mSolid = false;         
         mFacingRect.render();
      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = mMatrix;
         mAreaBox.mSolid = true;
         mAreaBox.render(true);

         BRenderDevice.getDevice().Transform.World = mMatrix;
         mFacingRect.mSolid = true;
         mFacingRect.render(true);
      }

      BRenderDebugCube mAABBVis = null;
      BRenderDebugCube mAreaBox = null;
      BRenderDebugCube mFacingRect = null;

      public override EditorObject Copy()
      {
         HelperAreaBoxObject beacon = new HelperAreaBoxObject();
         beacon.Name = this.Name;
         beacon.HalfLengthX = this.HalfLengthX;
         beacon.HalfLengthY = this.HalfLengthY;
         beacon.HalfLengthZ = this.HalfLengthZ;
         beacon.ID = SimGlobals.getSimMain().GetUniqueID();

         return (beacon);
      }

      public override object GetProperties()
      {
         return (this);
      }
   }

   public class HelperPositionObject : SimHelperObject
   {
      public HelperPositionObject() 
      {
         mColor = unchecked((int)0xAAFF8000);
         mRadius = 2;
         initVisuals();
         mTypeName = "Position";
         mLockToTerrain = false;
      }

      public override string ToString()
      {
         return GetTypeName();
      }
      public override string GetName()
      {
         return Name;//ugh
      }
      public override string GetTypeName()
      {
         return mTypeName;
      }

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

      protected int mColor = 0;//(0x00FFFFFF);

      int mRadius = 3;


      //for xml i/o
      public string Position
      {
         get
         {            
            return TextVectorHelper.ToString(this.getPosition());
         }
         set
         {
            this.setPosition( TextVectorHelper.FromString(value) );
            mbChanged = true;
         }
      }

      bool mbChanged = true;

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mRadius;
         mAABB.min.Y = -mRadius;
         mAABB.min.Z = -mRadius;

         mAABB.max.X = mRadius;
         mAABB.max.Y = mRadius;
         mAABB.max.Z = mRadius;

      }
      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();

         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(getPosition(), mRadius, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }


      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(mRadius, 2, mColor);
         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();
         mAxisCircle = new BRenderDebug2DCircle(mRadius, mColor);
         Vector3 top = new Vector3(0, mRadius, 0);// getPosition();
         Vector3 bottom = new Vector3(0, -mRadius, 0);// getPosition();
         mCenterMarker = new BRenderDebugCube(top, bottom, mColor, false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if(mAxisCircle != null)
            mAxisCircle.destroy();
         if(mLocSphere != null)
            mLocSphere.destroy();
         if (mCenterMarker != null)
            mCenterMarker.destroy();
      }


      public override void render()
      {

         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());

         mCenterMarker.render();

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;


         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();

      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());


         mLocSphere.render(false,true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;


         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();
         
         //if (mAABBVis == null)
         //   mAABBVis = new BRenderDebugCube(new Vector3(-1, -1, -1), new Vector3(1, 1, 1), (int)System.Drawing.Color.Yellow.ToArgb(), false);
         //BRenderDevice.getDevice().Transform.World = mMatrix;
         //mAABBVis.render();
      }


      BRenderDebugCube mCenterMarker = null;
      BRenderDebugCube mAABBVis = null;
      BRenderDebugSphere mLocSphere = null;
      BRenderDebug2DCircle mAxisCircle = null;

      public override EditorObject Copy()
      {
         HelperPositionObject beacon = new HelperPositionObject();
         beacon.Name = this.Name;
         
         beacon.ID = SimGlobals.getSimMain().GetUniqueID();


         return beacon;
         //return (EditorObject)this.MemberwiseClone();
      }
      public override object GetProperties()
      {
         return this;
      }


   }


   //public class 

   //Game design objects

   public class GameDesignObject : EditorObject, IHasID
   {
      public GameDesignObject()
      {
         mDepartmentVisPermission = (int)(eDepartment.Design);
      }

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
      protected string mName = "";

      int mID = -1;
      public int ID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
            if (mName == "")
            {
               mName = mTypeName + ID.ToString();
            }
         }
      }
      int mGroup = -1;
      public int Group
      {
         get
         {
            return mGroup;
         }
         set
         {
            mGroup = value;
         }
      }
      protected string mTypeName = "GameDesignObject";

      protected DesignerData mDesignerData = new DesignerData();

      public DesignerData DesignerData
      {
         get
         {
            return mDesignerData;
         }
         set
         {
            mDesignerData = value;
         }
      }


      public override int GetDepartment()
      {
         return (int)eDepartment.Design;
      }

      
   }

   public class GameDesignValueSphere : GameDesignValuePoint
   {
      public GameDesignValueSphere() 
      {

         Color = System.Drawing.Color.Blue;
         initVisuals();
         mTypeName = "DesignSphere";

         mRadius = 4;
      }
      public override EditorObject Copy()
      {
         GameDesignValueSphere obj = new GameDesignValueSphere();
         obj.Name = this.Name;
         obj.Radius = this.Radius;
         obj.ID = SimGlobals.getSimMain().GetUniqueID();
         return obj;
      }

      public int Radius
      {
         get
         {
            return mRadius;
         }
         set
         {
            mRadius = value;
            mbChanged = true;
         }
      }

      public DesignSphereXML ToXML()
      {
         DesignSphereXML data = new DesignSphereXML();
         data.Name = this.Name;
         data.Position = TextVectorHelper.ToString(this.getPosition());
         data.Radius = this.Radius;
         data.Data = this.mDesignerData;
         data.ID = this.ID;

         return data;
      }
      public void FromXML(DesignSphereXML data)
      {
         this.Name = data.Name;
         this.setPosition(TextVectorHelper.FromString(data.Position));
         this.Radius = (int)data.Radius;
         this.mDesignerData = data.Data;
         this.ID = data.ID;
      }
   }

   public class GameDesignValuePoint : GameDesignObject
   {
      public GameDesignValuePoint() 
      {
         Color = System.Drawing.Color.Yellow;
         initVisuals();
         mTypeName = "DesignPoint";
      }
      public override string ToString()
      {
         return GetTypeName();
      }
      public override string GetName()
      {
         return Name;//ugh
      }
      public override string GetTypeName()
      {
         return mTypeName;
      }
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
   
      int mColor = 0;//(0x00FFFFFF);
      System.Drawing.Color mAreaColor;
      [XmlIgnore]
      public System.Drawing.Color Color
      {
         get
         {
            return System.Drawing.Color.FromArgb((int)(mColor | 0xFF000000));// & 0x00FFFFFF);
         }
         set
         {
            mAreaColor = value;
            mColor = mAreaColor.ToArgb() + 0x44000000;
            mbChanged = true;
         }
      }
      protected int mRadius = 2;


      //for xml i/o
      public string Position
      {
         get
         {            
            return TextVectorHelper.ToString(this.getPosition());
         }
         set
         {
            this.setPosition( TextVectorHelper.FromString(value) );
            mbChanged = true;
         }
      }
      public string XMLColor
      {
         get
         {
            return TextColorHelper.ToString(this.Color);
         }
         set
         {
            this.Color = TextColorHelper.FromString(value);
            mbChanged = true;
         }
      }


      protected bool mbChanged = true;

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mRadius;
         mAABB.min.Y = -mRadius;
         mAABB.min.Z = -mRadius;

         mAABB.max.X = mRadius;
         mAABB.max.Y = mRadius;
         mAABB.max.Z = mRadius;

      }
      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(getPosition(), mRadius, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }


      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(mRadius, 2, mColor);
         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();
         mAxisCircle = new BRenderDebug2DCircle(mRadius, mColor);
         Vector3 top = new Vector3(0, mRadius, 0);// getPosition();
         Vector3 bottom = new Vector3(0, -mRadius, 0);// getPosition();
         mCenterMarker = new BRenderDebugCube(top, bottom, mColor, false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if(mAxisCircle != null)
            mAxisCircle.destroy();
         if(mLocSphere != null)
            mLocSphere.destroy();
         if (mCenterMarker != null)
            mCenterMarker.destroy();
      }


      public override void render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mCenterMarker.render();
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)-45)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)45)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();
      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());


         mLocSphere.render(false,true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();         
      }

      BRenderDebugCube mCenterMarker = null;
      BRenderDebugCube mAABBVis = null;
      BRenderDebugSphere mLocSphere = null;
      BRenderDebug2DCircle mAxisCircle = null;

      public override EditorObject Copy()
      {

         GameDesignValuePoint obj = new GameDesignValuePoint();
         obj.Name = this.Name;
         //obj.Radius = this.Radius;
         //obj.ID = SimGlobals.getSimMain().GetUniqueID();
         return obj;

      }
      public override object GetProperties()
      {
         return this;
      }
      
   }

   public class GameDesignLine : GameDesignObject, IPointOwner, IHasID
   {
      public GameDesignLine() 
      {
         Color = System.Drawing.Color.Blue;
         initVisuals();
         mTypeName = "DesignPath";
      }

      public DesignLineXML ToXML()
      {
         DesignLineXML data = new DesignLineXML();
         data.Name = this.Name;
         data.Position = TextVectorHelper.ToString(this.getPosition());
         //data.Radius = this.Radius;
         data.ID = this.ID;

         foreach (EditorObject obj in mPoints)
         {
            data.mPoints.Add(obj.getPosition());
         }

         data.Data = this.mDesignerData;

         return data;
      }
      public void FromXML(DesignLineXML data)
      {
         this.Name = data.Name;
         this.setPosition(TextVectorHelper.FromString(data.Position));
         //this.Radius = (int)data.Radius;
         this.mDesignerData = data.Data;
         this.ID = data.ID;

         if (String.Compare(this.mDesignerData.Type, "CameraBoundary-HoverAndCamera",true) == 0)
         {
            this.mDesignerData.Type = "CameraBoundary-Camera";
         }
      }


      public override string ToString()
      {
         return GetTypeName();
      }
      public override string GetName()
      {
         return Name;//ugh
      }
      public override string GetTypeName()
      {
         return mTypeName;
      }
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
   
      int mColor = 0;//(0x00FFFFFF);
      System.Drawing.Color mAreaColor;
      [XmlIgnore]
      public System.Drawing.Color Color
      {
         get
         {
            return System.Drawing.Color.FromArgb((int)(mColor | 0xFF000000));// & 0x00FFFFFF);
         }
         set
         {
            mAreaColor = value;
            mColor = mAreaColor.ToArgb() + 0x44000000;
            mbChanged = true;
         }
      }
      int mRadius = 3;
      //public int Radius
      //{
      //   get
      //   {
      //      return mRadius;
      //   }
      //   set
      //   {
      //      mRadius = value;
      //      mbChanged = true;
      //   }
      //}

      //for xml i/o
      public string Position
      {
         get
         {            
            return TextVectorHelper.ToString(this.getPosition());
         }
         set
         {
            this.setPosition( TextVectorHelper.FromString(value) );
            mbChanged = true;
         }
      }
      public string XMLColor
      {
         get
         {
            return TextColorHelper.ToString(this.Color);
         }
         set
         {
            this.Color = TextColorHelper.FromString(value);
            mbChanged = true;
         }
      }


      bool mbChanged = true;

      public override void updateBB()
      {
         mAABB.min.X = 0.0f;
         mAABB.min.Y = 0.0f;
         mAABB.min.Z = 0.0f;
         mAABB.max.X = 0.0f;
         mAABB.max.Y = 0.0f;
         mAABB.max.Z = 0.0f;

         if (mPoints.Count >= 1)
         {
            Vector3 orig = getPosition();

            foreach (BasicPoint point in mPoints)
            {
               Vector3 pos = point.getPosition() - orig;

               if (pos.X < mAABB.min.X) mAABB.min.X = pos.X;
               if (pos.Y < mAABB.min.Y) mAABB.min.Y = pos.Y;
               if (pos.Z < mAABB.min.Z) mAABB.min.Z = pos.Z;
               if (pos.X > mAABB.max.X) mAABB.max.X = pos.X;
               if (pos.Y > mAABB.max.Y) mAABB.max.Y = pos.Y;
               if (pos.Z > mAABB.max.Z) mAABB.max.Z = pos.Z;
            }
         }

         mAABB.min.X -= mRadius;
         mAABB.min.Y -= mRadius;
         mAABB.min.Z -= mRadius;

         mAABB.max.X += mRadius;
         mAABB.max.Y += mRadius;
         mAABB.max.Z += mRadius;
      }

      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(getPosition(), mRadius, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }

      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(mRadius, 2, mColor);
         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();
         mAxisCircle = new BRenderDebug2DCircle(mRadius, mColor);
         Vector3 top = new Vector3(0, mRadius, 0);// getPosition();
         Vector3 bottom = new Vector3(0, -mRadius, 0);// getPosition();
         mCenterMarker = new BRenderDebugCube(top, bottom, mColor, false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if(mAxisCircle != null)
            mAxisCircle.destroy();
         if(mLocSphere != null)
            mLocSphere.destroy();
         if (mCenterMarker != null)
            mCenterMarker.destroy();
      }

      public override void render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mCenterMarker.render();
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         //BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)-45)) * Matrix.Translation(getPosition());
         //mAxisCircle.render();
         //BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)45)) * Matrix.Translation(getPosition());
         //mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();



         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         if (mPoints.Count > 0)
         {
            VertexTypes.Pos[] verts = new VertexTypes.Pos[mPoints.Count + 1];
            Vector3 startPos = this.getPosition();
            verts[0].x = startPos.X;
            verts[0].y = startPos.Y + 3f;
            verts[0].z = startPos.Z;

            int i = 1;
            int color = System.Drawing.Color.Red.ToArgb();
            foreach (BasicPoint point in mPoints)
            {
               Vector3 pos = point.getPosition();
               verts[i].x = pos.X;
               verts[i].y = pos.Y + 3f;
               verts[i].z = pos.Z;
               //verts[i].Color = color;
               i++;
            }
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, color);

            BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos.FVF_Flags;
            BRenderDevice.getDevice().DrawUserPrimitives(PrimitiveType.LineStrip, mPoints.Count, verts);
         }
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());


         mLocSphere.render(false,true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();         
      }

      BRenderDebugCube mCenterMarker = null;
      BRenderDebugCube mAABBVis = null;
      BRenderDebugSphere mLocSphere = null;
      BRenderDebug2DCircle mAxisCircle = null;

      public override EditorObject Copy()
      {
         GameDesignLine obj = new GameDesignLine();
         obj.Name = this.Name;
         //obj.Radius = this.Radius;
         obj.ID = SimGlobals.getSimMain().GetUniqueID();
         return obj;
      }
      public override object GetProperties()
      {
         return this;
      }

      public List<BasicPoint> mPoints = new List<BasicPoint>();

      public override void OnPlaced()
      {
         BasicPoint p = new BasicPoint();
         p.mOwner = this;
         SimGlobals.getSimMain().PlacementDummyObject = p;
      }
      public override bool IsSelectable()
      {
         return true;
      }
      public override bool IsVisible()
      {
         return true;
      }

      public void PointPlaced(BasicPoint point)
      {
         if (point.PlaceAt == -1)
         {
            mPoints.Add(point);
         }
         else
         {
            mPoints.Insert(point.PlaceAt, point);
            point.PlaceAt = -1;
         }

         point.mVisible = false;

         updateBB();
      }
      public void PointPlacedAt(BasicPoint point, int index)
      {
         mPoints.Insert(index, point);
         point.mVisible = false;

         updateBB();
      }
      public void PointMoved(BasicPoint point)
      {
         updateBB();
      }
      public void PointRemoved(BasicPoint point)
      {
         mPoints.Remove(point);

         updateBB();
      }


      public override bool AutoSelectChildComponents()
      {
         return false;
      }

      public override List<EditorObject> getChildComponents()
      {
         List<EditorObject> blah = new List<EditorObject>();
         //blah.AddRange(mPoints<EditorObject>.ToArray());//<EditorObject>
         foreach (BasicPoint point in mPoints)
         {
            blah.Add(point);
         }
         return blah;
      }


   }
   public interface IPointOwner
   {
      void PointPlaced(BasicPoint point);
      void PointMoved(BasicPoint point);
      void PointRemoved(BasicPoint point);
   }
   //  

   public interface ISubControl
   {
      IPointOwner GetOwner();
   }

   public class BasicPoint : EditorObject, ISubControl
   {
      string mTypeName;
      string mName;
      int    mPlaceAt;  // specifies where the point needs to be place in the line (use -1 when adding
                        // to the end of the line).  This is only used when placing the point and is 
                        // meaningless at any other time.

      public BasicPoint()
      {
         Color = System.Drawing.Color.Blue;
         initVisuals();
         mTypeName = "BasicPoint";
         mPlaceAt = -1;

         mDepartmentVisPermission = (int)(eDepartment.Design );
      }

      public IPointOwner mOwner = null;
      public IPointOwner GetOwner() { return mOwner; }

      //insert to current object
      public override void OnPlaced()
      {
         //base.OnPlaced();
         if (mOwner != null)
         {
            mOwner.PointPlaced(this);
         }
      }
      public override bool IsSelectable()
      {
         return true;
      }
      public bool mVisible = true;
      public override bool IsVisible()
      {
         return mVisible;
      }

      public override void moved()
      {
         if (mOwner != null)
         {
            mOwner.PointMoved(this);
         }
      }
      public override void removed()
      {
         if (mOwner != null)
         {
            mOwner.PointRemoved(this);
         }
      }



      public override string ToString()
      {
         return GetTypeName();
      }
      public override string GetName()
      {
         return Name;//ugh
      }
      public override string GetTypeName()
      {
         return mTypeName;
      }
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

      public int PlaceAt
      {
         get
         {
            return mPlaceAt;
         }
         set
         {
            mPlaceAt = value;
         }
      }

      int mColor = 0;//(0x00FFFFFF);
      System.Drawing.Color mAreaColor;
      [XmlIgnore]
      public System.Drawing.Color Color
      {
         get
         {
            return System.Drawing.Color.FromArgb((int)(mColor | 0xFF000000));// & 0x00FFFFFF);
         }
         set
         {
            mAreaColor = value;
            mColor = mAreaColor.ToArgb() + 0x44000000;
            mbChanged = true;
         }
      }
      int mRadius = 3;
      public int Radius
      {
         get
         {
            return mRadius;
         }
         set
         {
            mRadius = value;
            mbChanged = true;
         }
      }

      //for xml i/o
      public string Position
      {
         get
         {
            return TextVectorHelper.ToString(this.getPosition());
         }
         set
         {
            this.setPosition(TextVectorHelper.FromString(value));
            mbChanged = true;
         }
      }
      public string XMLColor
      {
         get
         {
            return TextColorHelper.ToString(this.Color);
         }
         set
         {
            this.Color = TextColorHelper.FromString(value);
            mbChanged = true;
         }
      }


      bool mbChanged = true;

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -mRadius;
         mAABB.min.Y = -mRadius;
         mAABB.min.Z = -mRadius;

         mAABB.max.X = mRadius;
         mAABB.max.Y = mRadius;
         mAABB.max.Z = mRadius;

      }
      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(getPosition(), mRadius, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }


      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(mRadius, 2, mColor);
         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();
         mAxisCircle = new BRenderDebug2DCircle(mRadius, mColor);
         Vector3 top = new Vector3(0, mRadius, 0);// getPosition();
         Vector3 bottom = new Vector3(0, -mRadius, 0);// getPosition();
         mCenterMarker = new BRenderDebugCube(top, bottom, mColor, false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         if (mAxisCircle != null)
            mAxisCircle.destroy();
         if (mLocSphere != null)
            mLocSphere.destroy();
         if (mCenterMarker != null)
            mCenterMarker.destroy();
      }


      public override void render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mCenterMarker.render();
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();
      }

      public override void renderSelected()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());


         mLocSphere.render(false, true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircle.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircle.render();
      }

      BRenderDebugCube mCenterMarker = null;
      BRenderDebugCube mAABBVis = null;
      BRenderDebugSphere mLocSphere = null;
      BRenderDebug2DCircle mAxisCircle = null;

      public override EditorObject Copy()
      {
         BasicPoint obj = new BasicPoint();
         obj.Name = this.Name;
         obj.mOwner = this.mOwner;
         obj.Radius = this.Radius;
         //obj.ID = SimGlobals.getSimMain().GetUniqueID();
         return obj;
      }
      public override object GetProperties()
      {
         return this;
      }


      public override int GetDepartment()
      {
         return (int)eDepartment.Design;
      }

      
   } 


   //------------------------------------------
   //------------------------------------------
   //------------------------------------------
   public class LocalLight : EditorObject
   {
      public LocalLight()
      {
         mDepartmentVisPermission = (int)(eDepartment.Art);
      }

      public LightXML LightData
      {
         set
         {
            mLightXML = value;
         }
         get
         {
            if(mLightXML.InnerAngle > mLightXML.OuterAngle)
            {
               float temp = mLightXML.InnerAngle;
               mLightXML.InnerAngle = mLightXML.OuterAngle;
               mLightXML.OuterAngle = temp;
            }


            return mLightXML;
         }

      }
      protected LightXML mLightXML = new LightXML();



  
      public override int GetDepartment()
      {
         return (int)eDepartment.Art;
      }

      
   }

   //CLM - this is a dummy object just for spotlight placement when it's on your mouse cursor
   public class SpotLightPlacement : EditorObject
   {
     

      public SpotLightPlacement()
      {
         updateBB();
         initVisuals();

         mDepartmentVisPermission = (int)(eDepartment.Art);
      }
      ~SpotLightPlacement()
      {
         clearVisuals();
      }

      public override void initVisuals()
      {
         mBoundingConeOuterAngle = new BRenderDebugCone(1.0f, 0.5f, System.Drawing.Color.White.ToArgb(), false, false);
      }
      public override void clearVisuals()
      {
         mBoundingConeOuterAngle.destroy();
      }
      public override void render()
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mBoundingConeOuterAngle.render();
      }
      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -1;
         mAABB.min.Y = -1;
         mAABB.min.Z = -1;

         mAABB.max.X = 1;
         mAABB.max.Y = 1;
         mAABB.max.Z = 1;
      }
      BRenderDebugCone mBoundingConeOuterAngle;


 
      public override  int GetDepartment()
      {
         return (int)eDepartment.Art;
      }

      
   }

   public class SpotLight : LocalLight
   {

      public SpotLight(LightXML lightData)
      {
         LoadFromXML(lightData);
      }
      public void LoadFromXML(LightXML lightData)
      {
         mLockToTerrain = false;
     
         //mLightXML.mType = "spot";
         //mLightXML.mColor = lightData.mColor;
         //mLightXML.mRadius = lightData.Radius;
         //mLightXML.mPosition = lightData.mPosition;

         //mLightXML.mDirection = lightData.mDirection;


         LightData = (LightXML)lightData.Clone();

         
         mDirection = TextVectorHelper.FromString(mLightXML.mDirection);

         calcAngleLengths();

         mTargetObject = new VisualControlObject(this, 0.25f, System.Drawing.Color.Pink.ToArgb()," target");
         mTargetObject.setPosition(mLightXML.Position  + Vector3.Normalize(mLightXML.Direction) * mLightXML.Radius);
         mLocationObject = new VisualControlObject(this, 0.25f, System.Drawing.Color.Red.ToArgb()," origin");
         mLocationObject.setPosition(mLightXML.Position);
         
         //mLocationObject.mAddToWorldList = false;


         updateBB();
         initVisuals();
      }


      public override void postAddCB()
      {
         mDirection = new Vector3(0, -1, 0);
         setPosition(getPosition() - (mDirection*LightData.mRadius));
         moved();
      }

      public override void postSelectCB() 
      {
         SimGlobals.getSimMain().unselectObject(mLocationObject);
         SimGlobals.getSimMain().unselectObject(mTargetObject);
      }

      public override void initVisuals()
      {
     
         mTargetObject.initVisuals();
         mLocationObject.initVisuals();
        
         initCone();
      }
      public override void clearVisuals()
      {
         clearCone();
         
         mTargetObject.clearVisuals();
         mLocationObject.clearVisuals();
       
      }

      private void initCone()
      {
         int col = mLightXML.LightColor.ToArgb();
         mBoundingConeInnerAngle = new BRenderDebugCone(mLightXML.mRadius, mInnerRadLen, System.Drawing.Color.SkyBlue.ToArgb(), true, false);
         mBoundingConeOuterAngle = new BRenderDebugCone(mLightXML.mRadius, mOuterRadLen, (int)(col), true, false);
         mSelectedCone = new BRenderDebugCone(mLightXML.mRadius, mOuterRadLen+0.25f, System.Drawing.Color.Yellow.ToArgb(), true, false);
      }
      private void clearCone()
      {
         if (mBoundingConeInnerAngle!=null) mBoundingConeInnerAngle.destroy();
         if (mBoundingConeOuterAngle != null) mBoundingConeOuterAngle.destroy();
         if (mSelectedCone != null) mSelectedCone.destroy();
      }


      private Matrix giveConeMatrix()
      {
         //create our rotation matrix for the cone so that it 'looks' like it's the actual cone
         Matrix LAM = Matrix.Identity;
         LAM = BMathLib.makeRotateMatrix(-BMathLib.unitY, mDirection) * Matrix.Translation(mLocationObject.getPosition());
         
         return LAM;
      }
      public override void render()
      {
         if (mLightXML.SettingsChanged())
         {
            CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(SimGlobals.getSimMain().getObjectIndex(this));
            calcAngleLengths();
            clearCone();
            initCone();
            //rotated();
         }


         BRenderDevice.getDevice().Transform.World = giveConeMatrix();

         mBoundingConeInnerAngle.render();
         mBoundingConeOuterAngle.render();
      }
      public override void renderSelected()
      {
         {
            BRenderDevice.getDevice().Transform.World = giveConeMatrix();
            mSelectedCone.render();
         }
      }

      public override void updateBB()
      {
         mAABB.min.X = -LightData.mRadius * 2;
         mAABB.min.Y = -LightData.mRadius * 2;
         mAABB.min.Z = -LightData.mRadius * 2;

         mAABB.max.X = LightData.mRadius * 2;
         mAABB.max.Y = LightData.mRadius * 2;
         mAABB.max.Z = LightData.mRadius * 2;
      }

      public override void rotated()
      {
         //move our target around us
         Vector3 loc = getPosition();
         Vector4 pos = Vector3.Transform(mDirection * LightData.mRadius, getRotationOnly());
         pos.X += loc.X;
         pos.Y += loc.Y;
         pos.Z += loc.Z;
         mTargetObject.setMatrix(Matrix.Translation(pos.X, pos.Y, pos.Z));

         //clear our own rotation so we don't bone up this whole process
         setMatrix(Matrix.Translation(getPosition()));

         //update our direction
         mDirection = mTargetObject.getPosition() - getPosition();
         LightData.mRadius = mDirection.Length();
         mDirection = BMathLib.Normalize(mDirection);

      }
      public override void moved()
      {
         //if (SimGlobals.getSimMain().getSimEditorMode() != SimMain.eSimEditorMode.cTranslateItem)
         {
            mTargetObject.setMatrix(Matrix.Translation(getPosition() + (mDirection * LightData.mRadius)));
            mLocationObject.setMatrix(Matrix.Translation(getPosition()));
         }

         //update our direction
         mDirection = mTargetObject.getPosition() - getPosition();
         LightData.mRadius = mDirection.Length();
         mDirection = BMathLib.Normalize(mDirection);

         updateBB();

         CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(SimGlobals.getSimMain().getObjectIndex(this));
      }



      private void calcAngleLengths()
      {
       //  return;
         mInnerRadLen = (float)(Math.Tan((Geometry.DegreeToRadian(mLightXML.InnerAngle/2.0f)))) * mLightXML.mRadius;
         mOuterRadLen = (float)(Math.Tan((Geometry.DegreeToRadian(mLightXML.OuterAngle/2.0f)))) * mLightXML.mRadius;
      }
      public override void childComponentChanged(VisualControlObject obj)
      {
         if (obj == mTargetObject || obj == mLocationObject)
         {
            setPosition(mLocationObject.getPosition());
            mDirection = mTargetObject.getPosition() - mLocationObject.getPosition();
            LightData.mRadius = mDirection.Length();
            mDirection=BMathLib.Normalize(mDirection);

            calcAngleLengths();
         }
     /*    else if (obj == mInnerAngleTarget || obj == mOuterAngleTarget)
         {
            Vector3 sideVec = Vector3.Cross(BMathLib.unitY, mDirection);
            sideVec=BMathLib.Normalize(sideVec);
            Vector3 upVec = Vector3.Cross(mDirection, sideVec);
            upVec=BMathLib.Normalize(upVec);

            Vector3 prevInner = mTargetObject.getPosition() - mInnerAngleTarget.getPosition();
            Vector3 prevouter = mTargetObject.getPosition() - mOuterAngleTarget.getPosition();

            float inAngle = (float)((float)Math.Atan(prevInner.Length() / LightData.mRadius));
            float outAngle = (float)((float)Math.Atan(prevouter.Length() / LightData.mRadius));
            if (inAngle > outAngle || inAngle < 0.1)
            {
               updateAngleTargetPositions();
               return;
            }


            mLightXML.InnerAngle = inAngle;
            mLightXML.OuterAngle = outAngle;

            calcAngleLengths();

            updateAngleTargetPositions();
         }*/
         updateBB();
         propertiesChanged();
      }
      public override List<EditorObject> getChildComponents()
      {
         List<EditorObject> l = new List<EditorObject>();

         l.Add(mTargetObject);
         l.Add(mLocationObject);

         return l;
      }


      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         
         Vector3 kOrg = r0 - getPosition();
         {
            
            //if we intersected our children, choose them over us.
            if(mLocationObject.testForRayIntersection(r0,rD,ref tVal) || mTargetObject.testForRayIntersection(r0,rD,ref tVal))
               return false;

            Vector3 dir = mTargetObject.getPosition() - mLocationObject.getPosition();
            //bool ret = BMathLib.rayConeIntersect(mLocationObject.getPosition(), dir, Geometry.DegreeToRadian(mLightXML.OuterAngle),mOuterRadLen, r0, rD);// (BMathLib.raySphereIntersect(Vector3.Empty, LightData.mRadius * 2, kOrg, rD, ref tVal));
            bool ret = BMathLib.rayCylindirIntersect(r0, rD, mLocationObject.getPosition(), dir, mOuterRadLen);
            if (ret == true)
               return true;
            return ret;
         }

         return false;
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         //if we intersected our children, choose them over us.

         bool locSel = mLocationObject.testForBoxIntersection(points);
         bool tarSet = mTargetObject.testForBoxIntersection(points);
         if (locSel && tarSet) return true;

         if(locSel || tarSet)  return false;

         {
            Vector3 pos = getPosition();
            return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect(mAABB.min + pos, mAABB.max + pos,
              points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
         }
      }

      public override EditorObject Copy()
      {
         //this.rotated();
         //this.moved();
         SpotLight obj = (SpotLight)this.MemberwiseClone();
         obj.LoadFromXML(this.mLightXML);
         obj.mMatrix = this.mMatrix;

         obj.mTargetObject = this.mTargetObject.Copy();
         obj.mTargetObject.Bind(obj);

         obj.mLocationObject = this.mLocationObject.Copy();
         obj.mLocationObject.Bind(obj);

         obj.mTargetObject.rotated();
         obj.mLocationObject.rotated();
         //obj.rotated();
         //obj.moved();

         //mTargetObject = new VisualControlObject(this, 0.25f, System.Drawing.Color.Pink.ToArgb(), " target");
         //mTargetObject.setPosition(mLightXML.Position + Vector3.Normalize(mLightXML.Direction) * mLightXML.Radius);
         //mLocationObject = new VisualControlObject(this, 0.25f, System.Drawing.Color.Red.ToArgb(), " origin");
         //mLocationObject.setPosition(mLightXML.Position);

         return obj;

      }
      public override string Name
      {
         get
         {
            return mLightXML.mName;
         }
      }

      public override string ToString()
      {
         return mLightXML.mName + " (" + mLightXML.mType + ")";
      }
      public override string GetTypeName()
      {
         return "SpotLight";
      }
      public override object GetProperties()
      {
         return mLightXML;
      }
      
      public override void propertiesChanged()
      {
   //      if (SimGlobals.getSimMain().getSimEditorMode() == SimMain.eSimEditorMode.cTranslateItem)
         {
            clearCone();
            initCone();
         }
         CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(SimGlobals.getSimMain().getObjectIndex(this));
      }
      public Vector3 GetLocationObjectPosition()
      {
         return mLocationObject.getPosition();
      }

     
      public BRenderDebugCone mBoundingConeInnerAngle;
      public BRenderDebugCone mBoundingConeOuterAngle;
      public BRenderDebugCone mSelectedCone;
      

      float mInnerRadLen = 0.5f;
      float mOuterRadLen = 0.5f;
      public Vector3 mDirection = new Vector3(0, -1, 0);

      //child components
      VisualControlObject mLocationObject;
      VisualControlObject mTargetObject;

   }

   public class OmniLight : LocalLight
   {
      public OmniLight(LightXML lightData)
      {
         mLockToTerrain = false;


         LightData = (LightXML)lightData.Clone();

         
         updateBB();
         initVisuals();
      }

      public override void postAddCB()
      {
         moved();
      }


      public override void initVisuals()
      {
         mLocSphere = new BRenderDebugSphere(0.25f, 1, unchecked((int)0x00FFFFFF));
         initSphere();
      }
      public override void clearVisuals()
      {
         clearSphere();
      }

      public void initSphere()
      {
         updateBB();

         int col = (int)(mLightXML.LightColor.ToArgb() & (0x00FFFFFF));

         mAxisCircleNear = new BRenderDebug2DCircle(mLightXML.mRadius * mLightXML.mFarAttnStart * 1.01f, col);
         mAxisCircleFar = new BRenderDebug2DCircle(mLightXML.mRadius * 1.01f, col);


         mAABBVis = new BRenderDebugCube(mAABB.min, mAABB.max, (int)System.Drawing.Color.Yellow.ToArgb(), false);
      }
      public void clearSphere()
      {
         if (mAABBVis != null)
         {
            mAABBVis.destroy();
            mAABBVis = null;
         }
         mAxisCircleNear.destroy();
         mAxisCircleFar.destroy();

      }

      public override void updateBB()
      {
         Vector3 orig = getPosition();

         mAABB.min.X = -LightData.mRadius;
         mAABB.min.Y = -LightData.mRadius;
         mAABB.min.Z = -LightData.mRadius;

         mAABB.max.X = LightData.mRadius;
         mAABB.max.Y = LightData.mRadius;
         mAABB.max.Z = LightData.mRadius;

      }

      public override bool testForRayIntersection(Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 kOrg = r0 - getPosition();
         return (BMathLib.raySphereIntersect(Vector3.Empty, LightData.mRadius, kOrg, rD, ref tVal));
      }
      public override bool testForBoxIntersection(Vector3[] points)
      {
         Vector3 pos = getPosition();
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect(mAABB.min + pos, mAABB.max + pos,
            points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));
      }

      public override object GetProperties()
      {
         return mLightXML;
      }
      public override void propertiesChanged()
      {
         
      }

      public override void render()
      {
         if ( mLightXML.SettingsChanged())
         {
            CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(SimGlobals.getSimMain().getObjectIndex(this));
            clearVisuals();
            initVisuals();
         }

         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());


     //    mLocSphere.render();

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         
         //inner sphere render
  /*       BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircleNear.render(true, true, 0, 1);
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircleNear.render(true, true, 0, 1);
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircleNear.render(true, true, 0, 1);
         */
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircleNear.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircleNear.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircleNear.render();
        

         //outer sphere render
          /*  BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircleFar.render(true, true, 0, 1);
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircleFar.render(true, true, 0, 1);
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircleFar.render(true, true, 0, 1);
       * */
         BRenderDevice.getDevice().Transform.World = Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(getPosition());
         mAxisCircleFar.render();
         BRenderDevice.getDevice().Transform.World = Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(getPosition());
         mAxisCircleFar.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Translation(getPosition());
         mAxisCircleFar.render();       
         
      }
      public override void renderSelected()
      {
         if (mAABBVis == null)
            mAABBVis = new BRenderDebugCube(new Vector3(-1, -1, -1), new Vector3(1, 1, 1), (int)System.Drawing.Color.Yellow.ToArgb(), false);
         BRenderDevice.getDevice().Transform.World = mMatrix;
         mAABBVis.render();
      }

      public override EditorObject Copy()
      {
         OmniLight obj = new OmniLight(LightData);
         obj.mMatrix = this.mMatrix;

         return obj;

      }

      public override string Name
      {
         get
         {
            return mLightXML.mName;
         }
      }
      public override string ToString()
      {
         return mLightXML.mName + " (" + mLightXML.mType + ")";
      }

      public override string GetTypeName()
      {
         return "OmniLight";
      }

      public override void moved()
      {
         base.moved();
         CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(SimGlobals.getSimMain().getObjectIndex(this));
      }
      BRenderDebugCube mAABBVis = null;

      BRenderDebugSphere mLocSphere = null;

      BRenderDebug2DCircle mAxisCircleNear = null;
      BRenderDebug2DCircle mAxisCircleFar = null;
   }

}
