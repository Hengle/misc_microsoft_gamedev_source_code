using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Text.RegularExpressions;
using System.IO;
using System.Reflection;
using System.Collections;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;

using EditorCore;
using SimEditor;
using Rendering;
using Terrain;
using ModelSystem;
using PhoenixEditor.ScenarioEditor;

namespace PhoenixEditor.ClientTabPages 
{
   public partial class ObjectEditorPage : EditorCore.BaseClientPage, IUnitPicker
   {
      public ObjectEditorPage()
      {
         InitializeComponent();

         mObjectEditor = propertyList1;
         // Init material
         m_mtrl = new Material();
         m_mtrl.Ambient = Color.White;
         m_mtrl.Diffuse = Color.White;
         m_mtrl.Specular = Color.Black;


         if (s_renderAxisPrim == null)
         {
            s_renderAxisPrim = new BRenderDebugAxis(1);
         }

         GrannyManager2.init();

         LoadProtoXml();

         try
         {
            FileStream f = new FileStream(CoreGlobals.getWorkPaths().mEditorSettings + "\\ObjectEditor\\ObjectEditorUI.xml", FileMode.Open, FileAccess.Read);
            mObjectEditor.LoadSettingsFromStream(f);
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().SendToErrorWarningViewer(ex.ToString());
         }

         ChangedObjectsListBox.Click += new EventHandler(ChangedObjectsListBox_Click);
         mObjectEditor.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(betterPropertyGrid1_AnyPropertyChanged);

         //mObjectEditor.LastRowHack = true;
      }

      public override void update()
      {
         mCameraTransition.update(ref mCameraManager.mEye, ref mCameraManager.mLookAt);
         //base.update();
      }


      #region visual stuff
      private Grid m_grid = new Grid();
      public static BRenderDebugAxis s_renderAxisPrim = null;
      private BCameraManager mCameraManager = new BCameraManager();
      Matrix m_worldMatrix = Matrix.Identity;      
      private CameraTransition mCameraTransition = new CameraTransition();
      private List<BRenderGrannyMesh> mMeshes = new List<BRenderGrannyMesh>();
      private Material m_mtrl;
      private GrannyInstance.eVisualEditorRenderMode mRenderMode = GrannyInstance.eVisualEditorRenderMode.cRenderFull;
      private visual mVisualFile = null;
      private bool m_bShowModel = true;
      private bool m_bShowPlayerColor = false;
      private bool m_bShowSkeleton = false;
      private bool m_bShowBoneAxis = false;
      private bool m_bShowBoneNames = false;
      private bool m_bShowBoundingBox = false;
      private bool m_bShowMeshBoxes = false;
      private bool m_bShowStatsInfo = false;
      private bool m_bShowGrid = true;
      private BBoundingBox mModelBoundingBox = new BBoundingBox();
      #endregion

      #region visual stuff

      private void LoadVisual(string fileName)
      {
         //return;
         try
         {
            fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, fileName);

            visual visFile = BVisualManager.getOrLoadProtoVisual(fileName);

            if (visFile == null)
            {
               MessageBox.Show("Error Loading vis file");
               return;
            }
            // Create a copy of the visual
            mVisualFile = (visual)visFile.clone(true);

            // Update tree view
            //CreateTreeView();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error parsing visual file: {0}.  {1}", fileName, ex.ToString()));
         }


         //SetTabName(animFileName);

         // Init bounding box
         mModelBoundingBox.empty();

         preLoad(ref mModelBoundingBox);

         if (mModelBoundingBox.isEmpty())
         {
            mModelBoundingBox.addPoint(5.0f, 5.0f, 5.0f);
            mModelBoundingBox.addPoint(-5.0f, 0.0f, -5.0f);
         }


         mCameraManager.setModelBoundingBox(mModelBoundingBox);

         // Init camera position
         resetCamera(mModelBoundingBox);

         // Create grid
         m_grid.resetGrid(mModelBoundingBox);


         this.Activate();

      }

      #region Preload
      public void preLoadModel(visualModel curModel, ref BBoundingBox bbox)
      {
         if (curModel.component != null)
         {
            preLoadComponent(curModel.component, ref bbox);
         }
      }

      public void preLoadComponent(visualModelComponent curComponent, ref BBoundingBox bbox)
      {
         // Render component asset
         if (curComponent.asset != null)
         {
            preLoadComponentAsset(curComponent.asset, ref bbox);
         }
         else if (curComponent.logic != null)
         {
            preLoadLogic(curComponent.logic, ref bbox);
         }

         foreach (visualModelComponentOrAnimAttach curAttachment in curComponent.attach)
         {
            preLoadAttachment(curAttachment, ref bbox);
         }
      }

      public void preLoadComponentAsset(visualModelComponentAsset curCompAsset, ref BBoundingBox bbox)
      {
         // Render component asset
         switch (curCompAsset.type)
         {
            case visualModelComponentAsset.ComponentAssetType.Model:
               {
                  curCompAsset.loadAsset();
                  if (curCompAsset.mInstance != null)
                  {
                     bbox.addBox(curCompAsset.mInstance.mStaticBoundingBox);
                  }
               }
               break;
         }
      }

      public void preLoadAttachment(visualModelComponentOrAnimAttach curAttachment, ref BBoundingBox bbox)
      {
         switch (curAttachment.type)
         {
            case visualModelComponentOrAnimAttach.AttachType.ModelFile:
               curAttachment.loadAsset();
               break;
            case visualModelComponentOrAnimAttach.AttachType.ModelRef:
               foreach (visualModel curModel in mVisualFile.model)
               {
                  if (string.Compare(curModel.name, curAttachment.name, true) == 0)
                  {
                     preLoadModel(curModel, ref bbox);
                  }
               }
               break;
         }
      }

      public void preLoadLogic(visualLogic logic, ref BBoundingBox bbox)
      {
         // preload first child
         if (logic.logicdata.Count > 0)
         {
            visualLogicData curLogicData = logic.logicdata[0];
            preLoadLogicData(curLogicData, ref bbox);
         }
      }

      public void preLoadLogicData(visualLogicData logicData, ref BBoundingBox bbox)
      {

         if (!String.IsNullOrEmpty(logicData.modelref))
         {
            foreach (visualModel curModel in mVisualFile.model)
            {
               if (string.Compare(logicData.modelref, curModel.name, true) == 0)
               {
                  preLoadModel(curModel, ref bbox);
               }
            }
         }
         else if (logicData.asset != null)
         {
            preLoadComponentAsset(logicData.asset, ref bbox);
         }
         else if (logicData.logic != null)
         {
            preLoadLogic(logicData.logic, ref bbox);
         }
      }


      public void preLoad(ref BBoundingBox bbox)
      {
         // return if nothing is selected
         //if (VisualTreeView.SelectedNode == null)
         //   return;


         //TreeNode node = VisualTreeView.SelectedNode;
         //if (node.Tag != null)
         {
            visual visual = mVisualFile;// node.Tag as visual;
            if (visual != null)
            {
               foreach (visualModel curModel in visual.model)
               {
                  if (string.Compare(visual.defaultmodel, curModel.name, true) == 0)
                  {
                     preLoadModel(curModel, ref bbox);
                     return;
                  }
               }
            }

            //visualModel model = node.Tag as visualModel;
            //if (model != null)
            //{
            //   preLoadModel(model, ref bbox);
            //   return;
            //}

            //visualModelComponent component = node.Tag as visualModelComponent;
            //if (component != null)
            //{
            //   preLoadComponent(component, ref bbox);
            //   return;
            //}

            //visualModelComponentAsset componentAsset = node.Tag as visualModelComponentAsset;
            //if (componentAsset != null)
            //{
            //   preLoadComponentAsset(componentAsset, ref bbox);
            //   return;
            //}

            //visualModelComponentOrAnimAttach attach = node.Tag as visualModelComponentOrAnimAttach;
            //if (attach != null)
            //{
            //   preLoadAttachment(attach, ref bbox);
            //   return;
            //}

            //visualModelAnim anim = node.Tag as visualModelAnim;
            //if (anim != null)
            //{
            //   TreeNode modelNode = node.Parent;
            //   visualModel vmodel = modelNode.Tag as visualModel;
            //   if (vmodel != null)
            //   {
            //      preLoadModel(vmodel, ref bbox);
            //   }

            //   foreach (visualModelComponentOrAnimAttach curAttachment in anim.attach)
            //   {
            //      preLoadAttachment(curAttachment, ref bbox);
            //   }
            //   return;
            //}

            //visualModelAnimAsset animAsset = node.Tag as visualModelAnimAsset;
            //if (animAsset != null)
            //{
            //   // load anim
            //   animAsset.loadAsset();

            //   TreeNode animNode = node.Parent;
            //   visualModelAnim manim = animNode.Tag as visualModelAnim;
            //   if (manim != null)
            //   {
            //      TreeNode modelNode = animNode.Parent;
            //      visualModel vmodel = modelNode.Tag as visualModel;
            //      if (vmodel != null)
            //      {
            //         preLoadModel(vmodel, ref bbox);
            //      }

            //      foreach (visualModelComponentOrAnimAttach curAttachment in manim.attach)
            //      {
            //         preLoadAttachment(curAttachment, ref bbox);
            //      }
            //   }
            //   return;
            //}

            //visualModelAnimAssetTag animTag = node.Tag as visualModelAnimAssetTag;
            //if (animTag != null)
            //{
            //   visualModelAnimAsset animAsset2 = node.Parent.Tag as visualModelAnimAsset;
            //   if (animAsset2 != null)
            //   {
            //      // load anim
            //      animAsset2.loadAsset();

            //      TreeNode animNode = node.Parent.Parent;
            //      visualModelAnim manim = animNode.Tag as visualModelAnim;
            //      if (manim != null)
            //      {
            //         TreeNode modelNode = animNode.Parent;
            //         visualModel vmodel = modelNode.Tag as visualModel;
            //         if (vmodel != null)
            //         {
            //            preLoadModel(vmodel, ref bbox);
            //         }

            //         foreach (visualModelComponentOrAnimAttach curAttachment in manim.attach)
            //         {
            //            preLoadAttachment(curAttachment, ref bbox);
            //         }
            //      }
            //   }
            //   return;
            //}

            //visualModelComponentPoint point = node.Tag as visualModelComponentPoint;
            //if (point != null)
            //{
            //   return;
            //}

            //visualLogic logic = node.Tag as visualLogic;
            //if (logic != null)
            //{
            //   preLoadLogic(logic, ref bbox);
            //   return;
            //}

            //visualLogicData logicData = node.Tag as visualLogicData;
            //if (logicData != null)
            //{
            //   preLoadLogicData(logicData, ref bbox);
            //   return;
            //}
         }
      }
      #endregion

