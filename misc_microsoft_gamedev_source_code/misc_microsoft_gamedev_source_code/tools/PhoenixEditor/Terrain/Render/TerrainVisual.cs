using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using Rendering;
using EditorCore;
using SimEditor;
//
//--------------------------------------------
namespace Terrain
{

    #region TerrainVisual
    //--------------------------------------------
   //----------------------------
   //----------------------------
    public class BTerrainVisualData
    {
       public BTerrainVisualData()
       {
       }
       ~BTerrainVisualData()
        {
           destroy();
         }
       public void destroy()
       {
          if (mPositionsTexture != null)
          {
             mPositionsTexture.Dispose();
             mPositionsTexture = null;
          }
          if (mNormalsTexture != null)
          {
             mNormalsTexture.Dispose();
             mNormalsTexture = null;
          }
          if (mAlphasTexture != null)
          {
             mAlphasTexture.Dispose();
             mAlphasTexture = null;
          }
          if (mLightTexture != null)
          {
             mLightTexture.Dispose();
             mLightTexture = null;
          }
          if (mTessellationTexture != null)
          {
             mTessellationTexture.Dispose();
             mTessellationTexture = null;
          }
       }

       public Texture mPositionsTexture = null;
       public Texture mNormalsTexture = null;
       public Texture mAlphasTexture = null;
       public Texture mLightTexture = null;
       public Texture mTessellationTexture = null;
      
    }
   //----------------------------
   public class BTerrainVisualDataHandle
   {
      public BTerrainVisualData mHandle   = null;
      public bool inUse                   = false;
      public int LOD                      = 0;
      public float minX                   = 0;
      public float minZ                   = 0;          
   }
   //----------------------------
   public class BTerrainVisual
   {
      public enum eLODLevel
      {
         cLOD0 = 0,
         cLOD1,
         cLOD2,
         cLOD3,
         cLODCount
      };
      //public enum eLODLevelDists
      //{
      //   cLOD0Dist = 0,
      //   cLOD1Dist = 70,
      //   cLOD2Dist = 80,
      //   cLOD3Dist = 100,
      //   cLODDistCount
      //};
      public enum eLODLevelDists
      {
         cLOD0Dist = 0,
         cLOD1Dist = 70 * 2,
         cLOD2Dist = 80 * 2,
         cLOD3Dist = 100 * 2,
         cLODDistCount
      };
      public BTerrainVisual()
      {


      }
      //----------------------------
      ~BTerrainVisual()
      {
         destroy();
         mLODData = null;
         mHandles = null;
      }

      public bool init(int size)
      {
         destroy();
         if (mHandles == null)
            mHandles = new List<BTerrainVisualDataHandle>(size);

        
         if (mLODData == null)
            mLODData = new BTerrainVisualLODData();

         mLODData.init();

         return true;
      }
      public void destroy()
      {
         //visualhandles
         destroyAllHandles();

     
         if (mLODData != null)
         {
            mLODData.destroy();
            mLODData = null;
         }

      }
      public void update()
      {

      }

      public void d3dDeviceLost()
      {
         destroyAllHandles();
      }

      public void d3dDeviceRestored()
      {
      }

      public void destroyAllHandles()
      {
         if (mHandles != null)
         {
            for (int i = 0; i < mHandles.Count; i++)
            {
               if (mHandles[i].mHandle != null)
               {
                  mHandles[i].mHandle.destroy();
                  mHandles[i].mHandle = null;
               }
               mHandles[i].inUse = false;
            }
            mHandles.Clear();
         }
      }
      //----------------------------
      public void getLODVB(ref VertexBuffer vb, ref uint numVerts, eLODLevel lod)
      {
         mLODData.getLODVB(ref vb, ref numVerts, lod);
      }
      public void getCursorLODVB(ref VertexBuffer vb, ref uint numVerts, eLODLevel lod)
      {
         mLODData.getCursorLODVB(ref vb, ref numVerts, lod);
      }

      bool mHandleGenExporting = false;
      public void setHandleGenMode(bool exporting)
      {
         mHandleGenExporting = exporting;
      }

