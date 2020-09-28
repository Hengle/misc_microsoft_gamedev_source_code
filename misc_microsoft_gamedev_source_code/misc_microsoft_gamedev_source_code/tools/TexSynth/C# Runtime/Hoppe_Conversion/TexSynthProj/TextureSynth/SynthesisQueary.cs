/*===================================
  SynthesisQueary.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis  (MGS Ensemble studios) [12.01.06]
===================================*/
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Rendering;
using System.IO;
using EditorCore;

namespace TextureSynthesis
{
   //CLM this class owned by each quadnode
   //this defines information from the worldspace existance that 
   //controls how our synth occurs 
   //ie holds coords for where to synth on the imaginary plane, as well as per-chunk modifier data)
   class SynthControlPacket
   {
      public SynthControlPacket()
      {
      }

      ~SynthControlPacket()
      {
         mJacobianMapPyramid = null;
         mJacobianMapInversePyramid = null;
      }

      //window control
      public void setWindowValues(int x, int y, int width, int height)
      {
         mWindowMinX = x;
         mWindowMinY = y;
         mWindowWidth = width;
         mWindowHeight = height;
         updateSynthWindow();
      }

       float mWindowMinX = 0;
      public float WindowMinX
      {
         get
         {
            return mWindowMinX;
         }
         set
         {
           // mWindowMinX = value;
//updateSynthWindow();
         }
      }
       float mWindowMinY = 0;
      public float WindowMinY
      {
         get
         {
            return mWindowMinY;
         }
         set
         {
          //  mWindowMinY = value;
          //  updateSynthWindow();
         }
      }
       int mWindowWidth = 0;
      public int WindowWidth
      {
         get
         {
            return mWindowWidth;
         }
         set
         {
            //mWindowWidth = value;
           // updateSynthWindow();
         }
      }
       int mWindowHeight = 0;
      public int WindowHeight
      {
         get
         {
            return mWindowHeight;
         }
         set
         {
         //   mWindowHeight = value;
          //  updateSynthWindow();
         }
      }

      //constraint map (over the synthesized entire world texture)
      public Texture mConstraintMap;

      //synth windows 
      public SynthWindows m_Windows;

      public void updateSynthWindow()
      {
         m_Windows = new SynthWindows(Window.LTWH((int)mWindowMinX, (int)mWindowMinY, mWindowWidth, mWindowHeight), Globals.cNumLevels,
             Globals.SYNTH_SUB1234_BORDER_L, Globals.SYNTH_SUB1234_BORDER_R, Globals.SYNTH_SUB1234_BORDER_T, Globals.SYNTH_SUB1234_BORDER_B, Globals.SYNTH_PREFETCH_BORDER);
      }

      //jacobian pyramid stuff
      D3DGenMipMapPyramid mJacobianMapPyramid = null;
      D3DGenMipMapPyramid mJacobianMapInversePyramid = null;
      public D3DGenMipMapPyramid getJacobianPyramid()
      {
         return mJacobianMapPyramid;
      }
      public D3DGenMipMapPyramid getJacobianPyramidINV()
      {
         return mJacobianMapInversePyramid;
      }
      public void updateJacobianPyramid(Texture jacobMap, Texture inv_jacobMap)
      {
         if (mJacobianMapPyramid == null || mJacobianMapInversePyramid == null)
         {
            mJacobianMapPyramid = new D3DGenMipMapPyramid(AppDomain.CurrentDomain.BaseDirectory + "shaders\\query.jmap.pyramid.fx", jacobMap);
            mJacobianMapInversePyramid = new D3DGenMipMapPyramid(AppDomain.CurrentDomain.BaseDirectory + "shaders\\query.jmap.pyramid.fx", inv_jacobMap);
         }
         else
         {
            mJacobianMapPyramid.bind(jacobMap);
            mJacobianMapInversePyramid.bind(inv_jacobMap);
         }
      }
   }

   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //CLM this class now enscapulates the params need for a synthesis process.
   //the exemplar is the current texture to synthesise (and it has it's own params for that)
   //the control packet is the world based information on where in the imaginary plane we'd like to synth at.
   //flags.. are .. uh... flags....
   //NOTE we could just get rid of this class, but it's a nice wrapper for multiple data inputs...
   class SynthesizeParameters
   {
      public SynthesizeParameters() 
      {
         mExemplar = null;
         mControlPacket = null;
         mResultPacket = null;
         m_Flags=(Synthesiser.SynthesisFlags._Standard);
      }
      public SynthesizeParameters(Exemplar exemplar, SynthControlPacket ControlPacket, SynthResultPacket ResultPacket, Synthesiser.SynthesisFlags flags) 
      {
            mExemplar = exemplar;
            m_Flags=(flags);
            mControlPacket = ControlPacket;
            mResultPacket = ResultPacket;

            Globals.Assert((m_Flags & Synthesiser.SynthesisFlags._Standard) != 0 || 
                           (m_Flags & Synthesiser.SynthesisFlags._Spherical) != 0 || 
                           (m_Flags & Synthesiser.SynthesisFlags._Octahedron) != 0 || 
                           (m_Flags & Synthesiser.SynthesisFlags._Atlas) != 0);
       }
      ~SynthesizeParameters()
      {
         mExemplar = null;       
         mControlPacket = null;
         mResultPacket = null ;
      }