      public override void Activate()
      {
         base.Activate();


         BRenderDevice.setZNearPlane(0.1f);
         BRenderDevice.setZFarPlane(1000f);

         //this call passes in the panel width and height
         CoreGlobals.getEditorMain().mIGUI.deviceResize(this.panel1.Width, this.panel1.Height, false);

         //this call passes in the panel handle in which you want to render to.
         //this is handled during the 'present' call
         BRenderDevice.setWindowTarget(this.panel1);
      }

      public override Control GetUITarget()
      {
         return this.panel1;
      }

      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {

      }

      //CLM [04.26.06] called when this page is closed from the main tab
      public override void destroyPage()
      {
         base.destroyPage();

         //deinitDeviceData();
         //deinit();

         //for (int i = 0; i < mMeshes.Count; i++)
         //   mMeshes[i].destroy();

         m_grid = null;
      }

      //these two functions are called when the tab is created, and deinitalized respectivly
      public override void init()
      {
         base.init();

         // Init camera position
         mCameraManager.setCamMode(BCameraManager.eCamModes.cCamMode_ModelView);
         mCameraManager.mEye = new Vector3(10.0f, 10.0f, 10.0f);
         mCameraManager.mLookAt = new Vector3(0.0f, 0.0f, 0.0f);
      }


      VertexBuffer mIdentityInstanceVB = null;
      void makeInstanceVB()
      {
         mIdentityInstanceVB = new VertexBuffer(typeof(Matrix), 1, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.None, Pool.Managed);
         mIdentityInstanceVB.SetData(Matrix.Identity, 0, LockFlags.None);


      }
      override public void render()
      {
         // Check if there is anything to render
         if (mVisualFile != null)
         {
            //return;

            if (mIdentityInstanceVB == null)
               makeInstanceVB();
            BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
            BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
            BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);

         }


      //override public void render()
      //{
      //   // Check if there is anything to render
      //   //if (mVisualFile == null)
      //   //   return;

      //   // apply animation state
      //   //m_animationControl.apply(ref m_worldMatrix);


         base.render();


         mCameraManager.camUpdate();

         BRenderDevice.clear(true, true, unchecked((int)0xff000000), 1.0f, 0);
         BRenderDevice.getDevice().SetTransform(TransformType.World, Matrix.Identity);

         BRenderDevice.beginScene();



         GrannyInstance.setRenderMode(mRenderMode);


