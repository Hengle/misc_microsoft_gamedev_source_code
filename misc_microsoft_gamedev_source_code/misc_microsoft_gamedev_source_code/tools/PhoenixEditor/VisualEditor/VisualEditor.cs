using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;




using EditorCore;
using Rendering;
using Terrain;
using ModelSystem;
using SimEditor;
using VisualEditor.PropertyPages;



namespace VisualEditor
{
   enum PropertyPageEnum
   {
      eVisual = 0,
      eModel,
      eComponentAssetModel,
      eComponentAssetParticle,
      eComponentAssetLight,
      eAttachLight,
      eAttachModel,
      eAttachModelRef,
      eAttachParticle,
      eAttachTerrainEffect,
      eAnim,
      eAnimAsset,
      eAnimAssetTagAttack,
      eAnimAssetTagSound,
      eAnimAssetTagParticle,
      eAnimAssetTagTerrainEffect,
      eAnimAssetTagLight,
      eAnimAssetTagCameraShake,
      eAnimAssetTagGroundIK,
      eAnimAssetTagAttachTarget,
      eAnimAssetTagSweetSpot,
      eAnimAssetTagTerrainAlpha,
      eAnimAssetTagRumble,
      eAnimAssetTagUVOffset,
      eAnimAssetTagBuildingDecal,
      eAnimAssetTagKillAndThrow,
      eAnimAssetTagPhysicsImpulse,
      ePoint,
      eLogic,
      eLogicVariation,
      eLogicBuildingCompletion,
      eLogicTech,
      eLogicSquadMode,
      eLogicImpactSize,
      eLogicDestruction,
      eLogicUserCiv,
      eGeneric,
      eNone
   };


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



   public partial class VisualEditorPage : EditorCore.BaseClientPage, IUnitPicker
   {
      private static int s_newVisualCount = 0;
      private static XMLVisualNode s_copyNode = null;            // Node copied when user Ctrl+C
      private static bool s_bVisualQuickViewClicked = false;

      public static BRenderDebugAxis s_renderAxisPrim = null;

      private CameraTransition mCameraTransition = new CameraTransition();
      private BCameraManager mCameraManager = new BCameraManager();
      private BBoundingBox mModelBoundingBox = new BBoundingBox();

      private Grid m_grid = new Grid();

      private List<BRenderGrannyMesh> mMeshes = new List<BRenderGrannyMesh>();
      private Material m_mtrl;

      private string    mVisualDefaultName = null;
      private string    mVisualFileName = null;
      private visual    mVisualFile = null;

      private bool m_bVisualWasSavedThisFrame = false;
      public bool visualWasSavedThisFrame
      {
         get { return this.m_bVisualWasSavedThisFrame; }
         set { this.m_bVisualWasSavedThisFrame = value; }
      }

      public  UndoRedoManager mUndoRedoManager = null;

      private bool m_bShowModel = true;
      private bool m_bShowPlayerColor = false;
      private bool m_bShowSkeleton = false;
      private bool m_bShowBoneAxis = false;
      private bool m_bShowBoneNames = false;
      private bool m_bShowBoundingBox = false;
      private bool m_bShowMeshBoxes = false;
      private bool m_bShowStatsInfo = false;
      private bool m_bShowGrid = true;

      private GrannyInstance.eVisualEditorRenderMode mRenderMode = GrannyInstance.eVisualEditorRenderMode.cRenderFull;

      private bool mIsDocumentModified = false;
      public bool isDocumentModified
      {
         get { return this.mIsDocumentModified; }
         set { this.mIsDocumentModified = value; }
      }

      public visual visualFile
      {
         get { return this.mVisualFile; }
         set { this.mVisualFile = value; }
      }

      public string visualFileName
      {
         get { return this.mVisualFileName; }
         set { this.mVisualFileName = value; }
      }
      
      AnimationControl m_animationControl = new AnimationControl();
      Matrix m_worldMatrix = Matrix.Identity;


      public void setAnimationControlNormalizedTime(float time)
      {
         m_animationControl.setNormalizedTime(time);
      }

      public float getAnimationControlNormalizedTime()
      {
         return (m_animationControl.getNormalizedTime());
      }

      public AnimationControl.AnimControlStateEnum getAnimationControlState()
      {
         return (m_animationControl.getState());
      }

      public void setAnimationControlState(AnimationControl.AnimControlStateEnum state)
      {
         m_animationControl.setState(state);
      }

      // Property pages
      private PropertyPageEnum m_curPageShown = PropertyPageEnum.eNone;

      private VisualPage mVisualPage = new VisualPage();
      private ModelPage mModelPage = new ModelPage();
      private ComponentAssetModelPage mComponentAssetModelPage = new ComponentAssetModelPage();
      private ComponentAssetParticlePage mComponentAssetParticlePage = new ComponentAssetParticlePage();
      private ComponentAssetLightPage mComponentAssetLightPage = new ComponentAssetLightPage();
      private AttachLightPage mAttachLightPage = new AttachLightPage();
      private AttachModelPage mAttachModelPage = new AttachModelPage();
      private AttachModelRefPage mAttachModelRefPage = new AttachModelRefPage();
      private AttachParticlePage mAttachParticlePage = new AttachParticlePage();
      private AttachTerrainEffectPage mAttachTerrainEffectPage = new AttachTerrainEffectPage();
      private AnimPage mAnimPage = new AnimPage();
      private AnimAssetPage mAnimAssetPage = new AnimAssetPage();
      private AnimTagAttackPage mAnimTagAttackPage = new AnimTagAttackPage();
      private AnimTagSoundPage mAnimTagSoundPage = new AnimTagSoundPage();
      private AnimTagParticlePage mAnimTagParticlePage = new AnimTagParticlePage();
      private AnimTagTerrainEffectPage mAnimTagTerrainEffectPage = new AnimTagTerrainEffectPage();
      private AnimTagUVOffsetPage mAnimTagUVOffsetPage = new AnimTagUVOffsetPage();
      private AnimTagLightPage mAnimTagLightPage = new AnimTagLightPage();
      private AnimTagCameraShakePage mAnimTagCameraShakePage = new AnimTagCameraShakePage();
      private AnimTagGroundIKPage mAnimTagGroundIKPage = new AnimTagGroundIKPage();
      private AnimTagAttachTargetPage mAnimTagAttachTargetPage = new AnimTagAttachTargetPage();
      private AnimTagSweetSpotPage mAnimTagSweetSpotPage = new AnimTagSweetSpotPage();
      private AnimTagBuildingDecalPage mAnimTagBuildingDecalPage = new AnimTagBuildingDecalPage();
      private AnimTagTerrainAlpha mAnimTagTerrainAlphaPage = new AnimTagTerrainAlpha();
      private AnimTagRumblePage mAnimTagRumblePage = new AnimTagRumblePage();
      private AnimTagKillAndThrowPage mAnimTagKillAndThrowPage = new AnimTagKillAndThrowPage();
      private AnimTagPhysicsImpulsePage mAnimTagPhysicsImpulsePage = new AnimTagPhysicsImpulsePage();
      private PointPage mPointPage = new PointPage();
      private LogicPage mLogicPage = new LogicPage();
      private LogicNodeVariationPage mLogicVariationPage = new LogicNodeVariationPage();
      private LogicNodeBuildingCompletionPage mLogicBuildingCompletionPage = new LogicNodeBuildingCompletionPage();
      private LogicNodeTechPage mLogicTechPage = new LogicNodeTechPage();
      private LogicNodeSquadModePage mLogicSquadModePage = new LogicNodeSquadModePage();
      private LogicNodeImpactSizePage mLogicImpactSizePage = new LogicNodeImpactSizePage();
      private LogicNodeDestructionPage mLogicDestructionPage = new LogicNodeDestructionPage();
      private LogicNodeUserCivPage mLogicUserCivPage = new LogicNodeUserCivPage();
      private GenericPage mGenericPage = new GenericPage();


      // Tree Context menu
      private ContextMenuStrip mVisualTreeViewContextMenuStrip = null;
      private ToolStripMenuItem mMenuItemAddModel = null;
      private ToolStripMenuItem mMenuItemAddComponent = null;
      private ToolStripMenuItem mMenuItemAddAttach = null;
      private ToolStripMenuItem mMenuItemAddAttachLight = null;
      private ToolStripMenuItem mMenuItemAddAttachModel = null;
      private ToolStripMenuItem mMenuItemAddAttachModelRef = null;
      private ToolStripMenuItem mMenuItemAddAttachParticle = null;
      private ToolStripMenuItem mMenuItemAddAttachTerrainEffect = null;
      private ToolStripMenuItem mMenuItemAddLogic = null;
      private ToolStripMenuItem mMenuItemAddLogicVariation = null;
      private ToolStripMenuItem mMenuItemAddLogicBuildingCompletion = null;
      private ToolStripMenuItem mMenuItemAddLogicTech = null;
      private ToolStripMenuItem mMenuItemAddLogicSquadMode = null;
      private ToolStripMenuItem mMenuItemAddLogicImpactSize = null;
      private ToolStripMenuItem mMenuItemAddLogicDestruction = null;
      private ToolStripMenuItem mMenuItemAddLogicUserCiv = null;
      private ToolStripMenuItem mMenuItemAddLogicData = null;
      private ToolStripMenuItem mMenuItemAddPoint = null;
      private ToolStripMenuItem mMenuItemAddComponentAsset = null;
      private ToolStripMenuItem mMenuItemAddComponentAssetModel = null;
      private ToolStripMenuItem mMenuItemAddComponentAssetParticle = null;
      private ToolStripMenuItem mMenuItemAddComponentAssetLight = null;
      private ToolStripMenuItem mMenuItemAddAnim = null;
      private ToolStripMenuItem mMenuItemAddAnimAsset = null;
      private ToolStripMenuItem mMenuItemAddAnimTag = null;
      private ToolStripMenuItem mMenuItemAddAnimTagAttack = null;
      private ToolStripMenuItem mMenuItemAddAnimTagSound = null;
      private ToolStripMenuItem mMenuItemAddAnimTagParticle = null;
      private ToolStripMenuItem mMenuItemAddAnimTagTerrainEffect = null;
      private ToolStripMenuItem mMenuItemAddAnimTagUVOffset = null;
      private ToolStripMenuItem mMenuItemAddAnimTagLight = null;
      private ToolStripMenuItem mMenuItemAddAnimTagCameraShake = null;
      private ToolStripMenuItem mMenuItemAddAnimTagGroundIK = null;
      private ToolStripMenuItem mMenuItemAddAnimTagAttachTarget = null;
      private ToolStripMenuItem mMenuItemAddAnimTagSweetSpot = null;
      private ToolStripMenuItem mMenuItemAddAnimTagBuildingDecal = null;
      private ToolStripMenuItem mMenuItemAddAnimTagTerrainAlpha = null;
      private ToolStripMenuItem mMenuItemAddAnimTagRumble = null;
      private ToolStripMenuItem mMenuItemAddAnimTagKillAndThrow = null;
      private ToolStripMenuItem mMenuItemAddAnimTagPhysicsImpulse = null;
      private ToolStripMenuItem mMenuItemCut = null;
      private ToolStripMenuItem mMenuItemCopy = null;
      private ToolStripMenuItem mMenuItemPaste = null;
      private ToolStripMenuItem mMenuItemDelete = null;

      // 3D View Context menu
      private ContextMenuStrip mViewportContextMenuStrip = null;
      private ToolStripMenuItem mMenuItemShowModel = null;
      private ToolStripMenuItem mMenuItemShowPlayerColor = null;
      private ToolStripMenuItem mMenuItemShowSkeleton = null;
      private ToolStripMenuItem mMenuItemShowBoneAxis = null;
      private ToolStripMenuItem mMenuItemShowBoneNames = null;
      private ToolStripMenuItem mMenuItemShowBoundingBox = null;
      private ToolStripMenuItem mMenuItemShowMeshBoxes = null;
      private ToolStripMenuItem mMenuItemShowStatsInfo = null;
      private ToolStripMenuItem mMenuItemShowGrid = null;

      public float degreeToRadian(float degree)
      {
         return (degree * (3.141592654f / 180.0f));
      }

