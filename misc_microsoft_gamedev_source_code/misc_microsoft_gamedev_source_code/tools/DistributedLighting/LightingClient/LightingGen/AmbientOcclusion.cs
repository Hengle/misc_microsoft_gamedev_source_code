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
using Sim;

//----------------------------------

namespace LightingClient
{
     

   public class AmbientOcclusion
   {
      public AmbientOcclusion()
      {
      }
      ~AmbientOcclusion()
      {
         destroy();
      }
      public void destroy()
      {
         if(mPeeler!=null)
         {
            mPeeler.destroy();
            mPeeler = null;
         }
      }

      void blackoutAO()
      {
         int count = (int)(TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts);
         float[] mAmbientOcclusionValues = TerrainGlobals.getTerrain().mTerrainAOVals;
         for (int i = 0; i < count; i++)
            mAmbientOcclusionValues[i] = 0.0f;

      }
      
      public static long IncrementCount=0;
      public static int[] AOIncrements = null;
      bool mUsingMultithreading = false;
      //--------------------------------------------------------------------------
      //--------------------------------------------------------------------------
      void computeWorldToView(ref Vector3 rayDir, ref Vector3 mWorldMin, ref Vector3 mWorldMax,   out Matrix worldToView)
      {
         float diameter = Vector3.Length(mWorldMax - mWorldMin);
         float radius = diameter * 0.5f;
         Vector3 worldCenter = (mWorldMax - mWorldMin) * 0.5f;

         Vector3 eyePos = worldCenter + (-rayDir * radius);// new Vector3((mWorldMin.X + mWorldMax.X) * .5f, mWorldMax.Y + 5.0f, (mWorldMin.Z + mWorldMax.Z) * .5f);
         Vector3 focusPos = worldCenter;// eyePos; focusPos.Y -= 1;

         Vector3 upDir = BMathLib.unitY;

         Vector3 nFP = Vector3.Normalize(eyePos - focusPos);
         float dotRes = Vector3.Dot(nFP, BMathLib.unitY);
         if (dotRes > 0 && 1 - dotRes < BMathLib.cTinyEpsilon)
            upDir = BMathLib.unitZ;

         worldToView = Matrix.LookAtLH(eyePos, focusPos, upDir);
      }
      void computeViewToProj(ref Vector3 rayDir, BBoundingBox worldBounds, Matrix worldToView, out Matrix viewToProj, 
                  int numXSegments, int numYSegments, int xSegment, int ySegment)
      {
         BBoundingBox viewBounds = new BBoundingBox();

         for (int i = 0; i < 8; i++)
         {
            Vector3 corner = worldBounds.getCorner((BBoundingBox.eCornerIndex)i);
            Vector4 viewCorner = Vector3.Transform(corner, worldToView);

            viewBounds.addPoint(viewCorner.X, viewCorner.Y, viewCorner.Z);
         }

         float minZ = viewBounds.min.Z;// *0.5f;
         float maxZ = viewBounds.max.Z;// *2;

         float distanceX = Math.Abs(viewBounds.max.X - viewBounds.min.X);
         float segSizeX = distanceX / numXSegments;
         float distanceY = Math.Abs(viewBounds.max.Y - viewBounds.min.Y);
         float segSizeY = distanceY / numYSegments;

         float minXSeg = viewBounds.min.X + (segSizeX * xSegment);
         float minYSeg = viewBounds.min.Y + (segSizeY * ySegment);
         viewToProj = Matrix.OrthoOffCenterLH(
                  minXSeg, minXSeg + segSizeX,
                  minYSeg, minYSeg + segSizeY,
                  minZ, maxZ);
      }
      void calculateWorldBounds(ref  Vector3 mWorldMin, ref Vector3 mWorldMax, ref BBoundingBox worldBounds, bool renderWorldObjects)
      {
         float tileScale = TerrainGlobals.getTerrain().mTileScale;
         mWorldMin = TerrainGlobals.getTerrain().mTerrainBBMin;// -new Vector3(tileScale, tileScale, tileScale);
         mWorldMax = TerrainGlobals.getTerrain().mTerrainBBMax;// +new Vector3(tileScale, tileScale, tileScale);
         mWorldMin.Y -= 1.0f;
         mWorldMax.Y += 1.0f;
         mWorldMin.X = (float)Math.Floor(mWorldMin.X);
         mWorldMin.Y = (float)Math.Floor(mWorldMin.Y);
         mWorldMin.Z = (float)Math.Floor(mWorldMin.Z);
         mWorldMax.X = (float)Math.Ceiling(mWorldMax.X);
         mWorldMax.Y = (float)Math.Ceiling(mWorldMax.Y);
         mWorldMax.Z = (float)Math.Ceiling(mWorldMax.Z);

         worldBounds.min = mWorldMin;
         worldBounds.min.Y -= 2;
         worldBounds.max = mWorldMax;
         worldBounds.max.Y += 2;

      }