      unsafe public int newVisualHandle(float minX, float minZ, float maxX, float maxZ, Vector3 center, Vector3 BBmin, Vector3 BBmax)
      {
         int lod = (int)mLODData.getLODLevel(center, TerrainGlobals.getTerrainFrontEnd().mCameraManager.mEye);

         return newVisualHandle(minX, minZ, maxX, maxZ, center, BBmin, BBmax, lod);
      }
      unsafe public int newVisualHandle(float minX, float minZ, float maxX, float maxZ, Vector3 center, Vector3 BBmin, Vector3 BBmax, int lod)
      {
         int indx = findFirstFreeHandle(lod);


         mHandles[indx].inUse = true;
         mHandles[indx].LOD = lod;

         float mxX = maxX;
         float mxZ = maxZ;
         float mnX = minX;
         float mnZ = minZ;
         int width = (int)(mxX - minX);


         mHandles[indx].minX = minX;
         mHandles[indx].minZ = minZ;


         int stride = (int)Math.Pow(2.0, (double)mHandles[indx].LOD);

         int hwidth = (width >> mHandles[indx].LOD) +1;
         int hheight = (width >> mHandles[indx].LOD) +1;

         bool doPos = true;
         bool doNormal = !mHandleGenExporting;
         bool doAlpha = !mHandleGenExporting;
         bool doLight = !mHandleGenExporting;
         bool doTesselation = mHandleGenExporting ? !mHandleGenExporting : TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeVertTessellation;

         try
         {
         
         //CREATE OUR POSITIONS TEXTURE
         if (mHandles[indx].mHandle.mPositionsTexture == null)
            mHandles[indx].mHandle.mPositionsTexture = new Texture(BRenderDevice.getDevice(), (hwidth - 1) * 2, (hwidth - 1) * 2, 1, Usage.Dynamic, Format.A32B32G32R32F, Pool.Default);

         //CREATE OUR NORMALS TEXTURE      
         if (mHandles[indx].mHandle.mNormalsTexture == null && doNormal)
            mHandles[indx].mHandle.mNormalsTexture = new Texture(BRenderDevice.getDevice(), (hwidth - 1) * 2, (hwidth - 1) * 2, 1, Usage.Dynamic, Format.A32B32G32R32F, Pool.Default);

         //CREATE OUR ALPHAS TEXTURE      
             //CLM 8800 cards don't like non-POW2 l8 textures..
         if (mHandles[indx].mHandle.mAlphasTexture == null && doAlpha)
             mHandles[indx].mHandle.mAlphasTexture = new Texture(BRenderDevice.getDevice(), (hwidth - 1) * 2, (hwidth - 1) * 2, 1, Usage.Dynamic, Format.A8R8G8B8, Pool.Default);

         //CREATE OUR LIGHT TEXTURE      
         if (mHandles[indx].mHandle.mLightTexture == null && doLight)
            mHandles[indx].mHandle.mLightTexture = new Texture(BRenderDevice.getDevice(), (hwidth - 1) * 2, (hwidth - 1) * 2, 1, Usage.Dynamic, Format.A8R8G8B8, Pool.Default);

         //CREATE OUR TESSELLATION TEXTURE      
         if (doTesselation)
         {
            if (mHandles[indx].mHandle.mTessellationTexture == null)
               mHandles[indx].mHandle.mTessellationTexture = new Texture(BRenderDevice.getDevice(), (hwidth - 1) * 2, (hwidth - 1) * 2, 1, Usage.Dynamic, Format.A8R8G8B8, Pool.Default);
         }
         else if (mHandles[indx].mHandle.mTessellationTexture != null)
         {
            mHandles[indx].mHandle.mTessellationTexture.Dispose();
            mHandles[indx].mHandle.mTessellationTexture = null;
         }

         
         }
         catch (OutOfVideoMemoryException e)
         {
            if (!CoreGlobals.IsBatchExport)
               MessageBox.Show("newVisualHandle : You've run out of goddamn video memory... Talk to the editor guys..");
            return -1;
         }

         unsafe
         {
            GraphicsStream streamPos = mHandles[indx].mHandle.mPositionsTexture.LockRectangle(0, LockFlags.Discard);
            GraphicsStream streamNrm = doNormal? mHandles[indx].mHandle.mNormalsTexture.LockRectangle(0, LockFlags.Discard) : null;
            GraphicsStream streamAlp = doNormal?mHandles[indx].mHandle.mAlphasTexture.LockRectangle(0, LockFlags.Discard) : null;
            GraphicsStream streamLht = doLight?mHandles[indx].mHandle.mLightTexture.LockRectangle(0, LockFlags.Discard) : null;
            GraphicsStream streamTss = doTesselation ? mHandles[indx].mHandle.mTessellationTexture.LockRectangle(0, LockFlags.Discard) : null;
            byte* tss = doTesselation ? (byte*)streamTss.InternalDataPointer : null;
            byte* lht = doLight ? (byte*)streamLht.InternalDataPointer : null;
            Byte* alpha = doAlpha?(byte*)streamAlp.InternalDataPointer:null;
            float* nrm = doNormal?(float*)streamNrm.InternalDataPointer:null;
            float* pos = (float*)streamPos.InternalDataPointer;
            {

               int twidth = (hwidth - 1) * 2;
               int tx = 0;
               int tz = 0;
               for (int z = 0; z < width + 1; z += stride,tz++)
               {
                  tx = 0;
                  for (int x = 0; x < width + 1; x += stride,tx++)
                  {
                     int index = (tz * twidth + tx) * 4;
                     int xVal = (int)(mnX + x);
                     int zVal = (int)(mnZ + z);

                     Vector3 rel = TerrainGlobals.getTerrain().getPostDeformRelPos(xVal, zVal);
                     //Vector3 wpos = TerrainGlobals.getTerrain().getPostDeformPos(xVal, zVal);
                     float ao = TerrainGlobals.getTerrain().getAmbientOcclusion(xVal, zVal);

                     pos[index + 0] = rel.X;
                     pos[index + 1] = rel.Y;
                     pos[index + 2] = rel.Z;
                     pos[index + 3] = ao;

                     if(doNormal)
                     {
                        Vector3 normal = TerrainGlobals.getTerrain().getPostDeformNormal(xVal, zVal);
                        float weight = TerrainGlobals.getTerrain().getSoftSelectionWeightRaw(xVal, zVal);

                        nrm[index + 0] = normal.X;
                        nrm[index + 1] = normal.Y;
                        nrm[index + 2] = normal.Z;
                        nrm[index + 3] = weight;
                     }

                     if (doAlpha)
                     {
                         byte alph = TerrainGlobals.getTerrain().getAlphaValue(xVal, zVal);
                         alpha[index + 0] = alph;
                         alpha[index + 1] = alph;
                         alpha[index + 2] = alph;
                         alpha[index + 3] = alph;
                         //alpha[(tz * twidth + tx)] =;
                     }

                     if (doLight)
                     {
                        Vector3 localLight = TerrainGlobals.getTerrain().getLightValue(xVal, zVal);

                        lht[index + 0] = (byte)BMathLib.Clamp((float)(Math.Sqrt(localLight.X)) * 255, 0, 255);
                        lht[index + 1] = (byte)BMathLib.Clamp((float)(Math.Sqrt(localLight.Y)) * 255, 0, 255);
                        lht[index + 2] = (byte)BMathLib.Clamp((float)(Math.Sqrt(localLight.Z)) * 255, 0, 255);
                        lht[index + 3] = 0;  
                     }

                     if(doTesselation)
                     {
                        byte v = TerrainGlobals.getTerrain().getTessOverride(xVal, zVal);

                     

                           tss[index + 0] = (byte)(v == (byte)BTerrainEditor.eTessOverrideVal.cTess_Max ? 255 : 0);
                           tss[index + 1] = (byte)(v == (byte)BTerrainEditor.eTessOverrideVal.cTess_Min ? 255 : 0);
                           tss[index + 2] = 0;
                           tss[index + 3] = 0;  
                      
                       
                     }
                     
                  }
               }
               mHandles[indx].mHandle.mPositionsTexture.UnlockRectangle(0);
               if (doNormal) mHandles[indx].mHandle.mNormalsTexture.UnlockRectangle(0);
               if (doAlpha) mHandles[indx].mHandle.mAlphasTexture.UnlockRectangle(0);
               if (doLight) mHandles[indx].mHandle.mLightTexture.UnlockRectangle(0);
               if (doTesselation) mHandles[indx].mHandle.mTessellationTexture.UnlockRectangle(0);
            }
         }

          return indx;
      }