      public bool equalTo(SynthesizeParameters parms)
      {
         bool same = true;
         same &= getConstraintThreshold() == parms.getConstraintThreshold();
         same &= getCoherenceThreshold() == parms.getCoherenceThreshold();
         same &= getCutAvoidance() == parms.getCutAvoidance();
         same &= getFlags() == parms.getFlags();

         return same;
      }

      public int getRandomness(int l)
      {
         if (l < mExemplar.mSynthParams.mPerLevelJitterRandomness.Length)
            return (mExemplar.mSynthParams.mPerLevelJitterRandomness[l]);

         return (0);
      }

      public int getConstraintThreshold() { return (mExemplar.mSynthParams.mConstraintThres); }
      public int constraintThreshold() { return (mExemplar.mSynthParams.mConstraintThres); }

      public int getCoherenceThreshold() { return (mExemplar.mSynthParams.mCoherenceThres); }
      public int coherenceThreshold() { return (mExemplar.mSynthParams.mCoherenceThres); }

      public float getCutAvoidance() { return (mExemplar.mSynthParams.mCutAvoidance); }
      public float cutAvoidance() { return (mExemplar.mSynthParams.mCutAvoidance); }

      public Synthesiser.SynthesisFlags getFlags() { return (m_Flags); }
      public Synthesiser.SynthesisFlags flags() { return (m_Flags); }

      public Texture ConstraintMap() { return (mControlPacket.mConstraintMap); }

      public Texture exemplarRndMap()  {return (mExemplar.getRandomTexture());}



      public Exemplar mExemplar = null;
      public SynthControlPacket mControlPacket = null; //given to us by an owning quadnode
      public SynthResultPacket mResultPacket = null;
      public Synthesiser.SynthesisFlags m_Flags;


   };

   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   class SynthResultPacket
   {
      public SynthResultPacket(int w, int h)
      {
         createTextures(w, h);
      }

      ~SynthResultPacket()
      {
         destroy();
      }

      public void destroy()
      {
         if (mResultTex != null)
         {
            mResultTex.Dispose();
            mResultTex = null;
         }
         if (mResultTexSurface != null)
         {
            mResultTexSurface.Dispose();
            mResultTexSurface = null;
         }
         if (mSynthResultIndexTexture != null)
         {
            mSynthResultIndexTexture.Dispose();
            mSynthResultIndexTexture = null;
         }
      }
   
      public void createTextures(int w, int h)
      {
         mResultTex = new Texture(BRenderDevice.getDevice(), w, h, 1, Usage.RenderTarget, Format.A8R8G8B8, Pool.Default);
         mResultTexSurface = mResultTex.GetSurfaceLevel(0);
      }
      public Texture mResultTex;
      public Surface mResultTexSurface;
      public Texture mSynthResultIndexTexture = null;    //this is where the index is stored
   }
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //----------------------------------------
   //CLM this class is now the middleground between synthesis.
   //it's still required we send it, as it's the params sled that takes the data through the synthesizer riegns
   //I plan on eventually destroying this class, and sending members of it into the synthesizer perminant.
   class SynthesisQuery
   {
      #region MEMBERS
      public SynthesizeParameters m_Params;
      public int m_iLevel;

      // buffers storing synthesis result (1 per level)
      public List<single_buffer> m_SynthBuffers;

      // double buffer for synthesis
      public swap_buffer m_WorkBuffer = new swap_buffer();

