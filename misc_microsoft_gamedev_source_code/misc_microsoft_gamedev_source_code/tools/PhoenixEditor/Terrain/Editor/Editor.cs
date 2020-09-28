using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System;

using Rendering;
using EditorCore;
using SimEditor;

////


//------------------------------------------
namespace Terrain
{
   #region TerrainEditor

   public struct CursorVert
   {
      public Vector3 xyz;
      public void SetValues(float x, float y, float z)
      {
         xyz.X = x;
         xyz.Y = y;
         xyz.Z = z;
      }
   }

   public struct CursorColorVert
   {
      public Vector3 xyz;
      public uint color;
      public void SetValues(float x, float y, float z, uint vertColor)
      {
         xyz.X = x;
         xyz.Y = y;
         xyz.Z = z;
         color = vertColor;
      }
   }

   public struct CursorSpriteVert
   {
      public Vector4 xyzw;
      public Vector2 uv;
   }



   // //make generic? .. link constraints with alternate constructor?
   public class ConstrainedValue
   {
      public float mMin = 0f;
      public float mMax = 1f;
      private float mValue = 0.5f;

      public ConstrainedValue(float min, float max, float val)
      {
         mMin = min;
         mMax = max;
         mValue = val;
      }
      public float Value
      {
         get
         {
            return mValue;
         }
         set
         {
            if ((value < mMax) && (value > mMin))
            {
               mValue = value;
            }
         }
      }

   }

   public class Modifier
   {
      public string mName = "";
      public ConstrainedValue mMin = new ConstrainedValue(0f, 5f, 0.1f);
      public ConstrainedValue mMax = new ConstrainedValue(0f, 5f, 1.5f);
      public int mInputType = 0;

      public const int cNone = 0;
      public const int cPressure = 1;
      public const int cXTilt = 2;
      public const int cYTilt = 3;

      public Modifier(string name)
      {
         mName = name;
         TerrainGlobals.getTerrainFrontEnd().RegisterModifier(this);
      }

      public float getCurrent()
      {
         float input = 1;
         if (mInputType == 1)
         {

            input = UIManager.currentPressure();
         }
         if (mInputType != 0)
            input = (mMin.Value) + input * (mMax.Value - mMin.Value);
         return input;
      }
   }

   public class BrushInfo
   {
      public enum eIntersectionShape
      {
         Sphere = 0,
         Cylinder,
      }

      private float mBaseRadius;
      private float mBaseHotspot;
      private float mBaseIntensity;
      private int mBaseCurveType = 0;
      private float mBaseRotation;
      private eIntersectionShape mBaseIntersectionShape = eIntersectionShape.Sphere;

      private int mRotateCount = 0;

      public Modifier mRadiusModifier = new Modifier("Radius");
      public Modifier mHotspotModifier = new Modifier("Hotspot");
      public Modifier mIntensityModifier = new Modifier("Intensity");

      public float mRadius
      {
         set
         {
            mBaseRadius = value;
         }
         get
         {
            return mBaseRadius * mRadiusModifier.getCurrent();
         }

      }
      public float mHotspot
      {
         set
         {
            mBaseHotspot = value;
         }
         get
         {
            return mBaseHotspot * mHotspotModifier.getCurrent();
         }
      }
      public float mIntensity
      {
         set
         {
            mBaseIntensity = value;
         }
         get
         {
            return mBaseIntensity * mIntensityModifier.getCurrent();
         }
      }
      public int mCurveType
      {
         set
         {
            mBaseCurveType = value;
         }
         get
         {
            return mBaseCurveType;
         }
      }
      public float mRotation
      {
         set
         {
            mBaseRotation = value;
         }
         get
         {
            return mBaseRotation;
         }
      }
      public eIntersectionShape mIntersectionShape
      {
         set
         {
            mBaseIntersectionShape = value;
         }
         get
         {
            return mBaseIntersectionShape;
         }
      }

      public int RotateCount
      {
         set
         {
            mRotateCount = value;
         }
         get
         {
            return mRotateCount;
         }
      }

   }

   public class BTerrainEditor
   {
      public BTerrainEditor()
      {
         muScale = 2;
         mvScale = 2;
         mMaskScalar = 1f;
         mCurrMode = eEditorMode.cModeNone;
         mRenderMode = eEditorRenderMode.cRenderFull;
         mBrushShape = eBrushShape.cCircleBrush;
         mCurrBrush = null;
         mBrushInfo.mRadius = 3.0f;
         mBrushInfo.mHotspot = 0.5f;
         mBrushInfo.mIntensity = 0.19f;
         mDrawQuadNodeBounds = false;
         mDetailPoints = null;
         mNormals = null;
         mSkirtHeights = null;
         mAmbientOcclusionValues = null;

         mSimRep = null;
  
         if(File.Exists(CoreGlobals.getWorkPaths().mEditorSettings + "\\KillUndo.txt"))
         {
            mbVertexUndo = false;
            mbTextureUndo = false;

         }
      }
      ~BTerrainEditor()
      {
         destroy();
      }


      #region cutt & Paste
      public bool mUseClipartVertexData = true;
      public bool mUseClipartTextureData = true;
      public bool mUseClipartSimData = true;
      public bool mTransformClipartValues = false;

      CopiedVertexData mVertexData = new CopiedVertexData();
      CopiedTextureData mCopiedTextureData = new CopiedTextureData();
      CopiedUnitData mCopiedUnitData = new CopiedUnitData();

      public ClipArtData mClipArtData = new ClipArtData();

      public void CopySelected()
      {
         CopySelected(true);
      }

      public void CopySelected(bool includeSimData)
      {
         mClipArtData = new ClipArtData();
         mVertexData = mClipArtData.getData("CopiedVertexData") as CopiedVertexData;
         mVertexData.MakeCopy(this);

         mCopiedTextureData = mClipArtData.getData("CopiedTextureData") as CopiedTextureData;
         mCopiedTextureData.copySelectedData(this);

         if (includeSimData)
         {
            mCopiedUnitData = mClipArtData.getData("CopiedUnitData") as CopiedUnitData;
            mCopiedUnitData.copySelectedData(this);
         }

         if (mUseClipartVertexData) mVertexData.transformData(1, 1, 1, 0);
         if (mUseClipartTextureData) mCopiedTextureData.transformData(1, 1, 1, 0);
         if (mUseClipartSimData && includeSimData) mCopiedUnitData.transformData(1, 1, 1, 0);
      }


      public void TransformClipart(float xscale, float yscale, float zscale, float rotation)
      {
         if (mUseClipartVertexData && mVertexData.HasData()) mVertexData.transformData(xscale, yscale, zscale, rotation);
         if (mUseClipartTextureData && mCopiedTextureData.HasData()) mCopiedTextureData.transformData(xscale, yscale, zscale, rotation);
         if (mUseClipartSimData && mCopiedUnitData.HasData()) mCopiedUnitData.transformData(xscale, yscale, zscale, rotation);

         mLastTerrainPasteRotation = rotation;
         mLastTerrainScaleX = xscale;
         mLastTerrainScaleY = yscale;
         mLastTerrainScaleZ = zscale;
      }


      //almost works but needs to apply deformations and update...
      public void CutSelected(bool upperCut)
      {
         CopySelected(false);

         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         TerrainGlobals.getEditor().PushSelectedDetailPoints(true);
         BTileBoundingBox bounds = mVertexData.DirectCut(this, upperCut);
         TerrainGlobals.getEditor().PushSelectedDetailPoints(false);

         updateTerrainVisuals(true);

#if whydothisshit
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, bounds.minX, bounds.maxX, bounds.minZ, bounds.maxZ);


         TerrainGlobals.getEditor().extendCurrBrushDeformation(bounds.minX, bounds.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(bounds.maxX, bounds.maxZ);


         //ClearVis();

         TerrainGlobals.getTerrain().computeBasisCurr(bounds.minX, bounds.maxX, bounds.minZ, bounds.maxZ);

         for (int i = 0; i < nodes.Count; i++)
         {
            nodes[i].mDirty = true;
            BTerrainQuadNodeDesc desc = nodes[i].getDesc();
            SimGlobals.getSimMain().updateHeightsFromTerrain(desc.m_min, desc.m_max);
         }
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         //TerrainGlobals.getTerrain().rebuildDirty(BRenderDevice.getDevice());

         TerrainGlobals.getEditor().flushBrushDeformations();


#endif
         //TerrainGlobals.getTerrain()
         return; //This will act the same as copy until the code below is working          
#if false
          Dictionary<long, float>.Enumerator it = mCurrSelectionMaskHash.GetEnumerator();

          Vector3 v = new Vector3(0, 0, 0);

          int stride = TerrainGlobals.getTerrain().getNumZVerts();
          long id;
          while (it.MoveNext() == true)
          {
             id = it.Current.Key;
             int x = (int)(id / stride);
             int z = (int)(id - x * stride);
             float selectionWeight = it.Current.Value;

             //alternate?
             
             if (selectionWeight == 0.0f)
                continue;

             mCurrBrushDeformationsHash[id] = (v) * selectionWeight;

             //extendCurrBrushDeformation(x, z);
          }
          
          ClearVis();
#endif

      }
      public void SetClipArt(ClipArtData data)
      {
         mClipArtData = data;
         mVertexData = mClipArtData.getData("CopiedVertexData") as CopiedVertexData;
         mCopiedTextureData = mClipArtData.getData("CopiedTextureData") as CopiedTextureData;
         mCopiedUnitData = mClipArtData.getData("CopiedUnitData") as CopiedUnitData;
      }


      bool mbPasteModeDirty = false;
      public void SetPasteDirty()
      {
         mbPasteModeDirty = true;
         //mbPendingPasteDeformations = true;

         
      }
      bool mbCancelPaste = false;
      public void CancelPaste()
      {
         mbCancelPaste = true;
         mbPendingPasteDeformations = false;
      }
      bool mbOKPaste = false;
      public void OKPaste()
      {
         mbOKPaste = true;
         mbPendingPasteDeformations = false;
      }

      BTerrainFrontend.ePasteOperation mPasteOperation = BTerrainFrontend.ePasteOperation.Add;
      public void setPasteOperation(BTerrainFrontend.ePasteOperation op)
      {
         mPasteOperation = op;
      }

      BTileBoundingBox mPastedDataPreviewBounds = null;
      public int mLastTerrainPasteLocationX = 0;
      public int mLastTerrainPasteLocationZ = 0;
      public float mLastTerrainOffsetY = 0;
      public float mLastTerrainPasteRotation = 0;
      public float mLastTerrainScaleX = 1;
      public float mLastTerrainScaleY = 1;
      public float mLastTerrainScaleZ = 1;
      public bool mbClipartBBOnly = false;

      BRenderDebugCube mPastePreviewBox = null;//= new BRenderDebugCube(new Vector3(-mRadius, -mRadius, -mRadius), new Vector3(mRadius, mRadius, mRadius), System.Drawing.Color.Yellow.ToArgb(), false);
      Matrix mPastePreviewBoxMatrix;
      public void setPastedDataPreview(int xtile, int ztile, float yOffset, float rotation, float xscale, float yscale, float zscale)
      {      

         if(mPastePreviewBox == null)
         {
            mPastePreviewBox = new BRenderDebugCube(new Vector3(-0.5f, -0.5f, -0.5f), new Vector3(0.5f, 0.5f, 0.5f), 0x00ff00, false);            
         }
         if (mUseClipartVertexData && mVertexData.HasData())
         {
            Vector3 min;
            Vector3 max;
            mVertexData.GetOriginalSize(out min, out max);
            mPastePreviewBox = new BRenderDebugCube(min, max, 0x00ff00, false);
         }        
         
         //mBrushIntersectionPoint
         //Matrix.RotationY()mLastTerrainPasteRotation         
         //Matrix.Scaling()
         //Matrix.Scaling(xscale,yscale,zscale) *

         mPastePreviewBoxMatrix = Matrix.Scaling(xscale, yscale, zscale) * Matrix.RotationY(rotation) * Matrix.Translation(new Vector3(xtile / 2f, yOffset, ztile / 2f));

      }
      public CopiedVertexData GetClipartVertexData()
      {
         if (mVertexData != null && mVertexData.HasData())
         {
            return mVertexData;
         }
         return null;
      }

      public void drawPasteBB()
      {
         if (mPastePreviewBox != null)
         {
            BRenderDevice.getDevice().Transform.World = mPastePreviewBoxMatrix;
            mPastePreviewBox.render();
         }
      }

      public void applyPastedData(int x, int z, float y)
      {
         Vector3 intPoint = mBrushIntersectionPoint;
         Vector3 intNormal = mBrushIntersectionNormal;
         BTerrainQuadNode node = mBrushIntersectionNode;
         
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mCurrBrushExtends.minX, mCurrBrushExtends.maxX,
                                                                                       mCurrBrushExtends.minZ, mCurrBrushExtends.maxZ);
         TerrainGlobals.getEditor().clearBrushDeformations();

         // Clear undo
         mVertBackup.Clear();
         
         //int x = mVisTileIntetersectionX;
         //int z = mVisTileIntetersectionZ;
         mLastTerrainPasteLocationX = x;
         mLastTerrainPasteLocationZ = z;
         mLastTerrainOffsetY = y;

         // Undo info
         BTileBoundingBox boundsEstimate = this.mVertexData.getBounds(x, z);
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, boundsEstimate.minX, boundsEstimate.maxX, boundsEstimate.minZ, boundsEstimate.maxZ);

         int tx = 0;
         int tz = 0;

         if (mVertexData.HasData())
         {
            tx = mVertexData.mTransSizeX;
            tz = mVertexData.mTransSizeZ;

         }

         mVertexData.SetHeightModifier(mBrushInfo.mIntensity);
         if (mUseClipartVertexData && mVertexData.HasData())
         {
            mPastedDataPreviewBounds = this.mVertexData.Preview(this, x, z, y,  mBrushInfo.RotateCount, mPasteOperation);
            TerrainGlobals.getEditor().extendCurrBrushDeformation(mPastedDataPreviewBounds.minX, mPastedDataPreviewBounds.minZ);
            TerrainGlobals.getEditor().extendCurrBrushDeformation(mPastedDataPreviewBounds.maxX, mPastedDataPreviewBounds.maxZ); 
         }
         if (mUseClipartTextureData && mCopiedTextureData.HasData())
         {
            mCopiedTextureData.mTransSizeX = tx;
            mCopiedTextureData.mTransSizeZ = tz;

            mPastedDataPreviewBounds = mCopiedTextureData.Preview(this, x, z);
            TerrainGlobals.getEditor().extendCurrBrushDeformation(mPastedDataPreviewBounds.minX, mPastedDataPreviewBounds.minZ);
            TerrainGlobals.getEditor().extendCurrBrushDeformation(mPastedDataPreviewBounds.maxX, mPastedDataPreviewBounds.maxZ);
         }

         TerrainGlobals.getTerrain().computeBasisCurr(mCurrBrushExtends.minX, mCurrBrushExtends.maxX, mCurrBrushExtends.minZ, mCurrBrushExtends.maxZ);
      
         if (mUseClipartSimData && mCopiedUnitData.HasData())
         {
            mCopiedUnitData.mTransSizeX = tx;
            mCopiedUnitData.mTransSizeZ = tz;

            mCopiedUnitData.Preview(this, x, z);
         }
         else
         {
            SimGlobals.getSimMain().clipartClearObjects();
         }