      public void freeVisualHandle(int handleIndx)
      {
         if (mHandles != null && handleIndx < mHandles.Count && mHandles[handleIndx] != null)
         {
            mHandles[handleIndx].inUse = false;
         }
      }
      public BTerrainVisualDataHandle getVisualHandle(int handleIndx)
      {
         return mHandles[handleIndx];
      }
      private int findFirstFreeHandle(int lod)
      {
         for (int i = 0; i < mHandles.Count; i++)
         {
            if (!mHandles[i].inUse && mHandles[i].LOD == lod)
            {
               return i;
            }
         }

         BTerrainVisualDataHandle vdh = new BTerrainVisualDataHandle();
         vdh.mHandle = new BTerrainVisualData();
         vdh.inUse = false;
         mHandles.Add(vdh);

         return mHandles.Count - 1;
      }

      //----------------------------
      public int getLODLevel(Vector3 center)
      {
         return (int)mLODData.getLODLevel(center, TerrainGlobals.getTerrainFrontEnd().mCameraManager.mEye);
      }

      public void setDynamicLODEnabled(bool onoff)
      {
         mDyanimcLODEnabled = onoff;
      }
      public bool isDynamicLODEnabled()
      {
         return mDyanimcLODEnabled;
      }
      public void setStaticLODLevel(eLODLevel level)
      {
         mStaticLODLevel = level;
      }
      public eLODLevel getStaticLODLevel()
      {
         return mStaticLODLevel;
      }

