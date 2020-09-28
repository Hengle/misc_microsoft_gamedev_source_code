using System;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Drawing;
using System.Diagnostics;

using Rendering;
using EditorCore;

namespace FinalGather
{
   class FGScene
   {
      Viewport mViewport;
      Matrix mWorldMatrix = Matrix.Identity;
      Matrix mViewMatrix = Matrix.Identity;
      Matrix mProjectionMatrix = Matrix.Identity;
      Surface mBackBuffer = null;
      Surface mDepthStencil = null;

      BRenderDebugFullscreenQuad mFullscreenQuad = null;

      Texture mTempTargetTexFLOAT = null;
      Surface mTempTargetSurfFLOAT = null;
      Texture mTempResolveTexFLOAT = null;
      Surface mTempResolveSurfFLOAT = null;

      Texture mTempTargetTexCOLOR = null;
      Surface mTempTargetSurfCOLOR = null;
      Texture mTempResolveTexCOLOR = null;
      Surface mTempResolveSurfCOLOR = null;

      Texture mPointBufferA = null;
      Surface mPointBufferASurf = null;

      Texture mDepthBufferC = null;
      Surface mDepthBufferCSurf = null;

      Texture mCurrentDepthLayerD = null;
      Surface mCurrentDepthLayerDSurf = null;

      Texture mIrradianceE = null;
      Surface mIrradianceESurf = null;

      Query mOC = null;

      int mWidth = 256;
      int mHeight = 256;

      SceneManager.SimpleScene mScene;
     

      ShaderHandle mDepthPeelShader = null;
      public EffectHandle mShaderPrevDepthTexture;
      public EffectHandle mShaderWorldPosFromCam;
      public EffectHandle mShaderGlobalRayDir;

      public EffectHandle mShaderWorldToCamMatrix;       //step5
      public EffectHandle mShaderWorldToLightMatrix;//step5

      public EffectHandle mShaderIrradianceTexture;
      public EffectHandle mShaderDepthLayerTexture;
      
      public EffectHandle mShaderWVPHandle;
      enum eShaderPassIndex
      {
         eShaderPass0_ScenePosition =0,
         eShaderPass5_ProjectLayerTest = 1,
         eShaderPass6_CompareDepth =2,
         eShaderPass9_AccumulateIrradiance = 3,
      };


      void loadShader()
      {
         mDepthPeelShader = BRenderDevice.getShaderManager().getShader(AppDomain.CurrentDomain.BaseDirectory + "\\shaders\\GPUFinalGather.fx", null);

         mShaderWVPHandle = mDepthPeelShader.getEffectParam("worldViewProj");
         mShaderPrevDepthTexture = mDepthPeelShader.getEffectParam("prevDepthTexture");
         mShaderGlobalRayDir = mDepthPeelShader.getEffectParam("globalRayDirection");
         mShaderWorldPosFromCam = mDepthPeelShader.getEffectParam("worldPosFromCamTexture");

         mShaderWorldToCamMatrix = mDepthPeelShader.getEffectParam("gWorldToCamera");
         mShaderWorldToLightMatrix = mDepthPeelShader.getEffectParam("gWorldToLight");

         mShaderIrradianceTexture = mDepthPeelShader.getEffectParam("irradianceTexture");

         mShaderDepthLayerTexture = mDepthPeelShader.getEffectParam("depthLayerTexture");
         
         
         
         
         
      }
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
      //-------------------------
     
      //-------------------------
      void initTextures()
      {
         Format fmt = Format.R32F;
         mTempTargetTexFLOAT = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, Usage.RenderTarget, fmt, Pool.Default);
         mTempTargetSurfFLOAT = mTempTargetTexFLOAT.GetSurfaceLevel(0);
         mTempResolveTexFLOAT = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, fmt, Pool.SystemMemory);
         mTempResolveSurfFLOAT = mTempResolveTexFLOAT.GetSurfaceLevel(0);