         //update our affected quad nodes.
         for (int i = 0; i < nodes.Count; i++)
         {
            nodes[i].mDirty = true;
            BTerrainQuadNodeDesc desc = nodes[i].getDesc();
           // SimGlobals.getSimMain().updateHeightsFromTerrain(desc.m_min, desc.m_max);
            //nodes[i].getTextureData().free();
            for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
            {
               nodes[i].getTextureData(lod).free();
            }
         }
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
      }


      class ClipartUndoData : UndoInstanceData
      {
         public ClipartUndoData()
         {
            mMemorySize = 0;
         }
         ~ClipartUndoData()
         {
            mTexUndoData = null;
            mVertexUndoData = null;
         }
         public TextureUndoData mTexUndoData = null;
         public VertUndoData mVertexUndoData = null;
         //SimUndoData mSimUndoData;
      }
      public void clipartUndoCallback(UndoInstanceData pkt)
      {
         ClipartUndoData vud = (ClipartUndoData)pkt;
         if(vud.mTexUndoData!=null)
         {
            textureUndoCallback(vud.mTexUndoData);
         }
         if (vud.mVertexUndoData != null)
         {
            vertexUndoCallback(vud.mVertexUndoData);
         }
      }
      void addPasteTerrainUndoNodes(bool isCurrent)
      {
         if (!mbOKPaste)
            return;

         if (mCurrentUndoPacket == null)
         {
            mCurrentUndoPacket = new UndoPacket();
            mCurrentUndoPacket.mCallbackFunction = clipartUndoCallback;

            mCurrentUndoPacket.mPrevData = new ClipartUndoData();
            if (mUseClipartVertexData)
               ((ClipartUndoData)mCurrentUndoPacket.mPrevData).mVertexUndoData = new VertUndoData();
            if (mUseClipartTextureData)
               ((ClipartUndoData)mCurrentUndoPacket.mPrevData).mTexUndoData = new TextureUndoData();
            ((ClipartUndoData)mCurrentUndoPacket.mPrevData).mMemorySize = 0;

            mCurrentUndoPacket.mCurrData = new ClipartUndoData();
            if (mUseClipartVertexData)
               ((ClipartUndoData)mCurrentUndoPacket.mCurrData).mVertexUndoData = new VertUndoData();
            if (mUseClipartTextureData)
               ((ClipartUndoData)mCurrentUndoPacket.mCurrData).mTexUndoData = new TextureUndoData();
            ((ClipartUndoData)mCurrentUndoPacket.mCurrData).mMemorySize = 0;
         }


         ClipartUndoData vud = isCurrent ? (ClipartUndoData)mCurrentUndoPacket.mCurrData : (ClipartUndoData)mCurrentUndoPacket.mPrevData;

          List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
        TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mPastedDataPreviewBounds.minX, mPastedDataPreviewBounds.maxX,
                                                                                       mPastedDataPreviewBounds.minZ, mPastedDataPreviewBounds.maxZ);
        if (mUseClipartTextureData)
        {
           mbTextureUndo = true;
           addTextureUndoNodes(vud.mTexUndoData, nodes.ToArray(), isCurrent);
           mbTextureUndo = false;
        }
        if (mUseClipartVertexData)
        {
           mbVertexUndo = true;
           addVertexUndoNodes(vud.mVertexUndoData, nodes.ToArray(), isCurrent);
           mbVertexUndo = false;
        }
      }
      #endregion


      #region init Crate Destroy update render
      public void initFromMem(TerrainCreationParams param, Vector3[] dp, Byte[] av, float[] aov,
                           JaggedContainer<int> mSimBuildableOverrideValues,
                           JaggedContainer<int> mSimPassableOverrideValues,
                           JaggedContainer<float> mSimHeightOverrideValues,
                           JaggedContainer<byte> tessOverrideVals,
                           JaggedContainer<float> cameraHeightOverride,
                           JaggedContainer<float> flightHeightOverride,
                           JaggedContainer<int> mSimFloodPassableOverrideValues,
                           JaggedContainer<int> mSimScarabPassableOverrideValues,
                           JaggedContainer<int> mSimTileTypeOverrideValues)
      {

         init(param);
         dp.CopyTo(mDetailPoints, 0);
         av.CopyTo(mAlphaValues, 0);

         if (aov!=null)
            aov.CopyTo(mAmbientOcclusionValues, 0);

         mSimRep.getHeightRep().mRenderHeights = false;
         mSimRep.getDataTiles().createJaggedBuildableFrom(mSimBuildableOverrideValues);
         mSimRep.getDataTiles().createJaggedPassableFrom(mSimPassableOverrideValues);
         mSimRep.getDataTiles().createJaggedFloodPassableFrom(mSimFloodPassableOverrideValues);
         mSimRep.getDataTiles().createJaggedScarabPassableFrom(mSimScarabPassableOverrideValues);
         mSimRep.getHeightRep().createJaggedFrom(mSimHeightOverrideValues);
         mSimRep.getDataTiles().createJaggedTileTypeFrom(mSimTileTypeOverrideValues);

         mSimRep.getFlightRep().createJaggedFrom(flightHeightOverride);
         mSimRep.getFlightRep().mRenderHeights = false;

         createTessOverrideFrom(tessOverrideVals);
         
         mCameraHeightRep.createJaggedFrom(cameraHeightOverride);
         mCameraHeightRep.mRenderHeights = false;
      }
      public void init(TerrainCreationParams param)
      {
         int numVerts  = TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts();
         mDetailPoints = new Vector3[numVerts];
         mNormals = new Vector3[numVerts];
         mLightValues = new Vector3[numVerts];

         mSkirtHeights = new float[TerrainGlobals.getTerrain().getTotalSkirtXVerts() * TerrainGlobals.getTerrain().getTotalSkirtZVerts()];
         clearSkirtHeights();


         mAlphaValues = new byte[numVerts];
         for (int i = 0; i < numVerts; i++)
            mAlphaValues[i] = 255;
         

         mAmbientOcclusionValues = new float[TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts()];
         clearAmbientOcclusion();

         mCurrBrushDeformations = new JaggedContainer<Vector3>(MaskFactory.mMaxCapacity);
         mCurrBrushDeformationNormals = new JaggedContainer<Vector3>(MaskFactory.mMaxCapacity);
         mCurrBrushDeformationTextures = new JaggedContainer<BTerrainTextureVector>(MaskFactory.mMaxCapacity);

         mSimRep = new BTerrainSimRep();
         mSimRep.init(param.mNumSimXTiles, param.mNumSimZTiles, param.mSimTileSpacing, 1.0f / (float)param.mSimToVisMultiplier);
         mSimRep.getHeightRep().initHeightOverride();
         mSimRep.getFlightRep().initHeightOverride();
         mSimRep.getDataTiles().initPassableOverride();
         mSimRep.getDataTiles().initBuildableOverride();
         mSimRep.getDataTiles().initFloodPassableOverride();
         mSimRep.getDataTiles().initScarabPassableOverride();
         mSimRep.getDataTiles().initTileTypeOverride();
         

         mCameraHeightRep = new CameraHeightRep();
         mCameraHeightRep.init();
         mCameraHeightRep.initHeightOverride();

         mRealLOSRep = new RealLOSRep();
         mRealLOSRep.init();
         mRealLOSRep.initHeightOverride();

         initTesselationOverride();

         mTransWidget = new TranslationWidget();
         mTransWidget.init();

         mCurrMode = BTerrainEditor.eEditorMode.cModeNone;

         // create VB for 2D circle brush interface
         BTerrainVertexBrush.createCursor();
        
         BTerrainSimBrush.createCursor();
         BTerrainFlightBrush.createCursor();
         BTerrainFoliageBrush.createCursor();
         BTerrainCameraBrush.createCursor();
         createSelectionCameraSprite();

         Masking.clearSelectionMask();

         FoliageManager.init();
      }
      
      public void postCreate()
      {
         mSimRep.getHeightRep().recalculateHeights(false);
      }
      public void destroy()
      {
         cleanBrush();

         BTerrainVertexBrush.destroyCursor();
       
         BTerrainSimBrush.destroyCursor();
         BTerrainFoliageBrush.destroyCursor();
         BTerrainCameraBrush.destroyCursor();
         if (mTransWidget!=null)
         {
            mTransWidget.destroy();
            mTransWidget = null;
         }
         

         if (gSelectionCameraSpriteVB != null)
         {
            gSelectionCameraSpriteVB.Dispose();
            gSelectionCameraSpriteVB = null;
         }

         if (m2DSelectionBox != null)
         {
            m2DSelectionBox.Dispose();
            m2DSelectionBox = null;
         }


         if (mThumbnailTexture != null)
         {
            mThumbnailTexture.Dispose();
            mThumbnailTexture = null;
         }

         if (mDetailPoints != null)
         {
            mDetailPoints = null;
         }
         if (mNormals != null)
         {
            mNormals = null;
         }
         if (mSkirtHeights != null)
         {
            mSkirtHeights = null;
         }
         if (mAmbientOcclusionValues != null)
         {
            mAmbientOcclusionValues = null;
         }
         if (mdebugPrimBuffer != null)
         {
            mdebugPrimBuffer.Dispose();
            mdebugPrimBuffer = null;
         }
         if (mSimRep != null)
         {
            mSimRep.getHeightRep().destroyHeightOverride();
            mSimRep.getFlightRep().destroyHeightOverride();
            mSimRep.getDataTiles().destroyPassableOverride();
            mSimRep.getDataTiles().destroyBuildableOverride();
            mSimRep.getDataTiles().destroyFloodPassableOverride();
            mSimRep.getDataTiles().destroyScarabPassableOverride();
            mSimRep.getDataTiles().destroyTileTypeOverride();
            mSimRep = null;
         }
         if(mCameraHeightRep !=null)
         {
            mCameraHeightRep.destroy();
            mCameraHeightRep = null;
         }
         RoadManager.destroy();
         FoliageManager.destroy();

         destroyTesselationOverride();
         
      }

      public void update(float dt)
      {
         /*
          switch (mCurrMode)
          {
              case eEditorMode.cModeTexEdit:
                  updateTextureModeCursor();
                  break;
              case eEditorMode.cModeVertHeightEdit:
              case eEditorMode.cModeVertPushEdit:
              case eEditorMode.cModeVertAvgEdit:
              case eEditorMode.cModeVertStd:
              case eEditorMode.cModeVertStdDot:
              case eEditorMode.cModeVertInflat:
              case eEditorMode.cModeVertInflatDot:
              case eEditorMode.cModeVertLayer:
              case eEditorMode.cModeVertPinch:
              case eEditorMode.cModeVertSmooth:
              case eEditorMode.cModeVertUniform:
              case eEditorMode.cModeVertSetHeight:
              case eEditorMode.cModePasteMode:
                  updateVertModeCursor();
                  break;
              case eEditorMode.cModeControlEdit:

                  break;
              case eEditorMode.cModeNone:
              default:
                  break;
          };*/

         BTerrainQuadNode[] mNodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < mNodes.Length; i++)
            mNodes[i].update();
      }

      bool mbPendingPasteDeformations = false;
      bool mLeftMouseDown = false;
      bool mRightMouseDown = false;
      Point mReleasePoint = new Point();
      Point mPressPoint = new Point();
      public bool mbRightClickApply = false;

      public ComponentMaskSettings mComponentMaskSettings = new ComponentMaskSettings();
      void setNoMode()
      {
            if (mCurrMode == eEditorMode.cModePasteMode)
            {
               TerrainGlobals.getEditor().clearBrushDeformations();
            }

            setMode(eEditorMode.cModeNone);

            // clear selection mask
            Masking.clearSelectionMask();
            mTransWidget.clear();
            clearMaskLines();
            RoadManager.clearSelectedPoints();
      }
      public void input()
      {
         // Handle escape key first
         if ((UIManager.GetAsyncKeyStateB(Key.Escape)))
         {
            setNoMode();

            return;
         }

         // Handle F4 key
         //CLM [09.11.07] I HAVE REMOVED THIS!
         //if (UIManager.GetAsyncKeyStateB(Key.F4))
         //{
         //   Terrain.Controls.AOGenDialog dlg = new Terrain.Controls.AOGenDialog();
         //   dlg.Show();
         //   //if (MessageBox.Show("Compute AO?", "Compute AO?", MessageBoxButtons.OKCancel) == DialogResult.OK)
         //   //{
         //   //   TerrainGlobals.getEditor().computeAmbientOcclusion();
         //   //   // Rebuild visual geometry now that the ambient occlusion values have been updated
         //   //   TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         //   //}
         //}


         if (CoreGlobals.getEditorMain().MainMode != BEditorMain.eMainMode.cTerrain)
         {
            return;//nothing else to do for now
         }

         if (SimGlobals.getSimMain().isUsingDesignerControls() && (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
         {
            return;//we're using designer controls, which require mouse clicks to do their jobs.
         }


     
         //state data we'll use everywhere else
         bool left = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft);
         bool right = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight);
         bool bMouseMoved = UIManager.Moved(0);
         bool bClickedLeft = (!left && mLeftMouseDown);
         bool bClickedRight = (!right && mRightMouseDown);
         bool bDragging = (left && mLeftMouseDown);

        
         //DRAG SHAPE SELECT
         //if we're in select mode, don't do a great deal of intersections
         if ((mCurrMode == eEditorMode.cModeShapeSelect))
         {
            if (bClickedLeft)//we've released
            {
               UIManager.GetCursorPos(ref mReleasePoint);

               doSelectionFromShape(UIManager.GetAsyncKeyStateB(Key.LeftControl));

               mShowDragBox = false;
            }
            else if (left)
            {
               {
                  //have we clicked on a selected object for non-advanced movement mode
                  if (!mLeftMouseDown)
                  {
                     //if shift | alt isn't held, clear our selected points
                     if (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.LeftControl))
                        Masking.clearSelectionMask();

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

            //mark our current state for next frame
            mLeftMouseDown = left;
            mRightMouseDown = right;

            return;   // we don't need to input all the brush mouse input...
         }

         //VERTEX TRANSLATION WIDGET
         if (mCurrMode == eEditorMode.cModeVertWidgetTrans)
         {
            if (!mTransWidget.isLocked())
               mTransWidget.testIntersection();

            if (bClickedLeft)//we've released
            {
               if (mTransWidget.isLocked())
               {
                  mTransWidget.clear();
               }
               else
               {
                  UIManager.GetCursorPos(ref mReleasePoint);
                 // doSelection(UIManager.GetAsyncKeyStateB(Key.LeftControl));

               }
            }
            else if (left)
            {
               //translation widget
               if ((mTransWidget.isLocked() || mTransWidget.testIntersection()))
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
                        translateVerts();
                        UIManager.GetCursorPos(ref mPressPoint);
                     }
                  }
               }
               
            }
            //mark our current state for next frame
            mLeftMouseDown = left;
            mRightMouseDown = right;
            return;
         }

         //mark our current state for next frame
         mLeftMouseDown = left;
         mRightMouseDown = right;



         //INTERSECTIONS FOR TERRAIN OPERATIONS
         //get our view ray
         Vector3 orig = getRayPosFromMouseCoords(false);
         Vector3 dir = getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         //get our intersect point
         Vector3 intPoint = Vector3.Empty;
         if (TerrainGlobals.getTerrain().rayIntersects(ref orig, ref dir, ref intPoint, ref mBrushIntersectionNormal,ref mVisTileIntetersectionX,ref mVisTileIntetersectionZ, ref mBrushIntersectionNode, true))
         {
            // Store intersection point to use when drawing the 2D circle cross hair.
            mBrushIntersectionPoint = intPoint;
         }

         if (CoreGlobals.getEditorMain().mIGUI != null)
            CoreGlobals.getEditorMain().mIGUI.setCursorLocationLabel(mBrushIntersectionPoint.X, mBrushIntersectionPoint.Y, mBrushIntersectionPoint.Z);

         //ROAD MODE(needs terrain intersection data)
         if (mCurrMode == eEditorMode.cModeRoadEdit || mCurrMode == eEditorMode.cModeRoadAdd)
         {
            RoadManager.input(mCurrMode);
            return;
         }

         //may need to disable alt messing with the ui
         bool altDirection = UIManager.GetAsyncKeyStateB(Key.RightAlt) || UIManager.GetAsyncKeyStateB(Key.LeftAlt);
         bool selectionMode = UIManager.GetAsyncKeyStateB(Key.RightControl) || UIManager.GetAsyncKeyStateB(Key.LeftControl);


          //CUSTOM SHAPE SELECTION
         if (mCurrMode == eEditorMode.cModeCustomShape)
         {
            if(bClickedLeft || bClickedRight)
            {
               if(mMaskLines.Count==0)
               {
                  //we're starting!
                  if (mShapeStarted == false)
                  {
                     mShapeStarted = true;
                     mMaskLineStartPoint.X = mVisTileIntetersectionX;
                     mMaskLineStartPoint.Y = mVisTileIntetersectionZ;
                     mMaskLinePrevPoint = mMaskLineStartPoint;
                  }
                  else
                  {
                     Point b = new Point();
                     b.X = mVisTileIntetersectionX;
                     b.Y = mVisTileIntetersectionZ;
                     addLine(mMaskLineStartPoint, b, true);
                     mMaskLinePrevPoint = b;
                  }
               }
               else
               {
                  Point b = new Point();
                  b.X = mVisTileIntetersectionX;
                  b.Y = mVisTileIntetersectionZ;
                  addLine(mMaskLinePrevPoint, b, true);
                  mMaskLinePrevPoint = b;
                  if(maskLinesCloseShape())
                  {
                     fillMaskFromLines(bClickedLeft);
                  }
               }
            }

            return;
         }

         
         //TEXTURE SELECTION
         if (  (mCurrMode == eEditorMode.cModeTexEdit && right && !selectionMode)
            || (mCurrMode == eEditorMode.cModeTexErase && right && !selectionMode)
            || (mCurrMode == eEditorMode.cModeDecalEdit && right && !selectionMode))
         {
              selectTextureAtCursor();
            return;
         }
            

         //SELECTED DECAL INSTANCE SELECTION
         if (mCurrMode == eEditorMode.cModeDecalModify)
         {
            if (TerrainGlobals.getTexturing().getSelectedDecalInstanceCount() > 0)
            {
               //if mousewheel, add the rotation amount to our objects
               //if multiple selected, rotate around central position?
               if (UIManager.WheelDelta !=0)
               {
                  float amt = -((float)UIManager.WheelDelta / 50.0f) * 0.05f;
                  
                  if(UIManager.GetAsyncKeyStateB(Key.LeftShift))
                     TerrainGlobals.getTexturing().resizeSelectedDecals(amt,amt);
                  else
                     TerrainGlobals.getTexturing().rotateSelectedDecals(-amt);

                  TerrainGlobals.getTexturing().reloadCachedVisuals();
               }
               
            }

            //if we're dragging, then we're dragging...
            if( left  && !bDragging)
            {   
               {
                  if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                     TerrainGlobals.getTexturing().unselectAllDecalInstances();
                  selectDecalAtCursor();
               }
               return;
            }
            if (UIManager.GetAsyncKeyStateB(Key.Delete))
            {
               TerrainGlobals.getTexturing().removeSelectedDecalInstances();
               return; 
            }

            
         }
         
         //CLIPART PASTE TERRAIN
         if (mCurrMode == eEditorMode.cModePasteMode)
         {
            if (UIManager.WheelDelta != 0)
            {


               if (UIManager.GetAsyncKeyStateB(Key.LeftShift))
               {
                  float amt = ((float)UIManager.WheelDelta / 50.0f) * 0.05f;
                  mLastTerrainPasteRotation += amt;
                  if (mbClipartBBOnly == false)
                  {
                     if (mUseClipartVertexData) mVertexData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                     if (mUseClipartTextureData) mCopiedTextureData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                     if (mUseClipartSimData) mCopiedUnitData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                  }
               }
               else
               {
                  float amt = ((float)UIManager.WheelDelta / 50.0f) * 0.05f;
                  mLastTerrainScaleX += amt;
                  mLastTerrainScaleY += amt;
                  mLastTerrainScaleZ += amt;
                  if (mbClipartBBOnly == false)
                  {
                     if (mUseClipartVertexData) mVertexData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                     if (mUseClipartTextureData) mCopiedTextureData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                     if (mUseClipartSimData) mCopiedUnitData.transformData(mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ, mLastTerrainPasteRotation);
                  }
               }
               if (mbClipartBBOnly == false)
               {
                  applyPastedData(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY);
               }
               setPastedDataPreview(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY, mLastTerrainPasteRotation, mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ);
            }

         }


         //STROKE CREATION
         if ((mCurrMode == eEditorMode.cModeVertAvgEdit)
          || (mCurrMode == eEditorMode.cModeVertHeightEdit)
          || (mCurrMode == eEditorMode.cModeVertPushEdit)
          || (mCurrMode == eEditorMode.cModeVertStd)
          || (mCurrMode == eEditorMode.cModeVertStdDot)
          || (mCurrMode == eEditorMode.cModeVertInflat)
          || (mCurrMode == eEditorMode.cModeVertInflatDot)
          || (mCurrMode == eEditorMode.cModeVertLayer)
          || (mCurrMode == eEditorMode.cModeVertPinch)
          || (mCurrMode == eEditorMode.cModeVertSmooth)
          || (mCurrMode == eEditorMode.cModeVertUniform)
          || (mCurrMode == eEditorMode.cModeVertScale)
          || (mCurrMode == eEditorMode.cModeVertAlpha)
          || (mCurrMode == eEditorMode.cModeVertTessellation)
          || (mCurrMode == eEditorMode.cModeVertSkirtHeight)
          || (mCurrMode == eEditorMode.cModeTexEdit)
          || (mCurrMode == eEditorMode.cModeTexErase)
          || (mCurrMode == eEditorMode.cModeVertSetHeight)
          || (mCurrMode == eEditorMode.cModePasteMode)
          || (mCurrMode == eEditorMode.cModeWaterOceanPaint)
          || (mCurrMode == eEditorMode.cModeWaterBodyPaint)
          || (mCurrMode == eEditorMode.cModeSimPassibility)
          || (mCurrMode == eEditorMode.cModeSimBuildibility)
          || (mCurrMode == eEditorMode.cModeSimHeightEdit)
          || (mCurrMode == eEditorMode.cModeSimSetHeight)
          || (mCurrMode == eEditorMode.cModeSimSmoothHeight)
          || (mCurrMode == eEditorMode.cModeSimEraseHeight)
          || (mCurrMode == eEditorMode.cModeFoliageSet)
          || (mCurrMode == eEditorMode.cModeFoliageErase))
         {
            if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft)
             || UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight))
            {

               if (UIManager.NewStroke())
               {
                  if ((mCurrMode != eEditorMode.cModeTexEdit) && mCurrMode != eEditorMode.cModeTexErase)
                  {
                     TerrainGlobals.getEditor().PushChanges();
                  }
               }
            }
         }

         if (mCurrMode == eEditorMode.cModeSampleHeightSlope)
         {
             if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft))
             {
                if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                   Masking.clearSelectionMask();

                float slopeVal = Vector3.Dot(BMathLib.Normalize(mBrushIntersectionNormal), BMathLib.unitY);

                mComponentMaskSettings.mSlopeValue = slopeVal;

                Masking.createSelectionMaskFromTerrain(mComponentMaskSettings);

                //Masking.createSelectionMaskFromTerrain(-100, 100, slopeVal, 0.05f);
                //Masking.createSelectionMaskFromTerrain(intPoint.Y - 3, intPoint.Y + 3);
             }
         }
         if (mCurrMode == eEditorMode.cModeSampleMinHeight)
         {
            if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft))
            {
               if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                  Masking.clearSelectionMask();

               mComponentMaskSettings.mMinHeight = intPoint.Y;
               Masking.createSelectionMaskFromTerrain(mComponentMaskSettings);
            }
         }
         if (mCurrMode == eEditorMode.cModeSampleMaxHeight)
         {
            if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft))
            {
               if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
                  Masking.clearSelectionMask();

               mComponentMaskSettings.mMaxHeight = intPoint.Y;
               Masking.createSelectionMaskFromTerrain(mComponentMaskSettings);
            }
         }
         switch (mCurrStrokeState)
         {
            case eEditorStrokeState.cStrokeStateInactive:
               if (left)
               {
                  mCurrStrokeState = eEditorStrokeState.cStrokeStateActiveLeft;
                  mCurrStrokeInput = eEditorStrokeInput.cStrokeInputMouse;
               }
               else if (right)
               {
                  mCurrStrokeState = eEditorStrokeState.cStrokeStateActiveRight;
                  mCurrStrokeInput = eEditorStrokeInput.cStrokeInputMouse;

                  if (mbRightClickApply)
                  {
                     mbOKPaste = true;
                  }
               }
               else if (mCurrBrush != null && mCurrBrush.isApplyOnSelectionEnabled())
               {
                  if (UIManager.GetAsyncKeyStateB(Key.F1))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateActiveLeft;
                     mCurrStrokeInput = eEditorStrokeInput.cStrokeInputKeyboard;

                    // if (mBackupDetailPoints.HasData())
                    //    PushChanges();
                  }
                  else if (UIManager.GetAsyncKeyStateB(Key.F2))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateActiveRight;
                     mCurrStrokeInput = eEditorStrokeInput.cStrokeInputKeyboard;

                   //  if (mBackupDetailPoints.HasData())
                   //     PushChanges();
                  }
               }
               break;

            case eEditorStrokeState.cStrokeStateActiveLeft:
               if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
               {
                  // Mouse Input
                  if (!UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateInactive;

                     if (mCurrMode != eEditorMode.cModePasteMode)
                        flushBrushDeformations();
                  }
               }
               else if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
               {
                  // Keyboard input
                  if (!UIManager.GetAsyncKeyStateB(Key.F1))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateInactive;

                     //      flushBrushDeformations();
                  }
               }
               break;

            case eEditorStrokeState.cStrokeStateActiveRight:
               if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
               {
                  // Mouse Input
                  if (!UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateInactive;
                     if (mCurrMode != eEditorMode.cModePasteMode)

                        flushBrushDeformations();
                  }
               }
               else if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
               {
                  // Keyboard input
                  if (!UIManager.GetAsyncKeyStateB(Key.F2))
                  {
                     mCurrStrokeState = eEditorStrokeState.cStrokeStateInactive;

                     //flushBrushDeformations();
                  }
               }
               break;

         }

         if (mbOKPaste)
         {
            addPasteTerrainUndoNodes(false);
            applyBrushDeformations();
            addPasteTerrainUndoNodes(true);
            pushBrushToUndo();
            clearBrushDeformations();
            mbOKPaste = false;
            return;
         }
         if (mbCancelPaste)
         {
            mbCancelPaste = false;
            clearBrushDeformations();
            return;
         }

         if (mbPasteModeDirty)
         {
            mbPasteModeDirty = false;
            if (mCurrMode == eEditorMode.cModePasteMode)
            {
               applyVertBrush(altDirection, true, true);
               mbPendingPasteDeformations = true;
               return;
            }
         }

         if (mbPendingPasteDeformations && mCurrMode != eEditorMode.cModePasteMode)
         {
            mbPendingPasteDeformations = false;
            mbCancelPaste = false;

            clearBrushDeformations();
            updateTerrainVisuals(false);
            return;
         }

         bool bAllowSingleClick = false;
         if (mCurrMode == eEditorMode.cModePasteMode)
         {
            bAllowSingleClick = true;
         }


         // When taking mouse input, only apply brush if the mouse has been moved or pressed.
         if (!bAllowSingleClick && mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
         {
            if (!(bMouseMoved || bClickedLeft || bClickedRight))
            {
               return;
            }
         }

         //DO SELECTION MODE (MASKING) FUNCTIONS
         if (selectionMode)
         {
            switch (mCurrMode)
            {
               case eEditorMode.cModeTexEdit:
               case eEditorMode.cModeTexErase:
                  {
                     switch (mCurrStrokeState)
                     {
                        case eEditorStrokeState.cStrokeStateActiveRight:
                           applyMaskTextureBrush(true);
                           break;

                        case eEditorStrokeState.cStrokeStateActiveLeft:
                           applyMaskTextureBrush(altDirection);
                           break;

                        case eEditorStrokeState.cStrokeStateInactive:
                           if (bClickedRight)
                              applyMaskTextureBrush(true);
                           else if (bClickedLeft)
                              applyMaskTextureBrush(altDirection);
                           break;
                     }
                     return;
                  }
               case eEditorMode.cModeDecalEdit:
                  if (bClickedLeft)
                  {
                     applyTextureDecalToMask();
                     return;
                  }
                  break;

               
               case eEditorMode.cModeVertHeightEdit:
               case eEditorMode.cModeVertPushEdit:
               case eEditorMode.cModeVertAvgEdit:
               case eEditorMode.cModeVertStd:
               case eEditorMode.cModeVertStdDot:
               case eEditorMode.cModeVertInflat:
               case eEditorMode.cModeVertInflatDot:
               case eEditorMode.cModeVertLayer:
               case eEditorMode.cModeVertPinch:
               case eEditorMode.cModeVertSmooth:
               case eEditorMode.cModeVertUniform:
               case eEditorMode.cModeVertScale:
               case eEditorMode.cModeVertAlpha:
               case eEditorMode.cModeVertTessellation:
               case eEditorMode.cModeVertSetHeight:
               case eEditorMode.cModePasteMode:
                  {
                     switch (mCurrStrokeState)
                     {
                        case eEditorStrokeState.cStrokeStateActiveRight:
                           applyMaskVertBrush(true); 
                           break;

                        case eEditorStrokeState.cStrokeStateActiveLeft:
                           applyMaskVertBrush(altDirection);
                           break;

                        case eEditorStrokeState.cStrokeStateInactive:
                           if (bClickedRight)
                              applyMaskVertBrush(true);
                           else if (bClickedLeft)
                              applyMaskVertBrush(altDirection);
                           break;
                     }
                     return;
                  }
            }
         }


         //NON MASKING FUNCTIONS
         switch (mCurrMode)
         {
            case eEditorMode.cModeTexEdit:
            case eEditorMode.cModeTexErase:
               if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
               {
                  if (UIManager.GetAsyncKeyStateB(Key.F1))
                     applyTextureToMask();
               }
               else
               {
                  if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  {
                     applyTextureBrush();
                  }
                  else if (mCurrStrokeState == eEditorStrokeState.cStrokeStateInactive)
                  {
                     if (bClickedLeft)
                        applyTextureBrush();
                  }
               }

               break;
            case eEditorMode.cModeDecalEdit:
               if (bClickedLeft)
               {
                  applyTextureDecal();
               }
               break;
            case eEditorMode.cModeDecalModify:
               if (bDragging && left)
               {
                  TerrainGlobals.getTexturing().moveSelectdDecalsFromEditorInput();
               }
               break;

            case eEditorMode.cModeVertPushEdit:
            case eEditorMode.cModeVertHeightEdit:
            case eEditorMode.cModeVertAvgEdit:
            case eEditorMode.cModeVertLayer:
            case eEditorMode.cModeVertPinch:
            case eEditorMode.cModeVertSmooth:
            case eEditorMode.cModeVertUniform:
            case eEditorMode.cModeVertScale:
            case eEditorMode.cModeVertAlpha:
            case eEditorMode.cModeVertTessellation:
            case eEditorMode.cModeVertSkirtHeight:
               switch (mCurrStrokeState)
               {
                  case eEditorStrokeState.cStrokeStateActiveRight:
                     applyVertBrush(true, false);
                     break;

                  case eEditorStrokeState.cStrokeStateActiveLeft:
                     applyVertBrush(altDirection, false);
                     break;
               }
               break;

            case eEditorMode.cModeVertSetHeight:
               switch (mCurrStrokeState)
               {
                  case eEditorStrokeState.cStrokeStateActiveRight:
                     applyVertBrush(true, false);
                     break;

                  case eEditorStrokeState.cStrokeStateActiveLeft:
                     applyVertBrush(altDirection, false);
                     break;

                  case eEditorStrokeState.cStrokeStateInactive:
                     if (bClickedRight)
                        applyVertBrush(true, false);
                     else if (bClickedLeft)
                        applyVertBrush(altDirection, false);
                     break;
               }
               break;


            case eEditorMode.cModeVertStd:
            case eEditorMode.cModeVertStdDot:
               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  applyVertBrush(true, (mCurrMode == eEditorMode.cModeVertStdDot) ? true : false);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  applyVertBrush(altDirection, (mCurrMode == eEditorMode.cModeVertStdDot) ? true : false);

               break;

            case eEditorMode.cModeVertInflat:
            case eEditorMode.cModeVertInflatDot:
               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  applyVertBrush(true, (mCurrMode == eEditorMode.cModeVertInflatDot) ? true : false);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  applyVertBrush(altDirection, (mCurrMode == eEditorMode.cModeVertInflatDot) ? true : false);

               break;

            case eEditorMode.cModePasteMode:
               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
               {
                  //applyVertBrush(true, true);
                  //setPastedDataPreview(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY,  mLastTerrainPasteRotation, mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ);
                  //if (mbClipartBBOnly == false)
                  //{
                  //   applyPastedData(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY);
                  //}
               }
               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
               {
                  //applyVertBrush(altDirection, true);
                  setPastedDataPreview(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY,  mLastTerrainPasteRotation, mLastTerrainScaleX, mLastTerrainScaleY, mLastTerrainScaleZ);

                  mLastTerrainPasteLocationX = mVisTileIntetersectionX;
                  mLastTerrainPasteLocationZ = mVisTileIntetersectionZ;
                  //mLastTerrainOffsetY = mLastTerrainOffsetY;

                  if (mbClipartBBOnly == false)
                  {
                     applyPastedData(mVisTileIntetersectionX, mVisTileIntetersectionZ, mLastTerrainOffsetY);
                  }
               }
               break;


               break;
            case eEditorMode.cModeSimPassibility:
            case eEditorMode.cModeSimFloodPassibility:
            case eEditorMode.cModeSimScarabPassibility:
            case eEditorMode.cModeSimTileType:
            case eEditorMode.cModeSimBuildibility:
            case eEditorMode.cModeSimHeightEdit:
            case eEditorMode.cModeSimSmoothHeight:
            case eEditorMode.cModeSimEraseHeight:
            case eEditorMode.cModeSimSetHeight:


               Vector3 simIntersect = mSimRep.getHeightRep().getIntersectPointFromScreenCursor();
               if (CoreGlobals.getEditorMain().mIGUI != null)
                  CoreGlobals.getEditorMain().mIGUI.setCursorLocationLabel(simIntersect.X, simIntersect.Y, simIntersect.Z);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  mSimRep.getHeightRep().applySimRepBrush(true);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  mSimRep.getHeightRep().applySimRepBrush(false);


               //if (mCurrStrokeState == eEditorStrokeState.cStrokeStateInactive)
               //{
               //   if (bClickedRight)
               //      mSimRep.getHeightRep().applySimRepBrush(true);
               //   else if (bClickedLeft)
               //      mSimRep.getHeightRep().applySimRepBrush(false);
               //}

               break;


            case eEditorMode.cModeFoliageSet:
            case eEditorMode.cModeFoliageErase:
               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  applyFoliageBrush(true);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  applyFoliageBrush(false);
               break;

            case eEditorMode.cModeCameraHeight:
            case eEditorMode.cModeCameraEraseHeights:
            case eEditorMode.cModeCameraHeightSmooth:
            case eEditorMode.cModeCameraSetHeight:

               Vector3 camIntersect = mCameraHeightRep.getIntersectPointFromScreenCursor();
               if (CoreGlobals.getEditorMain().mIGUI != null)
                  CoreGlobals.getEditorMain().mIGUI.setCursorLocationLabel(camIntersect.X, camIntersect.Y, camIntersect.Z);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  mCameraHeightRep.applyCameraRepBrush(true);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  mCameraHeightRep.applyCameraRepBrush(false);
               break;

            case eEditorMode.cModeFlightHeight:
            case eEditorMode.cModeFlightEraseHeights:
            case eEditorMode.cModeFlightHeightSmooth:
            case eEditorMode.cModeFlightSetHeight:

               Vector3 flightIntersect = mSimRep.getFlightRep().getIntersectPointFromScreenCursor();
               if (CoreGlobals.getEditorMain().mIGUI != null)
                  CoreGlobals.getEditorMain().mIGUI.setCursorLocationLabel(flightIntersect.X, flightIntersect.Y, flightIntersect.Z);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                  mSimRep.getFlightRep().applyFlightRepBrush(true);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  mSimRep.getFlightRep().applyFlightRepBrush(false);
               break;

            case eEditorMode.cModeRealLOSUnit:
               
                if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveRight)
                    mRealLOSRep.applyUnitLOS(true);

               if (mCurrStrokeState == eEditorStrokeState.cStrokeStateActiveLeft)
                  mRealLOSRep.applyUnitLOS(false);
               break;



            case eEditorMode.cModeNone:
            default:
               break;
         };
      }

      public void renderSimReps()
      {
         mSimRep.getHeightRep().render(CoreGlobals.getEditorMain().MainMode == BEditorMain.eMainMode.cTerrain && (
                                mCurrMode == eEditorMode.cModeSimBuildibility ||
                                mCurrMode == eEditorMode.cModeSimEraseHeight ||
                                mCurrMode == eEditorMode.cModeSimHeightEdit ||
                                mCurrMode == eEditorMode.cModeSimSetHeight ||
                                mCurrMode == eEditorMode.cModeSimSmoothHeight ||
                                mCurrMode == eEditorMode.cModeSimPassibility));

         mSimRep.getFlightRep().render(CoreGlobals.getEditorMain().MainMode == BEditorMain.eMainMode.cTerrain && (
                                mCurrMode == eEditorMode.cModeFlightHeight ||
                                mCurrMode == eEditorMode.cModeFlightSetHeight ||
                                mCurrMode == eEditorMode.cModeFlightHeightSmooth ||
                                mCurrMode == eEditorMode.cModeFlightEraseHeights));

         mCameraHeightRep.render(CoreGlobals.getEditorMain().MainMode == BEditorMain.eMainMode.cTerrain && (
                                mCurrMode == eEditorMode.cModeCameraHeight ||
                                mCurrMode == eEditorMode.cModeCameraSetHeight ||
                                mCurrMode == eEditorMode.cModeCameraHeightSmooth ||
                                mCurrMode == eEditorMode.cModeCameraEraseHeights));

         mRealLOSRep.render(false);
      }
      public void renderWidget()
      {
         if (CoreGlobals.getEditorMain().MainMode == BEditorMain.eMainMode.cTerrain)
         {
           if(mCurrMode== eEditorMode.cModeVertWidgetTrans)
           {
                  mTransWidget.calculateScale();

                  if (!Masking.isEmpty())
                  {
                     mTransWidget.render();
                     BRenderDevice.getDevice().Transform.World = Matrix.Identity;
                  }
            
            }
         }
      }
      public void render()
      {
         //set these to visible = false
         BTerrainQuadNode[] nodeList = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < nodeList.Length; i++)
            nodeList[i].IsVisibleThisFrame = false;

         //draw our version of the terrain
         TerrainGlobals.getTerrain().getFrustum().update(BRenderDevice.getDevice());
         List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();

         List<int> mHandles = new List<int>();
         List<BTerrainQuadNodeRenderInstance> nodeInstances = new List<BTerrainQuadNodeRenderInstance>();

         Matrix identMat = Matrix.Identity;
         TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), identMat, -1);


         int numOriginalNodes = mNodes.Count;

         if (m_bAlwaysRenderSkirt || (mCurrMode == eEditorMode.cModeVertSkirtHeight))
         {
            Matrix mat2 = Matrix.Identity;
            mat2.M11 = -1;
            mat2.M33 = -1;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat2, 0);
            
            Matrix mat1 = Matrix.Identity;
            mat1.M11 = -1;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat1, 1);

            Matrix mat8 = Matrix.Identity;
            mat8.M11 = -1;
            mat8.M33 = -1;
            mat8.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat8, 2);

            Matrix mat7 = Matrix.Identity;
            mat7.M33 = -1;
            mat7.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat7, 3);

            Matrix mat6 = Matrix.Identity;
            mat6.M11 = -1;
            mat6.M33 = -1;
            mat6.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
            mat6.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat6, 4);

            Matrix mat5 = Matrix.Identity;
            mat5.M11 = -1;
            mat5.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat5, 5);

            Matrix mat4 = Matrix.Identity;
            mat4.M11 = -1;
            mat4.M33 = -1;
            mat4.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat4, 6);

            Matrix mat3 = Matrix.Identity;
            mat3.M33 = -1;
            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, TerrainGlobals.getTerrain().getFrustum(), mat3, 7);
         }

         TerrainGlobals.getTerrain().getQuadNodeRoot().clearNonVisibleNodes(mHandles);


         for (int i = 0; i < mNodes.Count; i++)
         {
            mNodes[i].IsVisibleThisFrame = true;
            mNodes[i].evaluate(nodeInstances[i].texturingLOD);
         }

         //draw quadnode bounds if needed
         if (mDrawQuadNodeBounds)
         {
            for (int i = 0; i < numOriginalNodes; i++)
               DrawQuadNodeBounds(mNodes[i]);
         }

         //make setups for our render states
         //CLM moved to TerrainRender
         //if (mRenderMode == eEditorRenderMode.cRenderFullWireframe || mRenderMode == eEditorRenderMode.cRenderFlatWireframe)
         //   BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
         //else if (mRenderMode == eEditorRenderMode.cRenderFull || mRenderMode == eEditorRenderMode.cRenderFlat || mRenderMode == eEditorRenderMode.cRenderFullOverlay)
         //   BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);

         bool dotexturedRender = false;
         if (mRenderMode == eEditorRenderMode.cRenderFullWireframe || mRenderMode == eEditorRenderMode.cRenderFull || mRenderMode == eEditorRenderMode.cRenderFullOverlay
            || mRenderMode == eEditorRenderMode.cRenderTextureSelectRender)
         {
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_Full);
            dotexturedRender = true;
         }
         if (mRenderMode == eEditorRenderMode.cRenderFlatWireframe || mRenderMode == eEditorRenderMode.cRenderFlat)
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_NoTexturing);
       
         if (mRenderMode == eEditorRenderMode.cRenderTextureNormals)
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_Normals);
         if (mRenderMode == eEditorRenderMode.cRenderAmbientOcclusion)
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_AO);
         if (mRenderMode == eEditorRenderMode.cTerrainPerfEval)
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_ChunkPerf);



         if (dotexturedRender)
         {
            TerrainGlobals.getRender().renderEntireList(mNodes, nodeInstances);
         }
         else
         {
            TerrainGlobals.getRender().preRenderSetup();
            for (int i = 0; i < mNodes.Count; i++)
               mNodes[i].render(nodeInstances[i]);
            TerrainGlobals.getRender().postRender();   
         }



         if (mRenderMode == eEditorRenderMode.cRenderFullOverlay)
         {
            TerrainGlobals.getRender().preRenderSetup();   //in SM3 this sets up all the shader vars
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_NoTexturing);
            for (int i = 0; i < mNodes.Count; i++)
               mNodes[i].render(nodeInstances[i]);

         }
        
         // Render wireframe skirt offset data
         if (mCurrMode == eEditorMode.cModeVertSkirtHeight)
         {
            Vector4 skirtColorFront = new Vector4(1.0f, 0.0f, 0.0f, 1.0f);
            Vector4 skirtColorBack = new Vector4(0.5f, 0.0f, 0.0f, 0.3f);
            TerrainGlobals.getRender().preRenderSetup();   //in SM3 this sets up all the shader vars

            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
            BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);


            TerrainGlobals.getRender().setRenderPass(BTerrainRender.eRenderPass.eRP_SkirtWireframe);

            // - (first pass - non-ocluded)
               BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);

               for (int i = 0; i < 8; i++)
                  TerrainGlobals.getRender().renderSkirtOffsets(i, skirtColorFront);

            // - (second pass - ocluded)
               BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Greater);

               for (int i = 0; i < 8; i++)
                  TerrainGlobals.getRender().renderSkirtOffsets(i, skirtColorBack);


            // Restore state
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
            BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         }
         

         

        


         // Restore state to normal
         BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);


         
         
     
         {
            if (CoreGlobals.getEditorMain().MainMode == BEditorMain.eMainMode.cTerrain)
            {
               //draw our cursors
               switch (mCurrMode)
               {
                  case eEditorMode.cModeTexEdit:
                  case eEditorMode.cModeTexErase:
                     ((BTerrainSplatBrush)mCurrBrush).render();
                     break;

                  case eEditorMode.cModeDecalEdit:
                     ((BTerrainDecalBrush)mCurrBrush).render();
                     break;
                  case eEditorMode.cModeVertHeightEdit:
                  case eEditorMode.cModeVertPushEdit:
                  case eEditorMode.cModeVertAvgEdit:
                  case eEditorMode.cModeVertStd:
                  case eEditorMode.cModeVertStdDot:
                  case eEditorMode.cModeVertInflat:
                  case eEditorMode.cModeVertInflatDot:
                  case eEditorMode.cModeVertLayer:
                  case eEditorMode.cModeVertPinch:
                  case eEditorMode.cModeVertSmooth:
                  case eEditorMode.cModeVertUniform:
                  case eEditorMode.cModeVertScale:
                  case eEditorMode.cModeVertAlpha:
                  case eEditorMode.cModeVertTessellation:
                  case eEditorMode.cModeVertSkirtHeight:
                  case eEditorMode.cModeVertSetHeight:
                  case eEditorMode.cModePasteMode:
                     ((BTerrainVertexBrush)mCurrBrush).render();

                     if (mShowSelectionCamera)
                        drawSelectionCamera();
                     if (mCurrMode == eEditorMode.cModePasteMode)
                        drawPasteBB();

                     break;
                  case eEditorMode.cModeControlEdit:

                     break;
                  case eEditorMode.cModeVertWidgetTrans:
                     mTransWidget.calculateScale();

                     if (!Masking.isEmpty())
                     {
                        mTransWidget.render();
                        BRenderDevice.getDevice().Transform.World = Matrix.Identity;
                     }
                     break;
                  case eEditorMode.cModeShapeSelect:
                     render2DSelectionBox();
                     break;

                  case eEditorMode.cModeSimPassibility:
                  case eEditorMode.cModeSimBuildibility:
                  case eEditorMode.cModeSimFloodPassibility:
                  case eEditorMode.cModeSimScarabPassibility:
                  case eEditorMode.cModeSimTileType:
                  case eEditorMode.cModeSimHeightEdit:
                  case eEditorMode.cModeSimSetHeight:
                  case eEditorMode.cModeSimSmoothHeight:
                  case eEditorMode.cModeSimEraseHeight:
                     ((BTerrainSimBrush)mCurrBrush).render();
                     break;
                  case eEditorMode.cModeFoliageSet:
                  case eEditorMode.cModeFoliageErase:
                     ((BTerrainFoliageBrush)mCurrBrush).render();
                     break;
                  case eEditorMode.cModeNone:
                  default:
                     break;
               };
            }
         }


         RoadManager.render(mCurrMode == eEditorMode.cModeRoadEdit || mCurrMode == eEditorMode.cModeRoadAdd);

         if (mRenderFoliage || mCurrMode == eEditorMode.cModeFoliageSet || mCurrMode == eEditorMode.cModeFoliageErase)
            FoliageManager.render(mCurrMode == eEditorMode.cModeFoliageSet || mCurrMode == eEditorMode.cModeFoliageErase);

         //CLM RESTORE THIS DEVICE STATE FOR SIM OBJECTS
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
      }

      #endregion

      #region enums
      public enum eEditorRenderMode
      {
         cRenderFull = 0,
         cRenderFullWireframe,
         cRenderFlat,
         cRenderFlatWireframe,
         cRenderFullOverlay,
         cRenderTextureSelectRender,
         cRenderTextureNormals,
         cRenderAmbientOcclusion,
         cTerrainPerfEval,
      };

      public enum eEditorShapeSelectMode
      {
         eShapeSquare = 0,
         eShapeOval,
         eShapeCount,
      }
      public enum eEditorMode
      {
         cModeNone = 0,
         //texturing 
         cModeTexEdit,
         cModeTexErase,
         cModeDecalEdit,
         cModeDecalModify,

         //vertex
         cModeVertHeightEdit,
         cModeVertPushEdit,
         cModeVertAvgEdit,
         cModeVertStd,
         cModeVertStdDot,
         cModeVertInflat,
         cModeVertInflatDot,
         cModeVertLayer,
         cModeVertPinch,
         cModeVertSmooth,
         cModeVertUniform,
         cModeVertScale,
         cModeVertSetHeight,
         cModeVertAlpha,
         cModeVertTessellation,
         cModeVertWidgetTrans,

         //skirt
         cModeVertSkirtHeight,
        
         //masking
         cModeShapeSelect,
         cModeSampleHeightSlope,
         cModeSampleMinHeight,
         cModeCustomShape,

         //water
         cModeWaterEdit,
         cModeWaterBodyPaint,
         cModeWaterOceanPaint,
         cModeRiverEdit,

         //roads
         cModeRoadEdit,
         cModeRoadAdd,

         //foliage
         cModeFoliageSet,
         cModeFoliageErase,
         
         //sim heights
         cModeSimPassibility,
         cModeSimBuildibility,
         cModeSimFloodPassibility,
         cModeSimScarabPassibility,
         cModeSimTileType,
         cModeSimHeightEdit,
         cModeSimSetHeight,
         cModeSimSmoothHeight,
         cModeSimEraseHeight,

         //camera height rep
         cModeCameraHeight,
         cModeCameraSetHeight,
         cModeCameraHeightSmooth,
         cModeCameraEraseHeights,

         //flight height rep
         cModeFlightHeight,
         cModeFlightSetHeight,
         cModeFlightHeightSmooth,
         cModeFlightEraseHeights,

         //misc
         cModeSampleMaxHeight,
         cModeControlEdit,
         cModePasteMode,
         
         //real los
         cModeRealLOSUnit,
         
         cModeCount
      };

      public enum eEditorStrokeState
      {
         cStrokeStateInactive = 0,
         cStrokeStateActiveLeft,
         cStrokeStateActiveRight,
      };

      public enum eEditorStrokeInput
      {
         cStrokeInputMouse = 0,
         cStrokeInputKeyboard,
      };

      #endregion


      static public bool mbTextureUndo = true;
      static public bool mbVertexUndo = true;

      #region Widgets
      public void updateWidgetPos()
      {
         BBoundingBox box = new BBoundingBox();
         box.empty();
        // if (!Masking.isEmpty())
         {
            long id;
            float value;
            Masking.getCurrSelectionMaskWeights().ResetIterator();
            while (Masking.getCurrSelectionMaskWeights().MoveNext(out id, out value))
            {
               if (value == 0)
                  continue;

               int x = (int)(id / TerrainGlobals.getTerrain().getNumXVerts());
               int z = (int)(id - x * TerrainGlobals.getTerrain().getNumXVerts());

               box.addPoint(TerrainGlobals.getTerrain().getPostDeformPos(x, z));
            }
            mTransWidget.update(box.getCenter());
         }
 
          
      }
      //translation
      public Vector3 giveDominantPlane()
      {
         Vector3 dir = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget() - CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
         dir = BMathLib.Normalize(dir);

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

         if (x && !y && z) return new Vector3(0, 1, 0);

         Vector3 dir = CoreGlobals.getEditorMain().mITerrainShared.getCameraTarget() - CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
         dir = BMathLib.Normalize(dir);

         return dir;


      }
      public void translateVerts()
      {
         int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         //GET OUR ORIGIONAL RAY
         Vector3 press_r0 = BRenderDevice.getRayPosFromMouseCoords(false, mPressPoint);
         Vector3 press_rD = BRenderDevice.getRayPosFromMouseCoords(true, mPressPoint) - press_r0;
         press_rD = BMathLib.Normalize(press_rD);

         //GET OUR NEW RAY
         Point p = new Point();
         UIManager.GetCursorPos(ref p);
         Vector3 r0 = BRenderDevice.getRayPosFromMouseCoords(false, p);
         Vector3 rD = BRenderDevice.getRayPosFromMouseCoords(true, p) - r0;
         rD = BMathLib.Normalize(rD);

     
         //compute our intersection with the terrain
         Vector3 oldMovePointLTT = Vector3.Empty;
         Vector3 desiredMovePointLTT = Vector3.Empty;
         CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrain(ref press_r0, ref press_rD, ref oldMovePointLTT);
         CoreGlobals.getEditorMain().mITerrainShared.intersectPointOnTerrain(ref r0, ref rD, ref desiredMovePointLTT);

         //compute our intersection with the best plane
         Vector3 oldMovePointBP = Vector3.Empty;
         Vector3 desiredMovePointBP = Vector3.Empty;
         Vector3 pN = Vector3.Empty;
         pN = giveWidgetSelectionPlane();
         Plane pl = Plane.FromPointNormal(mTransWidget.mTranslation, pN);
         float tVal = 0;
         BMathLib.rayPlaneIntersect(pl, r0, rD, false, ref tVal);
         desiredMovePointBP = r0 + (rD * tVal);
         BMathLib.rayPlaneIntersect(pl, press_r0, press_rD, false, ref tVal);
         oldMovePointBP = press_r0 + (press_rD * tVal);

          long id;
          float value;
          Masking.getCurrSelectionMaskWeights().ResetIterator();
          while (Masking.getCurrSelectionMaskWeights().MoveNext(out id, out value))
          {
             if (value == 0)
                continue;

             int x = (int) (id / TerrainGlobals.getTerrain().getNumXVerts());
             int z = (int) (id - x * TerrainGlobals.getTerrain().getNumXVerts());

             Vector3 pos = TerrainGlobals.getTerrain().getPostDeformRelPos(x,z);
             Vector3 diff = desiredMovePointBP - oldMovePointBP;
             mTransWidget.capMovement(ref diff);

             float weight = Masking.getCurrSelectionMaskWeights().GetMaskWeight(id);


             Vector3 newDeformation = diff * weight;

             // Fixed edge to not move along the X-Z plane
             if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
                newDeformation.X = 0.0f;
             if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
                newDeformation.Z = 0.0f;

             TerrainGlobals.getEditor().getDetailPoints()[id] = pos + newDeformation;

             int qnXIndx = (int)(x / BTerrainQuadNode.cMaxWidth);
             int qnZIndx = (int)(z / BTerrainQuadNode.cMaxHeight);
             TerrainGlobals.getTerrain().getQuadNodeLeafArray()[qnXIndx + numXChunks * qnZIndx].mDirty = true;

          }
         
         updateWidgetPos();

        // Masking.rebuildVisualsAfterSelection();
         TerrainGlobals.getTerrain().getQuadNodeRoot().rebuildDirty(BRenderDevice.getDevice());
      }

      public void translateVerts2(float xDiff, float yDiff, float zDiff)
      {
         translateVerts2(new Vector3(xDiff, yDiff, zDiff));
      }
      public void translateVerts2(Vector3 diff)
      {
         int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         long id;
         float value;
         Masking.getCurrSelectionMaskWeights().ResetIterator();
         while (Masking.getCurrSelectionMaskWeights().MoveNext(out id, out value))
         {
            if (value == 0)
               continue;

            int x = (int)(id / TerrainGlobals.getTerrain().getNumXVerts());
            int z = (int)(id - x * TerrainGlobals.getTerrain().getNumXVerts());

            Vector3 pos = TerrainGlobals.getTerrain().getPostDeformRelPos(x, z);
            float weight = Masking.getCurrSelectionMaskWeights().GetMaskWeight(id);
            TerrainGlobals.getEditor().getDetailPoints()[id] = pos + diff * weight;

            int qnXIndx = (int)(x / BTerrainQuadNode.cMaxWidth);
            int qnZIndx = (int)(z / BTerrainQuadNode.cMaxHeight);
            TerrainGlobals.getTerrain().getQuadNodeLeafArray()[qnXIndx + numXChunks * qnZIndx].mDirty = true;


         }
         // Masking.rebuildVisualsAfterSelection();
         TerrainGlobals.getTerrain().getQuadNodeRoot().rebuildDirty(BRenderDevice.getDevice());
      }

      

      #endregion

      public void renderSelectionThumbnail(bool TopDownCamera, bool doTexturing, bool doObjects)
      {
         Surface savedRenderTarget = null;
         Surface thumbnailRenderTarget = null;
         float selectionCameraFOV = 45.0f;

         try
         {

            // if empty box only clear texture and return
            if (Masking.mCurrSelectionMaskExtends.isEmpty())
            {
               // Changed render target to off-screen buffer
               savedRenderTarget = BRenderDevice.getDevice().GetRenderTarget(0);
               thumbnailRenderTarget = mThumbnailTexture.GetSurfaceLevel(0);
               BRenderDevice.getDevice().SetRenderTarget(0, thumbnailRenderTarget);

               // Clear target
               BRenderDevice.getDevice().Clear(ClearFlags.Target, unchecked((int)0x8C003F3F), 1.0f, 0);

               // Restore render target back to normal
               BRenderDevice.getDevice().SetRenderTarget(0, savedRenderTarget);



               return;
            }



            // Create a new frustum based on the selected region
            //
            BBoundingBox selectionBox = new BBoundingBox();

            // loop through all selected verts and 
            long index;
            float value;
            Masking.getCurrSelectionMaskWeights().ResetIterator();
            while (Masking.getCurrSelectionMaskWeights().MoveNext(out index, out value))
            {
               if (value == 0.0f)
                  continue;

               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               Vector3 vertPos = TerrainGlobals.getTerrain().getPos(x, z);

               selectionBox.addPoint(vertPos);
            }

            Vector3 selectionCenter = selectionBox.getCenter();
            float boundingSphereRadius = (selectionBox.min - selectionCenter).Length() * 0.85f;  // use some fudge factor
            float cameraSeparationDistance = (float)(boundingSphereRadius / (Math.Tan(Geometry.DegreeToRadian(selectionCameraFOV / 2.0f))));

            Vector3 up = new Vector3(0.0f, 0.0f, 1.0f);
            //Vector3 eye = selectionCenter + new Vector3(-cameraSeparationDistance, cameraSeparationDistance, -cameraSeparationDistance);
            Vector3 eye = selectionCenter + new Vector3(0.0f, cameraSeparationDistance, 0.0f);


            Matrix savedProj = BRenderDevice.getDevice().GetTransform(TransformType.Projection);
            Matrix savedView = BRenderDevice.getDevice().GetTransform(TransformType.View);
            if (TopDownCamera)
            {

            Matrix Proj;
            Proj = Matrix.PerspectiveFovLH(Geometry.DegreeToRadian(selectionCameraFOV), 1.0f, 1.0f, 10000.0f);
            BRenderDevice.getDevice().SetTransform(TransformType.Projection, Proj);

            Matrix View;
            View = Matrix.LookAtLH(eye, selectionCenter, up);
            BRenderDevice.getDevice().SetTransform(TransformType.View, View);

            }
            BFrustum selectionFrustum = new BFrustum();
            selectionFrustum.update(BRenderDevice.getDevice());

            // Changed render target to off-screen buffer
            savedRenderTarget = BRenderDevice.getDevice().GetRenderTarget(0);
            thumbnailRenderTarget = mThumbnailTexture.GetSurfaceLevel(0);
            BRenderDevice.getDevice().SetRenderTarget(0, thumbnailRenderTarget);

            // Clear target
            BRenderDevice.getDevice().Clear(ClearFlags.Target, unchecked((int)0x8C003F3F), 1.0f, 0);


            // Draw the terrain
            List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();
            List<BTerrainQuadNode> mSelectedNodes = new List<BTerrainQuadNode>();
            //TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafMaskedNodes(mNodes);

            List<int> mHandles = new List<int>();
            List<BTerrainQuadNodeRenderInstance> nodeInstances = new List<BTerrainQuadNodeRenderInstance>();

            TerrainGlobals.getTerrain().getQuadNodeRoot().getVisibleNodes(mNodes, mHandles, nodeInstances, selectionFrustum, Matrix.Identity, -1);

          

               //make setups for our render states
               BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
             BTerrainRender.eRenderPass passNum = BTerrainRender.eRenderPass.eRP_Select_NoTextured;
             if (doTexturing)
                passNum = BTerrainRender.eRenderPass.eRP_Select_Textured;
            TerrainGlobals.getRender().setRenderPass(passNum);




            TerrainGlobals.getRender().preRenderSetup();   //in SM3 this sets up all the shader vars


            // Enable alpha blending
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);


            if (doTexturing)
            {
               TerrainGlobals.getRender().renderEntireList(mNodes, nodeInstances);
            }
            else
            {
               for (int i = 0; i < mNodes.Count; i++)
                  mNodes[i].render(nodeInstances[i]);
            }

            //cleanup, isle 12
            mNodes.Clear();

            if (TopDownCamera)
            {
            // Restore transforms back to normal
            BRenderDevice.getDevice().SetTransform(TransformType.Projection, savedProj);
            BRenderDevice.getDevice().SetTransform(TransformType.View, savedView);
            }
            // Restore render target back to normal
            BRenderDevice.getDevice().SetRenderTarget(0, savedRenderTarget);

         }
         finally
         {
            savedRenderTarget.Dispose();
            thumbnailRenderTarget.Dispose();
         }
      }
      public Image GetThumbnailBitmap()
      {
         return Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, mThumbnailTexture));
      }


      public void setRenderMode(eEditorRenderMode mode) 
      {
         mRenderMode = mode;
         TerrainGlobals.getTexturing().reloadCachedVisuals();
      }

      public eEditorRenderMode getRenderMode()
      {
         return mRenderMode;
      }
      
      public void setMode(eEditorMode mode)
      {
         if(mCurrMode == eEditorMode.cModeVertTessellation && mode != eEditorMode.cModeVertTessellation)
            TerrainGlobals.getTerrain().refreshTerrainVisuals();

         mCurrMode = mode;

         // Vertex brushes only make sense with circle brush shape
         if (mCurrMode != eEditorMode.cModeTexEdit && mCurrMode != eEditorMode.cModeDecalEdit)
         {
            setBrushShape((BTerrainEditor.eBrushShape.cCircleBrush));

            if(mCurrMode != eEditorMode.cModeDecalModify)
               TerrainGlobals.getTexturing().unselectAllDecalInstances();
         }
         SimGlobals.getSimMain().modeChanged();
         clearUndoPacket();
      }

      public eEditorMode getMode() { return mCurrMode; }



      //misc states
      public void setRenderNodeBounds(bool onOff) { mDrawQuadNodeBounds = onOff; }
      public bool getRenderNodeBounds() { return mDrawQuadNodeBounds; }


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

      #region TEXTURE REGION
      public bool mEraseTextureInstead = false;
      //texture Editing
      public float getUScale() { return muScale; }
      public float getVScale() { return mvScale; }
      public bool newSplatBrush(int texArrayIndex, Texture msk, string maskFileName, BrushStroke stroke)
      {
         cleanBrush();
         //assert(msk);
         mCurrBrush = BrushManager.getSplatBrush();
         //throw msk through the resampler to get proper size
         if (((BTerrainSplatBrush)mCurrBrush).init(texArrayIndex, msk))
         {
            ((BTerrainSplatBrush)mCurrBrush).mStroke = stroke;
            ((BTerrainSplatBrush)mCurrBrush).mStroke.mBrushRadius = this.mBrushInfo.mRadius;
            ((BTerrainSplatBrush)mCurrBrush).mStroke.mRotationAmt = this.mBrushInfo.mRotation;
            ((BTerrainSplatBrush)mCurrBrush).mStroke.SetBrushImage(maskFileName);
            return true;
         }
         return false;
      }
      public bool newDecalBrush(int texDecalIndex, BrushStroke stroke,string maskFileName)
      {
         cleanBrush();
         
         mCurrBrush = BrushManager.getDecalBrush();

         //throw msk through the resampler to get proper size
         if (((BTerrainDecalBrush)mCurrBrush).init(texDecalIndex))
         {
            this.mBrushInfo.mRotation = 0;
            ((BTerrainDecalBrush)mCurrBrush).mStroke = stroke;
            ((BTerrainDecalBrush)mCurrBrush).mStroke.mBrushRadius = this.mBrushInfo.mRadius;
            ((BTerrainDecalBrush)mCurrBrush).mStroke.SetBrushImage(maskFileName);
            ((BTerrainDecalBrush)mCurrBrush).mStroke.mRotationAmt = this.mBrushInfo.mRotation ;
            return true;
         }
         return false;
      }

      public BrushStroke getCurrentStroke()
      {
         if ((mCurrBrush != null) && (mCurrBrush is BTerrainSplatBrush))
         {
            return ((BTerrainSplatBrush)mCurrBrush).mStroke;
         }
         return null;
      }
      public void resizeMask(float scalarFromOrigional)
      {
         //grab current brush mask data
         //throw it through the resampler
         //update it.
      }
      public bool isInMaskSelectionMode()
      {
         return (UIManager.GetAsyncKeyStateB(Key.RightControl) || UIManager.GetAsyncKeyStateB(Key.LeftControl)) && (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.RightShift));
      }

     
      //TEXTURE UNDO DATA

      public class TextureUndoData : UndoInstanceData
      {
         public TextureUndoData()
         {
            mMemorySize=0;
         }
         ~TextureUndoData()
         {
            if(affectedQNs!=null)
            {
               for (int i = 0; i < affectedQNs.Count; i++)
                  affectedQNs[i] = null;
               affectedQNs.Clear();
               affectedQNs = null;
            }
            
         }
         public class TextureUndoQN
         {
            ~TextureUndoQN()
            {
               cont.destroy();
               cont = null;
            }
            public BTerrainLayerContainer cont;
            public int minXVert;
            public int minZVert;
         }
         public List<TextureUndoQN> affectedQNs = new List<TextureUndoData.TextureUndoQN>();
      }



      public void textureUndoCallback(UndoInstanceData pkt)
      {

         TextureUndoData vud = (TextureUndoData)pkt;
         for(int i=0;i<vud.affectedQNs.Count;i++)
         {
            BTerrainQuadNode node = TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingPoint(vud.affectedQNs[i].minXVert + 1, vud.affectedQNs[i].minZVert + 1);
            if (node == null)
               MessageBox.Show("SHIT!");

            vud.affectedQNs[i].cont.copyTo(ref node.mLayerContainer);
            for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
               node.getTextureData(lod).free();
         }
      }

      void textureCurrentUndoPacket()
      {
         //this packet has been pushed and cleared.
         if (mCurrentUndoPacket == null)
         {
            mCurrentUndoPacket = new UndoPacket();
            mCurrentUndoPacket.mCallbackFunction = textureUndoCallback;

            mCurrentUndoPacket.mPrevData = new TextureUndoData();
            mCurrentUndoPacket.mCurrData = new TextureUndoData();
         }
      }
      public void addTextureUndoNodes(List<BTerrainQuadNode> nodes, bool isCurrent)
      {
         textureCurrentUndoPacket();
         TextureUndoData vud = isCurrent ? (TextureUndoData)mCurrentUndoPacket.mCurrData : (TextureUndoData)mCurrentUndoPacket.mPrevData;
         addTextureUndoNodes(vud, nodes.ToArray(), isCurrent);
      }
      public void addTextureUndoNodes(BTerrainQuadNode[] nodes, bool isCurrent)
      {
         textureCurrentUndoPacket();
         TextureUndoData vud = isCurrent ? (TextureUndoData)mCurrentUndoPacket.mCurrData : (TextureUndoData)mCurrentUndoPacket.mPrevData;
         addTextureUndoNodes(vud, nodes, isCurrent);
      }
      public void addTextureUndoNodes(TextureUndoData vud, BTerrainQuadNode[] nodes, bool isCurrent)
      {
         if (mbTextureUndo == false)
            return;
         
         for (int i = 0; i < nodes.Length; i++)
         {
             int found = -1;
            for (int x = 0; x < vud.affectedQNs.Count; x++)
            {
               if (vud.affectedQNs[x].minXVert == nodes[i].getDesc().mMinXVert &&
                   vud.affectedQNs[x].minZVert == nodes[i].getDesc().mMinZVert)
               {
                  found = x;
                  break;
               }
            }

            if (!isCurrent)
            {
               if(found!=-1)
                  continue;

               int c = vud.affectedQNs.Count;
               vud.affectedQNs.Add(new TextureUndoData.TextureUndoQN());
               vud.affectedQNs[c].cont = new BTerrainLayerContainer();
               nodes[i].mLayerContainer.copyTo(ref vud.affectedQNs[c].cont);
               vud.affectedQNs[c].minXVert = nodes[i].getDesc().mMinXVert;
               vud.affectedQNs[c].minZVert = nodes[i].getDesc().mMinZVert;
               vud.mMemorySize += nodes[i].mLayerContainer.getMemorySize();
            }
            else
            {
               int c = found;
               if(found==-1)
               {
                  c = vud.affectedQNs.Count;
                  vud.affectedQNs.Add(new TextureUndoData.TextureUndoQN());
                  vud.affectedQNs[c].cont = new BTerrainLayerContainer();
                  vud.affectedQNs[c].minXVert = nodes[i].getDesc().mMinXVert;
                  vud.affectedQNs[c].minZVert = nodes[i].getDesc().mMinZVert;
                  vud.mMemorySize += nodes[i].mLayerContainer.getMemorySize();
               }
               

               nodes[i].mLayerContainer.copyTo(ref vud.affectedQNs[c].cont);
            }
         }
      }




      public void applySelectedTextureToMask()
      {
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
                                                                                   Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);

         addTextureUndoNodes(nodes,false);
         

         for (int i = 0; i < nodes.Count; i++)
         {
           ((BTerrainSplatBrush)mCurrBrush).applyToMask(nodes[i]);
           
         }

         addTextureUndoNodes(nodes, true);
         PushChanges();
         
  
      }

      private void applyTextureBrush()
      {
         Vector3 intpt = mBrushIntersectionPoint;
         float vertsToPixelsRatio = 1.0f;

         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         //{
         int x = 0; int z = 0;
         // in this node, find the CLOSEST vertex to this actual position.
         findClosestVertex(ref x, ref z, ref intpt, mBrushIntersectionNode);
         intpt.X = x * TerrainGlobals.getTerrain().getTileScale();
         intpt.Z = z * TerrainGlobals.getTerrain().getTileScale();

         int rad;
         ((BTerrainSplatBrush)mCurrBrush).mStroke.mBrushRadius = this.mBrushInfo.mRadius;
         ((BTerrainSplatBrush)mCurrBrush).mStroke.mAlphaValue = this.mBrushInfo.mIntensity;
         ((BTerrainSplatBrush)mCurrBrush).mStroke.mRotationAmt = this.mBrushInfo.mRotation;
         if (((BTerrainSplatBrush)mCurrBrush).mStroke.brushInput(intpt.X, intpt.Z, false))
         {

            //int rad = ((BTerrainPaintBrush)mCurrBrush).getMaskWidth() >> 1;
            //rad = (int)((((BTerrainSplatBrush)mCurrBrush).mStroke.mMaxSize) / 3);
            rad = ((BTerrainSplatBrush)mCurrBrush).mStroke.EstimateInfluence();

            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, (int)(x - rad),
                                                                                     (int)(x + rad),
                                                                                     (int)(z - rad),
                                                                                     (int)(z + rad));
            //  }

          
            addTextureUndoNodes(nodes,false);

            for (int i = 0; i < nodes.Count; i++)
            {
               BTerrainQuadNodeDesc desc;
               desc = nodes[i].getDesc();

               //int cx = (int)(((x - desc.mMinXVert)) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() >>1));
               //int cy = (int)(((z - desc.mMinZVert)) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() >>1));

             //  for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
               {
                  if (((BTerrainPaintBrush)mCurrBrush).applyToDstImg(nodes[i],  x, z, false, false))
                  {
                     
                  }
               }
            }

            addTextureUndoNodes(nodes, true);
         }
      
      }

      private void applyMaskTextureBrush(bool alternate)
      {
         //TODO : this is a bit slow... speed up!

         {
            Vector3 intpt = mBrushIntersectionPoint;


            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

            int x = 0; int z = 0;            
            {

               // in this node, find the CLOSEST vertex to this actual position.
               findClosestVertex(ref x, ref z, ref intpt, mBrushIntersectionNode);
               intpt.X = x * TerrainGlobals.getTerrain().getTileScale();
               intpt.Z = z * TerrainGlobals.getTerrain().getTileScale();
            }
            ((BTerrainSplatBrush)mCurrBrush).mStroke.mBrushRadius = this.mBrushInfo.mRadius;
            ((BTerrainSplatBrush)mCurrBrush).mStroke.mAlphaValue = this.mBrushInfo.mIntensity;
            if (((BTerrainSplatBrush)mCurrBrush).mStroke.brushInput(intpt.X, intpt.Z, false))
            {
               //float validRadius = 6 * ((mMaskScalar * ((BTerrainPaintBrush)mCurrBrush).getMaskWidth()) / (BTerrainQuadNode.getMaxNodeWidth() / muScale)) / 2f;
               //float validRadius = (int)((((BTerrainSplatBrush)mCurrBrush).mStroke.mMaxSize) / 4);

               float validRadius = (float)((BTerrainSplatBrush)mCurrBrush).mStroke.EstimateInfluence();// *2f;

               TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(nodes, ref intpt, validRadius);




               for (int i = 0; i < nodes.Count; i++)
               {
                  //conv world coords to local XY pixel coords
                  int cx = 0;
                  int cy = 0;
                  BTerrainQuadNodeDesc desc;
                  desc = nodes[i].getDesc();
                  cx = (int)((((intpt.X - desc.m_min.X) / (desc.m_max.X - desc.m_min.X)) * BTerrainTexturing.getAlphaTextureWidth()) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() / 2f));
                  cy = (int)((((intpt.Z - desc.m_min.Z) / (desc.m_max.Z - desc.m_min.Z)) * BTerrainTexturing.getAlphaTextureHeight()) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() / 2f));

                  int cx2 = (int)(((x - desc.mMinXVert)) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() / 2f));

                  //ugh , need more research

                  //cx = cx2;// ((cx + cx2) / 2);
                  //cy = (int)(((z - desc.mMinZVert)) - (((BTerrainPaintBrush)mCurrBrush).getMaskWidth() / 2f));


                  //Apply
                  //((BTerrainPaintBrush)mCurrBrush).applyToDstImg(nodes[i], nodes[i].getTextureData(), x, y, true, alternate);
                  
                  //((BTerrainPaintBrush)mCurrBrush).applyToDstImg(nodes[i], nodes[i].getTextureData(), cx, cy, true, alternate);
                  
                  {
                     ((BTerrainPaintBrush)mCurrBrush).applyToDstImg(nodes[i], cx, cy, true, alternate);
                  }
               }
            }

            for (int i = 0; i < nodes.Count; i++)
            {
               nodes[i].mDirty = true;
            }

            TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         }
      }
      public void applyTextureToMask()
      {

         BTerrainQuadNode []nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         

         addTextureUndoNodes(nodes, false);
         
         for (int i = 0; i < nodes.Length; i++)
         {
            BTerrainQuadNodeDesc desc = nodes[i].getDesc();
            if (Masking.doesAreaContainSelection(desc.mMinXVert, desc.mMaxXVert, desc.mMinZVert, desc.mMaxZVert))
            {
               ((BTerrainSplatBrush)mCurrBrush).applyToMask(nodes[i]);
            }
         }
         addTextureUndoNodes(nodes, true);
         PushChanges();

         nodes = null;

      }

      private int getTextureInfoAtCursor(bool doSelectDecal, ref BTerrainTexturingLayer.eLayerType layerType)
      {
         Vector3 intpt = mBrushIntersectionPoint;

         float vertsToPixelsRatio = 1.0f;


         int x = 0; int z = 0;
         // in this node, find the CLOSEST vertex to this actual position.
         findClosestVertex(ref x, ref z, ref intpt, mBrushIntersectionNode);

         BTerrainQuadNodeDesc desc = mBrushIntersectionNode.getDesc();
         x -= desc.mMinXTile;
         z -= desc.mMinZTile;


         int selectedIndex = mBrushIntersectionNode.mLayerContainer.giveLayerIDAtPixel(x, z, ref layerType, doSelectDecal);
         return selectedIndex;
      }
      private void selectTextureAtCursor()
      {
            BTerrainTexturingLayer.eLayerType layerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
            int selectedIndex = getTextureInfoAtCursor(false,ref layerType);

            if(layerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
            {
               TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = selectedIndex;
               setMode(eEditorMode.cModeTexEdit);
               TerrainGlobals.getTexturing().unselectAllDecalInstances();
            }
            else if (layerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
            {
               TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex = selectedIndex;
               setMode(eEditorMode.cModeDecalEdit);
            }
            
            
            
            CoreGlobals.getEditorMain().mIGUI.ReloadVisibleTextureThumbnails(true);
      }
      private void selectDecalAtCursor()
      {
         BTerrainTexturingLayer.eLayerType layerType = BTerrainTexturingLayer.eLayerType.cLayer_Splat;
         int selectedIndex = getTextureInfoAtCursor(true, ref layerType);

         if (layerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
         {
            selectedIndex = TerrainGlobals.getTexturing().getActiveDecalInstance(selectedIndex).mActiveDecalIndex;
            TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex = selectedIndex;
            setMode(BTerrainEditor.eEditorMode.cModeDecalModify);
         }

         CoreGlobals.getEditorMain().mIGUI.ReloadVisibleTextureThumbnails(true);
      }

      private void applyTextureDecal()
      {

         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!
         //CLM THIS MATH IS VERY TRICKY!! PLEASE TALK TO ME BEFORE SCREWING WITH IT!!!!

         Vector3 intpt = mBrushIntersectionPoint;

         //add a new decal instance

         //convert worldspace interceptions into pixel space.
         //we conver to the 512x512 version to ensure we've got good pixel accuracy

         //get the XZ tile that this intersects with, and scale that into 512x512 pixel space
         float vertsToHighResPixelSpaceRatio =BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         Vector3 a = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX, mVisTileIntetersectionZ);
         Vector3 b = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX+1, mVisTileIntetersectionZ);
         Vector3 c = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX, mVisTileIntetersectionZ+1);
         Vector3 d = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX+1, mVisTileIntetersectionZ+1);

         //find the max def for this tile
         float x1 = (float)Math.Abs(a.X - b.X);
         float x2 = (float)Math.Abs(c.X - d.X);
         float xM = x1 > x2 ? x1 : x2;
         float xPT = xM>0?(intpt.X - a.X) / xM:0;   //gives us percentage IN THE TILE

         float z1 = (float)Math.Abs(a.Z - c.Z);
         float z2 = (float)Math.Abs(b.Z - d.Z);
         float zM = z1 > z2 ? z1 : z2;
         float zPT = zM>0?(intpt.Z - a.Z) / zM:0;   //gives us percentage IN THE TILE

         int xCenterInPixelSpace = (int)((xPT * vertsToHighResPixelSpaceRatio) + (mVisTileIntetersectionX * vertsToHighResPixelSpaceRatio));
         int zCenterInPixelSpace = (int)((zPT * vertsToHighResPixelSpaceRatio) + (mVisTileIntetersectionZ * vertsToHighResPixelSpaceRatio));

         int h = ((BTerrainDecalBrush)mCurrBrush).getDecalHeight();
         int w = ((BTerrainDecalBrush)mCurrBrush).getDecalWidth();

         float uScale =  (mBrushInfo.mRadius * 4 * vertsToHighResPixelSpaceRatio / (float)w);
         float vScale =  (mBrushInfo.mRadius * 4 * vertsToHighResPixelSpaceRatio / (float)h);

         float rotation = mBrushInfo.mRotation;



         //In order to keep some sanity, first see if this decal will pop the cap for the max num decals allowed per chunk.. if so, then don't place this instance at all
         
         //now, tell the chunks that this decal overlaps that they have a new layer.
         //compute our bounds in pixel space, then divide to get them in tile space
         //TODO : Rotation??
         w = (int)(w*uScale);
         h = (int)(h*vScale);
         int mnXTile = (int)((xCenterInPixelSpace - (w >> 1)) / vertsToHighResPixelSpaceRatio);
         int mxXTile = (int)((xCenterInPixelSpace + (w >> 1)) / vertsToHighResPixelSpaceRatio);
         int mnZTile = (int)((zCenterInPixelSpace - (h >> 1)) / vertsToHighResPixelSpaceRatio);
         int mxZTile = (int)((zCenterInPixelSpace + (h >> 1)) / vertsToHighResPixelSpaceRatio);

         bool okToCreateInstance=true;
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mnXTile, mxXTile, mnZTile, mxZTile);
         for(int i=0;i<nodes.Count;i++)
         {
            if(nodes[i].mLayerContainer.getNumDecalLayers() +1 >= BTerrainTexturing.cMaxDecalsPerChunk)
            {
               okToCreateInstance = false;
               MessageBox.Show("Error: This decal exceeds the max number of decals for one or more chunks.\n Please remove decals to get below the " + BTerrainTexturing.cMaxDecalsPerChunk + " max value.");
               setNoMode();
               break;
            }
         }

         if(okToCreateInstance)
         {
            int dcalInstance = TerrainGlobals.getTexturing().addActiveDecalInstance(TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex,
                                                                                                xCenterInPixelSpace,   zCenterInPixelSpace, 
                                                                                                 1.0f / uScale, 1.0f / vScale, 
                                                                                                 rotation,
                                                                                                 mnXTile,mxXTile,mnZTile,mxZTile);
            for(int i=0;i<nodes.Count;i++)
            {
               nodes[i].mLayerContainer.newDecalLayer(dcalInstance);
               nodes[i].mLayerContainer.computeDecalLayerAlphaContrib(nodes[i].mLayerContainer.getNumLayers() - 1, nodes[i].getDesc().mMinXVert, nodes[i].getDesc().mMinZVert);
               //nodes[i].getTextureData().free();
               for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
               {
                  nodes[i].getTextureData(lod).free();
               }
            }
         }
      }
      private void applyTextureDecalToMask()
      {
         int h = ((BTerrainDecalBrush)mCurrBrush).getDecalHeight();
         int w = ((BTerrainDecalBrush)mCurrBrush).getDecalWidth();
         float uScale = (mBrushInfo.mRadius * 4  / (float)w);
         float vScale = (mBrushInfo.mRadius * 4  / (float)h);
         int nW = (int)(w * uScale);
         int nH = (int)(h * vScale);
         


         //get our opacity information so we can resize it
         byte[] imgDat = new byte[h * w];
         BTerrainActiveDecalContainer dcl = TerrainGlobals.getTexturing().getActiveDecal(TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex);
         GraphicsStream texstream = dcl.mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity].mTexture.LockRectangle(0, LockFlags.None);
         texstream.Read(imgDat, 0, w * h);
         dcl.mTexChannels[(int)BTerrainTexturing.eTextureChannels.cOpacity].mTexture.UnlockRectangle(0);

         //apply transformations to match the decal itself
         byte[] tImgResized = null;
         byte[] tImgRotated = null;
        // byte[] imgDat = new byte[h * w];
         //imgDat = ImageManipulation.extractChannelFromImg(imgDatFull, w, h, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_R);
         tImgResized = ImageManipulation.resizeGreyScaleImg(imgDat, w, h, nW, nH, ImageManipulation.eFilterType.cFilter_Linear);
         tImgRotated = ImageManipulation.rotateGreyScaleImg(tImgResized, nW, nH, (float)(-mBrushInfo.mRotation * (180.0f / Math.PI)), false, out nW, out nH, ImageManipulation.eFilterType.cFilter_Nearest);
         byte[] tImg = tImgRotated;

         //translate our image
         Vector3 intpt = mBrushIntersectionPoint;
         float vertsToHighResPixelSpaceRatio = BTerrainTexturing.getTextureWidth() / BTerrainQuadNode.cMaxWidth;

         Vector3 a = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX, mVisTileIntetersectionZ);
         Vector3 b = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX + 1, mVisTileIntetersectionZ);
         Vector3 c = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX, mVisTileIntetersectionZ + 1);
         Vector3 d = TerrainGlobals.getTerrain().getPostDeformPos(mVisTileIntetersectionX + 1, mVisTileIntetersectionZ + 1);

         //find the max def for this tile
         float x1 = (float)Math.Abs(a.X - b.X);
         float x2 = (float)Math.Abs(c.X - d.X);
         float xM = x1 > x2 ? x1 : x2;
         float xPT = xM > 0 ? (intpt.X - a.X) / xM : 0;   //gives us percentage IN THE TILE

         float z1 = (float)Math.Abs(a.Z - c.Z);
         float z2 = (float)Math.Abs(b.Z - d.Z);
         float zM = z1 > z2 ? z1 : z2;
         float zPT = zM > 0 ? (intpt.Z - a.Z) / zM : 0;   //gives us percentage IN THE TILE

         int xCenterInPixelSpace = (int)((xPT ) + (mVisTileIntetersectionX ));
         int zCenterInPixelSpace = (int)((zPT ) + (mVisTileIntetersectionZ ));
         int startX = xCenterInPixelSpace - (nW >> 1);
         int startY = zCenterInPixelSpace - (nH >> 1);

         //copy back to masking
         for (int i = 0; i < nW;i++ )
         {
            for (int j = 0; j < nH; j++)
            {
               int imgIndx = j + i * nW;
               float factor = 0;
               Masking.isPointSelected(startX + i, startY + j, ref factor);
               Masking.addSelectedVert(startX+i, startY+j, Math.Max(tImg[imgIndx]/255.0f,factor) );
            }
         }
         
         //UGG, could be faster!
         Masking.rebuildVisualsAfterSelection();
         

         tImgRotated = null;
         tImgResized=null;
         tImg = null;
         imgDat = null;
        // imgDatFull = null;
      }

      public int giveDominantTextureAtTile(int x, int z, BTerrainSimRep simRep)
      {
         //CLM - This breaks the batch exporting process!!!!
         if ((simRep == null))
            return 0;

         if (x < 0 || z < 0 || x >= simRep.getNumXTiles() || z >= simRep.getNumXTiles())
            return -1;

         // Weight accumulation array
         int numActiveTextures = SimTerrainType.mActiveWorkingSet.Count;
         float []weights = new float[numActiveTextures];
   
         // Convert tile coord to vert coord
         BTerrainTextureVector tdat = new BTerrainTextureVector();
         float simToVisScale = 1.0f / simRep.getVisToSimScale();
         int iSimToVisScale = (int)simToVisScale;
         x *= iSimToVisScale;
         z *= iSimToVisScale;

         int numXVerts = TerrainGlobals.getTerrain().getNumXVerts();
         int quadNodeSize = (int)BTerrainQuadNode.cMaxWidth;
         int numQuadsPerAxis = numXVerts / quadNodeSize;

         // Iterate through each vertex in the tile
         int maxX = Math.Min(x + iSimToVisScale, numXVerts - 1);
         int maxZ = Math.Min(z + iSimToVisScale, numXVerts - 1);

         BTerrainQuadNode[] bqn = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

         for (int i = x; i <= maxX; i++)
         {
            for (int j = z; j <= maxZ; j++)
            {
               int qX = i / quadNodeSize;
               int qZ = j / quadNodeSize;
               int quadNodeIndex = qX * numQuadsPerAxis + qZ;
               int xOffset = i - qX * quadNodeSize;
               int zOffset = j - qZ * quadNodeSize;
               int alphaTextureIndex = xOffset * quadNodeSize + zOffset;

               BTerrainLayerContainer vdat = bqn[quadNodeIndex].mLayerContainer;

               // Iterate through each layer
               int numLayers = vdat.getNumLayers();
               float weight = 1.0f;
               float k = 1.0f / 255.0f;
               for (int layerIndex = numLayers - 1; layerIndex >= 0; layerIndex--)
               {
                  BTerrainTexturingLayer layer = vdat.giveLayer(layerIndex);

                  if (layer.mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                  {
                     float fAlphaContrib = (float)layer.mAlphaLayer[alphaTextureIndex] * weight * k;
                     weight -= fAlphaContrib;

                     // Accumulate the weight for the layer's active texture index
                     weights[layer.mActiveTextureIndex] += fAlphaContrib;

                     if (weight <= 0.0f)
                        break;
                  }
               }
            }
         }

         // Scan the accumulation array to find the dominant weight
         float dominantWeight = 0.0f;
         int dominantIndex = -1;
         for (int i = 0; i < numActiveTextures; i++)
         {
            float w = weights[i];
            if (w > dominantWeight)
            {
               dominantWeight = w;
               dominantIndex = i;
            }
         }

         // Convert active working texture index to global texture index
         if (dominantIndex != -1)
            dominantIndex = SimTerrainType.mActiveWorkingSet[dominantIndex];

         return dominantIndex;
      }

      public BTerrainTextureVector giveTextureDataAtVertex(int x, int z)
      {
         
         if (x < 0 || z < 0 || x >= TerrainGlobals.getTerrain().getNumXVerts() || z >= TerrainGlobals.getTerrain().getNumZVerts())
            return null;

         BTerrainQuadNode node = TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingTile(x, z);
         if (node == null)
            return null;

         int localX = x - node.getDesc().mMinXVert;
         int localZ = z - node.getDesc().mMinZVert;

         BTerrainTextureVector vdat = node.mLayerContainer.giveLayerChainAtPixel(localX, localZ);

         return vdat;
      }
      public void clearTextureDataAtVertex(int x, int z)
      {
         if (x < 0 || z < 0 || x >= TerrainGlobals.getTerrain().getNumXVerts() || z >= TerrainGlobals.getTerrain().getNumZVerts())
            return;

         BTerrainQuadNode node = TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingPoint(x, z);
         if (node == null)
            return;

         int localX = x - node.getDesc().mMinXVert;
         int localZ = z - node.getDesc().mMinZVert;

         node.mLayerContainer.clearAllAlphasAtPixel(localX, localZ);

         return;
      }
      public BTerrainLayerContainer generateContainerFromTexDeformations(int minxVert, int minzVert)
      {
         BTerrainLayerContainer cont = new BTerrainLayerContainer();
         for (int i = 0; i < TerrainGlobals.getTexturing().getActiveTextureCount(); i++)
            cont.newSplatLayer(i);

         bool[,] containsTexDef = new bool[BTerrainTexturing.getAlphaTextureWidth(), BTerrainTexturing.getAlphaTextureHeight()];
         bool containsTexDeformations = false;
         for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
         {
            for (int z = 0; z < BTerrainTexturing.getAlphaTextureHeight(); z++)
            {
               long vIndex = (minxVert + x) + (minzVert + z) * TerrainGlobals.getTerrain().getNumXVerts();

               BTerrainTextureVector v = mCurrBrushDeformationTextures.GetValue(vIndex);
               if (v == null || v.getNumLayers() == 0)
               {
                  containsTexDef[x,z]=false;
                  continue;
               }
               else
               {
                  containsTexDeformations = true;
                  containsTexDef[x,z]=true;
               }

                  

               

               int aIndex = (int)(x + z * BTerrainTexturing.getAlphaTextureHeight());

               for (int k = 0; k < v.getNumLayers(); k++)
               {
                  if (cont.containsID(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType))
                  {
                     cont.giveLayer(cont.giveLayerIndex(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType)).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                  }
                  else
                  {
                     int lIdx = 0;
                     if (v.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                        lIdx = cont.newSplatLayer(v.mLayers[k].mActiveTextureIndex);
                     else
                        lIdx = cont.newDecalLayer(v.mLayers[k].mActiveTextureIndex);

                     cont.giveLayer(lIdx).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                  }
               }
            }
         }


         // calling TerrainGlobals.getEditor().giveTextureDataAtVertex(minxVert + x, minzVert + z)
         //is pretty costly, so only do it if we ACTUALLY have defomations to do!
         if (containsTexDeformations)
         {


            for (int x = 0; x < BTerrainTexturing.getAlphaTextureWidth(); x++)
            {
               for (int z = 0; z < BTerrainTexturing.getAlphaTextureHeight(); z++)
               {
                  long vIndex = (minxVert + x) + (minzVert + z) * TerrainGlobals.getTerrain().getNumXVerts();

                  
                  if (containsTexDef[x, z])
                     continue;

                  BTerrainTextureVector v =  TerrainGlobals.getEditor().giveTextureDataAtVertex(minxVert + x, minzVert + z);
                  

                  int aIndex = (int)(x + z * BTerrainTexturing.getAlphaTextureHeight());

                  for (int k = 0; k < v.getNumLayers(); k++)
                  {
                     if (cont.containsID(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType))
                     {
                        cont.giveLayer(cont.giveLayerIndex(v.mLayers[k].mActiveTextureIndex, v.mLayers[k].mLayerType)).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                     }
                     else
                     {
                        int lIdx = 0;
                        if (v.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Splat)
                           lIdx = cont.newSplatLayer(v.mLayers[k].mActiveTextureIndex);
                        else
                           lIdx = cont.newDecalLayer(v.mLayers[k].mActiveTextureIndex);

                        cont.giveLayer(lIdx).mAlphaLayer[aIndex] = v.mLayers[k].mAlphaContrib;
                     }
                  }
               }
            }

            return cont;
         }

         return null;
      }

      public void selectedTextureToMask()
      {
         TextureToMask(TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);
      }

      public void TextureToMask(int selTexIndex)
      {
         if (!UIManager.GetAsyncKeyStateB(Key.LeftShift))
            Masking.clearSelectionMask();

         //walk all leaf nodes
         BTerrainQuadNode [] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();

         int width = (int)BTerrainTexturing.getAlphaTextureWidth();
         int height = (int)BTerrainTexturing.getAlphaTextureHeight();
         for(int i=0;i<nodes.Length;i++)
         {
            int index = nodes[i].mLayerContainer.giveLayerIndex(selTexIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat);
            if(index != -1)
            {
               BTerrainTexturingLayer l = nodes[i].mLayerContainer.giveLayer(index);
               int minXV = (int)nodes[i].getDesc().mMinXVert;
               int minZV = (int)nodes[i].getDesc().mMinZVert;
               for (int x = 0; x < width; x++)
               {
                  for (int z = 0; z < height; z++)
                  {
                     byte val = l.mAlphaLayer[x + z * BTerrainTexturing.getAlphaTextureWidth()];
                     float factor = 0;
                     Masking.isPointSelected((minXV + x), (minZV + z), ref factor);
                     factor = Math.Max(factor, val / 255.0f);
                     Masking.addSelectedVert((minXV + x), (minZV + z), factor);
                  }
               }
            }
         }
         Masking.rebuildVisualsAfterSelection();
      }

      #endregion

      #region POSITION REGION
      //positions editing
      public enum eBrushShape
      {
         cCircleBrush = 0,
         cSquareBrush,
         cBrushShapeCount
      };
      public void setBrushShape(eBrushShape shape) { mBrushShape = shape; }
      eBrushShape getBrushShape() { return mBrushShape; }


      public bool vertInBrushArea(ref Vector3 vert, ref Vector3 applyPt, float radius, float hotspot, int curveType, BrushInfo.eIntersectionShape brushIntersectionShape, out float inflFactor)
      {
         if (mBrushShape == eBrushShape.cCircleBrush)
         {
            // Get distance from this vertex to the center point.
            float diff_X = applyPt.X - vert.X;
            float diff_Y = applyPt.Y - vert.Y;
            float diff_Z = applyPt.Z - vert.Z;

            if (brushIntersectionShape == BrushInfo.eIntersectionShape.Cylinder)
               diff_Y = 0.0f;

            float distance = (float) Math.Sqrt(diff_X * diff_X + diff_Y * diff_Y + diff_Z * diff_Z);

            // Skip if too far.
            if (distance <= radius)
            {
               if (distance <= hotspot)
               {
                  inflFactor = 1.0f;
               }
               else
               {
                  float falloffValue = (distance - hotspot) / (radius - hotspot);

                  switch (curveType)
                  {
                     case 0:
                        inflFactor = (float)Math.Sin((1.0f - falloffValue) * (1.570796326795f));
                        break;
                     case 1:
                        inflFactor = (float)(1.0f - falloffValue);
                        break;
                     case 2:
                     default:
                        inflFactor = 1.0f + (float)Math.Sin(falloffValue * (-1.570796326795f));
                        break;
                  }

               }

               return true;
            }
         }
         else if (mBrushShape == eBrushShape.cSquareBrush)
         {
            /*
            // Get distance from this vertex to the center point.
            float dx = x * TerrainGlobals.getTerrain().getTileScale() - applyPt.X;
            float dz = z * TerrainGlobals.getTerrain().getTileScale() - applyPt.Z;
            float distance = (float)Math.Sqrt(dx * dx + dz * dz);

            {
               float halfPiOverRadius = (1.570796326795f) / radius; // cPiOver2/radius

               // Get height factor based on distance from center.
               inflFactor = 1f;//sin((radius-distance)*halfPiOverRadius);

               return true;
            }
            */
         }

         inflFactor = 0;
         return false;
      }

      public bool newHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getHeightBrush();

         return true;//mCurrBrush.init();
      }
      public bool newPushBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getPushBrush();

         return true;//mCurrBrush.init();
      }
      public bool newAvgHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getAvgHeightBrush();

         return true;//mCurrBrush.init();
      }
      public bool newStdBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getStdBrush();

         return true;//mCurrBrush.init();
      }
      public bool newLayerBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getLayerBrush();

         return true;//mCurrBrush.init();
      }
      public bool newInflateBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getInflateBrush();

         return true;//mCurrBrush.init();
      }
      public bool newSmoothBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSmoothBrush();

         return true;//mCurrBrush.init();
      }
      public bool newPinchBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getPinchBrush();

         return true;//mCurrBrush.init();
      }
      public bool newSetHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSetHeightBrush();

         return true;//mCurrBrush.init();
      }
      public bool newUniformBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getUniformBrush();

         return true;//mCurrBrush.init();
      }
      public bool newScaleBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getScalarBrush();

         return true;//mCurrBrush.init();
      }
      public bool newSkirtHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSkirtHeightBrush();

         return true;//mCurrBrush.init();
      }
      public bool newAlphaBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getAlphaBrush();
         return true;
      }
      private void applyVertBrush(bool alternate, bool dot)
      {
         applyVertBrush(alternate, dot, false);
      }

      Vector3 LastintPoint;
      Vector3 LastintNormal;
      BTerrainQuadNode Lastnode = null;

      //vertex undo data
      class VertUndoData : UndoInstanceData
      {
         public VertUndoData()
         {
            mMemorySize = 0;
         }
         ~VertUndoData()
         {
            if (affectedQNs != null)
            {
               for (int i = 0; i < affectedQNs.Count; i++)
                  affectedQNs[i] = null;
               affectedQNs.Clear();
               affectedQNs = null;
            }
         }

         public class vertexUndoQN
         {
            ~vertexUndoQN()
            {
               mVerts = null;
            }
            public Vector3[] mVerts;
            public int minXVert;
            public int minZVert;
            public BTerrainQuadNode mpNodePtr;
         }

         public List<vertexUndoQN> affectedQNs= new List<VertUndoData.vertexUndoQN>();
      }


      public void vertexUndoCallback( UndoInstanceData pkt)
      {
         clearBrushDeformations();

         int width = (int)BTerrainQuadNode.cMaxWidth;
         int height = (int)BTerrainQuadNode.cMaxHeight;

         VertUndoData vud = (VertUndoData)pkt;
         for (int i = 0; i < vud.affectedQNs.Count; i++)
         {
            BTerrainQuadNode node = vud.affectedQNs[i].mpNodePtr;//] TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingPoint(vud.affectedQNs[i].minXVert + (width >> 1), vud.affectedQNs[i].minZVert + (height >> 1));
            for (int x = 0; x < width; x++)
            {
               for (int z = 0; z < height; z++)
               {
                  int dstindex = (z + vud.affectedQNs[i].minZVert) + (x + vud.affectedQNs[i].minXVert) * TerrainGlobals.getTerrain().getNumXVerts();
                  int srcIndex = x + z * width;

                  mDetailPoints[dstindex] = vud.affectedQNs[i].mVerts[srcIndex];
               }
            }


        
            node.mDirty = true;
            node.clearVisibleDatHandle();

            BTerrainQuadNodeDesc desc = node.getDesc();
         //   SimGlobals.getSimMain().updateHeightsFromTerrain(desc.m_min, desc.m_max);

            BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                 TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                 desc.mMinXVert, desc.mMaxXVert, desc.mMinZVert, desc.mMaxZVert);
   
         }

         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         
      }
      void vertexCurrentUndoPacket()
      {
         //this packet has been pushed and cleared.
         if (mCurrentUndoPacket == null)
         {
            mCurrentUndoPacket = new UndoPacket();
            mCurrentUndoPacket.mCallbackFunction = vertexUndoCallback;

            mCurrentUndoPacket.mPrevData = new VertUndoData();

            mCurrentUndoPacket.mCurrData = new VertUndoData();


         }
      }
      public void addVertexUndoNodes(List<BTerrainQuadNode> nodes,bool isCurrent)
      {
         vertexCurrentUndoPacket();
         VertUndoData vud = isCurrent ? (VertUndoData)mCurrentUndoPacket.mCurrData : (VertUndoData)mCurrentUndoPacket.mPrevData;

         addVertexUndoNodes(vud, nodes.ToArray(),  isCurrent);
      }
      public void addVertexUndoNodes(BTerrainQuadNode[] nodes, bool isCurrent)
      {
         vertexCurrentUndoPacket();
         VertUndoData vud = isCurrent ? (VertUndoData)mCurrentUndoPacket.mCurrData : (VertUndoData)mCurrentUndoPacket.mPrevData;

         addVertexUndoNodes(vud, nodes,  isCurrent);
      }
      void addVertexUndoNodes(VertUndoData vud, BTerrainQuadNode[] nodes, bool isCurrent)
      {
      
         if (mbVertexUndo == false)
            return;
        

         
         for (int i = 0; i < nodes.Length; i++)
         {
            //do we already contain this node?
            int found = -1;
            for (int x = 0; x < vud.affectedQNs.Count; x++)
            {
               if (vud.affectedQNs[x].mpNodePtr == nodes[i])
               {
                  found = x;
                  break;
               }
            }
            int width = (int)BTerrainQuadNode.cMaxWidth;
            int height = (int)BTerrainQuadNode.cMaxHeight;

            bool add = false;
            int c = vud.affectedQNs.Count;

            if (!isCurrent)
            {
               if(found!=-1)
                  continue;

               vud.affectedQNs.Add(new VertUndoData.vertexUndoQN());
               vud.affectedQNs[c].mVerts = new Vector3[width * height];
               vud.affectedQNs[c].minXVert = nodes[i].getDesc().mMinXVert;
               vud.affectedQNs[c].minZVert = nodes[i].getDesc().mMinZVert;
               vud.affectedQNs[c].mpNodePtr = nodes[i];
               vud.mMemorySize += width * height * sizeof(float) * 3;
               

               add = true;
            }
            else
            {
               c = found;
               if(found==-1)
               {
                  c = vud.affectedQNs.Count;
                  vud.affectedQNs.Add(new VertUndoData.vertexUndoQN());
                  vud.affectedQNs[c].mVerts = new Vector3[width * height];
                  vud.affectedQNs[c].minXVert = nodes[i].getDesc().mMinXVert;
                  vud.affectedQNs[c].minZVert = nodes[i].getDesc().mMinZVert;
                  vud.affectedQNs[c].mpNodePtr = nodes[i];
                  vud.mMemorySize += width * height * sizeof(float) * 3;
               }
               add = true;
            }

            if (add)
            {
               for (int x = 0; x < width; x++)
               {
                  for (int z = 0; z < height; z++)
                  {
                     int srcindex = (x + nodes[i].getDesc().mMinXVert) + (z + nodes[i].getDesc().mMinZVert) * TerrainGlobals.getTerrain().getNumXVerts();
                     int dstIndex = x + z * width;

                     if(isCurrent)
                        vud.affectedQNs[c].mVerts[dstIndex] = TerrainGlobals.getTerrain().getPostDeformRelPos((x + nodes[i].getDesc().mMinXVert), (z + nodes[i].getDesc().mMinZVert));// mDetailPoints[srcindex];
                     else
                        vud.affectedQNs[c].mVerts[dstIndex] = TerrainGlobals.getTerrain().getRelPos((x + nodes[i].getDesc().mMinXVert), (z + nodes[i].getDesc().mMinZVert));// mDetailPoints[srcindex];
                  }
               }
            }
         }
      }
      

      private void applyVertBrush(bool alternate, bool dot, bool useLastPostition)
      {
         if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
         {
            //get our view ray
            /* Vector3 orig = getRayPosFromMouseCoords(false);
             Vector3 dir = getRayPosFromMouseCoords(true) - orig;
             dir = BTerrain.Normalize(dir);
             */
            //get our intersect point
            Vector3 intPoint = mBrushIntersectionPoint;
            Vector3 intNormal = mBrushIntersectionNormal;
            BTerrainQuadNode node = mBrushIntersectionNode;

            if (node != null && useLastPostition == true)
            {
               intPoint = LastintPoint;
               intNormal = LastintNormal;
               node = Lastnode;
            }
            else
            {
               LastintPoint = intPoint;
               LastintNormal = intNormal;
               Lastnode = node;
            }

            // Get intersection type
            BrushInfo.eIntersectionShape intersectionType = mBrushInfo.mIntersectionShape;

            //cast our ray
            // if (TerrainGlobals.getTerrain().rayIntersects(ref orig, ref dir, ref intPoint, ref intNormal, ref node, true))
            {

               if (mCurrMode != eEditorMode.cModeVertSkirtHeight)
               {
                  List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

                  if (mCurrMode != eEditorMode.cModePasteMode)
                  {

                     // For Dot brushes
                     if (dot)
                     {
                        TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mCurrBrushExtends.minX, mCurrBrushExtends.maxX,
                                                                                                 mCurrBrushExtends.minZ, mCurrBrushExtends.maxZ);

                        TerrainGlobals.getEditor().clearBrushDeformations();

                        // Clear undo
                        mVertBackup.Clear();
                     }

                     // Undo info
                     float validRadius = (mBrushInfo.mRadius * 1.5f);   //scaled out for dramatic changes


                     if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
                        TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref intPoint, validRadius);
                     else
                        TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(nodes, ref intPoint, validRadius);


                     Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
                     //for (int i = 0; i < nodes.Count; i++)
                     //{
                     //   int code = nodes[i].GetHashCode();
                     //   if (mVertBackup.ContainsKey(code) == false)
                     //   {
                     //      mVertBackup.Add(code, new VertexBackup(nodes[i], detail));
                     //   }
                     //}

                        
                    addVertexUndoNodes(nodes,false);

                     // Find affected points
                     List<int> points = new List<int>();
                     if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
                        TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(points, ref intPoint, mBrushInfo.mRadius);
                     else
                        TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(points, ref intPoint, mBrushInfo.mRadius);

                     ((BTerrainVertexBrush)mCurrBrush).applyOnBrush(points, ref intPoint, ref intNormal, ref mBrushInfo, alternate);

                     addVertexUndoNodes(nodes, true);

                  }
                  else
                  {

                  }
                  
                  /**/
                  //update our affected quad nodes.
                  for (int i = 0; i < nodes.Count; i++)
                  {
                     nodes[i].mDirty = true;
                     //    BTerrainQuadNodeDesc desc=nodes[i].getDesc();
                     //     mSimRep.updateSimRep(ref nodes[i].mSimVisHandle, desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
                     BTerrainQuadNodeDesc desc = nodes[i].getDesc();
                 //    SimGlobals.getSimMain().updateHeightsFromTerrain(desc.m_min, desc.m_max);
                  }

                  TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
               }
               else
               {
                  // Find affected skirt points
                  List<int> points = new List<int>();
                  getCylinderIntersectionSkirt(points, ref intPoint, mBrushInfo.mRadius);

                  ((BTerrainVertexBrush)mCurrBrush).applySkirtOnBrush(points, ref intPoint, ref intNormal, ref mBrushInfo, alternate);


                  TerrainGlobals.getRender().updateAllSkirtTextures();
                  /*
                  // Update only the affect quadrants
                  List<int> quadrants = new List<BTerrainQuadNode>();
                  TerrainGlobals.getTerrain().getQuadrantCylinderIntersection(quadrants, ref intPoint, validRadius);

                  for (int i = 0; i < quadrants.Count; i++)
                  {
                     TerrainGlobals.getRender().updateSkirtTexture(quadrants[i]);
                  }
                  */
               }
            }
         }
         else if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
         {
            if (mCurrMode == eEditorMode.cModePasteMode)
            {
               return;
            }

            float multiplier = 10;
            if (eEditorMode.cModeVertSmooth == mCurrMode)
               multiplier = 1;

            if (eEditorMode.cModeVertUniform == mCurrMode)
               multiplier = 30;

            if (eEditorMode.cModeVertScale == mCurrMode)
               multiplier = 4;

            // Apply on selection

            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
                                                                                     Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);

            Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
            //for (int i = 0; i < nodes.Count; i++)
            //{
            //   int code = nodes[i].GetHashCode();
            //   if (mVertBackup.ContainsKey(code) == false)
            //   {
            //      mVertBackup.Add(code, new VertexBackup(nodes[i], detail));
            //      //mNodeBackup.Add(code, nodes[i].copy());
            //   }
            //}
            addVertexUndoNodes(nodes, false);


            ((BTerrainVertexBrush)mCurrBrush).applyOnSelection(Masking.getCurrSelectionMaskWeights(), multiplier * this.mBrushInfo.mIntensity, alternate);

            addVertexUndoNodes(nodes, true);
            PushChanges();

            for (int i = 0; i < nodes.Count; i++)
            {
               nodes[i].mDirty = true;
            }

            TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         }
      }

      private void applyMaskVertBrush(bool alternate)
      {
         Vector3 intPoint = mBrushIntersectionPoint;
         Vector3 intNormal = mBrushIntersectionNormal;
         BTerrainQuadNode node = mBrushIntersectionNode;
         //cast our ray
         //if (TerrainGlobals.getTerrain().rayIntersects(ref orig, ref dir, ref intPoint, ref intNormal, ref node, true))
         {
            // Get intersection type
            BrushInfo.eIntersectionShape intersectionType = mBrushInfo.mIntersectionShape;

            // Find affected nodes
            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
               TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref intPoint, mBrushInfo.mRadius);
            else
               TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(nodes, ref intPoint, mBrushInfo.mRadius);

            // Find affected points
            List<int> points = new List<int>();
            if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
               TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(points, ref intPoint, mBrushInfo.mRadius);
            else
               TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(points, ref intPoint, mBrushInfo.mRadius);

            // Go through points and adjust accordingly.
            for (int i = 0; i < points.Count; i++)
            {
               int index = points[i];

               int x = index / TerrainGlobals.getTerrain().getNumZVerts();
               int z = index % TerrainGlobals.getTerrain().getNumZVerts();


               Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);
               Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformRelPos(x, z);

               float factor = 0f;
               if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref intPoint, mBrushInfo.mRadius, mBrushInfo.mHotspot * mBrushInfo.mRadius, mBrushInfo.mCurveType, intersectionType, out factor))
                  continue;

               float curWeight = 0.0f;
               Masking.isPointSelected(index, ref curWeight);

               factor *= mBrushInfo.mIntensity;

               float newWeight;
              //CLM we don't care about inversion any more.
                  if (!alternate)
                     newWeight = (curWeight > factor) ? curWeight : factor;
                  else//CLM Changes to erasing..
                     newWeight = BMathLib.Clamp(curWeight - factor,0,1);
               
              

               Masking.addSelectedVert(x, z, newWeight);
            }



            for (int i = 0; i < nodes.Count; i++)
            {
               nodes[i].mDirty = true;
            }

            TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         }
      }

      public void viewLODEval()
      {
         int mWidth = TerrainGlobals.getTerrain().getNumXVerts();
         int mHeight = TerrainGlobals.getTerrain().getNumZVerts();

         int nodeWidth = 16;
         int numXPatches = (int)(mWidth / (float)nodeWidth);
         int numZPatches = (int)(mHeight / (float)nodeWidth);
         int numXChunks = (int)(mWidth / (float)BTerrainQuadNode.getMaxNodeDepth());
         int numPatches = numXPatches * numZPatches;
         Export360.ExportSettings es = TerrainGlobals.getTerrain().getExportSettings();
         float eLOD = es.RefineEpsilon;
         float mB = es.RefineMinorityBias;
         Refinement.ErrorMetricRefine refiner  = new Refinement.ErrorMetricRefine();
         refiner.init(mWidth, mHeight);
         refiner.refine(eLOD);
         //refiner.refineTerrain(eLOD, nodeWidth + 1);


         Masking.clearSelectionMask();
         for (int x = 0; x < mWidth; x++)
         {
            for (int z = 0; z < mHeight; z++)
            {
               int xMinVert = x * nodeWidth;
               int zMinVert = z * nodeWidth;

               bool used = refiner.getMarkedPt(x, z);
               if (used)
                  Masking.addSelectedVert(x, z, 1.0f);
            }
         }

         Masking.rebuildVisualsAfterSelection();
         refiner = null;
      }

      public void setScaleBrushBrushMode(bool doXZOnly)
      {
         if (mCurrBrush is BTerrainScalarBrush)
            ((BTerrainScalarBrush)mCurrBrush).mOnlyDoXZ = doXZOnly;
      }

      public void setSmudgeBrushBrushMode(bool museNormals)
      {
         if (mCurrBrush is BTerrainAvgHeightBrush)
            ((BTerrainAvgHeightBrush)mCurrBrush).mUseIntersectionNormal = museNormals;
      }


      #endregion
      
      #region BRUSH UI VALUES REGION
      public void setVertBrushRadius(float rad) { mBrushInfo.mRadius = rad * mBrushRadiusFactor; }
      public void setVertBrushHotspot(float rad) { mBrushInfo.mHotspot = rad; }
      public void setVertBrushIntensity(float intensity) { mBrushInfo.mIntensity = intensity; }
      public void setVertBrushCurveType(int curveType) { mBrushInfo.mCurveType = curveType; }
      public void setVertBrushIntersectionShape(BrushInfo.eIntersectionShape intersectionType) { mBrushInfo.mIntersectionShape = intersectionType; }
      public void setTextureBrushRotation(float rad) { mBrushInfo.mRotation = rad;}

      public void setVertBrushRadiusFactor(float radFactor) { mBrushRadiusFactor = radFactor; }
      #endregion

      #region SIM EDIT REGION
      public bool newSimFloodPassableBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimFloodPassibilityBrush();

         mSimRep.setChannel(BTerrainSimRep.eChannels.cFloodObstructionChannel);
         return true;
      }
      public bool newSimScarabPassableBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimScarabPassibilityBrush();

         mSimRep.setChannel(BTerrainSimRep.eChannels.cScarabObstructionChannel);
         return true;
      }
      public bool newSimTileTypeBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimTileTypeBrush();

         mSimRep.setChannel(BTerrainSimRep.eChannels.cTileTypeChannel);
         return true;
      }
      public bool newSimBuildabilityBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimBuildabilityBrush();

         mSimRep.setChannel(BTerrainSimRep.eChannels.cBuildableChannel);
         return true;
      }
      public bool newSimPassibilityBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimPassibilityBrush();

         mSimRep.setChannel(BTerrainSimRep.eChannels.cObstrtuctionChannel);
         return true;
      }
      public bool newSimHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimHeightOverrideBrush();

         return true;
      }
      public bool newSimSetHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimSetHeightOverrideBrush();

         return true;
      }
      public bool newSimSmoothBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimSmoothOverrideBrush();

         return true;
      }
      public bool newSimEraseHeightBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getSimEraseHeightOverrideBrush();

         return true;
      }

      //private void applySimBrush(bool alternate)
      //{
      //   if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
      //   {
      //      //get our intersect point
      //      Vector3 intPoint = mBrushIntersectionPoint;
      //      Vector3 intNormal = mBrushIntersectionNormal;
      //      BTerrainQuadNode node = mBrushIntersectionNode;

      //      int xTileIntersect = mVisTileIntetersectionX;
      //      int zTileIntersect = mVisTileIntetersectionZ;
      //      // Get intersection type
      //      BrushInfo.eIntersectionShape intersectionType = mBrushInfo.mIntersectionShape;

      //      float validRadius = (mBrushInfo.mRadius * 2);   //scaled out for dramatic changes
                    
      //      //cast our ray for the sim
      //      //if this particular ray cast fails, then there's no modified heights in the jagged array.
      //      //which means our height values have been created from the terrain positions.
      //      //so use the passed in intersection point & normal values..
      //       Vector3 orig = getRayPosFromMouseCoords(false);
      //       Vector3 dir = getRayPosFromMouseCoords(true) - orig;
      //       dir = BMathLib.Normalize(dir);

      //      // TerrainGlobals.getTerrain().getQuadNodeRoot().rayIntersectsSimOverrideHeights(ref orig, ref dir, ref intPoint, ref intNormal, ref xTileIntersect, ref zTileIntersect, ref node);
      //       getSimRep().getHeightOverrideIntersection(orig, dir, ref intPoint, ref xTileIntersect, ref zTileIntersect);

      //      //scale the tile intersect back to whatever our simrep density is...
            

      //      {
               

      //            {
      //               // Find affected points
      //               List<int> points = new List<int>();
      //               int numXVerts = TerrainGlobals.getTerrain().getNumXVerts();
      //               float ts = TerrainGlobals.getTerrain().getTileScale();
                     
      //               int minX = (int)(BMathLib.Clamp(xTileIntersect - validRadius, 0, numXVerts-1));
      //               int maxX = (int)(BMathLib.Clamp(xTileIntersect + validRadius, 0, numXVerts - 1));
      //               int minZ = (int)(BMathLib.Clamp(zTileIntersect - validRadius, 0, numXVerts - 1));
      //               int maxZ = (int)(BMathLib.Clamp(zTileIntersect + validRadius, 0, numXVerts - 1));

      //               for (int x = minX; x <= maxX; x++)
      //               {
      //                  for (int z = minZ; z <= maxZ; z++)
      //                  {
      //                     Vector3 vert = Vector3.Empty;

      //                     int indx = x * numXVerts + z;

      //                     float overideHeight = TerrainGlobals.getEditor().getSimRep().getHeightRep().getJaggedHeight(indx);
      //                     if (overideHeight != SimHeightRep.cJaggedEmptyValue)
      //                     {
      //                        vert = new Vector3(x * ts, overideHeight, z * ts);
      //                     }
      //                     else
      //                     {
      //                        //CLM i'd prefer this be a vertical raycast incase of XZ deformation...
      //                        vert = TerrainGlobals.getTerrain().getPos(x, z);
      //                     }

      //                     if (Vector3.LengthSq(vert - intPoint) < validRadius * validRadius)
      //                        points.Add(indx);
                           
      //                  }
      //               }
                     


      //               //APPLY!!
      //               ((BTerrainSimBrush)mCurrBrush).applyOnBrush(points, ref intPoint, ref intNormal, ref mBrushInfo, alternate);

      //            }


      //         //find our affected quad nodes
      //         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
      //         if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
      //            TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref intPoint, validRadius);
      //         else
      //            TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(nodes, ref intPoint, validRadius);


      //         //update dirty stuff...
      //         for (int i = 0; i < nodes.Count; i++)
      //         {
      //            BTerrainQuadNodeDesc desc = nodes[i].getDesc();

      //            getSimRep().getDataTiles().updateLandObstructions(TerrainGlobals.getTerrain().getTileScale(), (int)(desc.mMinXVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMinZVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMaxXVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMaxZVert * getSimRep().getVisToSimScale()));
      //            getSimRep().getDataTiles().updateLandObstructionsPainted(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
      //            getSimRep().getDataTiles().updateBoundsObstructions(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);

      //            getSimRep().getDataTiles().updateBuildablePainted(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
      //            nodes[i].mDirty = true;
      //         }
      //         TerrainGlobals.getTerrain().getQuadNodeRoot().rebuildSimHandleIfDirty();
               
      //      }
      //   }
      //   else if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
      //   {
            

      //      // Apply on selection

      //      List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
      //      TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
      //                                                                               Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);

      //      Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
          
            
      //      ((BTerrainSimBrush)mCurrBrush).applyOnSelection(Masking.getCurrSelectionMaskWeights(),  this.mBrushInfo.mIntensity, alternate);


      //      for (int i = 0; i < nodes.Count; i++)
      //      {
      //         BTerrainQuadNodeDesc desc = nodes[i].getDesc();

      //         getSimRep().getDataTiles().updateLandObstructions(TerrainGlobals.getTerrain().getTileScale(), (int)(desc.mMinXVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMinZVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMaxXVert * getSimRep().getVisToSimScale()),
      //                                                                                           (int)(desc.mMaxZVert * getSimRep().getVisToSimScale()));
      //         getSimRep().getDataTiles().updateLandObstructionsPainted(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
      //         getSimRep().getDataTiles().updateBoundsObstructions(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
      //         getSimRep().getDataTiles().updateBuildablePainted(TerrainGlobals.getTerrain().getNumXVerts(), desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert);
      //      }
      //   //   TerrainGlobals.getTerrain().getQuadNodeRoot().rebuildSimHandle();
      //   }
      //}

      public void maskFromSimPassibility()
      {
         Masking.clearSelectionMask();
         Masking.clearSelectionMask();
         TerrainGlobals.getTerrainFrontEnd().UpdateSimRep();

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         int height = TerrainGlobals.getTerrain().getNumXVerts();

         for(int x=0;x<width;x++)
         {
            for (int z = 0; z < height; z++)
            {
               int simX = 0;
               int simZ = 0;
               mSimRep.giveSimTileXZfromVisVertXZ(x, z, ref simX, ref simZ);
               if (mSimRep.getDataTiles().isTileLandObstructed(simX, simZ))
               {
                  Masking.addSelectedVert(x, z, 1);
               }
            }
         }

      }

      #endregion

      #region FOLIAGE REGION
      bool mRenderFoliage = true;
      public void toggleViewFoliage() { mRenderFoliage = !mRenderFoliage; if (!mRenderFoliage)FoliageManager.clearAllChunkVBs(); }
      public bool newFoliageBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getFoliageBrush();

         return true;
      }
      private void applyFoliageBrush(bool alternate)
      {
         if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputMouse)
         {
            //get our intersect point
            Vector3 intPoint = mBrushIntersectionPoint;
            Vector3 intNormal = mBrushIntersectionNormal;
            BTerrainQuadNode node = mBrushIntersectionNode;

            // Get intersection type
            BrushInfo.eIntersectionShape intersectionType = mBrushInfo.mIntersectionShape;

            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

            // Undo info
            float validRadius = (mBrushInfo.mRadius * 1.5f);   //scaled out for dramatic changes


            if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
               TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref intPoint, validRadius);
            else
               TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(nodes, ref intPoint, validRadius);


            Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
           

           // addVertexUndoNodes(nodes, false);

            // Find affected points
            List<int> points = new List<int>();
            if (intersectionType == BrushInfo.eIntersectionShape.Sphere)
               TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(points, ref intPoint, mBrushInfo.mRadius);
            else
               TerrainGlobals.getTerrain().getQuadNodeRoot().getCylinderIntersection(points, ref intPoint, mBrushInfo.mRadius);


            ((BTerrainFoliageBrush)mCurrBrush).applyOnBrush(points, ref intPoint, ref intNormal, ref mBrushInfo, 
                  alternate || mCurrMode == eEditorMode.cModeFoliageErase);

           // addVertexUndoNodes(nodes, true);
         }
         else if (mCurrStrokeInput == eEditorStrokeInput.cStrokeInputKeyboard)
         {


            int multiplier = 10;
            // Apply on selection

            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
                                                                                     Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);




            ((BTerrainFoliageBrush)mCurrBrush).applyOnSelection(Masking.getCurrSelectionMaskWeights(), this.mBrushInfo.mIntensity, 
               alternate || mCurrMode == eEditorMode.cModeFoliageErase);


         }
  
      }


      #endregion

      #region CAMERA REGION
      public bool newCameraHeightsBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getCameraHeightBrush();

         return true;
      }
      public bool newCameraSetHeightsBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getCameraSetHeightBrush();

         return true;
      }
      public bool newCameraSmoothBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getCameraSmoothBrush();

         return true;
      }
      public bool newCameraEraseBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getCameraEraseBrush();

         return true;
      }
      #endregion

      #region FLIGHT REGION
      public bool newFlightHeightsBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getFlightHeightBrush();

         return true;
      }
      public bool newFlightSetHeightsBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getFlightSetHeightBrush();

         return true;
      }
      public bool newFlightSmoothBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getFlightSmoothBrush();

         return true;
      }
      public bool newFlightEraseBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getFlightEraseBrush();

         return true;
      }
      #endregion

      //control editing
      public Vector3[] getDetailPoints() { return mDetailPoints; }
      public Vector3[] getNormals() { return mNormals; }
      public float[] getSkirtHeights() { return mSkirtHeights; }
      public float[] getAmbientOcclusionValues() { return mAmbientOcclusionValues; }
      public byte[] getAlphaValues() { return mAlphaValues; }
      public Vector3[] getLightValues() { return mLightValues; }


      //states
      private eEditorMode mCurrMode;
      private eEditorStrokeState mCurrStrokeState = eEditorStrokeState.cStrokeStateInactive;
      private eEditorStrokeInput mCurrStrokeInput = eEditorStrokeInput.cStrokeInputMouse;
      public eEditorStrokeInput getStrokeInputType() { return mCurrStrokeInput; }
      private eEditorRenderMode mRenderMode;

      public void toggleViewSim() { mSimRep.getHeightRep().mRenderHeights = !mSimRep.getHeightRep().mRenderHeights; }
      public BTerrainSimRep getSimRep() { return mSimRep; }



      //misc states
      private bool mDrawQuadNodeBounds;


      private float muScale;
      private float mvScale;

      #region MASKING
      //MASKING
      private float mMaskScalar;
      bool mShowDragBox = false;
      VertexBuffer m2DSelectionBox = null;

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
         double distance = Distance(A, B);
         double area = Area(A, B);
         //double ratio = Ratio(A, B);
         if ((distance > 20) && (area > 10))// && (ratio < 100))
            return true;
         else
            return false;
      }

      public bool mbIsDoingDragImage = true;
      Vector4[] mDragMaskNearPoints = new Vector4[2];
      float mDragMaskNearWidth = 0;
      float mDragMaskNearHeight = 0;

      public AbstractImage mCurrentAbstractImage = new AbstractImage();
      public float getDragMaskTextureValue(int x, int z, ref Matrix projection)
      {
         //ANDREW!
         if(mbIsDoingDragImage)
         {
            float imgWidth = mCurrentAbstractImage.mMaxA;
            float imgHeight = mCurrentAbstractImage.mMaxB;
            //Matrix worldViewProjection = BRenderDevice.getDevice().Transform.World * BRenderDevice.getDevice().Transform.View * BRenderDevice.getDevice().Transform.Projection;
            //Vector3 pt = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
            Vector3 pt = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
            Vector4 ptInScreenSpace = Vector3.Transform(pt, projection);

            ptInScreenSpace = ptInScreenSpace * (1/ptInScreenSpace.W);

            ptInScreenSpace.X -= mDragMaskNearPoints[0].X /5 ;  //translate to origin
            ptInScreenSpace.Y -= mDragMaskNearPoints[0].Y /5;  //translate to origin

            ptInScreenSpace.X /= mDragMaskNearWidth;        //scale between [0,1]
            ptInScreenSpace.Y /= mDragMaskNearHeight;       //scale between [0,1]

            ptInScreenSpace.Y *= 5;
            ptInScreenSpace.Y += 1f;

            ptInScreenSpace.X *= 5;
            ptInScreenSpace.X += 0f;

            //return (ptInScreenSpace.Y + ptInScreenSpace.X) * 2;
            //return (ptInScreenSpace.X);// *5;
            return mCurrentAbstractImage.Evaluate(ptInScreenSpace.X * imgWidth,  ptInScreenSpace.Y * imgHeight);

            //return mCurrentAbstractImage.Evaluate(ptInScreenSpace.X * imgWidth,  ptInScreenSpace.Y * imgHeight);

         }
        

         return 1.0f;
      }

      public void doSelectionFromShape(bool delFromSet)
      {
         if (!IsGoodBox(mPressPoint, mReleasePoint))
         {
            return;
         }

         //if shape ==square

         Vector3[] points = new Vector3[8];

         Point min = mPressPoint;
         Point max = mReleasePoint;

         if (min.X > max.X)
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

         if (mbIsDoingDragImage)
         {
            Matrix worldViewProjection = BRenderDevice.getDevice().Transform.World * BRenderDevice.getDevice().Transform.View * BRenderDevice.getDevice().Transform.Projection;
            
            mDragMaskNearPoints[0] = Vector3.Transform(points[0], worldViewProjection);
            mDragMaskNearPoints[1] = Vector3.Transform(points[3], worldViewProjection);

            mDragMaskNearWidth = Math.Abs(mDragMaskNearPoints[1].X - mDragMaskNearPoints[0].X);
            mDragMaskNearHeight = Math.Abs(mDragMaskNearPoints[1].Y - mDragMaskNearPoints[0].Y);
         }


         //find the quadnodes that our projection box is intersecting.
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getAddBoxIntersectionVertsToSelection(points, delFromSet);

         Masking.rebuildVisualsAfterSelection();

      }

      class maskLine
      {
         public Point a;
         public Point b;
         public void sortByY()   //will ensure a.y > b.y
         {
            if(a.Y < b.Y)
            {
               Point c = a;
               a = b;
               b = c;
            }
         }
         public float giveSlope()
         {
            if (a.X - b.X == 0)
               return 0;

            return (a.Y - b.Y) / (float)(a.X - b.X);
         }
      }
      class maskLinePoint : IComparable
      {
         public int X;
         public int Y;

         public int CompareTo(object y)
         {
            maskLinePoint b = y as maskLinePoint;
            return Y <= b.Y ? -1 : 1;
         }
      }

      List<maskLine> mMaskLines = new List<maskLine>();
      bool mShapeStarted = false;
      Point mMaskLineStartPoint = new Point();
      Point mMaskLinePrevPoint = new Point();

      public void addLine(Point a, Point b)
      {
         addLine(a, b, false);
      }
      public void addLine(Point a, Point b,bool drawToMask)
      {
         maskLine ml = new maskLine();
         ml.a = a;
         ml.b = b;
         mMaskLines.Add(ml);

         maskLinePoint t = new maskLinePoint();
         t.X = a.X;
         t.Y = a.Y;
         addMaskPoint(t);

         maskLinePoint j = new maskLinePoint();
         j.X = b.X;
         j.Y = b.Y;
         addMaskPoint(j);

         if(drawToMask)
         {
            //Bresenham
            int x0 = a.X;
            int y0 = a.Y;
            int x1 = b.X;
            int y1 = b.Y;
            {
               int dy = y1 - y0;
               int dx = x1 - x0;
               int stepx, stepy;

               if (dy < 0) { dy = -dy; stepy = -1; } else { stepy = 1; }
               if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }
               dy <<= 1;                                                  // dy is now 2*dy
               dx <<= 1;                                                  // dx is now 2*dx

               Masking.addSelectedVert(x0, y0, 1.0f);
               if (dx > dy)
               {
                  int fraction = dy - (dx >> 1);                         // same as 2*dy - dx
                  while (x0 != x1)
                  {
                     if (fraction >= 0)
                     {
                        y0 += stepy;
                        fraction -= dx;                                // same as fraction -= 2*dx
                     }
                     x0 += stepx;
                     fraction += dy;                                    // same as fraction -= 2*dy
                     Masking.addSelectedVert(x0, y0, 1.0f);
                  }
               }
               else
               {
                  int fraction = dx - (dy >> 1);
                  while (y0 != y1)
                  {
                     if (fraction >= 0)
                     {
                        x0 += stepx;
                        fraction -= dy;
                     }
                     y0 += stepy;
                     fraction += dx;
                     Masking.addSelectedVert(x0, y0, 1.0f);
                     
                  }
               }
            }
            Masking.rebuildVisualsAfterSelection();
         }
      }
      public void clearMaskLines()
      {
         mMaskLines.Clear();
         mShapeStarted = false;
      }
      public bool maskLinesCloseShape()
      {
         //CLM change: do this close to the end point, not the line itself.
         float epsilon = 3;

         int t = mMaskLines.Count-1;
         /*
         float kd = BMathLib.pointLineDistance( new Vector3(mMaskLines[t].a.X,0,mMaskLines[t].a.Y),
                                                new Vector3(mMaskLines[t].b.X,0,mMaskLines[t].b.Y),
                                                new Vector3(mMaskLineStartPoint.X,0,mMaskLineStartPoint.Y)
                                                );
          * */
         Vector3 a = new Vector3(mMaskLines[t].b.X,0,mMaskLines[t].b.Y);
         Vector3 b = new Vector3(mMaskLineStartPoint.X,0,mMaskLineStartPoint.Y);
         Vector3 c = b-a;
         float kd = c.Length();
         return kd<epsilon?true:false;
      }

      class Edge : IComparable//polyedge
      {		
         public double x;	/* x coordinate of edge's intersection with current scanline */
         public double dx;	/* change in x with respect to y */
         public int i;	/* edge number: edge i goes from pt[i] to pt[i+1] */

         public int CompareTo(object y)
         {
            Edge b = y as Edge;
            return x <= b.x ? -1 : 1;
         }
      }
      List<Edge> active = new List<Edge>();
      List<maskLinePoint> pts = new List<maskLinePoint>();
      void addMaskPoint(maskLinePoint pt)
      {
         for(int i=0;i<pts.Count;i++)
         {
            if (pts[i].X == pt.X && pts[i].Y == pt.Y)
               return;
         }
         pts.Add(pt);
      }


      class edgeTableElement: IComparable
      {
         public int mXInt;
         public float mSlope;
         public int yMin;
         public int CompareTo(object y)
         {
            edgeTableElement b = y as edgeTableElement;
            return mXInt <= b.mXInt ? -1 : 1;
         }
      }
      public void fillMaskFromLines(bool addOrRemove)
      {
         mMaskLines[mMaskLines.Count - 1].b = mMaskLines[0].a;

         //CLM we use a seperate grid here to ensure we're not disrupting any previous masks.
         bool[,] fillArray = new bool[TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumXVerts()];
         List<edgeTableElement> AEL = new List<edgeTableElement>();
         for (int y = 0; y < TerrainGlobals.getTerrain().getNumXVerts(); y++)
         {
            AEL.Clear();
            for (int i = 0; i < mMaskLines.Count; i++)
            {
              // mMaskLines[i].sortByY();
               int yMin = mMaskLines[i].b.Y < mMaskLines[i].a.Y ? mMaskLines[i].b.Y : mMaskLines[i].a.Y;
               int yMax = mMaskLines[i].b.Y < mMaskLines[i].a.Y ? mMaskLines[i].a.Y : mMaskLines[i].b.Y;

               if (y <= yMax && y >= yMin)
               {
                  //this edge intersects this scan line.
                  //add it to the ael
                  edgeTableElement ete = new edgeTableElement();
                  ete.mSlope = mMaskLines[i].giveSlope();
                  if (ete.mSlope != 0) ete.mSlope = 1.0f / ete.mSlope;
                  ete.yMin = yMin;
                  ete.mXInt = (int)(( mMaskLines[i].b.X + (ete.mSlope) * (y - mMaskLines[i].b.Y)));

                  //is there a pixel in my AEL that is to my left/right? if so, use that instead..
                  bool ok = true;
                  for (int k = 0; k < AEL.Count;k++ )
                  {
                     if (AEL[k].mXInt == ete.mXInt - 1 || AEL[k].mXInt == ete.mXInt + 1
                        ||
                        AEL[k].mXInt == ete.mXInt - 2 || AEL[k].mXInt == ete.mXInt + 2)
                     {
                        ete.mXInt = AEL[k].mXInt;
                        ok = false;
                        break;
                     }
                  }

                     // if (ete.mXInt < 0) 
                     //  ete.mXInt = 0;
                     //if (ete.mXInt >=TerrainGlobals.getTerrain().getNumXVerts()) 
                     //  ete.mXInt = TerrainGlobals.getTerrain().getNumXVerts() - 1;

                  if (ok) 
                     AEL.Add(ete);
               }
            }

            //CLM HAXOR
            //if (AEL.Count % 2 == 1)
            //{
            //   if(AEL.Count>1)
            //      AEL.Insert(2,AEL[1]);
            //}

            if(AEL.Count!=0)
            {
               ////remove redundant points
               for (int q = 0; q < AEL.Count; q++)
               {
                  int xp = AEL[q].mXInt;
                  for (int qt = q + 1; qt < AEL.Count; qt++)
                  {
                     if (AEL[qt].mXInt == xp)
                     {
                        AEL.RemoveAt(qt);
                        qt--;
                     }
                  }
               }

               AEL.Sort();
               for(int q=0;q<AEL.Count-1;q+=2)
               {
                  //fill!
                  int xl = AEL[q].mXInt;
                  int xr = AEL[q+1].mXInt;
                  for (int x = xl; x <= xr; x++)
                  {
                     fillArray[x, y] = true;
                  }
               }
            }
         }

         //step 3, transfer to masking system
         for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
         {
            for (int y = 0; y < TerrainGlobals.getTerrain().getNumXVerts(); y++)
            {
               if (fillArray[x, y] == true)
               {
                  Masking.addSelectedVert(x, y, addOrRemove ? 1.0f : 0);
               }
            }
         }
         fillArray = null;
         Masking.rebuildVisualsAfterSelection();
         clearMaskLines();
      }
      public void fillMaskFromLines0(bool addOrRemove)
      {
         //CLM we use a seperate grid here to ensure we're not disrupting any previous masks.
         bool[,] fillArray = new bool[TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumXVerts()];

         //step 1, raster lines to a 2d grid
         for (int i = 0; i < mMaskLines.Count;i++ )
         {
            //Bresenham
            int x0 = mMaskLines[i].a.X;
            int y0 = mMaskLines[i].a.Y;
            int x1 = mMaskLines[i].b.X;
            int y1 = mMaskLines[i].b.Y;
             {
                 int dy = y1 - y0;
                 int dx = x1 - x0;
                 int stepx, stepy;

                 if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
                 if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
                 dy <<= 1;                                                  // dy is now 2*dy
                 dx <<= 1;                                                  // dx is now 2*dx

                 fillArray[x0, y0] = true;
                 if (dx > dy) 
                 {
                     int fraction = dy - (dx >> 1);                         // same as 2*dy - dx
                     while (x0 != x1) 
                     {
                         if (fraction >= 0) 
                         {
                             y0 += stepy;
                             fraction -= dx;                                // same as fraction -= 2*dx
                         }
                         x0 += stepx;
                         fraction += dy;                                    // same as fraction -= 2*dy
                         fillArray[x0, y0] = true;
                     }
                 } 
                 else 
                 {
                     int fraction = dx - (dy >> 1);
                     while (y0 != y1) 
                     {
                         if (fraction >= 0) 
                         {
                             x0 += stepx;
                             fraction -= dy;
                         }
                         y0 += stepy;
                         fraction += dx;
                         //raster.setPixel(pix, x0, y0);
                         fillArray[x0, y0] = true; 
                     }
                 }
             }
         }
         
         //step 2, raster fill 2d polygon
         for (int y = 0; y < TerrainGlobals.getTerrain().getNumXVerts(); y++)
         {
            //find all our line edges for this scanline
            List<int> pts = new List<int>();
            for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts()-1; x++)
            {
               if (fillArray[x, y] == true)
               {
                  //is this a run of edges?
                  if (fillArray[x + 1, y] == true)
                  {
                     int kx = x + 1;
                     for (kx = x + 1; kx < TerrainGlobals.getTerrain().getNumXVerts() - 1; kx++)
                     {
                        if (fillArray[x + 1, y] == false)
                           break;
                     }
                     if ((kx - x % 2) == 0)
                     {
                        pts.Add(kx);
                     }
                     else if ((kx - x % 2) == 1)
                     {
                        pts.Add(x);
                        pts.Add(kx);
                     }
                  }
                  else
                  {
                     pts.Add(x);
                  }
               }
            }
            

            for (int i = 0; i < pts.Count - 1; i++)
            {
               if (pts[i] + 1 == pts[i + 1])
                  continue;
               
               for (int x = pts[i]; x < pts[i + 1]; x++)
                  fillArray[x, y] = true;
               i++;
            }
         }
         
         //step 3, transfer to masking system
         for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
         {
            for (int y = 0; y < TerrainGlobals.getTerrain().getNumXVerts(); y++)
            {
               if(fillArray[x,y]==true)
               {
                  Masking.addSelectedVert(x, y, addOrRemove?1.0f:0);
               }
            }
         }
         fillArray = null;
         Masking.rebuildVisualsAfterSelection();
         clearMaskLines();
      }

      #endregion


    

      //positions editing
      private void drawSelectionCamera()
      {


         {
            // Draw Selection Map
            BRenderDevice.getDevice().VertexFormat = VertexFormats.Transformed | VertexFormats.Texture1;
            BRenderDevice.getDevice().SetStreamSource(0, gSelectionCameraSpriteVB, 0);
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;
            BRenderDevice.getDevice().SetTexture(0, mThumbnailTexture);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleStrip, 0, 2);


            int size = 100;
            int margin = 7;
            Vector2 spriteCenterPos = new Vector2(BRenderDevice.getDevice().Viewport.Width - size - margin, size + margin);

            // Draw border
            Line line = new Line(BRenderDevice.getDevice());

            Vector2[] points = new Vector2[5];

            points[0] = new Vector2(spriteCenterPos.X + size, spriteCenterPos.Y + size);
            points[1] = new Vector2(spriteCenterPos.X + size, spriteCenterPos.Y - size);
            points[2] = new Vector2(spriteCenterPos.X - size, spriteCenterPos.Y - size);
            points[3] = new Vector2(spriteCenterPos.X - size, spriteCenterPos.Y + size);
            points[4] = new Vector2(spriteCenterPos.X + size, spriteCenterPos.Y + size);

            line.Begin();
            line.Draw(points, Color.LightGray);
            line.End();
         }
      }

      #region UNDO-REDO
      public class EditData
      {
         public Dictionary<int, VertexBackup> mVertBackup = new Dictionary<int, VertexBackup>();
         public Dictionary<int, BTerrainQuadNode> mNodeBackup = new Dictionary<int, BTerrainQuadNode>();
         

         ~EditData()
         {
            foreach (int key in mNodeBackup.Keys)
            {
               mNodeBackup[key].DetachFromVis();
            }
         }
         public int GetCost()
         {
            int cost = mVertBackup.Keys.Count * 64 * 64;// +mBackupDetailPoints.mGuessCount;
            return cost;
         }
      }

    //  JaggedContainer<Vector3> mBackupDetailPoints = new JaggedContainer<Vector3>(MaskFactory.mMaxCapacity);

 
      public void PushRawDeformationChanges(bool isUndoCopy)
      {
          List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();
          TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(mNodes, mCurrBrushExtends.minX, mCurrBrushExtends.maxX, mCurrBrushExtends.minZ, mCurrBrushExtends.maxZ);
         addVertexUndoNodes(mNodes, !isUndoCopy);
         if(!isUndoCopy)
            PushChanges();

         //mCurrBrushExtends
         //Vector3 Value;
         //long Key;
         //mCurrBrushDeformations.ResetIterator();
         //while (mCurrBrushDeformations.MoveNext(out Key, out Value))
         //{
         //   mBackupDetailPoints.SetValue(Key, mDetailPoints[Key]);
         //}

         //CleanUndoStack();
      }
     
      public void PushWholeFrigginMap(bool isUndoCopy)
      {
         addVertexUndoNodes(TerrainGlobals.getTerrain().getQuadNodeLeafArray(), !isUndoCopy);
         if(!isUndoCopy)
            PushChanges();
      }

      public void PushSelectedDetailPoints(bool isUndoCopy)
      {
         List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafMaskedNodes(mNodes);
         addVertexUndoNodes(mNodes, !isUndoCopy);
         if (!isUndoCopy)
            PushChanges();
      }

      public void PushChanges()
      {
         pushBrushToUndo();

         CleanUndoStack();
      }

      private void CleanUndoStack()
      {
         List<EditData> newStack = new List<EditData>();

         long sizeTotal = 0;
         long sizeMax = 2048 * 2048;
         int undosteps = 0;
         int maxundosteps = 20;
         foreach (EditData d in mEditStack)
         {

            sizeTotal += d.GetCost();
            if (d.GetCost() == 0 || undosteps > maxundosteps)
               continue;
            if (sizeTotal < sizeMax)
            {
               newStack.Add(d);
               undosteps++;
            }
            else
            {

               foreach (int key in d.mNodeBackup.Keys)
               {
                  d.mNodeBackup[key].Dispose();
                  d.mNodeBackup[key].DetachFromVis();
               }
            }
         }

         mEditStack = newStack;
         //clear old changes
         //if(mEditStack.Count > 10)
         //{
         //   EditData toremove =  mEditStack[mEditStack.Count - 1];
         //   foreach (int key in toremove.mNodeBackup.Keys)
         //   {
         //      toremove.mNodeBackup[key].Dispose();
         //      toremove.mNodeBackup[key].DetachFromVis();
         //   }
         //   mEditStack.RemoveAt(mEditStack.Count - 1);
         //}
      }

     
      public void UndoChanges()
      {
      //   pushBrushToUndo();
         UndoManager.undo();
         return;

         //lock (this)
         //{
         //   if (mNodeBackup.Count + mVertBackup.Count == 0 && !mBackupDetailPoints.HasData())
         //      return;


         //   Cursor c = Cursor.Current;
         //   Cursor.Current = Cursors.WaitCursor;

         //   //redo stack push
         //   undo();

         //   if (mEditStack.Count > 0)
         //   {

         //      EditData d = mEditStack[0];
         //      mEditStack.RemoveAt(0);
         //      mNodeBackup = d.mNodeBackup;
         //      mVertBackup = d.mVertBackup;
         //      mBackupDetailPoints = d.mBackupDetailPoints;
         //   }


         //   Cursor.Current = c;
         //}
      }

      public void RedoChanges()
      {
         UndoManager.redo();
         return;

         //lock (this)
         //{
         //   Cursor c = Cursor.Current;
         //   Cursor.Current = Cursors.WaitCursor;

         //   //if(mEditStack)

         //   if (mRedoStack.Count > 0)
         //   {
         //      PushChangesNoClearRedo();
         //      EditData d = mRedoStack[0];
         //      mRedoStack.RemoveAt(0);

         //      mNodeBackup = d.mNodeBackup;
         //      mVertBackup = d.mVertBackup;
         //      mBackupDetailPoints = d.mBackupDetailPoints;

         //      redo();
         //   }


         //   Cursor.Current = c;
         //}
        


      }


      List<EditData> mEditStack = new List<EditData>();


      Dictionary<int, BTerrainQuadNode> mNodeBackup = new Dictionary<int, BTerrainQuadNode>();
      Dictionary<int, BTerrainQuadNode> mTempNodeBackup = new Dictionary<int, BTerrainQuadNode>();

      Dictionary<int, VertexBackup> mVertBackup = new Dictionary<int, VertexBackup>();


      public class VertexBackup
      {
         BTerrainQuadNodeDesc mDesc;
         Vector3[] mVertData;



         public VertexBackup(BTerrainQuadNode node, Vector3[] detailData)
         {
            copy(node, detailData);
         }

         public VertexBackup(VertexBackup other)
         {
            this.mDesc = other.mDesc;


         }


         public void copy(Vector3[] detailData)
         {
            long xDest = 0;
            long zDest = 0;
            long numZVerts = TerrainGlobals.getTerrain().getNumZVerts();

            mVertData = new Vector3[65 * 65];
            for (long x = mDesc.mMinXVert; x < mDesc.mMaxXVert; x++)
            {
               zDest = 0;
               for (long z = mDesc.mMinZVert; z < mDesc.mMaxZVert; z++)
               {
                  long mainIndex = x * numZVerts + z;
                  long tempIndex = xDest * 64 + zDest;
                  mVertData.SetValue(detailData.GetValue(mainIndex), tempIndex);

                  //mVertData[tempIndex] = detailData[mainIndex];

                  zDest++;
               }
               xDest++;
            }


         }

         public void copy(BTerrainQuadNode node, Vector3[] detailData)
         {
            mDesc = node.getDesc();
            copy(detailData);

         }
         public void restore(Vector3[] detailData)
         {
            long xDest = 0;
            long zDest = 0;
            long numZVerts = TerrainGlobals.getTerrain().getNumZVerts();

            for (long x = mDesc.mMinXVert; x < mDesc.mMaxXVert; x++)
            {
               zDest = 0;
               for (long z = mDesc.mMinZVert; z < mDesc.mMaxZVert; z++)
               {
                  long mainIndex = x * numZVerts + z;
                  long tempIndex = xDest * 64 + zDest;
                  detailData.SetValue(mVertData[tempIndex], mainIndex);  //[mainIndex] = mVertData[tempIndex];

                  //detailData[mainIndex] = mVertData[tempIndex];
                  zDest++;
               }
               xDest++;
            }

            BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                  TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                  (int)mDesc.mMinXVert, (int)mDesc.mMaxXVert, (int)mDesc.mMinZVert, (int)mDesc.mMaxZVert);

         }

      }


      #endregion
      //protected void updateTerrainVisuals(bool applyToMaskOnly)
      //{

      //}
      protected void updateTerrainVisuals(bool applyToMaskOnly)
      {

         if (!applyToMaskOnly)
         {
            BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                      TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                      0, TerrainGlobals.getTerrain().getNumXVerts(), 0, TerrainGlobals.getTerrain().getNumXVerts());
         }
         else
         {
            BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                      TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                      Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX, Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);


         }
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         if (applyToMaskOnly)
         {
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
                                                                                     Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);


         }
         else
         {
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, 0, TerrainGlobals.getTerrain().getNumXVerts(),
                                                                                     0, TerrainGlobals.getTerrain().getNumXVerts());
         }

         for (int i = 0; i < nodes.Count; i++)
         {
            nodes[i].mDirty = true;
         }
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());

      }


      public void ClearVis()
      {
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();

         TerrainGlobals.getTerrain().rebuildDirty(BRenderDevice.getDevice());

         TerrainGlobals.getTerrain().recreate();
      }

      public BrushInfo getCurrentBrushInfo()
      {
         return mBrushInfo;
      }

      public BTerrainBrush getCurrentBrush()
      {
         return mCurrBrush;
      }


      JaggedContainer<Vector3> mCurrBrushDeformations;
      JaggedContainer<Vector3> mCurrBrushDeformationNormals;
      JaggedContainer<BTerrainTextureVector> mCurrBrushDeformationTextures = null;
      public JaggedContainer<Vector3> getCurrBrushDeformations() { return mCurrBrushDeformations; }
      public JaggedContainer<Vector3> getCurrBrushDeformationNormals() { return mCurrBrushDeformationNormals; }
      public JaggedContainer<BTerrainTextureVector> getCurrBrushDeformationTextures() { return mCurrBrushDeformationTextures; }


      UndoPacket mCurrentUndoPacket = null;
      UndoPacket mCurrentUndoBrushPacket = null;

      private void clearUndoPacket()
      {
         mCurrentUndoPacket = null;
      }
      private void pushBrushToUndo()
      {
         if (mCurrentUndoPacket == null)
            return;

         UndoManager.pushUndo(mCurrentUndoPacket);
         clearUndoPacket();
      }
      private void flushBrushDeformations()
      {
         flushBrushDeformations(true);
      }
      void applyBrushDeformations()
      {
         long Key;
         
         if(mUseClipartVertexData)
         {
            Vector3 Value;
            mCurrBrushDeformations.ResetIterator();
            while (mCurrBrushDeformations.MoveNext(out Key, out Value))
            {
               mDetailPoints[Key] += Value;
            }
         
            Vector3 normalValue;       
            mCurrBrushDeformationNormals.ResetIterator();
            while (mCurrBrushDeformationNormals.MoveNext(out Key, out normalValue))
            {
               mNormals[Key] += normalValue;
            }
         }

         //copy our texture data..
         if(mUseClipartTextureData)
            flushTerrainTexDef();


         // copy all quadnodes' postDeform BB's to regular BB's.
         TerrainGlobals.getTerrain().getQuadNodeRoot().flushBrushDeformations();

         //object data
         if (mUseClipartSimData)
            SimGlobals.getSimMain().clipartPlaceObjects(mCopiedUnitData.mIntersectCenter);

      }
      private void flushBrushDeformations(bool pushToUndo)
      {
         if (pushToUndo)
            pushBrushToUndo();

         applyBrushDeformations();

         clearBrushDeformations();
 
      }
      public void clearBrushDeformations()
      {
         clearUndoPacket();
         mCurrBrushDeformations.Clear();
         mCurrBrushDeformationNormals.Clear();
         if (mCurrBrushDeformationTextures.HasData())
            TerrainGlobals.getTexturing().reloadCachedVisuals();
         mCurrBrushDeformationTextures.Clear();

         SimGlobals.getSimMain().clipartClearObjects();
         // Reset extends
         mCurrBrushExtends.empty();
      }
      public void extendCurrBrushDeformation(int x, int z)
      {
         mCurrBrushExtends.addPoint(x, z);
      }
      public bool hasBrushData(int x, int z)
      {
         return (mCurrBrushExtends.isPointInside(x, z));
      }

      public void flushTerrainTexDef()
     {
        if (!mCurrBrushDeformationTextures.HasData())
           return;
        if (mPastedDataPreviewBounds == null)
           return;

        List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
        TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mPastedDataPreviewBounds.minX, mPastedDataPreviewBounds.maxX,
                                                                                       mPastedDataPreviewBounds.minZ, mPastedDataPreviewBounds.maxZ);
        
        for(int i=0;i<nodes.Count;i++)
        {
           BTerrainQuadNodeDesc desc = nodes[i].getDesc();
           BTerrainLayerContainer tContainer = TerrainGlobals.getEditor().generateContainerFromTexDeformations(desc.mMinXVert, desc.mMinZVert);
           if (tContainer != null)
           {
              tContainer.copyTo(ref nodes[i].mLayerContainer);
              //             .merge(tContainer);
              tContainer.destroy();
              tContainer = null;
           }
        }
     }

     public void enableShowVerts(bool state)
      {
         mBrushShowVerts = state;
      }
      public void toggleShowVerts()
      {
         enableShowVerts(!mBrushShowVerts);
      }

      public bool isShowSelectionCameraEnabled()
      {
         return (mShowSelectionCamera);
      }
      public void enableShowSelectionCamera(bool state)
      {
         mShowSelectionCamera = state;
      }
      public void toggleShowSelectionCamera()
      {
         enableShowSelectionCamera(!mShowSelectionCamera);
      }
      private void createSelectionCameraSprite()
      {




         // Release first
         if (gSelectionCameraSpriteVB != null)
         {
            gSelectionCameraSpriteVB.Dispose();
            gSelectionCameraSpriteVB = null;
         }

         gSelectionCameraSpriteVB = new VertexBuffer(typeof(CursorSpriteVert), 4, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.Transformed | VertexFormats.Texture1, Pool.Default);

         CursorSpriteVert[] spriteVerts = new CursorSpriteVert[4];

         float size = 300.0f;
         float margin = 7.0f;

         Vector3 spriteCenterPos = new Vector3(BRenderDevice.getDevice().Viewport.Width - size - margin, size + margin, 0);

         spriteVerts[0].xyzw = new Vector4(spriteCenterPos.X + size, spriteCenterPos.Y + size, 0.0f, 1.0f);
         spriteVerts[0].uv = new Vector2(1.0f, 1.0f);
         spriteVerts[1].xyzw = new Vector4(spriteCenterPos.X + size, spriteCenterPos.Y - size, 0.0f, 1.0f);
         spriteVerts[1].uv = new Vector2(1.0f, 0.0f);
         spriteVerts[2].xyzw = new Vector4(spriteCenterPos.X - size, spriteCenterPos.Y + size, 0.0f, 1.0f);
         spriteVerts[2].uv = new Vector2(0.0f, 1.0f);
         spriteVerts[3].xyzw = new Vector4(spriteCenterPos.X - size, spriteCenterPos.Y - size, 0.0f, 1.0f);
         spriteVerts[3].uv = new Vector2(0.0f, 0.0f);


         //copy verts over
         unsafe
         {
            using (GraphicsStream stream = gSelectionCameraSpriteVB.Lock(0, 4 * sizeof(CursorSpriteVert), LockFlags.None))
            {
               stream.Write(spriteVerts);
               gSelectionCameraSpriteVB.Unlock();
            }
         }


         if (mThumbnailTexture != null)
         {
            mThumbnailTexture.Dispose();
            mThumbnailTexture = null;
         }

         unsafe
         {
            mThumbnailTexture = new Texture(BRenderDevice.getDevice(), 512, 512, 1, Usage.RenderTarget, Format.X8R8G8B8, Pool.Default);
            //mThumbnailTexture = new Texture(BRenderDevice.getDevice(), 128, 128, 1, Usage.RenderTarget, Format.X8R8G8B8, Pool.Default);
         }

      }

      public void deviceReset()
      {
         BTerrainVertexBrush.createCursor();
         createSelectionCameraSprite();

         TerrainGlobals.getRender().deviceReset();
      }

      public BrushInfo mBrushInfo = new BrushInfo();
      private eBrushShape mBrushShape;
      private float mBrushRadiusFactor = 1.0f;
      public bool mBrushShowVerts = false;

      private bool mShowSelectionCamera = false;


      bool mShowLighting = false;
      public void toggleShowLighting()
      {
         mShowLighting = !mShowLighting;
         if(mShowLighting)
         {
            LightManager.rasterTerrainLightsToExportGrid(TerrainGlobals.getEditor().getLightValues());
            TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         }
      }
      public bool doShowLighting()
      {
         return mShowLighting;
      }
      //current brush
      public void cleanBrush()
      {
         mCurrBrush = null;
      }
      private BTerrainBrush mCurrBrush;
      private BTileBoundingBox mCurrBrushExtends = new BTileBoundingBox();

      private Vector3[] mDetailPoints;
      private Vector3[] mNormals;
      private float[] mSkirtHeights;
      private float[] mAmbientOcclusionValues;
      private byte[] mAlphaValues;
      private Vector3[] mLightValues;

      //sim rep
      private BTerrainSimRep mSimRep;

      //camera height rep
      private CameraHeightRep mCameraHeightRep;
      public CameraHeightRep getCameraRep() { return mCameraHeightRep; }

      //RealLOSRep
      private RealLOSRep mRealLOSRep;
      public RealLOSRep getRealLOSRep() { return mRealLOSRep; }

      //widgets
      TranslationWidget mTransWidget = null;


      public VertexBuffer gSelectionCameraSpriteVB = null;

      //intersection information
      public Vector3 mBrushIntersectionPoint;
      public BTerrainQuadNode mBrushIntersectionNode;
      public Vector3 mBrushIntersectionNormal;
      public int mVisTileIntetersectionX;
      public int mVisTileIntetersectionZ;

      //for drawing debug bounds of quadnodes
      static public VertexBuffer mdebugPrimBuffer = null;

      public Texture mThumbnailTexture = null;


      private bool m_bAlwaysRenderSkirt = false;
      private bool m_bRenderFog = true;

      public void toggleRenderSkirt()
      {
         m_bAlwaysRenderSkirt = !m_bAlwaysRenderSkirt;
      }

      public bool isRenderSkirtEnabled()
      {
         return (m_bAlwaysRenderSkirt);
      }

      public void setRenderFog(bool bFog)
      {
         m_bRenderFog = bFog;// !m_bRenderFog;
         

      }

      public bool isRenderFogEnabled()
      {
         return (m_bRenderFog);
      }

      //------------------------------------
      static public void DrawQuadNodeBounds(BTerrainQuadNode node)
      {
         int numVerts = 16;
         if (mdebugPrimBuffer == null)
         {
            //BRenderDevice.getDevice().CreateVertexBuffer( numVerts * sizeof(CursorVert),0, D3DFVF_XYZ | D3DFVF_DIFFUSE, Pool.Default, &mdebugPrimBuffer, null);
            mdebugPrimBuffer = new VertexBuffer(typeof(CursorColorVert), numVerts, BRenderDevice.getDevice(), Usage.None, VertexFormats.Position | VertexFormats.Diffuse, Pool.Default);
         }

         uint blue = 0xFF0000FF;
         uint red = 0xFFFF0000;
         uint green = 0xFF00FF00;

         BTerrainQuadNodeDesc mDesc;
         mDesc = node.getDesc();
         Vector3 min = mDesc.m_minPostDeform;
         Vector3 max = mDesc.m_maxPostDeform;
         CursorColorVert[] verts = new CursorColorVert[16];
         int index = 0;

         verts[index++].SetValues(min.X, max.Y, min.Z, blue);
         verts[index++].SetValues(max.X, max.Y, min.Z, blue);
         verts[index++].SetValues(min.X, max.Y, min.Z, red);
         verts[index++].SetValues(min.X, max.Y, max.Z, red);

         verts[index++].SetValues(min.X, min.Y, min.Z, blue);
         verts[index++].SetValues(max.X, min.Y, min.Z, blue);
         verts[index++].SetValues(min.X, min.Y, min.Z, red);
         verts[index++].SetValues(min.X, min.Y, max.Z, red);

         verts[index++].SetValues(min.X, max.Y, min.Z, green);
         verts[index++].SetValues(min.X, min.Y, min.Z, green);

         verts[index++].SetValues(max.X, max.Y, min.Z, green);
         verts[index++].SetValues(max.X, min.Y, min.Z, green);

         verts[index++].SetValues(min.X, max.Y, min.Z, green);
         verts[index++].SetValues(min.X, min.Y, min.Z, green);

         verts[index++].SetValues(min.X, max.Y, max.Z, green);
         verts[index++].SetValues(min.X, min.Y, max.Z, green);

         //void *pVertices = null;
         //if(!FAILED(mdebugPrimBuffer.Lock( 0, numVerts * sizeof(CursorVert), (void**)&pVertices, 0 )))
         //{
         //   memcpy( pVertices, verts, numVerts * sizeof(CursorVert));
         //   mdebugPrimBuffer.Unlock();

         //   BRenderDevice.getDevice().SetStreamSource( 0, mdebugPrimBuffer, 0, sizeof(CursorVert) );
         //   BRenderDevice.getDevice().SetFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE );

         //   BRenderDevice.getDevice().SetVertexShader(0);
         //   BRenderDevice.getDevice().SetPixelShader(0);
         //   BRenderDevice.getDevice().SetTexture(0,0);

         //   BRenderDevice.getDevice().DrawPrimitive( PrimitiveType.LineList, 0, numVerts / 2);
         //}

         unsafe
         {
            using (GraphicsStream stream = mdebugPrimBuffer.Lock(0, numVerts * sizeof(CursorColorVert), LockFlags.None))
            {
               stream.Write(verts);
               mdebugPrimBuffer.Unlock();

               BRenderDevice.getDevice().SetStreamSource(0, mdebugPrimBuffer, 0, sizeof(CursorColorVert));
               BRenderDevice.getDevice().VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;// (D3DFVF_XYZ | D3DFVF_DIFFUSE);
               BRenderDevice.getDevice().VertexShader = null;
               BRenderDevice.getDevice().PixelShader = null;
               BRenderDevice.getDevice().SetTexture(0, null);
               BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, (int)(numVerts / 2));
            }
         }


      }

      //------------------------------------
      static public float findClosestVertex(ref int x, ref int z, ref  Vector3 intpt, BTerrainQuadNode node)
      {
         //assert(node);

         //cycle through all the verts of this node, check distance to intpt
         BTerrainQuadNodeDesc desc;
         desc = ((BTerrainQuadNode)node).getDesc();

         float smallestDistSq = float.MaxValue;

         for (int i = desc.mMinXVert; i < desc.mMaxXVert; i++)
         {
            for (int j = desc.mMinZVert; j < desc.mMaxZVert; j++)
            {
               Vector3 pos = TerrainGlobals.getTerrain().getPos(i, j) - intpt;
               float cDistSq = pos.LengthSq();
               if (cDistSq < smallestDistSq)
               {
                  smallestDistSq = cDistSq;
                  x = i;
                  z = j;
               }
            }
         }
         return ((float)Math.Sqrt(smallestDistSq));
      }
      //------------------------------------
      static public float findClosestVertexInNodes(ref int x, ref int z, ref Vector3 intpt, List<BTerrainQuadNode> nodes)
      {
         int tx = 0; int tz = 0;
         float smallestDist = float.MaxValue;
         for (int i = 0; i < nodes.Count; i++)
         {
            float dist = findClosestVertex(ref tx, ref tz, ref intpt, nodes[i]);
            if (dist < smallestDist)
            {
               x = tx;
               z = tz;
               smallestDist = dist;
            }

         }
         return smallestDist;
      }


      #region Tesselation REGION
      JaggedContainer<byte> mTesselationOverride = null;
      public static byte cTesselationEmptyVal = 255;
      eTessOverrideVal mCurrTessMode = eTessOverrideVal.cTess_None;
      public enum eTessOverrideVal
      {
         cTess_None =0,
         cTess_Min =1,
         cTess_Max =2,
      }
      public void initTesselationOverride()
      {
         mTesselationOverride = new JaggedContainer<byte>(TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts());
         mTesselationOverride.SetEmptyValue(cTesselationEmptyVal);
      }
      public void destroyTesselationOverride()
      {
         if (mTesselationOverride != null)
         {
            clearTesselationOverride();
            mTesselationOverride = null;
         }
      }
      public void clearTesselationOverride()
      {
         mTesselationOverride.Clear();
         if (TerrainGlobals.getTerrain().getQuadNodeRoot()!=null)
            TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
      }
      public void createTessOverrideFrom(JaggedContainer<byte> v)
      {
         destroyTesselationOverride();
         initTesselationOverride();

         long id;
         byte maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == cTesselationEmptyVal)
               continue;

            mTesselationOverride.SetValue(id, maskValue);
         }
      }
      public JaggedContainer<byte> getJaggedTesselation()
      {
         return mTesselationOverride;
      }
      public void setTessellationBrushValue(eTessOverrideVal val)
      {
         mCurrTessMode = val;
         if (mCurrBrush is BTerrainTesselationBrush)
            ((BTerrainTesselationBrush)mCurrBrush).mTessOverideSetting = mCurrTessMode;
      }
      
      public bool newTesselationBrush()
      {
         cleanBrush();
         mCurrBrush = BrushManager.getTesselationBrush();

         ((BTerrainTesselationBrush)mCurrBrush).mTessOverideSetting = mCurrTessMode;
         mBrushShowVerts = true;
         TerrainGlobals.getTerrain().refreshTerrainVisuals();

         return true;
      }

   
      #endregion
      #region AO REGION
      public float computeAmbientOcclusion()
      {
         return computeAmbientOcclusion(Terrain.AmbientOcclusion.eAOQuality.cAO_Worst, false);//true);//
      }
      public float computeAmbientOcclusion(Terrain.AmbientOcclusion.eAOQuality quality, bool renderWorldObjects)
      {
         Terrain.AmbientOcclusion ao = new Terrain.AmbientOcclusion();
         float totalTime = 0;
         float peelTime = 0;
         float gatherTime = 0;
         ao.calcualteAO(quality, renderWorldObjects, ref totalTime, ref peelTime, ref gatherTime);
         ao.destroy();
         ao = null;
         return totalTime;

        
      }

      public void clearAmbientOcclusion()
      {
         int count = TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts();
         for (int i = 0; i < count; i++)
         {
            mAmbientOcclusionValues[i] = 1.0f;
         }
         if(TerrainGlobals.getTerrain().getQuadNodeRoot()!=null)
            TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
      }

      #endregion

      #region SKIRT REGION
      public void clearSkirtHeights()
      {
         int count = TerrainGlobals.getTerrain().getTotalSkirtXVerts() * TerrainGlobals.getTerrain().getTotalSkirtZVerts();
         for (int i = 0; i < count; i++)
         {
            mSkirtHeights[i] = 0.0f;
         }
      }


       public void getCylinderIntersectionSkirt(List<int> indexes, ref Vector3 center, float radius)
       {
          int numSkirtXVerts = TerrainGlobals.getTerrain().getTotalSkirtXVerts();
          int numSkirtZVerts = TerrainGlobals.getTerrain().getTotalSkirtZVerts();
          
          int numSkirtXVertsPerQuadrant = TerrainGlobals.getTerrain().getNumSkirtXVertsPerQuadrant();
          int numSkirtZVertsPerQuadrant = TerrainGlobals.getTerrain().getNumSkirtZVertsPerQuadrant();


          Vector3 min = center;
          Vector3 max = center;
          min.X -= radius; min.Y = float.MinValue; min.Z -= radius;
          max.X += radius; max.Y = float.MaxValue; max.Z += radius;


          int minXSkirtTile = (int) ((((min.X / TerrainGlobals.getTerrain().getWorldSizeX()) + 1.0f) / 3.0f) * numSkirtXVerts - 1);
          int maxXSkirtTile = (int) ((((max.X / TerrainGlobals.getTerrain().getWorldSizeX()) + 1.0f) / 3.0f) * numSkirtXVerts + 1);
          int minZSkirtTile = (int) ((((min.Z / TerrainGlobals.getTerrain().getWorldSizeZ()) + 1.0f) / 3.0f) * numSkirtZVerts - 1);
          int maxZSkirtTile = (int) ((((max.Z / TerrainGlobals.getTerrain().getWorldSizeZ()) + 1.0f) / 3.0f) * numSkirtZVerts + 1);


          minXSkirtTile = (int)BMathLib.Clamp(minXSkirtTile, 0, numSkirtXVerts - 1);
          maxXSkirtTile = (int)BMathLib.Clamp(maxXSkirtTile, 0, numSkirtXVerts - 1);
          minZSkirtTile = (int)BMathLib.Clamp(minZSkirtTile, 0, numSkirtZVerts - 1);
          maxZSkirtTile = (int)BMathLib.Clamp(maxZSkirtTile, 0, numSkirtZVerts - 1);

          for (int x = minXSkirtTile; x <= maxXSkirtTile; x++)
          {
             for (int z = minZSkirtTile; z <= maxZSkirtTile; z++)
             {
                // Do not allow editing of skirt rim around the playable area
                if ((x >= (numSkirtXVertsPerQuadrant - 1)) && (x <= (numSkirtXVertsPerQuadrant - 1) * 2) &&
                   (z >= (numSkirtZVertsPerQuadrant - 1)) && (z <= (numSkirtZVertsPerQuadrant - 1) * 2))
                   continue;

                Vector3 vert = TerrainGlobals.getTerrain().getSkirtPos(x, z);

                if (BMathLib.pointCylinderIntersect(ref center, radius, ref vert))
                {
                   int indx = x * numSkirtZVerts + z;
                   indexes.Add(indx);
                }
             }
          }
       }
      #endregion
    };

   #endregion

}
