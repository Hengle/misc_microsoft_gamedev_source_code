/*===================================
  D3DHelperClasses.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis  (MGS Ensemble studios) [12.01.06]
===================================*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Diagnostics;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.IO;
using System.Diagnostics;
using Rendering;
using EditorCore;

namespace TextureSynthesis
{


   class D3DProtectRenderTarget
   {
     
  
     Surface      m_d3dTarget;
     Viewport     m_d3dVP;
     bool         m_bEnabled=true;


     public D3DProtectRenderTarget(bool enabled)//=true
     {
       m_bEnabled=enabled;
       if (m_bEnabled)
       {
         m_d3dVP = BRenderDevice.getDevice().Viewport;
         m_d3dTarget = BRenderDevice.getDevice().GetRenderTarget(0);
       }
     }

      public void destroy()
      {
         if (m_bEnabled && BRenderDevice.getDevice()!=null)
         {
            if (m_d3dTarget!=null)
            {
               BRenderDevice.getDevice().SetRenderTarget(0, m_d3dTarget);
               m_d3dTarget.Dispose();
               m_d3dTarget = null;
            }
            
            BRenderDevice.getDevice().Viewport = m_d3dVP;
            m_bEnabled = false;  
         }
      }
     ~D3DProtectRenderTarget()
     {
        destroy();
     }
   }
   //-------------------------------------------------
   class FxImageProcessing
   {
      public Microsoft.DirectX.Direct3D.Effect mShader = null;
      D3DQuad s_Quad = null;

      ~FxImageProcessing()
      {
         s_Quad = null;
         mShader.Dispose();
      }
      EffectHandle m_fxTechMain;
      EffectHandle m_fxViewport;
      EffectHandle m_fxDestRegionCoord;
      EffectHandle m_fxDestRegionSize;
      EffectHandle m_fxInvDestRegionSize;


      public void init(String filename)
      {
         mShader = ShaderManager.loadShader(filename, null);

         
         s_Quad = new D3DQuad(false);



         // call FxShader constructor

         mShader = ShaderManager.loadShader(filename, null);

         // init default parameters

         m_fxViewport = mShader.GetParameter(null, "Viewport");
         m_fxDestRegionCoord = mShader.GetParameter(null, "DestRegionCoord");
         m_fxDestRegionSize = mShader.GetParameter(null, "DestRegionSize");
         m_fxInvDestRegionSize = mShader.GetParameter(null, "InvDestRegionSize");
         m_fxTechMain = mShader.GetTechnique("t_main");

         mShader.Technique = m_fxTechMain;
      }

      public void renderFull(int w, int h)
      {

         Vector4 v = new Vector4(0, 0, (float)w, (float)h);
         mShader.SetValue(m_fxViewport, v);
         Vector4 v2 = new Vector4(0, 0, 0, 0);
         mShader.SetValue(m_fxDestRegionCoord, v2);
         Vector4 v3 = new Vector4((float)w, (float)h, 0, 0);
         mShader.SetValue(m_fxDestRegionSize, v3);
         Vector4 v4 = new Vector4(1.0f / (float)w, 1.0f / (float)h, 0, 0);
         mShader.SetValue(m_fxInvDestRegionSize, v4);

         Viewport vp = new Viewport();
         vp.X = 0;
         vp.Y = 0;
         vp.Width = w;
         vp.Height = h;
         vp.MinZ = 0;
         vp.MaxZ = 1;
         BRenderDevice.getDevice().Viewport = vp;

         mShader.CommitChanges();

         BRenderDevice.beginScene();
         mShader.Begin(FX.DoNotSaveState | FX.DoNotSaveShaderState | FX.DoNotSaveSamplerState);
         mShader.BeginPass(0);
         s_Quad.render();
         mShader.EndPass();
         mShader.End();
         BRenderDevice.endScene();

      }

   }

   //-------------------------------------------------
   class D3DGenMipMapPyramid
   {

      protected Texture m_d3dInputTexture;
      protected FxImageProcessing m_fxParentFromChildren = new FxImageProcessing();
      protected EffectHandle m_fxChildrenTexture;
      protected List<Texture> m_Pyramid = new List<Texture>();
      protected bool m_bReleaseBindedTexture=false;


      public D3DGenMipMapPyramid(String shader, Texture tex)
      {
         m_d3dInputTexture = tex;
         // . load shader

         m_fxParentFromChildren.init(shader);
         m_fxChildrenTexture  = m_fxParentFromChildren.mShader.GetParameter(null,"ChildrenTexture");
         // . create pyramid
         SurfaceDescription desc=tex.GetLevelDescription(0);
         int w = (desc.Width >> 1);
         int h = (desc.Height >> 1);
         while (w > 0 || h > 0) 
         {
            m_Pyramid.Add(new Texture(BRenderDevice.getDevice(),Math.Max(1, w), Math.Max(1, h), 1,Usage.RenderTarget,desc.Format,Pool.Default));
            w=w >> 1;
            h=h >> 1;
         }
         m_bReleaseBindedTexture=false;
         // . init pyramid
         refresh();
      }
      ~D3DGenMipMapPyramid()
      {
         m_fxParentFromChildren=null;
         for (int l = 0; l < m_Pyramid.Count; l++)
            m_Pyramid[l] = null;
         if (m_bReleaseBindedTexture)
         {
            m_d3dInputTexture.Dispose();
            m_d3dInputTexture = null;
         }
      }


      void refresh()
      {
         
         for (int l = 1; l < numLevels(); l++)
         {
            D3DProtectRenderTarget protect = new D3DProtectRenderTarget(true);
            Surface srf = pyramidLevel(l).GetSurfaceLevel(0);
            BRenderDevice.getDevice().SetRenderTarget(0, srf);
            m_fxParentFromChildren.mShader.SetValue(m_fxChildrenTexture, pyramidLevel(l - 1));

            SurfaceDescription desc = pyramidLevel(l).GetLevelDescription(0);

            m_fxParentFromChildren.renderFull(desc.Width, desc.Height);

            protect.destroy();
            protect = null;
            srf = null;
         }
      }


      public void bind(Texture tex)
      {
         SurfaceDescription desc_tex = tex.GetLevelDescription(0);
         SurfaceDescription desc_current = m_d3dInputTexture.GetLevelDescription(0);

         if (!(desc_tex.Width == desc_current.Width && desc_tex.Height == desc_current.Height))
         {
            throw new Exception("D3DGenTexturePyramid - cannot bind to a texture of different size.");
         }
         if (m_bReleaseBindedTexture)
         {
            m_d3dInputTexture = null;
         }
         m_d3dInputTexture = tex;
         refresh();
      }


      bool compatible(Texture tex)
      {
         SurfaceDescription desc_tex = tex.GetLevelDescription(0);
         SurfaceDescription desc_current = m_d3dInputTexture.GetLevelDescription(0);
         if (!(desc_tex.Width == desc_current.Width && desc_tex.Height == desc_current.Height))
         {
            return (false);
         }
         return (true);
      }

      void setReleaseBindedTexture(bool b)
      {
         m_bReleaseBindedTexture = b;
      }


      public Texture pyramidLevel(int l)
      { 
         if (l == 0) 
            return m_d3dInputTexture; 

         return (m_Pyramid[l - 1]); 
      }

      public int numLevels() 
      {
         return ((int)m_Pyramid.Count) + 1; 
      }

   };

   // ----------------------------------------------

   class D3DQuad
   {
	   protected static VertexBuffer s_QuadVBId;
	   protected static VertexBuffer s_QuadVBNotId;
     protected VertexBuffer        m_QuadVB;

    // --------------------------------------------------------------
   public D3DQuad(bool identity)
   {
     
     if (!identity)
     {
       if (s_QuadVBNotId == null)
       {
         VertexTypes.Pos_uv0_uv1[] vertices =new VertexTypes.Pos_uv0_uv1[]
         {
           new VertexTypes.Pos_uv0_uv1(  -1.0f,-1.0f,0.0f, 0.0f,0.0f, 0.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(   1.0f,-1.0f,0.0f, 1.0f,0.0f, 1.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(   1.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f ),

           new VertexTypes.Pos_uv0_uv1(  -1.0f,-1.0f,0.0f, 0.0f,0.0f, 0.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(   1.0f, 1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f ),
           new VertexTypes.Pos_uv0_uv1(  -1.0f, 1.0f,0.0f, 0.0f,1.0f, 0.0f,1.0f),
         };

          int mNumVerts = vertices.Length;

         s_QuadVBNotId = new VertexBuffer(typeof(VertexTypes.Pos_uv0_uv1), vertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_uv0_uv1.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = s_QuadVBNotId.Lock(0, 0, LockFlags.None);
         gStream.Write(vertices);
         s_QuadVBNotId.Unlock();

       }
       m_QuadVB=s_QuadVBNotId;
     }
     else
     {
       if (s_QuadVBId == null)
       {
         VertexTypes.Pos_uv0_uv1[] vertices =new VertexTypes.Pos_uv0_uv1[]
         {
           new VertexTypes.Pos_uv0_uv1(  0.0f,0.0f,0.0f, 0.0f,0.0f, 0.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(  1.0f,0.0f,0.0f, 1.0f,0.0f, 1.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(  1.0f,1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f ),

           new VertexTypes.Pos_uv0_uv1(  0.0f,0.0f,0.0f, 0.0f,0.0f, 0.0f,0.0f ),
           new VertexTypes.Pos_uv0_uv1(  1.0f,1.0f,0.0f, 1.0f,1.0f, 1.0f,1.0f ),
           new VertexTypes.Pos_uv0_uv1(  0.0f,1.0f,0.0f, 0.0f,1.0f, 0.0f,1.0f ),
         };

          int mNumVerts = vertices.Length;

          s_QuadVBId = new VertexBuffer(typeof(VertexTypes.Pos_uv0_uv1), vertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_uv0_uv1.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = s_QuadVBId.Lock(0, 0, LockFlags.None);
         gStream.Write(vertices);
         s_QuadVBId.Unlock();



       }
       m_QuadVB=s_QuadVBId;
     }
   }
   // --------------------------------------------------------------
   ~D3DQuad()
   {
      {
         if (s_QuadVBId != null)
         {
            s_QuadVBId.Dispose();
            s_QuadVBId = null;
         }
         if (s_QuadVBNotId != null)
         {
            s_QuadVBNotId.Dispose();
            s_QuadVBNotId = null;
         }
      }
   }
   // --------------------------------------------------------------
   public void render()
   {
      BRenderDevice.getDevice().SetStreamSource(0, m_QuadVB, 0);
      BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_uv0_uv1.vertDecl;
      BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, 2);	
   }
   // --------------------------------------------------------------

   };

   // ----------------------------------------------
   // class for single render target // TODO: update with class from d3d_tools.h
   class single_buffer
   {

      Texture m_d3dTexture;
      Surface m_d3dSurface;
      int m_iSize;

      public single_buffer() { m_iSize = (0); m_d3dTexture = (null); m_d3dSurface = (null); }
      ~single_buffer()
      {
         release();
      }

      public void create(int sz)
      {
         m_d3dSurface = null;
         m_d3dTexture = null;

         m_iSize = sz;
         m_d3dTexture = new Texture(BRenderDevice.getDevice(), sz, sz, 1, Usage.RenderTarget, Format.A16B16G16R16F, Pool.Default);
         m_d3dSurface = m_d3dTexture.GetSurfaceLevel(0);

      }
      public void release()
      {
         if (m_d3dTexture!=null)
         {
            m_d3dTexture.Dispose();
            m_d3dTexture = null;
         }
         if (m_d3dSurface != null)
         {
            m_d3dSurface.Dispose();
            m_d3dSurface = null;
         }
      }

      public Texture texture() { return (m_d3dTexture); }
      public Surface surface() { return (m_d3dSurface); }
      public int size() { return (m_iSize); }
      public bool ready() { return (m_d3dTexture != null); }
   };

   // ----------------------------------------------
   // class for double buffered render targets // TODO: update with class from d3d_tools.h
   class swap_buffer
   {
      single_buffer[] m_Buffer = new single_buffer[2];
      int m_iCurrent;
      int m_iSize;

      public swap_buffer() { m_iSize = (0); m_iCurrent = (0); }

      public void create(int sz)
      {
         m_iSize = sz;
         m_Buffer[0] = new single_buffer();
         m_Buffer[0].create(m_iSize);
         m_Buffer[1] = new single_buffer();
         m_Buffer[1].create(m_iSize);
      }

      public single_buffer front() { return (m_Buffer[m_iCurrent]); }
      public single_buffer back() { return (m_Buffer[(m_iCurrent + 1) % 2]); }
      public void swap() { m_iCurrent = (m_iCurrent + 1) % 2; }
      public int size() { return (m_iSize); }
   };

     

}