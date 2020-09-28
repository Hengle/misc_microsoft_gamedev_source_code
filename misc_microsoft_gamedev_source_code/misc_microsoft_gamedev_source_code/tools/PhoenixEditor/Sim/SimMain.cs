#define SAVELIGHTSETS

using System;
using System.Collections.Generic;
using System.Text;
using Rendering;
using System.Drawing;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Text.RegularExpressions;
using System.Reflection;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;


using ModelSystem;
using EditorCore;
using UIHelper;


namespace SimEditor
{

   public class SimGlobals
   {
      static public SimMain getSimMain() { return mSimMain; }
      static private SimMain mSimMain = new SimMain();
   }

   [TopicType(Name="Sim")]
   public class SimMain
   {
      public enum eSimEditorMode
      {
         cNone,
         cPlaceItem,
         cSelectItem,
         cTranslateItem,
         cRotateItem
      }
      eSimEditorMode mCurrentMode;

      public Players PlayerData = new Players();
      //public Data PlacementData = new Data();
      public PlayerPlacement PlayerPlacementData = new PlayerPlacement();
      public List<TriggerRoot> TriggerData = new List<TriggerRoot>();
      //public TriggerRoot TriggerData = new TriggerRoot();
      public List<DiplomacyXml> DiplomacyData = new List<DiplomacyXml>();

       //mExternalScripts
      public List<ExternalScriptInfo> ExternalScripts = new List<ExternalScriptInfo>();


      public Material m_mtrl = new Material();
      public ObjectivesXML ObjectivesData = new ObjectivesXML();
      public List<ObjectGroup> ObjectGroups = new List<ObjectGroup>();

      public bool mPlaceItemMultipleMode = false;
      public float mMultipleMaxRotation = 0;
      public float mMultipleFillPercent = 1.0f;
      public float mMultipleRadius = 25;
      public int mMultipleNumObjectsToPlace = 0;   //calculated @ runtime

      public bool mAllowBoundsOverlapPlacement = false;
      public bool mPaintObjectVariations = false;

      public bool mShowNames = false;
      List<EditorObject> mEditMasterObjects = new List<EditorObject>();

      public bool mbDisableSmartHide = false;

      public List<string> GlobalExcludeObjects = new List<string>();


      public eSimEditorMode getSimEditorMode() 
      {
         return mCurrentMode;
      }

      
      public void modeChanged()
      {
         clearPlacementObjet();
         clearLastClick();
      }
      private void clearLastClick()
      {
         mLastItemPlacePoint.X = float.MinValue;
         mLastItemPlacePoint.Y = float.MinValue;
         mLastItemPlacePoint.Z = float.MinValue;
      }
      private void clearPlacementObjet()
      {
        // if (mPlacementDummyObject!=null)
       //     mPlacementDummyObject.clearVisuals();
         mPlacementDummyObject = null;
      }

      bool mbDontSwitchMenusItDrivesTimCrazy = true;


      #region buttonInterfaces
      
      [ToolCommand(Name = "             ")]
      public void Blank()
      {
         //this is a spacer...
      }


      [ToolCommand(Name = "Select", Icon = "SimSelect", TopicName = "Deferred")]
      public void SelectObjectMode()
      {
         if (CheckDepartmentPermission() == false)
            return;

         modeChanged();
         mCurrentMode = eSimEditorMode.cSelectItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         if (!mbDontSwitchMenusItDrivesTimCrazy)
            CoreGlobals.getEditorMain().mIGUI.ShowDialog("WorldObjectList");


         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Select");
      }

      [ToolCommand(Name = "Move", Icon = "Translate", TopicName = "Deferred")]
      public void TranslateObjectMode()
      {
         if (CheckDepartmentPermission() == false)
            return;

         if (mCurrentMode == eSimEditorMode.cTranslateItem && UIManager.GetAsyncKeyStateB(Key.LeftShift))
         {
            Controls.SimObjAdvNumericMove soanm = new Controls.SimObjAdvNumericMove();
            soanm.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            soanm.TopMost = true;
            soanm.Show();
            return;
         }

         modeChanged();
         mCurrentMode = eSimEditorMode.cTranslateItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         if (!mbDontSwitchMenusItDrivesTimCrazy)
            CoreGlobals.getEditorMain().mIGUI.ShowDialog("WorldObjectList");

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Move");

         

      }

      [ToolCommand(Name = "Rotate", Icon = "Rotate", TopicName = "Deferred")]
      public void RotateObjectMode()
      {
         if (CheckDepartmentPermission() == false)
            return;

         if (mCurrentMode == eSimEditorMode.cRotateItem && UIManager.GetAsyncKeyStateB(Key.LeftShift))
         {
            Controls.SimObjAdvNumericRotate soanm = new Controls.SimObjAdvNumericRotate();
            soanm.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            soanm.TopMost = true;
            soanm.Show();
            return;

         }
         modeChanged();

         mCurrentMode = eSimEditorMode.cRotateItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         if (!mbDontSwitchMenusItDrivesTimCrazy)
            CoreGlobals.getEditorMain().mIGUI.ShowDialog("WorldObjectList");

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Rotate");

       
      }

      [ToolCommand(Name = "Add Object", Icon = "SimAdd", TopicName = "Deferred")]
      public void PlaceItemMode()
      {

         if (CheckDepartmentPermission() == false)
            return;

         mCopiedObjects.Clear();

         modeChanged();

         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         //CoreGlobals.getEditorMain().mIGUI.ShowDialog("WorldObjectList");
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("UnitPicker");
         

         SimObject obj = new SimObject(SelectedProtoUnit,null, false);
         if (obj.IsSoundObject && mCurrentDepartmentMode != (int)eDepartment.Sound)
         {
            CoreGlobals.ShowMessage("Please switch to Sound Departement mode");
            return;
         }

         obj.setPlayerID(mSelectedPlayerID);
         PlacementDummyObject = obj;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Add Object");         
         
      }



      public void PlaceSquadMode()
      {

         if (CheckDepartmentPermission() == false)
            return;

         mCopiedObjects.Clear();

         modeChanged();

         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("UnitPicker");


         SimObject obj = new SimObject(SelectedProtoSquad, null, false);

         obj.setPlayerID(mSelectedPlayerID);
         PlacementDummyObject = obj;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Add Object");

      }


      [ToolCommand(Name = "Player Start", Icon = "PlayerStart")]
      public void PlacePlayerStart()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         PlayerPosition playerPosition = new PlayerPosition(SelectedPlayerID);
         PlacementDummyObject = playerPosition;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Player Start");         

      }

      [ToolCommand(Name = "Place Area")]
      public void PlaceArea()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         HelperAreaObject beacon = new HelperAreaObject();
         PlacementDummyObject = beacon;

      }

      [ToolCommand( Name = "Place Area Box" )]
      public void PlaceAreaBox()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         HelperAreaBoxObject beacon = new HelperAreaBoxObject();
         PlacementDummyObject = beacon;
      }

      [ToolCommand(Name = "Place Position")]
      public void PlacePosition()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         HelperPositionObject beacon = new HelperPositionObject();
         PlacementDummyObject = beacon;
      }

      [ToolCommand(Name = "Add Omni", Icon = "AddOmni", TopicName = "Lights")]
      public void PlaceOmniLight()
      {
         mCopiedObjects.Clear();

         modeChanged();

         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         SelectedLightData.mType = "omni";
         OmniLight localLight = new OmniLight(SelectedLightData);
         PlacementDummyObject = localLight;


         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Add Omni");         

      }

      [ToolCommand(Name = "Add Spot", Icon = "AddOmni", TopicName = "Lights")]
      public void PlaceSpotLight()
      {
         mCopiedObjects.Clear();

         modeChanged();

         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         SelectedLightData.mType = "spot";
         SpotLightPlacement localLight = new SpotLightPlacement();
         PlacementDummyObject = localLight;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Add Spot");         

      }



      public class EditorObjectCounter
      {
         public int mLightCount = 0;
         public int mSimCount = 0;
         public void Count(EditorObject obj)
         {
            if((obj is SimObject) || obj is PlayerPosition)
            {
               mSimCount++;

            }
            else if(obj is SpotLight || obj is OmniLight)
            {
               mLightCount++;
            }
         }

         public void SmartSetDirty()
         {
            if(mSimCount > 0)
            {
               SimGlobals.getSimMain().SetDirty();
            }
            if(mLightCount > 0)
            {
               SimGlobals.getSimMain().SetLightsDirty();
            }
         }
      }


      [ToolCommand(Name = "Delete", Icon = "SimDelete", TopicName = "Deferred")]
      public void DeleteSelected()
      {
         if (CheckDepartmentPermission() == false)
            return;

         modeChanged();

         EditorObjectCounter counter = new EditorObjectCounter();

         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            if (!(obj is VisualControlObject))
            {
               RemoveObject(obj);

               counter.Count(obj);
            }
         }
         mSelectedEditorObjects.Clear();


         counter.SmartSetDirty();
      }

      List<EditorObject> mCopiedObjects = new List<EditorObject>();

      [ToolCommand(Name = "Copy Objects", TopicName = "Deferred")]
      public void CopySelected()
      {
         if (CheckDepartmentPermission() == false)
            return;

         mCopiedObjects.Clear();

         modeChanged();

         EditorObjectCounter counter = new EditorObjectCounter();

         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            if (!(obj is VisualControlObject))
            {


               EditorObject copy = obj.Copy();
               if (copy is SpotLight)
               {
                  ((SpotLight)copy).moved();
               }
               if(copy != null)
               {
                  mCopiedObjects.Add(copy);
               }
            }
         }
         updateClipboardData();
      }


      [ToolCommand(Name = "Paste Objects", TopicName = "Deferred")]
      public void PasteObjectsMode()
      {
         if (CheckDepartmentPermission() == false)
            return;

         modeChanged();

         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         //SimObject obj = new SimObject(SelectedProtoUnit, null, false);
         //PlacementDummyObject = obj;         
         PlacementDummyObject = null ;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paste Objects");

      }


      [ToolCommand(Name = "Set Player Cam")]
      public void SetPlayerCamrea()
      {
         modeChanged();
         mCurrentMode = eSimEditorMode.cSelectItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         List<PlayerPosition> mPlayerPositions = new List<PlayerPosition>();
         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            if (obj is PlayerPosition)
            {
               mPlayerPositions.Add((PlayerPosition)obj);
            }
         }
         if(mPlayerPositions.Count == 0)
         {            
            return;//please select a player
         }
         if(mPlayerPositions.Count > 1)
         {
            return;//select only one start at a time.
         }
         PlayerPosition p = mPlayerPositions[0];
         CoreGlobals.getEditorMain().mITerrainShared.setCameraTarget(p.getPosition());


         Vector3 pos = CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
         Vector3 lookat = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget();
         Vector3 Dir = pos - lookat;

         p.PlayerPositionXML.CameraZoom = Dir.Length();
          
         Vector3 ndir = -Dir;
         ndir.Y = 0;
         ndir = BMathLib.Normalize(ndir);
         Vector3 kDir = BMathLib.Normalize(Dir);

         double a = Math.Atan2(ndir.X , ndir.Z) * 180 / Math.PI;

         double b = Math.Atan2(Math.Abs(kDir.X + kDir.Z) / 2, kDir.Y) * 180 / Math.PI;
         b = Math.Atan2(kDir.Y, (Math.Abs(kDir.X) + Math.Abs(kDir.Z)) / 2) * 180 / Math.PI;
         p.PlayerPositionXML.CameraYaw = (float)a;
         p.PlayerPositionXML.CameraPitch = (float)b;

      }


      [ToolCommand(Name = "Show Player Cam")]
      public void ShowPlayerCamrea()
      {
         /*
         modeChanged();
         mCurrentMode = eSimEditorMode.cSelectItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;
         */

         PlayerPosition p = null;

         List<PlayerPosition> mPlayerPositions = new List<PlayerPosition>();
         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            if (obj is PlayerPosition)
            {
               mPlayerPositions.Add((PlayerPosition)obj);
            }
         }
         if (mPlayerPositions.Count == 1)
            p = mPlayerPositions[0];

         float cameraYaw, cameraPitch, cameraZoom;
         Vector3 target;
         if (p == null || p.PlayerPositionXML.DefaultCamera)
         {
            // Get the default camera values
            PlayerPositionXML cameraDefaults = new PlayerPositionXML();
            cameraYaw = cameraDefaults.CameraYaw;
            cameraPitch = cameraDefaults.CameraPitch;
            cameraZoom = cameraDefaults.CameraZoom;
            target = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget();
         }
         else
         {
            // Get the camera values from the current player position
            cameraYaw = p.PlayerPositionXML.CameraYaw;
            cameraPitch = p.PlayerPositionXML.CameraPitch;
            cameraZoom = p.PlayerPositionXML.CameraZoom;
            target = p.getPosition();
         }

         // Yaw
         float rads = (float)(cameraYaw * (Math.PI / 180));
         Matrix rotmat = BMathLib.makeRotateXZMatrix(rads);
         Vector3 forward = new Vector3(0, 0, 1);
         forward.TransformNormal(rotmat);
         forward.Normalize();
         
         // Pitch
         Vector3 up = new Vector3(0, 1, 0);
         Vector3 right = Vector3.Cross(up, forward);
         rads = (float)(cameraPitch * (Math.PI / 180));
         rotmat = BMathLib.makeRotateArbitraryMatrix(rads, right);
         forward.TransformNormal(rotmat);
         forward.Normalize();

         // Set position and target
         Vector3 pos = target - (forward * cameraZoom);
         CoreGlobals.getEditorMain().mITerrainShared.setCameraPos(pos);
         CoreGlobals.getEditorMain().mITerrainShared.setCameraTarget(target);
      }


      [ToolCommand(Name = "Paths")]
      public void designerPanelShow()
      {
         CoreGlobals.getEditorMain().mIGUI.ShowDialog("DesignerObjectsPanel");
      }

      #endregion

#region Commands not on toolbar

      public void PlaceGameDesignValueSphere()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         GameDesignValueSphere obj = new GameDesignValueSphere();
         PlacementDummyObject = obj;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paths");

      }
      public void PlaceGameDesignValuePoint()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         GameDesignValuePoint obj = new GameDesignValuePoint();
         PlacementDummyObject = obj;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paths");

      }
      public void PlaceGameDesignLine()
      {
         mCopiedObjects.Clear();

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;
         CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;

         GameDesignLine obj = new GameDesignLine();
         PlacementDummyObject = obj;

         CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paths");

      }


      public void EditSelectedLines()
      {
         GameDesignLine line = null;

         mEditMasterObjects.Clear();

         mCurrentMode = eSimEditorMode.cSelectItem;
         foreach (EditorObject obj in this.mSelectedEditorObjects)
         {
            line = obj as GameDesignLine;
            mEditMasterObjects.Add(line);

         }
      }


      public void PlaceGameDesignLinePoints()
      {
         GameDesignLine line = null;

         foreach (EditorObject obj in this.mSelectedEditorObjects)
         {
            line = obj as GameDesignLine;

            BasicPoint point = obj as BasicPoint;
            if (point != null)
            {
               line = point.mOwner as GameDesignLine;
            }

            if (line != null)
               break;
         }

         if (line != null)
         {
            mCopiedObjects.Clear();

            modeChanged();
            mCurrentMode = eSimEditorMode.cPlaceItem;
            CoreGlobals.getEditorMain().MainMode = BEditorMain.eMainMode.cSim;


            BasicPoint obj = new BasicPoint();
            obj.mOwner = line;
            PlacementDummyObject = obj;

            CoreGlobals.getEditorMain().mPhoenixScenarioEditor.HandleCommand("Paths");
         }
      }

      public void InsertGameDesignLinePoint()
      {
         // Only one object should be selected when inserting a point
         if (this.mSelectedEditorObjects.Count != 1)
            return;


         BasicPoint point = null;
         GameDesignLine line = null;

         point = this.mSelectedEditorObjects[0] as BasicPoint;
         if (point != null)
         {
            line = point.mOwner as GameDesignLine;
         }

         if ((line != null) && (point != null))
         {
            SimEditor.EditorObject obj = point.Copy();

            Vector3 newPos = point.getPosition();
            newPos.Y += 6.0f;

            obj.setPosition(newPos);

            // Find the index of the current point in the line
            int index = line.mPoints.IndexOf(point);

            ((BasicPoint)obj).PlaceAt = index;

            AddObject(obj);

            // Change selection to new object
            mSelectedEditorObjects.Clear();
            mSelectedEditorObjects.Add(obj);
         }
      }


      public void CloseGameDesignLine()
      {
         // Only one object should be selected when inserting a point
         if (this.mSelectedEditorObjects.Count != 1)
            return;

         BasicPoint point = null;
         GameDesignLine line = null;

         // Find selected line or point
         bool bPointIsSelected = false;
         line = this.mSelectedEditorObjects[0] as GameDesignLine;

         if (line == null)
         {
            point = this.mSelectedEditorObjects[0] as BasicPoint;
            if (point != null)
            {
               bPointIsSelected = true;
               line = point.mOwner as GameDesignLine;
            }
         }

         if (line != null)
         {
            if (line.mPoints.Count > 0)
            {
               // Make sure line is not already close
               Vector3 startpos = line.getPosition();
               Vector3 endpos = line.mPoints[line.mPoints.Count - 1].getPosition();

               if (!startpos.Equals(endpos))
               {
                  // Add the new point to close the line loop
                  SimEditor.EditorObject obj = line.mPoints[0].Copy();

                  obj.setPosition(startpos);

                  AddObject(obj);

                  if (bPointIsSelected)
                  {
                     // Change selection to new object if a point was selected
                     mSelectedEditorObjects.Clear();
                     mSelectedEditorObjects.Add(obj);
                  }
               }
            }
         }
      }