      public VisualEditorPage()
      {
         InitializeComponent();

         SuspendLayout();

         mDynamicMenus.BuildDynamicMenus(toolStripContainer1, menuStrip1);


         mVisualPage.Tag = PropertyPageEnum.eVisual;
         mModelPage.Tag = PropertyPageEnum.eModel;
         mComponentAssetModelPage.Tag = PropertyPageEnum.eComponentAssetModel;
         mComponentAssetParticlePage.Tag = PropertyPageEnum.eComponentAssetParticle;
         mComponentAssetLightPage.Tag = PropertyPageEnum.eComponentAssetLight;
         mAttachLightPage.Tag = PropertyPageEnum.eAttachLight;
         mAttachModelPage.Tag = PropertyPageEnum.eAttachModel;
         mAttachModelRefPage.Tag = PropertyPageEnum.eAttachModelRef;
         mAttachParticlePage.Tag = PropertyPageEnum.eAttachParticle;
         mAttachTerrainEffectPage.Tag = PropertyPageEnum.eAttachTerrainEffect;
         mAnimPage.Tag = PropertyPageEnum.eAnim;
         mAnimAssetPage.Tag = PropertyPageEnum.eAnimAsset;
         mAnimTagAttackPage.Tag = PropertyPageEnum.eAnimAssetTagAttack;
         mAnimTagSoundPage.Tag = PropertyPageEnum.eAnimAssetTagSound;
         mAnimTagParticlePage.Tag = PropertyPageEnum.eAnimAssetTagParticle;
         mAnimTagTerrainEffectPage.Tag = PropertyPageEnum.eAnimAssetTagTerrainEffect;
         mAnimTagUVOffsetPage.Tag = PropertyPageEnum.eAnimAssetTagUVOffset;
         mAnimTagLightPage.Tag = PropertyPageEnum.eAnimAssetTagLight;
         mAnimTagCameraShakePage.Tag = PropertyPageEnum.eAnimAssetTagCameraShake;
         mAnimTagGroundIKPage.Tag = PropertyPageEnum.eAnimAssetTagGroundIK;
         mAnimTagAttachTargetPage.Tag = PropertyPageEnum.eAnimAssetTagAttachTarget;
         mAnimTagSweetSpotPage.Tag = PropertyPageEnum.eAnimAssetTagSweetSpot;
         mAnimTagBuildingDecalPage.Tag = PropertyPageEnum.eAnimAssetTagBuildingDecal;
         mAnimTagTerrainAlphaPage.Tag = PropertyPageEnum.eAnimAssetTagTerrainAlpha;
         mAnimTagRumblePage.Tag = PropertyPageEnum.eAnimAssetTagRumble;
         mAnimTagKillAndThrowPage.Tag = PropertyPageEnum.eAnimAssetTagKillAndThrow;
         mAnimTagPhysicsImpulsePage.Tag = PropertyPageEnum.eAnimAssetTagPhysicsImpulse;
         mPointPage.Tag = PropertyPageEnum.ePoint;
         mLogicPage.Tag = PropertyPageEnum.eLogic;
         mLogicVariationPage.Tag = PropertyPageEnum.eLogicVariation;
         mLogicBuildingCompletionPage.Tag = PropertyPageEnum.eLogicBuildingCompletion;
         mLogicTechPage.Tag = PropertyPageEnum.eLogicTech;
         mLogicSquadModePage.Tag = PropertyPageEnum.eLogicSquadMode;
         mLogicImpactSizePage.Tag = PropertyPageEnum.eLogicImpactSize;
         mLogicDestructionPage.Tag = PropertyPageEnum.eLogicDestruction;
         mLogicUserCivPage.Tag = PropertyPageEnum.eLogicUserCiv;
         mGenericPage.Tag = PropertyPageEnum.eGeneric;

         panelProperty.Controls.Clear();
         panelProperty.Controls.Add(mVisualPage);
         panelProperty.Controls.Add(mModelPage);
         panelProperty.Controls.Add(mComponentAssetModelPage);
         panelProperty.Controls.Add(mComponentAssetParticlePage);
         panelProperty.Controls.Add(mComponentAssetLightPage);
         panelProperty.Controls.Add(mAttachLightPage);
         panelProperty.Controls.Add(mAttachModelPage);
         panelProperty.Controls.Add(mAttachModelRefPage);
         panelProperty.Controls.Add(mAttachParticlePage);
         panelProperty.Controls.Add(mAttachTerrainEffectPage);
         panelProperty.Controls.Add(mAnimPage);
         panelProperty.Controls.Add(mAnimAssetPage);
         panelProperty.Controls.Add(mAnimTagAttackPage);
         panelProperty.Controls.Add(mAnimTagSoundPage);
         panelProperty.Controls.Add(mAnimTagParticlePage);
         panelProperty.Controls.Add(mAnimTagTerrainEffectPage);
         panelProperty.Controls.Add(mAnimTagUVOffsetPage);
         panelProperty.Controls.Add(mAnimTagLightPage);
         panelProperty.Controls.Add(mAnimTagCameraShakePage);
         panelProperty.Controls.Add(mAnimTagGroundIKPage);
         panelProperty.Controls.Add(mAnimTagAttachTargetPage);
         panelProperty.Controls.Add(mAnimTagSweetSpotPage);
         panelProperty.Controls.Add(mAnimTagBuildingDecalPage);
         panelProperty.Controls.Add(mAnimTagTerrainAlphaPage);
         panelProperty.Controls.Add(mAnimTagRumblePage);
         panelProperty.Controls.Add(mAnimTagKillAndThrowPage);
         panelProperty.Controls.Add(mAnimTagPhysicsImpulsePage);
         panelProperty.Controls.Add(mPointPage);
         panelProperty.Controls.Add(mLogicPage);
         panelProperty.Controls.Add(mLogicVariationPage);
         panelProperty.Controls.Add(mLogicBuildingCompletionPage);
         panelProperty.Controls.Add(mLogicTechPage);
         panelProperty.Controls.Add(mLogicSquadModePage);
         panelProperty.Controls.Add(mLogicImpactSizePage);
         panelProperty.Controls.Add(mLogicDestructionPage);
         panelProperty.Controls.Add(mLogicUserCivPage);
         panelProperty.Controls.Add(mGenericPage);

         mVisualPage.setVisualEditorPage(this);
         mModelPage.setVisualEditorPage(this);
         mComponentAssetModelPage.setVisualEditorPage(this);
         mComponentAssetParticlePage.setVisualEditorPage(this);
         mComponentAssetLightPage.setVisualEditorPage(this);
         mAttachLightPage.setVisualEditorPage(this);
         mAttachModelPage.setVisualEditorPage(this);
         mAttachModelRefPage.setVisualEditorPage(this);
         mAttachParticlePage.setVisualEditorPage(this);
         mAttachTerrainEffectPage.setVisualEditorPage(this);
         mAnimPage.setVisualEditorPage(this);
         mAnimAssetPage.setVisualEditorPage(this);
         mAnimTagAttackPage.setVisualEditorPage(this);
         mAnimTagSoundPage.setVisualEditorPage(this);
         mAnimTagParticlePage.setVisualEditorPage(this);
         mAnimTagTerrainEffectPage.setVisualEditorPage(this);
         mAnimTagUVOffsetPage.setVisualEditorPage(this);
         mAnimTagLightPage.setVisualEditorPage(this);
         mAnimTagCameraShakePage.setVisualEditorPage(this);
         mAnimTagGroundIKPage.setVisualEditorPage(this);
         mAnimTagAttachTargetPage.setVisualEditorPage(this);
         mAnimTagSweetSpotPage.setVisualEditorPage(this);
         mAnimTagBuildingDecalPage.setVisualEditorPage(this);
         mAnimTagTerrainAlphaPage.setVisualEditorPage(this);
         mAnimTagRumblePage.setVisualEditorPage(this);
         mAnimTagKillAndThrowPage.setVisualEditorPage(this);
         mAnimTagPhysicsImpulsePage.setVisualEditorPage(this);
         mPointPage.setVisualEditorPage(this);
         mLogicPage.setVisualEditorPage(this);
         mLogicVariationPage.setVisualEditorPage(this);
         mLogicBuildingCompletionPage.setVisualEditorPage(this);
         mLogicTechPage.setVisualEditorPage(this);
         mLogicSquadModePage.setVisualEditorPage(this);
         mLogicImpactSizePage.setVisualEditorPage(this);
         mLogicDestructionPage.setVisualEditorPage(this);
         mLogicUserCivPage.setVisualEditorPage(this);
         mGenericPage.setVisualEditorPage(this);


         UndoButton.Enabled = false;
         RedoButton.Enabled = false;
         undoToolStripMenuItem.Enabled = false;
         redoToolStripMenuItem.Enabled = false;
         deleteToolStripMenuItem.Enabled = false;

         hideAllPropertyPages();

         // Init material
         m_mtrl = new Material();
         m_mtrl.Ambient = Color.White;
         m_mtrl.Diffuse = Color.White;
         m_mtrl.Specular = Color.Black;


         if (s_renderAxisPrim == null)
         {
            s_renderAxisPrim = new BRenderDebugAxis(1);
         }

         

         ResumeLayout();
      }

      //CLM [04.26.06] this function called when a tab becomes 'active'
      public override void Activate()
      {
         base.Activate();


         BRenderDevice.setZNearPlane(0.1f);
         BRenderDevice.setZFarPlane(1000f);

         //this call passes in the panel width and height
         CoreGlobals.getEditorMain().mIGUI.deviceResize(this.panel2.Width, this.panel2.Height, false);

         //this call passes in the panel handle in which you want to render to.
         //this is handled during the 'present' call
         BRenderDevice.setWindowTarget(this.panel2);
      }


      public override Control GetUITarget()
      {
         return this.panel2;
      }


      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {

      }

      //CLM [04.26.06] called when this page is closed from the main tab
      public override void destroyPage()
      {
         base.destroyPage();
         deinitDeviceData();
         deinit();

         for (int i = 0; i < mMeshes.Count; i++)
            mMeshes[i].destroy();

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


         // Show PropertyEditor
         //         CoreGlobals.getEditorMain().mIGUI.ShowDialog("PropertyEditor");

         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(VisualEditorPage));


         // Create a new ContextMenuStrip control.
         mVisualTreeViewContextMenuStrip = new ContextMenuStrip();

         // Attach event handlers for the ContextMenuStrip control
         mVisualTreeViewContextMenuStrip.Opening += new System.ComponentModel.CancelEventHandler(VisualTreeContextMenuStrip_Opening);

         // Create all ToolStripMenuItem that will be added to the ContextMenuStrip with their corresponding handlers.
         mMenuItemAddModel = new ToolStripMenuItem();
         mMenuItemAddModel.Text = "Add Model";
         mMenuItemAddModel.Click += new EventHandler(MenuItemAddModel_Click);

         mMenuItemAddComponent = new ToolStripMenuItem();
         mMenuItemAddComponent.Text = "Add Component";
         mMenuItemAddComponent.Click += new EventHandler(MenuItemAddComponent_Click);

         //mMenuItemAddAttach = new ToolStripMenuItem();
         //mMenuItemAddAttach.Text = "Add Attachment";
         //mMenuItemAddAttach.Click += new EventHandler(MenuItemAddAttach_Click);

         mMenuItemAddAttach = new ToolStripMenuItem();
         mMenuItemAddAttach.Text = "Add Attachment";

            mMenuItemAddAttachModel = new ToolStripMenuItem();
            mMenuItemAddAttachModel.Text = "Model";
            mMenuItemAddAttachModel.Click += new EventHandler(MenuItemAddAttachModel_Click);
            mMenuItemAddAttach.DropDownItems.Add(mMenuItemAddAttachModel);

            mMenuItemAddAttachModelRef = new ToolStripMenuItem();
            mMenuItemAddAttachModelRef.Text = "ModelRef";
            mMenuItemAddAttachModelRef.Click += new EventHandler(MenuItemAddAttachModelRef_Click);
            mMenuItemAddAttach.DropDownItems.Add(mMenuItemAddAttachModelRef);

            mMenuItemAddAttachParticle = new ToolStripMenuItem();
            mMenuItemAddAttachParticle.Text = "Particle";
            mMenuItemAddAttachParticle.Click += new EventHandler(MenuItemAddAttachParticle_Click);
            mMenuItemAddAttach.DropDownItems.Add(mMenuItemAddAttachParticle);

            mMenuItemAddAttachLight = new ToolStripMenuItem();
            mMenuItemAddAttachLight.Text = "Light";
            mMenuItemAddAttachLight.Click += new EventHandler(MenuItemAddAttachLight_Click);
            mMenuItemAddAttach.DropDownItems.Add(mMenuItemAddAttachLight);

            mMenuItemAddAttachTerrainEffect = new ToolStripMenuItem();
            mMenuItemAddAttachTerrainEffect.Text = "TerrainEffect";
            mMenuItemAddAttachTerrainEffect.Click += new EventHandler(MenuItemAddAttachTerrainEffect_Click);
            mMenuItemAddAttach.DropDownItems.Add(mMenuItemAddAttachTerrainEffect);

         mMenuItemAddLogic = new ToolStripMenuItem();
         mMenuItemAddLogic.Text = "Add Logic";