         mTempTargetTexCOLOR = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, Usage.RenderTarget, Format.A8R8G8B8, Pool.Default);
         mTempTargetSurfCOLOR = mTempTargetTexCOLOR.GetSurfaceLevel(0);
         mTempResolveTexCOLOR = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, Format.A8R8G8B8, Pool.SystemMemory);
         mTempResolveSurfCOLOR = mTempResolveTexCOLOR.GetSurfaceLevel(0);

         mPointBufferA = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, Usage.RenderTarget, Format.A32B32G32R32F, Pool.Default);
         mPointBufferASurf = mPointBufferA.GetSurfaceLevel(0);


         mDepthBufferC = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, fmt, Pool.Managed);
         mDepthBufferCSurf = mDepthBufferC.GetSurfaceLevel(0);

         //this should be a color texture..
         mCurrentDepthLayerD = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, Format.A8R8G8B8, Pool.Managed);
         mCurrentDepthLayerDSurf = mCurrentDepthLayerD.GetSurfaceLevel(0);

         //this should be a color texture..
         mIrradianceE = new Texture(BRenderDevice.getDevice(), (int)mWidth, (int)mHeight, 1, 0, Format.A8R8G8B8, Pool.Managed);
         mIrradianceESurf = mIrradianceE.GetSurfaceLevel(0); 

      }
      public void init(int width, int height)
      {
         mScene = new SceneManager.SimpleScene();
         mScene.initScene();

         loadShader();

         mWidth = width;
         mHeight = height;

         initTextures();

         mFullscreenQuad = new BRenderDebugFullscreenQuad(mWidth, mHeight);

         mOC = new Query(BRenderDevice.getDevice(), QueryType.Occlusion);
      }
      public void destroy()
      {
         mScene.destroy();
         mScene = null;
         
         if (mDepthPeelShader != null)
         {
            BRenderDevice.getShaderManager().freeShader(mDepthPeelShader.mFilename);
            mDepthPeelShader.destroy();
            mDepthPeelShader = null;
         }

         mTempTargetSurfFLOAT.Dispose();
         mTempTargetSurfFLOAT = null;
         mTempTargetTexFLOAT.Dispose();
         mTempTargetTexFLOAT = null;

         mTempTargetSurfCOLOR.Dispose();
         mTempTargetSurfCOLOR = null;
         mTempTargetTexCOLOR.Dispose();
         mTempTargetTexCOLOR = null;

         mFullscreenQuad.destroy();
      }
      //--------------------------------------------------------------------------
      void computeTransforms(ref Vector3 rayDir, out Matrix worldToView, out Matrix viewToProj)
      {
         Vector3 mWorldMin = mScene.getWorldMin();
         Vector3 mWorldMax = mScene.getWorldMax();

         Vector3 eyePos = new Vector3((mWorldMin.X + mWorldMax.X) * .5f, mWorldMax.Y + 50.0f, (mWorldMin.Z + mWorldMax.Z) * .5f);
         Vector3 focusPos = eyePos; focusPos.Y -= 1;
         Vector3 upDir = BMathLib.unitZ;
         worldToView = Matrix.LookAtLH(eyePos, focusPos, upDir);

         BBoundingBox worldBounds = new BBoundingBox();
         worldBounds.min = mWorldMin;
         worldBounds.min.Y -= 2;       //for depth peeling
         worldBounds.max = mWorldMax;
         worldBounds.max.Y += 2;       //for depth peeling

         BBoundingBox viewBounds = new BBoundingBox();

         for (int i = 0; i < 8; i++)
         {
            Vector3 corner = worldBounds.getCorner((BBoundingBox.eCornerIndex)i);
            Vector4 viewCorner = Vector3.Transform(corner, worldToView);

            viewBounds.addPoint(viewCorner.X, viewCorner.Y, viewCorner.Z);
         }

         float minZ = viewBounds.min.Z;
         float maxZ = viewBounds.max.Z;

         float offsetX = 0.5f / (viewBounds.max.X - viewBounds.min.X);
         float offsetY = 0;// -0.5f / (viewBounds.max.Y - viewBounds.min.Y);

         viewToProj = Matrix.OrthoOffCenterLH(
                  viewBounds.min.X + offsetX, viewBounds.max.X + offsetX,
                  viewBounds.min.Y + offsetY, viewBounds.max.Y + offsetY,
                  minZ, maxZ);

      }
      
      //----------------------------------------------
      void copySurface(Surface src, Surface dst)
      {
         Rectangle rect = new Rectangle(0,0,mWidth,mHeight);
         BRenderDevice.getDevice().StretchRectangle(src, rect, dst, rect, TextureFilter.Linear);
      }
      unsafe void copyTexturesFLOAT(Texture srcTex, Texture destTex)
      {
         GraphicsStream srcStream = srcTex.LockRectangle(0, LockFlags.None);
         float* src = (float*)srcStream.InternalDataPointer;
         GraphicsStream destStream = destTex.LockRectangle(0, LockFlags.None);
         float* dest = (float*)destStream.InternalDataPointer;

         int size = mWidth * mHeight;

         for (int i = 0; i < size; i++)
            dest[i] = src[i];

         srcTex.UnlockRectangle(0);
         destTex.UnlockRectangle(0);
      }
      unsafe void copyRT0toDepthTexture( Texture destTex)
      {
         BRenderDevice.getDevice().GetRenderTargetData(mTempTargetSurfFLOAT, mTempResolveSurfFLOAT);

       

         copyTexturesFLOAT(mTempResolveTexFLOAT, destTex);
      }
      unsafe void copyTexturesCOLOR(Texture srcTex, Texture destTex)
      {
         GraphicsStream srcStream = srcTex.LockRectangle(0, LockFlags.None);
         Int32* src = (Int32*)srcStream.InternalDataPointer;
         GraphicsStream destStream = destTex.LockRectangle(0, LockFlags.None);
         Int32* dest = (Int32*)destStream.InternalDataPointer;

         int size = mWidth * mHeight;

         for (int i = 0; i < size; i++)
            dest[i] = src[i];

         srcTex.UnlockRectangle(0);
         destTex.UnlockRectangle(0);
      }
      unsafe void copyRT1toColorTexture( Texture destTex)
      {
         BRenderDevice.getDevice().GetRenderTargetData(mTempTargetSurfCOLOR, mTempResolveSurfCOLOR);

         copyTexturesCOLOR(mTempResolveTexCOLOR, destTex);
      }

      //----------------------------------------------
     
      public void render()
      {
         //renderInternalScene();
         //return;

         copyTempD3DValues();
         Matrix worldToCamera = mViewMatrix * mProjectionMatrix;  //needed in step5 & 9
         
         mDepthPeelShader.mShader.Begin(0);


         calculateVisiblePoints(ref worldToCamera);

         //ADDITIVE BLENDING
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.One);

         
         

         

         int numSamples = 1;
         for (int sampleCount = 0; sampleCount < numSamples; sampleCount++)
         {
            BRenderDevice.getDevice().SetRenderTarget(0, mTempTargetSurfFLOAT);


            clearBuffers();




            //[ ] step4 sample one global ray direction by uniform sampling
            Vector3 globalRayDir = -BMathLib.unitY;
            Matrix worldToView = Matrix.Identity;
            Matrix viewToProj = Matrix.Identity;
            computeTransforms(ref globalRayDir, out worldToView, out viewToProj);
            
            Viewport viewport = new Viewport();
            viewport.X = 0;
            viewport.Y = 0;
            viewport.Width = (int)mWidth;
            viewport.Height = (int)mHeight;
            viewport.MinZ = 0.0f;
            viewport.MaxZ = 1.0f;
            BRenderDevice.getDevice().Viewport = viewport;

            Matrix worldToLight = worldToView * viewToProj;
            //  Vector3 currRayDir = BMathLib.unitY;
            //  mDepthPeelShader.mShader.SetValue(mShaderGlobalRayDir, currRayDir);


            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual); //redundant?


            int i = 0;
            do
            {
               generateIrradiance(ref worldToCamera, ref worldToLight);
               generateNextDepthLayer(ref worldToLight);

               if (testOcclusionResults())
                  break;
               //else
               //   copySurface(mTempTargetSurfCOLOR, mBackBuffer);//CLM copy to backBuffer for debugging... THIS WILL BE THE CLEAR COLOR ON EXIT!

               i++;
            } while (true);//(i < 4);//


            accumulateIrradiance();

         }//[X] step10 return to step 2 until the number of samples becomes sufficient.



         mDepthPeelShader.mShader.End();
         BRenderDevice.getDevice().SetRenderTarget(1, null);
         restoreTempD3DValues();

      }

      void calculateVisiblePoints(ref Matrix worldToCamera)
      {
         /////////////////////////////////////////////////////////////////////////////
         //[X] step 1 store the position and normal at each visible point to visible point buffer A
         // prepare position, normal, color of visible points.
         // To do so, simply render the scene from the camera and write out this data to floating-point buffers using a fragment shader
         BRenderDevice.getDevice().SetRenderTarget(0, mPointBufferASurf);
         BRenderDevice.getDevice().Clear(ClearFlags.Target, (unchecked((int)0x00000000)), 1, 0);
         mDepthPeelShader.mShader.SetValue(mShaderWVPHandle, worldToCamera);
         mDepthPeelShader.mShader.BeginPass((int)eShaderPassIndex.eShaderPass0_ScenePosition);
         mScene.renderScene();
         mDepthPeelShader.mShader.EndPass();
      }
      void clearBuffers()
      {
         /////////////////////////////////////////////////////////////////////////////
         //[X] step 2 initlize "depth" buffer B and buffer C to max distance
         BRenderDevice.getDevice().Clear(ClearFlags.Target, (unchecked((int)0xFFFFFFFF)), 1, 0);
         copyRT0toDepthTexture(mDepthBufferC);

         /////////////////////////////////////////////////////////////////////////////
         //[X] step3 clear the sampled depth layer buffer D to zero
         BRenderDevice.getDevice().SetRenderTarget(1, mTempTargetSurfCOLOR);
         BRenderDevice.getDevice().Clear(ClearFlags.Target, (unchecked((int)0x00000000)), 1, 0);
         copyRT1toColorTexture(mCurrentDepthLayerD);
         copyTexturesCOLOR(mCurrentDepthLayerD, mIrradianceE);  //clear irradiance as well.

            
      }
      void generateIrradiance(ref Matrix worldToCamera, ref Matrix worldToLight)
      {
         //CLM this provides accurate depth peeling results (in ISO view...)
         BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, (unchecked((int)0x00000000)), 0, 0);


         /////////////////////////////////////////////////////////////////////////////
         //[ ] step5 GENERATE THE IRRADIANCE FOR THIS DEPTH LAYER
         // Read buffer D at the position of parallel projection of the visible point in buffer A
         //and store the result into Buffer E if this point is in front of (read: depth is less than) 
         //the visible point along the selected global ray direction
         //CLM
         // Render fullscreen quad
         //   for each fragment, get the visible point (A) and project it into lightSpace
         //   get the depth (C) at the point that A lands.
         //   if A.depth > C.depth, E=D;
         mDepthPeelShader.mShader.SetValue(mShaderPrevDepthTexture, mDepthBufferC);
         mDepthPeelShader.mShader.SetValue(mShaderWorldPosFromCam, mPointBufferA);
         mDepthPeelShader.mShader.SetValue(mShaderWorldToCamMatrix, worldToCamera);
         mDepthPeelShader.mShader.SetValue(mShaderWorldToLightMatrix, worldToLight);
         mDepthPeelShader.mShader.SetValue(mShaderDepthLayerTexture, mCurrentDepthLayerD);

         mDepthPeelShader.mShader.BeginPass((int)eShaderPassIndex.eShaderPass5_ProjectLayerTest);
         mFullscreenQuad.render(true);
         mDepthPeelShader.mShader.EndPass();
         copyRT1toColorTexture(mIrradianceE);

         // copySurface(mTempTargetSurfCOLOR, mBackBuffer);//CLM copy to backBuffer for debugging...


      }
      void generateNextDepthLayer(ref Matrix worldToLight)
      {
         //CLM this provides accurate depth peeling results (in ISO view...)
         BRenderDevice.getDevice().Clear(ClearFlags.ZBuffer | ClearFlags.Target, (unchecked((int)0x00000000)), 0, 0);

         /////////////////////////////////////////////////////////////////////////////
         //[X] step6 GENERATE THE NEXT DEPTH LAYER
         // Render the scene geometry 
         // into buffer B/C (for depth)
         // into buffer D (for color) 
         // using parallel projection by selecting global ray direction, with reversed depth test
         // If the fragment of the point has larger depth value than buffer C, then it is culled. (this is handled in the shader.)
         mDepthPeelShader.mShader.SetValue(mShaderWVPHandle, worldToLight);
         mDepthPeelShader.mShader.SetValue(mShaderPrevDepthTexture, mDepthBufferC);
         mDepthPeelShader.mShader.BeginPass((int)eShaderPassIndex.eShaderPass6_CompareDepth);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Greater);

         mOC.Issue(IssueFlags.Begin);

         mScene.renderScene();

         mOC.Issue(IssueFlags.End);
         mDepthPeelShader.mShader.EndPass();

         copyRT0toDepthTexture(mDepthBufferC);
         copyRT1toColorTexture(mCurrentDepthLayerD);  //buffer D for color..
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
      void accumulateIrradiance()
      { 
         /////////////////////////////////////////////////////////////////////////////
         //[ ] step9 accumulate buffer E into indirect illumintation buffer
         //CLM E is in cameraSpace.
         // Render fullscreen quad
         // set blend mode to accumulate.
         BRenderDevice.getDevice().SetRenderTarget(0, mBackBuffer);
         BRenderDevice.getDevice().SetRenderTarget(1, null);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);

         mDepthPeelShader.mShader.SetValue(mShaderIrradianceTexture, mIrradianceE);
         mDepthPeelShader.mShader.BeginPass((int)eShaderPassIndex.eShaderPass9_AccumulateIrradiance);
         mFullscreenQuad.render(true);
         mDepthPeelShader.mShader.EndPass();

         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
            
      }
   }
}