      Synthesiser m_Synthesizer = null;

      // depth-stencil surface
      public Surface m_DepthStencilSurface;





      public int level() 
      {
         return (m_iLevel); 
      }
      public void setLevel(int i)
      {
         m_iLevel = i;
      }
      public Texture getJacobianMap(int l) 
      {
         return (m_Params.mControlPacket.getJacobianPyramid().pyramidLevel(l));
      }
      public Texture getJacobianMapInverse(int l) 
      {
         return (m_Params.mControlPacket.getJacobianPyramidINV().pyramidLevel(l)); 
      }
      public Pair<Vector4, Vector4> requestWindow()
      {
         return new Pair<Vector4, Vector4>(m_Synthesizer.bufferSize(m_SynthBuffers[m_iLevel].size()),m_Synthesizer.windowAsVec4(m_Params.mControlPacket.m_Windows.requested(m_iLevel)));
      }
      public single_buffer getBuffer(int l)
      {
         return (m_SynthBuffers[l]);
      }

      #endregion

      #region INIT LOAD
      public SynthesisQuery(Synthesiser synth,SynthesizeParameters parms)
      {
         m_Synthesizer = (synth);
         m_iLevel = (-1);
         m_Params = (parms);

         //m_Params.mControlPacket.updateSynthWindow();
         
         // allocate buffers
         allocateBuffers();

         // set current level to start synthesis at coarsest
         m_iLevel = Globals.cNumLevels;
         
      }
      ~SynthesisQuery()
      {
         m_Params = null;
         m_SynthBuffers.Clear();
         m_WorkBuffer = null;
         m_Synthesizer = null;
         if(m_DepthStencilSurface!=null)
         {
            m_DepthStencilSurface.Dispose();
            m_DepthStencilSurface = null;
         }

      }

      void loadPrograms()
      {
       
      }
      void allocateBuffers()
      {

         int maxsz = 0;


         // allocate level's buffers
         m_SynthBuffers = new List<single_buffer>(Globals.cNumLevels);//m_Synthesizer.getAnalyser().nbLevels());
         for (int l = 0; l < Globals.cNumLevels/*m_Synthesizer.getAnalyser().nbLevels()*/; l++)
         {
            int sz = Synthesiser.bufferSizeFromWindow(m_Params.mControlPacket.m_Windows.bufferrgn(l));
            maxsz = Math.Max(maxsz, sz);
            m_SynthBuffers.Add(new single_buffer());
            m_SynthBuffers[l].create(sz);
         }


         // allocate double buffer
         m_WorkBuffer.create(maxsz);

         // allocate depth-stencil buffer
         m_DepthStencilSurface = BRenderDevice.getDevice().CreateDepthStencilSurface(maxsz, maxsz, DepthFormat.D24S8, MultiSampleType.None, 0, true);

      }
      
      public void reinit(SynthesizeParameters parms)
      {
         //we're the same packet, don't recreate everything.
         if (!parms.equalTo(m_Params))  
         {

            int wsize = m_Params.mControlPacket.m_Windows.requested(0).width();
            int hsize = m_Params.mControlPacket.m_Windows.requested(0).height();

            updateParams(parms);

            // compute windows
            m_Params.mControlPacket.setWindowValues((int)parms.mControlPacket.WindowMinX, (int)parms.mControlPacket.WindowMinY, wsize, hsize);
         }


         // set current level to start synthesis at coarsest
         m_iLevel = Globals.cNumLevels;// m_Synthesizer.getAnalyser().nbLevels();
         
      }
      public List<int> update(SynthesizeParameters parms)
       {  
         updateParams(parms);


         int wsize = m_Params.mControlPacket.m_Windows.requested(0).width();
         int hsize = m_Params.mControlPacket.m_Windows.requested(0).height();
         List<int> regen = m_Params.mControlPacket.m_Windows.update(Window.LTWH((int)parms.mControlPacket.WindowMinX, (int)parms.mControlPacket.WindowMinY, wsize, hsize));
         return (regen);
      }
      void updateParams(SynthesizeParameters parms)
      {
         // TODO: check parameters compatibility
         if (m_Params.getFlags() != parms.getFlags())
            throw new Exception("SynthesisQuery::update - new parameters are not compatible with previous!");
         m_Params=parms;
      }
    
      #endregion





      };
     
}