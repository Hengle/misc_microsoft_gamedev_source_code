/*===================================
  Synthesiser.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis  (MGS Ensemble studios) [12.01.06]
===================================*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Rendering;
using System.IO;
using EditorCore;

 
namespace TextureSynthesis
{
   class Synthesiser
   {
      // synthesis flags enum
      public enum SynthesisFlags
      {
         _Undef = 0,
         _Subpass1212 = 1,
         _Subpass1234 = 2,
         _Standard = 4,
         _Toroidal = 8,
         _Spherical = 16,
         _Octahedron = 32,
         _FixedDomain = 64,
         _Atlas = 128
      };
      #region INIT LOAD
      public Synthesiser()
      {
         m_iKSearch        = 2;

         m_fPeriodX=1.0f;
         m_fPeriodY=1.0f;
         m_bUsePeriod=false;
         m_SavedDepthStencilSurface=null;


         loadTexturesForSynthesis();

         //CLM this is called w/ an exemplar for now...
         //loadPrograms(null,SynthesisFlags._Standard | SynthesisFlags._Subpass1234);

         // quad for rendering
         m_QuadDual  =new D3DQuad(false);
         m_Quad = new D3DQuad(true);

         loadDefaultParams();
         initSubpassTables();
      }


      void loadDefaultParams()
      {
         // add global paramters
         Globals.addDef("neighavg");
         Globals.addDef("compress_simsets");
      }

      unsafe void loadTexturesForSynthesis()
      {
         // create pixpack offset texture
         m_d3dPixpackOffsets = new Texture(BRenderDevice.getDevice(), 5, 5, 1, 0, Format.G16R16F, Pool.Managed);    
      }      
      

      public void reload(Exemplar e) 
      {
         loadPrograms(e,m_CompiledFlags); 
      }
      public void loadPrograms(Exemplar exemplar,SynthesisFlags flags)//=_Undef)
      {
         ScopeTimer tm = new ScopeTimer("[D3DSynthesizer::loadPrograms]");

         Macro    []defines = new Macro[128];
         int      nbdef=0;

         if (flags != SynthesisFlags._Undef)
            m_CompiledFlags = flags;

         #region DISPLAY SHADER

         m_fxDisplay = ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\magnification.fx", defines);


         m_fxDispTechSimple = m_fxDisplay.GetTechnique("t_display");
         m_fxDispTechColor = m_fxDisplay.GetTechnique("t_display_color");
         m_fxDispTechMagnify = m_fxDisplay.GetTechnique("t_display_magnification");
         m_fxDispTechPatches = m_fxDisplay.GetTechnique("t_display_patches");
         m_fxDispTechIndices = m_fxDisplay.GetTechnique("t_display_indices");
         m_fxDispTechColorNoVP = m_fxDisplay.GetTechnique("t_display_color_novp");
         m_fxDispTechPatchesNoVP = m_fxDisplay.GetTechnique("t_display_patches_novp");
         m_fxDispTechIndicesNoVP = m_fxDisplay.GetTechnique("t_display_indices_novp");

         m_fxDispIndexMap = m_fxDisplay.GetParameter(null, "IndexMap");
         m_fxDispExemplar = m_fxDisplay.GetParameter(null, "Exemplar");
         m_fxDispTrl = m_fxDisplay.GetParameter(null, "DispTrl");
         m_fxDispScl = m_fxDisplay.GetParameter(null, "DispScl");
         m_fxDispRes = m_fxDisplay.GetParameter(null, "DispRes");
         m_fxDispIndexMapRes = m_fxDisplay.GetParameter(null, "IndexMapRes");
         m_fxDispExemplarRes = m_fxDisplay.GetParameter(null, "ExemplarRes");
         m_fxDispExemplarHighRes = m_fxDisplay.GetParameter(null, "ExemplarHighRes");
         m_fxDispInvZoomFactor = m_fxDisplay.GetParameter(null, "invZoomFactor");
         m_fxDispNumExHighresLevels = m_fxDisplay.GetParameter(null, "NumExHighresLevels");
         m_fxDispLocalFrames = m_fxDisplay.GetParameter(null, "LocalFrames");


         m_fxDisplay.SetValue(m_fxDispInvZoomFactor, 1.0f);

         m_fxDisplay.CommitChanges();

         #endregion
           
         #region INIT SHADER

         nbdef=0;
         if (m_bUsePeriod && (m_fPeriodX > 1.0f || m_fPeriodY > 1.0f))
         {
            addDefine("PERIODIC_OFFSET","1",ref defines,ref nbdef);
            addDefine("PERIOD", "float2(" + m_fPeriodX + "," + m_fPeriodY + ")", ref defines, ref nbdef);
         }
         else if (exemplar.isToroidal())//(m_CompiledFlags & SynthesisFlags._Toroidal) != 0)
         {
            
         }

         addDefaultDefines(ref defines,ref nbdef);
         endDefine(ref defines,ref nbdef);

         m_fxInit = Rendering.ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\init.fx", defines);


         m_fxInitTech = m_fxInit.GetTechnique("t_init_std");
         m_fxInitTechForce = m_fxInit.GetTechnique("t_init_force");

         m_fxInit.Technique = m_fxInitTech;


         m_fxInitApprExemplar        =m_fxInit.GetParameter(null, "Exemplar");  
         m_fxInitExemplarSize        =m_fxInit.GetParameter(null, "ExemplarSize");
         m_fxInitScale               =m_fxInit.GetParameter(null, "Scale");
         m_fxInitWindowPos           =m_fxInit.GetParameter(null, "WindowPos");
         m_fxInitLevelSize           =m_fxInit.GetParameter(null, "LevelSize");
         m_fxInitPermutationTable    =m_fxInit.GetParameter(null, "PermutationTable");
         m_fxInitLocalFramesRes      =m_fxInit.GetParameter(null, "LocalFramesRes");
         m_fxInitLocalFrames         =m_fxInit.GetParameter(null, "LocalFrames");

         
         #endregion

         #region UPSAMPLE SHADER
         // Upsampling shader

         nbdef=0;
         if (m_bUsePeriod && (m_fPeriodX > 1.0f || m_fPeriodY > 1.0f)) 
         {
            addDefine("PERIODIC_OFFSET","1",ref defines,ref nbdef);
            addDefine("PERIOD", "float2(" + m_fPeriodX + "," + m_fPeriodY + ")", ref defines, ref nbdef);
         }
         else if (exemplar.isToroidal())//(m_CompiledFlags & SynthesisFlags._Toroidal) != 0)
         {
            addDefine("TOROIDAL","1",ref defines,ref nbdef);
         }

         if (!exemplar.isToroidal()) 
         {
            addDefine("NON_TOROIDAL_EXEMPLAR","1",ref defines,ref nbdef);
         }
         if (Globals.gSynthMethod == Globals.eSynthMethod.cSynth_Ansiometric) 
         {
            addDefine("ANISOSYNTH","1",ref defines,ref nbdef);
         }

         addDefaultDefines(ref defines,ref nbdef);
         endDefine(ref defines,ref nbdef);

         m_fxUpsmpl = Rendering.ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory+"shaders\\upsampling.fx", defines);

         m_fxUpsmplTech = m_fxUpsmpl.GetTechnique("upsampling");
         m_fxUpsmplJitterTech = m_fxUpsmpl.GetTechnique("upsampling_jitter");
         


         m_fxUpsmplExemplarCstr        =m_fxUpsmpl.GetParameter(null,"ExemplarCstr");
         m_fxUpsmplCstrMap             =m_fxUpsmpl.GetParameter(null,"CstrMap");
         m_fxUpsmplPrevious            =m_fxUpsmpl.GetParameter(null,"Previous");
         m_fxUpsmplExemplarSize        =m_fxUpsmpl.GetParameter(null,"ExemplarSize");
         m_fxUpsmplRandomness          =m_fxUpsmpl.GetParameter(null,"Randomness");
         m_fxUpsmplCstrThreshold       =m_fxUpsmpl.GetParameter(null,"CstrThreshold");
         m_fxUpsmplCstrMapScale        =m_fxUpsmpl.GetParameter(null,"CstrMapScale");
         m_fxUpsmplScale               =m_fxUpsmpl.GetParameter(null,"Scale");
         m_fxUpsmplTime                =m_fxUpsmpl.GetParameter(null,"Time");
         m_fxUpsmplNeighOffset         =m_fxUpsmpl.GetParameter(null,"NeighOffset");
         m_fxUpsmplWindowPos           =m_fxUpsmpl.GetParameter(null,"WindowPos");
         m_fxUpsmplParentWindowPos     =m_fxUpsmpl.GetParameter(null,"ParentWindowPos");
         m_fxUpsmplParentQuadSize      =m_fxUpsmpl.GetParameter(null,"ParentQuadSize");
         m_fxUpsmplPositionMap0        =m_fxUpsmpl.GetParameter(null,"PositionMap0");
         m_fxUpsmplPositionMap1        =m_fxUpsmpl.GetParameter(null,"PositionMap1");
         m_fxUpsmplPositionMapScale    =m_fxUpsmpl.GetParameter(null,"PositionMapScale");
         m_fxUpsmplPositionData        =m_fxUpsmpl.GetParameter(null,"PositionData");
         m_fxUpsmplPixPackIndex        =m_fxUpsmpl.GetParameter(null,"PixPackIndex");
         m_fxUpsmplExRndMap            =m_fxUpsmpl.GetParameter(null,"ExRndMap");
         m_fxUpsmplPermutationTable    =m_fxUpsmpl.GetParameter(null,"PermutationTable");
         m_fxUpsmplIsLevel0            =m_fxUpsmpl.GetParameter(null,"IsLevel0");
         m_fxUpsmplLevelId             =m_fxUpsmpl.GetParameter(null,"LevelId");
         m_fxUpsmplLocalFrames         =m_fxUpsmpl.GetParameter(null,"LocalFrames");
         m_fxUpsmplLocalFramesRes      =m_fxUpsmpl.GetParameter(null,"LocalFramesRes");
         m_fxUpsmplLocalFramesInverse  =m_fxUpsmpl.GetParameter(null,"LocalFramesInverse");
         m_fxUpsmplCutAvoidance        =m_fxUpsmpl.GetParameter(null,"CutAvoidance");
         m_fxUpsmplCurrent_T_1         =m_fxUpsmpl.GetParameter(null,"Current_T_1");
         m_fxUpsmplCurrent_T_1_QuadSize=m_fxUpsmpl.GetParameter(null,"Current_T_1_QuadSize");
         m_fxUpsmplIsFirstAdvected     =m_fxUpsmpl.GetParameter(null,"IsFirstAdvected");
         m_fxUpsmplVectorField         =m_fxUpsmpl.GetParameter(null,"VectorField");


         #endregion
    
         #region CORRECTION SHADER
       // Correction shader
          

          nbdef=0;
          addDefine("K_SEARCH", m_iKSearch.ToString(), ref defines,ref nbdef);

          if ((flags & SynthesisFlags._Spherical) != 0 || (flags & SynthesisFlags._Octahedron) != 0 || (flags & SynthesisFlags._Atlas) != 0)
          {
               addDefine("USE_INDIRECTION","1",ref defines,ref nbdef);
          }

          if (!exemplar.isToroidal()) 
          {
              addDefine("NON_TOROIDAL_EXEMPLAR","1",ref defines,ref nbdef);
          }
          if (Globals.gSynthMethod == Globals.eSynthMethod.cSynth_Ansiometric) 
          {
               addDefine("ANISOSYNTH","1",ref defines,ref nbdef);
          }
         if(Globals.isDefined("4D"))
         {
            addDefine("LIMIT_TO_4D", "1", ref defines, ref nbdef);
         }

       
          addDefaultDefines(ref defines,ref nbdef);
          endDefine(ref defines,ref nbdef);
          m_fxCorrection = Rendering.ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\correction.fx", defines);



          m_fxCorrTech = m_fxCorrection.GetTechnique( "t_correction");
          m_fxCorrForceTech = m_fxCorrection.GetTechnique("t_correction_force");
          m_fxCorrFlagChartsTech = m_fxCorrection.GetTechnique( "t_correction_flag_charts");

          
          m_fxCorrection.Technique = m_fxCorrTech;
          
          m_fxCorrPrevious                   =m_fxCorrection.GetParameter(null,"Previous");
          m_fxCorrQuadrantIndex              =m_fxCorrection.GetParameter(null,"QuadrantIndex");
          m_fxCorrPassId                     =m_fxCorrection.GetParameter(null,"PassId");
          m_fxCorrExemplarSize               =m_fxCorrection.GetParameter(null,"ExemplarSize");
          m_fxCorrNeighOffset                =m_fxCorrection.GetParameter(null,"NeighOffset");
          m_fxCorrWindowPos                  =m_fxCorrection.GetParameter(null,"WindowPos");
          m_fxCorrPrevQuad                   =m_fxCorrection.GetParameter(null,"PrevQuad");
          m_fxCorrCoherence                  =m_fxCorrection.GetParameter(null,"Coherence");
          m_fxCorrIsLevel0                   =m_fxCorrection.GetParameter(null,"IsLevel0");
          m_fxCorrLevelId                    =m_fxCorrection.GetParameter(null,"LevelId");
          m_fxCorrTime                       =m_fxCorrection.GetParameter(null,"Time");
          m_fxCorrNeighOffsetLSynth          =m_fxCorrection.GetParameter(null,"NeighOffsetLSynth");
          m_fxCorrLocalFrames                =m_fxCorrection.GetParameter(null,"LocalFrames");
          m_fxCorrLocalFramesInverse         =m_fxCorrection.GetParameter(null,"LocalFramesInverse");
          m_fxCorrLocalFramesRes             =m_fxCorrection.GetParameter(null,"LocalFramesRes");
          m_fxCorrPixPackOffsets             =m_fxCorrection.GetParameter(null,"PixPackOffsets");
          m_fxCorrTexPixPackOffsets          =m_fxCorrection.GetParameter(null,"TexPixPackOffsets");
          m_fxCorrRelativeWindowPos          =m_fxCorrection.GetParameter(null,"RelativeWindowPos");
          m_fxCorrCutAvoidance               =m_fxCorrection.GetParameter(null,"CutAvoidance");
          m_fxCorrSimilaritySet              =m_fxCorrection.GetParameter(null,"SimilaritySet");
          m_fxCorrNeighborhoods_0_3          =m_fxCorrection.GetParameter(null,"Neighborhoods_0_3");
          
          m_fxCorrRecolored_0_3              =m_fxCorrection.GetParameter(null,"Recolored_0_3");
          
          m_fxCorrIndirection                =m_fxCorrection.GetParameter(null,"Indirection");
          m_fxCorrIndirectionTexPos          =m_fxCorrection.GetParameter(null,"IndirectionTexPos");
          m_fxCorrUnqNeighborhoods_Mean_0_3  =m_fxCorrection.GetParameter(null,"UnqNeighborhoods_Mean_0_3");
          
          m_fxCorrUnqNeighborhoods_Scale_0_3 =m_fxCorrection.GetParameter(null,"UnqNeighborhoods_Scale_0_3");
          
          m_fxCorrUnqRecolored_Mean_0_3      =m_fxCorrection.GetParameter(null,"UnqRecolored_Mean_0_3");
          
          m_fxCorrUnqRecolored_Scale_0_3     =m_fxCorrection.GetParameter(null,"UnqRecolored_Scale_0_3");
          
          m_fxCorrQuadZ                      =m_fxCorrection.GetParameter(null,"QuadZ");
          m_fxCorrScale                       = m_fxCorrection.GetParameter(null, "Scale");
          {
            string ev = "Ev";
            for (int i=0;i<Globals.NUM_RUNTIME_PCA_COMPONENTS;i++)
            {
               string v = ev + i;
              m_fxCorrEv[i]=m_fxCorrection.GetParameter(null,v);
            }
          }
      
         #endregion
          
         #region PIXEL PACKING SHADER


         defines[0].Name=null;
         defines[0].Definition = null;

         m_fxPixpack = Rendering.ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\pixelpacking.fx", defines);


         m_fxPixpackTech = m_fxPixpack.GetTechnique( "t_pixelpack");
         m_fxPixunpackTech = m_fxPixpack.GetTechnique( "t_pixelunpack");

         m_fxPixpack.Technique = m_fxPixpackTech;

         m_fxPixpackBuffer = m_fxPixpack.GetParameter(null, "Buffer");
         m_fxPixpackWindowPos = m_fxPixpack.GetParameter(null, "WindowPos");
         m_fxPixpackBufferQuad = m_fxPixpack.GetParameter(null, "BufferQuad");
         m_fxPixpackRegion = m_fxPixpack.GetParameter(null, "Region");

         #endregion

         #region COPY SHADER

          defines[0].Name = null;
          defines[0].Definition = null;

          m_fxCopy = Rendering.ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\copy.fx", defines);

          m_fxCopyTech = m_fxCopy.GetTechnique("t_copy");
          m_fxCopy.Technique = m_fxCopyTech;

          m_fxCopyTex = m_fxCopy.GetParameter(null, "Tex");
          m_fxCopySrcWindow = m_fxCopy.GetParameter(null, "SrcWindow");
          m_fxCopyDstWindow = m_fxCopy.GetParameter(null, "DstWindow");
          m_fxCopySrcBufferSize = m_fxCopy.GetParameter(null, "SrcBufferSize");
          m_fxCopyDstBufferSize = m_fxCopy.GetParameter(null, "DstBufferSize");

          #endregion

         #region INDEX BUFFER RETREVIAL SHADER
         s_fxRndrLvl = ShaderManager.loadShader(AppDomain.CurrentDomain.BaseDirectory + "shaders\\query.renderlevel.fx", null);

         s_fxRndrLvlNoIndirection = s_fxRndrLvl.GetTechnique("t_renderlevel_no_indirection");
         s_fxRndrLvlIndirection = s_fxRndrLvl.GetTechnique("t_renderlevel_indirection");
         s_fxRndrApplyPackedIndirection = s_fxRndrLvl.GetTechnique("t_apply_packed_indirection");
         s_fxRndrApplyIndirection = s_fxRndrLvl.GetTechnique("t_apply_indirection");

         s_fxSynthesisBuffer = s_fxRndrLvl.GetParameter(null, "SynthesisBuffer");
         s_fxSrcWindow = s_fxRndrLvl.GetParameter(null, "SrcWindow");
         s_fxDstWindow = s_fxRndrLvl.GetParameter(null, "DstWindow");
         s_fxWorkBufferSize = s_fxRndrLvl.GetParameter(null, "WorkBufferSize");
         s_fxIndirectionMap = s_fxRndrLvl.GetParameter(null, "IndirectionMap");
         s_fxSkipBorderTrsfrm = s_fxRndrLvl.GetParameter(null, "SkipBorderTrsfrm");
         #endregion

         tm.destroy();
         tm = null;

      }
      public static void addDefine(string name,string def,ref Macro []defs,ref int nbdef)
      {
         defs[nbdef].Name=name;
         defs[nbdef].Definition=def;
         nbdef++;  
      }
      public static void addDefaultDefines(ref Macro []defs,ref int nbdef)
      {
         /*
         defs[*nbdef].Name="FORCE_PRECISION";
         defs[*nbdef].Definition="1";
         (*nbdef)++;
         */
      }
      public static void endDefine(ref Macro[] defs, ref int nbdef)
      {
         defs[nbdef].Name=null;
         defs[nbdef].Definition=null;
         nbdef=0;
      }
      #endregion
      // -----------------------------------------------------
      #region HELPERS
     
      void setWindowViewPort(Window w)
      {

         Viewport vp = new Viewport();

         vp.X = 0;
         vp.Y = 0;
         vp.Width = w.width();
         vp.Height = w.height();
         vp.MinZ = 0;
         vp.MaxZ = 1;
         clampViewport(ref vp);
         BRenderDevice.getDevice().Viewport = vp;
      }
      public Vector4 bufferSize(int bufsize)
      {
        int sz=bufsize;
        return new Vector4((float)sz, (float)sz, 1.0f / (float)sz, 1.0f / (float)sz);
      }
      public static int bufferSizeFromWindow(Window w)
      {

         int n = 0;
         for (int sz = Math.Max(w.width(), w.height()) - 1; sz > 0; sz = sz >> 1)
            n++;

         //  int r=1 << n; // DEBUG

         int r = Math.Max(w.width(), w.height());
         if ((r & 1) != 0)
            r++;

         /*
         { 
         static char __str[256]; 
         sprintf(__str,"w = %d,%d  %dx%d\n",w.left(),w.top(),w.width(),w.height()); 
         OutputDebugStringA(__str); 
         }
         */
         Globals.Assert(r < 4096);
         return (r);
      }
      public Vector4 windowAsVec4(Window w)
      {
         return (new Vector4((float)w.left(), (float)w.top(), (float)w.width(), (float)w.height()));
      }
      Window computeWindowQuadrant(int sz,Window w,int i,int j)
      {
         int l = w.left();
         int t = w.top();
         int r = w.right();
         int b = w.bottom();

         int ol=0,ot=0,or=0,ob=0;

         if ((l & 1)!=0 && (i & 1)==0)
            ol=1;
         if ((t & 1) != 0 && (j & 1) == 0)
          ot=1;

       if ((r & 1) == 0 && (i & 1) != 0)
          or=-1;
       if ((b & 1) == 0 && (j & 1) != 0)
           ob=-1;

         l = (l/2) + (i*(sz >> 1)) + ol;
         r = (r/2) + (i*(sz >> 1)) + or;
         t = (t/2) + (j*(sz >> 1)) + ot;
         b = (b/2) + (j*(sz >> 1)) + ob;

         if (l > r)
            return (new Window());
         if (t > b)
          return (new Window());

         return Window.LRTB(l,r,t,b);
      }
      #endregion
      // -----------------------------------------------------
      #region SYNTHESIS
      public bool doForceMode = false;
      public int synthesize(SynthesisQuery query, SynthesizeParameters parms, bool force)
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().Transform.View = Matrix.Identity;
         BRenderDevice.getDevice().Transform.Projection = Matrix.OrthoOffCenterRH(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         bind(query);
         ScopeTimer st = new ScopeTimer("[Synthesize]");

         BRenderDevice.getDevice().BeginScene();
         if (force)
         {

            query.reinit(parms);
            initSynthesize(query);
            while (!synthesizeNextLevel(query, true)) { }
            unbind(query);
            locktarget.destroy();
            locktarget = null;
            st.destroy();
            st = null;
            //synthesis is done, grab our resulting index texture
            prepareIndexTexture(query, parms.mResultPacket);
            BRenderDevice.getDevice().EndScene();
            return (Globals.cNumLevels);
         }
         else
         {
            List<int> regen = query.update(parms);

            if (regen.Count == 0)
            {
               unbind(query);
               locktarget.destroy();
               locktarget = null;
               BRenderDevice.getDevice().EndScene();
               return (0);
            }

            for (int i = 0; i < regen.Count; i++)
               updateLevel(query, regen[i]);
            

            query.setLevel(0);
            unbind(query);
            st.destroy();
            st = null;
            locktarget.destroy();
            locktarget = null;
            prepareIndexTexture(query, parms.mResultPacket);
            BRenderDevice.getDevice().EndScene();

            return ((int)regen.Count);
         }
         
      }
      public SynthesisQuery synthesize(SynthesizeParameters parms)
      {
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().Transform.View = Matrix.Identity;
         BRenderDevice.getDevice().Transform.Projection = Matrix.OrthoOffCenterRH(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         SynthesisQuery q = new SynthesisQuery(this,parms);

         ScopeTimer st = new ScopeTimer("[Synthesize]");
         bind(q);
         BRenderDevice.getDevice().BeginScene();


         initSynthesize(q);
         while (!synthesizeNextLevel(q, true)) { }
         unbind(q);
         st.destroy();
         st = null;

         
         locktarget.destroy();
         locktarget = null;

         //synthesis is done, grab our resulting index texture
         prepareIndexTexture(q, parms.mResultPacket);

         BRenderDevice.getDevice().EndScene();

         return (q);
      }
      public void bind(SynthesisQuery q)
      {
         // check whether flags are ok or not
         if (m_CompiledFlags != q.m_Params.flags()) 
         {
            if (Globals.PRINT_DEBUG_MSG) Debug.Print("\n\n<=== Query flags different - Reloading programs ===>\n");
            loadPrograms(q.m_Params.mExemplar, q.m_Params.flags());
         }
    
         m_SavedDepthStencilSurface = BRenderDevice.getDevice().DepthStencilSurface;
         BRenderDevice.getDevice().DepthStencilSurface = q.m_DepthStencilSurface;

      }
      public void unbind(SynthesisQuery q)
      {
         // restore depth-stencil
         BRenderDevice.getDevice().DepthStencilSurface = m_SavedDepthStencilSurface;
         m_SavedDepthStencilSurface = null;
      }

      bool initSynthesize(SynthesisQuery q)
      {
         //CLM force mode..
         if (doForceMode == true)
         {
            q.m_iLevel = 0;
            return true;
         }

         if (q.level() > Globals.cNumLevels - 2)
         {
            init(q, q.m_WorkBuffer.back(), false);
            q.m_WorkBuffer.swap();

            if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(q.m_WorkBuffer.front().texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\pre_pack_level_" + q.level() + ".bmp");
            packLevel(q, q.m_WorkBuffer.front(), q.m_SynthBuffers[q.level()], false);
            if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(q.m_SynthBuffers[q.level()].texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\post_pack_level_" + q.level() + ".bmp");

            if (q.level() > Globals.cNumLevels - 1)
               return (false);

            return true;
         }
         return false;
      }
      bool synthesizeNextLevel(SynthesisQuery q,bool store)
      {
         if (doForceMode == true)
         {
           
           
         }
         //-----

         if ((q.level() - 1) < mIndexLevelToGrab)
            return true;

         if (!q.m_Params.mExemplar.mSynthParams.mPerLevelDoCorrection[q.level()])//(q.level()/*-1*/) > Math.Min(Globals.cNumLevels-1,q.m_Params.getStartAtLevel())|| (q.level()-1) < q.m_Params.getStopAtLevel())
         {
            upSamplingAndJitter(q,q.m_SynthBuffers[q.level()],store ? q.m_SynthBuffers[q.level()-1] :  q.m_WorkBuffer.front(),false);
         }
         else
         {
            if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(q.m_SynthBuffers[q.level()].texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\pre_upsampleJitter_" + q.level() + ".bmp");
            upSamplingAndJitter(q,q.m_SynthBuffers[q.level()],q.m_WorkBuffer.back(),false);
            if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(q.m_WorkBuffer.back().texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\post_upsampleJitter_" + q.level() + ".bmp");
            // ***** after upsampling q.level() has been updated *****

            q.m_WorkBuffer.swap();

            for (int n = 0; n < Globals.cNumCorrectionPasses; n++)
            {
               bool last = (n == Globals.cNumCorrectionPasses - 1);

               single_buffer dest=null;
               if (last && store) // store final correction sub-pass result into level buffer
                  dest = (q.m_SynthBuffers[q.level()]);

             
               
                  if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(q.m_WorkBuffer.back().texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\pre_Correction_" + n + "_" + q.level() + ".bmp");
                  correction(q, n, false, q.m_WorkBuffer, dest, false);
                  if (Globals.TAKE_SCREEN_CAPTURES) BRenderDevice.writeTextureToFile(dest == null ? q.m_WorkBuffer.back().texture() : dest.texture(), AppDomain.CurrentDomain.BaseDirectory + "screens\\post_Correction_" + n + "_" + q.level() + ".bmp");

               
            }
         }
         //else
         //  no correction

         return (q.level() == 0);
      }
      void updateLevel(SynthesisQuery q, int l)
      {

         if (l == Globals.cNumLevels - 1)
            return;


         single_buffer level_buffer = q.getBuffer(l);

         // set query to current level
         q.setLevel(l);

         // Copy valid region from work buffer (translation)
         if (!q.m_Params.mControlPacket.m_Windows.updCopySrc(l).empty())
         {
            Window srcwin = q.m_Params.mControlPacket.m_Windows.updCopySrc(l);
            Window dstwin = q.m_Params.mControlPacket.m_Windows.updCopyDst(l);
            // direct copy into work buffer
            copyPackedWindows(level_buffer.texture(), level_buffer.size(), srcwin, q.m_WorkBuffer.back().surface(), q.m_WorkBuffer.size(), srcwin, -1, -1);
            // copy and translate into level buffer
            copyPackedWindows(q.m_WorkBuffer.back().texture(), q.m_WorkBuffer.size(), srcwin, level_buffer.surface(), level_buffer.size(), dstwin, -1, -1);
         }

         // Synthesize horizontal update region
         if (!q.m_Params.mControlPacket.m_Windows.updHorz(l).empty())
         {
            // . replace level window
            q.m_Params.mControlPacket.m_Windows.beginUpdateHorz(l);
            // . query state on coarser level
            q.setLevel(l + 1);
            // . synthesize level - result in work buffer
            synthesizeNextLevel(q, false);
            // . copy to level buffer
            Window srcwin = q.m_Params.mControlPacket.m_Windows.valid(l);
            Window dstwin = q.m_Params.mControlPacket.m_Windows.updHorz(l);
            copyPackedWindows(q.m_WorkBuffer.front().texture(), q.m_WorkBuffer.size(), srcwin, level_buffer.surface(), level_buffer.size(), dstwin, -1, -1);
            // . restore level window
            q.m_Params.mControlPacket.m_Windows.endUpdate();
         }

         // Synthesize vertical update region
         if (!q.m_Params.mControlPacket.m_Windows.updVert(l).empty())
         {
            // . replace level window
            q.m_Params.mControlPacket.m_Windows.beginUpdateVert(l);
            // . query state on coarser level
            q.setLevel(l + 1);
            // . synthesize level in query update buffer
            synthesizeNextLevel(q, false);
            // . copy to level buffer
            Window srcwin = q.m_Params.mControlPacket.m_Windows.valid(l);
            Window dstwin = q.m_Params.mControlPacket.m_Windows.updVert(l);
            copyPackedWindows(q.m_WorkBuffer.front().texture(), q.m_WorkBuffer.size(), srcwin, level_buffer.surface(), level_buffer.size(), dstwin, -1, -1);
            // . restore level window
            q.m_Params.mControlPacket.m_Windows.endUpdate();
         }

         // Done
         q.setLevel(l);

      }

      public void clampViewport(ref Viewport vp)
      {
         //CLM This placed here to test when we get D3D deaths due to misaligned VPs
         //Surface sf = BRenderDevice.getDevice().GetRenderTarget(0);
         //int x = vp.X + vp.Width;
         //int y = vp.Y + vp.Height;
         //int w = sf.Description.Width;
         //int h = sf.Description.Height;
         //Globals.Assert(vp.X + vp.Width <= sf.Description.Width);
         //Globals.Assert(vp.Y + vp.Height <= sf.Description.Height);
         //sf.Dispose();
         //sf = null;
      }
      #region CORE ALGORITHM
      public void init(SynthesisQuery q, single_buffer dst, bool clear)
      {
         int lvl = q.m_Params.mExemplar.mSynthParams.mStartAtLevel;// Globals.cNumLevels - 1;
         if (lvl == 0)
            return;

         q.m_iLevel = (lvl);

         // disable z-test
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, false);
         BRenderDevice.getDevice().SetRenderTarget(0, dst.surface());

         if (clear || Globals.TEST_PADDING)
            BRenderDevice.getDevice().Clear(ClearFlags.Target, Color.White, 0, 0);

         Window winit = q.m_Params.mControlPacket.m_Windows.bufferrgn(q.level());
         setWindowViewPort(winit);

         // Jacobian Map
         Globals.Assert(q.getJacobianMap(lvl) != null);
         SurfaceDescription desc = q.getJacobianMap(0).GetLevelDescription(0);
         Vector4 tr = new Vector4((float)desc.Width, (float)desc.Height, 1.0f / (float)desc.Width, 1.0f / (float)desc.Height);
         m_fxInit.SetValue(m_fxInitLocalFramesRes, tr);
         
         // . exemplar size  // TODO / FIXME assumes square exemplar
         float w = (float)q.m_Params.mExemplar.stack(Globals.cNumLevels - 1).getWidth();
         m_fxInit.SetValue(m_fxInitExemplarSize, (float)w);

         // . randomness scale
         m_fxInit.SetValue(m_fxInitScale, ((float)q.m_Params.getRandomness(lvl)) / (0.25f * 100.0f));

         // . window pos
         Vector4 wpos = new Vector4((float)winit.left(), (float)winit.top(), (float)winit.width(), (float)winit.height());
         m_fxInit.SetValue(m_fxInitWindowPos, wpos);

        
        // if(q.level() == 1)
        //    m_fxInit.Technique = m_fxInitTechForce;
        // else 
            m_fxInit.Technique = m_fxInitTech;

         m_fxInit.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
         m_fxInit.BeginPass(0);
         m_QuadDual.render();
         m_fxInit.EndPass();
         m_fxInit.End();
      }
      void packLevel(SynthesisQuery      q,single_buffer  src,single_buffer  dst,bool clear)
      {   
        D3DProtectRenderTarget protect = new D3DProtectRenderTarget(true);

        int l=q.level();
        Globals.Assert(l >= 0);

        // . Level window
        Window window = q.m_Params.mControlPacket.m_Windows.bufferrgn(l);

         // . window pos
        Vector4 wpos=new Vector4((float)window.left(), (float)window.top(), (float)window.width(), (float)window.height());
        m_fxPixpack.SetValue(m_fxPixpackWindowPos,wpos);

        // . Buffer quad
        Vector4 qsz=bufferSize(src.size());
        m_fxPixpack.SetValue(m_fxPixpackBufferQuad,qsz);

        // . Previous step texture
        m_fxPixpack.SetValue(m_fxPixpackBuffer,src.texture());

        // . Target buffer size
        Vector4 tsz = new Vector4(0, 0, (float)dst.size(), (float)dst.size());
        m_fxPixpack.SetValue(m_fxPixpackRegion,tsz);

        // . Target buffer
        BRenderDevice.getDevice().SetRenderTarget(0, dst.surface());

        if (clear || Globals.TEST_PADDING)
           BRenderDevice.getDevice().Clear(ClearFlags.Target,Color.White,1,0);

        // . Viewport covers full texture       TODO: four quads to reduce fragment count ! 
                                                   // (pack is only done after init so not critical for now)
        Viewport vp = new Viewport();
        vp.X     =0;
        vp.Y     =0;
        vp.Width =dst.size();
        vp.Height=dst.size();
        vp.MinZ  =0;
        vp.MaxZ  =1;
        clampViewport(ref vp);
        BRenderDevice.getDevice().Viewport = vp;

        // . Apply shader
        m_fxPixpack.Technique = m_fxPixpackTech;
      //  m_fxPixpack.CommitChanges();

       // BRenderDevice.beginScene();
        {

           m_fxPixpack.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
          m_fxPixpack.BeginPass(0);
          m_QuadDual.render();
          m_fxPixpack.EndPass();
          m_fxPixpack.End();

       //   BRenderDevice.endScene();
        }
        protect.destroy();
        protect = null;
      }
      void upSamplingAndJitter(SynthesisQuery q,single_buffer src,single_buffer dst,bool clear)
      {
      
        int l=q.level()-1; // upsampled level - parent is q.level()
        Globals.Assert(l >= 0);

        BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, false);

        // . Level window
        Window window = q.m_Params.mControlPacket.m_Windows.bufferrgn(l);
        Window parent = q.m_Params.mControlPacket.m_Windows.bufferrgn(l + 1);

        // . Is parent large enough ?
        Globals.Assert((q.level() == Globals.cNumLevels - 1) || q.m_Params.mControlPacket.m_Windows.absValid(l + 1).includes(q.m_Params.mControlPacket.m_Windows.windowCoarserLevel(q.m_Params.mControlPacket.m_Windows.bufferrgn(l))));

        // . Buffer size (so called 'quad size') FIXME: rename quad size => buffer size
        Vector4 qprt=bufferSize(src.size());  // current buffer is parent buffer at this point
        m_fxUpsmpl.SetValue(m_fxUpsmplParentQuadSize,qprt);

        // . Neigh offset
        m_fxUpsmpl.SetValue(m_fxUpsmplNeighOffset,(float)(1 << l));

        // . Flag level 0 (used to handle level 0 special case with compress_clr)
        m_fxUpsmpl.SetValue(m_fxUpsmplIsLevel0,(l == 0 ? 1.0f : 0.0f));

        // . Level id
        m_fxUpsmpl.SetValue(m_fxUpsmplLevelId,(float)l);

        //// . Time
        // float g_iExSpaceRotationStartTime =0;
        //float time=0;//float(timeGetTime());
        //m_fxUpsmpl.SetValue(m_fxUpsmplTime,time-g_iExSpaceRotationStartTime);
        //// DEBUG
        //m_fxUpsmpl.SetValue(m_fxCorrTime, time - g_iExSpaceRotationStartTime);

        // . Parent packed index map texture
        m_fxUpsmpl.SetValue(m_fxUpsmplPrevious, src.texture());

        // . Destination buffer
        BRenderDevice.getDevice().SetRenderTarget(0,dst.surface());

        //if (clear || Globals.TEST_PADDING)
        //  BRenderDevice.getDevice().Clear(ClearFlags.Target,Color.White,1.0f,0);

        // . Exemplar size  // TODO / FIXME assumes square exemplar
        float w=(float)q.m_Params.mExemplar.stack(l).getWidth();
        m_fxUpsmpl.SetValue(m_fxUpsmplExemplarSize,(float)w);

        // . Randomness
        m_fxUpsmpl.SetValue(m_fxUpsmplRandomness, 4.0f * (float)(q.m_Params.getRandomness(l)));
        
            m_fxUpsmpl.SetValue(m_fxUpsmplScale, ((float)q.m_Params.getRandomness(l))/(0.25f*100.0f*((int)w >> l))); // TODO what should this formulae ideally be ?
         //    *0.015/((int)w >> l) ));
         


           // . Cut avoidance
         m_fxUpsmpl.SetValue(m_fxUpsmplCutAvoidance, (float)q.m_Params.getCutAvoidance());

        // . raint map
        if (q.m_Params.mControlPacket.mConstraintMap != null)
           m_fxUpsmpl.SetValue(m_fxUpsmplCstrMap, q.m_Params.mControlPacket.mConstraintMap);


        // . Local frames
        SurfaceDescription desc = q.getJacobianMap(0).GetLevelDescription(0);
        Vector4 tr = new Vector4((float)desc.Width,(float)desc.Height,1.0f/(float)desc.Width,1.0f/(float)desc.Height);
        m_fxUpsmpl.SetValue(m_fxUpsmplLocalFramesRes, tr);
        m_fxUpsmpl.SetValue(m_fxUpsmplLocalFrames, q.getJacobianMap(l));
        m_fxUpsmpl.SetValue(m_fxUpsmplLocalFramesInverse, q.getJacobianMapInverse(l));

        // . Exemplar Constraint
        if (q.m_Params.mExemplar.isConstrained()) 
        {
          // . exemplar raint
           m_fxUpsmpl.SetValue(m_fxUpsmplExemplarCstr, q.m_Params.mExemplar.levelData(l).m_d3dExemplarConstraint);
          // . raint threshold
          m_fxUpsmpl.SetValue(m_fxUpsmplCstrThreshold,q.m_Params.constraintThreshold()/100.0f);
        }

        // . raint map scale
        m_fxUpsmpl.SetValue(m_fxUpsmplCstrMapScale, 1.0f / (8.0f * (1 << (Globals.cNumLevels - 1 - l))));

        // . Exemplar randomness map
        if (q.m_Params.exemplarRndMap() != null)
           m_fxUpsmpl.SetValue(m_fxUpsmplExRndMap, q.m_Params.exemplarRndMap());

        if (q.m_Params.getRandomness(q.level() - 1) == 0)
            m_fxUpsmpl.Technique = m_fxUpsmplTech;
         else
            m_fxUpsmpl.Technique = m_fxUpsmplJitterTech;

        for (int n=0;n<4;n++)
        {
          int i=(n/2);
          int j=(n%2);
          Vector4 idx = new Vector4((float)i,(float)j,0,0);
          m_fxUpsmpl.SetValue(m_fxUpsmplPixPackIndex,idx);

          // . set viewport on packed part
          Window rquad   = computeWindowQuadrant(0,Window.LTWH(0,0,window.width(),window.height()),i,j);

          Globals.Assert(rquad.left() == 0 && rquad.top() == 0);

          Viewport vp = new Viewport();
          vp.X      = ((i*dst.size()) >> 1); //  + rquad.left() 
          vp.Y      = ((j*dst.size()) >> 1); //  + rquad.top() 
          vp.Width  = rquad.width();
          vp.Height = rquad.height();
          vp.MinZ   = 0;
          vp.MaxZ   = 1;
          clampViewport(ref vp);
          BRenderDevice.getDevice().Viewport = vp;

          // . Window pos
          Vector4 wpos=new Vector4((float)window.left(), (float)window.top(),(float)rquad.width(), (float)rquad.height());
          m_fxUpsmpl.SetValue(m_fxUpsmplWindowPos, wpos);

          Window pquad   = computeWindowQuadrant(0,parent,i,j);
          Vector4 wprt=new Vector4((float)parent.left(), (float)parent.top(),(float)pquad.width(), (float)pquad.height());
          m_fxUpsmpl.SetValue(m_fxUpsmplParentWindowPos, wprt);

          // . Apply changes
       //   m_fxUpsmpl.CommitChanges();

      //    BRenderDevice.beginScene();
          {
             m_fxUpsmpl.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
            m_fxUpsmpl.BeginPass(0);
            m_QuadDual.render();
            m_fxUpsmpl.EndPass();
            m_fxUpsmpl.End();  
          }
       //   BRenderDevice.endScene();

        }

        // . update query level
        q.m_iLevel=l;
       
      }

  
      unsafe void correction(SynthesisQuery q,int pass,bool advecting,swap_buffer dblbuf,single_buffer finaldst,bool clear)
      {
         m_fxCorrection.Technique = m_fxCorrTech;

         int l=q.level();
         Globals.Assert(l >= 0);

         int lsynth=l + 0;// Globals.g_iLevelShift;
         lsynth=Math.Min(Globals.cNumLevels-1,Math.Max(0,lsynth));

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable,false);

         // . window
         Window window = q.m_Params.mControlPacket.m_Windows.bufferrgn(l);

         // . randomness scale
         m_fxInit.SetValue(m_fxInitScale, ((float)q.m_Params.getRandomness(q.level())) / (0.25f * 100.0f));

         // . buffers size
         Vector4 qsz      = bufferSize(dblbuf.size());
         m_fxCorrection.SetValue(m_fxCorrPrevQuad,qsz);

         // . neigh offset
         m_fxCorrection.SetValue(m_fxCorrNeighOffset,(float)(1 << l));
         m_fxCorrection.SetValue(m_fxCorrNeighOffsetLSynth,(float)(1 << lsynth));

         // . flag level 0
         m_fxCorrection.SetValue(m_fxCorrIsLevel0,(l == 0 ? 1.0f : 0.0f));

         // . level Id
         m_fxCorrection.SetValue(m_fxCorrLevelId,(float)l);

         // . exemplar size  // TODO / FIXME assumes square exemplar
         float w = (float)q.m_Params.mExemplar.stack(lsynth).getWidth();
         m_fxCorrection.SetValue(m_fxCorrExemplarSize,(float)w);

         // . pass id
         m_fxCorrection.SetValue(m_fxCorrPassId,(float)pass);
        
          // . unquantize, recolored exemplar

          m_fxCorrection.SetValue(m_fxCorrUnqRecolored_Mean_0_3,q.m_Params.mExemplar.levelData(lsynth).qr_mean_0_3);
          m_fxCorrection.SetValue(m_fxCorrUnqRecolored_Scale_0_3,q.m_Params.mExemplar.levelData(lsynth).qr_scale_0_3);

         // . unquantize, neighbors
          m_fxCorrection.SetValue(m_fxCorrUnqNeighborhoods_Mean_0_3,q.m_Params.mExemplar.levelData(lsynth).qn_mean_0_3);
          m_fxCorrection.SetValue(m_fxCorrUnqNeighborhoods_Scale_0_3,q.m_Params.mExemplar.levelData(lsynth).qn_scale_0_3);
          

          // . setup eigen vectors
          {
            for (int v=0;v<Globals.NUM_RUNTIME_PCA_COMPONENTS;v++)
            {
              fixed (Vector4* p0 = q.m_Params.mExemplar.pcaSynth()[lsynth].mGroupedEigenVecs[v])
              {
                 m_fxCorrection.SetValue(m_fxCorrEv[v], p0, 18 * (sizeof(float)*4));
              }
            }
          }

          // . local frames
          SurfaceDescription desc = q.getJacobianMap(0).GetLevelDescription(0);
          Vector4 tr = new Vector4((float)desc.Width,(float)desc.Height,1.0f/(float)desc.Width,1.0f/(float)desc.Height);
          m_fxCorrection.SetValue(m_fxCorrLocalFramesRes,tr);
          m_fxCorrection.SetValue(m_fxCorrLocalFrames, q.getJacobianMap(l));
          m_fxCorrection.SetValue(m_fxCorrLocalFramesInverse, q.getJacobianMapInverse(l));

          m_fxCorrection.SetValue(m_fxCorrNeighborhoods_0_3, q.m_Params.mExemplar.levelData(lsynth).m_d3dNeighborhoods_0_3);    // . neighborhoods texture
          m_fxCorrection.SetValue(m_fxCorrRecolored_0_3, q.m_Params.mExemplar.levelData(lsynth).m_d3dRecolored_0_3);// . recolored exemplar textures
          m_fxCorrection.SetValue(m_fxCorrSimilaritySet, q.m_Params.mExemplar.levelData(lsynth).m_d3dSimilaritySet);// . similarity set
         

         // . coherence
         if (!advecting) 
         {
           m_fxCorrection.SetValue(m_fxCorrCoherence,1.0f + (q.m_Params.coherenceThreshold())*2.0f/100.0f);
         }
         else 
         {
           // during advection use randomness controls for per-level coherence
            float global = 1.0f + (q.m_Params.coherenceThreshold()) * 2.0f / 100.0f;
           float level  = 1.0f + (q.m_Params.getRandomness(l))*2.0f/100.0f;
           m_fxCorrection.SetValue(m_fxCorrCoherence,Math.Max(global,level)); 
         }

          // . Cut avoidance
          m_fxCorrection.SetValue(m_fxCorrCutAvoidance,(float)q.m_Params.getCutAvoidance());

          // . Correction sub-passes
          for (int sub=0; sub<4; sub++)
          {
              bool last =    (sub == 3)|| ((m_CompiledFlags & SynthesisFlags._Subpass1212)!=0 && (sub >= 2));

            // . which quadrant to update ?
            int ai=(m_SubpassesOrderingTbl[pass % Globals.SYNTH_ORDER_NUM_PASSES][sub][0] + window.left()) & 1;
            int aj = (m_SubpassesOrderingTbl[pass % Globals.SYNTH_ORDER_NUM_PASSES][sub][1] + window.top()) & 1;
            Globals.Assert(ai >= 0 && aj >= 0);

            // . pass index
            Vector4 idx = new Vector4((float)ai,(float)aj,0.0f,0.0f);
            m_fxCorrection.SetValue(m_fxCorrQuadrantIndex,idx);

            // . destination buffer
             single_buffer target= (dblbuf.back());

            // . output to finaldst buffer if last sub-pass (and finaldst buffer present)
            if (last && finaldst != null)
              target = finaldst;

           
               
              // . copy previous subpass result
              copyPackedWindows(
                         dblbuf.front().texture(),dblbuf.front().size(),
                         Window.LTWH(0,0,window.width(),window.height()),
                         target.surface(),       target.size(),       
                         Window.LTWH(0,0,window.width(),window.height()),
                         ai,aj); // skip corrected quadrant

             
              // . set render target
              BRenderDevice.getDevice().SetRenderTarget(0,target.surface());

              // . previous subpass texture
              m_fxCorrection.SetValue(m_fxCorrPrevious,dblbuf.front().texture());
            



             //CLM if this assert fires, it usually means that somewhere, m_windows has been recomputed or destroyed (destroying the d3d data it contains..)
             //CLM TODO: this is currently pre-computed per RenderControl packet. We should have a global place for it to go... (here? maybe?)
             //CLM TODO: reduce this from 56 5x5 FLOAT16RG textures to something more memory friendly!!!
             //CLM NOTE: I tried putting these in constant registers... that actually ended up SLOWER?!?!?!!?
             //CLM NOTE: Turns out this is due to NVIDIA emulating constant registers on the GPU. This should be fine to put into const regs on the 360.
            Globals.Assert(q.m_Params.mControlPacket.m_Windows.mD3DPixPackOffsets[l, pass, sub] != null);
            m_fxCorrection.SetValue(m_fxCorrTexPixPackOffsets,q.m_Params.mControlPacket.m_Windows.mD3DPixPackOffsets[l, pass, sub]);//m_d3dPixpackOffsets);


            // . shrink correction window
            Window shrinked = window;
            for (int r = 0; r <= pass * 4 + sub; r++)
            {
               shrinked = Window.LRTB(
                 shrinked.left() + m_SubpassesShrinkTbl[r % 9][0],
                 shrinked.right() - m_SubpassesShrinkTbl[r % 9][0],
                 shrinked.top() + m_SubpassesShrinkTbl[r % 9][1],
                 shrinked.bottom() - m_SubpassesShrinkTbl[r % 9][1]);
            }
            // . set viewport on packed part
            Window rquad = computeWindowQuadrant(0,shrinked.relativeTo(window),ai,aj);

            Viewport tvp = new Viewport();
            tvp.X      = ((ai*target.size()) >> 1) + rquad.left();
            tvp.Y      = ((aj*target.size()) >> 1) + rquad.top();
            tvp.Width  = rquad.width();
            tvp.Height = rquad.height();
            tvp.MinZ   = 0;
            tvp.MaxZ   = 1;
            clampViewport(ref tvp);
            BRenderDevice.getDevice().Viewport =  tvp;

            // . set window pos
            Vector4 wpos=new Vector4((float)window.left(), (float)window.top(),(float)rquad.width(), (float)rquad.height());
            m_fxCorrection.SetValue(m_fxCorrWindowPos,wpos);

            Vector4 rpos=new Vector4((float)rquad.left(),(float)rquad.top(),0,0);
            m_fxCorrection.SetValue(m_fxCorrRelativeWindowPos,rpos);

            // . apply shader (only 1/4 will be updated)
              // . correct
              m_fxCorrection.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
              m_fxCorrection.BeginPass(0);
              m_QuadDual.render();
              m_fxCorrection.EndPass();
              m_fxCorrection.End();
             
        
              if (target != finaldst)
                dblbuf.swap();

          } // for p


          Vector4 rtpos=new Vector4((float)0,(float)0,0,0);
          m_fxCorrection.SetValue(m_fxCorrRelativeWindowPos,rtpos);
        
     }
      void copyPackedWindows(Texture d3dsrc,int srcsz, Window wsrc,Surface d3ddst,int dstsz, Window wdst,int skipi,int skipj)
      {
         // . set source parameters
         Vector4 vsrcbufsz = new Vector4((float)srcsz, (float)srcsz, (float)1.0f / srcsz, (float)1.0f / srcsz);
         m_fxCopy.SetValue(m_fxCopySrcBufferSize,vsrcbufsz);

         // .set destination parameters    
         Vector4 vdstbufsz = new Vector4((float)dstsz,(float)dstsz,(float)1.0f/dstsz,(float)1.0f/dstsz);
         m_fxCopy.SetValue(m_fxCopyDstBufferSize,vdstbufsz);

         // . set render target & source texture
         BRenderDevice.getDevice().SetRenderTarget(0,d3ddst);
         m_fxCopy.SetValue(m_fxCopyTex,d3dsrc);

         m_fxCopy.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
         m_fxCopy.BeginPass(0);
         // copy four quadrants
         for (int n=0;n<4;n++)
         {
            int i=n/2;
            int j=n%2;

            if (i == skipi && j == skipj) continue;

            // . set source parameters
            Window qwsrc=computeWindowQuadrant(srcsz,wsrc,i,j);
            Vector4 vsrcwin = new Vector4((float)qwsrc.left(),(float)qwsrc.top(),(float)qwsrc.width(),(float)qwsrc.height());
            m_fxCopy.SetValue(m_fxCopySrcWindow,vsrcwin);

            // .set destination parameters    
            Window qwdst=computeWindowQuadrant(dstsz,wdst,(i+wdst.left()-wsrc.left()) & 1,(j+wdst.top()-wsrc.top()) & 1);
            Vector4 vdstwin = new Vector4((float)qwdst.left(),(float)qwdst.top(),(float)qwdst.width(),(float)qwdst.height());
            m_fxCopy.SetValue(m_fxCopyDstWindow,vdstwin);

            if (!qwsrc.empty() && !qwdst.empty())
            {
               Globals.Assert ((qwdst.width() == qwsrc.width() && qwdst.height() == qwsrc.height()));

               Globals.Assert(qwdst.left() >= 0 && qwdst.top() >=0);
               Globals.Assert(qwsrc.left() >= 0 && qwsrc.top() >=0);

               // viewport on destination window
               Viewport vp = new Viewport();
               vp.X      = qwdst.left();
               vp.Y      = qwdst.top();
               vp.Width  = qwdst.width();
               vp.Height = qwdst.height();
               vp.MinZ   = 0;
               vp.MaxZ   = 1;
               clampViewport(ref vp);
               BRenderDevice.getDevice().Viewport = vp;

               // render !
               m_fxCopy.CommitChanges();
          //     BRenderDevice.getDevice().BeginScene();
               
                 
                  m_QuadDual.render();
                  

           //       BRenderDevice.getDevice().EndScene();

               

            }
         }
         m_fxCopy.EndPass();
         m_fxCopy.End();

      }
      
      #endregion

      #region RETRIEVE RESULTS
      public int mIndexLevelToGrab = 0;

      public void renderResultToTexture(Surface depthStencil, SynthResultPacket rp, SynthesisQuery m_Query, int targetw, int targeth, float zoomFactor, bool useHighres)
      {
         if (m_Query == null)
            return;

         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);

         
          m_SavedDepthStencilSurface = BRenderDevice.getDevice().DepthStencilSurface;
          BRenderDevice.getDevice().DepthStencilSurface = depthStencil;
        // bind(m_Query);



          BRenderDevice.getDevice().SetRenderTarget(0, rp.mResultTexSurface);

         SurfaceDescription desc = rp.mSynthResultIndexTexture.GetLevelDescription(0);


         int exsz = m_Query.m_Params.mExemplar.stack(0).getWidth();

         //CLM set our window, this needs to translate with each terrain chunk
         Vector4 corner = new Vector4(
                               ((int)m_Query.m_Params.mControlPacket.WindowMinX) / (float)desc.Width,
                               ((int)m_Query.m_Params.mControlPacket.WindowMinY) / (float)desc.Height,
                               (m_Query.m_Params.mControlPacket.WindowMinX - (int)m_Query.m_Params.mControlPacket.WindowMinX),
                               (m_Query.m_Params.mControlPacket.WindowMinY - (int)m_Query.m_Params.mControlPacket.WindowMinY));
         m_fxDisplay.SetValue(m_fxDispTrl, corner);

         Vector4 res = new Vector4((float)targetw, (float)targeth, 1.0f / (float)targetw, 1.0f / (float)targeth);
         m_fxDisplay.SetValue(m_fxDispRes, res);
         Vector4 bufres = new Vector4((float)desc.Width, (float)desc.Height, 1.0f / (float)desc.Width, 1.0f / (float)desc.Height);
         m_fxDisplay.SetValue(m_fxDispIndexMapRes, bufres);

         //zoom factor
         float zoomFact = (float)(Math.Exp(1.0) / Math.Exp(zoomFactor));
         m_fxDisplay.SetValue(m_fxDispInvZoomFactor, zoomFact);

         bool okForHighRes = useHighres && m_Query.m_Params.mExemplar.getHighResExemplar() != null;
         if (okForHighRes) // && !m_bAnisoEnabled)
         {
            SurfaceDescription descexhr = m_Query.m_Params.mExemplar.getHighResExemplar().mTexture.GetLevelDescription(0);
            m_fxDisplay.SetValue(m_fxDispExemplarHighRes, (float)descexhr.Width);
            m_fxDisplay.SetValue(m_fxDispExemplar, m_Query.m_Params.mExemplar.getHighResExemplar().mTexture);
            m_fxDisplay.SetValue(m_fxDispNumExHighresLevels, (float)(int)(0.01 + BMathLib.log2((float)descexhr.Width)));
         }
         else
         {
            m_fxDisplay.SetValue(m_fxDispExemplarHighRes, (float)exsz);
            m_fxDisplay.SetValue(m_fxDispExemplar, m_Query.m_Params.mExemplar.getInternalExemplarTexture());
         }

         // bind texture to shader
         m_fxDisplay.SetValue(m_fxDispIndexMap, rp.mSynthResultIndexTexture);


         Vector4 exres = new Vector4((float)exsz, 1.0f / (float)exsz, 0, 0);
         m_fxDisplay.SetValue(m_fxDispExemplarRes, exres);

         m_fxDisplay.SetValue(m_fxDispLocalFrames, m_Query.getJacobianMap(0));


         if (/*(mRenderTechnique == eRenderTechnique.cMagLinear) || */(okForHighRes || Globals.gSynthMethod == Globals.eSynthMethod.cSynth_Isometric))
            m_fxDisplay.Technique = (m_fxDispTechMagnify);
         else
            m_fxDisplay.Technique = (m_fxDispTechColor);

     //    m_fxDisplay.CommitChanges();

         //CLM uncomment if we move this back into a non-scene loop
         //BRenderDevice.getDevice().BeginScene();
         m_fxDisplay.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
         m_fxDisplay.BeginPass(0);
         m_Quad.render();
         m_fxDisplay.EndPass();

         m_fxDisplay.End();
         m_fxDisplay.Technique = m_fxDispTechSimple;

         //BRenderDevice.getDevice().EndScene();

         BRenderDevice.getDevice().DepthStencilSurface = m_SavedDepthStencilSurface;
         m_SavedDepthStencilSurface = null;
       //  unbind(m_Query);
         locktarget.destroy();
         locktarget = null;
      }
      public void prepareIndexTexture(SynthesisQuery m_Query, SynthResultPacket rp)
      {
         //// create a texture to receive level data if necessary
         if (rp.mSynthResultIndexTexture == null)
         {
            rp.mSynthResultIndexTexture = createLevelIndexMap(m_Query, mIndexLevelToGrab);
         }
         else
         {
            if (!isTextureSuitableToRetrieveLevelIndexMap(m_Query, mIndexLevelToGrab, rp.mSynthResultIndexTexture))
            {
               if (rp.mSynthResultIndexTexture != null)
                  rp.mSynthResultIndexTexture.Dispose();

               rp.mSynthResultIndexTexture = createLevelIndexMap(m_Query, mIndexLevelToGrab);
            }
         }

         // retrieve level data
         Vector4 m_IndexMapTexCoordTransform = new Vector4();
         retrieveLevelIndexMap(m_Query, mIndexLevelToGrab, rp.mSynthResultIndexTexture, ref m_IndexMapTexCoordTransform);
      }
      Texture createLevelIndexMap(SynthesisQuery m_Query, int l)
      {
         Texture tex;
         // . retrieve level dimensions
         int w = m_Query.m_Params.mControlPacket.m_Windows.requested(l).width();
         int h = m_Query.m_Params.mControlPacket.m_Windows.requested(l).height();

         // . create
         tex = new Texture(BRenderDevice.getDevice(), w, h, 1, Usage.RenderTarget, Format.A16B16G16R16F, Pool.Default);

         return (tex);
      }
      void retrieveLevelIndexMap(SynthesisQuery m_Query, int l, Texture tex, ref Vector4 _textransform)
      {
         // . protect render target
         D3DProtectRenderTarget locktarget = new D3DProtectRenderTarget(true);
         bind(m_Query);

         // . set requested level
         int prevlvl = m_Query.m_iLevel;
         m_Query.setLevel(l);

         // . check texture dimensions
         SurfaceDescription desc = tex.GetLevelDescription(0);
         int wsrc = m_Query.m_Params.mControlPacket.m_Windows.requested(l).width();
         int hsrc = m_Query.m_Params.mControlPacket.m_Windows.requested(l).height();
         int wdst = wsrc;
         int hdst = hsrc;

         if (wdst != desc.Width || hdst != desc.Height)
            throw new Exception("SynthesisQuery::retrieveLevelIndexMap - texture does not have correct dimensions. Please create the texture with SynthesisQuery::createLevelIndexMap");

         Vector4 wpos = windowAsVec4(m_Query.m_Params.mControlPacket.m_Windows.requested(m_Query.m_iLevel));


         // . unpack synthesized data
         unpackRegion(
           m_Query,
           Window.LTWH(
           (int)0,
           (int)0,
           (int)wpos.Z,
           (int)wpos.W), false, true, true);

         // . set render target
         Surface srf = tex.GetSurfaceLevel(0);
         BRenderDevice.getDevice().SetRenderTarget(0, srf);

         // . set shader parameters
         s_fxRndrLvl.SetValue(s_fxSynthesisBuffer, m_Query.m_WorkBuffer.front().texture());

         Globals.Assert(wdst == wpos.Z && hdst == wpos.W);

         Vector4 vsrc = new Vector4(0, 0, (float)wsrc, (float)hsrc);
         s_fxRndrLvl.SetValue(s_fxSrcWindow, vsrc);
         Vector4 vdst = new Vector4(0, 0, (float)wdst, (float)hdst);
         s_fxRndrLvl.SetValue(s_fxDstWindow, vdst);
         Vector4 vbufsz = new Vector4(
           (float)m_Query.m_WorkBuffer.size(), (float)m_Query.m_WorkBuffer.size(),
           1.0f / (float)m_Query.m_WorkBuffer.size(), 1.0f / (float)m_Query.m_WorkBuffer.size());
         s_fxRndrLvl.SetValue(s_fxWorkBufferSize, vbufsz);

         Viewport vp = new Viewport();
         vp.X = 0;
         vp.Y = 0;
         vp.Width = wdst;
         vp.Height = hdst;
         vp.MinZ = 0;
         vp.MaxZ = 1;
         BRenderDevice.getDevice().Viewport = vp;


         s_fxRndrLvl.Technique = s_fxRndrLvlNoIndirection;

         s_fxRndrLvl.CommitChanges();

         // render
        // BRenderDevice.getDevice().BeginScene();

         s_fxRndrLvl.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
         s_fxRndrLvl.BeginPass(0);
         quad().render();
         s_fxRndrLvl.EndPass();
         s_fxRndrLvl.End();
       //  BRenderDevice.getDevice().EndScene();


         // . compute transform of texture coordinates
         {
            _textransform.X = 0.0f;
            _textransform.Y = 0.0f;
            _textransform.Z = 1.0f;
            _textransform.W = 1.0f;
         }

         // . restore level
         m_Query.setLevel(prevlvl);

         srf.Dispose();
         srf = null;
         unbind(m_Query);
         locktarget.destroy();
         locktarget = null;
      }
      bool isTextureSuitableToRetrieveLevelIndexMap(SynthesisQuery m_Query, int l, Texture tex)
      {
         // -> check texture dimensions and format
         SurfaceDescription desc = tex.GetLevelDescription(0);
         if (desc.Format != Format.G16R16F && desc.Format != Format.A16B16G16R16F)
            return (false);
         int w = m_Query.m_Params.mControlPacket.m_Windows.requested(l).width();
         int h = m_Query.m_Params.mControlPacket.m_Windows.requested(l).height();
        
         if (w != desc.Width || h != desc.Height)
            return (false);
         else
            return (true);
      }
      #endregion



      public void unpackRegion(SynthesisQuery q, Window rgn, bool clear, bool settarget,  bool protect)
      {
        int l=q.level();
        Globals.Assert(l >= 0);

        // . Level buffer
        single_buffer level_buffer = (q.getBuffer(l));
        
        unpackRegion(q,rgn,level_buffer,q.m_WorkBuffer.front(),clear,settarget,protect);
      }
      public void unpackRegion(SynthesisQuery q, Window rgn,  single_buffer src, single_buffer dst, bool clear, bool settarget,  bool protect)
      {
        D3DProtectRenderTarget protectrt = new D3DProtectRenderTarget(protect);

        int l=q.level();
        Globals.Assert(l >= 0);

        // . Level window
        Window window = q.m_Params.mControlPacket.m_Windows.bufferrgn(l);

        // . Window pos
        Vector4 wpos=windowAsVec4(window);
        m_fxPixpack.SetValue(m_fxPixpackWindowPos, wpos);

        // . Buffer quad
        Vector4 qsz=bufferSize(src.size());
        m_fxPixpack.SetValue(m_fxPixpackBufferQuad, qsz);

        // . Previous step texture
        m_fxPixpack.SetValue(m_fxPixpackBuffer,src.texture());

        // . Target scratch buffer
        if (settarget)
           BRenderDevice.getDevice().SetRenderTarget(0, dst.surface());

        
        
        // . Viewport so that unpacked region is at maped at the correct location within the scratch buffer
        Viewport vp = new Viewport();

        vp.X      = rgn.left();
        vp.Y      = rgn.top();
        vp.Width  = rgn.width();
        vp.Height = rgn.height();
        vp.MinZ   = 0;
        vp.MaxZ   = 1;
        clampViewport(ref vp);
        BRenderDevice.getDevice().Viewport = vp;

        if (clear || Globals.TEST_PADDING)
           BRenderDevice.getDevice().Clear(ClearFlags.Target, Color.White, 0, 0);


        // . Region to unpack
        Pair<Vector4,Vector4> vwin=q.requestWindow();
        Vector4 tsz = new Vector4(
          (float)rgn.left()+vwin.second.X,
          (float)rgn.top() +vwin.second.Y,
          (float)rgn.width(),
          (float)rgn.height());
        m_fxPixpack.SetValue(m_fxPixpackRegion,tsz);

        // . Apply shader
        m_fxPixpack.Technique = m_fxPixunpackTech;
   //     m_fxPixpack.CommitChanges();

      //  BRenderDevice.getDevice().BeginScene();
        {
          m_fxPixpack.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
          m_fxPixpack.BeginPass(0);
          m_QuadDual.render();
          m_fxPixpack.EndPass();
          m_fxPixpack.End();

         // BRenderDevice.getDevice().EndScene();
        }

     }

      #endregion
     // -----------------------------------------------------
      

     

      #region MEMBERS

      // K-search
      int m_iKSearch;

      // Periodic offset
      float m_fPeriodX;
      float m_fPeriodY;
      bool m_bUsePeriod;


      // Permutation table
      Texture m_d3dPermutationTable;

      // Pixpack offsets texture
      Texture m_d3dPixpackOffsets;

      // Flags with whom the programs where compiled. Updated by bind(query).
      SynthesisFlags m_CompiledFlags;

      // Saved depth-stencil surface
      Surface m_SavedDepthStencilSurface;
      // Quad for rendering
      D3DQuad m_QuadDual;
      D3DQuad m_Quad;

      //Moved from SynthQueary
      // synthesis internal parameters
      public int[][][] m_SubpassesOrderingTbl;//[2][4][2];
      public int[][] m_SubpassesShrinkTbl;//[9][2];

      void initSubpassTables()
      {

         m_SubpassesOrderingTbl = new int[2][][];
         for (int i = 0; i < 2; i++)
         {
            m_SubpassesOrderingTbl[i] = new int[4][];
            for (int k = 0; k < 4; k++)
               m_SubpassesOrderingTbl[i][k] = new int[2];
         }


         m_SubpassesOrderingTbl[0][0][0] = 0;
         m_SubpassesOrderingTbl[0][0][1] = 0;
         m_SubpassesOrderingTbl[0][1][0] = 1;
         m_SubpassesOrderingTbl[0][1][1] = 1;
         m_SubpassesOrderingTbl[0][2][0] = 0;
         m_SubpassesOrderingTbl[0][2][1] = 1;
         m_SubpassesOrderingTbl[0][3][0] = 1;
         m_SubpassesOrderingTbl[0][3][1] = 0;
         m_SubpassesOrderingTbl[1][0][0] = 1;
         m_SubpassesOrderingTbl[1][0][1] = 1;
         m_SubpassesOrderingTbl[1][1][0] = 0;
         m_SubpassesOrderingTbl[1][1][1] = 0;
         m_SubpassesOrderingTbl[1][2][0] = 0;
         m_SubpassesOrderingTbl[1][2][1] = 1;
         m_SubpassesOrderingTbl[1][3][0] = 1;
         m_SubpassesOrderingTbl[1][3][1] = 0;

         m_SubpassesShrinkTbl = new int[9][];
         for (int i = 0; i < 9; i++)
         {
            m_SubpassesShrinkTbl[i] = new int[2];
         }

         m_SubpassesShrinkTbl[0][0] = 2;
         m_SubpassesShrinkTbl[0][1] = 2;
         m_SubpassesShrinkTbl[1][0] = 1;
         m_SubpassesShrinkTbl[1][1] = 1;
         m_SubpassesShrinkTbl[2][0] = 1;
         m_SubpassesShrinkTbl[2][1] = 2;
         m_SubpassesShrinkTbl[3][0] = 1;
         m_SubpassesShrinkTbl[3][1] = 1;
         m_SubpassesShrinkTbl[4][0] = 2;
         m_SubpassesShrinkTbl[4][1] = 1;
         m_SubpassesShrinkTbl[5][0] = 1;
         m_SubpassesShrinkTbl[5][1] = 1;
         m_SubpassesShrinkTbl[6][0] = 2;
         m_SubpassesShrinkTbl[6][1] = 1;
         m_SubpassesShrinkTbl[7][0] = 1;
         m_SubpassesShrinkTbl[7][1] = 1;
         m_SubpassesShrinkTbl[8][0] = 0;
         m_SubpassesShrinkTbl[8][1] = 0;

      }

#region EFFECTHANDLES
      // Initialization shader
      Microsoft.DirectX.Direct3D.Effect m_fxInit;
      EffectHandle m_fxInitTech;
      EffectHandle m_fxInitTechForce;
      EffectHandle m_fxInitExemplarSize;
      EffectHandle m_fxInitScale;
      EffectHandle m_fxInitWindowPos;
      EffectHandle m_fxInitLevelSize;
      EffectHandle m_fxInitPermutationTable;
      EffectHandle m_fxInitLocalFramesRes;
      EffectHandle m_fxInitLocalFrames;
      EffectHandle m_fxInitApprExemplar;

      // Upsampling shader
      Microsoft.DirectX.Direct3D.Effect m_fxUpsmpl;
      EffectHandle m_fxUpsmplTech;
      EffectHandle m_fxUpsmplJitterTech;
      EffectHandle m_fxUpsmplExemplarCstr;
      EffectHandle m_fxUpsmplCstrMap;
      EffectHandle m_fxUpsmplPrevious;
      EffectHandle m_fxUpsmplExemplarSize;
      EffectHandle m_fxUpsmplRandomness;
      EffectHandle m_fxUpsmplScale;
      EffectHandle m_fxUpsmplTime;
      EffectHandle m_fxUpsmplCstrThreshold;
      EffectHandle m_fxUpsmplCstrMapScale;
      EffectHandle m_fxUpsmplNeighOffset;
      EffectHandle m_fxUpsmplWindowPos;
      EffectHandle m_fxUpsmplParentWindowPos;
      EffectHandle m_fxUpsmplParentQuadSize;
      EffectHandle m_fxUpsmplPositionMap0;
      EffectHandle m_fxUpsmplPositionMap1;
      EffectHandle m_fxUpsmplPositionMapScale;
      EffectHandle m_fxUpsmplPositionData;
      EffectHandle m_fxUpsmplPixPackIndex;
      EffectHandle m_fxUpsmplExRndMap;
      EffectHandle m_fxUpsmplPermutationTable;
      EffectHandle m_fxUpsmplIsLevel0;
      EffectHandle m_fxUpsmplLevelId;
      EffectHandle m_fxUpsmplLocalFrames;
      EffectHandle m_fxUpsmplLocalFramesRes;
      EffectHandle m_fxUpsmplLocalFramesInverse;
      EffectHandle m_fxUpsmplCutAvoidance;
      EffectHandle m_fxUpsmplCurrent_T_1;
      EffectHandle m_fxUpsmplCurrent_T_1_QuadSize;
      EffectHandle m_fxUpsmplIsFirstAdvected;
      EffectHandle m_fxUpsmplVectorField;

      // Correction shader
      Microsoft.DirectX.Direct3D.Effect m_fxCorrection;
      EffectHandle m_fxCorrTech;
      EffectHandle m_fxCorrForceTech;
      EffectHandle m_fxCorrFlagChartsTech;
      
      EffectHandle m_fxCorrPrevious;
      EffectHandle m_fxCorrExemplar;
      EffectHandle m_fxCorrSimilaritySet;
      EffectHandle m_fxCorrFeatureDistanceMap;
      EffectHandle m_fxCorrQuadrantIndex;
      EffectHandle m_fxCorrPassId;
      EffectHandle m_fxCorrExemplarSize;
      EffectHandle m_fxCorrNeighOffset;
      EffectHandle m_fxCorrWindowPos;
      EffectHandle m_fxCorrPrevQuad;
      EffectHandle m_fxCorrCoherence;
      EffectHandle m_fxCorrIsLevel0;
      EffectHandle m_fxCorrLevelId;
      EffectHandle m_fxCorrTime;
      EffectHandle m_fxCorrPixPackOffsets;
      EffectHandle m_fxCorrTexPixPackOffsets;
      EffectHandle m_fxCorrRelativeWindowPos;
      EffectHandle m_fxCorrLocalFrames;
      EffectHandle m_fxCorrLocalFramesInverse;
      EffectHandle m_fxCorrLocalFramesRes;
      EffectHandle m_fxCorrCutAvoidance;
      EffectHandle[] m_fxCorrEv = new EffectHandle[8];
      EffectHandle m_fxCorrTrEv0123;
      EffectHandle m_fxCorrTrEv4567;
      EffectHandle m_fxCorrRecolored_0_3;
      
      EffectHandle m_fxCorrUnqRecolored_Scale_0_3;
      EffectHandle m_fxCorrUnqRecolored_Mean_0_3;
      
      EffectHandle m_fxCorrNeighborhoods_0_3;
      
      EffectHandle m_fxCorrUnqNeighborhoods_Scale_0_3;
      EffectHandle m_fxCorrUnqNeighborhoods_Mean_0_3;
      EffectHandle m_fxCorrIndirection;
      EffectHandle m_fxCorrIndirectionTexPos;
      EffectHandle m_fxCorrNeighOffsetLSynth;
      EffectHandle m_fxCorrQuadZ;
      EffectHandle m_fxCorrScale;

      // Pixel Packing
      Microsoft.DirectX.Direct3D.Effect m_fxPixpack;
      EffectHandle m_fxPixpackTech;
      EffectHandle m_fxPixunpackTech;
      EffectHandle m_fxPixpackBuffer;
      EffectHandle m_fxPixpackBufferQuad;
      EffectHandle m_fxPixpackWindowPos;
      EffectHandle m_fxPixpackRegion;

      // Copy shader
      Microsoft.DirectX.Direct3D.Effect m_fxCopy;
      EffectHandle m_fxCopyTech;
      EffectHandle m_fxCopyTex;
      EffectHandle m_fxCopySrcWindow;
      EffectHandle m_fxCopyDstWindow;
      EffectHandle m_fxCopySrcBufferSize;
      EffectHandle m_fxCopyDstBufferSize;

      // Display shader
      Microsoft.DirectX.Direct3D.Effect m_fxDisplay;
      EffectHandle m_fxDispTechSimple;
      EffectHandle m_fxDispTechMagnify;
      EffectHandle m_fxDispTechColor;
      EffectHandle m_fxDispTechPatches;
      EffectHandle m_fxDispTechIndices;
      EffectHandle m_fxDispTechColorNoVP;
      EffectHandle m_fxDispTechPatchesNoVP;
      EffectHandle m_fxDispTechIndicesNoVP;
      EffectHandle m_fxDispIndexMap;
      EffectHandle m_fxDispExemplar;
      EffectHandle m_fxDispTrl;
      EffectHandle m_fxDispScl;
      EffectHandle m_fxDispRes;
      EffectHandle m_fxDispIndexMapRes;
      EffectHandle m_fxDispExemplarRes;
      EffectHandle m_fxDispExemplarHighRes;
      EffectHandle m_fxDispInvZoomFactor;
      EffectHandle m_fxDispNumExHighresLevels;
      EffectHandle m_fxDispLocalFrames;

      //Index buffer retrevial shader
      Effect s_fxRndrLvl = null;
      EffectHandle s_fxRndrLvlNoIndirection;
      EffectHandle s_fxRndrLvlIndirection;
      EffectHandle s_fxRndrApplyIndirection;
      EffectHandle s_fxRndrApplyPackedIndirection;
      EffectHandle s_fxSynthesisBuffer;
      EffectHandle s_fxSrcWindow;
      EffectHandle s_fxDstWindow;
      EffectHandle s_fxWorkBufferSize;
      EffectHandle s_fxIndirectionMap;
      EffectHandle s_fxSkipBorderTrsfrm;
#endregion

      public D3DQuad quad()
      {
         return m_QuadDual;
      }


      public void setExemplarPeriod(Exemplar e, bool enable, float px, float py)
      {
         m_fPeriodX = px;
         m_fPeriodY = py;
         m_bUsePeriod = enable;
         loadPrograms(e,Synthesiser.SynthesisFlags._Undef);
      }




      #endregion

     }
}