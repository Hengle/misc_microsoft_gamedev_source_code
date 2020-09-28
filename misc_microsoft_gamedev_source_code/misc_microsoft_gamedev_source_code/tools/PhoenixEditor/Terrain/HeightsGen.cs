//#define DEBUG_AMBIENT_OCCLUSION


using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;
using System.Diagnostics;


using EditorCore;
using Rendering;
using SimEditor;
using Export360;

//----------------------------------

namespace Terrain
{

   public class HeightsGen
   {
      public HeightsGen()
      {

      }
      ~HeightsGen()
      {
         destroy();
      }
      public void destroy()
      {
         releaseHeightField();
      }
     

 

      //--------------------------------------------------------------------------
      void createTempTextures(uint width, uint height)
      {
         Format fmt = Format.R32F;//Format.A8R8G8B8;//
         pTargetTex = new Texture(BRenderDevice.getDevice(), (int)width, (int)height, 1, Usage.RenderTarget, fmt, Pool.Default);
         targetTexSurf = pTargetTex.GetSurfaceLevel(0);

         pHighDepths = new Texture(BRenderDevice.getDevice(), (int)width, (int)height, 1, 0, fmt, Pool.SystemMemory);
         pHighDepthSurf = pHighDepths.GetSurfaceLevel(0);

         pLowDepths = new Texture(BRenderDevice.getDevice(), (int)width, (int)height, 1, 0, fmt, Pool.SystemMemory);
         pLowDepthSurf = pLowDepths.GetSurfaceLevel(0);

         pDepthStencilSurf = BRenderDevice.getDevice().CreateDepthStencilSurface((int)width, (int)height, DepthFormat.D24S8, MultiSampleType.None, 0, true);
         
      }
      void releaseTempTextures()
      {
         pHighDepths.Dispose();
         pHighDepths = null;
         pHighDepthSurf.Dispose();
         pHighDepthSurf = null;

         pLowDepths.Dispose();
         pLowDepths = null;
         pLowDepthSurf.Dispose();
         pLowDepthSurf = null;

         pTargetTex.Dispose();
         pTargetTex = null;
         targetTexSurf.Dispose();
         targetTexSurf = null;

         pDepthStencilSurf.Dispose();
         pDepthStencilSurf = null;
      }
      //--------------------------------------------------------------------------
      //--------------------------------------------------------------------------
      void copyTempD3DValues()
      {
         mViewport = BRenderDevice.getDevice().Viewport;
         mWorldMatrix = BRenderDevice.getDevice().Transform.World;
         mViewMatrix = BRenderDevice.getDevice().Transform.View;
         mProjectionMatrix = BRenderDevice.getDevice().Transform.Projection;
         mRenderTarget = BRenderDevice.getDevice().GetRenderTarget(0);
         mDepthStencil = BRenderDevice.getDevice().DepthStencilSurface;
         BRenderDevice.beginScene();
      }
      void restoreTempD3DValues()
      {
         BRenderDevice.getDevice().Viewport = mViewport;
         BRenderDevice.getDevice().Transform.World = mWorldMatrix;
         BRenderDevice.getDevice().Transform.View = mViewMatrix;
         BRenderDevice.getDevice().Transform.Projection = mProjectionMatrix;
         BRenderDevice.getDevice().SetRenderTarget(0, mRenderTarget);
         BRenderDevice.getDevice().DepthStencilSurface = mDepthStencil;
         BRenderDevice.endScene();
      }
      //--------------------------------------------------------------------------
      void computeTransforms(Vector3 upDir, ref Matrix worldToView, ref Matrix viewToProj, ref BBoundingBox worldBounds, ref double worldMinY, ref double worldMaxY)
      {
         float diameter = Vector3.Length(mWorldMax - mWorldMin);
         float radius = diameter * 0.5f;
         Vector3 worldCenter = (mWorldMax - mWorldMin) * 0.5f;

         Vector3 eyePos = worldCenter + (BMathLib.unitY * radius);
         Vector3 focusPos = worldCenter;

         worldToView = Matrix.LookAtLH(eyePos, focusPos, upDir);

         worldBounds.min = mWorldMin;
    //     worldBounds.min.Y -= 2;
         worldBounds.max = mWorldMax;
     //    worldBounds.max.Y += 2;
         //worldBounds.max.X += 1; //CLM WHAT?
         //worldBounds.max.Z += 1; //CLM WHAT?


         BBoundingBox viewBounds = new BBoundingBox();

         for (int i = 0; i < 8; i++)
         {
            Vector3 corner = worldBounds.getCorner((BBoundingBox.eCornerIndex)i);
            Vector4 viewCorner = Vector3.Transform(corner, worldToView);
            viewBounds.addPoint(viewCorner.X, viewCorner.Y, viewCorner.Z);
         }

         float minZ = viewBounds.min.Z;// *0.5f;
         float maxZ = viewBounds.max.Z;// *2;

         viewToProj = Matrix.OrthoOffCenterLH(
                  viewBounds.min.X, viewBounds.max.X,
                  viewBounds.min.Y, viewBounds.max.Y,
                  minZ, maxZ);

         worldMinY = eyePos.Y + -minZ;
         worldMaxY = eyePos.Y + -maxZ;
      }
      //--------------------------------------------------------------------------
      void renderInit(uint width, uint height, Surface pRenderTargetSurf, bool furthestDepth, Matrix worldToView, Matrix viewToProj)
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().Transform.View = worldToView;
         BRenderDevice.getDevice().Transform.Projection = viewToProj;
         Viewport viewport = new Viewport();
         viewport.X = 0;
         viewport.Y = 0;
         viewport.Width = (int)width;
         viewport.Height = (int)height;
         viewport.MinZ = 0.0f;
         viewport.MaxZ = 1.0f;
         BRenderDevice.getDevice().Viewport = viewport;