      //--------------------------------------------------------------------------
      //--------------------------------------------------------------------------
      #region final gathering
      class gatherWorkerData
      {
         public gatherWorkerData(int mnX, int mnZ, int mxX, int mxZ, int width, int height, Vector3 RayDir, Matrix worldToProj, float rcpnumSamples)
         {
            minXVert = mnX;
            minZVert = mnZ;
            maxXVert = mxX;
            maxZVert = mxZ;

            imgWidth = width;
            imgHeight = height;
            rayDir = RayDir;
            workdToProj = worldToProj;
            rcpNumSamples = rcpnumSamples;

            fragList = new DepthPeelTerrain.fragmentDepths[width, height];
            for (int k = 0; k < width; k++)
            {
               for (int j = 0; j < height; j++)
               {
                  fragList[k, j] = new DepthPeelTerrain.fragmentDepths();
                  fragList[k, j].mDepths = new List<float>();
               }
            }

            
         }
         ~gatherWorkerData()
         {
            destroy();
         }
         public void destroy()
         {
            if(fragList!=null)
            {
               for (int i = 0; i < imgWidth; i++)
               {
                  for (int j = 0; j < imgHeight; j++)
                  {
                     if (fragList[i, j]!=null)
                        if (fragList[i, j].mDepths!=null)
                           fragList[i, j].mDepths.Clear();
                     fragList[i, j] = null;
                  }
               }
            }
            fragList = null;
         }
         public int minXVert = 0;
         public int minZVert = 0;
         public int maxXVert = 0;
         public int maxZVert = 0;

         public int imgWidth = 0;
         public int imgHeight = 0;

         public DepthPeelTerrain.fragmentDepths[,] fragList = null; //this is a pointer to the main fragment list..

         public Matrix workdToProj;