      public Texture giveEntireTerrainInTexture()
      {
         try
         {
            int width = TerrainGlobals.getTerrain().getNumXVerts();
            int height = TerrainGlobals.getTerrain().getNumXVerts();
            Texture mainTex = new Texture(BRenderDevice.getDevice(), width, height, 1, Usage.None, Format.A32B32G32R32F, Pool.Managed);

            Vector3[] details = TerrainGlobals.getEditor().getDetailPoints();
            unsafe
            {
               GraphicsStream streamPos = mainTex.LockRectangle(0, LockFlags.None);
               Vector4* pos = (Vector4*)streamPos.InternalDataPointer;

               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     int srcIndex = x * width + y;
                     int dstIndex = x * width + y;
                     Vector3 input = details[srcIndex];
                     pos[dstIndex].X = input.X;
                     pos[dstIndex].Y = input.Y;
                     pos[dstIndex].Z = input.Z;
                     pos[dstIndex].W = 1;
                  }
               }

               mainTex.UnlockRectangle(0);
            }

            return mainTex;
         }
         catch (OutOfVideoMemoryException eGPU)
         {
            if (!CoreGlobals.IsBatchExport)
               MessageBox.Show("giveEntireTerrainInTexture: You've run out of graphics card memory... Talk to the editor guys..");
            return null;
         }
         catch(OutOfMemoryException eCPU)
         {
            if (!CoreGlobals.IsBatchExport)
               MessageBox.Show("giveEntireTerrainInTexture: You've run out of memory... Talk to the editor guys..");
            return null;
         }
         return null;
      }

      //--------------------------------------MEMBERS

      //these are the handles we deal out to everyone.
      private List<BTerrainVisualDataHandle> mHandles;


      static public int cVisualTextureImageWidth = 512;
      static public int cVisualTextureImageHeight = 512;

      //LOD Data
      private BTerrainVisualLODData mLODData = null;
      bool mDyanimcLODEnabled = true;
      eLODLevel mStaticLODLevel = eLODLevel.cLOD0;

   };



    #endregion




}