#endregion


      public bool CheckDepartmentPermission()
      {
         string topicToCheck = "Sim";

         if (mCurrentDepartmentMode == (int)eDepartment.Design)
         {
            topicToCheck = "Sim";
         }
         else if (mCurrentDepartmentMode == (int)eDepartment.Art)
         {
            topicToCheck = "ArtObjects";
         }
         else if (mCurrentDepartmentMode == (int)eDepartment.Sound)
         {
            topicToCheck = "Sounds";
         }

         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission(topicToCheck) == false)
         {
            return false;
         }

         return true;
      }



      private void updateClipboardData()
      {
         if (mCopiedObjects.Count > 0)
         {

            Vector3 average = Vector3.Empty;
            foreach (EditorObject clipboardobj in mCopiedObjects)
            {
               average += clipboardobj.getPosition();
            }
            average *= (1 / (float)mCopiedObjects.Count);
            foreach (EditorObject clipboardobj in mCopiedObjects)
            {
               clipboardobj.mTempGroupOffset = (clipboardobj.getPosition() - average);
               clipboardobj.mTempGroupOffset.Y = clipboardobj.getPosition().Y; //don't avererage y
            }
         }
      }
      public void clipartAddObjects(List<ScenarioSimUnitXML> mUnits, List<LightXML> mLights)
      {
         mCopiedObjects.Clear();

         foreach (ScenarioSimUnitXML obj in mUnits)
         {
            try
            {
               if (!mSimFileData.mProtoObjectsByName.ContainsKey(obj.mProtoUnit))
               {

                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing protounit {0} not found in xobjects.xml.  Object will not be saved to scenario.", obj.mProtoUnit));

                  continue;
               }

               obj.mID = -1;

               SimObject o = new SimObject( obj, true);
              // AddObject(o);
               mCopiedObjects.Add(o);

            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }

         modeChanged();
         mCurrentMode = eSimEditorMode.cPlaceItem;

         updateClipboardData();
      }
      public void clipartUpdateObjects(float mRotation, Vector3 translationCenter)
      {
         if (mCopiedObjects.Count == 0)
            return;

         {
            if (mRotation != 0)
            {
               float rotAmt = mRotation;

               foreach (EditorObject obj in mCopiedObjects)
               {
                  Vector4 outv = Vector3.Transform(obj.mTempGroupOffset, Matrix.RotationY(rotAmt));
                  obj.mTempGroupOffset = new Vector3(outv.X, outv.Y, outv.Z);
                  obj.setMatrix(obj.getRotationOnly() * Matrix.RotationY(rotAmt) * Matrix.Translation(obj.getPosition()));
                  if (obj is SpotLight)
                  {
                     SpotLight light = (SpotLight)obj;
                     light.rotated();
                  }
               }
            }
         }

         foreach (EditorObject clipboardobj in mCopiedObjects)
         {
            clipboardobj.setMatrix(clipboardobj.getRotationOnly() * Matrix.Translation(translationCenter + clipboardobj.mTempGroupOffset));
            if (clipboardobj is SpotLight)
            {
               ((SpotLight)clipboardobj).moved();
            }
            if (clipboardobj is OmniLight)
            {
               ((OmniLight)clipboardobj).moved();

            }
         }       
      }
      public void clipartPlaceObjects(Vector3 intersectOverride)
      {
         if (mCopiedObjects.Count > 0)
         {
            placeGroup(intersectOverride);
         }
      }
      public void clipartClearObjects()
      {
         mCopiedObjects.Clear();
      }

      public SimMain()
      {
         try
         {
            if (mSimFileData.Load())
            {
               //set default proto unit
               if ((mSimFileData.mProtoObjectsXML.mUnits != null) && (mSimFileData.mProtoObjectsXML.mUnits.Count > 0))
               {
                  SelectedProtoUnit = mSimFileData.mProtoObjectsXML.mUnits[0];

               }
            }
            
            //set default light
            LightXML def = new LightXML();
            def.mRadius = 2;
            SelectedLightData = def;

            //set default player start
            SelectedPlayerID = 1;


            SetCurrentDepartmentMode(CoreGlobals.getSettingsFile().DepartementMode);
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("Error loading sim data" + ex.ToString());
         }
      }

      public void init()
      {
         mTransWidget = new TranslationWidget();
         mTransWidget.init();

         mRotWidget = new RotationWidget();
         mRotWidget.init();

         // Init material
         m_mtrl.Ambient = Color.White;
         m_mtrl.Diffuse = Color.White;
         m_mtrl.Specular = Color.Black;

         ClearSim();
      }

      public void deinit()
      {
         if (mTransWidget!=null)
         {
            mTransWidget.destroy();
            mTransWidget = null;
         }

         if (mRotWidget!=null)
         {
            mRotWidget.destroy();
            mRotWidget = null;
         }
         
         ClearSim();
         ClearVisuals();
         GrannyManager2.destroy();
      }
      #region SAVE LOAD
      public string givePrePathingBackspaces(string filename, string terrainFilename)
      {
         //get the distance between scenario and terrain
         //add that number of spaces between the two

         int numDirsDiff = 0;

         string scnName = Path.GetDirectoryName(filename);
         string trrName = Path.GetDirectoryName(terrainFilename);
         if (!scnName.Equals(trrName) && trrName!="")
         {
            //CLM WE ASSUME THE SCENARIO FILE IS LOWER THAN THE TERRAIN FILE
            string diff = scnName.Remove(0, trrName.Length);
            string[] ids = diff.Split('\\');
            numDirsDiff = ids.Length-1;   //minus 1 to disregard the filename
         }
         

         string outString = "";
         for(int i=0;i<numDirsDiff;i++)
         {
            outString += @"..\";
         }
         return outString;
      }



      public void SaveScenarioArtObjects(string filename)
      {
         SaveScenarioExtra(filename, (int)eDepartment.Art);
      }
      public void SaveScenarioSoundObjects(string filename)
      {
         SaveScenarioExtra(filename, (int)eDepartment.Sound);
      }
      private void SaveScenarioExtra(string filename, int department)
      {
         ScenarioExtraXML scen = new ScenarioExtraXML();
         using (PerfSection p1 = new PerfSection("Backup .SCN"))
         {
            if (CoreGlobals.FatalSCNSaveError == false)
            {
               FileUtilities.BackupScenarioFile(filename);
            }
         }
         try
         {
            List<string> editorOnlyUnits = new List<string>();
            editorOnlyUnits.Add("test_firebase_01");
            editorOnlyUnits.Add("test_expander_01");
            editorOnlyUnits.Add("test_base_01");

            foreach (EditorObject eobj in mEditorObjects)
            {
               if (eobj.GetType() == typeof(SimObject))
               {
                  SimObject obj = eobj as SimObject;
                  if (editorOnlyUnits.Contains(obj.GetProtoName()))
                  {
                     //if (obj.ProtoObject != null || obj.ProtoSquad != null)
                     //   scen.mEditorOnlyData.mEditorOnlyObjects.Add(obj.toXMLData(false));
                  }
                  else
                  {
                     if (obj.ProtoObject != null || obj.ProtoSquad != null)
                     {
                        if (obj.Department == department)//(int)(eDepartment.Design))
                        {
                           scen.mUnits.Add(obj.toXMLData(false));
                        }
                     }
                  }
               }
            }

            scen.mUnits.Sort(delegate(ScenarioSimUnitXML p1, ScenarioSimUnitXML p2) { return p1.mProtoUnit.CompareTo(p2.mProtoUnit); });

            mSimFileData.SaveScenarioExtras(filename, scen);
         }
         catch (System.Exception ex)
         {
            CoreGlobals.FatalSCNSaveError = true;
            throw ex;
         }
      }


      /// <summary>
      /// helper to save the other files
      /// </summary>
      /// <param name="mainScenarioFile"></param>
      public void SaveExtasMacro(string mainScenarioFile)
      {
         SaveScenarioArtObjects(Path.ChangeExtension(mainScenarioFile, "sc2"));
         SaveScenarioSoundObjects(Path.ChangeExtension(mainScenarioFile, "sc3"));

      }

      
      public void SaveScenario(string filename, string terrainFilename, string lightsetfile)
      {
         SaveScenario(filename, terrainFilename, lightsetfile, false);
      }

      public void SaveScenario(string filename, string terrainFilename, string lightsetfile, bool bNoScript)
      {      
         using (PerfSection p1 = new PerfSection("Backup .SCN"))
         {
            if (CoreGlobals.FatalSCNSaveError == false)
            {
               FileUtilities.BackupScenarioFile(filename);
            }
         }
         try
         {

            //this.TriggerData = this.triggerEditor1.TriggerData;  //this is a ref but getting the property does the final baking
            presaveSetSkirtObjectsToFly();
            updateHeightsFromTerrain();

            ScenarioXML scen = new ScenarioXML();

            scen.mTerrain.mTerrainFileName = givePrePathingBackspaces(filename, terrainFilename) + Path.GetFileNameWithoutExtension(terrainFilename);
            scen.mTerrain.mLoadVisRep = CoreGlobals.mbLoadTerrainVisRep;

            scen.mLightset = Path.GetFileNameWithoutExtension(lightsetfile);
            scen.mPathingFile = Path.GetFileNameWithoutExtension(filename);

            //we "opt in" on scenario data

            scen.mDiplomacyData = DiplomacyData;
            scen.mPlayers = PlayerData;
            scen.mObjectives = ObjectivesData;
            scen.mPlayerPlacement = PlayerPlacementData;
            scen.mTriggers = TriggerData;
            scen.mExternalScripts = ExternalScripts;
            scen.mSkyBox = CoreGlobals.ScenarioSkyboxFilename;
            scen.mTerrainEnv = CoreGlobals.ScenarioTerrainEnvMapFilename;
            scen.mObjectGroups = ObjectGroups;
            scen.mGlobalExcludeObjects = GlobalExcludeObjects;
            scen.mMinimap.MinimapTexture = CoreGlobals.ScenarioMinimapTextureFilename;
            scen.mBuildingTextureIndexUNSC = CoreGlobals.ScenarioBuildingTextureIndexUNSC;
            scen.mBuildingTextureIndexCOVN = CoreGlobals.ScenarioBuildingTextureIndexCOVN;

            scen.mSimBoundsMinX = CoreGlobals.mPlayableBoundsMinX;
            scen.mSimBoundsMinZ = CoreGlobals.mPlayableBoundsMinZ;
            scen.mSimBoundsMaxX = CoreGlobals.mPlayableBoundsMaxX;
            scen.mSimBoundsMaxZ = CoreGlobals.mPlayableBoundsMaxZ;
            
            scen.mbAllowVeterancy = CoreGlobals.mbAllowVeterancy;

            scen.mSoundbanks = CoreGlobals.ScenarioSoundbankFilenames;

            scen.mScenarioWorld = CoreGlobals.ScenarioWorld;

            int count = CoreGlobals.getGameResources().getNumCinematics();
            for (int i = 0; i < count; i++)
            {
               EditorCinematic ecin = CoreGlobals.getGameResources().getCinematic(i);

               ScenarioCinematicXML cinXML = new ScenarioCinematicXML();
               cinXML.mID = ecin.ID;
               cinXML.mName = ecin.Name;

               scen.mCinematics.Add(cinXML);
            }
#if SAVELIGHTSETS
         int count2 = CoreGlobals.getGameResources().getNumLightsets();
         for (int i = 0; i < count2; i++)
         {
            EditorLightset egls = CoreGlobals.getGameResources().getLightset(i);

            ScenarioLightsetXML glsXML = new ScenarioLightsetXML();
            glsXML.mID = egls.ID;
            glsXML.mName = egls.Name;
            glsXML.mLightProbeObject = egls.ObjectPropertyForFLSGen;

            scen.mLightsets.Add(glsXML);
         }
#endif

            count = CoreGlobals.getGameResources().getNumTalkingHeadVideos();
            for (int i = 0; i < count; i++)
            {
               EditorCinematic ecin = CoreGlobals.getGameResources().getTalkingHeadVideo(i);
               ScenarioTalkingHeadXML talkXML = new ScenarioTalkingHeadXML();
               talkXML.mID = ecin.ID;
               talkXML.mName = ecin.Name;

               scen.mTalkingHeads.Add(talkXML);
            }

            List<string> editorOnlyUnits = new List<string>();
            editorOnlyUnits.Add("test_firebase_01");
            editorOnlyUnits.Add("test_expander_01");
            editorOnlyUnits.Add("test_base_01");

            scen.mNextID = this.mHighestID + 1;

            foreach (EditorObject eobj in mEditorObjects)
            {
               if (eobj.GetType() == typeof(SimObject))
               {
                  SimObject obj = eobj as SimObject;

                  if (editorOnlyUnits.Contains(obj.GetProtoName()))
                  {
                     if (obj.ProtoObject != null || obj.ProtoSquad != null)
                        scen.mEditorOnlyData.mEditorOnlyObjects.Add(obj.toXMLData(false));
                  }
                  else
                  {
                     if (obj.ProtoObject != null || obj.ProtoSquad != null)
                     {
                        if (obj.Department == (int)(eDepartment.Design))
                        {
                           scen.mUnits.Add(obj.toXMLData(false));
                        }
                     }
                  }
               }
               //else if (eobj.GetType() == typeof(SpotLight))
               //{
               //   SpotLight light = eobj as SpotLight;
               //   light.mLightXML.Position = (light.getPosition());

               //   light.mLightXML.Direction = (light.mDirection);
               //   scen.mLights.Add(light.LightData);
               //}
               //else if (eobj.GetType() == typeof(OmniLight))
               //{
               //   OmniLight light = eobj as OmniLight;
               //   light.mLightXML.Position = (light.getPosition());
               //   scen.mLights.Add(light.LightData);
               //}
               else if (eobj.GetType() == typeof(PlayerPosition))
               {
                  PlayerPosition p = eobj as PlayerPosition;
                  p.PlayerPositionXML.Position = TextVectorHelper.ToString(p.getPosition());

                  Vector3 forward = new Vector3(0, 0, 1);
                  Vector4 v = Vector3.Transform(forward, p.getRotationOnly());
                  forward.X = v.X;
                  forward.Y = v.Y;
                  forward.Z = v.Z;
                  if (forward.LengthSq() == 0)
                     forward = BMathLib.unitZ;
                  p.PlayerPositionXML.Forward = TextVectorHelper.ToString(forward);

                  scen.mPlayerPosition.Add(p.PlayerPositionXML);
               }
            }

            //sort our simunits for load times
            scen.mUnits.Sort(delegate(ScenarioSimUnitXML p1, ScenarioSimUnitXML p2) { return p1.mProtoUnit.CompareTo(p2.mProtoUnit); });

            SaveEditorOnlyData(scen.mEditorOnlyData);

            SaveDesignerObjects(scen.mDesignObjects);

            if (bNoScript == true)
            {
               scen.mTriggers = new List<TriggerRoot>();
            }


            mSimFileData.SaveScenario(filename, scen);
         }
         catch (System.Exception ex)
         {
            CoreGlobals.FatalSCNSaveError = true;
            throw ex;
         }
      }



      //(int)eDepartment.Sound


      public void LoadScenarioArtObjects(string filename)
      {
         LoadScenarioExtra(filename, (int)eDepartment.Art);
      }
      public void LoadScenarioSoundObjects(string filename)
      {
         LoadScenarioExtra(filename, (int)eDepartment.Sound);
      }
      private void LoadScenarioExtra(string filename, int department)
      {
         ScenarioExtraXML scen = mSimFileData.LoadScenarioExtras(filename);

         if (scen == null)
            return;

         foreach (ScenarioSimUnitXML obj in scen.mUnits)
         {
            try
            {
               if (obj.mIsSquad == false && !mSimFileData.mProtoObjectsByName.ContainsKey(obj.mProtoUnit))
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing database entry for protounit {0} not found in objects.xml.  Object will not be saved to scenario.", obj.mProtoUnit));
                  continue;
               }
               if (obj.mIsSquad == true && !mSimFileData.mProtoSquadsByName.ContainsKey(obj.mProtoUnit))
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing database entry for squad {0} not found in squads.xml.  Object will not be saved to scenario.", obj.mProtoUnit));
                  continue;
               }

               SimObject o = new SimObject(obj, true);
               AddObject(o, department);
               InitID(o, (eDepartment)department);
               //o.Department = department;

            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }
      }


      public void LoadScenario(string filename )
      {
         LoadScenario(filename,false,false,false);
      }
      public void LoadScenario(string filename, bool ignoreSimUnits, bool ignorePlayerPositions, bool ignoreLights)
      {
         PerfSection p11 = new PerfSection("Sim.LoadScenario.LoadXML");
         
            ScenarioXML scen = mSimFileData.LoadScenario(filename);

         p11.Complete();

         PerfSection p2 = new PerfSection("Sim.LoadScenario.Settings");

            ClearSim();

            DiplomacyData = scen.mDiplomacyData;
            PlayerData = scen.mPlayers;
            ObjectivesData = scen.mObjectives;
            PlayerPlacementData = scen.mPlayerPlacement;
            TriggerData = scen.mTriggers;
            ExternalScripts = scen.mExternalScripts;
            ObjectGroups = scen.mObjectGroups;
            ClearGroupCache();

            GlobalExcludeObjects = scen.mGlobalExcludeObjects;

            if (scen.mNextID > this.mHighestID)
               this.mHighestID = scen.mNextID;

            CoreGlobals.TerrainFile = scen.mTerrain.mTerrainFileName + ".TED";
            CoreGlobals.mbLoadTerrainVisRep = scen.mTerrain.mLoadVisRep;
            
            CoreGlobals.ScenarioLightsetFilename = scen.mLightset + ".gls";
            CoreGlobals.ScenarioSkyboxFilename = scen.mSkyBox;
            CoreGlobals.ScenarioTerrainEnvMapFilename = scen.mTerrainEnv;
            CoreGlobals.ScenarioMinimapTextureFilename = scen.mMinimap.MinimapTexture;
            CoreGlobals.ScenarioBuildingTextureIndexUNSC = scen.mBuildingTextureIndexUNSC;
            CoreGlobals.ScenarioBuildingTextureIndexCOVN = scen.mBuildingTextureIndexCOVN;

            CoreGlobals.mPlayableBoundsMinX = scen.mSimBoundsMinX;
            CoreGlobals.mPlayableBoundsMinZ = scen.mSimBoundsMinZ;
            CoreGlobals.mPlayableBoundsMaxX = scen.mSimBoundsMaxX;
            CoreGlobals.mPlayableBoundsMaxZ = scen.mSimBoundsMaxZ;

            CoreGlobals.ScenarioSoundbankFilenames = scen.mSoundbanks;

            CoreGlobals.ScenarioWorld = scen.mScenarioWorld;


            CoreGlobals.mbAllowVeterancy = scen.mbAllowVeterancy;

         p2.Complete();
         using (PerfSection p3 = new PerfSection("Sim. Cinematics, Lightsets and videos"))
         {
            foreach (ScenarioCinematicXML cin in scen.mCinematics)
            {
               EditorCinematic c = new EditorCinematic(cin.mName, cin.mID, true);
               CoreGlobals.getGameResources().AddCinematic(c);
            }
#if SAVELIGHTSETS

            foreach (ScenarioLightsetXML cin in scen.mLightsets)
            {
               EditorLightset c = new EditorLightset(cin.mName, cin.mID, true);
               c.ObjectPropertyForFLSGen = cin.mLightProbeObject;
               CoreGlobals.getGameResources().AddLightset(c);
            }
#endif

            foreach (ScenarioTalkingHeadXML talk in scen.mTalkingHeads)
            {
               EditorCinematic c = new EditorCinematic(talk.mName, talk.mID, true);
               CoreGlobals.getGameResources().AddTalkingHeadVideo(c);
            }
            CoreGlobals.getGameResources().FixTalkingHeadDuplicates();//this fixes a previous bug
         }

         using (PerfSection p4 = new PerfSection("Sim. Squads and units"))
         {

            if (!ignoreSimUnits)
            {
               scen.mUnits.AddRange(scen.mEditorOnlyData.mEditorOnlyObjects);

               foreach (ScenarioSimUnitXML obj in scen.mUnits)
               {
                  try
                  {
                     if (obj.mIsSquad == false && !mSimFileData.mProtoObjectsByName.ContainsKey(obj.mProtoUnit))
                     {
                        CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing database entry for protounit {0} not found in objects.xml.  Object will not be saved to scenario.", obj.mProtoUnit));
                        continue;
                     }
                     if (obj.mIsSquad == true && !mSimFileData.mProtoSquadsByName.ContainsKey(obj.mProtoUnit))
                     {
                        CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing database entry for squad {0} not found in squads.xml.  Object will not be saved to scenario.", obj.mProtoUnit));
                        continue;
                     }

                     SimObject o = new SimObject(obj, true);
                     AddObject(o, (int)eDepartment.Design);
                     InitID(o, eDepartment.Design);
                     //o.Department = (int)eDepartment.Design;

                  }
                  catch (System.Exception ex)
                  {
                     CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
                  }
               }
            }

         }

         using (PerfSection p5 = new PerfSection("Player pos, lights, ed, des"))
         {
            if (!ignorePlayerPositions)
            {
               foreach (PlayerPositionXML obj in scen.mPlayerPosition)
               {
                  PlayerPosition p = new PlayerPosition(1);
                  p.PlayerPositionXML = obj;

                  Vector3 up;
                  up.X = 0;
                  up.Y = 1;
                  up.Z = 0;
                  Vector3 right = Vector3.Cross(up, obj.mForward);
                  right.Normalize();
                  Matrix rotationMatrix = BMathLib.makeRotateMatrix2(obj.mForward, right);
                  p.setMatrix(rotationMatrix * Matrix.Translation(obj.mPosition));

                  p.PlayerID = p.PlayerID;

                  p.updateBB();
                  AddObject(p);
               }
            }

            if (!ignoreLights)
            {
               foreach (LightXML light in scen.mLights)
               {
                  LocalLight local = null;
                  if (light.mType.ToLower() == @"spot")
                     local = (LocalLight)new SpotLight(light);
                  else if (light.mType.ToLower() == @"omni")
                     local = (LocalLight)new OmniLight(light);

                  local.setMatrix(Matrix.Translation(light.Position));
                  local.updateBB();

                  AddObject(local);
               }
            }


            LoadEditorOnlyData(scen.mEditorOnlyData);

            LoadDesignerObjects(scen.mDesignObjects);

            if (SimLoaded != null)
            {
               SimLoaded.Invoke(this);

            }
         }
      }


      public void SaveDesignerObjects(DesignObjects objects)
      {
         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj.GetType() == typeof(GameDesignValueSphere))
            {
               GameDesignValueSphere obj = eobj as GameDesignValueSphere;
               objects.mSpheres.Add(obj.ToXML());
            }
            if (eobj.GetType() == typeof(GameDesignLine))
            {
               GameDesignLine obj = eobj as GameDesignLine;
               objects.mLines.Add(obj.ToXML());
            }

         }
      }

      public void LoadDesignerObjects(DesignObjects objects)
      {
         foreach (DesignSphereXML obj in objects.mSpheres)
         {
            GameDesignValueSphere sphere = new GameDesignValueSphere();
            sphere.FromXML(obj);
            //p.updateBB();
            AddObject(sphere);
         }
         foreach (DesignLineXML obj in objects.mLines)
         {
            GameDesignLine line = new GameDesignLine();
            line.FromXML(obj);
            //p.updateBB();
            AddObject(line);

            foreach (Vector3 point in obj.mPoints)
            {
               BasicPoint p = new BasicPoint();
               p.mOwner = line;
               p.setPosition(point);
               AddObject(p);
            }

         }
      }

      public ScenarioScrapsXML ExportGroups(List<string> groupNames)
      {
         ScenarioScrapsXML scraps = new ScenarioScrapsXML();
         List<int> groupids = new List<int>();
         foreach (ObjectGroup group in ObjectGroups)
         {
            if (groupNames.Contains(group.Name))
            {
               scraps.mObjectGroups.Add(group);
               groupids.Add(group.ID);
            }
         }
         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj.GetType() == typeof(SimObject))
            {
               SimObject obj = eobj as SimObject;
               if (groupids.Contains(obj.Group) && (obj.ProtoObject != null || obj.ProtoSquad != null))
               {
                  scraps.mUnits.Add(obj.toXMLData(false));
               }
            }

            if (eobj.GetType() == typeof(HelperAreaObject))
            {
               HelperAreaObject obj = eobj as HelperAreaObject;
               if (groupids.Contains(obj.Group))
               {
                  scraps.mEditorOnlyData.mHelperAreas.Add(obj);
               }
            }

            if (eobj.GetType() == typeof(HelperAreaBoxObject))
            {
               HelperAreaBoxObject obj = eobj as HelperAreaBoxObject;
               if (groupids.Contains(obj.Group))
               {
                  scraps.mEditorOnlyData.mHelperAreaBoxes.Add(obj);
               }
            }

            if (eobj.GetType() == typeof(HelperPositionObject))
            {
               HelperPositionObject obj = eobj as HelperPositionObject;
               if (groupids.Contains(obj.Group))
               {
                  scraps.mEditorOnlyData.mHelperPositions.Add(obj);
               }
            }
         }
         return scraps;
      }
      public void ImportGroups(ScenarioScrapsXML scraps, List<string> groupNames)
      {

         List<int> groupids = new List<int>();     
         //look up existing groups...
         foreach (ObjectGroup group in ObjectGroups)
         {
            if (groupNames.Contains(group.Name))
            {
               groupids.Add(group.ID);
               groupNames.Remove(group.Name);
            }
         }
         //add new groups
         foreach (ObjectGroup group in scraps.mObjectGroups)
         {
            if (groupNames.Contains(group.Name))
            {
               ObjectGroups.Add(group);
               groupids.Add(group.ID);

            }
         }


         //Delete existing groups and build id table
         List<SimObject> hitlist = new List<SimObject>();
         Dictionary<int, bool> usedIDs = new Dictionary<int, bool>();
         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj.GetType() == typeof(SimObject))
            {
               SimObject obj = eobj as SimObject;
               if (groupids.Contains(obj.Group) && (obj.ProtoObject != null || obj.ProtoSquad != null))
               {
                  hitlist.Add(obj);
               }
            }
         }
         foreach(SimObject obj in hitlist )
         {
            mEditorObjects.Remove(obj);
         }
         foreach (int i in mUniqueIDs)
         {
            usedIDs[i] = true;
         }

         //delete areas
         List<HelperAreaObject> areahitlist2 = new List<HelperAreaObject>();
         List<HelperAreaBoxObject> areaboxhitlist2 = new List<HelperAreaBoxObject>();
         List<HelperPositionObject> poshitlist2 = new List<HelperPositionObject>();


         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj.GetType() == typeof(HelperAreaObject))
            {
               HelperAreaObject obj = eobj as HelperAreaObject;
               if (groupids.Contains(obj.Group))
               {
                  areahitlist2.Add(obj);
               }
            }

            if (eobj.GetType() == typeof(HelperAreaBoxObject))
            {
               HelperAreaBoxObject obj = eobj as HelperAreaBoxObject;
               if (groupids.Contains(obj.Group))
               {
                  areaboxhitlist2.Add(obj);
               }
            }

            if (eobj.GetType() == typeof(HelperPositionObject))
            {
               HelperPositionObject obj = eobj as HelperPositionObject;
               if (groupids.Contains(obj.Group))
               {
                  poshitlist2.Add(obj);
               }
            }
         }

         foreach (HelperAreaObject helper in areahitlist2)
         {
            mEditorObjects.Remove(helper);
         }

         foreach (HelperAreaBoxObject helper in areaboxhitlist2)
         {
            mEditorObjects.Remove(helper);
         }

         foreach (HelperPositionObject helper in poshitlist2)
         {
            mEditorObjects.Remove(helper);
         }


         //import areas
         foreach (HelperAreaObject helper in scraps.mEditorOnlyData.mHelperAreas)
         {
            try
            {
               if (groupids.Contains(helper.Group) == false)
                  continue;

               bool isInGroup = groupids.Contains(helper.Group);
               if (isInGroup == false || usedIDs.ContainsKey(helper.ID) == true)
               {
                  helper.ID = this.GetUniqueID();
               }
               usedIDs[helper.ID] = true;

               //SimObject o = new SimObject(obj, false);
               //HelperAreaObject o = new HelperAreaObject();
               

               AddObject(helper);

            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }

         //import area boxes
         foreach (HelperAreaBoxObject helper in scraps.mEditorOnlyData.mHelperAreaBoxes)
         {
            try
            {
               if (groupids.Contains(helper.Group) == false)
                  continue;

               bool isInGroup = groupids.Contains(helper.Group);
               if ((isInGroup == false) || (usedIDs.ContainsKey(helper.ID) == true))
               {
                  helper.ID = this.GetUniqueID();
               }
               usedIDs[helper.ID] = true;

               AddObject(helper);
            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }

         //import positions
         foreach (HelperPositionObject helper in scraps.mEditorOnlyData.mHelperPositions)
         {
            try
            {
               if (groupids.Contains(helper.Group) == false)
                  continue;

               bool isInGroup = groupids.Contains(helper.Group);
               if (isInGroup == false || usedIDs.ContainsKey(helper.ID) == true)
               {
                  helper.ID = this.GetUniqueID();
               }
               usedIDs[helper.ID] = true;


               AddObject(helper);

            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }


         //import units
         foreach (ScenarioSimUnitXML obj in scraps.mUnits)
         {
            try
            {
               if (groupids.Contains(obj.Group) == false)
                  continue;
               if (!mSimFileData.mProtoObjectsByName.ContainsKey(obj.mProtoUnit))
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Missing protounit {0} not found in xobjects.xml.  Object will not be saved to scenario.", obj.mProtoUnit));
                  continue;
               }

               bool isInGroup = groupids.Contains(obj.Group);
               //if(usedIDs.ContainsKey(obj.mID))
               if (isInGroup == false || usedIDs.ContainsKey(obj.mID) == true)
               {
                  obj.mID = this.GetUniqueID();
               }
               usedIDs[obj.mID] = true;

               SimObject o = new SimObject(obj, false);
       
               AddObject(o);
           
            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Exception while loading unit: {0}", ex.ToString()));
            }
         }
      }

      public void LoadEditorOnlyData(EditorOnlyData data)
      {
         foreach (HelperAreaObject eobj in data.mHelperAreas)
         {
            AddObject(eobj);
            RegisterID(eobj.ID);
         }

         foreach (HelperAreaBoxObject eobj in data.mHelperAreaBoxes)
         {
            AddObject(eobj);
            RegisterID(eobj.ID);
         }

         foreach (HelperPositionObject eobj in data.mHelperPositions)
         {
            AddObject(eobj);
            RegisterID(eobj.ID);
         }

      }

      public void SaveEditorOnlyData(EditorOnlyData data)
      {
         data.mHelperAreas.Clear();
         data.mHelperAreaBoxes.Clear();
         data.mHelperPositions.Clear();
         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj is HelperAreaObject)
            {
               data.mHelperAreas.Add((HelperAreaObject)eobj);
            }
            else if (eobj is HelperAreaBoxObject)
            {
               data.mHelperAreaBoxes.Add((HelperAreaBoxObject)eobj);
            }
            else if (eobj is HelperPositionObject)
            {
               data.mHelperPositions.Add((HelperPositionObject)eobj);
            }
         }
      }

      private void addLightFromFile(LightXML light)
      {
         LocalLight local = null;
         if (light.mType.ToLower() == @"spot")
         {
            local = (LocalLight)new SpotLight(light);
            local.setMatrix(Matrix.Translation(light.Position));
         }
         else if (light.mType.ToLower() == @"omni")
         {
            local = (LocalLight)new OmniLight(light);
            local.setMatrix(Matrix.Translation(light.Position));
         }
         local.LightData.Name = light.Name;

         local.updateBB();

         AddObject(local);
      }

      public lightSetXML LoadScenarioLights(string fileName, bool bloadlocal)
      {

         lightSetXML lightsetdata = new lightSetXML();

         XmlSerializer s = new XmlSerializer(typeof(lightSetXML), new Type[] { });
         Stream st = File.OpenRead(fileName);
         lightsetdata = (lightSetXML)s.Deserialize(st);
         st.Close();

         if (bloadlocal == true)
         {

            foreach (LightXML light in lightsetdata.mLights)
            {
               addLightFromFile(light);
            }
            foreach (LightXML light in lightsetdata.mTLights)
            {
               addLightFromFile(light);
            }
         }
         mGlobalLightSettings = lightsetdata;

         return lightsetdata;
      }

      lightSetXML mGlobalLightSettings = null;
      public lightSetXML GlobalLightSettings
      {
         get
         {
            return mGlobalLightSettings;
         }

      }

      public void SaveScenarioLights(string fileName, lightSetXML lightsetdata)
      {
         //ScenarioXML scen = new ScenarioXML();
         lightsetdata.mLights.Clear();
         lightsetdata.mTLights.Clear();

         WorkTopic topic = null ;
         if (CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().TryGetValue("Lights", out topic))
         {
            topic.Paused = true;
         }

         foreach (EditorObject eobj in mEditorObjects)
         {
            if(eobj is LocalLight)
            {
               if (eobj.GetType() == typeof(SpotLight))
               {
                  SpotLight light = eobj as SpotLight;
                  //light.LightData.Position = light.GetLocationObjectPosition();
                  light.LightData.Position = light.getPosition();
                  light.LightData.Direction = light.mDirection;// light.mLightXML.mDirection;//(light.mDirection);
                  
                  if ((eobj as LocalLight).LightData.TerrainOnly)
                     lightsetdata.mTLights.Add(light.LightData);
                  else
                     lightsetdata.mLights.Add(light.LightData);
               }
               else if (eobj.GetType() == typeof(OmniLight))
               {
                  OmniLight light = eobj as OmniLight;
                  light.LightData.Position = (light.getPosition());
                  if ((eobj as LocalLight).LightData.TerrainOnly)
                     lightsetdata.mTLights.Add(light.LightData);
                  else
                     lightsetdata.mLights.Add(light.LightData);
               }
            }
            
         }

         if(topic != null)
            topic.Paused = false;


         XmlSerializer s = new XmlSerializer(typeof(lightSetXML), new Type[] { });
         Stream st = File.Open(fileName, FileMode.OpenOrCreate);
         s.Serialize(st, lightsetdata);
         st.Close();

         XMBProcessor.CreateXMB(fileName, false);

         string flsfile = Path.ChangeExtension(fileName, "fls");
         if (File.Exists(flsfile) == true)
         {
            if (File.Exists(flsfile + ".xmb") )
            {
               if((File.GetAttributes(flsfile + ".xmb") & FileAttributes.ReadOnly) != FileAttributes.ReadOnly)
               {
                  XMBProcessor.CreateXMB(flsfile, false);
               }
            }
            else
            {
               XMBProcessor.CreateXMB(flsfile, false);

            }

            //cant write
         }


      }

      #endregion

      void presaveSetSkirtObjectsToFly()
      {
         float worldX = CoreGlobals.getEditorMain().mITerrainShared.getWorldSizeX();
         float worldZ = CoreGlobals.getEditorMain().mITerrainShared.getWorldSizeZ();
         for(int i=0;i<mEditorObjects.Count;i++)
         {
            Vector3 pos = mEditorObjects[i].getPosition();
            if (pos.X < 0 || pos.X > worldX || pos.Z < 0 || pos.Z > worldZ)
            {
               SimObject so = mEditorObjects[i] as SimObject;
               if(so!=null)
                  so.GetTypedProperties().LockToTerrain = false;
            }
         }
      }

      #region OBJECT PROPERTIES
      //System for using string references to refer to scenario object properties
      static Regex sRegexsSimObjectStringRef = new Regex(@"(?<id>[0-9]+)\.(?<prop>.*)");  
      public bool TryParseObjectPropertyRef(string s, out object value)
      {
         //value = null;
         //Match m = sRegexsSimObjectStringRef.Match(s);
         //if(m.Success == false)
         //{
         //   return false;
         //}
         //int id = int.Parse(m.Groups["id"].Value);
         //string propname = m.Groups["prop"].Value;

         //value = GetObjectProperty(id, propname);

         //return true;
         string blah;
         int id;
         return TryParseObjectPropertyRef(s, out value,out id, out blah);
      }
      public bool TryParseObjectPropertyRef(string s, out object value, out int id, out string propname)
      {
         propname = "";
         id = -1;
         value = null;
         Match m = sRegexsSimObjectStringRef.Match(s);
         if (m.Success == false)
         {
            return false;
         }
         id = int.Parse(m.Groups["id"].Value);
         propname = m.Groups["prop"].Value;

         return TryGetObjectProperty(id, propname, out value);

         
      }
      public bool TryGetObjectProperty(int id, string PropertyName, out object value)
      {
         value = null;
         foreach(EditorObject obj in mEditorObjects)
         {
            if(obj is IHasID)
            {
               if(((IHasID)obj).ID == id)
               {
                  PropertyInfo prop = obj.GetType().GetProperty(PropertyName);
                  if (prop == null)
                     return false;
                  value = prop.GetValue(obj, null);
                  return true;

               }
            }
         }
         return false;
      }
      public EditorObject GetEditorObjectByID(int id) 
      {
         foreach (EditorObject obj in mEditorObjects)
         {
            if (obj is IHasID)
            {
               if (((IHasID)obj).ID == id)
               {
                  return obj;
               }
            }
         }
         return null;
      }

      public void ClearBoundObjects()
      {
         foreach (EditorObject obj in mEditorObjects)
         {
            obj.SetBound(false);
         }
      }
      

      //public void CheckForDuplicateIDs()
      //{
      //   int[] idCount = new int[];
      //   foreach (EditorObject obj in mEditorObjects)
      //   {
      //      if (obj is IHasID)
      //      {
      //         if (((IHasID)obj).ID == id)
      //         {
      //            //return obj;


      //         }
      //      }
      //   }

      //}

      #endregion


      #region RELOAD
      public void ReloadSimData()
      {
         if (mSimFileData.Load())
         {
            //set default proto unit
            if ((mSimFileData.mProtoObjectsXML.mUnits != null) && (mSimFileData.mProtoObjectsXML.mUnits.Count > 0))
            {
               SelectedProtoUnit = mSimFileData.mProtoObjectsXML.mUnits[0];

            }
         }
         foreach (EditorObject eobj in mEditorObjects)
         {
            if (eobj.GetType() == typeof(SimObject))
            {
               SimObject obj = eobj as SimObject;
               if (obj.ProtoObject != null)
               {
                  string protoName = obj.ProtoObject.mName;

                  obj.mbSusbendProtoUpdate = true;
                  obj.ProtoObject = mSimFileData.mProtoObjectsByName[protoName];
                  obj.mbSusbendProtoUpdate = false;
               }
            }
         }
    
      }

      public void ReloadVisible()
      {

         GrannyManager.clearGR2Cache();
         foreach (EditorObject eobj in mEditorObjects)
         {
            //if (eobj.GetType() == typeof(SimObject))
            {
               //SimObject obj = eobj as SimObject;
               //if (obj.ProtoObject != null)
               {
                  eobj.initVisuals();


               }                
            }

         }


      }



      public void CheckForChanges()
      {
         CheckDirty();
         CheckLightsDirty();

      }




      public void SetDirty()
      {
         mLastChanged = System.DateTime.Now;
         mbDirty = true;
      }
      private void CheckDirty()
      {
         if (mbDirty == true && ((TimeSpan)(System.DateTime.Now - mLastChanged)).TotalMilliseconds > 1000)
         {
            Changed.Invoke(this);
            mbDirty = false;
         }

      }

      System.DateTime mLastLightsChanged = System.DateTime.Now;
      bool mbLightsDirty = false;
      public void SetLightsDirty()
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Lights") == false)
         {
            return;
         }

         mLastLightsChanged = System.DateTime.Now;
         mbLightsDirty = true;

      }
      private void CheckLightsDirty()
      {
         if (mbLightsDirty == true && ((TimeSpan)(System.DateTime.Now - mLastLightsChanged)).TotalMilliseconds > 1000)
         {
            LightsChanged.Invoke(this);
            mbLightsDirty = false;
         }

      }

      bool mbDirty = false;
      DateTime mLastChanged = System.DateTime.Now;


      #endregion
      #region INPUT
      public void input_DesignerControls(bool left, bool bMouseMoved, bool bClicked, bool bDragging)
      {
         switch (mCurrentMode)
         {
            case eSimEditorMode.cSelectItem:

               if (bClicked)//we've released
               {
                  if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count == 0)
                  {
                     UIManager.GetCursorPos(ref mReleasePoint);
                     doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                  }
                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  if (mRegTranslationLocked) //are we in regular selection mode?
                  {
                     moveObjects(true);
                     UIManager.GetCursorPos(ref mPressPoint);
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);

                        if (selectedObjectsClicked())
                        {
                           mRegTranslationLocked = true;
                        }
                        else
                        {
                           if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                           {
                              mSelectedEditorObjects.Clear();
                           }

                           UIManager.GetCursorPos(ref mReleasePoint);
                           doSelection( UIManager.GetAsyncKeyStateB(Key.LeftControl));
                           mRegTranslationLocked = selectedObjectsClicked();
                        }
                     }
                     else
                     {
                      if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count==0)
                        {
                           update2DSelectionBox();
                           mShowDragBox = true;
                        }

                     }
                  }
               }

               break;

            case eSimEditorMode.cRotateItem:

               if (!mRotWidget.isLocked())
                  mRotWidget.testIntersection();

               if (bClicked)//we've released
               {
                  if (mRotWidget.isLocked())
                  {
                     mRotWidget.clear();
                  }
                  else
                  {
                     if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count == 0)
                     {
                        UIManager.GetCursorPos(ref mReleasePoint);
                        doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                     }
                  }
                  

                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  //translation widget

                  if (!mShowDragBox && (mRotWidget.isLocked() || mRotWidget.testIntersection()) && mSelectedEditorObjects.Count > 0)
                  {
                     mRotWidget.lockIntersection();

                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        if (bMouseMoved)
                        {
                           rotateObjects(false);
                           UIManager.GetCursorPos(ref mPressPoint);
                        }
                     }
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                        if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                        {
                           mSelectedEditorObjects.Clear();
                           
                        }
                        UIManager.GetCursorPos(ref mReleasePoint);
                        doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                     }
                     else
                     {
                        if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count == 0)
                        {
                           update2DSelectionBox();
                           mShowDragBox = true;
                        }

                     }
                  }
               }

               break;
            case eSimEditorMode.cTranslateItem:

               if (!mTransWidget.isLocked())
                  mTransWidget.testIntersection();

               if (bClicked)//we've released
               {
                  if (mTransWidget.isLocked())
                  {
                     mTransWidget.clear();
                  }
                  else
                  {
                     if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count == 0)
                     {
                        UIManager.GetCursorPos(ref mReleasePoint);
                        doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                     }

                  }

                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  //translation widget
                  if (!mShowDragBox && (mTransWidget.isLocked() || mTransWidget.testIntersection()) && mSelectedEditorObjects.Count > 0)
                  {
                     mTransWidget.lockIntersection();

                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        if (bMouseMoved)
                        {
                           moveObjects(false);
                           UIManager.GetCursorPos(ref mPressPoint);
                        }
                     }
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                         if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                           {
                              mSelectedEditorObjects.Clear();
                              
                           }

                           UIManager.GetCursorPos(ref mReleasePoint);
                           doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                     }
                     else
                     {
                        //we're dragging, update and display our dragging icons
                        if (UIManager.GetAsyncKeyStateB(Key.LeftShift) || mSelectedEditorObjects.Count == 0)
                        {
                           update2DSelectionBox();
                           mShowDragBox = true;
                        }

                     }
                  }
               }
               break;
 
         };
         
      }

      public void input_ArtistControls(bool left, bool bMouseMoved,bool bClicked, bool bDragging)
      {
         switch (mCurrentMode)
         {
            case eSimEditorMode.cSelectItem:

               if (bClicked)//we've released
               {
                  UIManager.GetCursorPos(ref mReleasePoint);
                  if (!mRegTranslationLocked)
                  {
                     doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));
                  }
                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  if (mRegTranslationLocked) //are we in regular selection mode?
                  {
                     moveObjects(true);
                     UIManager.GetCursorPos(ref mPressPoint);
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {


                        if (selectedObjectsClicked() && !UIManager.GetAsyncKeyStateB(Key.LeftControl))
                        {
                           mRegTranslationLocked = true;
                        }
                        else
                        {
                           if (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.LeftControl))
                              mSelectedEditorObjects.Clear();
                        }
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        //we're dragging, update and display our dragging icons
                        update2DSelectionBox();
                        mShowDragBox = true;
                     }
                  }
               }

              
               break;

            case eSimEditorMode.cRotateItem:



               if (!mRotWidget.isLocked())
                  mRotWidget.testIntersection();

               if (bClicked)//we've released
               {
                  if (mRotWidget.isLocked())
                  {
                     mRotWidget.clear();
                  }
                  else
                  {
                     UIManager.GetCursorPos(ref mReleasePoint);
                     doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));


                  }

                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  //translation widget

                  if (!mShowDragBox && (mRotWidget.isLocked() || mRotWidget.testIntersection()) && mSelectedEditorObjects.Count > 0)
                  {
                     mRotWidget.lockIntersection();

                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        if (bMouseMoved)
                        {
                           rotateObjects(false);
                           UIManager.GetCursorPos(ref mPressPoint);
                        }
                     }
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {


                        if (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.LeftControl))
                           mSelectedEditorObjects.Clear();

                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        //we're dragging, update and display our dragging icons
                        update2DSelectionBox();
                        mShowDragBox = true;
                     }
                  }
               }
               break;
            case eSimEditorMode.cTranslateItem:



               if (!mTransWidget.isLocked())
                  mTransWidget.testIntersection();

               if (bClicked)//we've released
               {
                  if (mTransWidget.isLocked())
                  {
                     mTransWidget.clear();
                  }
                  else
                  {
                     UIManager.GetCursorPos(ref mReleasePoint);
                     doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));

                  }

                  mRegTranslationLocked = false;
                  mShowDragBox = false;
               }
               else if (left)
               {
                  //translation widget
                  if (!mShowDragBox && (mTransWidget.isLocked() || mTransWidget.testIntersection()) && mSelectedEditorObjects.Count > 0)
                  {
                     mTransWidget.lockIntersection();

                     if (!mLeftMouseDown)
                     {
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        if (bMouseMoved)
                        {
                           moveObjects(false);
                           UIManager.GetCursorPos(ref mPressPoint);
                        }
                     }
                  }
                  else
                  {
                     //have we clicked on a selected object for non-advanced movement mode
                     if (!mLeftMouseDown)
                     {
                        if (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.LeftControl))
                           mSelectedEditorObjects.Clear();


                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                     else
                     {
                        //we're dragging, update and display our dragging icons
                        update2DSelectionBox();
                        mShowDragBox = true;
                     }
                  }
               }
               break;
         };
      }

      public delegate void SimChanged(SimMain sender);

      public event SimChanged Changed;
      public event SimChanged LightsChanged;
      public event SimChanged SimLoaded;


      public void input()
      {
         if (CoreGlobals.getEditorMain().MainMode != BEditorMain.eMainMode.cSim || !mDoRendering)
            return;

         bool left = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft);
         bool right = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight);
         bool middle = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle);
         bool bMouseMoved = UIManager.Moved(0);
         bool bClicked = (!left && mLeftMouseDown);
         bool bClickedRight = (!right && mRightMouseDown);
         bool bClickedMiddle = (!middle && mMiddleMouseDown);
         bool bDragging = (left && mLeftMouseDown);




         switch (mCurrentMode)
         {
            case eSimEditorMode.cPlaceItem:

               if (PlacementDummyObject != null)
               {
                  if (UIManager.WheelDelta != 0)
                  {
                     mRotWidget.setSelected(false, true, false);
                     float rotAmt = ((float)(UIManager.WheelDelta / 50 * 0.05f));

                     PlacementDummyObject.setMatrix(PlacementDummyObject.getRotationOnly() * Matrix.RotationY(rotAmt) * Matrix.Translation(PlacementDummyObject.getPosition()));
                  }

               }
               if (mCopiedObjects.Count > 0)
               {
                  if (UIManager.WheelDelta != 0)
                  {
                     mRotWidget.setSelected(false, true, false);
                     float rotAmt = ((float)(UIManager.WheelDelta / 50 * 0.05f));

                     foreach (EditorObject obj in mCopiedObjects)
                     {
                        Vector4 outv = Vector3.Transform(obj.mTempGroupOffset, Matrix.RotationY(rotAmt));
                        obj.mTempGroupOffset = new Vector3(outv.X, outv.Y, outv.Z);
                        obj.setMatrix(obj.getRotationOnly() * Matrix.RotationY(rotAmt) * Matrix.Translation(obj.getPosition()));
                        if (obj is SpotLight)
                        {
                           SpotLight light = (SpotLight)obj;
                           light.rotated();
                        }
                     }

                  }

               }

               updateTempItem();


               if (left)
               {
                  if (PlacementDummyObject != null)
                  {
                     placeItem();
                  }
                  if (mCopiedObjects.Count > 0)
                  {
                     placeGroup();
                  }
               }

               //allow user to rotate object on the cursor

               //if this object has variations, cycle between them
               if (bClickedMiddle)
               {
                  if (PlacementDummyObject is SimObject)
                     ((SimObject)PlacementDummyObject).cycleVariationVisual(false);
               }


               //escape out of obj placement
               if (UIManager.GetAsyncKeyStateB(Key.Escape) || right)
               {
                  mCurrentMode = eSimEditorMode.cSelectItem;
               }



               break;

            default:
               if (mUseDesignerControls)
                  input_DesignerControls(left, bMouseMoved, bClicked, bDragging);
               else
                  input_ArtistControls(left, bMouseMoved, bClicked, bDragging);

               if (UIManager.WheelDelta != 0 && !(UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
               {
                  mRotWidget.setSelected(false, true, false);
                  rotateObjects((float)(UIManager.WheelDelta / 50 * 0.1f), true);

               }

               break;
         }

         if (UIManager.GetAsyncKeyStateB(Key.Delete) && !UIManager.GetAsyncKeyStateB(Key.LeftControl) && !UIManager.GetAsyncKeyStateB(Key.RightControl))
         {
            DeleteSelected();
         }
         if (UIManager.GetAsyncKeyStateB(Key.Escape) || right)
         {
            unselectAll();
         }



         //mark our current state for next frame
         mLeftMouseDown = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft);
         mRightMouseDown = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight);
         mMiddleMouseDown = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle);



      }
      #endregion


      #region RENDER
      public BBoundingBox getBBoxForDecalObjects()
      {
         BBoundingBox bb = new BBoundingBox();
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
         {
            if (!(mVisibleEditorObjects[i] is SimObject))
               continue;

            Matrix wTrans = ((SimObject)mVisibleEditorObjects[i]).getMatrix();
            if (mVisibleEditorObjects[i] != null && ((SimObject)mVisibleEditorObjects[i]).mVisual != null)
            {
               if (((SimObject)mVisibleEditorObjects[i]).IncludeInSimRep)
               {
                  bb.addPoint(((SimObject)mVisibleEditorObjects[i]).mAABB.max + ((SimObject)mVisibleEditorObjects[i]).getPosition());
                  bb.addPoint(((SimObject)mVisibleEditorObjects[i]).mAABB.min + ((SimObject)mVisibleEditorObjects[i]).getPosition());
               }
            }
         }
         return bb;
      }
      public BBoundingBox getBBoxForAOObjects()
      {
         BBoundingBox bb = new BBoundingBox();
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
         {
            if (!(mVisibleEditorObjects[i] is SimObject))
               continue;

            Matrix wTrans = ((SimObject)mVisibleEditorObjects[i]).getMatrix();
            if (mVisibleEditorObjects[i] != null && ((SimObject)mVisibleEditorObjects[i]).mVisual != null)
            {
               if (!((SimObject)mVisibleEditorObjects[i]).IgnoreToAO)
               {
                  bb.addPoint( ((SimObject)mVisibleEditorObjects[i]).mAABB.max + ((SimObject)mVisibleEditorObjects[i]).getPosition());
                  bb.addPoint( ((SimObject)mVisibleEditorObjects[i]).mAABB.min + ((SimObject)mVisibleEditorObjects[i]).getPosition());
               }
            }
         }
         return bb;
      }
      public void renderObjectsForAO(Texture prevDepthAOTex)
      { 
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);

         Vector3 color = Vector3.Empty;
         GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderAOPrevDepthTexHandle, prevDepthAOTex);
          
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
         {
            
           
            {
               if (!(mVisibleEditorObjects[i] is SimObject))
                     continue;

                  BRenderDevice.getDevice().Transform.World = ((SimObject)mVisibleEditorObjects[i]).getMatrix();
                  if (mVisibleEditorObjects[i]!=null && ((SimObject)mVisibleEditorObjects[i]).mVisual!=null &&
                     !((SimObject)mVisibleEditorObjects[i]).IgnoreToAO)
                     ((SimObject)mVisibleEditorObjects[i]).mVisual.render(color, true, GrannyInstance.eVisualEditorRenderMode.cRenderDepthPeelAO);
            }
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderAOPrevDepthTexHandle, BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_White));

      }
      public void renderObjectsForDecal()
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);

         Vector3 color = Vector3.Empty;
         
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
         {
               if (!(mVisibleEditorObjects[i] is SimObject))
                  continue;

               BRenderDevice.getDevice().Transform.World = ((SimObject)mVisibleEditorObjects[i]).getMatrix();
               if (mVisibleEditorObjects[i] != null && ((SimObject)mVisibleEditorObjects[i]).mVisual != null &&
                  ((SimObject)mVisibleEditorObjects[i]).IncludeInSimRep)
                  ((SimObject)mVisibleEditorObjects[i]).mVisual.render(color, true, GrannyInstance.eVisualEditorRenderMode.cRenderDepth);
         
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderAOPrevDepthTexHandle, BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_White));

      }
      public void renderObjectsForSimrep()
      {
         try
         {
            BRenderDevice.getDevice().Transform.World = Matrix.Identity;
            BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
            BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
            BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
            BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);

            Vector3 color = Vector3.Empty;

            for (int i = 0; i < mVisibleEditorObjects.Count; i++)
            {
               if (!(mVisibleEditorObjects[i] is SimObject))
                  continue;

               BRenderDevice.getDevice().Transform.World = ((SimObject)mVisibleEditorObjects[i]).getMatrix();
               if (mVisibleEditorObjects[i] != null && ((SimObject)mVisibleEditorObjects[i]).mVisual != null &&
                  ((SimObject)mVisibleEditorObjects[i]).IncludeInSimRep)
                  ((SimObject)mVisibleEditorObjects[i]).mVisual.render(color, true, GrannyInstance.eVisualEditorRenderMode.cRenderHeightForSimrep);

            }
            BRenderDevice.getDevice().Transform.World = Matrix.Identity;
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderAOPrevDepthTexHandle, BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_White));
         }
         catch(Exception e)
         {

         }
      }

      VertexBuffer mInstanceVB = null;
      VertexBuffer mIdentityInstanceVB = null;
      int mCurrInstanceCount = 0;
      int cMaxNumBatchInstances = 100;
      void makeInstanceVB()
      {
         mInstanceVB = new VertexBuffer(typeof(Matrix), cMaxNumBatchInstances, BRenderDevice.getDevice(), Usage.Dynamic | Usage.WriteOnly, VertexFormats.None, Pool.Default);
         mIdentityInstanceVB = new VertexBuffer(typeof(Matrix), 1, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.None, Pool.Managed);
         mIdentityInstanceVB.SetData(Matrix.Identity,0,LockFlags.None);
      }
      void flushBachedObjects(int startIndex)
      {
         BRenderDevice.getDevice().SetStreamSource(1, mInstanceVB, 0, 64);

         BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | mCurrInstanceCount);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);


         BRenderDevice.getDevice().Transform.World = Matrix.Identity;

         mVisibleEditorObjects[startIndex].render();

         BRenderDevice.getDevice().SetStreamSourceFrequency(0, 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, 1);
      }
      List<int> batchRenderEditorObject(int startIndex, SimObject targ, BFrustum selectionFrustum)
      {
           mCurrInstanceCount = 0;
            bool renderBB = false;
            bool render = IsRenderable(mVisibleEditorObjects[startIndex]) && !mVisibleEditorObjects[startIndex].mRenderedThisFrame;
            List<int> similar = new List<int>();
         
           
            unsafe
            {
               Matrix[] mats = (Matrix[])mInstanceVB.Lock(0, LockFlags.Discard);
               //queue up batched instances
               for (int j = startIndex; j < mVisibleEditorObjects.Count; j++)
               {
                  if (!(mVisibleEditorObjects[j] is SimObject))
                     continue;


                  SimObject targ2 = mVisibleEditorObjects[j] as SimObject;
                  if (targ2 == null || (targ2.ProtoObject == null && targ2.ProtoSquad == null))
                     continue;

                  render = IsRenderable(mVisibleEditorObjects[j]) && !mVisibleEditorObjects[j].mRenderedThisFrame;
                  if (!render)
                     continue;

                  //if we're not owned by the same player
                  if (mVisibleEditorObjects[startIndex].ToString() != mVisibleEditorObjects[j].ToString() ||
                     targ.getPlayerID() != targ2.getPlayerID())
                     continue;

                  //if we have a different variation visual index
                  if(targ2.VisualVariationIndex != ((SimObject)mVisibleEditorObjects[startIndex]).VisualVariationIndex )
                     continue;
                  

                  Vector3 pos = mVisibleEditorObjects[j].getPosition();
                  if (selectionFrustum.AABBVisible(mVisibleEditorObjects[j].mAABB.min + pos, mVisibleEditorObjects[j].mAABB.max + pos))
                  {
                     mats[mCurrInstanceCount++] = targ2.getMatrix();

                     mVisibleEditorObjects[j].mRenderedThisFrame = true;

                     similar.Add(j);
                  }


                  if (mCurrInstanceCount >= cMaxNumBatchInstances)
                  {
                     mInstanceVB.Unlock();

                     flushBachedObjects(startIndex);
                     mCurrInstanceCount = 0;
                     mats = (Matrix[])mInstanceVB.Lock(0, LockFlags.Discard);
                  }
               }

               mInstanceVB.Unlock();
            }
         

            //we're done batching, render.
            flushBachedObjects(startIndex);


            return similar;

         
      }
      public void renderObjectsOnly(bool bFogEnable)
      {
         BFrustum selectionFrustum = new BFrustum();
         selectionFrustum.update(BRenderDevice.getDevice());

         BRenderDevice.getDevice().Transform.World = Matrix.Identity;

         if (mInstanceVB == null)
            makeInstanceVB();
         mCurrInstanceCount = 0;

         // Set State
         //

         // Linear filtering
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MinFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MagFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MipFilter, (int)TextureFilter.Linear);

         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MinFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MagFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MipFilter, (int)TextureFilter.Linear);

         // Lighting
         Color globalAmbient = Color.FromArgb(255, 80, 80, 80);

         Vector3 lightDirection = new Vector3(-1.0f, -1.0f, -1.0f);
         lightDirection.Normalize();
         Color lightDiffuse = System.Drawing.Color.White;
         Color lightSpecular = System.Drawing.Color.White;

         // Enable lighting
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, true);
         BRenderDevice.getDevice().RenderState.Ambient = globalAmbient;

         // Set light
         BRenderDevice.getDevice().Lights[0].Type = LightType.Directional;
         BRenderDevice.getDevice().Lights[0].Diffuse = lightDiffuse;
         BRenderDevice.getDevice().Lights[0].Specular = lightSpecular;
         BRenderDevice.getDevice().Lights[0].Direction = lightDirection;
         BRenderDevice.getDevice().Lights[0].Enabled = true;

         // Set material
         BRenderDevice.getDevice().Material = m_mtrl;


         // Apply fog
         GrannyManager2.setFogParams(bFogEnable, 
                  GlobalLightSettings.zFogColor, GlobalLightSettings.zFogIntensity, GlobalLightSettings.zFogDensity, GlobalLightSettings.zFogStart,
                  GlobalLightSettings.planarFogColor, GlobalLightSettings.planarFogIntensity, GlobalLightSettings.planarFogDensity, GlobalLightSettings.planarFogStart);

         Vector3 cameraPos = CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();

         bool render = false;
         bool renderBB = false;
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
            mVisibleEditorObjects[i].mRenderedThisFrame = false;

         //render our objects
         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
         {
            render = IsRenderable(mVisibleEditorObjects[i]) && !mVisibleEditorObjects[i].mRenderedThisFrame;
            

            if (render == false)
               continue;

            mCurrInstanceCount = 0;

            Vector3 pos = mVisibleEditorObjects[i].getPosition();
            System.Drawing.Color textColor = System.Drawing.Color.Yellow;
            if (selectionFrustum.AABBVisible(mVisibleEditorObjects[i].mAABB.min + pos, mVisibleEditorObjects[i].mAABB.max + pos))
            {

               if (!mDoLightsRendering)
                  if (mVisibleEditorObjects[i] is LocalLight)                
                     continue;

               if(!mbRenderSimHelpers)
                  if (mVisibleEditorObjects[i] is SimHelperObject)
                     continue;

               

               //if we're not available for instancing
               if (!(mVisibleEditorObjects[i] is SimObject) || mVisibleEditorObjects[i] is PlayerPosition ||
                ( (mVisibleEditorObjects[i] is SimObject) && (mVisibleEditorObjects[i] as SimObject).mVisual == null))
               {

                  if (mVisibleEditorObjects[i] is PlayerPosition)
                  {
                     BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
                     BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
                     BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);
                  }

                  mVisibleEditorObjects[i].render();

                  renderBB = IsRenderBB(mVisibleEditorObjects[i]);

                  if (mbShowBB || renderBB)
                  {
                     mVisibleEditorObjects[i].renderSelected();
                  }
                  if (mShowNames && (render || renderBB))
                  {
                     BRenderDevice.getDevice().Transform.World = Matrix.Identity;
                     float fontSize = 3000 / Math.Abs(Vector3.Length(cameraPos - pos));
                     TextRendring.renderText(mVisibleEditorObjects[i].Name, pos, textColor, fontSize);
                  }
               }
               else
               {
                  SimObject targ = mVisibleEditorObjects[i] as SimObject;

                  List<int> similar = batchRenderEditorObject(i, targ, selectionFrustum);



                  for (int j = 0; j < similar.Count; j++)
                  {
                     SimObject targ2 = mVisibleEditorObjects[similar[j]] as SimObject;

                     render = IsRenderable(mVisibleEditorObjects[similar[j]]);
                     renderBB = IsRenderBB(mVisibleEditorObjects[similar[j]]);
                     if (mbShowBB || renderBB)
                     {
                        mVisibleEditorObjects[similar[j]].renderSelected();
                     }
                     if (mShowNames && (render || renderBB))
                     {
                        BRenderDevice.getDevice().Transform.World = Matrix.Identity;
                        float fontSize = 3000 / Math.Abs(Vector3.Length(cameraPos - pos));
                        TextRendring.renderText(mVisibleEditorObjects[j].Name, pos, textColor, fontSize);
                     }
                  }
     
                  
               }
            }
         }

         BRenderDevice.getDevice().Transform.World = Matrix.Identity;

         BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);

         switch (mCurrentMode)
         {
            case eSimEditorMode.cPlaceItem:
               if (PlacementDummyObject!=null)
               {
                  BRenderDevice.getDevice().Transform.World = PlacementDummyObject.getMatrix();// getRotationOnly();
                  PlacementDummyObject.render();
               }

               foreach(EditorObject obj in mCopiedObjects)
               {
                  BRenderDevice.getDevice().Transform.World = obj.getMatrix();
                  obj.render();
               }

               break;
         }

         BRenderDevice.getDevice().Transform.World = Matrix.Identity;


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetStreamSourceFrequency(0, 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, 1);


         for (int i = 0; i < mVisibleEditorObjects.Count; i++)
            mVisibleEditorObjects[i].mRenderedThisFrame = false;
      }
      public void renderWidgets()
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;

         // Enable AA
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         for (int i = 0; i < mSelectedEditorObjects.Count; i++)
            mSelectedEditorObjects[i].renderSelected();

         render2DSelectionBox();

         switch (mCurrentMode)
         {
            case eSimEditorMode.cTranslateItem:

               mTransWidget.calculateScale();

               if (mSelectedEditorObjects.Count > 0)
               {
                  mTransWidget.render();
                  BRenderDevice.getDevice().Transform.World = Matrix.Identity;
               }
               break;

            case eSimEditorMode.cRotateItem:

               mRotWidget.calculateScale();

               if (mSelectedEditorObjects.Count > 0)
               {
                  mRotWidget.render();
                  BRenderDevice.getDevice().Transform.World = Matrix.Identity;
               }
               break;
         }

         // Disable AA
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
      }
      public void render(bool bFogEnable)
      {
         if (!mDoRendering) return;

         renderObjectsOnly(bFogEnable);

         //if we're not in the sim editing mode, bail.
         if (CoreGlobals.getEditorMain().MainMode != BEditorMain.eMainMode.cSim)
            return;

         renderWidgets();

         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
      }
      #endregion


      #region PLACEMENT
      public void update()
      {
      }
      public void updateTempItem()
      {


         // //get our view ray
         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         Vector3 intersectionPoint = Vector3.Empty;

         if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref orig, ref dir, ref intersectionPoint))
         {
            if (PlacementDummyObject != null)
               PlacementDummyObject.setMatrix(PlacementDummyObject.getRotationOnly() * Matrix.Translation(intersectionPoint));

            foreach (EditorObject clipboardobj in mCopiedObjects)
            {
               clipboardobj.setMatrix(clipboardobj.getRotationOnly() * Matrix.Translation(intersectionPoint + clipboardobj.mTempGroupOffset));
               if(clipboardobj is SpotLight)
               {
                  ((SpotLight)clipboardobj).moved();

               }
               if (clipboardobj is OmniLight)
               {
                  ((OmniLight)clipboardobj).moved();

               }
            }            

         }


      }

      
      public void placeGroup()
      {
         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         Vector3 intersectionPoint = Vector3.Empty;// firstobj.getPosition();
         EditorObjectCounter counter = new EditorObjectCounter();


         if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref orig, ref dir, ref intersectionPoint))
         {
            placeGroup(intersectionPoint);
         }
         
      }
      public void placeGroup(Vector3 intersectOverride)
      {

         if (mCopiedObjects.Count == 0)
            return;

         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         Vector3 intersectionPoint = intersectOverride;// Vector3.Empty;// firstobj.getPosition();
         EditorObjectCounter counter = new EditorObjectCounter();

        
         //if (okIntersect)
         {
            //ensure we don't place 500 fucking copies of the same object on top of itself..
            Vector3 diff = mLastItemPlacePoint - intersectionPoint;
            float dist = diff.Length();

            EditorObject firstobj = mCopiedObjects[0];

            //Vector3 len = firstobj.mAABB.max;
            //len.X -= firstobj.mAABB.min.X;
            //len.Y = 0;
            //len.Z -= firstobj.mAABB.min.Z;
            //float rad = len.Length() / 2.0f;
            //if (rad > 8)
            float rad = 10;

            //ensure we've got a proper stride going
            if (dist < rad)
               return;

            mLastItemPlacePoint = intersectionPoint;



            foreach(EditorObject copiedObject in mCopiedObjects)
            {
               //if (copiedObject is SpotLightPlacement)
               //{
               //   SpotLight localLight = new SpotLight(SelectedLightData);
               //   localLight.setMatrix(copiedObject.getRotationOnly() * Matrix.Translation(intersectionPoint + new Vector3(0, 0.5f, 0)));
               //   localLight.updateBB();
               //   localLight.postAddCB();

               //   AddObject(localLight);
               //}
               //else
               {
                  EditorObject e = copiedObject.Copy();
                  
                  e.setMatrix(copiedObject.getRotationOnly() * Matrix.Translation(intersectionPoint + copiedObject.mTempGroupOffset));
                  e.updateBB();
                  //e.postAddCB();
                  AddObject(e);               
               }

            }
         }



      }

      public void placeItemToMask(float min, float max, bool useMaskValue)
      {
         if (PlacementDummyObject == null)
            return;

         Random mRandom = new Random(System.DateTime.Now.Millisecond);
         int minX = 0;
         int minZ = 0;
         int maxX = 0;
         int maxZ = 0;
         
         CoreGlobals.getEditorMain().mITerrainShared.getMaskTileBounds(ref minX, ref maxX, ref minZ, ref maxZ);

         int xLen = maxX - minX;
         int zLen = maxZ - minZ;

         
         Vector3 len = PlacementDummyObject.mAABB.max;
         len.X -= PlacementDummyObject.mAABB.min.X;
         len.Y = 0;
         len.Z -= PlacementDummyObject.mAABB.min.Z;
         float rad = len.Length() / 2.0f;
         if (rad > 8)
            rad = 10;

         int multipleRadius = (maxX - minX) /2;
         int numObjectsToPlace = 1; 
         numObjectsToPlace = (int)(multipleRadius / rad);
         numObjectsToPlace *= numObjectsToPlace;

         numObjectsToPlace = (int)(numObjectsToPlace*mMultipleFillPercent);
         
         Vector3 mMyIntersectionPt = Vector3.Empty;
         Vector3 newDir = -BMathLib.unitY;
         IMask mask = CoreGlobals.getEditorMain().mITerrainShared.getMask();
         numObjectsToPlace = 25;
         for (int x = 0; x < numObjectsToPlace; x++)
         {
            bool hit = false;
            while (!hit)
            {
               int k = (mRandom.Next() % xLen) + minX;
               int y = (mRandom.Next() % zLen) + minZ;
               long vertID = (k * CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts()) + y;
               hit = CoreGlobals.getEditorMain().mITerrainShared.isVertexMasked(k, y);

               if (useMaskValue)
               {
                  float asdf = mask.GetMaskWeight(vertID);
                  if (asdf < 0)
                     hit = false;
                  if (mRandom.Next(100) > (asdf * asdf * asdf * 100))
                  //if (asdf < min || asdf > max)
                  {
                     hit = false;
                  }
                  else
                  {
                     hit = true;
                  }
               }

               mMyIntersectionPt = CoreGlobals.getEditorMain().mITerrainShared.getTerrainPos(k,y);
            }

            
            bool placementOK = true;
            
               //loop through all our placed objects, check to see if we're too close
               for (int e = 0; e < mEditorObjects.Count; e++)
               {
                  Vector3 posdiff = mEditorObjects[e].getPosition() - mMyIntersectionPt;
                  float pDist = posdiff.Length();

                  if (!mAllowBoundsOverlapPlacement)
                  {
                     if (pDist < rad)
                     {
                        placementOK = false;
                        break;
                     }
                  }
                  else
                  {
                       if (pDist < 0.25f)
                     {
                        placementOK = false;
                        break;
                     }
                  }
               }

            
            if (!placementOK)
               continue;

            Matrix randYRot = Matrix.Identity;
            if (mMultipleMaxRotation > 0)
            {
               float rotAmt = mRandom.Next() % mMultipleMaxRotation;
               randYRot = Matrix.RotationY(rotAmt);
            }

            mLastItemPlacePoint = mMyIntersectionPt;

            if (PlacementDummyObject is SpotLightPlacement)
            {
               SpotLight localLight = new SpotLight(SelectedLightData);
               localLight.setMatrix(PlacementDummyObject.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt + new Vector3(0, 0.5f, 0)));
               localLight.updateBB();
               localLight.postAddCB();

               AddObject(localLight);
            }
            else
            {
               EditorObject e = PlacementDummyObject.Copy();
               e.setMatrix(PlacementDummyObject.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt));
               e.updateBB();
               e.postAddCB();
               AddObject(e);
            }

            if (mPaintObjectVariations)
               if (PlacementDummyObject is SimObject)
                  ((SimObject)PlacementDummyObject).cycleVariationVisual(true);
         }
         

      }


      public string placeItemsToMask(List<ObjectPlacementSettings> objectPlacementSettings )
      {
         Random mRandom = new Random(System.DateTime.Now.Millisecond);
         int minX = 0;
         int minZ = 0;
         int maxX = 0;
         int maxZ = 0;

         CoreGlobals.getEditorMain().mITerrainShared.getMaskTileBounds(ref minX, ref maxX, ref minZ, ref maxZ);

         int xLen = maxX - minX;
         int zLen = maxZ - minZ;

         if (objectPlacementSettings.Count == 0)
         {
            return "Press settings and add items to the list with the blue button (lower left corner)";
         }

         foreach (ObjectPlacementSettings settings in objectPlacementSettings)
         {
            if (SimGlobals.getSimMain().mSimFileData.mProtoObjectsByName.ContainsKey(settings.Object) == false)
            {
               continue;
            }

            SimObject toPlace = new SimObject(settings.Object, false);

            Vector3 len = toPlace.mAABB.max;
            len.X -= toPlace.mAABB.min.X;
            len.Y = 0;
            len.Z -= toPlace.mAABB.min.Z;
            float rad = len.Length() / 2.0f;
            if (rad > 8)
               rad = 10;

            if (xLen < 3 && zLen < 3)
            {
               return "Please select a mask first.";
            }

            int multipleRadius = (maxX - minX) / 2;
            int numObjectsToPlace = 1;
            numObjectsToPlace = (int)(multipleRadius / rad);
            numObjectsToPlace *= numObjectsToPlace;

            numObjectsToPlace = (int)(numObjectsToPlace * mMultipleFillPercent);

            Vector3 mMyIntersectionPt = Vector3.Empty;
            Vector3 newDir = -BMathLib.unitY;
            IMask mask = CoreGlobals.getEditorMain().mITerrainShared.getMask();

            numObjectsToPlace = settings.NumberToPlace;

            for (int x = 0; x < numObjectsToPlace; x++)
            {
               bool hit = false;
               bool failed = false;
               int misses = 0;
               while (!hit && !failed)
               {
                  int k = (mRandom.Next() % xLen) + minX;
                  int y = (mRandom.Next() % zLen) + minZ;
                  long vertID = (k * CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts()) + y;
                  hit = CoreGlobals.getEditorMain().mITerrainShared.isVertexMasked(k, y);

                  float maskValue = mask.GetMaskWeight(vertID);
                  if (maskValue < settings.MinMaskRange || maskValue > settings.MaxMaskRange)
                  {
                     hit = false;
                  }

                  if (hit && settings.UseMaskDensity == true)
                  {
                     if (maskValue < 0)
                        hit = false;
                     int numToBeat = (int)Math.Abs((settings.MidMaskRange - maskValue) * 100);
                     if (mRandom.Next(100) > numToBeat)
                     {
                        hit = true;
                     }
                     else
                     {
                        hit = false;
                     }
                  }

                  mMyIntersectionPt = CoreGlobals.getEditorMain().mITerrainShared.getTerrainPos(k, y);

                  misses++;
                  if (misses > 1000)
                  {
                     failed = true;
                  }
               }

               if (failed)
                  break;

               bool placementOK = true;
               //loop through all our placed objects, check to see if we're too close
               for (int e = 0; e < mEditorObjects.Count; e++)
               {
                  Vector3 posdiff = mEditorObjects[e].getPosition() - mMyIntersectionPt;
                  float pDist = posdiff.Length();

                  if (!mAllowBoundsOverlapPlacement)
                  {
                     if (pDist < rad)
                     {
                        placementOK = false;
                        break;
                     }
                  }
                  else
                  {
                     if (pDist < 0.25f)
                     {
                        placementOK = false;
                        break;
                     }
                  }
               }
               if (!placementOK)
               {
                  continue;
               }

               Matrix randYRot = Matrix.Identity;
               //if (mMultipleMaxRotation > 0)
               {
                  float rotAmt = mRandom.Next() % 360;//mMultipleMaxRotation;
                  randYRot = Matrix.RotationY(rotAmt);
               }

               mLastItemPlacePoint = mMyIntersectionPt;
               {
                  EditorObject e = toPlace.Copy();
                  e.setMatrix(toPlace.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt));
                  e.updateBB();
                  e.postAddCB();
                  AddObject(e);

                  List<EditorObject> tempList = new List<EditorObject>();
                  tempList.Add(e);
                  updateHeightsFromTerrain(tempList, true, settings.UseTerrainSlope / 100f);
               }

               if (mPaintObjectVariations)
                  if (PlacementDummyObject is SimObject)
                     ((SimObject)PlacementDummyObject).cycleVariationVisual(true);
            }
         }
         return "";
      }
      public void placeItemToMaskSimple(string obj, bool useMaskAsDensity, int numObjToPlace)
      {
         Random mRandom = new Random(System.DateTime.Now.Millisecond);
         int minX = 0;
         int minZ = 0;
         int maxX = 0;
         int maxZ = 0;

         CoreGlobals.getEditorMain().mITerrainShared.getMaskTileBounds(ref minX, ref maxX, ref minZ, ref maxZ);

         int xLen = maxX - minX;
         int zLen = maxZ - minZ;

         if (obj=="")
            return;
         

         
         {
            if (SimGlobals.getSimMain().mSimFileData.mProtoObjectsByName.ContainsKey(obj) == false)
            {
               return;
            }

            SimObject toPlace = new SimObject(obj, false);

            Vector3 len = toPlace.mAABB.max;
            len.X -= toPlace.mAABB.min.X;
            len.Y = 0;
            len.Z -= toPlace.mAABB.min.Z;
            float rad = len.Length() / 2.0f;
            if (rad > 8)
               rad = 10;

            if (xLen < 3 && zLen < 3)
            {
               return;
            }

            int multipleRadius = (maxX - minX) / 2;
            int numObjectsToPlace = 1;
            numObjectsToPlace = (int)(multipleRadius / rad);
            numObjectsToPlace *= numObjectsToPlace;

            numObjectsToPlace = (int)(numObjectsToPlace * mMultipleFillPercent);

            Vector3 mMyIntersectionPt = Vector3.Empty;
            Vector3 newDir = -BMathLib.unitY;
            IMask mask = CoreGlobals.getEditorMain().mITerrainShared.getMask();

            numObjectsToPlace = numObjToPlace;

            for (int x = 0; x < numObjectsToPlace; x++)
            {
               bool hit = false;
               bool failed = false;
               int misses = 0;
               while (!hit && !failed)
               {
                  int k = (mRandom.Next() % xLen) + minX;
                  int y = (mRandom.Next() % zLen) + minZ;
                  long vertID = (k * CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts()) + y;
                  hit = CoreGlobals.getEditorMain().mITerrainShared.isVertexMasked(k, y);

                  float maskValue = mask.GetMaskWeight(vertID);


                  if (hit && useMaskAsDensity == true)
                  {
                     if (maskValue < 0)
                        hit = false;
                     int numToBeat = (int)Math.Abs(1.0f-maskValue) * 100;
                     if (mRandom.Next(100) > numToBeat)
                     {
                        hit = true;
                     }
                     else
                     {
                        hit = false;
                     }
                  }

                  mMyIntersectionPt = CoreGlobals.getEditorMain().mITerrainShared.getTerrainPos(k, y);

                  misses++;
                  if (misses > 1000)
                  {
                     failed = true;
                  }
               }

               if (failed)
                  break;

               bool placementOK = true;
               //loop through all our placed objects, check to see if we're too close
               for (int e = 0; e < mEditorObjects.Count; e++)
               {
                  Vector3 posdiff = mEditorObjects[e].getPosition() - mMyIntersectionPt;
                  float pDist = posdiff.Length();

                  if (!mAllowBoundsOverlapPlacement)
                  {
                     if (pDist < rad)
                     {
                        placementOK = false;
                        break;
                     }
                  }
                  else
                  {
                     if (pDist < 0.25f)
                     {
                        placementOK = false;
                        break;
                     }
                  }
               }
               if (!placementOK)
               {
                  continue;
               }

               Matrix randYRot = Matrix.Identity;
               //if (mMultipleMaxRotation > 0)
               {
                  float rotAmt = mRandom.Next() % 360;//mMultipleMaxRotation;
                  randYRot = Matrix.RotationY(rotAmt);
               }

               mLastItemPlacePoint = mMyIntersectionPt;
               {
                  EditorObject e = toPlace.Copy();
                  e.setMatrix(toPlace.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt));
                  e.updateBB();
                  e.postAddCB();
                  AddObject(e);

                  List<EditorObject> tempList = new List<EditorObject>();
                  tempList.Add(e);
                  updateHeightsFromTerrain(tempList, false, 1);
               }

               if (mPaintObjectVariations)
                  if (PlacementDummyObject is SimObject)
                     ((SimObject)PlacementDummyObject).cycleVariationVisual(true);
            }
         }
         
      }


      public void placeItem()
      {
         if (PlacementDummyObject == null)
            return;

         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir =BMathLib.Normalize(dir);

         Vector3 intersectionPoint = PlacementDummyObject.getPosition();
         EditorObjectCounter counter = new EditorObjectCounter();

         Random mRandom = new Random(System.DateTime.Now.Millisecond);

         if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref orig, ref dir, ref intersectionPoint))
         {
            Vector3 len = PlacementDummyObject.mAABB.max;
            len.X -= PlacementDummyObject.mAABB.min.X;
            len.Y = 0;
            len.Z -= PlacementDummyObject.mAABB.min.Z;
            float rad = len.Length() / 2.0f;
            if (rad > 8)
               rad = 10;

            mMultipleNumObjectsToPlace = 1;
            if (mPlaceItemMultipleMode)
            {
               mMultipleNumObjectsToPlace = (int)(mMultipleRadius / rad);
               mMultipleNumObjectsToPlace *= mMultipleNumObjectsToPlace;
            }

            Vector3 mMyIntersectionPt = intersectionPoint;
            Vector3 newDir = -BMathLib.unitY;
            for (int x = 0; x < mMultipleNumObjectsToPlace; x++)
            {
               //we're randomly placing, so move us around within the radius.
               if (mPlaceItemMultipleMode)
               {
                  bool hit = false;
                  while(!hit)
                  {
                     Vector3 nOrig = intersectionPoint;
                     nOrig.X += (mRandom.Next() % mMultipleRadius) - (((int)mMultipleRadius) >> 1);
                     nOrig.Y = 200;
                     nOrig.Z += (mRandom.Next() % mMultipleRadius) - (((int)mMultipleRadius) >> 1);
                     hit = CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref nOrig, ref newDir, ref mMyIntersectionPt);   
                  }
                  
               }

               
                  if (mPlaceItemMultipleMode)
                  {
                     //loop through all our placed objects, check to see if we're too close
                     for (int e = 0; e < mEditorObjects.Count; e++)
                     {
                        Vector3 posdiff = mEditorObjects[e].getPosition() - mMyIntersectionPt;
                        float pDist = posdiff.Length();

                        if (!mAllowBoundsOverlapPlacement)
                        {
                           if (pDist < rad)
                              return;
                        }
                        else
                        {
                           if (pDist < 0.25f)
                              return;
                        }
                     }
                  }
                  else
                  {              
                     //just check our last placed object
                     Vector3 diff = mLastItemPlacePoint - mMyIntersectionPt;
                     float dist = diff.Length();

                     if (dist < rad)
                        return;

                  }
               

               Matrix randYRot = Matrix.Identity;
               if (mMultipleMaxRotation > 0)
               {
                  float rotAmt = mRandom.Next() % mMultipleMaxRotation;
                  randYRot = Matrix.RotationY(rotAmt);
               }

               mLastItemPlacePoint = mMyIntersectionPt;

               if (PlacementDummyObject is SpotLightPlacement)
               {
                  SpotLight localLight = new SpotLight(SelectedLightData);
                  localLight.setMatrix(PlacementDummyObject.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt + new Vector3(0, 0.5f, 0)));
                  localLight.updateBB();
                  localLight.postAddCB();

                  AddObject(localLight);
               }
               else
               {
                  EditorObject e = PlacementDummyObject.Copy();
                  e.setMatrix(PlacementDummyObject.getRotationOnly() * randYRot * Matrix.Translation(mMyIntersectionPt));
                  e.updateBB();
                  e.postAddCB();
                  AddObject(e);
               }

               if (mPaintObjectVariations)
                  if(PlacementDummyObject is SimObject)
                     ((SimObject)PlacementDummyObject).cycleVariationVisual(true);
               

            }
         }

      }
      #endregion

      #region SELECTION
      private int mLastSelectedItemCount = 0;
      private bool mSelectionListChanged = false;
      public void addSelectedObject(EditorObject obj)
      {
         if (obj == null)
            return;
         mSelectedEditorObjects.Add(obj);
         List<EditorObject> l = obj.getChildComponents();
         if (l != null && obj.AutoSelectChildComponents())
            mSelectedEditorObjects.AddRange(l);
         
         obj.postSelectCB();
         if(l!=null)
         {
            for (int k = 0; k < l.Count; k++)
               l[k].postSelectCB();
         }

         updateWidgetPos();
      }
      public bool selectedObjectsClicked()
      {
         if (mSelectedEditorObjects.Count > 0)
         {
            Vector3 orig = getRayPosFromMouseCoords(false);
            Vector3 dir = getRayPosFromMouseCoords(true) - orig;
            dir = BMathLib.Normalize(dir);
            float tVal = 0;
            foreach (EditorObject obj in mSelectedEditorObjects)
            {
               if (obj.testForRayIntersection(orig, dir, ref tVal))
                  return true;
            }
         }
         return false;
      }
      public int giveSelectedObjectIndex(EditorObject obj)
      {
         for (int i = 0; i < mSelectedEditorObjects.Count; i++)
         {
            if (obj == mSelectedEditorObjects[i])
               return i;
         }
         return -1;
      }
      public void unselectObject(EditorObject obj)
      {
         mSelectedEditorObjects.Remove(obj);
         List<EditorObject> l = obj.getChildComponents();
         if (l != null)
            for (int i = 0; i < l.Count; i++)
               mSelectedEditorObjects.Remove(l[i]);
      }
      public bool selectedListChanged()
      {
         if (mSelectedEditorObjects.Count != mLastSelectedItemCount || mSelectionListChanged)
         {
            mLastSelectedItemCount = mSelectedEditorObjects.Count;
            mSelectionListChanged = false;
            return true;
         }
         return false;
      }
      public void selectAll()
      {
         for(int i=0;i<mEditorObjects.Count;i++)
         {
            addSelectedObject(mEditorObjects[i]);
         }
      }
      public void unselectAll()
      {
         mSelectedEditorObjects.Clear();
         mTransWidget.clear();
         mRotWidget.clear();
      }

      public List<EditorObject> selectItemsRayCast()
      {
         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         List<EditorObject> objects = new List<EditorObject>();
         EditorObject closestObj = null;
         
         Vector3 intersectionPoint = Vector3.Empty;
         CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref orig, ref dir, ref intersectionPoint);
         Vector3 diff = intersectionPoint - orig;
         float tVal = diff.Length();


         float closestDist = tVal;
         foreach (EditorObject obj in mVisibleEditorObjects)
         {
            if (IsSelectable(obj) == false)
            {
               continue;
            }

            if (obj.testForRayIntersection(orig, dir, ref tVal))
            {
               if (tVal <= closestDist)
               {
                  closestDist = tVal;
                  closestObj = obj;
               }
            }
         }
         if (closestObj != null)
         {
          //  if (!mSelectedEditorObjects.Contains(closestObj))
               objects.Add(closestObj);
         }

         //System.Diagnostics.Debug.WriteLine(string.Format("Raycast hit {0} objects.",objects.Count));

         return objects;
      }
      public List<EditorObject> selectItemsDragBox()
      {
         List<EditorObject> objects = new List<EditorObject>();

         Vector3[] points = new Vector3[8];

         Point min = mPressPoint;
         Point max = mReleasePoint;

         if(min.X > max.X)
         {
            min.X = max.X;
            max.X = mPressPoint.X;
         }
         if (min.Y > max.Y)
         {
            min.Y = max.Y;
            max.Y = mPressPoint.Y;
         }

         Point mid0 = new Point();
         Point mid1 = new Point();
         mid1.X = min.X;
         mid1.Y = max.Y;
         mid0.X = max.X;
         mid0.Y = min.Y;


         //TODO : this sucks. VERY ineffecient. We should only be doing 4 unprojects, and interpolating the rest
         //we're currently doing 8..
         points[0] = BRenderDevice.getRayPosFromMouseCoords(false, min);
         points[4] = BRenderDevice.getRayPosFromMouseCoords(true, min);

         points[1] = BRenderDevice.getRayPosFromMouseCoords(false, mid0);
         points[5] = BRenderDevice.getRayPosFromMouseCoords(true, mid0);

         points[2] = BRenderDevice.getRayPosFromMouseCoords(false, mid1);
         points[6] = BRenderDevice.getRayPosFromMouseCoords(true, mid1);

         points[3] = BRenderDevice.getRayPosFromMouseCoords(false, max);
         points[7] = BRenderDevice.getRayPosFromMouseCoords(true, max);

         foreach (EditorObject obj in mVisibleEditorObjects)
         {
            if(IsSelectable(obj) == false)
            {
               continue;
            }
           // Vector3 pos = obj.getPosition();
           // if(BMathLib.boxAABBIntersect(obj.mAABB.min + pos,obj.mAABB.max + pos,
            //   nearPts[0], nearPts[1], nearPts[2], nearPts[3], farPts[0], farPts[1], farPts[2], farPts[3]))
            if (obj.testForBoxIntersection(points) && (!mSelectedEditorObjects.Contains(obj)))
            {
               objects.Add(obj);
            }
         }

         //System.Diagnostics.Debug.WriteLine(string.Format("Box selected {0} objects.", objects.Count));


         return objects;
      }
      public List<EditorObject> selectItemsAABB(BBoundingBox worldAABB)
      {
         List<EditorObject> objects = new List<EditorObject>();
         foreach (EditorObject obj in mEditorObjects)
         {
            if (IsSelectable(obj) == false)
            {
               continue;
            }
            BBoundingBox bb = new BBoundingBox();
            bb.addPoint(obj.mAABB.min);
            bb.addPoint(obj.mAABB.max);
            bb.min += obj.getPosition();
            bb.max += obj.getPosition();
            if (bb.intersect(worldAABB))
            {
               objects.Add(obj);
            }
         }
         return objects;
      }

      public bool IsSelectable(EditorObject obj)
      {
         if(obj.IsSelectable() == false)
         {
            return false;
         }

         if(mbLockSimHelpers == true  && obj is SimHelperObject)
         {
            return false;
         }
         else if (mbLockLights == true && (obj is LocalLight || obj is SpotLight || obj is VisualControlObject))
         {
            return false;
         }
         else if (obj is IHasGroup)
         {
            int groupID = ((IHasGroup)obj).Group;
            ObjectGroup g;
            if (TryGetGroup(groupID, out g) == true)
            {
               if (g.Locked == true)
                  return false;
               //else
               //   return true;
            }
            else
            {
               ((IHasGroup)obj).Group = -1;
            }
         }

         //if (obj is ISupportsScenarioOwner)
         //{
         //   int owner = ((ISupportsScenarioOwner)obj).Department;

         //   if ((mOwnerPermissions & owner) == 0)
         //   {
         //      return false;
         //   }
         //}
         if ((obj.GetDepartment() & mCurrentDepartmentMode) == 0)
         {
            return false; 
         }

         return true;
      }

      int mOwnerPermissions = 0;//(int)(eScenarioOwner.cDesign | eScenarioOwner.cSound | eScenarioOwner.cArt);
      int mCurrentDepartmentMode = (int)(eDepartment.Design);
      int mDeletePermission = 0;
      int mMovePermission = 0;

      public int GetCurrentDepartmentMode()
      {
         return mCurrentDepartmentMode;
      }
      public void SetCurrentDepartmentMode(int mode)
      {
         mCurrentDepartmentMode = mode;

         mOwnerPermissions = mode;

         mSelectedEditorObjects.Clear();
         mSelectionListChanged = true;

         mCopiedObjects.Clear();

         CoreGlobals.getSettingsFile().DepartementMode = mode;
         CoreGlobals.getSettingsFile().Save();

      }
      //Save default?

      public void TransferID(SimObjectData data, eDepartment oldDepartment, eDepartment newDepartment)
      {
         if (oldDepartment == newDepartment)
         {
            return;
         }

         if (newDepartment == eDepartment.Design)
         {
            if (HasDesignID(data.ID) == false)
            {
               data.ID = this.GetUniqueID();
            }
         }
         else if (newDepartment == eDepartment.Art)
         {
            if (HasArtID(data.ID) == false)
            {
               data.ID = this.GetUniqueArtID();
            }
         }
         else if (newDepartment == eDepartment.Sound)
         {
            if (HasSoundID(data.ID) == false)
            {
               data.ID = this.GetUniqueSoundID();
            }
         }
      }

      /// <summary>
      /// This is to be use when loading a scenario only
      /// </summary>
      /// <param name="data"></param>
      /// <param name="department"></param>
      public void InitID(SimObject obj, eDepartment department)
      {
         SimObjectData data = obj.GetProperties() as SimObjectData;
         if (data == null)
            return;

         if (department == eDepartment.Design)
         {
            if (HasDesignID(data.ID) == false)
            {
               data.ID = this.GetUniqueID();
            }
         }
         else if (department == eDepartment.Art)
         {
            if (HasArtID(data.ID) == false)
            {
               data.ID = this.GetUniqueArtID();
            }
         }
         else if (department == eDepartment.Sound)
         {
            if (HasSoundID(data.ID) == false)
            {
               data.ID = this.GetUniqueSoundID();
            }
         }

      }
      public bool IsRenderable(EditorObject obj)
      {
         if (mbDisableSmartHide == false && (obj.GetDeptVisPermission() & mCurrentDepartmentMode) == 0)
            return false;

         if(obj.IsVisible() == false)
         {
            return false;
         }

         //if(obj.)
         if (obj is IHasGroup)
         {
            int groupID = ((IHasGroup)obj).Group;
            ObjectGroup g;
            if (TryGetGroup(groupID, out g) == true)
            {
               if (g.Visible == true)
                  return true;
               else
                  return false;
            }
            else
            {
               ((IHasGroup)obj).Group = -1;
            }
         }
         return true;
      }
      public bool IsRenderBB(EditorObject obj)
      {
         if (obj is IHasGroup)
         {
            int groupID = ((IHasGroup)obj).Group;
            ObjectGroup g;
            if (TryGetGroup(groupID, out g) == true)
            {
               return g.BB;
            }
            else
            {
               ((IHasGroup)obj).Group = -1;
            }
         }
         return false;
      }


      public void ClearGroupCache()
      {
         mGroupHash.Clear();
      }
      bool TryGetGroup(int id, out ObjectGroup g)
      {
         if (mGroupHash.ContainsKey(id) == false)
         {
            g = this.ObjectGroups.Find(new Predicate<ObjectGroup>(delegate(ObjectGroup compareTo) { return id == compareTo.ID; }));

            if (g != null)
            {
               mGroupHash[id] = g;
               return true;
            }
            else
            {

               return false;
            }
         }
         else
         {
            g = mGroupHash[id];
            return true;
         }
      }

      Dictionary<int, ObjectGroup> mGroupHash = new Dictionary<int, ObjectGroup>();


      public double Distance(Point A, Point B)
      {
         double val = ((A.X - B.X) * (A.X - B.X)) + ((A.Y - B.Y) * (A.Y - B.Y));
         return Math.Sqrt(val);
      }
      public double Area(Point A, Point B)
      {
         double val = Math.Abs(A.X - B.X) * Math.Abs(A.Y - B.Y);
         return val;
      }
      public bool IsGoodBox(Point A, Point B)
      {
         double distance = Distance(A,B);
         double area = Area(A,B);
         //double ratio = Ratio(A, B);
         if ((distance > 20) && (area > 10))// && (ratio < 100))
            return true;
         else
            return false;

      }

      public void doSelection(bool delFromSelection)
      {
         List<EditorObject> objects = null;

         //mPressPoint == mReleasePoint
         if (IsGoodBox(mPressPoint, mReleasePoint))
         {
            objects = selectItemsDragBox();
         }
         else
         {
            objects = selectItemsRayCast();
         }
  
         //System.Diagnostics.Debug.WriteLine(string.Format("mSelectedEditorObjects.Count = {0} ", mSelectedEditorObjects.Count));

         //mSelectableEditorObjects

         if (objects != null && objects.Count > 0)
         {
            if (delFromSelection)
            {
               for (int i = 0; i < objects.Count;i++ )
                  mSelectedEditorObjects.Remove(objects[i]);
            }
            else
            {
               //mSelectedEditorObjects.AddRange(objects);
               foreach(EditorObject toselect in objects)
               {    
                  if (mSelectableEditorObjects == null || mSelectableEditorObjects.Contains(toselect))
                  {
                     ISubControl sub = toselect as ISubControl;
                     if (sub != null)
                     {
                        if (sub.GetOwner() == null || mEditMasterObjects.Contains( (EditorObject)sub.GetOwner()) == false)
                        {
                           continue;
                        }
                     }

                     mSelectedEditorObjects.Add(toselect);
                  }

               }

               
            }
            updateWidgetPos();
         }

         for (int i = 0; i < mSelectedEditorObjects.Count; i++)
            mSelectedEditorObjects[i].postSelectCB();
         mSelectionListChanged = true;
      }

      private void update2DSelectionBox()
      {
         Point tPt = new Point();
         Point kPt = mPressPoint;
         UIManager.GetCursorPos(ref tPt);
         BRenderDevice.getScreenToD3DCoords(ref tPt);
         BRenderDevice.getScreenToD3DCoords(ref kPt);

         if (m2DSelectionBox == null)
            m2DSelectionBox = new VertexBuffer(typeof(VertexTypes.PosW_color), 8, BRenderDevice.getDevice(), Usage.None, VertexTypes.PosW_color.FVF_Flags, Pool.Managed);

         VertexTypes.PosW_color[] verts = new VertexTypes.PosW_color[]
         {
            new VertexTypes.PosW_color(kPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(tPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(tPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(tPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(tPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(kPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(kPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(kPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
         };

         GraphicsStream gStream = m2DSelectionBox.Lock(0, 0, LockFlags.None);
         gStream.Write(verts);
         m2DSelectionBox.Unlock();
         verts = null;

      }
      private void render2DSelectionBox()
      {
         if (mShowDragBox)
         {
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;

            BRenderDevice.getDevice().SetTexture(0, null);
            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.PosW_color.vertDecl;
            BRenderDevice.getDevice().SetStreamSource(0, m2DSelectionBox, 0);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, 4);
         }
      }

      #endregion

      #region WIDGETS
      public Vector3 giveWidgetPos()
      {
         return mTransWidget.getPosition();
      }
      public void setWidgetPos(Vector3 pos)
      {
         mTransWidget.update(pos);
         mRotWidget.update(pos);
      }
      private void updateWidgetPos()
      {
         if (mSelectedEditorObjects.Count > 0)
         {
            Vector3 avgObj = new Vector3(0, 0, 0);
            for (int i = 0; i < mSelectedEditorObjects.Count; i++)
               avgObj += mSelectedEditorObjects[i].getPosition();
            avgObj.X /= (float)(mSelectedEditorObjects.Count);
            avgObj.Y /= (float)(mSelectedEditorObjects.Count);
            avgObj.Z /= (float)(mSelectedEditorObjects.Count);

            mTransWidget.update(avgObj);
            mRotWidget.update(avgObj);
         }
      }

      //translation
      public Vector3 giveDominantPlane()
      {
         Vector3 dir = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget() - CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
         dir =BMathLib.Normalize(dir);

         float angle = Vector3.Dot(dir, new Vector3(0, 1, 0));
         if (Math.Abs(angle) > 0.01)
            return new Vector3(0, 1, 0);

         angle = Vector3.Dot(dir, new Vector3(1, 0, 0));
         if (Math.Abs(angle) > 0.01)
            return new Vector3(1, 0, 0);

         angle = Vector3.Dot(dir, new Vector3(0, 0, 1));
         if (Math.Abs(angle) > 0.01)
            return new Vector3(0, 0, 1);

         return dir;
      }
      public Vector3 giveWidgetSelectionPlane()
      {
         Vector3 pN = new Vector3(1, 1, 1);
         mTransWidget.capMovement(ref pN);
         bool x = pN.X > 0;
         bool y = pN.Y > 0;
         bool z = pN.Z > 0;

         if (x && !y && z) return new Vector3(0,1,0);

         Vector3 dir = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget() - CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
         dir =BMathLib.Normalize(dir);

         return dir;

    
      }
      public bool forceStandardMode()
      {
         bool sameObj = true;
         for(int i=0;i<mSelectedEditorObjects.Count;i++)
            sameObj |= mSelectedEditorObjects[i].mLockToTerrain;

         return !sameObj;
      }

      public void moveObjects(bool standardMode)
      {
         //force to standard mode if we have standard objects in our group selection
         if (forceStandardMode())
            standardMode = true;

         //GET OUR ORIGIONAL RAY
         Vector3 press_r0 = BRenderDevice.getRayPosFromMouseCoords(false, mPressPoint);
         Vector3 press_rD = BRenderDevice.getRayPosFromMouseCoords(true, mPressPoint) - press_r0;
         press_rD=BMathLib.Normalize(press_rD);

         //GET OUR NEW RAY
         Point p = new Point();
         UIManager.GetCursorPos(ref p);
         Vector3 r0 = BRenderDevice.getRayPosFromMouseCoords(false, p);
         Vector3 rD = BRenderDevice.getRayPosFromMouseCoords(true, p) - r0;
         rD=BMathLib.Normalize(rD);

         if (standardMode)
            mTransWidget.setSelected(true, false, true);

         //compute our intersection with the terrain
         Vector3 oldMovePointLTT = Vector3.Empty;
         Vector3 desiredMovePointLTT = Vector3.Empty;
         bool hit1 = CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref press_r0, ref press_rD, ref oldMovePointLTT);
         bool hit2 = CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref r0, ref rD, ref desiredMovePointLTT);

         if (hit1 != hit2)
            hit1 = false;
         //compute our intersection with the best plane
         Vector3 oldMovePointBP = Vector3.Empty;
         Vector3 desiredMovePointBP = Vector3.Empty;
         Vector3 pN = Vector3.Empty;
         if (standardMode)            pN = giveDominantPlane();
         else            pN = giveWidgetSelectionPlane();
         Plane pl = Plane.FromPointNormal(mTransWidget.mTranslation, pN);
         float tVal = 0;
         BMathLib.rayPlaneIntersect(pl, r0, rD, false, ref tVal);
         desiredMovePointBP = r0 + (rD * tVal);
         BMathLib.rayPlaneIntersect(pl, press_r0, press_rD, false, ref tVal);
         oldMovePointBP = press_r0 + (press_rD * tVal);

         EditorObjectCounter counter = new EditorObjectCounter();
 
         //now that we have our rays, compute our movement


         List<EditorObject> objectsToMove = new List<EditorObject>();
         objectsToMove.AddRange(mSelectedEditorObjects);
         foreach (EditorObject masterObject in mSelectedEditorObjects)
         {
            if (masterObject is IPointOwner) //consider making point owner more generic
            {
               if (this.mEditMasterObjects.Contains(masterObject) == false) //false, so that this object is not in detail edit mode
               {
                  objectsToMove.AddRange(masterObject.getChildComponents());
               }
            }

         }

         foreach (EditorObject obj in objectsToMove)
         {
            counter.Count(obj);

            Vector3 pos = obj.getPosition();
            if(obj.mLockToTerrain)
            {
               Vector3 diff = desiredMovePointLTT - oldMovePointLTT;
               mTransWidget.capMovement(ref diff);

                  Vector3 org = pos + diff;
                  org.Y = 200;
                  
               //CLM  [08.11.08]
                  org.Y = CoreGlobals.getEditorMain().mITerrainShared.getSimRepHeightInterpolated(ref org);
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(org));

                  //Vector3 pdir = new Vector3(0, -1, 0);
                  //Vector3 intersectionPoint = new Vector3();
                  //if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  //   obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));
                  //else
                   //  hit2 = false;
            }
            else
            {

               Vector3 diff = desiredMovePointBP - oldMovePointBP;
                  mTransWidget.capMovement(ref diff);

                  Vector3 org = pos + diff;
                  if(standardMode)
                     org.Y = obj.getPosition().Y;

                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(org));
            }

            //tell each object it was moved, incase it has child components
            obj.moved();

           // if(obj is LocalLight)
           // {
           //    int k = getObjectIndex(obj);
           //    CoreGlobals.getEditorMain().mITerrainShared.simEditorLightMoved(k);
          //  }
         
         }

         counter.SmartSetDirty();

         updateWidgetPos();       
      }


      public void moveObjectsExplicit(float xoffset, float yoffset, float zoffset, bool Relative)
      {
         EditorObjectCounter counter = new EditorObjectCounter();

         Vector3 oldMovePoint = mTransWidget.getPosition();
         Vector3 desiredMovePoint = new Vector3(xoffset,yoffset,zoffset);
         if (Relative)
            desiredMovePoint += oldMovePoint;


         List<EditorObject> objectsToMove = new List<EditorObject>();
         objectsToMove.AddRange(mSelectedEditorObjects);
         foreach (EditorObject masterObject in mSelectedEditorObjects)
         {
            if (masterObject is IPointOwner) //consider making point owner more generic
            {
               if (this.mEditMasterObjects.Contains(masterObject) == false) //false, so that this object is not in detail edit mode
               {
                  objectsToMove.AddRange(masterObject.getChildComponents());
               }
            }

         }

         foreach (EditorObject obj in objectsToMove)
         {
            counter.Count(obj);

            Vector3 pos = obj.getPosition();
            if (obj.mLockToTerrain)
            {
               Vector3 diff = desiredMovePoint - oldMovePoint;
             //  mTransWidget.capMovement(ref diff);

               Vector3 org = pos + diff;
               org.Y = 200;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));
            }
            else
            {

               Vector3 diff = desiredMovePoint - oldMovePoint;
              // mTransWidget.capMovement(ref diff);

               Vector3 org = pos + diff;
               
               obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(org));
            }

            //tell each object it was moved, incase it has child components
            obj.moved();

          

         }

         counter.SmartSetDirty();

         updateWidgetPos();       
      }
      public void forcedObjectTranslation(float xoffset, float yoffset, float zoffset)
      {
         EditorObjectCounter counter = new EditorObjectCounter();

         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            counter.Count(obj);

            Vector3 pos = obj.getPosition();
            if (obj.mLockToTerrain)
            {

               Vector3 org = pos;
               org.X += xoffset;
               org.Y = 200;
               org.Z += zoffset;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));

            }
            else
            {

               Vector3 org = pos;
               org.X += xoffset;
               org.Y = 200;
               org.Z += zoffset;

               obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(org));
            }

            //tell each object it was moved, incase it has child components
            obj.moved();

         }

         counter.SmartSetDirty();

         updateWidgetPos();       
      }
      public void forcedObjectRelativeTranslation(float oldXWorldSize, float oldZWorldSize, float newXWorldSize, float newZWorldSize)
      {
         EditorObjectCounter counter = new EditorObjectCounter();

         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            counter.Count(obj);

            Vector3 pos = obj.getPosition();
            if (obj.mLockToTerrain)
            {
               float relX = pos.X / oldXWorldSize;
               float relZ = pos.Z / oldZWorldSize;
               Vector3 org = pos;
               org.X = relX * newXWorldSize;
               org.Y = 200;
               org.Z = relZ * newZWorldSize;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));

            }
            else
            {

               float relX = pos.X / oldXWorldSize;
               float relZ = pos.Z / oldZWorldSize;
               Vector3 org = pos;
               org.X = relX * newXWorldSize;
               //org.Y = 200;
               org.Z = relZ * newZWorldSize;

               obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(org));
            }

            //tell each object it was moved, incase it has child components
            obj.moved();

         }

         counter.SmartSetDirty();

         updateWidgetPos();
      }
     
      //rotation
      public void rotateObjects(bool grouped)
      {
         //calculate the length of movement on our far plane.
         Point p = new Point();
         UIManager.GetCursorPos(ref p);

         Vector3 ptA = BRenderDevice.getRayPosFromMouseCoords(false, mPressPoint);
         Vector3 dir = BRenderDevice.getRayPosFromMouseCoords(false, p) - ptA;
         Vector3 sign = new Vector3(dir.X > 0 ? 1 : -1, dir.Y > 0 ? 1 : -1, dir.Z > 0 ? 1 : -1);
         float k = dir.Length();

         rotateObjects(sign.X * k, grouped);
      }
      public void rotateObjects(float kamt, bool grouped)
      {
         Vector3 amt = new Vector3(1, 1, 1);
         mRotWidget.capRotation(ref amt);


         EditorObjectCounter counter = new EditorObjectCounter();


         List<EditorObject> objectsToMove = new List<EditorObject>();
         objectsToMove.AddRange(mSelectedEditorObjects);
         foreach (EditorObject masterObject in mSelectedEditorObjects)
         {
            if (masterObject is IPointOwner) //consider making point owner more generic
            {
               if (this.mEditMasterObjects.Contains(masterObject) == false) //false, so that this object is not in detail edit mode
               {
                  objectsToMove.AddRange(masterObject.getChildComponents());
               }
            }

         }

         //if (mSelectedEditorObjects.Count == 1)
          //  grouped = false;
         foreach (EditorObject edObj in objectsToMove)
         {
            counter.Count(edObj);
            {
               edObj.mRotation.X += kamt;
               edObj.mRotation.Y += kamt;
               edObj.mRotation.Z += kamt;

               Vector3 orig = edObj.getPosition();
               Matrix mat = edObj.getRotationOnly();

               edObj.setMatrix(mat * Matrix.RotationAxis(amt, kamt) * Matrix.Translation(orig));
            }

            if (grouped)   //rotate objects around the grouped axis
            {
               Vector3 loc = edObj.getPosition();
               Matrix m = Matrix.Translation(-mRotWidget.mTranslation) * Matrix.RotationY(kamt) * Matrix.Translation(mRotWidget.mTranslation);
               Vector4 pos = Vector3.Transform(edObj.getPosition(),m);               
               
               if (edObj.mLockToTerrain)
               {
                  Vector3 org = new Vector3(pos.X, pos.Y, pos.Z);
                  org.Y = 200;
                  Vector3 pdir = new Vector3(0, -1, 0);
                  Vector3 intersectionPoint = new Vector3();
                  if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                     edObj.setPosition(intersectionPoint);
                     
               }
               else
               {
                  edObj.setPosition(new Vector3(pos.X, pos.Y, pos.Z));
               }
            }
            //else
            
            edObj.updateBB();
            edObj.rotated();
         }


         counter.SmartSetDirty();
      }
      public void rotateObjectsExplicit(float xoffset, float yoffset, float zoffset, bool Relative, bool grouped)
      {
         EditorObjectCounter counter = new EditorObjectCounter();


         List<EditorObject> objectsToMove = new List<EditorObject>();
         objectsToMove.AddRange(mSelectedEditorObjects);
         foreach (EditorObject masterObject in mSelectedEditorObjects)
         {
            if (masterObject is IPointOwner) //consider making point owner more generic
            {
               if (this.mEditMasterObjects.Contains(masterObject) == false) //false, so that this object is not in detail edit mode
               {
                  objectsToMove.AddRange(masterObject.getChildComponents());
               }
            }

         }

         foreach (EditorObject edObj in objectsToMove)
         {
            counter.Count(edObj);
            {
               if(Relative)
               {
                  edObj.mRotation.X += xoffset;
                  edObj.mRotation.Y += yoffset;
                  edObj.mRotation.Z += zoffset;
               }
               else
               {
                  edObj.mRotation.X = xoffset;
                  edObj.mRotation.Y = yoffset;
                  edObj.mRotation.Z = zoffset;
               }
               

               Vector3 orig = edObj.getPosition();
               Matrix mat = edObj.getRotationOnly();

               edObj.setMatrix(mat * Matrix.RotationX(xoffset) * Matrix.RotationY(yoffset) * Matrix.RotationZ(zoffset) * Matrix.Translation(orig));
            }

            if (grouped)   //rotate objects around the grouped axis
            {
               Vector3 loc = edObj.getPosition();
               Matrix m = Matrix.Translation(-mRotWidget.mTranslation) * Matrix.RotationX(xoffset) * Matrix.RotationY(yoffset) * Matrix.RotationZ(zoffset) * Matrix.Translation(mRotWidget.mTranslation);
               Vector4 pos = Vector3.Transform(edObj.getPosition(), m);

               if (edObj.mLockToTerrain)
               {
                  Vector3 org = new Vector3(pos.X, pos.Y, pos.Z);
                  org.Y = 200;
                  Vector3 pdir = new Vector3(0, -1, 0);
                  Vector3 intersectionPoint = new Vector3();
                  if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                     edObj.setPosition(intersectionPoint);

               }
               else
               {
                  edObj.setPosition(new Vector3(pos.X, pos.Y, pos.Z));
               }
            }
            //else

            edObj.updateBB();
            edObj.rotated();
         }


         counter.SmartSetDirty();
      }
      #endregion

      //utils
      public Vector3 getRayPosFromMouseCoords(bool farPlane)
      {
         return getRayPosFromMouseCoords(farPlane, Point.Empty);
      }
      public Vector3 getRayPosFromMouseCoords(bool farPlane, Point pt)
      {
         Point cursorPos = Point.Empty;
         if (pt == Point.Empty)
            UIManager.GetCursorPos(ref cursorPos);
         else
            cursorPos = pt;

         return BRenderDevice.getRayPosFromMouseCoords(farPlane, cursorPos);
      }


      public List<EditorObject> mEditorObjects = new List<EditorObject>();
      public List<EditorObject> mSelectedEditorObjects = new List<EditorObject>();
      public List<EditorObject> mSelectableEditorObjects = null;
      public List<EditorObject> mVisibleEditorObjects = null;


      //public List<EditorObject> mExtraArtObjects = new List<EditorObject>();
      //public List<EditorObject> mExtraSoundObjects = new List<EditorObject>();


      public SimFileData mSimFileData = new SimFileData();


      public void ClearSim()
      {
         ObjectGroups.Clear();

         DiplomacyData = new List<DiplomacyXml>();
         PlayerData = new Players() ;
         ObjectivesData = new ObjectivesXML();
         PlayerPlacementData = new PlayerPlacement();
         TriggerData = new List<TriggerRoot>();
         ExternalScripts = new List<ExternalScriptInfo>();
         ObjectGroups = new List<ObjectGroup>();



         mEditorObjects.Clear();
         mSelectedEditorObjects.Clear();
         mVisibleEditorObjects = mEditorObjects;
         mVisibleEditorObjects = mEditorObjects;

         ResetIDSystem();


         CoreGlobals.getGameResources().ClearCinematics();
#if SAVELIGHTSETS
         CoreGlobals.getGameResources().ClearLightsets();
         //EditorLightset c = new EditorLightset("Default", 0, true);
         //CoreGlobals.getGameResources().AddLightset(c);
#endif
         CoreGlobals.getGameResources().ClearTalkingHeadVideos();

         GrannyManager2.unload();
         BVisualManager.destroy();
      }
      public void ClearVisuals()
      {
         foreach(EditorObject obj in mEditorObjects)
            obj.clearVisuals();
         BVisualManager.destroy();
      }
    
      ////////////////////////////////////////////////////////////
 
      int mSelectedPlayerID = 1;
      public int SelectedPlayerID
      {
         set
         {
            mSelectedPlayerID = value;
            

            if (PlacementDummyObject != null)
            {
               if (PlacementDummyObject is PlayerPosition)
                  ((PlayerPosition)PlacementDummyObject).PlayerID = mSelectedPlayerID;

               if (PlacementDummyObject is SimObject)
                  ((SimObject)PlacementDummyObject).setPlayerID(mSelectedPlayerID);

            }
         }
         get
         {
            return mSelectedPlayerID;
         }
      }
      SimUnitXML mSelectedProtoUnit = null;
      public  SimUnitXML SelectedProtoUnit
      {
         set
         {
            mSelectedProtoUnit = value;
         }
         get
         {
            return mSelectedProtoUnit;
         }

      }

      ProtoSquadXml mSelectedProtoSquad = null;
      public ProtoSquadXml SelectedProtoSquad
      {
         set
         {
            mSelectedProtoSquad = value;
         }
         get
         {
            return mSelectedProtoSquad;
         }

      }


      LightXML mSelectedLightXML ;
      public LightXML SelectedLightData
      {
         set
         {
            mSelectedLightXML = value;
            
         }
         get
         {
            return mSelectedLightXML;
         }

      }


      ////////////////////////////////////////////////////////////

      //ui states

      EditorObject mPlacementDummyObject = null;
      public EditorObject PlacementDummyObject
      {
         set
         {
            mPlacementDummyObject = value;
         }
         get
         {
            return mPlacementDummyObject;
         }

      }


      //Object list operations
      public int getObjectIndex(EditorObject obj)
      {
         for(int i=0;i<mEditorObjects.Count;i++)
         {
            if (mEditorObjects[i] == obj)
               return i;
         }
         return -1;
      }

      public void AddObject(EditorObject obj)
      {
         AddObject(obj, mCurrentDepartmentMode);

         SimObject so = obj as SimObject;
         if (so != null)
         {
            InitID(so, (eDepartment)(so.Department));
         }
      }

      public void AddObject(EditorObject obj, int department)
      {
         if(obj is LocalLight)
         {
            mEditorObjects.Add(obj);
            CoreGlobals.getEditorMain().mITerrainShared.simEditorLightAdd(mEditorObjects.Count - 1);
         }
         else
         {
            //find the last light
            bool found = false;
            for(int i=mEditorObjects.Count-1;i>=0;i--)
            {
               if(!(mEditorObjects[i] is LocalLight))
               {
                  mEditorObjects.Insert(i, obj);
                  found = true;
                  break;
               }
            }
            if (!found)
               mEditorObjects.Insert(0, obj);
         }
         
         List<EditorObject> l = obj.getChildComponents();
         if (l != null)
            mEditorObjects.AddRange(l);

         EditorObjectCounter counter = new EditorObjectCounter();
         counter.Count(obj);
         counter.SmartSetDirty();

         //Set owner
         if (obj is ISupportsScenarioOwner)
         {
            ((ISupportsScenarioOwner)obj).Department = department;        
         }

         obj.OnPlaced();
      }
      public void RemoveObject(EditorObject obj)
      {
         if (obj is LocalLight)
         {
            int k = getObjectIndex(obj);
            CoreGlobals.getEditorMain().mITerrainShared.simEditorLightDelete(k);
         }

         obj.removed();
         mEditorObjects.Remove(obj);
         List<EditorObject> l = obj.getChildComponents();
         if (l != null)
         {
            for (int i = 0; i < l.Count; i++)
            {
               l[i].removed();
               mEditorObjects.Remove(l[i]);
            }
         }
      }
      public void LookAtObject(EditorObject obj)
      {
          CoreGlobals.getEditorMain().mITerrainShared.setCameraTarget(obj.getPosition());
      }
      public void LookAtSelectedObject()
      {
         if (SimGlobals.getSimMain().mSelectedEditorObjects.Count > 0)
         {
            Vector3 position = new Vector3();

            foreach(EditorObject obj in mSelectedEditorObjects)
            {
               position += obj.getPosition();
            }
            position.Scale(1.0f / mSelectedEditorObjects.Count);

            CoreGlobals.getEditorMain().mITerrainShared.setCameraTarget(position);
         }
      }
      public void TranslateToSelectedObject()
      {
         if (SimGlobals.getSimMain().mSelectedEditorObjects.Count > 0)
         {
            Vector3 position = new Vector3();

            foreach (EditorObject obj in mSelectedEditorObjects)
            {
               position += obj.getPosition();
            }
            position.Scale(1.0f / mSelectedEditorObjects.Count);

            Vector3 cameraTargetPos = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget();
            Vector3 delta = position - cameraTargetPos;

            CoreGlobals.getEditorMain().mITerrainShared.setCameraPos(CoreGlobals.getEditorMain().mITerrainShared.getCameraPos() + delta);
            CoreGlobals.getEditorMain().mITerrainShared.setCameraTarget(position);
            CoreGlobals.getEditorMain().mOneFrame = true;

         }
      }

      //used by worldObjectList for filtering
      public enum eFilterTypes
      {
         cFilterAll = 0,
         cFilterLocalLights,
         cFilterPlayerStarts,
         cFilterSimObj,
         cFilterFoliage
      }
      //CLM - this is gay. C# should be smater than forcing me to do this..
      private bool testType(eFilterTypes filterType, EditorObject obj)
      {
         switch (filterType)
         {
            case eFilterTypes.cFilterAll: return true; break;
            case eFilterTypes.cFilterLocalLights: return (obj is LocalLight); break;
            case eFilterTypes.cFilterPlayerStarts: return (obj is PlayerPosition); break;
            case eFilterTypes.cFilterSimObj: return (obj is SimObject); break;
            case eFilterTypes.cFilterFoliage: return false; break;
         }
         return false;
      }

      bool mbAllowDepartmentChanges = false;
      public bool AllowDepartmentChanges
      {
         get
         {
            return mbAllowDepartmentChanges;
         }
         set
         {
            mbAllowDepartmentChanges = value;
         }
      }

      public List<EditorObject> getEditorObjects(bool visibleOnly, eFilterTypes filterType, int playerID, bool forceToVisual)//, bool unlockVisible)
      {
         List<EditorObject> visibleObjects = new List<EditorObject>();
         
         foreach (EditorObject obj in mEditorObjects)
         {

            if ((obj.GetDepartment() & mCurrentDepartmentMode) == 0)
            {
               continue;
            }

            if (testType(filterType,obj))
            {
               if (obj is VisualControlObject)
               {
                  VisualControlObject k = obj as VisualControlObject;
                  if (!k.mAddToWorldList)
                     continue;
               }

               //do player ID filter, with sim obj filter
               if ((filterType==eFilterTypes.cFilterSimObj || filterType==eFilterTypes.cFilterAll) )//&& playerID > 0)
               {
                  if (obj is SimObject)
                  {
                     SimObject k = obj as SimObject;
                     if (playerID != -1 && k.getPlayerID() != playerID)
                        continue;
                  }
                  else if(playerID != -1)
                  {
                     continue;
                  }
               }
               

               //standard checks
               if (visibleOnly)
               {
                  Vector3 pos = obj.getPosition();
                  if (CoreGlobals.getEditorMain().mITerrainShared.getFustrum().AABBVisible(obj.mAABB.min + pos, obj.mAABB.max + pos))
                  {
                     visibleObjects.Add(obj);
                  }
               }
               else
               {
                  visibleObjects.Add(obj);
               }
            }
         }
         //if (forceToVisual)
         //{
         //   mVisibleEditorObjects = visibleObjects;
         //}
         //else
         //{
         //   mVisibleEditorObjects = mEditorObjects;
         //}
         
         return visibleObjects;
      }

      public void setVisibleObjects(List<EditorObject> objects)
      {

         mVisibleEditorObjects = objects;


      }

      public void setSelectableObjects(List<EditorObject> objects)
      {

         mSelectableEditorObjects = objects;


      }

      public EditorObject getEditorObject(int objIndex)
      {
         if (objIndex < 0 || objIndex >= mEditorObjects.Count)
            return null;
         return mEditorObjects[objIndex];
      }
      public void changeSelectedItemsOwningPlayer()
      {
         foreach (EditorObject obj in mSelectedEditorObjects)
         {
            if(obj is SimObject)
            {
               SimObject k = obj as SimObject;
               k.setPlayerID(SelectedPlayerID);
            }
         }
      }
    

      int mEditorListSize =0;
      public bool EditorListChanged()
      {
         if (mEditorListSize != mEditorObjects.Count)
         {
            mEditorListSize = mEditorObjects.Count;
            return true;
         }
         return false;
      }

      public bool mbShowBB = false;
      public void ToggleShowBB(bool showBB)
      {
         mbShowBB = showBB;
      }

      bool  mDoRendering=true;
      public void toggleRenderSimObjects()
      {
         mDoRendering = !mDoRendering;
      }

      bool mDoLightsRendering = true;
      public void toggleRenderLights()
      {
         mDoLightsRendering = !mDoLightsRendering;
         if (mDoLightsRendering == false)
            mbLockLights = true;
      }

      bool mUseDesignerControls = false;
      public void useDesignerControls(bool onOff)
      {
         mUseDesignerControls = onOff;
      }

      bool mbLockSimHelpers = false;
      public void toggleLockSimHelpers()
      {
         mbLockSimHelpers = !mbLockSimHelpers;
      }
      bool mbRenderSimHelpers = true;
      public void toggleRenderSimHelpers()
      {
         mbRenderSimHelpers = !mbRenderSimHelpers;
         if (mbRenderSimHelpers == false)
            mbLockSimHelpers = true;
      }
      bool mbLockLights = false;
      public void toggleLockLights()
      {
         mbLockLights = !mbLockLights;
         
      }



      public bool isUsingDesignerControls()
      {
         return mUseDesignerControls;
      }

      public void updateHeightsFromTerrain()
      {
         return;
         foreach (EditorObject obj in mEditorObjects)
         {
            if (!obj.mLockToTerrain)
               continue;
            {
               Vector3 org = obj.getPosition();
               org.Y = 200;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
               {
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));
               }
               else
               {
                  pdir = new Vector3(0, -1, 0);

               }
            }
         }
      }
      public void updateHeightsFromTerrain(Vector3 min, Vector3 max)
      {
        
         foreach (EditorObject obj in mEditorObjects)
         {
            if(!obj.mLockToTerrain)
               continue;
            Vector3 mMin = obj.mAABB.min + obj.getPosition();
            Vector3 mMax = obj.mAABB.max + obj.getPosition();
            if (!BMathLib.aabbsIntersect(ref min, ref max, ref mMin, ref mMax))
               continue;
      
            {
               Vector3 org =obj.getPosition();
               org.Y = 200;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint)); 
            }
         }
      }

      public void updateHeightsFromTerrain(List<EditorObject> objects)
      {
         foreach (EditorObject obj in objects)
         {
               Vector3 org = obj.getPosition();
               org.Y = 200;
               Vector3 pdir = new Vector3(0, -1, 0);
               Vector3 intersectionPoint = new Vector3();
               if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrainSimRep(ref org, ref pdir, ref intersectionPoint))
                  obj.setMatrix(obj.getRotationOnly() * Matrix.Translation(intersectionPoint));            
         }
      }
      public void updateHeightsFromTerrain(List<EditorObject> objects, bool bUseSlope, float alignAmount)
      {
         foreach (EditorObject obj in objects)
         {
            if (obj.mLockToTerrain)
               continue;
            if (obj.mAlignToTerrain)
               continue;

            Vector3 org = obj.getPosition();
            org.Y = 200;
            Vector3 pdir = new Vector3(0, -1, 0);
            Vector3 normal = new Vector3();
            int X = 0; int Z = 0;

            Vector3 intersectionPoint = new Vector3();
            if (CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrain(ref org, ref pdir, ref intersectionPoint, ref normal, ref X, ref Z))
            {
               Matrix rot = obj.getRotationOnly();

               if (bUseSlope)
               {

                  Vector3 up = new Vector3(0, 1, 0);
                  Vector3 forward = new Vector3(0, 0, 1);
                  Vector3 dir = new Vector3(rot.M31, 0, rot.M33);
                  dir.Normalize();
                  Matrix rot2 = BMathLib.makeRotateMatrix(dir, forward);


                  Vector3 newNormal = Vector3.Lerp(up, normal, alignAmount);



                  //Vector3 fwd = new Vector3(m.M31, m.M32, m.M33);
                  //fwd.Y = 0;
                  //fwd = BMathLib.Normalize(fwd);
                  double angle = Math.Acos(Vector3.Dot(new Vector3(0, 0, 1), dir));
                  //double finalAngle = angle;
                  if (dir.X < 0)
                     angle = -angle;

                  rot2 = Matrix.RotationAxis(newNormal, (float)angle);
                  rot = BMathLib.makeRotateMatrix(up, newNormal) * rot2;

               }

               obj.setMatrix(rot * Matrix.Translation(intersectionPoint));


            }
         }
      }
      private const int mIDArtBase   = 10000000;
      private const int mIDSoundBase = 30000000;

      private int mHighestID = -1;
      private int mHighestArtID = mIDArtBase;
      private int mHighestSoundID = mIDSoundBase;


      public int GetUniqueID()
      {
         mHighestID++;
         AddID(mHighestID);
         return mHighestID;
      }

      public int GetUniqueArtID()
      {
         mHighestArtID++;
         AddArtID(mHighestArtID);
         return mHighestArtID;
      }

      public int GetUniqueSoundID()
      {
         mHighestSoundID++;
         AddSoundID(mHighestSoundID);
         return mHighestSoundID;
      }

      public bool HasSoundID(int ID)
      {
         return (ID >= mIDSoundBase);
      }
      public bool HasArtID(int ID)
      {
         return (ID >= mIDArtBase) && (ID < mIDSoundBase);
      }
      public bool HasDesignID(int ID)
      {
         return ID < mIDArtBase;
      }

      public void RegisterID(int ID)
      {
         if (HasSoundID(ID))
         {
            RegisterSoundID(ID);
            return;
         }
         if (HasArtID(ID))
         {
            RegisterArtID(ID);
            return;
         }
         RegisterDesignID(ID);
      }

      //do not use directly
      private void RegisterDesignID(int ID)
      {
         if (ID > mHighestID)
            mHighestID = ID;
         AddID(ID);
      }

      //do not use directly
      private void RegisterArtID(int ID)
      {
         if (ID > mHighestArtID)
            mHighestArtID = ID;
         AddArtID(ID);
      }

      //do not use directly
      private void RegisterSoundID(int ID)
      {
         if (ID > mHighestSoundID)
            mHighestSoundID = ID;
         AddSoundID(ID);
      }

      private void AddID(int ID)
      {
         if (mUniqueIDs.Contains(ID))
         {
            throw new System.Exception("duplicate ID");
         }
         mUniqueIDs.Add(ID);
      }
      private void AddArtID(int ID)
      {
         if (mUniqueArtIDs.Contains(ID))
         {
            throw new System.Exception("duplicate art ID");
         }
         mUniqueArtIDs.Add(ID);
      }
      private void AddSoundID(int ID)
      {
         if (mUniqueSoundIDs.Contains(ID))
         {
            throw new System.Exception("duplicate sound ID");
         }
         mUniqueSoundIDs.Add(ID);
      }

      private void ResetIDSystem()
      {
         mHighestID = -1;
         mUniqueIDs.Clear();

         mHighestArtID = mIDArtBase;
         mUniqueArtIDs.Clear();

         mHighestSoundID = mIDSoundBase;
         mUniqueSoundIDs.Clear();
         ///
      }

      List<int> mUniqueIDs = new List<int>();
      List<int> mUniqueArtIDs = new List<int>();
      List<int> mUniqueSoundIDs = new List<int>();


      TranslationWidget mTransWidget =null;
      RotationWidget mRotWidget = null;

      public Vector3 mLastItemPlacePoint = new Vector3(float.MinValue,float.MinValue,float.MinValue);

      //public SimTileTypes mTileTypes = new SimTileTypes();

      private bool mLeftMouseDown = false;
      private bool mRightMouseDown = false;
      private bool mMiddleMouseDown = false;
      private Point mPressPoint = new Point();
      private Point mReleasePoint = new Point();

      bool mRegTranslationLocked = false;

      bool mShowDragBox = false;
      VertexBuffer m2DSelectionBox = null;
   }


}