         public float rcpNumSamples = 0;
         public Vector3 rayDir = Vector3.Empty;
      };
      void finalGatherWorker(object workerObjectDat)
      {
         gatherWorkerData workerDat = workerObjectDat as gatherWorkerData;
         int numXVerts = (int)TerrainGlobals.getTerrain().mNumXVerts;
         Matrix worldToProj = workerDat.workdToProj;
         
         for (int x = workerDat.minXVert; x < workerDat.maxXVert; x++)
         {
            for (int z = workerDat.minZVert; z < workerDat.maxZVert; z++)
            {

               //only cast rays towards the light..
               Vector3 nrm = -TerrainGlobals.getTerrain().getNormal(x,z);
               if (BMathLib.Dot(ref nrm, ref workerDat.rayDir) < 0)
                  continue;


               Vector3 pos = TerrainGlobals.getTerrain().getPos(x, z);
               Vector4 vPos = new Vector4(pos.X, pos.Y, pos.Z, 1);
               Vector4 rPos = BMathLib.vec4Transform(ref vPos, ref worldToProj);
               rPos = rPos * (1.0f / rPos.W);
               float depth = rPos.Z;

               if (rPos.X < -1 || rPos.X >= 1 || rPos.Y < -1 || rPos.Y >= 1)
                  continue;

               //grab our location in the depthGrid (screen space)
               float xPos = BMathLib.Clamp(0.5f * rPos.Y + 0.5f, 0, 1);
               float yPos = BMathLib.Clamp(0.5f * rPos.X + 0.5f, 0, 1);
               int xGridLoc = (int)((workerDat.imgWidth - 1) * (1 - xPos));
               int yGridLoc = (int)((workerDat.imgHeight - 1) * (yPos));

               if (workerDat.fragList[xGridLoc, yGridLoc]!=null && !workerDat.fragList[xGridLoc, yGridLoc].anyValueSmaller(depth))
               {
                  int index = z + x * numXVerts;
                  Interlocked.Increment(ref AmbientOcclusion.AOIncrements[index]);
               }
            }
         }
         Interlocked.Increment(ref AmbientOcclusion.IncrementCount);
       
         workerDat.destroy();
         workerDat = null;
         workerObjectDat = null;
      }
      void finalGatherMultiThreaded(gatherWorkerData wd)
      {
         ThreadPool.QueueUserWorkItem(new WaitCallback(finalGatherWorker), wd);
         wd = null;
      }
      void finalGatherSingleThreaded(gatherWorkerData wd)
      {   
         {
            float[] AOVals = TerrainGlobals.getTerrain().mTerrainAOVals;
            Vector3[] normals = TerrainGlobals.getTerrain().mTerrainNormals;
            int numXVerts = (int)TerrainGlobals.getTerrain().mNumXVerts;
            int numZVerts = (int)TerrainGlobals.getTerrain().mNumZVerts;
            Matrix worldToProj = wd.workdToProj;

            for (int x = 0; x < numXVerts; x++)
            {
               for (int z = 0; z < numZVerts; z++)
               {
                  int index = z + x * numXVerts;

                  //only cast rays towards the light..
                  Vector3 nrm = -normals[index];
                  if (BMathLib.Dot(ref nrm, ref wd.rayDir) < 0)
                     continue;


                  Vector3 pos = TerrainGlobals.getTerrain().getPos(x, z);
                  Vector4 vPos = new Vector4(pos.X, pos.Y, pos.Z, 1);
                  Vector4 rPos = BMathLib.vec4Transform(ref vPos, ref worldToProj);
                  rPos = rPos * (1.0f / rPos.W);
                  float depth = rPos.Z;// +0.07f;//CLM this matches the shader...

                  if (rPos.X < -1 || rPos.X >= 1 || rPos.Y < -1 || rPos.Y >= 1)
                     continue;

                  //grab our location in the depthGrid (screen space)
                  float xPos = BMathLib.Clamp(0.5f * rPos.Y + 0.5f, 0, 1);
                  float yPos = BMathLib.Clamp(0.5f * rPos.X + 0.5f, 0, 1);
                  

                  int xGridLoc = (int)((wd.imgWidth - 1) * (1 - xPos));
                  int yGridLoc = (int)((wd.imgHeight - 1) * (yPos));

                  if (!wd.fragList[xGridLoc, yGridLoc].anyValueSmaller(depth))
                  {
                     
                     AOVals[index] += wd.rcpNumSamples;
                     Debug.Assert(AOVals[index] <= 1);
                  }
               }
            }
         }
      }
      #endregion

      //--------------------------------------------------------------------------
      //--------------------------------------------------------------------------


      
      public enum eAOQuality
      {
         cAO_Off = 0,
         cAO_Worst = 50,
         cAO_Medium = 300,
         cAO_Best = 800,
         cAO_Final = 1200,
      }
      public void calcualteAO(eAOQuality quality, bool renderWorldObjects)
      {
         float totalTime = 0;
         float peelTime=0;
         float gatherTime=0;
         calcualteAO(quality, renderWorldObjects, ref totalTime, ref peelTime, ref gatherTime);
      }