            mMenuItemAddLogicVariation = new ToolStripMenuItem();
            mMenuItemAddLogicVariation.Text = "Variation";
            mMenuItemAddLogicVariation.Click += new EventHandler(MenuItemAddLogicVariation_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicVariation);

            mMenuItemAddLogicBuildingCompletion = new ToolStripMenuItem();
            mMenuItemAddLogicBuildingCompletion.Text = "BuildingCompletion";
            mMenuItemAddLogicBuildingCompletion.Click += new EventHandler(MenuItemAddLogicBuildingCompletion_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicBuildingCompletion);

            mMenuItemAddLogicTech = new ToolStripMenuItem();
            mMenuItemAddLogicTech.Text = "Tech";
            mMenuItemAddLogicTech.Click += new EventHandler(MenuItemAddLogicTech_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicTech);

            mMenuItemAddLogicSquadMode = new ToolStripMenuItem();
            mMenuItemAddLogicSquadMode.Text = "SquadMode";
            mMenuItemAddLogicSquadMode.Click += new EventHandler(MenuItemAddLogicSquadMode_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicSquadMode);

            mMenuItemAddLogicImpactSize = new ToolStripMenuItem();
            mMenuItemAddLogicImpactSize.Text = "ImpactSize";
            mMenuItemAddLogicImpactSize.Click += new EventHandler(MenuItemAddLogicImpactSize_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicImpactSize);

            mMenuItemAddLogicDestruction = new ToolStripMenuItem();
            mMenuItemAddLogicDestruction.Text = "Destruction";
            mMenuItemAddLogicDestruction.Click += new EventHandler(MenuItemAddLogicDestruction_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicDestruction);

            mMenuItemAddLogicUserCiv = new ToolStripMenuItem();
            mMenuItemAddLogicUserCiv.Text = "UserCiv";
            mMenuItemAddLogicUserCiv.Click += new EventHandler(MenuItemAddLogicUserCiv_Click);
            mMenuItemAddLogic.DropDownItems.Add(mMenuItemAddLogicUserCiv);

         mMenuItemAddLogicData = new ToolStripMenuItem();
         mMenuItemAddLogicData.Text = "Add Node";
         mMenuItemAddLogicData.Click += new EventHandler(MenuItemAddLogicData_Click);

         mMenuItemAddPoint = new ToolStripMenuItem();
         mMenuItemAddPoint.Text = "Add Point";
         mMenuItemAddPoint.Click += new EventHandler(MenuItemAddPoint_Click);

         mMenuItemAddComponentAsset = new ToolStripMenuItem();
         mMenuItemAddComponentAsset.Text = "Add Asset";

            mMenuItemAddComponentAssetModel = new ToolStripMenuItem();
            mMenuItemAddComponentAssetModel.Text = "Model";
            mMenuItemAddComponentAssetModel.Click += new EventHandler(MenuItemAddComponentAssetModel_Click);
            mMenuItemAddComponentAsset.DropDownItems.Add(mMenuItemAddComponentAssetModel);

            mMenuItemAddComponentAssetParticle = new ToolStripMenuItem();
            mMenuItemAddComponentAssetParticle.Text = "Particle";
            mMenuItemAddComponentAssetParticle.Click += new EventHandler(MenuItemAddComponentAssetParticle_Click);
            mMenuItemAddComponentAsset.DropDownItems.Add(mMenuItemAddComponentAssetParticle);

            mMenuItemAddComponentAssetLight = new ToolStripMenuItem();
            mMenuItemAddComponentAssetLight.Text = "Light";
            mMenuItemAddComponentAssetLight.Click += new EventHandler(MenuItemAddComponentAssetLight_Click);
            mMenuItemAddComponentAsset.DropDownItems.Add(mMenuItemAddComponentAssetLight);

         mMenuItemAddAnim = new ToolStripMenuItem();
         mMenuItemAddAnim.Text = "Add Anim";
         mMenuItemAddAnim.Click += new EventHandler(MenuItemAddAnim_Click);

         mMenuItemAddAnimAsset = new ToolStripMenuItem();
         mMenuItemAddAnimAsset.Text = "Add Asset";
         mMenuItemAddAnimAsset.Click += new EventHandler(MenuItemAddAnimAsset_Click);


         mMenuItemAddAnimTag = new ToolStripMenuItem();
         mMenuItemAddAnimTag.Text = "Add Tag";

            mMenuItemAddAnimTagAttack = new ToolStripMenuItem();
            mMenuItemAddAnimTagAttack.Text = "Attack";
            mMenuItemAddAnimTagAttack.Click += new EventHandler(MenuItemAddAnimAssetTagAttack_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagAttack);

            mMenuItemAddAnimTagSound = new ToolStripMenuItem();
            mMenuItemAddAnimTagSound.Text = "Sound";
            mMenuItemAddAnimTagSound.Click += new EventHandler(MenuItemAddAnimAssetTagSound_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagSound);

            mMenuItemAddAnimTagParticle = new ToolStripMenuItem();
            mMenuItemAddAnimTagParticle.Text = "Particle";
            mMenuItemAddAnimTagParticle.Click += new EventHandler(MenuItemAddAnimAssetTagParticle_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagParticle);

            mMenuItemAddAnimTagTerrainEffect = new ToolStripMenuItem();
            mMenuItemAddAnimTagTerrainEffect.Text = "TerrainEffect";
            mMenuItemAddAnimTagTerrainEffect.Click += new EventHandler(MenuItemAddAnimAssetTagTerrainEffect_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagTerrainEffect);

            mMenuItemAddAnimTagUVOffset = new ToolStripMenuItem();
            mMenuItemAddAnimTagUVOffset.Text = "UV Offset";
            mMenuItemAddAnimTagUVOffset.Click += new EventHandler(MenuItemAddAnimAssetTagUVOffset_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagUVOffset);

            mMenuItemAddAnimTagLight = new ToolStripMenuItem();
            mMenuItemAddAnimTagLight.Text = "Light";
            mMenuItemAddAnimTagLight.Click += new EventHandler(MenuItemAddAnimAssetTagLight_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagLight);

            mMenuItemAddAnimTagCameraShake = new ToolStripMenuItem();
            mMenuItemAddAnimTagCameraShake.Text = "CameraShake";
            mMenuItemAddAnimTagCameraShake.Click += new EventHandler(MenuItemAddAnimAssetTagCameraShake_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagCameraShake);

            mMenuItemAddAnimTagGroundIK = new ToolStripMenuItem();
            mMenuItemAddAnimTagGroundIK.Text = "GroundIK";
            mMenuItemAddAnimTagGroundIK.Click += new EventHandler(MenuItemAddAnimAssetTagGroundIK_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagGroundIK);

            mMenuItemAddAnimTagAttachTarget = new ToolStripMenuItem();
            mMenuItemAddAnimTagAttachTarget.Text = "AttachTarget";
            mMenuItemAddAnimTagAttachTarget.Click += new EventHandler(MenuItemAddAnimAssetTagAttachTarget_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagAttachTarget);

            mMenuItemAddAnimTagSweetSpot = new ToolStripMenuItem();
            mMenuItemAddAnimTagSweetSpot.Text = "SweetSpot";
            mMenuItemAddAnimTagSweetSpot.Click += new EventHandler(MenuItemAddAnimAssetTagSweetSpot_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagSweetSpot);

            mMenuItemAddAnimTagBuildingDecal = new ToolStripMenuItem();
            mMenuItemAddAnimTagBuildingDecal.Text = "BuildingDecal";
            mMenuItemAddAnimTagBuildingDecal.Click += new EventHandler(MenuItemAddAnimAssetTagBuildingDecal_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagBuildingDecal);

            mMenuItemAddAnimTagTerrainAlpha = new ToolStripMenuItem();
            mMenuItemAddAnimTagTerrainAlpha.Text = "TerrainAlpha";
            mMenuItemAddAnimTagTerrainAlpha.Click += new EventHandler(MenuItemAddAnimAssetTagTerrainAlpha_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagTerrainAlpha);

            mMenuItemAddAnimTagRumble = new ToolStripMenuItem();
            mMenuItemAddAnimTagRumble.Text = "Rumble";
            mMenuItemAddAnimTagRumble.Click += new EventHandler(MenuItemAddAnimAssetTagRumble_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagRumble);

            mMenuItemAddAnimTagKillAndThrow = new ToolStripMenuItem();
            mMenuItemAddAnimTagKillAndThrow.Text = "KillAndThrow";
            mMenuItemAddAnimTagKillAndThrow.Click += new EventHandler(MenuItemAddAnimAssetTagKillAndThrow_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagKillAndThrow);

            mMenuItemAddAnimTagPhysicsImpulse = new ToolStripMenuItem();
            mMenuItemAddAnimTagPhysicsImpulse.Text = "PhysicsImpulse";
            mMenuItemAddAnimTagPhysicsImpulse.Click += new EventHandler(MenuItemAddAnimAssetTagPhysicsImpulse_Click);
            mMenuItemAddAnimTag.DropDownItems.Add(mMenuItemAddAnimTagPhysicsImpulse);

         mMenuItemCut = new ToolStripMenuItem();
         mMenuItemCut.Text = "Cut";
         mMenuItemCut.Click += new EventHandler(MenuItemCut_Click);
         mMenuItemCut.Image = ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemCopy = new ToolStripMenuItem();
         mMenuItemCopy.Text = "Copy";
         mMenuItemCopy.Click += new EventHandler(MenuItemCopy_Click);
         mMenuItemCopy.Image = ((System.Drawing.Image)(resources.GetObject("copyToolStripMenuItem.Image")));

         mMenuItemPaste = new ToolStripMenuItem();
         mMenuItemPaste.Text = "Paste";
         mMenuItemPaste.Click += new EventHandler(MenuItemPaste_Click);
         mMenuItemPaste.Image = ((System.Drawing.Image)(resources.GetObject("pasteToolStripMenuItem.Image")));

         mMenuItemDelete = new ToolStripMenuItem();
         mMenuItemDelete.Text = "Delete";
         mMenuItemDelete.Click += new EventHandler(MenuItemDelete_Click);
         mMenuItemDelete.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem.Image")));



         // Create a ContextMenuStrip for viewport
         mViewportContextMenuStrip = new ContextMenuStrip();

         mMenuItemShowModel = new ToolStripMenuItem();
         mMenuItemShowModel.Checked = true;
         mMenuItemShowModel.CheckState = m_bShowModel ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowModel.Text = "Show Model";
         mMenuItemShowModel.Click += new EventHandler(MenuItemShowModel_Click);

         mMenuItemShowPlayerColor = new ToolStripMenuItem();
         mMenuItemShowPlayerColor.Checked = true;
         mMenuItemShowPlayerColor.CheckState = m_bShowPlayerColor ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowPlayerColor.Text = "Show Player Color";
         mMenuItemShowPlayerColor.Click += new EventHandler(MenuItemShowPlayerColor_Click);

         mMenuItemShowSkeleton = new ToolStripMenuItem();
         mMenuItemShowSkeleton.Checked = true;
         mMenuItemShowSkeleton.CheckState = m_bShowSkeleton ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowSkeleton.Text = "Show Skeleton";
         mMenuItemShowSkeleton.Click += new EventHandler(MenuItemShowSkeleton_Click);

         mMenuItemShowBoneAxis = new ToolStripMenuItem();
         mMenuItemShowBoneAxis.Checked = true;
         mMenuItemShowBoneAxis.CheckState = m_bShowBoneAxis ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowBoneAxis.Text = "Show Bone Axis";
         mMenuItemShowBoneAxis.Click += new EventHandler(MenuItemShowBoneAxis_Click);

         mMenuItemShowBoneNames = new ToolStripMenuItem();
         mMenuItemShowBoneNames.Checked = true;
         mMenuItemShowBoneNames.CheckState = m_bShowBoneNames ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowBoneNames.Text = "Show Bone Names";
         mMenuItemShowBoneNames.Click += new EventHandler(MenuItemShowBoneNames_Click);

         mMenuItemShowBoundingBox = new ToolStripMenuItem();
         mMenuItemShowBoundingBox.Checked = true;
         mMenuItemShowBoundingBox.CheckState = m_bShowBoundingBox ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowBoundingBox.Text = "Show Bounding Box";
         mMenuItemShowBoundingBox.Click += new EventHandler(MenuItemShowBoundingBox_Click);

         mMenuItemShowMeshBoxes = new ToolStripMenuItem();
         mMenuItemShowMeshBoxes.Checked = true;
         mMenuItemShowMeshBoxes.CheckState = m_bShowMeshBoxes ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowMeshBoxes.Text = "Show Mesh Boxes";
         mMenuItemShowMeshBoxes.Click += new EventHandler(MenuItemShowMeshBoxes_Click);