         // Linear filtering
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MinFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MagFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MipFilter, (int)TextureFilter.Linear);

         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MinFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MagFilter, (int)TextureFilter.Linear);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.MipFilter, (int)TextureFilter.Linear);





         // Lighting
         Color globalAmbient = Color.FromArgb(255, 80, 80, 80);

         Vector3 lightDirection = new Vector3(-0.5f, -1.0f, -0.5f);
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

         // Reset world matrix
         BRenderDevice.getDevice().SetTransform(TransformType.World, Matrix.Identity);


         GrannyManager2.resetStatsInfoVerticalPosition();


         // Apply fog (disabled)
         GrannyManager2.setFogParams(false,
                  System.Drawing.Color.White, 1.0f, 0.0f, 5000.0f,
                  System.Drawing.Color.White, 1.0f, 0.0f, 5000.0f);



         RenderModeMask renderMode = RenderModeMask.Markers;


         if (m_bShowModel)
            renderMode |= RenderModeMask.Model;

         if (m_bShowPlayerColor)
            renderMode |= RenderModeMask.PlayerColor;

         if (m_bShowSkeleton)
            renderMode |= RenderModeMask.Skeleton;

         if (m_bShowBoneAxis)
            renderMode |= RenderModeMask.BoneAxis;

         if (m_bShowBoneNames)
            renderMode |= RenderModeMask.BoneNames;

         if (m_bShowBoundingBox)
            renderMode |= RenderModeMask.BoundingBox;

         if (m_bShowMeshBoxes)
            renderMode |= RenderModeMask.MeshBoxes;

         if (m_bShowStatsInfo)
            renderMode |= RenderModeMask.StatsInfo;

         // Render object
         //if ((VisualTreeView != null) && (VisualTreeView.SelectedNode != null))
         //{
         if (mVisualFile != null)
         {
            BRenderDevice.getDevice().SetTransform(TransformType.World, m_worldMatrix);
            mVisualFile.render(renderMode);
         }
         //}


         // Render grid
         if (m_bShowGrid)
            m_grid.renderGrid();


         // Render axis
         renderAxis();

         if (mHelperVisual != null)
         {
            mHelperVisual.Render();
         }

         BRenderDevice.endScene();
         BRenderDevice.present();
      }

      //override these functions to ensure your app gets the proper processing.
      override public void input()
      {
         base.input();

         //If you want one of these settings add it to the combobox.
         if (UIManager.GetAsyncKeyStateB(Key.D1)) setRenderMode(GrannyInstance.eVisualEditorRenderMode.cRenderFull);
         if (UIManager.GetAsyncKeyStateB(Key.D2)) setRenderMode(GrannyInstance.eVisualEditorRenderMode.cRenderFullWireframe);
         if (UIManager.GetAsyncKeyStateB(Key.D3)) setRenderMode(GrannyInstance.eVisualEditorRenderMode.cRenderFlat);
         if (UIManager.GetAsyncKeyStateB(Key.D4)) setRenderMode(GrannyInstance.eVisualEditorRenderMode.cRenderFlatWireframe);
         if (UIManager.GetAsyncKeyStateB(Key.D5)) setRenderMode(GrannyInstance.eVisualEditorRenderMode.cRenderFullOverlay);

         mCameraManager.CameraMovement();

         UIManager.WheelDelta = 0;
      }
      public void setRenderMode(GrannyInstance.eVisualEditorRenderMode mode)
      {
         mRenderMode = mode;
      }
      public void renderAxis()
      {
         // Render axis
         if (s_renderAxisPrim == null)
            return;

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);


         Matrix w = BRenderDevice.getDevice().Transform.World;
         Matrix p = BRenderDevice.getDevice().Transform.Projection;
         Viewport vpp = BRenderDevice.getDevice().Viewport;

         Viewport vp = new Viewport();
         int viewPortSize = (int)(Math.Min(vpp.Width, vpp.Height) * 0.1f);
         vp.X = 0;
         vp.Y = vpp.Height - viewPortSize;
         vp.Width = viewPortSize;
         vp.Height = viewPortSize;
         BRenderDevice.getDevice().Viewport = vp;

         Vector3 tdir = mCameraManager.mEye - mCameraManager.mLookAt;
         tdir = BMathLib.Normalize(tdir);

         BRenderDevice.getDevice().Transform.World = Matrix.Translation(mCameraManager.mEye - (tdir * 3.0f));
         BRenderDevice.getDevice().Transform.Projection = Matrix.PerspectiveFovLH(Geometry.DegreeToRadian(45), 1.0f /*(float)BRenderDevice.getWidth() / (float)BRenderDevice.getHeight()*/, 0.5f, 6.0f);

         s_renderAxisPrim.render();

         BRenderDevice.getDevice().Transform.World = w;
         BRenderDevice.getDevice().Transform.Projection = p;
         BRenderDevice.getDevice().Viewport = vpp;


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
      }

      private void resetCamera(BBoundingBox modelBox)
      {
         Vector3 modelCenter = modelBox.getCenter();
         float maxSize = modelBox.getMaxSize();

         float cameraDistanceXZ = maxSize * 2.0f;
         float cameraDistanceY = maxSize * 1.0f;

         Vector3 camLookAt = modelCenter;
         Vector3 camEye = modelCenter + new Vector3(cameraDistanceXZ, cameraDistanceY, cameraDistanceXZ);

         mCameraTransition.triggerTransition(mCameraManager.mEye, mCameraManager.mLookAt, camEye, camLookAt);
      }

      #endregion

      public override void save_file()
      {
         SaveProtoXml();
      }
      public override void save_file_as()
      {
         CoreGlobals.ShowMessage("No save as option");
      }

      ObjectEditorControl mObjectEditor = null;
      HelperVisual mHelperVisual = null;

      public void PlayerIdSelectedChanged(int index) { }
      public void UnitSelectedChanged(object obj)
      {
         SimUnitXML unit = obj as SimUnitXML;
         if (unit != null)
         {
            UnitSelectedChanged(unit.mName);
         }

      }
      public void UnitSelectedChanged(string name)
      {         
         if (TryGetExistingObject(mProtoObjectsDoc, name, out mUnitXml))
         {
            //DB Nam
            ObjectNameLabel.Text = name;

            //Visual property
            string visual = "";
            XmlNodeList nodes = mUnitXml.SelectNodes("Visual");
            if (nodes.Count == 1)
            {
               visual = nodes[0].InnerText;
               LoadVisual(visual);
            }

            //Type
            string objectClass = "";
            nodes = mUnitXml.SelectNodes("ObjectClass");
            if (nodes.Count == 1)
            {
               objectClass = nodes[0].InnerText;              
            }
                       
            //object loaded already ////////
            foreach (LambdaObject l in mUndoProvider.mUndoTable.Keys)
            {
               if (l.mParentName == name)
               {
                  mObjectEditor.SelectedObject = l;
                  mHelperVisual = GetHelperVisual(objectClass);
                  mSelectedLambdaObject = l;
                  UpdateHelperVisual((LambdaObject)mSelectedLambdaObject);
                  UpdateUndoUI(mSelectedLambdaObject);
                  return;
               }
            }
            //else load object////////
            LambdaObject selectedObject = XMLDocHelper.BuildDataBinding(mProtoObjectsDoc, mUnitXml, name, objectClass, mObjectEditor);
            mObjectEditor.SelectedObject = selectedObject;
            mHelperVisual = GetHelperVisual(objectClass);
            mSelectedLambdaObject = selectedObject;
            UpdateHelperVisual((LambdaObject)mSelectedLambdaObject);
            UpdateUndoUI(mSelectedLambdaObject);
         }
         else
         {
            mHelperVisual = null;
            mObjectEditor.SelectedObject = null;
         }     
      }
      public void RefreshUI()
      {
         if (mSelectedLambdaObject != null)
         {
            mObjectEditor.SelectedObject = mSelectedLambdaObject;
            UpdateHelperVisual((LambdaObject)mSelectedLambdaObject);
            UpdateUndoUI(mSelectedLambdaObject);
         }
      }

      HelperVisual GetHelperVisual(string type)
      {
         if (type == "Squad" || type == "Unit" || type == "Building" || type == "Object")
         {
            return new SquadVisual(mCameraManager);
         }

         return null;
      }
      public void UpdateHelperVisual(LambdaObject o)
      {
         if (mHelperVisual == null)
            return;
         foreach (INamedTypedProperty prop in o.mProperties)
         {
            float res;
            if (float.TryParse(prop.GetValue().ToString(), out res))
            {
               if(mHelperVisual != null)
               {
                  PropertyInfo info = mHelperVisual.GetType().GetProperty(prop.GetName());
                  if (info != null)
                  {
                     info.SetValue(mHelperVisual, res, null);
                  }
               }
            }
         }
      }


      object mLastShownUndoInfo = null;
      object mSelectedLambdaObject = null;
      UndoProvider mUndoProvider = new UndoProvider();

      public class UndoData
      {
         public UndoData(object selectedObject, object newValue, object lastValue, HighLevelProperty prop)
         {
            mSelectedObject = selectedObject;
            mLastValue = lastValue;
            mNewValue = newValue;
            mProp = prop;
         }
         public object mSelectedObject;
         public object mLastValue;
         public object mNewValue;
         //public INamedTypedProperty mProp;
         public HighLevelProperty mProp; //Volitile?

         public override string ToString()
         {
            return mProp.Name + ":  " + mLastValue.ToString() + " --> " + mNewValue.ToString();
            //return base.ToString();
         }

         public void Revert()
         {
            //mProp.SetValue( mLastValue );
            mProp.DataValue = mLastValue;  
         }
      }

      public class UndoProvider
      {
         public Dictionary<object, List<UndoData>> mUndoTable = new Dictionary<object, List<UndoData>>();
         enum eUndoModel
         {
            cTransactional,
            cIndependant
         };
         eUndoModel mUndoModel = eUndoModel.cIndependant;

         public void ProcessChange(object selectedObject, HighLevelProperty prop)
         {
            if (mUndoTable.ContainsKey(selectedObject) == false)
            {
               mUndoTable[selectedObject] = new List<UndoData>();
            }
            List<UndoData> undoData = mUndoTable[selectedObject];
            if (undoData.Count > 0)
            {
               UndoData undoEntry = null;
               if (mUndoModel == eUndoModel.cTransactional)
               {
                  UndoData top = undoData[undoData.Count - 1];
                  if (top.mProp.Name == prop.Name)
                  {
                     undoEntry = top;
                  }
               }
               else if (mUndoModel == eUndoModel.cIndependant)
               {
                  undoEntry = undoData.Find(delegate(UndoData compareTo) { return prop.Name == compareTo.mProp.Name; });
               }
               if (undoEntry == null) //create entry
               {
                  undoData.Add(new UndoData(selectedObject, prop.DataValue, prop.LastValue, prop));
               }
               else //update entry
               {
                  undoEntry.mNewValue = prop.DataValue;
               }
            }
            else
            {
               undoData.Add(new UndoData(selectedObject, prop.DataValue, prop.LastValue, prop));
            }
         }
      }

      void UpdateUndoUI(object selectedObject)
      {
         bool hasSelectedItem = false;
         foreach (object o in mUndoProvider.mUndoTable.Keys)
         {
            if (this.ChangedObjectsListBox.Items.Contains(o) == false)
            {
               this.ChangedObjectsListBox.Items.Add(o);
            }
            if (o == selectedObject)
               hasSelectedItem = true;
         }
         if (hasSelectedItem == true)
         {
            this.ChangedObjectsListBox.SelectedItem = selectedObject;
         }
         else
         {
            this.ChangedObjectsListBox.SelectedItem = null;
         }
         //if (mLastSelectedObject != mSelectedLambdaObject)
         //{
         //   this.ChangesListBox.Items.Clear();
         //}
         this.ChangesListBox.Items.Clear();

         if (mUndoProvider.mUndoTable.ContainsKey(selectedObject))
         {
            foreach (object o in mUndoProvider.mUndoTable[selectedObject])
            {
               if (this.ChangesListBox.Items.Contains(o) == false)
               {
                  this.ChangesListBox.Items.Add(o);
               }
            }
         }
         ChangesListBox.Refresh();

         mLastShownUndoInfo = selectedObject;

      }


      private void ChangedObjectsListBox_SelectedIndexChanged(object sender, EventArgs e)
      {

      }  

      
      void ChangedObjectsListBox_Click(object sender, EventArgs e)
      {
         if (ChangedObjectsListBox.SelectedItem != null)
         {
            if (mUndoProvider.mUndoTable.ContainsKey(ChangedObjectsListBox.SelectedItem))
            {
               UpdateUndoUI(ChangedObjectsListBox.SelectedItem);
            }
         }         
         //ChangedObjectsListBox.SelectedItem;
      }
      bool mPausePropertyUpdate = false; //not great
      void betterPropertyGrid1_AnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         if(mPausePropertyUpdate == false)
         {         
            //mSelectedLambdaObject = selectedObject;
            //if (mSelectedLambdaObject != selectedObject)
            //{
            //   throw new System.Exception("not supposed to happen");
            //}
            
            //Undo portion////////////////////////////////////////////
            mUndoProvider.ProcessChange(selectedObject, prop);

            UpdateUndoUI(mSelectedLambdaObject);
            ///////////////////////////////////////////////


            //Reflected bind to observer class
            float res;
            if (float.TryParse(prop.PresentationValue.ToString(), out res))
            {
               if (mHelperVisual != null)
               {
                  PropertyInfo info = mHelperVisual.GetType().GetProperty(prop.Name);
                  if (info != null)
                  {
                     info.SetValue(mHelperVisual, res, null);
                  }
               }
            }
         }
         
      }



      XmlDocument mProtoObjectsDoc = null;
      XmlNode mUnitXml = null;
      public bool mbTestMode = false;
      public bool mbDBIDTool = true;
      string mFileName = "";
      Dictionary<string, XmlNode> mTemplates = new Dictionary<string, XmlNode>(); 

      public void LoadProtoXml()
      {
         string fileName = CoreGlobals.getWorkPaths().GetProtoObjectFile();
         if(mbTestMode)
         {
            fileName = fileName.Replace("objects.xml", "TestDataObjects.xml");
         }
         mFileName = fileName;

         mProtoObjectsDoc = new XmlDocument();
         mProtoObjectsDoc.Load(fileName);

         //Load our templates
         mTemplates.Clear();
         XmlNodeList nodes = mProtoObjectsDoc.SelectNodes("//Object[contains(@name,'template_')]");
         foreach (XmlNode n in nodes)
         {
            mTemplates[n.Attributes["name"].Value.ToString()] = n;
         }

      }

      public void SaveProtoXml()
      {
         string fileName = CoreGlobals.getWorkPaths().GetProtoObjectFile();
         if (mbTestMode)
         {
            fileName = fileName.Replace("objects.xml", "TestDataObjectsOut.xml");
         }   
         else
         {
            fileName = mFileName;
         }

         if (File.Exists(fileName) == false)
         {
            CoreGlobals.ShowMessage("File Missing: " + fileName);
            return;
         }
         if ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
         {
            CoreGlobals.ShowMessage("Please check out: " + fileName);
            return;
         }

         XmlWriterSettings settings = new XmlWriterSettings();
         settings.IndentChars = "   ";
         settings.CloseOutput = true;
         settings.Indent = true;
         settings.OmitXmlDeclaration = true;
         XmlWriter w = XmlWriter.Create(fileName, settings);
         mProtoObjectsDoc.Save(w);
         w.Close();

         if (mbTestMode == false && mbDBIDTool == true)
         {
            DBIDHelperTool.Run();
         }

         //XMLDocHelper.FixSpacing(fileName);  //no longer needed:  XmlWriter takes care of it

         XMBProcessor.CreateXMB(fileName, false);
      }

      public bool TryGetExistingObject(XmlDocument protoObjectsDoc, string objectName, out XmlNode unit)
      {
         unit = null;
         XmlNodeList nodes = mProtoObjectsDoc.SelectNodes("//Object[@name='" + objectName + "']");
         if (nodes.Count == 1)
         {
            unit = nodes[0];
            return true;
         }
         else
         {
            return false;
         }
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         SaveProtoXml();
      }

      private void NewButton_Click(object sender, EventArgs e)
      {
         PickOneOption p1 = new PickOneOption();
         {
            p1.Options = mTemplates.Keys;
         }
         if (p1.ShowDialog() == DialogResult.OK)
         {
            string selection = p1.SelectedOption;
         }
      }

      private void UndoButton_Click(object sender, EventArgs e)
      {

      }

      private void RedoButton_Click(object sender, EventArgs e)
      {

      }

      private void ResetSelectedButton_Click(object sender, EventArgs e)
      {
         UndoData d = this.ChangesListBox.SelectedItem as UndoData;
         if (d != null)
         {
            object lastUndoPage = mLastShownUndoInfo;
            if (lastUndoPage != mSelectedLambdaObject)
            {
               mPausePropertyUpdate = true;
            }

            d.Revert();

            ChangedObjectsListBox.Items.Remove(d);
            mUndoProvider.mUndoTable[d.mSelectedObject].Remove(d);

            if (lastUndoPage != mSelectedLambdaObject)
            {
               UpdateUndoUI(lastUndoPage);
            }
            else
            {
               RefreshUI();
            }

            mPausePropertyUpdate = false;
         }
      }

 
   }


   public class XmlEnumeration
   {
      List<string> mValues = new List<string>();
      public XmlEnumeration(XmlDocument doc, string xpath)
      {
         XmlNodeList nodes = doc.SelectNodes(xpath);
         foreach (XmlNode n in nodes)
         {
            if (mValues.Contains(n.InnerText) == false)
            {
               mValues.Add(n.InnerText);
            }
         }      
      }
      public List<string> Values
      {
         get
         {
            return mValues;
         }
      }

   }

   interface IHasXmlNode
   {
      XmlNode GetRoot();
   }

   public class XmlComplexElement : LamdaObjectProperty, IHasXmlNode
   {
      XMLValueElement mElement;
      public XmlComplexElement(XMLValueElement element, LambdaObject lobj)
         : base(lobj)
      {
         mElement = element;

      }

      public override bool TryCloneEntry(out object clone)
      {
         clone = null;

         //object clone =          
         //base.TryCloneEntry(out clone);
         object elementClone;
         if (mElement.TryCloneEntry(out elementClone))
         {
            XMLValueElement valueElement = elementClone as XMLValueElement;

            object subClone;
            if (mObject.TryCloneEntry(out subClone))
            {
               XmlComplexElement lprop = new XmlComplexElement(valueElement, (LambdaObject)subClone);
               lprop.MetaData = this.MetaData;

               clone = lprop;
               //todo attach: sub elements

               foreach (INamedTypedProperty p in ((LambdaObject)subClone).GetProperties())
               {
                  IHasXmlNode temp = p as IHasXmlNode;

                  XMLDocHelper.Bind(valueElement.GetRoot(), temp.GetRoot());
                  //  temp.TryBindEntry(mElement);
                  //  
               }
               return true;
            }
         }
         return false;

      }


      #region IHasXmlNode Members

      public XmlNode GetRoot()
      {
         return mElement.GetRoot();
         //throw new Exception("The method or operation is not implemented.");
      }

      #endregion
   }

   public class XMLArray : ObjectArrayProperty//, ICloneTemplate
   {
      XmlNode mOwner = null;
      XmlDocument mDocument = null;
      string mElementName = "";
      string mXpath = "";
      //XmlNode mNode = null;
      bool mValid = false;
      bool mEmpty = false;

      public XMLArray(XmlDocument doc, XmlNode owner, string xpath, string name, string type)
         : base(name, type)
      {
         mDocument = doc;
         mXpath = xpath;
         mElementName = xpath;
         mOwner = owner;

         //XmlNodeList nodes = mOwner.SelectNodes("@" + mXpath);
         //if (nodes.Count > 0)
         //{
         //   mNode = nodes[0];
         //   mElementName = mNode.Name;
         //   mValid = true;
         //}
         //else
         //{
         //   mEmpty = true;
         //}
         //mPropertyList.mCloneTemplate = this;

         this.MetaData["this"] = this;

      }

      public bool TryCloneEntry(out object clone)
      {
         clone = null;
         return false;

         //XmlNodeList nodes = mOwner.SelectNodes(mXpath);
         //if (nodes.Count > 0)
         //{
         //   //mNode = nodes[0];
         //   //mElementName = mNode.Name;
         //   //mValid = true;
         //}
         //else
         //{
         //   //mEmpty = true;
         //}
      }
      public bool TryAddChild(object child)
      {
         XMLValueElement ve = child as XMLValueElement;
         if (ve != null)
         {
            //could make this a method.
            ve.mOwner = this.mOwner;
            mOwner.AppendChild(ve.mNode );

            //mArray.Add(ve);
            //this.SetValue(this.GetValue);

            return true;
         }


         return false;
      }

      #region ICollectionObserver Members

      override public void Added(object obj)
      {
         IHasXmlNode newChild = obj as IHasXmlNode;
         if (newChild != null)
         {
            XMLDocHelper.Bind(mOwner, newChild.GetRoot());
            return;
         }


         LamdaObjectProperty lobjp = obj as LamdaObjectProperty;
         if (lobjp != null)
         {
            LambdaObject lobj = lobjp.GetValue() as LambdaObject;
            if(lobj != null && lobj.GetProperties().Count > 0)
            {
               IEnumerator<INamedTypedProperty> it = lobj.GetProperties().GetEnumerator();
               if (it.MoveNext() == false)
                  return;

               newChild = it.Current as IHasXmlNode;
               if (newChild != null)
               {
                  XMLDocHelper.Bind(mOwner, newChild.GetRoot());
               }

               //XMLValueElement ve = it.Current as XMLValueElement;
               //if (ve != null)
               //{
               //   //could make this a method.
               //   ve.mOwner = this.mOwner;
               //   XmlNodeList existingNodes = mOwner.SelectNodes(ve.GetName());
               //   if (existingNodes.Count == 0)
               //   {
               //      mOwner.AppendChild(ve.mNode);  //Insert after would be better.
               //   }
               //   else
               //   {
               //      mOwner.InsertAfter(ve.mNode, existingNodes.Item(existingNodes.Count - 1));
               //   }
               //   //return;
               //   XmlNodeList asdf = mOwner.SelectNodes("PickPriority");
               //   int i = asdf.Count;
               //}

            }
         }

      }

      override public void Removed(object obj)
      {         
         IHasXmlNode newChild = obj as IHasXmlNode;
         if (newChild != null)
         {
            XMLDocHelper.UnBind(mOwner, newChild.GetRoot());
            return;
         }


         LamdaObjectProperty lobjp = obj as LamdaObjectProperty;
         if (lobjp != null)
         {
            LambdaObject lobj = lobjp.GetValue() as LambdaObject;
            if (lobj != null && lobj.GetProperties().Count > 0)
            {
               IEnumerator<INamedTypedProperty> it = lobj.GetProperties().GetEnumerator();
               if (it.MoveNext() == false)
                  return;

               newChild = it.Current as IHasXmlNode;
               if (newChild != null)
               {
                  XMLDocHelper.UnBind(mOwner, newChild.GetRoot());
               }
            }
         }
      }

      #endregion


   }

   public class XMLDocHelper
   {
      static public void Bind(XmlNode parent, XmlNode child)
      {

         if (child is XmlAttribute)
         {
            parent.Attributes.Append((XmlAttribute)child); 
         }
         else if (child is XmlText)
         {
            parent.AppendChild(child); 
         }
         else if (child is XmlElement)
         {
            XmlNodeList existingNodes = parent.SelectNodes(child.Name);

            if (existingNodes.Count == 0)
            {
               parent.AppendChild(child);  //Insert after would be better.
            }
            else
            {
               parent.InsertAfter(child, existingNodes.Item(existingNodes.Count - 1));
            }
         }
      }
      static public void UnBind(XmlNode parent, XmlNode child)
      {

         if (child is XmlAttribute)
         {
            parent.Attributes.Remove((XmlAttribute)child);
         }
         else if (child is XmlText)
         {
            parent.RemoveChild(child);
         }
         else if (child is XmlElement)
         {
            parent.RemoveChild(child);  //Insert after would be better.
         }
      }


      //Replaces 2 space indenation with 3 space indentation
      static public void FixSpacing(string filename)
      {
         string[] lines = File.ReadAllLines(filename);
         List<string> output = new List<string>();

         foreach (string s in lines)
         {
            int whitespaceCount = 0;
            while (s[whitespaceCount] == ' ')
            {
               whitespaceCount++;
            }
            string outline = s.TrimStart(' ');
            for (int i = 0; i < whitespaceCount / 2; i++)
            {
               outline = outline.Insert(0, "   ");
            }
            output.Add(outline);
         }
         File.WriteAllLines(filename, output.ToArray());
      }


      static public LambdaObject BuildDataBinding(XmlDocument xmlDocument, XmlNode parentNode, string name, string type, ObjectEditorControl objEditorControl)
      {
         return BuildDataBinding(xmlDocument, parentNode, null, name, type, objEditorControl);
      }

      static public LambdaObject BuildDataBinding(XmlDocument xmlDocument, XmlNode parentNode, XmlNode node, string name, string type, ObjectEditorControl objEditorControl)
      {
         LambdaObject selectedObject = null;
         Dictionary<string, Dictionary<string, object>> data = objEditorControl.GetMetadataForType(type);
         if (data != null)
         {
            Dictionary<string, Dictionary<string, object>>.Enumerator it = data.GetEnumerator();
            List<INamedTypedProperty> props = new List<INamedTypedProperty>();
            while (it.MoveNext())
            {
               string elementName = it.Current.Key;
               Dictionary<string, object> metaData = it.Current.Value;
               INamedTypedProperty prop;

               prop = GetXMLProperty(xmlDocument, parentNode, type, node, elementName, metaData, objEditorControl);

               props.Add(prop);
            }


            selectedObject = new LambdaObject(name, type, props.ToArray());
         }
         else
         {

         }
         return selectedObject;

      }

      static public LambdaObject BuildDataBinding2(XmlDocument xmlDocument, XmlNode parentNode, XmlNode node, string name, string type, string elementName, ObjectEditorControl objEditorControl)
      {
         LambdaObject selectedObject = null;
         Dictionary<string, Dictionary<string, object>> data = objEditorControl.GetMetadataForType(type);
         if (data != null)
         {
            //Dictionary<string, Dictionary<string, object>>.Enumerator it = data.GetEnumerator();
            //while (it.MoveNext())
            //{
            //   string elementName = it.Current.Key;
            //   Dictionary<string, object> metaData = it.Current.Value;

            INamedTypedProperty prop;

            //   prop = GetXMLProperty(parentNode, type, node, elementName, metaData);

            //   props.Add(prop);
            //}

            List<INamedTypedProperty> props = new List<INamedTypedProperty>();
            Dictionary<string, object> metaData;
            if (data.TryGetValue(elementName, out metaData))
            {
               prop = GetXMLProperty(xmlDocument, parentNode, type, node, elementName, metaData, objEditorControl);
               props.Add(prop);

               selectedObject = new LambdaObject(name, type, props.ToArray());
            }
         }
         else
         {

         }
         return selectedObject;

      }

      static public INamedTypedProperty GetXMLProperty(XmlDocument xmlDocument, XmlNode parentNode, string parentType, XmlNode node, string elementName, Dictionary<string, object> metaData, ObjectEditorControl objEditorControl)
      {
         INamedTypedProperty prop;

         object behavior;
         object complextype;
         prop = null;
        
         if (node == null && metaData.TryGetValue("Array", out complextype))
         {
            XmlNodeList nlist = parentNode.SelectNodes(elementName);

            XMLArray objarray = new XMLArray(xmlDocument, parentNode, elementName, elementName, elementName);
            //ObjectArrayProperty objarray = new ObjectArrayProperty(elementName, elementName);
            objarray.mName = elementName + "[]";

            foreach (XmlNode arrayElement in nlist)
            {
               INamedTypedProperty subprop = null;

               if (complextype.ToString() == "Inline")
               {
                  //   subprop = GetXMLProperty(parentNode, arrayElement, elementName, metaData);
                  //   //GetXMLProperty
                  //   Dictionary<string, object>.Enumerator it1 = metaData.GetEnumerator();
                  //   while (it1.MoveNext())
                  //   {
                  //      if(it1.Current.Key != "Array")
                  //      {
                  //         subprop.MetaData.Add(it1.Current.Key, it1.Current.Value);
                  //      }
                  //   }
                  //   //subprop.MetaData = metaData;

                  LamdaObjectProperty p = new LamdaObjectProperty(BuildDataBinding2(xmlDocument, parentNode, arrayElement, parentType, parentType, elementName, objEditorControl));

                  subprop = p;
                  objarray.mArray.Add(subprop);

                  objarray.MetaData.Add("ArrayType", parentType);

               }
               else
               {

                  //LamdaObjectProperty p = new LamdaObjectProperty(BuildDataBinding(arrayElement, null, elementName, elementName));
                  //subprop = p;


                  XMLValueElement n = new XMLValueElement(xmlDocument, parentNode, arrayElement, elementName);
                  XmlComplexElement p = new XmlComplexElement(n, XMLDocHelper.BuildDataBinding(xmlDocument, arrayElement, null, elementName, elementName, objEditorControl));
                  subprop = p;




                  objarray.mArray.Add(subprop);
               }
            }
            //pass metadata data down...
            Dictionary<string, object>.Enumerator it2 = metaData.GetEnumerator();

            //while (it2.MoveNext())
            //{
            //   objarray.MetaData.Add(it2.Current.Key, it2.Current.Value);
            //}
            //Dictionary<string, Dictionary<string, object>>.Enumerator it2 = metaData.GetEnumerator();
            //while (it2.MoveNext())
            //{
            //   objarray.MetaData.Add(it2.Current.Key, it2.Current.Value);
            //}

            objarray.MetaData.Add("BaseType", "Array");

            if (metaData.TryGetValue("ComplexType", out complextype))
            {
               objarray.MetaData.Add("ArrayType", complextype.ToString());
            }
            prop = objarray;

            //props.Add(objarray);

         }
         else if (metaData.TryGetValue("ComplexType", out complextype))
         {
            //Recursive build

            XmlNode ele = parentNode.SelectSingleNode(elementName);
            if (ele != null)
            {
               XMLValueElement n = new XMLValueElement(xmlDocument, parentNode, ele, elementName);
               //LamdaObjectProperty p = new LamdaObjectProperty(BuildDataBinding(n, elementName, complextype.ToString()));
               XmlComplexElement p = new XmlComplexElement(n, BuildDataBinding(xmlDocument, ele, elementName, complextype.ToString(), objEditorControl));
               prop = p;
            }

            //props.Add(prop);
         }
         else if (metaData.TryGetValue("XMLBehavior", out behavior))
         {
            if (behavior.ToString() == "Flags")
            {
               XMLFLags t = new XMLFLags(xmlDocument, parentNode, node, elementName);
               //props.Add(t);
               prop = t;
            }
            else if (behavior.ToString() == "Attribute")
            {
               XMLValueAttribute a = new XMLValueAttribute(xmlDocument, parentNode, node, elementName);
               //props.Add(a);
               prop = a;
            }
            else if (behavior.ToString() == "Text")
            {
               XMLValueText a = new XMLValueText(xmlDocument, parentNode, node, elementName);
               //props.Add(a);
               prop = a;
            }
            else //default
            {
               XMLValueElement n = new XMLValueElement(xmlDocument, parentNode, node, elementName);
               //props.Add(n);
               prop = n;
            }

            //props.Add(prop);
         }
         else
         {
            XMLValueElement n = new XMLValueElement(xmlDocument, parentNode, node, elementName);
            prop = n;
            //props.Add(n);
         }
         return prop;
      }

   }

   public class XMLValueAttribute : INamedTypedProperty, IEnumProvider, ICloneTemplate, IHasXmlNode 
   {
      XmlNode mOwner = null;
      XmlDocument mDocument = null;
      string mElementName = "";
      string mXpath = "";
      XmlNode mNode = null;
      bool mValid = false;
      bool mEmpty = false;
      bool mCreated = false;

      public XMLValueAttribute(XmlDocument doc, XmlNode owner, XmlNode node, string xpath)
      {
         mDocument = doc;
         mXpath = xpath ;
         mElementName = xpath;
         mOwner = owner;

         if (node != null)
         {
            mNode = node;
            mElementName = mNode.Name;
            mValid = true;
         }
         else
         {
            XmlNodeList nodes = mOwner.SelectNodes("@" + mXpath);
            if (nodes.Count == 1)
            {
               mNode = nodes[0];
               mElementName = mNode.Name;
               mValid = true;
            }
            else
            {
               mEmpty = true;
            }
         }

      }

      public bool IsValid()
      {
         return mValid;
      }


      #region INamedTypedProperty Members

      public string GetName()
      {
         return mElementName;
      }

      public string GetTypeName()
      {
         return mElementName;//????
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

      public object GetValue()
      {
         if (mValid && mNode != null)
         {
            return mNode.Value;
         }
         else
         {
            return "";
         }
      }

      public void SetValue(object val)
      {
         //attach an element if empty
         if (mEmpty)
         {
            mEmpty = false;
            XmlAttribute attr = mDocument.CreateAttribute(mElementName);
            mCreated = true;
            mOwner.Attributes.Append(attr);
            mNode = attr;
            mValid = true;
         }

         if (!mEmpty && mNode != null)
         {
            mNode.Value = val.ToString();
            mEmpty = false;
         }

         if (mCreated && (val.ToString() == "") && (mNode != null)) //special reset case // may need to be made more explicit
         {
            mEmpty = true;
            mValid = false;
            mOwner.Attributes.Remove((XmlAttribute)mNode);
            mNode = null;
         }
      }

      string mEnumPath = "";

      #endregion

      #region IEnumProvider Members
      static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
      public ICollection<string> GetEnumStrings()
      {
         //string path = "//" + mOwner.Name + "//@" + mXpath;
         if (mEnumPath == "")
         {
            mEnumPath = "//" + mOwner.Name + "//@" + mXpath;
         }
         string path = mEnumPath;
         XmlEnumeration enums;
         if (!sEnumerationMap.TryGetValue(path, out enums))
         {
            sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
         }
         return sEnumerationMap[path].Values;
      }

      #endregion

      #region ICloneTemplate Members

      public bool TryCloneEntry(out object clone)
      {
         XmlAttribute ele = mDocument.CreateAttribute(mElementName);
         XMLValueAttribute xvalue = new XMLValueAttribute(mDocument, null, ele, mXpath);
         xvalue.MetaData = this.MetaData;
         xvalue.mValid = false; // no value set yet.
         xvalue.mEnumPath = this.mEnumPath;
         clone = xvalue;
         
         return true;
      }

      #endregion

      #region IHasXmlNode Members

      public XmlNode GetRoot()
      {
         return mNode;
      }

      #endregion
   }

   public class XMLValueElement : INamedTypedProperty, IEnumProvider, ICloneTemplate, IHasXmlNode
   {
      public XmlNode mOwner = null;
      XmlDocument mDocument = null;
      string mElementName = "";
      string mXpath = "";
      public XmlNode mNode = null;
      bool mValid = false;
      bool mEmpty = false;
      bool mCreated = false;

      public XMLValueElement(XmlDocument doc, XmlNode owner, XmlNode node, string xpath)
      {
         mDocument = doc;
         mXpath = xpath;
         mElementName = xpath;
         mOwner = owner;

         if (node != null)
         {
            mNode = node;
            mElementName = mNode.Name;
            mValid = true;
         }
         else
         {
            XmlNodeList nodes = mOwner.SelectNodes(mXpath);
            if (nodes.Count == 1)
            {
               mNode = nodes[0];
               mElementName = mNode.Name;
               mValid = true;
            }
            else
            {
               mEmpty = true;
            }
         }

      }

      public bool IsValid()
      {
         return mValid;
      }


      #region INamedTypedProperty Members

      public string GetName()
      {
         return mElementName;
      }

      public string GetTypeName()
      {
         return mElementName;//????
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

      public object GetValue()
      {
         if (mValid && mNode != null)
         {
            return mNode.InnerText;
         }
         else
         {
            return "";
         } 
      }

      public void SetValue(object val)
      {
         //attach an element if empty
         if (mEmpty)
         {
            mEmpty = false;
            XmlElement ele = mDocument.CreateElement(mElementName);
            mCreated = true;
            mOwner.AppendChild(ele);
            mNode = ele;
            mValid = true;
         }

         //if (mValid && mNode != null)
         if (!mEmpty && mNode != null)
         {
            mNode.InnerText = val.ToString();
            mEmpty = false;
            mValid = true;
         }

         if (mCreated && (val.ToString() == "") && (mNode != null)) //special reset case // may need to be made more explicit
         {
            mEmpty = true;
            mValid = false;
            mOwner.RemoveChild(mNode);
            mNode = null;

         }
      }

      #endregion

      #region IEnumProvider Members
      static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
      public ICollection<string> GetEnumStrings()
      {
         string path = "//" + mXpath;
         XmlEnumeration enums;
         if (!sEnumerationMap.TryGetValue(path, out enums ))
         {
            sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
         }
         return sEnumerationMap[path].Values;
      }

      #endregion

      #region ICloneTemplate Members

      public bool TryCloneEntry(out object clone)
      {
         XmlElement ele = mDocument.CreateElement(mElementName);
         XMLValueElement xvalue = new XMLValueElement(mDocument, null, ele, mXpath);
         xvalue.MetaData = this.MetaData;
         xvalue.mValid = false; // no value set yet.
         clone = xvalue;
         return true;
      }

      //public bool TryAddChild(object child)
      //{         
      //   throw new Exception("The method or operation is not implemented.");
      //}


      #endregion

      #region IHasXmlNode Members

      public XmlNode GetRoot()
      {
         return mNode;
      }

      #endregion
   }

   public class XMLValueText : INamedTypedProperty, IEnumProvider, ICloneTemplate, IHasXmlNode
   {
      XmlNode mOwner = null;
      XmlDocument mDocument = null;
      string mElementName = "";
      string mXpath = "";
      XmlNode mNode = null;
      bool mValid = false;
      bool mEmpty = false;

      public XMLValueText(XmlDocument doc, XmlNode owner, XmlNode node, string xpath)
      {
         mDocument = doc;
         mXpath = xpath;
         mElementName = xpath;
         mOwner = owner;

         if (node != null)
         {
            mNode = node;
            mElementName = mNode.Name;
            mValid = true;
         }
         else //find the node
         {
            XmlNodeList nodes = mOwner.SelectNodes("text()");//mXpath + "\\text()");
            if (nodes.Count == 1)
            {
               mNode = nodes[0];
               mElementName = mNode.Name;
               mValid = true;
            }
            else
            {
               mEmpty = true;
            }
         }
      }

      public bool IsValid()
      {
         return mValid;
      }

      #region INamedTypedProperty Members

      public string GetName()
      {
         return mElementName;
      }

      public string GetTypeName()
      {
         return mElementName;//????
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

      public object GetValue()
      {
         if (mValid && mNode != null)
         {
            return mNode.InnerText;
         }
         else
         {
            return "";
         }
      }

      public void SetValue(object val)
      {
         //attach an element if empty
         if (mEmpty)
         {
            mEmpty = false;
            //XmlElement ele = mDocument.CreateElement(mElementName);
            XmlText text = mDocument.CreateTextNode(val.ToString());
            mOwner.PrependChild(text);
            mNode = text;
            mValid = true;
         }

         if (!mEmpty && mNode != null)
         {
            //mNode.InnerText = val.ToString();
            mNode.Value = val.ToString();
            mEmpty = false;
            mValid = true;

         }

      }

      #endregion

      #region IEnumProvider Members
      static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
      public ICollection<string> GetEnumStrings()
      {
         string path = "text()";//"//" + mXpath + "//text()";
         XmlEnumeration enums;
         if (!sEnumerationMap.TryGetValue(path, out enums))
         {
            sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
         }
         return sEnumerationMap[path].Values;
      }

      #endregion

      #region ICloneTemplate Members

  
      public bool TryCloneEntry(out object clone)
      {
         XmlText ele = mDocument.CreateTextNode(mNode.Value);
         XMLValueText xvalue = new XMLValueText(mDocument, null, ele, mXpath);
         xvalue.MetaData = this.MetaData;
         xvalue.mValid = false; // no value set yet.
         clone = xvalue;
         return true;
      }      

      #endregion

      #region IHasXmlNode Members

      public XmlNode GetRoot()
      {
         return mNode;
      }

      #endregion
   }

   public class XMLFLags : INamedTypedProperty, IEnumProvider
   {
      XmlNode mOwner = null;
      XmlDocument mDocument = null;
      string mElementName = "";
      string mXpath = "";
      List<string> mFlags = new List<string>();
      //List<string> mAllOptions = null;

      public XMLFLags(XmlDocument doc, XmlNode parentNode, XmlNode node, string elementName)//, List<string> options)
      {
         mDocument = doc;
         mElementName = elementName;
         mXpath = mElementName;
         mOwner = parentNode;

         if (node != null)
         {
            //Yes, this is pretty dumb, just ad one node if flags is put in array...
            mFlags.Add(node.InnerText);
         }
         else
         {
            XmlNodeList nodes = mOwner.SelectNodes(mXpath);
            foreach (XmlNode n in nodes)
            {
               //if (restrictOptions.Contains(n.InnerText))
               {
                  mFlags.Add(n.InnerText);
               }
            }
         }
         this.MetaData["NotCommaSeparatedString"] = true;
      }
      public void AddFlag(string flag)
      {
         mFlags.Add(flag);
         UpdateChangesToXml();
      }
      public void RemoveFlag(string flag)
      {
         mFlags.Remove(flag);
         UpdateChangesToXml();
      }

      //public ICollection<string> GetRawFlags()
      //{
      //   return mFlags;
      //}

      public void UpdateChangesToXml()
      {
         XmlNodeList nodes = mOwner.SelectNodes(mXpath);

         List<string> restrictOptions = null;

         if(MetaData.ContainsKey("SimpleEnumeration"))
         {
            string theString = MetaData["SimpleEnumeration"] as string;
            if (theString != null)
            {
               MetaData["SimpleEnumeration"] = new List<string>(theString.Split(','));            
            }
            restrictOptions = (List<string>)MetaData["SimpleEnumeration"];
         }
              
         foreach (XmlNode node in nodes)
         {
            if (restrictOptions != null && restrictOptions.Contains(node.InnerText))
            {
               mOwner.RemoveChild(node);
            }
            else if (restrictOptions == null)
            {
               mOwner.RemoveChild(node);
            }
            else
            {
               node.ToString();
            }
         }
         foreach (string s in mFlags)
         {
            XmlElement node = mDocument.CreateElement(mElementName);
            node.InnerText = s;
            mOwner.AppendChild(node);
         }
      }

      #region INamedTypedProperty Members

      public string GetName()
      {
         return mElementName;
      }

      public string GetTypeName()
      {
         return mElementName;
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

      //Restrict options:  this allows us to ignore flags, and not stomp them even though they are not editable.
      public object GetValue()
      {
         List<string> flags = new List<string>();

         List<string> restrictOptions = null;

         if (MetaData.ContainsKey("SimpleEnumeration"))
         {
            string theString = MetaData["SimpleEnumeration"] as string;
            if (theString != null)
            {
               MetaData["SimpleEnumeration"] = new List<string>(theString.Split(','));
            }
            restrictOptions = (List<string>)MetaData["SimpleEnumeration"];
         }

         if (restrictOptions != null)
         {
            foreach (string s in mFlags)
            {
               if (restrictOptions.Contains(s))
               {
                  flags.Add(s);
               }
            }
         }
         else
         {
            flags.AddRange(mFlags);
            return flags;
         }
         return flags;
      }
    
      public void SetValue(object val)
      {
         
         mFlags.Clear();
         if (val is ICollection<string>)
         {
            mFlags.AddRange((ICollection<string>)val);
         }
         if (val is string[])
         {
            mFlags.AddRange((string[])val);
         }
         UpdateChangesToXml();
      }

      #endregion

      #region IEnumProvider Members

      static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
      public ICollection<string> GetEnumStrings()
      {
         string path = "//" + mXpath;
         XmlEnumeration enums;
         if (!sEnumerationMap.TryGetValue(path, out enums))
         {
            sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
         }
    

         List<string> restrictOptions = null;       
         if (MetaData.ContainsKey("SimpleEnumeration"))
         {
            string theString = MetaData["SimpleEnumeration"] as string;
            if (theString != null)
            {
               MetaData["SimpleEnumeration"] = new List<string>(theString.Split(','));
            }
            restrictOptions = (List<string>)MetaData["SimpleEnumeration"];
            if (restrictOptions != null)
            {
               return new List<string>();// restrictOptions;
              

            }
         }
           

         return sEnumerationMap[path].Values;
      }

      #endregion
   }

   public class HelperVisual
   {
      public virtual void Render() { }

   }

   public class SquadVisual : HelperVisual
   {
      BCameraManager mCamera;
      public SquadVisual(BCameraManager camera)
      {
         mbChanged = true;
         mCamera = camera;
      }
      ~SquadVisual()
      {
         clearVisuals();
      }
         
      bool mbChanged = false;
      public void initVisuals()
      {
         mAxisCircle = new BRenderDebug2DCircle(mPickRadius, Color.Red.ToArgb());
         mDebugCube = new BRenderDebugCube(mCubeMin, mCubeMax, Color.Yellow.ToArgb(), false);

      }
      public void clearVisuals()
      {
         if (mAxisCircle != null)
         {
            mAxisCircle.destroy();
            mAxisCircle = null;
         }
         if (mDebugCube != null)
         {
            mDebugCube.destroy();
            mDebugCube = null;
         }
      }

      BRenderDebug2DCircle mAxisCircle = null;//new BRenderDebug2DCircle(mRadius, mColor);
      BRenderDebugCube mDebugCube = null;
      float mPickRadius = 1;
      public float PickRadius
      {
         set
         {
            mPickRadius = value;
            mbChanged = true;
         }
      }
      float mPickOffset = 0;
      public float PickOffset
      {
         set
         {
            mPickOffset = value;
            mbChanged = true;
         }
      }
      public float ObstructionRadiusX
      {
         set
         {
            mCubeMax.X = (value );
            mCubeMin.X = -(value);
            mbChanged = true;
         }
      }
      public float ObstructionRadiusY
      {
         set
         {
            mCubeMax.Y = (value * 2);
            mCubeMin.Y = 0;
            mbChanged = true;
         }
      }
      public float ObstructionRadiusZ
      {
         set
         {
            mCubeMax.Z = (value );
            mCubeMin.Z = -(value);
            mbChanged = true;
         }
      }

      Vector3 mCubeMin = new Vector3(-1, 0, -1);
      Vector3 mCubeMax = new Vector3(1, 2, 1);

      public override void Render()
      {
         if (mbChanged == true)
         {
            clearVisuals();
            initVisuals();
            mbChanged = false;
         }
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         mDebugCube.render();

         Vector3 dir = mCamera.mEye - mCamera.mLookAt;
         dir.Normalize();
         Matrix world = BMathLib.makeRotateMatrix(BMathLib.unitY, dir);
         BRenderDevice.getDevice().Transform.World = world * Matrix.Translation(0, mPickOffset, 0);///  world;
         
         mAxisCircle.render();
         //base.Render();
      }
      
      
   }

   #region v1 stuff

   // ISubProperty
   // IArray

   //IList
   //interface INestedProperty : INamedTypedProperty, IList
   //{

   //}
   //interface IArrayProperty : INamedTypedProperty
   //{

   //}

   ////Has name and metadata, but holds a list of the subtype
   //public class XMLArray : INamedTypedProperty
   //{

   //   XmlNode mOwner = null;
   //   XmlDocument mDocument = null;
   //   string mElementName = "";
   //   string mXpath = "";
   //   XmlNode mNode = null;
   //   bool mValid = false;
   //   bool mEmpty = false;

   //   public XMLArray(XmlDocument doc, XmlNode owner, string xpath)
   //   {
   //      mDocument = doc;
   //      mXpath = xpath ;
   //      mElementName = xpath;
   //      mOwner = owner;

   //      XmlNodeList nodes = mOwner.SelectNodes("@" + mXpath);
   //      if (nodes.Count == 1)
   //      {
   //         mNode = nodes[0];
   //         mElementName = mNode.Name;
   //         mValid = true;
   //      }
   //      else
   //      {       
   //         mEmpty = true;
   //      }

   //   }

   //   public bool IsValid()
   //   {
   //      return mValid;
   //   }


   //   #region INamedTypedProperty Members

   //   public string GetName()
   //   {
   //      return mElementName;
   //   }

   //   public string GetTypeName()
   //   {
   //      return mElementName;//????
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

   //   public object GetValue()
   //   {
   //      if (mValid && mNode != null)
   //      {
   //         return mNode.Value;
   //      }
   //      else
   //      {
   //         return "";
   //      }
   //   }

   //   public void SetValue(object val)
   //   {
   //      //attach an element if empty
   //      if (mEmpty)
   //      {
   //         mEmpty = false;
   //         XmlAttribute attr = mDocument.CreateAttribute(mElementName);
   //         mOwner.Attributes.Append(attr);
   //         mNode = attr;
   //         mValid = true;
   //      }

   //      if (mValid && mNode != null)
   //      {
   //         mNode.Value = val.ToString();
   //         mEmpty = false;
   //      }
   //   }

   //   #endregion

   //   #region IEnumProvider Members
   //   static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
   //   public ICollection<string> GetEnumStrings()
   //   {
   //      string path = "@" + mXpath;
   //      XmlEnumeration enums;
   //      if (!sEnumerationMap.TryGetValue(path, out enums))
   //      {
   //         sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
   //      }
   //      return sEnumerationMap[path].Values;
   //   }

   //   #endregion

   //}


   //public class XMLValueComplex : INamedTypedProperty
   //{

   //   XmlNode mOwner = null;
   //   XmlDocument mDocument = null;
   //   string mElementName = "";
   //   string mXpath = "";
   //   XmlNode mNode = null;
   //   bool mValid = false;
   //   bool mEmpty = false;

   //   public XMLValueComplex(XmlDocument doc, XmlNode owner, string xpath)
   //   {
   //      mDocument = doc;
   //      mXpath = xpath;
   //      mElementName = xpath;
   //      mOwner = owner;

   //      XmlNodeList nodes = mOwner.SelectNodes("@" + mXpath);
   //      if (nodes.Count == 1)
   //      {
   //         mNode = nodes[0];
   //         mElementName = mNode.Name;
   //         mValid = true;
   //      }
   //      else
   //      {
   //         mEmpty = true;
   //      }

   //   }

   //   public bool IsValid()
   //   {
   //      return mValid;
   //   }


   //   #region INamedTypedProperty Members

   //   public string GetName()
   //   {
   //      return mElementName;
   //   }

   //   public string GetTypeName()
   //   {
   //      return mElementName;//????
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

   //   public object GetValue()
   //   {
   //      if (mValid && mNode != null)
   //      {
   //         return mNode.Value;
   //      }
   //      else
   //      {
   //         return "";
   //      }
   //   }

   //   public void SetValue(object val)
   //   {
   //      //attach an element if empty
   //      if (mEmpty)
   //      {
   //         mEmpty = false;
   //         XmlAttribute attr = mDocument.CreateAttribute(mElementName);
   //         mOwner.Attributes.Append(attr);
   //         mNode = attr;
   //         mValid = true;
   //      }

   //      if (mValid && mNode != null)
   //      {
   //         mNode.Value = val.ToString();
   //         mEmpty = false;
   //      }
   //   }

   //   #endregion

   //   #region IEnumProvider Members
   //   static Dictionary<string, XmlEnumeration> sEnumerationMap = new Dictionary<string, XmlEnumeration>();
   //   public ICollection<string> GetEnumStrings()
   //   {
   //      string path = "@" + mXpath;
   //      XmlEnumeration enums;
   //      if (!sEnumerationMap.TryGetValue(path, out enums))
   //      {
   //         sEnumerationMap[path] = new XmlEnumeration(mDocument, path);
   //      }
   //      return sEnumerationMap[path].Values;
   //   }

   //   #endregion

   //}
   #endregion


   #region CameraTransition

   public class CameraTransition
   {
      public enum eCameraTransitionState
      {
         cCameraTrasitionStateInactive = 0,
         cCameraTrasitionStateActive,
      };

      private eCameraTransitionState mState = eCameraTransitionState.cCameraTrasitionStateInactive;


      private Vector3 mStartCameraEyePos = new Vector3();
      private Vector3 mStartCameraLookAtPos = new Vector3();
      private Vector3 mEndCameraEyePos = new Vector3();
      private Vector3 mEndCameraLookAtPos = new Vector3();

      private float mTime = 0.0f;
      private float mTransitionTime = 0.5f;

      public void triggerTransition(Vector3 startEyePos, Vector3 startLookAtPos, Vector3 endEyePos, Vector3 endLookAtPos)
      {
         mStartCameraEyePos = startEyePos;
         mStartCameraLookAtPos = startLookAtPos;
         mEndCameraEyePos = endEyePos;
         mEndCameraLookAtPos = endLookAtPos;

         mState = eCameraTransitionState.cCameraTrasitionStateActive;
      }

      public void update(ref Vector3 eyePos, ref Vector3 lookAtPos)
      {
         if (mState == eCameraTransitionState.cCameraTrasitionStateActive)
         {
            mTime += 0.033f;

            float factor = mTime / mTransitionTime;

            if (factor < 1.0f)
            {
               eyePos.X = BMathLib.LERP(mStartCameraEyePos.X, mEndCameraEyePos.X, factor);
               eyePos.Y = BMathLib.LERP(mStartCameraEyePos.Y, mEndCameraEyePos.Y, factor);
               eyePos.Z = BMathLib.LERP(mStartCameraEyePos.Z, mEndCameraEyePos.Z, factor);


               lookAtPos.X = BMathLib.LERP(mStartCameraLookAtPos.X, mEndCameraLookAtPos.X, factor);
               lookAtPos.Y = BMathLib.LERP(mStartCameraLookAtPos.Y, mEndCameraLookAtPos.Y, factor);
               lookAtPos.Z = BMathLib.LERP(mStartCameraLookAtPos.Z, mEndCameraLookAtPos.Z, factor);
            }
            else
            {
               mTime = 0.0f;

               eyePos = mEndCameraEyePos;
               lookAtPos = mEndCameraLookAtPos;

               mState = eCameraTransitionState.cCameraTrasitionStateInactive;
            }

         }
      }
   }

   #endregion

   #region Grid
   public struct GridVert
   {
      public Vector3 xyz;
      public uint color;
   }

   public struct GlobalVert
   {
      public Vector3 xyz;
      public Vector3 normal;
      public Vector2 uv;
   }
   public class Grid
   {
      private float cGridIncrement = 1.0f;
      private VertexBuffer mGridVB = null;
      private int mNumGridVerts = 0;

      ~Grid()
      {
         if (mGridVB != null)
         {
            mGridVB.Dispose();
            mGridVB = null;
         }
      }


      public void resetGrid(BBoundingBox modelBox)
      {
         float sizeX = modelBox.max.X - modelBox.min.X;
         float sizeY = modelBox.max.Y - modelBox.min.Y;
         float sizeZ = modelBox.max.Z - modelBox.min.Z;

         float maxSize = float.MinValue;
         maxSize = Math.Max(maxSize, sizeX);
         maxSize = Math.Max(maxSize, sizeY);
         maxSize = Math.Max(maxSize, sizeZ);


         // Release first
         if (mGridVB != null)
         {
            mGridVB.Dispose();
            mGridVB = null;
         }

         int numLines = ((int)((maxSize / cGridIncrement) + 1.0f)) * 2 + 1;
         float gridMax = cGridIncrement * ((numLines - 1) / 2);

         mNumGridVerts = (numLines + numLines) * 2;
         mGridVB = new VertexBuffer(typeof(GridVert), mNumGridVerts, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.Position | VertexFormats.Diffuse, Pool.Managed);

         GridVert[] gridVerts = new GridVert[mNumGridVerts];


         uint color1 = 0xffe0e0e0;
         uint color2 = 0xff606060;

         int index = 0;
         int x, z;
         for (x = 0; x < numLines; x++)
         {
            float xValue = x * cGridIncrement - gridMax;
            uint color;

            if (x == ((numLines - 1) / 2))
            {
               color = color1;
            }
            else
            {
               color = color2;
            }

            gridVerts[index].xyz = new Vector3(xValue, 0.0f, -gridMax);
            gridVerts[index].color = color;
            index++;

            gridVerts[index].xyz = new Vector3(xValue, 0.0f, gridMax);
            gridVerts[index].color = color;
            index++;
         }

         for (z = 0; z < numLines; z++)
         {
            float zValue = z * cGridIncrement - gridMax;
            uint color;

            if (z == ((numLines - 1) / 2))
            {
               color = color1;
            }
            else
            {
               color = color2;
            }

            gridVerts[index].xyz = new Vector3(-gridMax, 0.0f, zValue);
            gridVerts[index].color = color;
            index++;

            gridVerts[index].xyz = new Vector3(gridMax, 0.0f, zValue);
            gridVerts[index].color = color;
            index++;
         }

         //copy verts over
         unsafe
         {
            using (GraphicsStream stream = mGridVB.Lock(0, mNumGridVerts * sizeof(GridVert), LockFlags.None))
            {
               stream.Write(gridVerts);
               mGridVB.Unlock();
            }
         }
      }

      public void renderGrid()
      {
         if (mGridVB == null)
            return;

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);

         BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);

         BRenderDevice.getDevice().SetTransform(TransformType.World, Matrix.Identity);

         // Draw
         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = null;

         BRenderDevice.getDevice().SetStreamSource(0, mGridVB, 0);
         BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, (int)(mNumGridVerts / 2.0f));

         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
   }

   #endregion
}