      public void calcualteAO(eAOQuality quality, bool renderWorldObjects, ref float totalTime, ref float peelTime, ref float gatherTime)
      {
         calcualteAO(quality, renderWorldObjects, ref totalTime, ref peelTime, ref gatherTime,-1, -1);
      }
      public void calcualteAO(eAOQuality quality, bool renderWorldObjects, ref float totalTime, ref float peelTime, ref float gatherTime,
                              int startSampleIndex, int endSampleIndex)
      {
         totalTime = 0;
         peelTime = 0;
         gatherTime = 0;

         DateTime n = DateTime.Now;


         //should we multithread?
         if (TerrainGlobals.mProcessorInfo.NumLogicalProcessors > 1)
         {
            mUsingMultithreading = true;
            AmbientOcclusion.IncrementCount = 0;
            AOIncrements = new int[TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts];
         }
         else
         {
            mUsingMultithreading = false;
         }




         int width = 512;
         int height = 512;

         int numXSegments = 1;
         int numYSegments = 1;

         //special settings for Final mode
         if (quality == eAOQuality.cAO_Final)
         {
            numXSegments = 4; //what should these values really be?
            numYSegments = 4; // they should scale, so that huge terrain gets more segments..

         //   width = 1024;
          //  height = 1024;

           // mUsingMultithreading = false;
         }

         
         blackoutAO();  //so we can gather light data

         Vector3 mWorldMin = Vector3.Empty;
         Vector3 mWorldMax = Vector3.Empty;
         BBoundingBox worldBounds = new BBoundingBox();
         calculateWorldBounds(ref mWorldMin, ref mWorldMax, ref worldBounds, renderWorldObjects);


         mPeeler.init(width, height);

         Vector3 rayDir = -BMathLib.unitY;
         rayDir.Normalize();
         Matrix worldToView = Matrix.Identity;
         Matrix viewToProj = Matrix.Identity;

         int numDesiredSamples = (int)Math.Sqrt((int)quality);

         int startIndex = 0;
         int endIndex = numDesiredSamples;
         List<Vector3> rayDirs = null;

         if (startSampleIndex != -1)
         {
            rayDirs = giveStratifiedOnHemiSphere(0, numDesiredSamples, false);
            startIndex = startSampleIndex;
            endIndex = Math.Min(endSampleIndex, rayDirs.Count);
         }
         else
         {
            rayDirs = giveStratifiedOnHemiSphere(0, numDesiredSamples, true);
            startIndex = 0;
            endIndex = rayDirs.Count;
         }
         
         float rcpNumSamples = 1.0f / (float)(rayDirs.Count-1);


         int numSamples = endIndex - startIndex;
         //begin our peel & gather process...
         BRenderDevice.getDevice().BeginScene();
         for (int dc = startIndex; dc < endIndex; dc++)
         {
            //choose one
            rayDir = -rayDirs[dc];

            computeWorldToView(ref rayDir, ref mWorldMin, ref mWorldMax, out worldToView);
            for (int x = 0; x < numXSegments; x++)
            {
               for (int y = 0; y < numYSegments; y++)
               {
                  //calculate our lightspace projection matrix   
                  computeViewToProj(ref rayDir, worldBounds, worldToView, out viewToProj,numXSegments, numYSegments, x,y);


                  //use worker threads..
                  gatherWorkerData wd = new gatherWorkerData(0, 0, (int)TerrainGlobals.getTerrain().mNumXVerts, (int)TerrainGlobals.getTerrain().mNumZVerts,
                                                            width, height,
                                                            rayDir,
                                                            worldToView * viewToProj,
                                                            rcpNumSamples);
                  mPeeler.mDepthArray = null;
                  mPeeler.mDepthArray = wd.fragList;

                  


                  //create our depth layers
                  DateTime dpn = DateTime.Now;
                  mPeeler.depthPeel(ref rayDir, ref worldToView, ref viewToProj, renderWorldObjects);
                  TimeSpan dpts = DateTime.Now - dpn;
                  peelTime += (float)dpts.TotalMinutes;



                  //do final gathering (AO SPECIFIC)
                  if (mUsingMultithreading)
                  {
                     finalGatherMultiThreaded(wd);
                  }
                  else
                  {
                     DateTime fgn = DateTime.Now;
                     finalGatherSingleThreaded(wd);
                     TimeSpan fgts = DateTime.Now - fgn;
                     gatherTime += (float)fgts.TotalMinutes;
                  }
                  wd = null;
                  mPeeler.mDepthArray = null;
               }
            }
         }




         BRenderDevice.getDevice().EndScene();
         mPeeler.destroy();




         //if we're using multithreading, wait for all our worker threads to finish.
         if (mUsingMultithreading)
         {
            while (AmbientOcclusion.IncrementCount <= numSamples - 1)
            {
               Thread.Sleep(1);
            }

            float[] AOVals = TerrainGlobals.getTerrain().mTerrainAOVals;
            for(int i=0;i<AOIncrements.Length;i++)
            {
               AOVals[i] = AOIncrements[i] * rcpNumSamples;
            }
         } 

         TimeSpan ts = DateTime.Now - n;
         totalTime = (float)ts.TotalMinutes;
      }

      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      List<Vector3> giveRandomOnHemiSphere(int numPositions)
      {
         List<Vector3> points = new List<Vector3>();
         points.Add(BMathLib.unitY);

         //Random rand = new Random(DateTime.Now.Millisecond);

         //for (int i = 0; i < numPositions - 1; i++)
         //{
         //   Vector3 pt = Vector3.Empty;

         //   pt.X = (float)(rand.NextDouble() * 2 - 1);
         //   pt.Y = (float)(rand.NextDouble() * 2 - 1);
         //   pt.Z = (float)(rand.NextDouble() * 2 - 1);
         //   pt.Normalize();
         //   if (pt.Y < 0)
         //      pt = -pt;   //KEEP POINTS IN UPPER HEMISPHERE!!!!!
         //   points.Add(pt);
         //}

         return points;
      }
      List<Vector3> giveStratifiedOnHemiSphere(float n, int sqrtNumSamples, bool randomReoganize)
      {
         // n = -1 : random (sphere)
         // n = 0 : proportional to solid angle (hemisphere)
         // n = 1 : proportional to cosine-weighted solid angle (hemisphere)
         // n > 1 : proportional to cosine lobe around normal, n = power (hemisphere)

         List<Vector3> points = new List<Vector3>();
         

         Random rand = new Random(0xB00B5);//HA! Boobs.

         
         if (n >= 0.0f)
            n = 1.0f / (1.0f + n);

         float q = 0.0f;   
         int index = 0;
         for(int i = 0; i < sqrtNumSamples; i++)
         {
            for(int j = 0; j < sqrtNumSamples; j++)
            {
               double x = (i + rand.NextDouble()) / sqrtNumSamples;
               double y = (j + rand.NextDouble()) / sqrtNumSamples;


               double theta, phi;
               if (n < 0.0f)
                  theta = Math.Acos(1.0f - 2.0f * x);
               else
                  theta = Math.Acos(Math.Pow(x, (double)n));

               phi = 2.0f * BMathLib.cPI * y;

               Vector3 point = new Vector3(
                              (float)(Math.Sin(theta) * Math.Cos(phi)),
                              (float)(Math.Sin(theta) * Math.Sin(phi)),
                              (float)Math.Cos(theta));

               if(point.Y < 0)
                  point = -point;   //keep in upper hemisphere

               points.Add(point);
               q += (float)Math.Cos(theta);            

               index++;            
            }
         }

         //CLM don't reoganize since we're distributed
         ////randomly re-organize points so we can 'quality metric'
         //if (randomReoganize)
         //{
         //   Random rnd = new Random();
         //   int numRands = sqrtNumSamples >> 1;
         //   for(int k=0;k<numRands;k++)
         //   {
         //      int A = rnd.Next(sqrtNumSamples);
         //      int B = rnd.Next(sqrtNumSamples);

         //      Vector3 t = points[A];
         //      points[A] = points[B];
         //      points[B] = t;
         //   }
         //}
         return points;
      }
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      public class DepthPeelTerrain
      {
         Viewport mViewport;
         Matrix mWorldMatrix = Matrix.Identity;
         Matrix mViewMatrix = Matrix.Identity;
         Matrix mProjectionMatrix = Matrix.Identity;
         Surface mBackBuffer = null;
         Surface mDepthStencil = null;

         //these texutres used to peel the data
         Texture mTempTargetTexFLOAT = null;
         Surface mTempTargetSurfFLOAT = null;
         Texture mTempResolveTexFLOAT = null;
         Surface mTempResolveSurfFLOAT = null;

         Surface pDepthStencilSurf = null;

         //current peeled depth
         Texture mDepthBufferC = null;
         Surface mDepthBufferCSurf = null;


         Query mOC = null; //used to allow us to know how many depths we're getting

         int mWidth = 256;
         int mHeight = 256;


         public EffectHandle mShaderPrevDepthTexture; //this is a handle to a variable in BTerrainRender::mTerrainGPUShader


         public class fragmentDepths
         {
            public fragmentDepths clone()
            {
               fragmentDepths fd = new fragmentDepths();
               fd.mDepths = new List<float>(mDepths.Count);
               for (int i = 0; i < mDepths.Count; i++)
                  fd.mDepths.Add(mDepths[i]);

               return fd;
            }
            public List<float> mDepths = null;
            public void insertValue(float val)
            {
               if (mDepths.Contains(val) || val == 0)
                  return;

               if (mDepths.Count == 0)
               {
                  mDepths.Add(val);
                  return;
               }

               for (int i = 0; i < mDepths.Count; i++)
               {
                  if (mDepths[i] > val)
                  {
                     mDepths.Insert(i, val);
                     return;
                  }
               }
            }

            public bool anyValueSmaller(float val)
            {
               for (int i = 0; i < mDepths.Count; i++)
               {
                  if (mDepths[i] < val)
                  {
                     return true;
                  }
               }
               return false;
            }
            public bool anyValueLarger(float val)
            {
               for (int i = 0; i < mDepths.Count; i++)
               {
                  if (mDepths[i] > val)
                  {
                     return true;
                  }
               }
               return false;
            }
         }
         public fragmentDepths[,] mDepthArray = null;

         //-----------------------
         void copyTempD3DValues()
         {
            mViewport = BRenderDevice.getDevice().Viewport;
            mWorldMatrix = BRenderDevice.getDevice().Transform.World;
            mViewMatrix = BRenderDevice.getDevice().Transform.View;
            mProjectionMatrix = BRenderDevice.getDevice().Transform.Projection;
            mBackBuffer = BRenderDevice.getDevice().GetRenderTarget(0);
            mDepthStencil = BRenderDevice.getDevice().DepthStencilSurface;
         }
         void restoreTempD3DValues()
         {
            BRenderDevice.getDevice().Viewport = mViewport;
            BRenderDevice.getDevice().Transform.World = mWorldMatrix;
            BRenderDevice.getDevice().Transform.View = mViewMatrix;
            BRenderDevice.getDevice().Transform.Projection = mProjectionMatrix;
            BRenderDevice.getDevice().SetRenderTarget(0, mBackBuffer);
            BRenderDevice.getDevice().DepthStencilSurface = mDepthStencil;
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         }
         //-----------------------


         //-------------------------
         void loadShader()
         {
            //grab these values from the TerrainRender shader, so we don't clog that pipe up any more than we have to..
            mShaderPrevDepthTexture = TerrainGlobals.getTerrainExportRender().mTerrainGPUShader.getEffectParam("prevDepthTexture");

         }
         void initTextures()
         {
            Format fmt = Format.R32F;
            mTempTargetTexFLOAT = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, Usage.RenderTarget, fmt, Pool.Default);
            mTempTargetSurfFLOAT = mTempTargetTexFLOAT.GetSurfaceLevel(0);
            mTempResolveTexFLOAT = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, fmt, Pool.SystemMemory);
            mTempResolveSurfFLOAT = mTempResolveTexFLOAT.GetSurfaceLevel(0);