         mMenuItemShowStatsInfo = new ToolStripMenuItem();
         mMenuItemShowStatsInfo.Checked = true;
         mMenuItemShowStatsInfo.CheckState = m_bShowStatsInfo ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowStatsInfo.Text = "Show Stats Info";
         mMenuItemShowStatsInfo.Click += new EventHandler(MenuItemShowStatsInfo_Click);

         mMenuItemShowGrid = new ToolStripMenuItem();
         mMenuItemShowGrid.Checked = true;
         mMenuItemShowGrid.CheckState = m_bShowGrid ? CheckState.Checked : CheckState.Unchecked;
         mMenuItemShowGrid.Text = "Show Grid";
         mMenuItemShowGrid.Click += new EventHandler(MenuItemShowGrid_Click);

         mViewportContextMenuStrip.Items.Add(mMenuItemShowModel);
         mViewportContextMenuStrip.Items.Add(mMenuItemShowPlayerColor);
         mViewportContextMenuStrip.Items.Add("-");
         mViewportContextMenuStrip.Items.Add(mMenuItemShowSkeleton);
         mViewportContextMenuStrip.Items.Add(mMenuItemShowBoneAxis);
         mViewportContextMenuStrip.Items.Add(mMenuItemShowBoneNames);
         mViewportContextMenuStrip.Items.Add(mMenuItemShowBoundingBox);
         mViewportContextMenuStrip.Items.Add(mMenuItemShowMeshBoxes);
         mViewportContextMenuStrip.Items.Add("-");
         mViewportContextMenuStrip.Items.Add(mMenuItemShowStatsInfo);
         mViewportContextMenuStrip.Items.Add("-");
         mViewportContextMenuStrip.Items.Add(mMenuItemShowGrid);

         mUndoRedoManager = new UndoRedoManager(this);

         // Initialize granny manager
         GrannyManager2.init();

     

      }

      public override void deinit()
      {
         base.deinit();

      }

      //CLM [04.26.06] these functions called for all data that's not in the MANAGED pool for d3d.
      //on a device resize, or reset, these functions are called for you.
      public override void initDeviceData()
      {
         base.initDeviceData();

      }
      override public void deinitDeviceData()
      {
         base.deinitDeviceData();

      }

      private void initStatic()
      {
      }


      private void hideAllPropertyPages()
      {
         for (int i = 0; i < panelProperty.Controls.Count; i++)
         {
            panelProperty.Controls[i].Hide();
         }
      }