         BRenderDevice.getDevice().DepthStencilSurface = pDepthStencilSurf;
         BRenderDevice.getDevice().SetRenderTarget(0, pRenderTargetSurf);


         BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, furthestDepth ? (unchecked((int)(0x00000000))) : (unchecked((int)(0xFFFFFFFF))), furthestDepth ? 0 : 1, 0);

         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, furthestDepth ? (int)Compare.GreaterEqual : (int)Compare.LessEqual);

         //BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
      }

      void renderTerrain(int passnum)
      {
         ExportTo360.mExportTerrainRender.setPassNum(passnum);
         ExportTo360.mExportTerrainRender.preRenderSetup();
         ExportTo360.mExportTerrainRender.renderAll();
         ExportTo360.mExportTerrainRender.postRender();
      }

      void renderDeinit(Texture pDepthStencilTex)
      {
         //BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, FALSE);

         //BD3D::mpDev->Resolve(D3DRESOLVE_DEPTHSTENCIL|D3DRESOLVE_ALLFRAGMENTS, NULL, pDepthStencilTex, NULL, 0, 0, NULL, 1.0f, 0, NULL);

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);


      }

      //--------------------------------------------------------------------------
      void generateDepthBuffer(ref Surface resultSurf, uint width, uint height, bool furthestDepth, Matrix worldToView, Matrix viewToProj, bool includeSimMod)
      {
         renderInit(width, height, targetTexSurf, furthestDepth, worldToView, viewToProj);

         renderTerrain(0);

         if (includeSimMod)
            SimGlobals.getSimMain().renderObjectsForDecal();

         renderDeinit(pTargetTex);

         BRenderDevice.getDevice().GetRenderTargetData(targetTexSurf, resultSurf);
      }
      //--------------------------------------------------------------------------
      void generateHeightBuffer(ref Surface resultSurf, uint width, uint height, bool furthestDepth, Matrix worldToView, Matrix viewToProj, bool includeSimMod)
      {
         renderInit(width, height, targetTexSurf, furthestDepth, worldToView, viewToProj);

         BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, 0, 1, 0);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);

         renderTerrain(2);

         if (includeSimMod)
            SimGlobals.getSimMain().renderObjectsForSimrep();

         renderDeinit(pTargetTex);

         BRenderDevice.getDevice().GetRenderTargetData(targetTexSurf, resultSurf);
      }
      //-----------------------------------------------------------------------------------

      //-----------------------------------------------------------------------------------
      unsafe void fillHeightFieldTexture(
                  uint width, uint height,
                  BHeightFieldAttributes heightAttribs,
                  Texture pLowDepths,
                  Texture pHighDepths,
                  ref double minDepthF, ref double maxDepthF)
      {

         GraphicsStream texstreamLOW = pLowDepths.LockRectangle(0, LockFlags.None);
         float* pSrcLow = (float*)texstreamLOW.InternalDataPointer;
         GraphicsStream texstreamHI = pHighDepths.LockRectangle(0, LockFlags.None);
         float* pSrcHigh = (float*)texstreamHI.InternalDataPointer;

         bool firstPass = (minDepthF == float.MaxValue);

         //find the max & min depth for this slice
         uint numVals = width * height;
         if (minDepthF == float.MaxValue)
         {
            minDepthF = float.MaxValue;
            maxDepthF = 0;

            for (uint c = 0; c < numVals; c++)
            {
               float l = pSrcLow[c];
               float h = pSrcHigh[c];

               if (!BMathLib.compare(l, 1) && !BMathLib.compare(l, 0))
               {
                  if (l < minDepthF) minDepthF = l;
                  if (l > maxDepthF) maxDepthF = l;
               }

               if (!BMathLib.compare(h, 1) && !BMathLib.compare(h, 0))
               {
                  if (h < minDepthF) minDepthF = h;
                  if (h > maxDepthF) maxDepthF = h;
               }
            }

            if (BMathLib.compare((float)minDepthF, float.MaxValue))
            {
               minDepthF = 0;
               maxDepthF = 1;
            }

            if (BMathLib.compare((float)minDepthF, (float)maxDepthF))
            {
               if (minDepthF == 0)
                  maxDepthF++;
               else
                  minDepthF--;
            }
         }


         //add the depth values to the array, compress them based upon difference
         float diff = (float)(maxDepthF - minDepthF);

         for (uint c = 0; c < numVals; c++)
         {
            float l = pSrcLow[c];
            if (!BMathLib.compare(l, 1) && !BMathLib.compare(l, 0))
            {
               if (l < minDepthF)
                  l = (float)minDepthF;
               else if (l > maxDepthF)
                  l = (float)maxDepthF;
            }

            float h = pSrcHigh[c];
            if (!BMathLib.compare(h, 1) && !BMathLib.compare(h, 0))
            {
               if (h < minDepthF)
                  h = (float)minDepthF;
               else if (h > maxDepthF)
                  h = (float)maxDepthF;
            }

            h = ((float)(h - minDepthF) / diff);
            l = ((float)(l - minDepthF) / diff);


            int shortRange = 0xFFFF;
            ushort hiVal = (ushort)BMathLib.Clamp((shortRange * h), 0, ushort.MaxValue);
            ushort loVal = (ushort)BMathLib.Clamp((shortRange * l), 0, ushort.MaxValue);

            if (firstPass)
            {
               //if(loVal != 0xFFFF && loVal != 0) 
               heightAttribs.mpTexelsLO[c] = loVal;
               //if (hiVal != 0xFFFF && hiVal != 0) 
               heightAttribs.mpTexelsHI[c] = hiVal;
            }
            else
            {
               // if (loVal != 0xFFFF && loVal != 0) 
               heightAttribs.mpTexelsLO[c] = (ushort)Math.Min(heightAttribs.mpTexelsLO[c], loVal);
               // if (hiVal != 0xFFFF && hiVal != 0) 
               heightAttribs.mpTexelsHI[c] = (ushort)Math.Min(heightAttribs.mpTexelsHI[c], hiVal);
            }

         }

         //fill in any blank spots that might be conflicting..

         //for (uint y = 0; y < height; y++)
         //{
         //   for (uint x = 0; x < width; x++)
         //   {
         //      uint ofs = x + y * mHeightFieldAttributes.mWidth;

         //      uint l = heightAttribs.mpTexelsLO[ofs];
         //      uint h = heightAttribs.mpTexelsHI[ofs];

         //      bool modified = false;

         //      if (l == 0)
         //      {
         //         float ave = 0.0f;
         //         uint num = 0;

         //         for (int xd = -1; xd < 1; xd++)
         //         {
         //            int yd;
         //            for (yd = -1; yd < 1; yd++)
         //            {
         //               int xx = (int)BMathLib.Clamp(xd + x, 0, width - 1);
         //               int yy = (int)BMathLib.Clamp(yd + y, 0, height - 1);

         //               uint p = heightAttribs.mpTexelsLO[xx + yy * mHeightFieldAttributes.mWidth];//lowHeights(xx, yy);
         //               if (p != 0)
         //               {
         //                  ave += p;
         //                  num++;
         //               }
         //            }
         //         }

         //         if (num != 0)
         //         {
         //            l = (uint)BMathLib.Clamp(BMathLib.FloatToIntRound(ave / num), 0, 65535U);
         //            heightAttribs.mpTexelsLO[ofs] = (ushort)l;
         //         }
         //      }

         //      modified = false;

         //      if (h == 1)
         //      {
         //         float ave = 0.0f;
         //         uint num = 0;

         //         for (int xd = -2; xd < 2; xd++)
         //         {
         //            int yd;
         //            for (yd = -2; yd < 2; yd++)
         //            {
         //               int xx = (int)BMathLib.Clamp(xd + x, 0, width - 1);
         //               int yy = (int)BMathLib.Clamp(yd + y, 0, height - 1);

         //               uint p = heightAttribs.mpTexelsHI[xx + yy * mHeightFieldAttributes.mWidth];//highHeights(xx, yy);
         //               if (p != 0)
         //               {
         //                  ave += p;
         //                  num++;
         //               }
         //            }
         //         }

         //         if (num != 0)
         //         {
         //            h = (uint)BMathLib.Clamp(BMathLib.FloatToIntRound(ave / num), 0, 65535U);
         //            heightAttribs.mpTexelsHI[ofs] = (ushort)h;
         //         }
         //      }
         //   }
         //}


         pLowDepths.UnlockRectangle(0);
         pHighDepths.UnlockRectangle(0);
      }
      //-----------------------------------------------------------------------------------

      //-----------------------------------------------------------------------------------
      void calculateWorldBounds(bool includeSimMod)
      {
         float tileScale = TerrainGlobals.getTerrain().getTileScale();
         mWorldMin = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_min;
         mWorldMax = TerrainGlobals.getTerrain().getQuadNodeRoot().getDesc().m_max;
         mWorldMin.Y -= 1.0f;
         mWorldMax.Y += 1.0f;
         mWorldMin.X = (float)Math.Floor(mWorldMin.X);
         mWorldMin.Y = (float)Math.Floor(mWorldMin.Y);
         mWorldMin.Z = (float)Math.Floor(mWorldMin.Z);
         mWorldMax.X = (float)Math.Ceiling(mWorldMax.X);
         mWorldMax.Y = (float)Math.Ceiling(mWorldMax.Y);
         mWorldMax.Z = (float)Math.Ceiling(mWorldMax.Z);

         if (includeSimMod)
         {
            //include any bounding boxes from objects we're including..
            BBoundingBox bb = SimGlobals.getSimMain().getBBoxForDecalObjects();
            bb.addPoint(mWorldMax);
            bb.addPoint(mWorldMin);
            mWorldMin.Y = bb.min.Y;
            mWorldMax.Y = bb.max.Y;
         }
         if (mWorldMin.X < 0) mWorldMin.X = 0;
         if (mWorldMin.Z < 0) mWorldMin.Z = 0;
      }
      public void computeHeightField(uint width, uint height, bool includeSimMod)
      {
         Vector3 upDir = BMathLib.unitZ;
         computeHeightField(width, height, includeSimMod, upDir,1);
      }
      public void computeHeightField(uint width, uint height, bool includeSimMod, Vector3 upDir, uint cNumSamples)
      {
         copyTempD3DValues();
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         TerrainGlobals.getVisual().destroyAllHandles();
         TerrainGlobals.getVisual().setHandleGenMode(true);
         GC.WaitForPendingFinalizers();

         ExportTo360.checkLoadRender();

         createTempTextures(width, height);

         calculateWorldBounds(includeSimMod);


         mHeightFieldAttributes = new BHeightFieldAttributes(width, height);

         Matrix worldToView = Matrix.Identity;
         Matrix viewToProj = Matrix.Identity;
         BBoundingBox worldBounds = new BBoundingBox();
         double worldMinY = 0, worldMaxY = 0;
         computeTransforms(upDir, ref worldToView, ref viewToProj, ref worldBounds, ref worldMinY, ref worldMaxY);

         Random rand = new Random();

         double minDepth = float.MaxValue;
         double maxDepth = float.MinValue;



         cNumSamples = (uint)BMathLib.Clamp(cNumSamples,1,32);
         
         for (uint sampleIndex = 0; sampleIndex < cNumSamples; sampleIndex++)
         {
            float xOfs = sampleIndex != 0 ? (float)((rand.NextDouble() * 2) - 1) : 0.0f;
            float yOfs = sampleIndex != 0 ? (float)((rand.NextDouble() * 2) - 1) : 0.0f;

            Matrix projOfs = Matrix.Translation(xOfs / width, yOfs / height, 0.0f);

            generateDepthBuffer(ref pHighDepthSurf, width, height, false, worldToView, viewToProj * projOfs, includeSimMod);
            generateDepthBuffer(ref pLowDepthSurf, width, height, true, worldToView, viewToProj * projOfs, includeSimMod);

            fillHeightFieldTexture(width, height, mHeightFieldAttributes, pLowDepths, pHighDepths, ref minDepth, ref maxDepth);

         }

         viewToProj = viewToProj * Matrix.Translation(0.0f, 0.0f, (float)(-minDepth)) * Matrix.Scaling(1.0f, 1.0f, (float)(1.0f / (maxDepth - minDepth)));
         double newWorldMinY = worldMinY + (worldMaxY - worldMinY) * minDepth;
         worldMaxY = worldMinY + (worldMaxY - worldMinY) * maxDepth;
         worldMinY = newWorldMinY;
         minDepth = 0.0f;
         maxDepth = 1.0f;

         mHeightFieldAttributes.mWidth = width;
         mHeightFieldAttributes.mHeight = height;
         mHeightFieldAttributes.mBounds = worldBounds;

         mHeightFieldAttributes.mWorldMinY = (float)worldMinY;
         mHeightFieldAttributes.mWorldMaxY = (float)worldMaxY;
         mHeightFieldAttributes.mWorldRangeY = (float)(worldMaxY - worldMinY);


         float minZ = 0;
         float maxZ = 1;
         float x = 0;
         float y = 0;

         Matrix cMTProjToScreen = BMathLib.matrixFrom16floats(
                              width * .5f, 0.0f, 0.0f, 0.0f,
                              0.0f, height * -.5f, 0.0f, 0.0f,
                              0.0f, 0.0f, maxZ - minZ, 0.0f,
                              x + width * .5f, y + height * .5f, minZ, 1.0f);

         mHeightFieldAttributes.mWorldToNormZ = worldToView * viewToProj * cMTProjToScreen;


         Matrix screenToProj = Matrix.Invert(cMTProjToScreen);
         Matrix projToView = Matrix.Invert(viewToProj);
         Matrix cMTScreenToView = Matrix.Multiply(screenToProj, projToView);

         mHeightFieldAttributes.mNormZToWorld = cMTScreenToView * Matrix.Invert(worldToView);

         releaseTempTextures();
         restoreTempD3DValues();
         TerrainGlobals.getVisual().setHandleGenMode(false);
         ExportTo360.checkUnloadRender();

         //copy our pixels from the previous point
         {
            for (int q = 0; q < width; q++)
            {
               mHeightFieldAttributes.mpTexelsLO[(width - 1) * width + q] = mHeightFieldAttributes.mpTexelsLO[(width - 2) * width + q];
               mHeightFieldAttributes.mpTexelsLO[q * width + (width - 1)] = mHeightFieldAttributes.mpTexelsLO[q * width + (width - 2)];
               mHeightFieldAttributes.mpTexelsLO[q] = mHeightFieldAttributes.mpTexelsLO[width + q];
               mHeightFieldAttributes.mpTexelsLO[q * width] = mHeightFieldAttributes.mpTexelsLO[q * width + (1)];

               mHeightFieldAttributes.mpTexelsHI[(width - 1) * width + q] = mHeightFieldAttributes.mpTexelsHI[(width - 2) * width + q];
               mHeightFieldAttributes.mpTexelsHI[q * width + (width - 1)] = mHeightFieldAttributes.mpTexelsHI[q * width + (width - 2)];
               mHeightFieldAttributes.mpTexelsHI[q] = mHeightFieldAttributes.mpTexelsHI[width + q];
               mHeightFieldAttributes.mpTexelsHI[q * width] = mHeightFieldAttributes.mpTexelsHI[q * width + (1)];

            }
         } 

         #region dumpHeightfiled
         //{
         //   FileStream s = File.Open(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory) + "\\out.raw", FileMode.OpenOrCreate, FileAccess.Write);
         //   BinaryWriter f = new BinaryWriter(s);
         //   for (int i = 0; i < width * height; i++)
         //   {
         //      f.Write(mHeightFieldAttributes.mpTexelsLO[i]);
         //      f.Write(mHeightFieldAttributes.mpTexelsHI[i]);
         //   }
         //   f.Close();
         //   s.Close();
         //}

         #endregion

      }
      void releaseHeightField()
      {
         if (mpHeightField != null)
         {
            mpHeightField.Dispose();
            mpHeightField = null;
         }
         if (mHeightFieldAttributes!=null)
         {
            mHeightFieldAttributes.destroy();
            mHeightFieldAttributes = null;
         }
      }
      //-----------------------------------------------------------------------------------
      //-----------------------------------------------------------------------------------

      public void computeHeightFieldDirectFloat(uint width, uint height, bool includeSimMod, Vector3 upDir, bool onlyDoOneSample, ref float[] output)
      {
         copyTempD3DValues();
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         TerrainGlobals.getVisual().destroyAllHandles();
         TerrainGlobals.getVisual().setHandleGenMode(true);
         GC.WaitForPendingFinalizers();

         ExportTo360.checkLoadRender();

         createTempTextures(width, height);

         calculateWorldBounds(includeSimMod);


         mHeightFieldAttributes = new BHeightFieldAttributes(width, height);

         Matrix worldToView = Matrix.Identity;
         Matrix viewToProj = Matrix.Identity;
         BBoundingBox worldBounds = new BBoundingBox();
         double worldMinY = 0, worldMaxY = 0;
         computeTransforms(upDir, ref worldToView, ref viewToProj, ref worldBounds, ref worldMinY, ref worldMaxY);

         float mWorldRangeY = (float)(mWorldMax.Y - mWorldMin.Y);

         uint numVals = width * height;
         if (output == null || output.Length != numVals)
         {
            output = new float[numVals];
            for (uint i = 0; i < numVals; i++)
               output[i] = float.MinValue;
         }

         uint cNumSamples = (uint)(onlyDoOneSample?1:4);
         Random rand = new Random();
         for (uint sampleIndex = 0; sampleIndex < cNumSamples; sampleIndex++)
         {
            float xOfs = sampleIndex != 0 ? (float)((rand.NextDouble() * 2) - 1) : 0.0f;
            float yOfs = sampleIndex != 0 ? (float)((rand.NextDouble() * 2) - 1) : 0.0f;

            Matrix projOfs = Matrix.Translation(xOfs / width, yOfs / height, 0.0f);

            generateHeightBuffer(ref pHighDepthSurf, width, height, true, worldToView, viewToProj * projOfs, includeSimMod);

            #region fill & lock

            //mWorldMin.Y += 1.0f;
            //mWorldMax.Y -= 1.0f;
        
            unsafe
            {
               GraphicsStream texstreamHI = pHighDepths.LockRectangle(0, LockFlags.None);
               float* pSrcHigh = (float*)texstreamHI.InternalDataPointer;

               //copy the data out so we can screw with it.
               for (uint c = 0; c < numVals; c++)
               {
                  output[c] = (float)Math.Max(output[c],pSrcHigh[c]);
               }
               pHighDepths.UnlockRectangle(0);
            }
            //find the max & min depth for this slice


            #endregion

         }
         
         


        

            releaseTempTextures();
         restoreTempD3DValues();
         TerrainGlobals.getVisual().setHandleGenMode(false);
         ExportTo360.checkUnloadRender();


         //#region dumpHeightfiled
         //{
         //   FileStream s = File.Open("_outHiDirect.raw", FileMode.OpenOrCreate, FileAccess.Write);
         //   BinaryWriter f = new BinaryWriter(s);
         //   for (int i = 0; i < width * height; i++)
         //   {
         //      float kp = (output[i] - mWorldMin.Y) / mWorldRangeY;
         //      ushort op = (ushort)(kp * ushort.MaxValue);
         //      f.Write(op);
         //   }
         //   f.Close();
         //   s.Close();
         //}

         //#endregion

      }
      //-----------------------------------------------------------------------------------
      //-----------------------------------------------------------------------------------
      public enum eHeightFileType
      {
         cType_R32 = 0,
         cType_R16 = 1,
         cType_R8 = 2,
      }
      public bool write_heights(string filename, eHeightFileType type)
      {
         computeHeightField(1024, 1024, false,-BMathLib.unitX,32);

         string ext = Path.GetExtension(filename);
         string baseName = filename.Substring(0, filename.LastIndexOf("."));

         if (filename.Contains("_LO."))
            baseName = filename.Substring(0, filename.LastIndexOf("_LO."));
         else if (filename.Contains("_HI."))
            baseName = filename.Substring(0, filename.LastIndexOf("_HI."));


         string lowName = baseName + "_LO" + ext;
         string hiName = baseName + "_HI" + ext;
         BinaryWriter fLO = new BinaryWriter(File.OpenWrite(lowName));
         BinaryWriter fHI = new BinaryWriter(File.OpenWrite(hiName));

         float minY = short.MaxValue + 1;
         float div = 1.0f / (float)(short.MaxValue - short.MinValue);
         for (uint y = 0; y < mHeightFieldAttributes.mHeight; y++)
         {
            for (uint x = 0; x < mHeightFieldAttributes.mWidth; x++)
            {
               uint ofs = (uint)(x + y * mHeightFieldAttributes.mWidth);

               float lowNormZ = 0.0f, highNormZ = 0.0f;



               if (type == eHeightFileType.cType_R32)
               {
                  lowNormZ = (ushort)(mHeightFieldAttributes.mpTexelsLO[ofs]) * div;
                  highNormZ = (ushort)(mHeightFieldAttributes.mpTexelsHI[ofs]) * div;

                  float l = 1 - lowNormZ;// (lowNormZ + minY);/// rangeY;
                  float h = 1 - highNormZ;// (highNormZ + minY);/// rangeY;
                  fLO.Write(l);
                  fHI.Write(h);
               }
               else if (type == eHeightFileType.cType_R16)
               {
                  ushort l = (ushort)(ushort.MaxValue - (ushort)(mHeightFieldAttributes.mpTexelsLO[ofs]));
                  ushort h = (ushort)(ushort.MaxValue - (ushort)(mHeightFieldAttributes.mpTexelsHI[ofs]));
                  fLO.Write(l);
                  fHI.Write(h);
               }
               else if (type == eHeightFileType.cType_R8)
               {
                  lowNormZ = (ushort)(mHeightFieldAttributes.mpTexelsLO[ofs]) * div;
                  highNormZ = (ushort)(mHeightFieldAttributes.mpTexelsHI[ofs]) * div;
                  byte l = (byte)((1 - lowNormZ) * Byte.MaxValue);
                  byte h = (byte)((1 - highNormZ) * Byte.MaxValue);
                  fLO.Write(l);
                  fHI.Write(h);
               }

            }
         }

         fLO.Close();
         fHI.Close();

         releaseHeightField();
         return true;
      }

      //-----------------------------------------------------------------------------------
      //-----------------------------------------------------------------------------------
      //-----------------------------------------------------------------------------------
      Texture mpHeightField = null;
      public BHeightFieldAttributes mHeightFieldAttributes = null;
      Vector3 mWorldMin = Vector3.Empty;
      Vector3 mWorldMax = Vector3.Empty;
      //--------------------------------------------------------------------------
      //temp D3D Values
      Viewport mViewport;
      Matrix mWorldMatrix = Matrix.Identity;
      Matrix mViewMatrix = Matrix.Identity;
      Matrix mProjectionMatrix = Matrix.Identity;
      Surface mRenderTarget = null;
      Surface mDepthStencil = null;

      //temp textures
      Texture pHighDepths; Surface pHighDepthSurf;
      Texture pLowDepths; Surface pLowDepthSurf;
      Texture pTargetTex; Surface targetTexSurf;
      Surface pDepthStencilSurf;

      public class BHeightFieldAttributes
      {
         public BHeightFieldAttributes(uint width, uint height)
         {
            mWidth = width;
            mHeight = height;
            mpTexelsLO = new ushort[width * height];
            mpTexelsHI = new ushort[width * height];
         }
         ~BHeightFieldAttributes()
         {
            destroy();
         }
         public void destroy()
         {
            mpTexelsHI = null;
            mpTexelsLO = null;
         }
         // Height field (x,y,z) to world space, where (x,y) are the texel coords, and z is the normalized depth [0.0, 1.0].
         public Matrix mNormZToWorld;

         // Transforms world coords to continuous height field map coordinates.
         public Matrix mWorldToNormZ;

         public uint mWidth;
         public uint mHeight;

         public ushort[] mpTexelsHI;//width*height
         public ushort[] mpTexelsLO;//width*height

         public BBoundingBox mBounds;

         public float mWorldMinY;
         public float mWorldMaxY;
         public float mWorldRangeY;
      };
   };
}