            mDepthBufferC = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, fmt, Pool.Managed);
            mDepthBufferCSurf = mDepthBufferC.GetSurfaceLevel(0);

            pDepthStencilSurf = BRenderDevice.getDevice().CreateDepthStencilSurface((int)mWidth, (int)mHeight, DepthFormat.D24S8, MultiSampleType.None, 0, true);
         }
         public void init(int width, int height)
         {
            mWidth = width;
            mHeight = height;

            loadShader();
            initTextures();

            mOC = new Query(BRenderDevice.getDevice(), QueryType.Occlusion);

            mDepthArray = new fragmentDepths[width, height];
            for (int x = 0; x < width; x++)
            {
               for (int y = 0; y < height; y++)
               {
                  mDepthArray[x, y] = new fragmentDepths();
                  mDepthArray[x, y].mDepths = new List<float>();

               }
            }

      

         }
         public void destroy()
         {
            if (mTempTargetSurfFLOAT != null)
            {
               mTempTargetSurfFLOAT.Dispose();
               mTempTargetSurfFLOAT = null;
            }
            if (mTempTargetTexFLOAT != null)
            {
               mTempTargetTexFLOAT.Dispose();
               mTempTargetTexFLOAT = null;
            }
            if (mDepthBufferC != null)
            {
               mDepthBufferC.Dispose();
               mDepthBufferC = null;
            }
            if (pDepthStencilSurf != null)
            {
               pDepthStencilSurf.Dispose();
               pDepthStencilSurf = null;
            }
            if (mOC != null)
            {
               mOC.Dispose();
               mOC = null;
            }
            if (mDepthArray != null)
            {
               for (int x = 0; x < mWidth; x++)
               {
                  for (int y = 0; y < mHeight; y++)
                  {
                     mDepthArray[x, y].mDepths.Clear();
                     mDepthArray[x, y] = null;
                  }
               }
               mDepthArray = null;
            }

         }

         //----------------------------------------------
         unsafe void copyRT0toDepthTexture(Texture destTex, bool toArray)
         {
            BRenderDevice.getDevice().GetRenderTargetData(mTempTargetSurfFLOAT, mTempResolveSurfFLOAT);




            GraphicsStream srcStream = mTempResolveTexFLOAT.LockRectangle(0, LockFlags.None);
            float* src = (float*)srcStream.InternalDataPointer;
            GraphicsStream destStream = destTex.LockRectangle(0, LockFlags.None);
            float* dest = (float*)destStream.InternalDataPointer;

            if (toArray)
            {
             //  FileStream s = File.Open("outDepth.raw", FileMode.OpenOrCreate, FileAccess.Write);
             // BinaryWriter f = new BinaryWriter(s);

               int i = 0;
               for (int x = 0; x < mWidth; x++)
               {
                  for (int y = 0; y < mHeight; y++)
                  {
                     dest[i] = src[i];

                     float k = dest[i];
                     mDepthArray[x, y].insertValue(k);

                 //    ushort v = (ushort)(dest[i] * ushort.MaxValue);
                 //    f.Write(v);

                     i++;
                  }
               }
            //   f.Close();
            //   s.Close();
             }
            else
            {
               int size = mWidth * mHeight;
               for (int i = 0; i < size; i++)
                  dest[i] = src[i];
            }



            mTempResolveTexFLOAT.UnlockRectangle(0);
            destTex.UnlockRectangle(0);



         }
         //-------------------------------------------

         public void depthPeel(ref Vector3 rayDir, ref Matrix worldToView, ref Matrix viewToProj, bool renderWorldObjects)
         {
            //renderInternalScene();
            //return;

            clearDepths();

            copyTempD3DValues();

            BRenderDevice.getDevice().DepthStencilSurface = pDepthStencilSurf;
            BRenderDevice.getDevice().SetRenderTarget(0, mTempTargetSurfFLOAT);     //since we're generating depth

            clearBuffers();

            //NOTE, because of the internal rendering, we have to set these to set them to the hardware
            BRenderDevice.getDevice().Transform.World = Matrix.Identity;
            BRenderDevice.getDevice().Transform.View = worldToView;
            BRenderDevice.getDevice().Transform.Projection = viewToProj;

            Viewport viewport = new Viewport();
            viewport.X = 0;
            viewport.Y = 0;
            viewport.Width = (int)mWidth;
            viewport.Height = (int)mHeight;
            viewport.MinZ = 0.0f;
            viewport.MaxZ = 1.0f;
            BRenderDevice.getDevice().Viewport = viewport;


            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual); //redundant?


            
            int i = 0;
            do
            {

               generateNextDepthLayer(renderWorldObjects);


               if (testOcclusionResults())
                  break;

               copyRT0toDepthTexture(mDepthBufferC, true);

               i++;
            } while (true);//(i < 1);//

            restoreTempD3DValues();
         }

         void clearBuffers()
         {
            /////////////////////////////////////////////////////////////////////////////
            //[X] step 2 initlize "depth" buffer B and buffer C to max distance
            BRenderDevice.getDevice().Clear(ClearFlags.Target, (unchecked((int)0xFFFFFFFF)), 1, 0);
            copyRT0toDepthTexture(mDepthBufferC, false);
         }
         void generateNextDepthLayer(bool renderWorldObjects)
         {
            //CLM this provides accurate depth peeling results (in ISO view...)
            BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, (unchecked((int)0x00000000)), 0, 0);

            TerrainGlobals.getTerrainExportRender().mTerrainGPUShader.mShader.SetValue(mShaderPrevDepthTexture, mDepthBufferC);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Greater);

            mOC.Issue(IssueFlags.Begin);

            renderTerrain();
            if(renderWorldObjects)
               renderWoldObjects();

            mOC.Issue(IssueFlags.End);

         }
         bool testOcclusionResults()
         {
            /////////////////////////////////////////////////////////////////////////////
            //[X] step8 return to step 5 until the number of the rendered fragments becomes 0 at step 6
            int numPixVis = 0;
            bool datReturned = false;
            while (!datReturned)
               numPixVis = (int)mOC.GetData(typeof(int), true, out datReturned);

            if (numPixVis == 0) //CLM we can make this a threshold.
               return true;
            return false;
         }
         void renderTerrain()
         {
            TerrainGlobals.getTerrainExportRender().setPassNum(0);
            TerrainGlobals.getTerrainExportRender().preRenderSetup();
            TerrainGlobals.getTerrainExportRender().renderAll();
            TerrainGlobals.getTerrainExportRender().postRender();
         }
         void renderWoldObjects()
         {
            ModelManager.renderModels(mDepthBufferC);
           // SimGlobals.getSimMain().renderObjectsForAO(mDepthBufferC);
         }

         public void clearDepths()
         {
            if (mDepthArray != null)
            {
               for (int x = 0; x < mWidth; x++)
               {
                  for (int y = 0; y < mHeight; y++)
                  {
                     mDepthArray[x, y].mDepths.Clear();
                  }
               }
            }
         }

         public void dumpDepthGridToFile(string filename)
         {
            if (mDepthArray != null)
            {
               FileStream s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.Write);
               StreamWriter f = new StreamWriter(s);
               for (int x = 0; x < mWidth; x++)
               {
                  for (int y = 0; y < mHeight; y++)
                  {
                     f.Write("[" + x + "," + y + "] : ");
                     for (int i = 0; i < mDepthArray[x, y].mDepths.Count; i++)
                        f.Write(mDepthArray[x, y].mDepths[i] + ", ");
                     f.WriteLine(" ");
                  }
               }
               f.Close();
               s.Close();
            }
         }
      }
      DepthPeelTerrain mPeeler = new DepthPeelTerrain();
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
   }

}