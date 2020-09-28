/*===================================
  texSynth.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis  (MGS Ensemble studios) [12.01.06]
===================================*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using Rendering;
using EditorCore;

namespace TextureSynthesis
{ 

   class SynthQuadNode
   {
      public SynthControlPacket mSynthControlPacket = new SynthControlPacket();
      public SynthResultPacket mSynthResultPacket = null;
      public SynthesisQuery m_Query = null;
      public void destroy()
      {
         m_Query = null;
         mSynthResultPacket.destroy();
         mSynthControlPacket = null;
      }

   }

   class TextureSynthesisMain
   {
      #region MEMBERS
      public Exemplar mExemplar = null;
      public Synthesiser mSynthesiser = null;


      
      public Synthesiser.SynthesisFlags m_SynthFlags = (Synthesiser.SynthesisFlags)(Synthesiser.SynthesisFlags._Standard | Synthesiser.SynthesisFlags._Subpass1234);

      List<SynthQuadNode> mSynthNodes = new List<SynthQuadNode>();

      Surface m_DepthStencilSurface = null;

      
      D3DQuad m_Quad=null;
      D3DQuad m_QuadDual = null;

      // Constraint map
      Texture m_d3dPaintLayer;
      Surface m_d3dPaintLayerSrf;
      public bool mPaintOrientation = true;
      public bool mPaintScale = false;

      TextureHandle m_d3dSplat;

      //jacobian (Local frames texture)
      public bool m_bNeedUpdateJacobianMap = false;
      Texture mJacobianMap;
      Surface mJacobianMapSrf;
      Texture mJacobianMapInverse;
      Surface mJacobianMapInverseSrf;
      
      Microsoft.DirectX.Direct3D.Effect m_fxJacobian;
      EffectHandle m_fxJacobianViewport;
      EffectHandle m_fxJacobianClearTech;
      EffectHandle m_fxJacobianSelectTech;
      EffectHandle m_fxGeomPaintJMapTech;
      EffectHandle m_fxJacobianGlobalScale;
      EffectHandle m_fxJacobianSelectedJMap;
      EffectHandle m_fxGeomGlobalScale;
      EffectHandle m_fxGeomPaintedJMap;
      EffectHandle m_fxGeomExemplarMainDirection;
      Surface savedDepthSten = null;

      public enum eJMapPattern
      {
         cNone = -1,
         cPaint =0,
         cIdentity,
         cSTwist,
         cSTwistScale,
         cScaling,
         cRotation,
         cRadial,
         cSink,
         c45DegRot,
      }
      private eJMapPattern mJMapPattern = eJMapPattern.cNone;
      public void setJMapPattern(eJMapPattern pattern)
      {
         mJMapPattern = pattern;

         m_bNeedUpdateJacobianMap = true;
      }
      public eJMapPattern getJMAPPattern()
      {
         return mJMapPattern;
      }

      float m_fExemplarMainDirection = 0;


      public bool m_bNeedRefresh = false;
      public bool m_bUpdateAllForce = false;
      public bool m_bNeedUpdate = false;


      Texture m_d3dRawDataBuffer;

      // size of synthesized viewport
      int m_iSynthWidth = 640;
      int m_iSynthHeight = 480;
      int m_iBuffersWidth;
      int m_iBuffersHeight;

      int mScalingFactor;  //CLM used to synth @ one res, and resolve to another.
      
      int m_iSynthLevelStart = 0;

      //DIALOG VALUES
      public float m_fZoomFactor = 1.0f;
      public bool m_bHighRes = false;

  
      public enum eRenderTechnique
      {
         cColor = 0,
         cPatches,
         cIndicies,
         cMagLinear,
         cPaintLayer,
         cJMAP,
         cWorkBuffer,
      }
      public eRenderTechnique mRenderTechnique = eRenderTechnique.cColor;

      public bool m_bShowRecolored = false;

      public int m_iAnchorX = 0;
      public int m_iAnchorY = 0;
      public float m_fInnerRadius = 0.1f;

      

      #endregion

   
      #region LOAD INIT
      ~TextureSynthesisMain()
      {
         mExemplar = null;
         mSynthesiser = null;
      }

      
      public bool LoadExemplar(String fname)
      {

         //load our Analyser
         mExemplar = null;
         mExemplar = new Exemplar(fname);

         //load our synthesiser
         mSynthesiser = null;
         mSynthesiser = new Synthesiser();
         mSynthesiser.loadPrograms(mExemplar, Synthesiser.SynthesisFlags._Standard | Synthesiser.SynthesisFlags._Subpass1234);
     
         clearPaintLayer();

         mSynthesiser.setExemplarPeriod(mExemplar,true, 1, 1);

         m_bNeedUpdate = true;
         m_bNeedUpdateJacobianMap = true;
         for (int i = 0; i < mSynthNodes.Count; i++)
            mSynthNodes[i].m_Query = null;

         postLoadCreateControlPackets();

         return true;
      }
      public void destroy()
      {
         for (int i = 0; i < mSynthNodes.Count; i++)
            mSynthNodes[i].destroy();
         mSynthNodes.Clear();

         m_Quad = null;
         m_QuadDual = null;
         
         if (mJacobianMapSrf != null) mJacobianMapSrf.Dispose();
         if (mJacobianMap != null) mJacobianMap.Dispose();
         if (mJacobianMapInverseSrf != null) mJacobianMapInverseSrf.Dispose();
         if (mJacobianMapInverse != null) mJacobianMapInverse.Dispose();
         if (m_d3dPaintLayerSrf != null) m_d3dPaintLayerSrf.Dispose();
         if (m_d3dPaintLayer != null) m_d3dPaintLayer.Dispose();
         
         
      }
      public void reinit(int newWidth, int newHeight, int scaleFactor)
      {
       //  destroy();
        // init(newWidth, newHeight, scaleFactor);
         mSynthesiser.mIndexLevelToGrab = scaleFactor >> 1;
         m_bNeedUpdate = true;
         m_bNeedUpdateJacobianMap = true;
      }
      public void init(int width, int height, int scaleFactor)
      {
         mScalingFactor = scaleFactor;

         Globals.setValue("4D", "true",true);

         m_iSynthWidth = width;
         m_iSynthHeight =  height;

         m_Quad = new D3DQuad(true);
         m_QuadDual = new D3DQuad(false);

         allocateBuffers(m_iSynthWidth * mScalingFactor, m_iSynthHeight * mScalingFactor);


         
         loadPrograms();
         clearJacobianMap();


         createControlPackets();

         // load splat texture for painting
         m_d3dSplat = BRenderDevice.getTextureManager().getTexture(AppDomain.CurrentDomain.BaseDirectory + "textures\\splatBrushMask.png");

  

         // set miscellaneous render states
         BRenderDevice.getDevice().SetRenderState(RenderStates.DitherEnable,false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SpecularEnable,false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

         // set the world matrix
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().Transform.View = Matrix.Identity;
         BRenderDevice.getDevice().Transform.Projection = Matrix.OrthoOffCenterRH(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

         
      }
      public void loadPrograms()
      {

           Macro []defines = new Macro[16];
           int       nbdef;

           nbdef=0;
           Synthesiser.addDefaultDefines(ref defines,ref nbdef);
           Synthesiser.endDefine(ref defines, ref nbdef);

           m_fxJacobian = ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\jacobian.fx", null);
           m_fxJacobianClearTech = m_fxJacobian.GetTechnique( "t_identity");
           m_fxJacobianSelectTech = m_fxJacobian.GetTechnique("t_selected");
           m_fxGeomPaintJMapTech = m_fxJacobian.GetTechnique("t_paint_jmap");

           m_fxJacobianViewport = m_fxJacobian.GetParameter(null, "Viewport");
           m_fxJacobianGlobalScale = m_fxJacobian.GetParameter(null, "GlobalScale");
           m_fxJacobianSelectedJMap = m_fxJacobian.GetParameter(null, "SelectedJMap");

           m_fxGeomGlobalScale = m_fxJacobian.GetParameter(null, "GlobalScale");
           m_fxGeomPaintedJMap = m_fxJacobian.GetParameter(null, "PaintedJMap");
           m_fxGeomExemplarMainDirection = m_fxJacobian.GetParameter(null, "ExemplarMainDirection");

      }
      public void allocateBuffers(int w, int h)
      {
        // if (m_iBuffersWidth != w || m_iBuffersHeight != h)
         {
            m_iBuffersWidth = w;
            m_iBuffersHeight = h;

            // . create local frames texture
            if (mJacobianMapSrf != null) mJacobianMapSrf.Dispose();
            if (mJacobianMap != null) mJacobianMap.Dispose();
            mJacobianMap = new Texture(BRenderDevice.getDevice(), w, h, 1, Usage.RenderTarget, Format.A16B16G16R16F, Pool.Default);
            mJacobianMapSrf = mJacobianMap.GetSurfaceLevel(0);

            if (mJacobianMapInverseSrf != null) mJacobianMapInverseSrf.Dispose();
            if (mJacobianMapInverse != null) mJacobianMapInverse.Dispose();
            mJacobianMapInverse = new Texture(BRenderDevice.getDevice(), w, h, 1, Usage.RenderTarget, Format.A16B16G16R16F, Pool.Default);

            mJacobianMapInverseSrf = mJacobianMapInverse.GetSurfaceLevel(0);

            //for use w/ jacobian ops
            m_DepthStencilSurface = BRenderDevice.getDevice().CreateDepthStencilSurface(m_iBuffersWidth , m_iBuffersHeight , DepthFormat.D24S8, MultiSampleType.None, 0, true);

            updateJacobianMap();

            // create paint layer
            if (m_d3dPaintLayerSrf != null) m_d3dPaintLayerSrf.Dispose();
            if (m_d3dPaintLayer != null) m_d3dPaintLayer.Dispose();
            //    m_d3dExtrapolatedVectorFieldSrf.Dispose();
            //    m_d3dExtrapolatedVectorField.Dispose();
            //   m_VectorFieldPyramid.Dispose();
            m_d3dPaintLayer = new Texture(BRenderDevice.getDevice(), w, h, 1, Usage.RenderTarget, Format.A8R8G8B8, Pool.Default);
            m_d3dPaintLayerSrf = m_d3dPaintLayer.GetSurfaceLevel(0);
            clearPaintLayer();

            m_bNeedUpdate = true;
            m_bNeedUpdateJacobianMap = true;
            
         }
      }

      public int numPaketsToGen =1;
      public void createControlPackets()
      {
         for (int i = 0; i < mSynthNodes.Count; i++)
            mSynthNodes[i].destroy();
         mSynthNodes.Clear();


         int[] xzOffs = new int[]{0,0,  
                                 m_iSynthWidth, 0,
                                 0,m_iSynthHeight,
                                 m_iSynthWidth,m_iSynthHeight,     //2x2box
                                 m_iSynthWidth*2, 0,
                                 m_iSynthWidth*2, m_iSynthHeight,
                                 m_iSynthWidth*2, m_iSynthHeight*2, //left column
                                 0, m_iSynthHeight*2,
                                 m_iSynthWidth, m_iSynthHeight*2 //bottomrow
         };

         for (int i = 0; i < numPaketsToGen; i++)
         {
            mSynthNodes.Add(new SynthQuadNode());
            mSynthNodes[i].mSynthControlPacket.setWindowValues(xzOffs[i * 2], xzOffs[i*2+1], m_iSynthWidth, m_iSynthHeight);
            mSynthNodes[i].mSynthResultPacket = new SynthResultPacket(m_iBuffersWidth, m_iBuffersHeight);
            mSynthNodes[i].mSynthControlPacket.mConstraintMap = m_d3dPaintLayer;
            mSynthNodes[i].m_Query = null;
         }
 
      }
      public void postLoadCreateControlPackets()
      {
         for(int i=0;i<mSynthNodes.Count;i++)
            mSynthNodes[i].mSynthControlPacket.m_Windows.computeD3DPixPackTextures(mSynthesiser);
      }
      #endregion

      #region PAINT

      public void paintAt(int mx, int my)
      {
         // save render target
         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         float dx=(float)(mx - m_iAnchorX);
         float dy=(float)(my - m_iAnchorY);
         float l=(float) Math.Sqrt(dx*dx+dy*dy);
         if (l < 0.01) return;
         dx/=l;      dy/=l;
         dx=dx*127.0f+128.0f; dy=dy*127.0f+128.0f;
         int r=0,g=0,b=0,a=0;

         if (mJMapPattern == eJMapPattern.cPaint)
         {
            if(mPaintOrientation && mPaintScale)
            {
               r = (int)(dx);
               g = (int)(dy);
               b = (int)(Math.Max(0.0f, Math.Min(255.0f, 255.0f * (1.0f - m_fInnerRadius))));
            }
            else if(mPaintOrientation)
            {
               r = (int)(dx);
               g = (int)(dy);
               b = 0;
            }
            else if(mPaintScale)
            {
               r = 0x80;
               g = 0xFF;
               b = (int)(Math.Max(0.0f, Math.Min(255.0f, 255.0f * (1.0f - m_fInnerRadius))));
            }
         }

         a = 0xff;

         Matrix mat, mattrl, matscl;
         float tx = mx / (float)(m_iSynthWidth);   // (m_dwCreationWidth);
         float ty = my / (float)(m_iSynthHeight);  // (m_dwCreationHeight);


         mattrl = Matrix.Translation(tx - m_fInnerRadius / 2.0f,
                                    ty - m_fInnerRadius / 2.0f,
                                    0.0f);
         matscl = Matrix.Scaling(m_fInnerRadius,
                                 m_fInnerRadius,
                                 0.0f);
         mat = Matrix.Multiply(matscl, mattrl);




         BRenderDevice.getDevice().SetTransform(TransformType.View, mat);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.BlendOperation, (int)BlendOperation.Add);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetTexture(0, m_d3dSplat.mTexture);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (a << 24) + (r << 16) + (g << 8) + b);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TFactor);

         // set new render target
         BRenderDevice.getDevice().SetRenderTarget(0,m_d3dPaintLayerSrf);

         // render
         BRenderDevice.getDevice().BeginScene();
         m_Quad.render();
         BRenderDevice.getDevice().EndScene();

         // restore state
         BRenderDevice.getDevice().SetTransform(TransformType.View, Matrix.Identity);

         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.BlendOperation, (int)BlendOperation.Add);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, 0xFFFFFFFF);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.Current);


         // restore render target
         locktarget.destroy();
         locktarget = null;
      }

      public void clearPaintLayer()
      {
        // save render target
        D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

        // set new render target
        BRenderDevice.getDevice().SetRenderTarget(0, m_d3dPaintLayerSrf);
        savedDepthSten = BRenderDevice.getDevice().DepthStencilSurface;
        BRenderDevice.getDevice().DepthStencilSurface = m_DepthStencilSurface;

        // render
        BRenderDevice.getDevice().BeginScene();
          if (mJMapPattern == eJMapPattern.cPaint)
             BRenderDevice.getDevice().Clear( ClearFlags.Target,unchecked((int)0x0080FF00),1.0f,0);
          else
            BRenderDevice.getDevice().Clear( ClearFlags.Target,unchecked((int)0x80808080),1.0f,0);
         BRenderDevice.getDevice().EndScene();

         BRenderDevice.getDevice().SetTexture(0, null);

        // CLM uncomment this for advection
         //if (mJMapPattern == eJMapPattern.cVfieldPaint) 
         // {
            //if (m_VectorFieldPyramid == null) 
            //{
            //   m_VectorFieldPyramid = new D3DGenMipMapPyramid(AppDomain.CurrentDomain.BaseDirectory + "shaders\\vfield.pyramid.fx", m_d3dPaintLayer);
            //}
            //if (m_VectorFieldPyramid.compatible(m_d3dPaintLayer)) 
            //{
            //  m_VectorFieldPyramid.bind(m_d3dPaintLayer);
            //}
            //else 
            //{
            //  m_VectorFieldPyramid.destroy();
            //  m_VectorFieldPyramid = null;
            //  m_VectorFieldPyramid = new D3DGenMipMapPyramid(AppDomain.CurrentDomain.BaseDirectory + "shaders\\vfield.pyramid.fx", m_d3dPaintLayer);
            //}
          //}

         if (mJMapPattern == eJMapPattern.cPaint)
            m_bNeedUpdateJacobianMap=true;

         BRenderDevice.getDevice().DepthStencilSurface = savedDepthSten;
         savedDepthSten.Dispose();
         savedDepthSten = null;


         // restore render target
         locktarget.destroy();
         locktarget = null;
     }
      #endregion

      #region JACOBIAN
      public void updateJacobianMap()
      {

        
          if (m_fxJacobian == null) 
               return;
             

             // clear
             

             // render
             bool isometric=false;
             if (mJMapPattern == eJMapPattern.cPaint) 
             {
               renderJacobianMapFromPaint();
             }
             else if ((int)mJMapPattern >= (int)eJMapPattern.cPaint) 
             {
               renderSelectedJacobianMap();    
             }
             else //if (Globals.gSynthMethod != Globals.eSynthMethod.cSynth_Ansiometric || mExemplar == null)// || m_Mesh == NULL || m_Analyser == NULL) 
             {
               clearJacobianMap();
               isometric=true;
             }
             
         
             if (isometric && mSynthesiser != null) 
             {
                if (Globals.gSynthMethod == Globals.eSynthMethod.cSynth_Ansiometric) 
               {
                  Globals.gSynthMethod = Globals.eSynthMethod.cSynth_Isometric;
                  mSynthesiser.reload(mExemplar);
               }
             }
             else 
             {
               if (Globals.gSynthMethod == Globals.eSynthMethod.cSynth_Isometric) 
               {
                  Globals.gSynthMethod = Globals.eSynthMethod.cSynth_Ansiometric;
                  mSynthesiser.reload(mExemplar);
               }
             }
      }
      unsafe public void clearJacobianMap()
      {

         if (m_fxJacobian == null)// || mSynthesiser == null)
            return;


         Vector4 tr = new Vector4(0, 0, (float)m_iBuffersWidth, (float)m_iBuffersHeight);


         // save render target
         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         // set new render target
         BRenderDevice.getDevice().SetRenderTarget(0, mJacobianMapSrf);
         BRenderDevice.getDevice().SetRenderTarget(1, mJacobianMapInverseSrf);

         savedDepthSten = BRenderDevice.getDevice().DepthStencilSurface;
         BRenderDevice.getDevice().DepthStencilSurface = m_DepthStencilSurface;


         // render
         BRenderDevice.getDevice().BeginScene();
         {
            BRenderDevice.getDevice().Clear(ClearFlags.Target, Color.Black, 1, 0);


            m_fxJacobian.Technique = m_fxJacobianClearTech;
            m_fxJacobian.SetValue(m_fxJacobianGlobalScale, mExemplar==null?0:mExemplar.mSynthParams.TextureScale);
            m_fxJacobian.SetValue(m_fxJacobianSelectedJMap, (int)mJMapPattern);
            m_fxJacobian.SetValue(m_fxJacobianViewport, tr);

            m_fxJacobian.CommitChanges();
            m_fxJacobian.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
            m_fxJacobian.BeginPass(0);
            m_QuadDual.render();
            m_fxJacobian.EndPass();
            m_fxJacobian.End();

            BRenderDevice.getDevice().EndScene();
         }

         m_bNeedUpdate = true;

         BRenderDevice.getDevice().DepthStencilSurface = savedDepthSten;
         savedDepthSten.Dispose();
         savedDepthSten = null;

         BRenderDevice.getDevice().SetRenderTarget(1, null);
         BRenderDevice.getDevice().SetTexture(0, null);
         locktarget.destroy();
         locktarget = null;

         //   BRenderDevice.writeTextureToFile(mJacobianMap, AppDomain.CurrentDomain.BaseDirectory + "screens\\localFrames.bmp");
         //   BRenderDevice.writeTextureToFile(mJacobianMapInverse, AppDomain.CurrentDomain.BaseDirectory + "screens\\localFramesINV.bmp");

      }
      void renderJacobianMapFromPaint()
      {

         if (m_fxJacobian == null)// || mSynthesiser == null)
            return;

        // save render target
        D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

        // set new render target
        BRenderDevice.getDevice().SetRenderTarget(0, mJacobianMapSrf);
        BRenderDevice.getDevice().SetRenderTarget(1, mJacobianMapInverseSrf);

        savedDepthSten = BRenderDevice.getDevice().DepthStencilSurface;
        BRenderDevice.getDevice().DepthStencilSurface = m_DepthStencilSurface;


        // render
        BRenderDevice.getDevice().BeginScene();

        BRenderDevice.getDevice().Clear(ClearFlags.Target, Color.Black, 1, 0);


          m_fxJacobian.Technique = m_fxGeomPaintJMapTech;
          m_fxJacobian.SetValue(m_fxGeomGlobalScale, mExemplar.mSynthParams.TextureScale);
          m_fxJacobian.SetValue(m_fxGeomPaintedJMap,m_d3dPaintLayer);
          m_fxJacobian.SetValue(m_fxGeomExemplarMainDirection,m_fExemplarMainDirection);

          m_fxJacobian.CommitChanges();
          m_fxJacobian.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
          m_fxJacobian.BeginPass(0);
          m_QuadDual.render();
          m_fxJacobian.EndPass();
          m_fxJacobian.End();

          BRenderDevice.getDevice().EndScene();


        m_bNeedUpdate=true;

        BRenderDevice.getDevice().DepthStencilSurface = savedDepthSten;
        savedDepthSten.Dispose();
        savedDepthSten = null;

        BRenderDevice.getDevice().SetRenderTarget(1,null);
        BRenderDevice.getDevice().SetTexture(0,null);

        locktarget.destroy();
        locktarget = null;
      }
      void renderSelectedJacobianMap()
      {
       
         if (m_fxJacobian == null)// || mSynthesiser == null)
            return;


         Vector4 tr = new Vector4(0, 0, (float)m_iBuffersWidth, (float)m_iBuffersHeight);


         // save render target
         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         // set new render target
         BRenderDevice.getDevice().SetRenderTarget(0, mJacobianMapSrf);
         BRenderDevice.getDevice().SetRenderTarget(1, mJacobianMapInverseSrf);

         savedDepthSten = BRenderDevice.getDevice().DepthStencilSurface;
         BRenderDevice.getDevice().DepthStencilSurface = m_DepthStencilSurface;


         // render
         BRenderDevice.getDevice().BeginScene();
         {
            BRenderDevice.getDevice().Clear(ClearFlags.Target, Color.Black, 1, 0);


            m_fxJacobian.Technique = m_fxJacobianSelectTech;
            m_fxJacobian.SetValue(m_fxJacobianGlobalScale, mExemplar.mSynthParams.TextureScale);
            m_fxJacobian.SetValue(m_fxJacobianSelectedJMap, (int)(mJMapPattern-1));
            m_fxJacobian.SetValue(m_fxJacobianViewport, tr);

            m_fxJacobian.CommitChanges();
            m_fxJacobian.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
            m_fxJacobian.BeginPass(0);
            m_QuadDual.render();
            m_fxJacobian.EndPass();
            m_fxJacobian.End();

            BRenderDevice.getDevice().EndScene();
         }

         m_bNeedUpdate = true;

         BRenderDevice.getDevice().DepthStencilSurface = savedDepthSten;
         savedDepthSten.Dispose();
        savedDepthSten = null;

         BRenderDevice.getDevice().SetRenderTarget(1, null);
         BRenderDevice.getDevice().SetTexture(0, null);
         locktarget.destroy();
         locktarget = null;

      }
      #endregion

      public void refreshBuffer()
      {
         if (mExemplar == null || (m_bNeedUpdate == false && m_bNeedUpdateJacobianMap == false))
            return;

         Matrix projMat = BRenderDevice.getDevice().Transform.Projection;
       
         if (m_bNeedUpdateJacobianMap)
         {
            updateJacobianMap();
            
         }
         int num_upd_lvls = 0;
         for(int i=0;i<mSynthNodes.Count;i++)
         {
            if (m_bNeedUpdateJacobianMap)
               mSynthNodes[i].mSynthControlPacket.updateJacobianPyramid(mJacobianMap, mJacobianMapInverse);

            // . parameters
            SynthesizeParameters parameters = new SynthesizeParameters(mExemplar, mSynthNodes[i].mSynthControlPacket, mSynthNodes[i].mSynthResultPacket, m_SynthFlags);

            // . call synthesizer
            
            if (mSynthNodes[i].m_Query == null)
            {
               mSynthNodes[i].m_Query = mSynthesiser.synthesize(parameters);
               num_upd_lvls = Globals.cNumLevels;// mAnalyser.nbLevels();
            }
            else
            {
               num_upd_lvls = mSynthesiser.synthesize(mSynthNodes[i].m_Query, parameters, m_bUpdateAllForce || m_bNeedUpdate || m_bNeedUpdateJacobianMap);

            }
         }

   
          m_iSynthLevelStart=num_upd_lvls;

          BRenderDevice.getDevice().Transform.Projection = projMat;

          // for debug
         // m_d3dRawDataBuffer = mSynthNodes[i].m_Query.m_WorkBuffer.front().texture();  // show work buffer
        
         
          m_bNeedUpdate=false;
          m_bNeedRefresh = true;
          m_bNeedUpdateJacobianMap = false;
          

      }

      #region RENDER
      public void renderBuffer()
      {
         if (mExemplar == null)
            return;


         switch (mRenderTechnique)
         {
            case eRenderTechnique.cIndicies:
               BRenderDevice.getDevice().VertexShader = null;
               BRenderDevice.getDevice().PixelShader = null;
               for (int i = 0; i < mSynthNodes.Count; i++)
               {
                  //translate
                  Vector3 v = new Vector3(mSynthNodes[i].mSynthControlPacket.WindowMinX / 512.0f, mSynthNodes[i].mSynthControlPacket.WindowMinY / 512.0f, 0);
                  BRenderDevice.getDevice().Transform.World = Matrix.Translation(v);

                  BRenderDevice.getDevice().SetTexture(0, mSynthNodes[i].mSynthResultPacket.mSynthResultIndexTexture);
                  m_Quad.render();
               }
               
               break;
            case eRenderTechnique.cWorkBuffer:
               if (m_d3dRawDataBuffer == null)
                  return;

               BRenderDevice.getDevice().VertexShader = null;
               BRenderDevice.getDevice().PixelShader = null;
               BRenderDevice.getDevice().SetTexture(0, m_d3dRawDataBuffer);
               m_Quad.render();
               BRenderDevice.getDevice().SetTexture(0, null);

               break;
            case eRenderTechnique.cJMAP:
               {
                 
                     BRenderDevice.getDevice().VertexShader = null;
                     BRenderDevice.getDevice().PixelShader = null;
                     BRenderDevice.getDevice().Transform.World = Matrix.Translation(Vector3.Empty);
                     BRenderDevice.getDevice().SetTexture(0, mJacobianMap);//Globals.g_iLevelShift));
                     m_Quad.render();
                     BRenderDevice.getDevice().SetTexture(0, null);
               }
               break;

            case eRenderTechnique.cPaintLayer:

               BRenderDevice.getDevice().VertexShader = null;
               BRenderDevice.getDevice().PixelShader = null;
               BRenderDevice.getDevice().Transform.World = Matrix.Translation(Vector3.Empty);
               BRenderDevice.getDevice().SetTexture(0, m_d3dPaintLayer);
               m_Quad.render();

               break;

            default:

               //if we need to refresh, redraw our texture to the buffer
               for (int i = 0; i < mSynthNodes.Count; i++)
               {
                  if (m_bNeedRefresh)
                     mSynthesiser.renderResultToTexture(m_DepthStencilSurface, mSynthNodes[i].mSynthResultPacket, mSynthNodes[i].m_Query, m_iBuffersWidth, m_iBuffersHeight, m_fZoomFactor, m_bHighRes);


                  // display current synthesized texture
                  BRenderDevice.getDevice().VertexShader = null;
                  BRenderDevice.getDevice().PixelShader = null;

                  //translate
                  Vector3 v = new Vector3(mSynthNodes[i].mSynthControlPacket.WindowMinX / 512.0f, mSynthNodes[i].mSynthControlPacket.WindowMinY / 512.0f,0);
                  BRenderDevice.getDevice().Transform.World = Matrix.Translation(v);

                  //  BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MagFilter, (int)Filter.Linear);
                  //  BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.MinFilter, (int)Filter.Linear);
                  BRenderDevice.getDevice().SetTexture(0, mSynthNodes[i].mSynthResultPacket.mResultTex);
                  m_Quad.render();
               }
               m_bNeedRefresh = false;
               
               break;
         }
         
       
      }
     
      #endregion

   }
}