      private void showPropertyPage(PropertyPageEnum page)
      {
         if (m_curPageShown == page)
            return;

         m_curPageShown = page;
         hideAllPropertyPages();

         for (int i = 0; i < panelProperty.Controls.Count; ++i)
         {
            if (page == (PropertyPageEnum)panelProperty.Controls[i].Tag)
            {
               panelProperty.Controls[i].Show();
            }
         }
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

      override public void update()
      {
         base.update();

         UndoButton.Enabled = mUndoRedoManager.canUndo();
         RedoButton.Enabled = mUndoRedoManager.canRedo();
         undoToolStripMenuItem.Enabled = mUndoRedoManager.canUndo();
         redoToolStripMenuItem.Enabled = mUndoRedoManager.canRedo();

         bool canCopy = (VisualTreeView.SelectedNode != null) ? true : false;
         bool canPaste = ((VisualTreeView.SelectedNode != null) && (s_copyNode != null)) ? true : false;
         bool canDelete  = (VisualTreeView.SelectedNode != null) ? true : false;

         if (VisualTreeView.Focused)
         {
            cutToolStripMenuItem.Enabled = canCopy && canDelete;
            copyToolStripMenuItem.Enabled = canCopy;
            pasteToolStripMenuItem.Enabled = canPaste;
            deleteToolStripMenuItem.Enabled = canDelete;
         }
         else
         {
            cutToolStripMenuItem.Enabled = false;
            copyToolStripMenuItem.Enabled = false;
            pasteToolStripMenuItem.Enabled = false;
            deleteToolStripMenuItem.Enabled = false;
         }

         mMenuItemCut.Enabled = canCopy;
         mMenuItemCopy.Enabled = canCopy;
         mMenuItemPaste.Enabled = canPaste;

         // Update anim control
         AnimationControlPanel.Enabled = (m_animationControl.getState() != AnimationControl.AnimControlStateEnum.eInactive) ? true : false;

         switch(m_animationControl.getState())
         {
            case AnimationControl.AnimControlStateEnum.ePlaying:
               PlayButton.ImageIndex = 1;
               break;
            case AnimationControl.AnimControlStateEnum.eInactive:
            case AnimationControl.AnimControlStateEnum.ePaused:
               PlayButton.ImageIndex = 0;
               break;
         }

         switch(m_animationControl.getLoopMode())
         {
            case AnimationControl.AnimControlLoopModeEnum.eRepeat:
               RepeatButton.ImageIndex = 2;
               break;
            case AnimationControl.AnimControlLoopModeEnum.ePlayOnce:
               RepeatButton.ImageIndex = 3;
               break;
         }

         if (m_animationControl.isSoundEnabled())
         {
            SoundButton.ImageIndex = 6;
         }
         else
         {
            SoundButton.ImageIndex = 7;
         }

         if (m_animationControl.isMotionEnabled())
         {
            MotionButton.ImageIndex = 2;
         }
         else
         {
            MotionButton.ImageIndex = 3;
         }

         switch (m_animationControl.getSpeed())
         {
            case AnimationControl.AnimControlSpeedEnum.eOneFourth:
               SpeedButton.Text = "1/4x";
               break;
            case AnimationControl.AnimControlSpeedEnum.eOneHalf:
               SpeedButton.Text = "1/2x";
               break;
            case AnimationControl.AnimControlSpeedEnum.eOne:
               SpeedButton.Text = "1x";
               break;
            case AnimationControl.AnimControlSpeedEnum.eTwo:
               SpeedButton.Text = "2x";
               break;
            case AnimationControl.AnimControlSpeedEnum.eFour:
               SpeedButton.Text = "4x";
               break;
         }

         m_animationControl.update(0.0333f);

         AnimationTimeTrackBar.Value = (int)(m_animationControl.getNormalizedTime() * 1000);
         AnimationTimeTextBox.Text = m_animationControl.getNormalizedTime().ToString("0.000;-0.000;0.000"); ;
         AnimationDurationTextBox.Text = m_animationControl.getDuration().ToString("0.000;-0.000;0.000"); ;

         // Update granny manager
         //GrannyManager2.update(0.0333f);
         GrannyManager2.reloadChangedResources();

         // Update camera transition
         mCameraTransition.update(ref mCameraManager.mEye, ref mCameraManager.mLookAt);
      }




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
                  if(curCompAsset.mInstance != null)
                  {
                     bbox.addBox(curCompAsset.mInstance.mStaticBoundingBox);
                  }
               }
               break;
         }
      }

      public void preLoadAttachment(visualModelComponentOrAnimAttach curAttachment, ref BBoundingBox bbox)
      {
         switch(curAttachment.type)
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
         if(logic.logicdata.Count > 0)
         {
            visualLogicData curLogicData = logic.logicdata[0];
            preLoadLogicData(curLogicData, ref bbox);
         }
      }

      public void preLoadLogicData(visualLogicData logicData, ref BBoundingBox bbox)
      {

         if(!String.IsNullOrEmpty(logicData.modelref))
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
         if (VisualTreeView.SelectedNode == null)
            return;


         TreeNode node = VisualTreeView.SelectedNode;
         if (node.Tag != null)
         {
            visual visual = node.Tag as visual;
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

            visualModel model = node.Tag as visualModel;
            if (model != null)
            {
               preLoadModel(model, ref bbox);
               return;
            }

            visualModelComponent component = node.Tag as visualModelComponent;
            if (component != null)
            {
               preLoadComponent(component, ref bbox);
               return;
            }

            visualModelComponentAsset componentAsset = node.Tag as visualModelComponentAsset;
            if (componentAsset != null)
            {
               preLoadComponentAsset(componentAsset, ref bbox);
               return;
            }

            visualModelComponentOrAnimAttach attach = node.Tag as visualModelComponentOrAnimAttach;
            if (attach != null)
            {
               preLoadAttachment(attach, ref bbox);
               return;
            }

            visualModelAnim anim = node.Tag as visualModelAnim;
            if (anim != null)
            {
               TreeNode modelNode = node.Parent;
               visualModel vmodel = modelNode.Tag as visualModel;
               if(vmodel != null)
               {
                  preLoadModel(vmodel, ref bbox);
               }

               foreach (visualModelComponentOrAnimAttach curAttachment in anim.attach)
               {
                  preLoadAttachment(curAttachment, ref bbox);
               }
               return;
            }

            visualModelAnimAsset animAsset = node.Tag as visualModelAnimAsset;
            if (animAsset != null)
            {
               // load anim
               animAsset.loadAsset();

               TreeNode animNode = node.Parent;
               visualModelAnim manim = animNode.Tag as visualModelAnim;
               if (manim != null)
               {
                  TreeNode modelNode = animNode.Parent;
                  visualModel vmodel = modelNode.Tag as visualModel;
                  if (vmodel != null)
                  {
                     preLoadModel(vmodel, ref bbox);
                  }

                  foreach (visualModelComponentOrAnimAttach curAttachment in manim.attach)
                  {
                     preLoadAttachment(curAttachment, ref bbox);
                  }
               }
               return;
            }

            visualModelAnimAssetTag animTag = node.Tag as visualModelAnimAssetTag;
            if (animTag != null)
            {
               visualModelAnimAsset animAsset2 = node.Parent.Tag as visualModelAnimAsset;
               if (animAsset2 != null)
               {
                  // load anim
                  animAsset2.loadAsset();

                  TreeNode animNode = node.Parent.Parent;
                  visualModelAnim manim = animNode.Tag as visualModelAnim;
                  if (manim != null)
                  {
                     TreeNode modelNode = animNode.Parent;
                     visualModel vmodel = modelNode.Tag as visualModel;
                     if (vmodel != null)
                     {
                        preLoadModel(vmodel, ref bbox);
                     }

                     foreach (visualModelComponentOrAnimAttach curAttachment in manim.attach)
                     {
                        preLoadAttachment(curAttachment, ref bbox);
                     }
                  }
               }
               return;
            }

            visualModelComponentPoint point = node.Tag as visualModelComponentPoint;
            if (point != null)
            {
               return;
            }

            visualLogic logic = node.Tag as visualLogic;
            if (logic != null)
            {
               preLoadLogic(logic, ref bbox);
               return;
            }

            visualLogicData logicData = node.Tag as visualLogicData;
            if (logicData != null)
            {
               preLoadLogicData(logicData, ref bbox);
               return;
            }
         }
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
         if(mVisualFile == null)
            return;

         if (mIdentityInstanceVB == null)
            makeInstanceVB();
         BRenderDevice.getDevice().SetStreamSourceFrequency(0, (1 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, (2 << 30) | 1);
         BRenderDevice.getDevice().SetStreamSource(1, mIdentityInstanceVB, 0, 64);
       


         // apply animation state
         m_animationControl.apply(ref m_worldMatrix);


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

         /*
         // Set Addressing mode
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.AddressU, (int)TextureAddress.Clamp);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.AddressV, (int)TextureAddress.Clamp);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.AddressU, (int)TextureAddress.Clamp);
         BRenderDevice.getDevice().SetSamplerState(1, SamplerStageStates.AddressV, (int)TextureAddress.Clamp);
         */



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
         if ((VisualTreeView != null) && (VisualTreeView.SelectedNode != null))
         {
            BRenderDevice.getDevice().SetTransform(TransformType.World, m_worldMatrix);
            mVisualFile.render(VisualTreeView.SelectedNode, renderMode);
         }


         // Render grid
         if (m_bShowGrid)
            m_grid.renderGrid();


         // Render axis
         renderAxis();

         

         BRenderDevice.endScene();
         BRenderDevice.present();
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
         BRenderDevice.getDevice().Transform.Projection = Matrix.PerspectiveFovLH(degreeToRadian(45), 1.0f /*(float)BRenderDevice.getWidth() / (float)BRenderDevice.getHeight()*/, 0.5f, 6.0f);

         s_renderAxisPrim.render();

         BRenderDevice.getDevice().Transform.World = w;
         BRenderDevice.getDevice().Transform.Projection = p;
         BRenderDevice.getDevice().Viewport = vpp;


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
      }

      //these functions will be called from the file menu. It's the panel's job to call the proper dialog
      override public void save_file()
      {
         // exit if nothing to save
         if (mVisualFile == null)
            return;

         // check for errors
         if (!mVisualFile.isBranchValid())
         {
            if (MessageBox.Show("Some errors have been encountered in this file.  Please resolve these before\nsaving.  Nodes that have errors (usually missing data) are shown in red in \nthe tree view.", "Errors Found!", MessageBoxButtons.OK) == DialogResult.OK)
            {
               return;
            }
         }

         if (mVisualFileName != null)
         {
            SaveVisual(mVisualFileName);
         }
         else
         {
            save_file_as();
         }
      }

      override public void save_file_as()
      {
         // exit if nothing to save
         if (mVisualFile == null)
            return;

         // check for errors
         if (!mVisualFile.isBranchValid())
         {
            if (MessageBox.Show("Some errors have been encountered in this file.  Please resolve these before\nsaving.  Nodes that have errors (usually missing data) are shown in red in \nthe tree view.", "Errors Found!", MessageBoxButtons.OK) == DialogResult.OK)
            {
               return;
            }
         }

         SaveFileDialog d = new SaveFileDialog();

         if (mVisualFileName != null)
         {
            d.FileName = mVisualFileName;
         }
         else if (mVisualDefaultName != null)
         {
            d.FileName = mVisualDefaultName;
         }
         else
         {
            d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;
         }
         d.Filter = "ES Visual files (*.vis)|*.vis";
         d.FilterIndex = 0;
         
         if (d.ShowDialog() == DialogResult.OK)
         {
            mVisualFileName = d.FileName; 
            SaveVisual(d.FileName);
         }
      }

      override public void open_file(string absoluteFilename)
      {
         ResourcePathInfo pathinfo = new ResourcePathInfo(absoluteFilename);
         string filename = pathinfo.RelativePath;

         //- strip out the art directory.   ResourcePathInfo should do this for you
         filename = filename.Substring("art\\".Length);

         mVisualFileName = filename;
         LoadVisual(filename);
      }

      override public void open_file() 
      {
         if (mIsDocumentModified)
         {
            if (MessageBox.Show("Unsaved changes will be lost.  Continue?", "Warning", MessageBoxButtons.OKCancel) != DialogResult.OK)
            {
               return;
            }
         }

         OpenFileDialog d = new OpenFileDialog();

         if (mVisualFileName != null)
         {
            d.FileName = mVisualFileName;
         }
         else
         {
            d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;
         }
         d.Filter = "ES Visual files (*.vis)|*.vis";
         d.FilterIndex = 0;

         if (d.ShowDialog() == DialogResult.OK)
         {
            mVisualFileName = d.FileName;
            LoadVisual(d.FileName);
         }
      }


      public bool SaveVisual(string fileName)
      {
         bool success = WriteVisual(fileName);

         if (!success)
            return false;

         SetTabName(fileName);

         // Clear modified flag
         mIsDocumentModified = false;

         return true;
      }


      public bool WriteVisual(string fileName)
      {
         Stream st;
         if (!File.Exists(fileName))
         {
            // create file
            st = File.Create(fileName);
         }
         else
         {
            // make sure we have it checked out
            if ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
            {
               string str = String.Format("File: {0} is read only!\nYou are not allowed to save!\nMake sure you checked the file out of Perforce!", fileName);
               MessageBox.Show(str, "Read Only File", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
               return false;
            }

            // Disable file watcher from checking this file
            m_bVisualWasSavedThisFrame = true;

            st = File.Open(fileName, FileMode.Truncate);
         }


         // Copy into a new file to resort items
         visual visualFileClone = (visual) mVisualFile.clone(true);
         visualFileClone.resort();


         XmlSerializer s = new XmlSerializer(typeof(visual));
         s.Serialize(st, visualFileClone);
         st.Close();

         XMBProcessor.CreateXMB(fileName, false);

         // Since this visual has been cloned we need to reload it in the manager, or else the old
         // version may come back when switching models.
         //
         BVisualManager.reloadProtoVisual(fileName);

         return (true);
      }

      public bool NewVisual()
      {
         if (mIsDocumentModified) 
         {
            if (MessageBox.Show("Are you sure you want to create a new visual, you will loose any unsaved data.", "Create new visual?", MessageBoxButtons.OKCancel) != DialogResult.OK)
            {
               return false;
            }
         }

         // Clear all undo/redo actions
         mUndoRedoManager.removeAllUndoRedoActions();

         // Clear modified flag
         mIsDocumentModified = false;


         s_newVisualCount++;
         String name = String.Format("Untitled_{0}.vis", s_newVisualCount);

         mVisualFileName = null;
         mVisualDefaultName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, name); 

         SetTabName(name);

         // Create new visual
         mVisualFile = new visual();
         mVisualFile.model.Add(new visualModel());

         // Update tree view
         CreateTreeView();



         // Init bounding box
         mModelBoundingBox.empty();
         mModelBoundingBox.addPoint(5.0f, 5.0f, 5.0f);
         mModelBoundingBox.addPoint(-5.0f, 0.0f, -5.0f);



         mCameraManager.setModelBoundingBox(mModelBoundingBox);

         // Init camera position
         resetCamera(mModelBoundingBox);

         // Create grid
         m_grid.resetGrid(mModelBoundingBox);


         return true;
      }

      public bool LoadVisual(string animFileName)
      {
         string fileName = "";
         try
         {
            if (animFileName == null)
               return false;

            fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, animFileName);

            if (!File.Exists(fileName))
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Can't find visual file: {0}", fileName));
               return false;
            }


            // Clear all undo/redo actions
            mUndoRedoManager.removeAllUndoRedoActions();

            // Clear modified flag
            mIsDocumentModified = false;

            // Clear old visual
            mVisualFile = null;


            // Keep file name
            mVisualFileName = fileName;
            mVisualDefaultName = null;

            visual visFile = BVisualManager.getOrLoadProtoVisual(fileName);

            // Create a copy of the visual
            mVisualFile = (visual) visFile.clone(true);


            // Update tree view
            CreateTreeView();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error parsing visual file: {0}.  {1}", fileName, ex.ToString()));
         }


         SetTabName(animFileName);



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

         return true;
      }

      public void ReloadVisual()
      {
         LoadVisual(mVisualFileName);
      }

      public void setRenderMode(GrannyInstance.eVisualEditorRenderMode mode)
      {
         mRenderMode = mode;
      }

      public GrannyInstance.eVisualEditorRenderMode getRenderMode()
      {
         return mRenderMode;
      }

      public void SetTabName(string filename)
      {
         Parent.Text = Path.GetFileNameWithoutExtension(filename);
      }

      private void CreateTreeView()
      {
         // Clear current
         VisualTreeView.Nodes.Clear();

         if (mVisualFile == null)
            return;


         VisualTreeView.BeginUpdate();
         TreeNode visualNode = mVisualFile.createTreeNode();

 
         VisualTreeView.Nodes.Add(visualNode);

         /*
         // Expand all
         VisualTreeView.ExpandAll();
         */

         // Expand only two levels
         foreach (TreeNode node in VisualTreeView.Nodes)
         {
            node.Expand();
            foreach (TreeNode childNode in node.Nodes)
            {
               childNode.Expand();
            }
         }

         VisualTreeView.EndUpdate();


         // Select top node
         if (VisualTreeView.Nodes.Count > 0)
         {
            VisualTreeView.SelectedNode = VisualTreeView.Nodes[0];
            VisualTreeView.Select();
         }
      }


      private void RefreshTreeViewText()
      {
         // Collapse everything at the given depth
         foreach (TreeNode node in VisualTreeView.Nodes)
         {
            RefreshTreeViewTextRecursive(node);
         }
      }

      public void RefreshTreeViewTextRecursive(TreeNode node)
      {
         // update info in current node
         UpdateTreeNodeText(node);

         // loop through all children
         foreach (TreeNode childNode in node.Nodes)
            RefreshTreeViewTextRecursive(childNode);
      }

      public void UpdateTreeNodeText(TreeNode node)
      {
         if (node.Tag == null)
            return;

         XMLVisualNode xmlNode = node.Tag as XMLVisualNode;
         if(xmlNode != null)
            xmlNode.updateTreeNodeText(node);
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

      /*

      private void renderGrid()
      {
         if (mGridVB == null)
            return;

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);

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
       */



      // IUnitPicker interface functions
      //
 
      public void PlayerIdSelectedChanged(int index) { }
      public void UnitSelectedChanged(object obj)
      {
         SimUnitXML unit = obj as SimUnitXML;
         if (unit != null)
         {
            if (mIsDocumentModified)
            {
               if (MessageBox.Show("Unsaved changes will be lost.  Continue?", "Warning", MessageBoxButtons.OKCancel) != DialogResult.OK)
               {
                  return;
               }
               else
               {
                  // Delete changes
                  mVisualFile = null;
               }
            }

            LoadVisual(unit.mAnimFile);
         }
      }




      /*
            static private VertexFormats GlobalVertexFormat = VertexFormats.Position | VertexFormats.Normal | VertexFormats.Texture1;

            static private int MaxBoneCount;
      //      static private granny_local_pose* SharedLocalPose;


            // A vertex blending buffer that I use for all meshes, since
            // it's contents also don't need to be preserved
            static private int MaxMutableVertexCount = 500;          // Number of verts per vertex buffer
            static private int MaxMutableVertexBufferCount = 4;      // Number of vertex buffers
            static private int MutableVertexBufferIndex;
            static private VertexBuffer[] MutableVertexBufferRing;
      */


      private void expandedOneLevelRecursive(TreeNode node)
      {
         if (node.IsExpanded)
         {
            foreach (TreeNode childNode in node.Nodes)
               expandedOneLevelRecursive(childNode);
         }
         else
         {
            node.Expand();
         }
      }

      private int getTreeDepthRecursive(TreeNode node, int depth)
      {
         if (node.IsExpanded)
         {
            int maxDepth = -1;
            foreach (TreeNode childNode in node.Nodes)
            {
               int newDepth = getTreeDepthRecursive(childNode, depth + 1);
               if (newDepth > maxDepth)
                  maxDepth = newDepth;
            }

            return (maxDepth);
         }
         else
         {
            return (depth);
         }
      }

      private void collapseAtDepthRecursive(TreeNode node, int depth, int collapseDepth)
      {
         if (collapseDepth == depth)
         {
            node.Collapse();
         }
         else
         {
            if (node.IsExpanded)
            {
               foreach (TreeNode childNode in node.Nodes)
               {
                  collapseAtDepthRecursive(childNode, depth + 1, collapseDepth);
               }
            }
         }
      }

      #region TreeView Handlers

      private void VisualTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         ShowPropertyPageForSelectedNode();
      }

      public void ShowPropertyPageForSelectedNode()
      {
         BBoundingBox box = new BBoundingBox();
         preLoad(ref box);


         // return if nothing is selected
         if (VisualTreeView.SelectedNode == null)
            return;


         TreeNode node = VisualTreeView.SelectedNode;
         if (node.Tag != null)
         {
            // stop playing animation
            //mVisualFile.stopAnimation();
            m_animationControl.setAnimationParams(null, -1, null);
            
            // Change property page shown
            //

            visual visual = node.Tag as visual;
            if (visual != null)
            {
               mVisualPage.bindData(visual, node);
               showPropertyPage(PropertyPageEnum.eVisual);
               return;
            }

            visualModel model = node.Tag as visualModel;
            if (model != null)
            {
               mModelPage.bindData(model, node);
               showPropertyPage(PropertyPageEnum.eModel);
               return;
            }

            visualModelComponent component = node.Tag as visualModelComponent;
            if (component != null)
            {
               showPropertyPage(PropertyPageEnum.eNone);
               return;
            }

            visualModelComponentAsset componentAsset = node.Tag as visualModelComponentAsset;
            if (componentAsset != null)
            {
               switch (componentAsset.type)
               {
                  case visualModelComponentAsset.ComponentAssetType.Model:
                     mComponentAssetModelPage.bindData(componentAsset, node);
                     showPropertyPage(PropertyPageEnum.eComponentAssetModel);
                     break;
                  case visualModelComponentAsset.ComponentAssetType.Particle:
                     mComponentAssetParticlePage.bindData(componentAsset, node);
                     showPropertyPage(PropertyPageEnum.eComponentAssetParticle);
                     break;
                  case visualModelComponentAsset.ComponentAssetType.Light:
                     mComponentAssetLightPage.bindData(componentAsset, node);
                     showPropertyPage(PropertyPageEnum.eComponentAssetLight);
                     break;
               }
               return;
            }

            visualModelComponentOrAnimAttach attach = node.Tag as visualModelComponentOrAnimAttach;
            if (attach != null)
            {
               switch (attach.type)
               {
                  case visualModelComponentOrAnimAttach.AttachType.LightFile:
                     mAttachLightPage.bindData(attach, node);
                     showPropertyPage(PropertyPageEnum.eAttachLight);
                     break;
                  case visualModelComponentOrAnimAttach.AttachType.ModelFile:
                     mAttachModelPage.bindData(attach, node);
                     showPropertyPage(PropertyPageEnum.eAttachModel);
                     break;
                  case visualModelComponentOrAnimAttach.AttachType.ModelRef:
                     mAttachModelRefPage.bindData(attach, node);
                     showPropertyPage(PropertyPageEnum.eAttachModelRef);
                     break;
                  case visualModelComponentOrAnimAttach.AttachType.ParticleFile:
                     mAttachParticlePage.bindData(attach, node);
                     showPropertyPage(PropertyPageEnum.eAttachParticle);
                     break;
                  case visualModelComponentOrAnimAttach.AttachType.TerrainEffect:
                     mAttachTerrainEffectPage.bindData(attach, node);
                     showPropertyPage(PropertyPageEnum.eAttachTerrainEffect);
                     break;
               }
               return;
            }

            visualModelAnim anim = node.Tag as visualModelAnim;
            if (anim != null)
            {
               mAnimPage.bindData(anim, node);
               showPropertyPage(PropertyPageEnum.eAnim);
               return;
            }

            visualModelAnimAsset animAsset = node.Tag as visualModelAnimAsset;
            if (animAsset != null)
            {
               mAnimAssetPage.bindData(animAsset, node);
               showPropertyPage(PropertyPageEnum.eAnimAsset);

               // play animation hack here
               TreeNode animNode = node.Parent;
               visualModelAnim manim = animNode.Tag as visualModelAnim;
               if (manim != null)
               {
                  TreeNode modelNode = animNode.Parent;
                  visualModel vmodel = modelNode.Tag as visualModel;
                  if ((vmodel != null) && (vmodel.component != null))
                  {
                     if (vmodel.component.asset != null)
                     {
                        if (vmodel.component.asset.mInstance != null)
                        {
                           m_animationControl.setAnimationParams(vmodel.component.asset.mInstance, animAsset.mAnimId, animAsset);
                        }
                     } 
                     else if ((vmodel.component.logic != null) &&
                              (vmodel.component.logic.type == visualLogic.LogicType.Variation) &&
                              (vmodel.component.logic.logicdata.Count > vmodel.component.logic.selectedItem))
                     {
                        visualLogicData curLogicData = vmodel.component.logic.logicdata[vmodel.component.logic.selectedItem];
                        if((curLogicData.asset != null) && (curLogicData.asset.mInstance != null))
                        {
                           m_animationControl.setAnimationParams(curLogicData.asset.mInstance, animAsset.mAnimId, animAsset);
                        }
                     }
                  }
               }
               return;
            }

            visualModelAnimAssetTag animTag = node.Tag as visualModelAnimAssetTag;
            if (animTag != null)
            {
               switch (animTag.type)
               {
                  case visualModelAnimAssetTag.TagType.Attack:
                     mAnimTagAttackPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagAttack);
                     break;
                  case visualModelAnimAssetTag.TagType.Sound:
                     mAnimTagSoundPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagSound);
                     break;
                  case visualModelAnimAssetTag.TagType.Particle:
                     mAnimTagParticlePage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagParticle);
                     break;
                  case visualModelAnimAssetTag.TagType.TerrainEffect:
                     mAnimTagTerrainEffectPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagTerrainEffect);
                     break;
                  case visualModelAnimAssetTag.TagType.UVOffset:
                     mAnimTagUVOffsetPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagUVOffset);
                     break;
                  case visualModelAnimAssetTag.TagType.Light:
                     mAnimTagLightPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagLight);
                     break;
                  case visualModelAnimAssetTag.TagType.CameraShake:
                     mAnimTagCameraShakePage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagCameraShake);
                     break;
                  case visualModelAnimAssetTag.TagType.GroundIK:
                     mAnimTagGroundIKPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagGroundIK);
                     break;
                  case visualModelAnimAssetTag.TagType.AttachTarget:
                     mAnimTagAttachTargetPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagAttachTarget);
                     break;
                  case visualModelAnimAssetTag.TagType.SweetSpot:
                     mAnimTagSweetSpotPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagSweetSpot);
                     break;
                  case visualModelAnimAssetTag.TagType.BuildingDecal:
                     mAnimTagBuildingDecalPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagBuildingDecal);
                     break;
                  case visualModelAnimAssetTag.TagType.TerrainAlpha:
                     mAnimTagTerrainAlphaPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagTerrainAlpha);
                     break;
                  case visualModelAnimAssetTag.TagType.Rumble:
                     mAnimTagRumblePage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagRumble);
                     break;
                  case visualModelAnimAssetTag.TagType.KillAndThrow:
                     mAnimTagKillAndThrowPage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagKillAndThrow);
                     break;
                  case visualModelAnimAssetTag.TagType.PhysicsImpulse:
                     mAnimTagPhysicsImpulsePage.bindData(animTag, node);
                     showPropertyPage(PropertyPageEnum.eAnimAssetTagPhysicsImpulse);
                     break;
               }

               // play animation hack here
               TreeNode animNode = node.Parent.Parent;
               visualModelAnim manim = animNode.Tag as visualModelAnim;
               visualModelAnimAsset animAsset2 = node.Parent.Tag as visualModelAnimAsset;
               if (manim != null)
               {
                  TreeNode modelNode = animNode.Parent;
                  visualModel vmodel = modelNode.Tag as visualModel;


                  if ((vmodel != null) && (vmodel.component != null))
                  {
                     GrannyInstance granInst = null;

                     if (vmodel.component.asset != null)
                     {
                        granInst = vmodel.component.asset.mInstance;
                     }
                     else if ((vmodel.component.logic != null) &&
                              (vmodel.component.logic.type == visualLogic.LogicType.Variation) &&
                              (vmodel.component.logic.logicdata.Count > vmodel.component.logic.selectedItem))
                     {
                        visualLogicData curLogicData = vmodel.component.logic.logicdata[vmodel.component.logic.selectedItem];
                        if (curLogicData.asset != null)
                        {
                           granInst = curLogicData.asset.mInstance;
                        }
                     }

                     if (granInst != null)
                     {                       
                        //vmodel.component.asset.mInstance.playAnimation(animAsset2.mAnimId);
                        m_animationControl.setAnimationParams(granInst, animAsset2.mAnimId, animAsset2);
                        m_animationControl.setNormalizedTime((float)animTag.position);
                        m_animationControl.setState(AnimationControl.AnimControlStateEnum.ePaused);
                     }
                  }
               }
               return;
            }

            visualModelComponentPoint point = node.Tag as visualModelComponentPoint;
            if (point != null)
            {
               mPointPage.bindData(point, node);
               showPropertyPage(PropertyPageEnum.ePoint);
               return;
            }

            /*
            visualLogic logic = node.Tag as visualLogic;
            if (logic != null)
            {
               mLogicPage.bindData(logic, node);
               showPropertyPage(PropertyPageEnum.eLogic);
               return;
            }

            visualLogicData logicData = node.Tag as visualLogicData;
            if (logicData != null)
            {
               mLogicNodePage.bindData(logicData, node);
               showPropertyPage(PropertyPageEnum.eLogicNode);
               return;
            }
            */


            visualLogic logic = node.Tag as visualLogic;
            if (logic != null)
            {
               mLogicPage.bindData(logic, node);
               showPropertyPage(PropertyPageEnum.eLogic);
               return;
            }

            visualLogicData logicData = node.Tag as visualLogicData;
            if (logicData != null)
            {
               // Get Parent to see type (variation, building completion, or tech)
               visualLogic parentLogic = VisualTreeView.SelectedNode.Parent.Tag as visualLogic;
               if (parentLogic != null)
               {
                  switch (parentLogic.type)
                  {
                     case visualLogic.LogicType.BuildingCompletion:
                        mLogicBuildingCompletionPage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicBuildingCompletion);
                        break;
                     case visualLogic.LogicType.Tech:
                        mLogicTechPage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicTech);
                        break;
                     case visualLogic.LogicType.Variation:
                        mLogicVariationPage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicVariation);
                        break;
                     case visualLogic.LogicType.SquadMode:
                        mLogicSquadModePage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicSquadMode);
                        break;
                     case visualLogic.LogicType.ImpactSize:
                        mLogicImpactSizePage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicImpactSize);
                        break;
                     case visualLogic.LogicType.Destruction:
                        mLogicDestructionPage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicDestruction);
                        break;
                     case visualLogic.LogicType.UserCiv:
                        mLogicUserCivPage.bindData(logicData, node);
                        showPropertyPage(PropertyPageEnum.eLogicUserCiv);
                        break;
                  }
                  return;
               }
            }

            // Show a property grid if no custom page is enabled
            mGenericPage.bindData(node.Tag, node);
            showPropertyPage(PropertyPageEnum.eGeneric);
         }
      }

      private void VisualTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         VisualTreeView.SelectedNode = e.Node;

         if (e.Button == MouseButtons.Right)
         {
            mVisualTreeViewContextMenuStrip.Show((Control)VisualTreeView, e.X, e.Y);
         }
      }

      #endregion


      #region ContextMenuStrip Handlers

      // This event handler is invoked when the ContextMenuStrip
      // control's Opening event is raised. It demonstrates
      // dynamic item addition and dynamic SourceControl 
      // determination with reuse.
      void VisualTreeContextMenuStrip_Opening(object sender, System.ComponentModel.CancelEventArgs e)
      {
         // Clear the ContextMenuStrip control's Items collection.
         mVisualTreeViewContextMenuStrip.Items.Clear();

         // If visual
         visual visual = VisualTreeView.SelectedNode.Tag as visual;
         if (visual != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddModel);

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddLogic);
            mMenuItemAddLogic.Enabled = (visual.logic == null) ? true : false;  // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
         }

         // If model
         visualModel model = VisualTreeView.SelectedNode.Tag as visualModel;
         if (model != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddComponent);
            mMenuItemAddComponent.Enabled = (model.component == null) ? true : false;  // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddAnim);
            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If component
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;
         if (component != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddComponentAsset);
            mMenuItemAddComponentAsset.Enabled = (component.asset == null) ? true : false;   // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddLogic);
            mMenuItemAddLogic.Enabled = (component.logic == null) ? true : false;   // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddAttach);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddPoint);
            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If component asset
         visualModelComponentAsset componentAsset = VisualTreeView.SelectedNode.Tag as visualModelComponentAsset;
         if (componentAsset != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If component or anim attach
         visualModelComponentOrAnimAttach attach = VisualTreeView.SelectedNode.Tag as visualModelComponentOrAnimAttach;
         if (attach != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If component point
         visualModelComponentPoint compPoint = VisualTreeView.SelectedNode.Tag as visualModelComponentPoint;
         if (compPoint != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If logic
         visualLogic logic = VisualTreeView.SelectedNode.Tag as visualLogic;
         if (logic != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddLogicData);
            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If logic data
         visualLogicData logicData = VisualTreeView.SelectedNode.Tag as visualLogicData;
         if (logicData != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddComponentAsset);
            mMenuItemAddComponentAsset.Enabled = (logicData.asset == null) ? true : false;   // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddLogic);
            mMenuItemAddLogic.Enabled = (logicData.logic == null) ? true : false;   // Enable / Disable menu

            mVisualTreeViewContextMenuStrip.Items.Add("-");

            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If anim
         visualModelAnim anim = VisualTreeView.SelectedNode.Tag as visualModelAnim;
         if (anim != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddAnimAsset);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddAttach);
            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If anim asset
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;
         if (animAsset != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemAddAnimTag);
            mVisualTreeViewContextMenuStrip.Items.Add("-");
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemPaste);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // If tag
         visualModelAnimAssetTag tag = VisualTreeView.SelectedNode.Tag as visualModelAnimAssetTag;
         if (tag != null)
         {
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCut);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemCopy);
            mVisualTreeViewContextMenuStrip.Items.Add(mMenuItemDelete);
         }

         // Set Cancel to false. 
         // It is optimized to true based on empty entry.
         e.Cancel = false;
      }

      void MenuItemAddModel_Click(object sender, EventArgs e)
      {
         visual visual = VisualTreeView.SelectedNode.Tag as visual;

         if (visual != null)
         {
            visualModel model = new visualModel();

            // Find a unique name for model by counting all the previous models
            int numModels = mVisualFile.model.Count;
            model.name = "model_" + (numModels + 1);


            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(visual, model);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddComponent_Click(object sender, EventArgs e)
      {
         visualModel model = VisualTreeView.SelectedNode.Tag as visualModel;

         if (model != null)
         {
            visualModelComponent component = new visualModelComponent();

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(model, component);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      /*
      void MenuItemAddAttach_Click(object sender, EventArgs e)
      {
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;

         if (component != null)
         {
            visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(component, attach);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else
         {
            visualModelAnim anim = VisualTreeView.SelectedNode.Tag as visualModelAnim;

            if (anim != null)
            {
               visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();

               // Add/Execute undo action
               UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(anim, attach);
               mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
            }
         }
      }
      */

      void AddAttachmentToSelectedNode(visualModelComponentOrAnimAttach attach)
      {
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;

         if (component != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(component, attach);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else
         {
            visualModelAnim anim = VisualTreeView.SelectedNode.Tag as visualModelAnim;

            if (anim != null)
            {
               // Add/Execute undo action
               UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(anim, attach);
               mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
            }
         }
      }

      void MenuItemAddAttachLight_Click(object sender, EventArgs e)
      {
         visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();
         attach.type = visualModelComponentOrAnimAttach.AttachType.LightFile;

         AddAttachmentToSelectedNode(attach);
      }

      void MenuItemAddAttachModel_Click(object sender, EventArgs e)
      {
         visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();
         attach.type = visualModelComponentOrAnimAttach.AttachType.ModelFile;

         AddAttachmentToSelectedNode(attach);
      }

      void MenuItemAddAttachModelRef_Click(object sender, EventArgs e)
      {
         visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();
         attach.type = visualModelComponentOrAnimAttach.AttachType.ModelRef;

         AddAttachmentToSelectedNode(attach);
      }

      void MenuItemAddAttachParticle_Click(object sender, EventArgs e)
      {
         visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();
         attach.type = visualModelComponentOrAnimAttach.AttachType.ParticleFile;

         AddAttachmentToSelectedNode(attach);
      }

      void MenuItemAddAttachTerrainEffect_Click(object sender, EventArgs e)
      {
         visualModelComponentOrAnimAttach attach = new visualModelComponentOrAnimAttach();
         attach.type = visualModelComponentOrAnimAttach.AttachType.TerrainEffect;

         AddAttachmentToSelectedNode(attach);
      }


      void AddLogicToSelectedNode(visualLogic logic)
      {
         visual visual = VisualTreeView.SelectedNode.Tag as visual;
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;
         visualLogicData logicDataNode = VisualTreeView.SelectedNode.Tag as visualLogicData;

         if (visual != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(visual, logic);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else if (component != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(component, logic);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else if (logicDataNode != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(logicDataNode, logic);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddLogicVariation_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.Variation;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicBuildingCompletion_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.BuildingCompletion;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicTech_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.Tech;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicSquadMode_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.SquadMode;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicImpactSize_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.ImpactSize;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicDestruction_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.Destruction;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicUserCiv_Click(object sender, EventArgs e)
      {
         // create new logic node
         visualLogic logic = new visualLogic();
         logic.type = visualLogic.LogicType.UserCiv;

         AddLogicToSelectedNode(logic);
      }

      void MenuItemAddLogicData_Click(object sender, EventArgs e)
      {
         visualLogic vLogic = VisualTreeView.SelectedNode.Tag as visualLogic;

         if (vLogic != null)
         {
            visualLogicData newLogicData = new visualLogicData();

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(vLogic, newLogicData);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddPoint_Click(object sender, EventArgs e)
      {
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;

         if (component != null)
         {
            visualModelComponentPoint point = new visualModelComponentPoint();

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(component, point);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void AddComponentAssetToSelectedNode(visualModelComponentAsset compAsset)
      {
         visualModelComponent component = VisualTreeView.SelectedNode.Tag as visualModelComponent;
         visualLogicData logicData = VisualTreeView.SelectedNode.Tag as visualLogicData;

         if (component != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(component, compAsset);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else if(logicData != null)
         {
            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(logicData, compAsset);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddComponentAssetModel_Click(object sender, EventArgs e)
      {
         visualModelComponentAsset compAsset = new visualModelComponentAsset();
         compAsset.type = visualModelComponentAsset.ComponentAssetType.Model;
         compAsset.file = "";

         AddComponentAssetToSelectedNode(compAsset);
      }

      void MenuItemAddComponentAssetParticle_Click(object sender, EventArgs e)
      {
         visualModelComponentAsset compAsset = new visualModelComponentAsset();
         compAsset.type = visualModelComponentAsset.ComponentAssetType.Particle;
         compAsset.file = "";

         AddComponentAssetToSelectedNode(compAsset);
      }

      void MenuItemAddComponentAssetLight_Click(object sender, EventArgs e)
      {
         visualModelComponentAsset compAsset = new visualModelComponentAsset();
         compAsset.type = visualModelComponentAsset.ComponentAssetType.Light;
         compAsset.file = "";

         AddComponentAssetToSelectedNode(compAsset);
      }

      void MenuItemAddAnim_Click(object sender, EventArgs e)
      {
         visualModel model = VisualTreeView.SelectedNode.Tag as visualModel;

         if (model != null)
         {
            visualModelAnim anim = new visualModelAnim();
            visualModelAnimAsset animAsset = new visualModelAnimAsset();
            anim.asset.Add(animAsset);

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(model, anim);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAsset_Click(object sender, EventArgs e)
      {
         visualModelAnim anim = VisualTreeView.SelectedNode.Tag as visualModelAnim;

         if (anim != null)
         {
            visualModelAnimAsset animAsset = new visualModelAnimAsset();

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(anim, animAsset);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }


      void MenuItemAddAnimAssetTagAttack_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.Attack;

            // Use position of time slider if the control is paused
            if(m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal) m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagSound_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.Sound;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagParticle_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.Particle;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagTerrainEffect_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.TerrainEffect;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagUVOffset_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.UVOffset;
            animAssetTag.userData = "";

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }
      

      void MenuItemAddAnimAssetTagLight_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.Light;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagCameraShake_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.CameraShake;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }


      void MenuItemAddAnimAssetTagGroundIK_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.GroundIK;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }


      void MenuItemAddAnimAssetTagAttachTarget_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.AttachTarget;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }


      void MenuItemAddAnimAssetTagSweetSpot_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.SweetSpot;
            animAssetTag.end = 1.0M;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagBuildingDecal_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.BuildingDecal;
            animAssetTag.userData = "";

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagTerrainAlpha_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.TerrainAlpha;
            animAssetTag.userData = "";

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagRumble_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.Rumble;
            animAssetTag.userData = "";

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemAddAnimAssetTagKillAndThrow_Click(object sender, EventArgs e)
      {
          visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

          if (animAsset != null)
          {
              visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
              animAssetTag.type = visualModelAnimAssetTag.TagType.KillAndThrow;

              // Use position of time slider if the control is paused
              if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
              {
                  animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
              }

              // Add/Execute undo action
              UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
              mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
          }
      }

      void MenuItemAddAnimAssetTagPhysicsImpulse_Click(object sender, EventArgs e)
      {
         visualModelAnimAsset animAsset = VisualTreeView.SelectedNode.Tag as visualModelAnimAsset;

         if (animAsset != null)
         {
            visualModelAnimAssetTag animAssetTag = new visualModelAnimAssetTag();
            animAssetTag.type = visualModelAnimAssetTag.TagType.PhysicsImpulse;

            // Use position of time slider if the control is paused
            if (m_animationControl.getState() == AnimationControl.AnimControlStateEnum.ePaused)
            {
               animAssetTag.position = (decimal)m_animationControl.getNormalizedTime();
            }

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(animAsset, animAssetTag);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void MenuItemCut_Click(object sender, EventArgs e)
      {
         CopySelectedNode();
         DeleteSelectedNode();
      }

      void MenuItemDelete_Click(object sender, EventArgs e)
      {
         DeleteSelectedNode();
      }

      void MenuItemCopy_Click(object sender, EventArgs e)
      {
         CopySelectedNode();
      }

      void MenuItemPaste_Click(object sender, EventArgs e)
      {
         PasteOnSelectedNode();
      }


      void DeleteSelectedNode()
      {
         if (VisualTreeView.SelectedNode == null)
            return;

         TreeNode parentNode = VisualTreeView.SelectedNode.Parent;
         TreeNode node = VisualTreeView.SelectedNode;

         if ((node != null) && (parentNode != null))
         {
            XMLVisualNode xmlNode = node.Tag as XMLVisualNode;
            XMLVisualNode xmlParentNode = parentNode.Tag as XMLVisualNode;

            // Add an undo action
            UndoRedoDeleteNodeAction undoAction = new UndoRedoDeleteNodeAction(xmlParentNode, xmlNode);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
      }

      void CopySelectedNode()
      {
         // exit if nothing is selected
         if (VisualTreeView.SelectedNode == null)
            return;

         XMLVisualNode parentNode = (XMLVisualNode)VisualTreeView.SelectedNode.Tag;

         // clear copy
         s_copyNode = null;
         s_copyNode = parentNode.clone(true);
      }

      void PasteOnSelectedNode()
      {
         // exit if nothing is selected
         if (VisualTreeView.SelectedNode == null)
            return;

         XMLVisualNode parentNode = (XMLVisualNode)VisualTreeView.SelectedNode.Tag;

         // Check if node can be added
         bool okayToAdd = parentNode.canAdd(s_copyNode);

         if (okayToAdd)
         {
            // We must copy the s_copyNode, since it might get pasted multiple times and we want different
            // objects and not the same node every time.
            XMLVisualNode copyOfCopyNode = s_copyNode.clone(true);

            // Add/Execute undo action
            UndoRedoAddNodeAction undoAction = new UndoRedoAddNodeAction(parentNode, copyOfCopyNode);
            mUndoRedoManager.addUndoRedoActionAndExecute(undoAction);
         }
         else
         {
            if (MessageBox.Show("The copied node can not be pasted into the selected node.  Select a different node.", "Warning", MessageBoxButtons.OK) == DialogResult.OK)
            {
               return;
            }
         }
      }


      void MenuItemShowModel_Click(object sender, EventArgs e)
      {
         m_bShowModel = !(m_bShowModel);
         mMenuItemShowModel.CheckState = m_bShowModel ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowPlayerColor_Click(object sender, EventArgs e)
      {
         m_bShowPlayerColor = !(m_bShowPlayerColor);
         mMenuItemShowPlayerColor.CheckState = m_bShowPlayerColor ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowSkeleton_Click(object sender, EventArgs e)
      {
         m_bShowSkeleton = !(m_bShowSkeleton);
         mMenuItemShowSkeleton.CheckState = m_bShowSkeleton ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowBoneAxis_Click(object sender, EventArgs e)
      {
         m_bShowBoneAxis = !(m_bShowBoneAxis);
         mMenuItemShowBoneAxis.CheckState = m_bShowBoneAxis ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowBoneNames_Click(object sender, EventArgs e)
      {
         m_bShowBoneNames = !(m_bShowBoneNames);
         mMenuItemShowBoneNames.CheckState = m_bShowBoneNames ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowBoundingBox_Click(object sender, EventArgs e)
      {
         m_bShowBoundingBox = !(m_bShowBoundingBox);
         mMenuItemShowBoundingBox.CheckState = m_bShowBoundingBox ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowMeshBoxes_Click(object sender, EventArgs e)
      {
         m_bShowMeshBoxes = !(m_bShowMeshBoxes);
         mMenuItemShowMeshBoxes.CheckState = m_bShowMeshBoxes ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowStatsInfo_Click(object sender, EventArgs e)
      {
         m_bShowStatsInfo = !(m_bShowStatsInfo);
         mMenuItemShowStatsInfo.CheckState = m_bShowStatsInfo ? CheckState.Checked : CheckState.Unchecked;
      }

      void MenuItemShowGrid_Click(object sender, EventArgs e)
      {
         m_bShowGrid = !(m_bShowGrid);
         mMenuItemShowGrid.CheckState = m_bShowGrid ? CheckState.Checked : CheckState.Unchecked;
      }

      #endregion

      private void NewButton_Click(object sender, EventArgs e)
      {
         NewVisual();
      }

      private void OpenButton_Click(object sender, EventArgs e)
      {
         open_file();
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         save_file();
      }

      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         save_file_as();
      }

      private void CollapseAllButton_Click(object sender, EventArgs e)
      {
         VisualTreeView.CollapseAll();

         // Select top node
         if (VisualTreeView.Nodes.Count > 0)
         {
            VisualTreeView.SelectedNode = VisualTreeView.Nodes[0];
            VisualTreeView.Select();
         }
      }

      private void ExpandAllButton_Click(object sender, EventArgs e)
      {
         VisualTreeView.ExpandAll();
         VisualTreeView.Select();
      }

      private void CollapseOneLevelButton_Click(object sender, EventArgs e)
      {
         if (VisualTreeView.SelectedNode != null)
         {
            // Get max depth
            int maxDepth = getTreeDepthRecursive(VisualTreeView.SelectedNode, 0);

            // Collapse everything at the given depth
            collapseAtDepthRecursive(VisualTreeView.SelectedNode, 0, (maxDepth - 1));
         }
         else
         {
            // Get max depth
            int maxDepth = -1;
            foreach (TreeNode node in VisualTreeView.Nodes)
            {
               int newDepth = getTreeDepthRecursive(node, 0);
               if (newDepth > maxDepth)
                  maxDepth = newDepth;
            }

            // Collapse everything at the given depth
            foreach (TreeNode node in VisualTreeView.Nodes)
            {
               collapseAtDepthRecursive(node, 0, (maxDepth - 1));
            }
         }
         VisualTreeView.Select();
      }

      private void ExpandOneLeveButton_Click(object sender, EventArgs e)
      {
         if (VisualTreeView.SelectedNode != null)
         {
            expandedOneLevelRecursive(VisualTreeView.SelectedNode);
         }
         else
         {
            foreach (TreeNode node in VisualTreeView.Nodes)
               expandedOneLevelRecursive(node);
         }
         VisualTreeView.Select();
      }

      private void UndoButton_Click(object sender, EventArgs e)
      {
         mUndoRedoManager.undo();
      }

      private void RedoButton_Click(object sender, EventArgs e)
      {
         mUndoRedoManager.redo();
      }

      private void undoToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mUndoRedoManager.undo();
      }

      private void redoToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mUndoRedoManager.redo();
      }

      private void cutToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (VisualTreeView.Focused)
         {
            CopySelectedNode();
            DeleteSelectedNode();
         }
      }

      private void copyToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (VisualTreeView.Focused)
            CopySelectedNode();
      }

      private void pasteToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (VisualTreeView.Focused)
            PasteOnSelectedNode();
      }
      
      private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if(VisualTreeView.Focused)
            DeleteSelectedNode();
      }
      
      private void panel2_Resize(object sender, EventArgs e)
      {
         if (CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() == this)
         {
            CoreGlobals.getEditorMain().mIGUI.deviceResize(panel2.Width, panel2.Height, false);
            Activate();
         }
      }

      public TreeView getVisualTreeView()
      {
         return (VisualTreeView);
      }

      private void panel2_MouseClick(object sender, MouseEventArgs e)
      {
         if (e.Button == MouseButtons.Right)
         {
            mViewportContextMenuStrip.Show(panel2, e.X, e.Y);
         }
      }

      private void PlayButton_Click(object sender, EventArgs e)
      {
         m_animationControl.toggleState();
      }

      private void RewindButton_Click(object sender, EventArgs e)
      {
         m_animationControl.setStartTime();
      }

      private void ForwardButton_Click(object sender, EventArgs e)
      {
         m_animationControl.setEndTime();
      }

      private void AnimationTimeTrackBar_Scroll(object sender, EventArgs e)
      {
         float trackValue = (AnimationTimeTrackBar.Value / 1000.0f);

         m_animationControl.setNormalizedTime(trackValue);

         m_animationControl.setState(AnimationControl.AnimControlStateEnum.ePaused);
      }

      private void SpeedButton_Click(object sender, EventArgs e)
      {
         m_animationControl.changeSpeed();
      }
      
      private void RepeatButton_Click(object sender, EventArgs e)
      {
         m_animationControl.toggleLoopMode();
      }
      
      private void SoundButton_Click(object sender, EventArgs e)
      {
         m_animationControl.toggleSound();
      }

      private void resortItemsToolStripMenuItem_Click(object sender, EventArgs e)
      {
         mVisualFile.resort();
         CreateTreeView();
      }

      private void MotionButton_Click(object sender, EventArgs e)
      {
         m_animationControl.toggleMotion();
      }

      private void QuickViewButton_Click(object sender, EventArgs e)
      {
         // Early out if not currently editing anything
         if (mVisualFile == null)
            return;

         // Save visual to quickview.vis
         WriteVisual(CoreGlobals.getWorkPaths().mGameArtDirectory + @"\system\quickview.vis");

         if (!s_bVisualQuickViewClicked)
         {
            if (XFSInterface.launchApp())
            {
               XFSInterface.launchGame();

               XFSInterface.launchVisual("sys_quickview");

               s_bVisualQuickViewClicked = true;
            }
         }
      }
   }

   #region Animation Control

   public partial class AnimationControl
   {
      public enum AnimControlStateEnum
      {
         eInactive = 0,
         ePlaying,
         ePaused,
      }

      public enum AnimControlLoopModeEnum
      {
         eRepeat = 0,
         ePlayOnce
      }

      public enum AnimControlSpeedEnum
      {
         eOneFourth = 0,
         eOneHalf,
         eOne,
         eTwo,
         eFour
      }

      GrannyInstance m_curGrannyInstance = null;
      GrannyAnimation m_curGrannyAnimation = null;
      visualModelAnimAsset m_curAnimationAssetNode = null;

      AnimControlLoopModeEnum    m_animationLoopMode = AnimControlLoopModeEnum.eRepeat;
      AnimControlStateEnum       m_animationState = AnimControlStateEnum.eInactive;
      AnimControlSpeedEnum       m_animationSpeed = AnimControlSpeedEnum.eOne;
      float                      m_animationTime = 0.0f;
      float                      m_animationAppliedDelta = 0.0f;
      float                      m_animationDuration = 1.0f;
      bool                       m_resetWorldMatrix = true;

      bool                       m_soundEnabled = true;

      bool                       m_motionEnabled = true;

      float cFloatCompareEpsilon = 0.000001f;



      public AnimControlStateEnum getState()
      {
         return (m_animationState);
      }

      public void setState(AnimControlStateEnum state)
      {
         m_animationState = state;
      }

      public void toggleState()
      {
         switch(m_animationState)
         {
            case AnimControlStateEnum.ePlaying:
               setState(AnimControlStateEnum.ePaused);
               CoreGlobals.getSoundManager().stopAllSounds();
               break;
            case AnimControlStateEnum.ePaused:
               setState(AnimControlStateEnum.ePlaying);
               break;
         }
      }

      public AnimControlLoopModeEnum getLoopMode()
      {
         return (m_animationLoopMode);
      }

      public void setLoopMode(AnimControlLoopModeEnum state)
      {
         m_animationLoopMode = state;
      }

      public void toggleLoopMode()
      {
         switch (m_animationLoopMode)
         {
            case AnimControlLoopModeEnum.ePlayOnce:
               setLoopMode(AnimControlLoopModeEnum.eRepeat);
               break;
            case AnimControlLoopModeEnum.eRepeat:
               setLoopMode(AnimControlLoopModeEnum.ePlayOnce);
               break;
         }
      }

      public void toggleSound()
      {
         m_soundEnabled = !m_soundEnabled;
      }

      public void toggleMotion()
      {
         m_motionEnabled = !m_motionEnabled;
      }

      public bool isSoundEnabled()
      {
         return (m_soundEnabled);
      }

      public bool isMotionEnabled()
      {
         return (m_motionEnabled);
      }

      public AnimControlSpeedEnum getSpeed()
      {
         return (m_animationSpeed);
      }

      public void setSpeed(AnimControlSpeedEnum state)
      {
         m_animationSpeed = state;
      }

      public void changeSpeed()
      {
         switch (m_animationSpeed)
         {
            case AnimControlSpeedEnum.eOneFourth:
               setSpeed(AnimControlSpeedEnum.eOneHalf);
               break;
            case AnimControlSpeedEnum.eOneHalf:
               setSpeed(AnimControlSpeedEnum.eOne);
               break;
            case AnimControlSpeedEnum.eOne:
               setSpeed(AnimControlSpeedEnum.eTwo);
               break;
            case AnimControlSpeedEnum.eTwo:
               setSpeed(AnimControlSpeedEnum.eFour);
               break;
            case AnimControlSpeedEnum.eFour:
               setSpeed(AnimControlSpeedEnum.eOneFourth);
               break;
         }
      }

      public float getSpeedFactor()
      {
         switch (m_animationSpeed)
         {
            case AnimControlSpeedEnum.eOneFourth:
               return (1.0f / 4.0f);
            case AnimControlSpeedEnum.eOneHalf:
               return (1.0f / 2.0f);
            case AnimControlSpeedEnum.eOne:
               return (1.0f);
            case AnimControlSpeedEnum.eTwo:
               return (2.0f);
            case AnimControlSpeedEnum.eFour:
               return (4.0f);
         }
         return (1.0f);
      }
      

      public float getNormalizedTime()
      {
         return(m_animationTime / m_animationDuration);
      }

      public void setNormalizedTime(float normalizedTime)
      {
         m_animationTime = normalizedTime * m_animationDuration;
         m_animationAppliedDelta = m_animationTime;
         m_resetWorldMatrix = true;
      }

      public float getTime()
      {
         return(m_animationTime);
      }

      public void setTime(float time)
      {
         m_animationTime = time;
         m_animationAppliedDelta = m_animationTime;
         m_resetWorldMatrix = true;
      }

      public float getDuration()
      {

         return (m_animationDuration);
      }

      public void setStartTime()
      {
         m_animationTime = 0.0f;
         m_animationAppliedDelta = m_animationTime;
         m_resetWorldMatrix = true;
      }
      
      public void setEndTime()
      {
         m_animationTime = m_animationDuration;
         m_animationAppliedDelta = m_animationTime;
         m_resetWorldMatrix = true;
      }
      

      public void update(float dt)
      {
         if (m_animationState == AnimControlStateEnum.ePlaying)
         {
            float oldNormalizedTime, newNormalizedTime;

            oldNormalizedTime = getNormalizedTime();
            m_animationAppliedDelta = dt * getSpeedFactor();
            m_animationTime += m_animationAppliedDelta;
            newNormalizedTime = getNormalizedTime();

            // Play sound tags
            if (m_soundEnabled && (m_animationSpeed == AnimControlSpeedEnum.eOne))
            {
               if (m_curAnimationAssetNode != null)
               {
                  foreach (visualModelAnimAssetTag animTag in m_curAnimationAssetNode.tag)
                  {

                     if(((float)animTag.position<cFloatCompareEpsilon && oldNormalizedTime<cFloatCompareEpsilon && newNormalizedTime>=cFloatCompareEpsilon) ||
                        ((float)animTag.position > oldNormalizedTime && (float)animTag.position <= newNormalizedTime))
                     {
                        switch (animTag.type)
                        {
                           case visualModelAnimAssetTag.TagType.Sound:
                              {
                                 // Play sound here
                                 CoreGlobals.getSoundManager().playSound(animTag.name);
                                 break;
                              }
                        }
                     }
                  }
               }
            }


            if (m_animationTime > m_animationDuration)
            {
               m_resetWorldMatrix = true;

               switch (m_animationLoopMode)
               {
                  case AnimControlLoopModeEnum.eRepeat:
                     m_animationTime = 0.0f;
                     break;

                  case AnimControlLoopModeEnum.ePlayOnce:
                     m_animationTime = 0.0f;
                     setState(AnimControlStateEnum.ePaused);
                     break;
               }

               m_animationAppliedDelta = m_animationTime;
            }

         }
      }

      public void apply(ref Matrix worldMatrix)
      {
         if (m_curGrannyInstance != null)
         {
            m_curGrannyInstance.setClock(m_animationTime);

            if (m_resetWorldMatrix)
            {
               if (isMotionEnabled())
                  m_curGrannyAnimation.applyInitialPlacement(ref worldMatrix);
               else
                  worldMatrix = Matrix.Identity;

               m_resetWorldMatrix = false;
            }

            if (isMotionEnabled())
            {
               m_curGrannyInstance.updateMotion(m_animationAppliedDelta, ref worldMatrix);
               BRenderDevice.getDevice().SetTransform(TransformType.World, worldMatrix);
            }

            m_animationAppliedDelta = 0.0f;
         }
      }



      public void setAnimationParams(GrannyInstance granInstance, int animationId, visualModelAnimAsset animAsset)
      {
         // stop current animation if any
         if ((m_curGrannyInstance != null) && (m_curGrannyAnimation != null))
         {
            m_curGrannyInstance.stopAnimations(0.0f);
            m_curGrannyInstance.setClock(float.MaxValue);   // set large time to kill all anims
         }

         m_curGrannyInstance = granInstance;
         m_curGrannyAnimation = GrannyManager2.getAnimation(animationId);
         m_curAnimationAssetNode = animAsset;

         if((m_curGrannyInstance == null) || (m_curGrannyAnimation == null))
         {
            m_curGrannyInstance = null;
            m_curGrannyAnimation = null;
            m_curAnimationAssetNode = null;

            setState(AnimControlStateEnum.eInactive);
            return;
         }

         // Get duration
         m_animationDuration = m_curGrannyAnimation.getDuration();
         
         m_curGrannyInstance.playAnimation(animationId);

         // Reset clock
         setTime(0.0f);

         // Start animation
         setState(AnimControlStateEnum.ePlaying);
      }
   }

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
