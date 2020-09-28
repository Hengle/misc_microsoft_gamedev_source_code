/*===================================
  Neighborhood.cs
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
   // ----------------------------------------------
   // class storing exemplar levels data
   #region LEVELDATA
   class LevelDataD3DPacket
   {
      #region MEMBERS
      // similarity set (k=0, k=1 in a single RGBA texture)
      public Texture m_d3dSimilaritySet;

      // recolored exemplar
      public Texture m_d3dRecolored_0_3;

      public List<float> m_UnqRecolored_Scale;
      public List<float> m_UnqRecolored_Mean;

      // synthesis neighborhoods
      public Texture m_d3dNeighborhoods_0_3;

      public List<float> m_UnqNeighborhoods_Scale;
      public List<float> m_UnqNeighborhoods_Mean;

      // exemplar raint
      public Texture m_d3dExemplarConstraint;
      #endregion

      public LevelDataD3DPacket(Exemplar a, int l)
      {
         m_d3dSimilaritySet = null;
         m_d3dRecolored_0_3 = null;
         m_d3dNeighborhoods_0_3 = null;
         m_d3dExemplarConstraint = null;

         recoloredExemplarToD3DTexture(a, l);
         synthesisNeighborhoodsToD3DTexture(a, l);
         similaritySetsToD3DTexture(a, l);

      }
      ~LevelDataD3DPacket()
      {
         m_UnqNeighborhoods_Scale.Clear();
         m_UnqNeighborhoods_Mean.Clear();
         m_UnqRecolored_Scale.Clear();
         m_UnqRecolored_Mean.Clear();

         if (m_d3dSimilaritySet != null)
         {
            m_d3dSimilaritySet.Dispose();
            m_d3dSimilaritySet = null;
         }
         if (m_d3dRecolored_0_3 != null)
         {
            m_d3dRecolored_0_3.Dispose();
            m_d3dRecolored_0_3 = null;
         }
        
         if (m_d3dNeighborhoods_0_3 != null)
         {
            m_d3dNeighborhoods_0_3.Dispose();
            m_d3dNeighborhoods_0_3 = null;
         }
        
         if (m_d3dExemplarConstraint != null)
         {
            m_d3dExemplarConstraint.Dispose();
            m_d3dExemplarConstraint = null;
         }
      }

      int memoryUsed()
      {

         SurfaceDescription desc;

         int memsz = 0;
         desc = m_d3dRecolored_0_3.GetLevelDescription(0);
         memsz += desc.Height * desc.Width * 4;

         desc = m_d3dNeighborhoods_0_3.GetLevelDescription(0);
         memsz += desc.Height * desc.Width * 4;

         desc = m_d3dSimilaritySet.GetLevelDescription(0);
         memsz += desc.Height * desc.Width * 4;
         return (memsz);
      }

      //CLM these just generate D3D Texture versions of the KNS  & Recolor data
      //we should be able to remove this class all together
      unsafe void recoloredExemplarToD3DTexture(Exemplar a, int l)
      {

         // recolored exemplar
         // . quantize !
         Quantizer q = new Quantizer(a.recoloredStack(l), Globals.QUANTIZE_NUM_BITS, Globals.QUANTIZE_PERCENT_INSIDE);

         m_d3dRecolored_0_3 = new Texture(BRenderDevice.getDevice(),
            a.recoloredStack(l).width(),
            a.recoloredStack(l).height(), 1, 0, Format.A8R8G8B8, Pool.Managed);


         // . fill
         GraphicsStream texstream = m_d3dRecolored_0_3.LockRectangle(0, LockFlags.None);
         byte* data = (byte*)texstream.InternalDataPointer;
         int rectPitch = a.recoloredStack(l).width() * 4;
         for (int j = 0; j < a.recoloredStack(l).height(); j++)
         {
            for (int i = 0; i < a.recoloredStack(l).width(); i++)
            {
               int v0 = q.quantized().get(i, j, 0);
               int v1 = q.quantized().get(i, j, 1);
               int v2 = q.quantized().get(i, j, 2);
               int v3 = q.quantized().get(i, j, 3);
               Globals.Assert(v0 >= 0 && v0 <= 255);
               Globals.Assert(v1 >= 0 && v1 <= 255);
               Globals.Assert(v2 >= 0 && v2 <= 255);
               Globals.Assert(v3 >= 0 && v3 <= 255);
               data[i * 4 + j * rectPitch + 2] = (byte)(v0);
               data[i * 4 + j * rectPitch + 1] = (byte)(v1);
               data[i * 4 + j * rectPitch + 0] = (byte)(v2);
               data[i * 4 + j * rectPitch + 3] = (byte)(v3);
            }
         }
         m_d3dRecolored_0_3.UnlockRectangle(0);



         // de-quantization parameters

         m_UnqRecolored_Scale = new List<float>(Globals.NUM_RUNTIME_PCA_COMPONENTS);
         m_UnqRecolored_Mean = new List<float>(Globals.NUM_RUNTIME_PCA_COMPONENTS);

         for (int c = 0; c < Globals.NUM_RUNTIME_PCA_COMPONENTS; c++)
         {
            m_UnqRecolored_Scale.Add(q.radius(c));
            m_UnqRecolored_Mean.Add(q.center(c));
         }


         // . unquantize, recolored exemplar
         qr_mean_0_3 = new Vector4(m_UnqRecolored_Mean[0],m_UnqRecolored_Mean[1],m_UnqRecolored_Mean[2],m_UnqRecolored_Mean[3]);
        qr_scale_0_3 = new Vector4(m_UnqRecolored_Scale[0],m_UnqRecolored_Scale[1],m_UnqRecolored_Scale[2],m_UnqRecolored_Scale[3]);

         Vector4 div3 = new Vector4(0.33333f, 0.3333f, 0.3333f, 0.3333f);
         Vector4 mul2 = new Vector4(2, 2, 2, 2);
         qr_mean_0_3 = qr_mean_0_3 - qr_scale_0_3;
         qr_scale_0_3 = Vector4.Multiply(Vector4.Multiply(qr_scale_0_3, 1 / 3.0f), 2);
      }
      unsafe void synthesisNeighborhoodsToD3DTexture(Exemplar a, int l)
      {

         // synthesis neighborhoods
         // . build float multidim texture from projected neighborhoods
         MultiDimFloatTexture tex = new MultiDimFloatTexture( a.recoloredStack(l).width(), a.recoloredStack(l).height(),  Globals.NUM_RUNTIME_PCA_COMPONENTS);

         for (int j = 0; j < tex.height(); j++)
         {
            for (int i = 0; i < tex.width(); i++)
            {
               NeighborhoodSynth nproj = a.getProjectedSynthNeighborhood(l, i, j);
               for (int c = 0; c < tex.numComp(); c++)
                  tex.set(nproj.getValue(c), i, j, c);
            }
         }

         // . quantize
         Quantizer q = new Quantizer(tex, Globals.QUANTIZE_NUM_BITS, Globals.QUANTIZE_PERCENT_INSIDE);

         m_d3dNeighborhoods_0_3 = new Texture(BRenderDevice.getDevice(),tex.width(), tex.height(), 1, 0, Format.A8R8G8B8, Pool.Managed);



         // . fill
         GraphicsStream texstream = m_d3dNeighborhoods_0_3.LockRectangle(0, LockFlags.None);
         byte* data = (byte*)texstream.InternalDataPointer;
         int rectPitch = tex.width() * 4;
         for (int j = 0; j < tex.height(); j++)
         {
            for (int i = 0; i < tex.width(); i++)
            {
               int v0 = q.quantized().get(i, j, 0);
               int v1 = q.quantized().get(i, j, 1);
               int v2 = q.quantized().get(i, j, 2);
               int v3 = q.quantized().get(i, j, 3);
               Globals.Assert(v0 >= 0 && v0 <= 255);
               Globals.Assert(v1 >= 0 && v1 <= 255);
               Globals.Assert(v2 >= 0 && v2 <= 255);
               Globals.Assert(v3 >= 0 && v3 <= 255);
               data[i * 4 + j * rectPitch + 2] = (byte)(v0);
               data[i * 4 + j * rectPitch + 1] = (byte)(v1);
               data[i * 4 + j * rectPitch + 0] = (byte)(v2);
               data[i * 4 + j * rectPitch + 3] = (byte)(v3);

            }
         }
         m_d3dNeighborhoods_0_3.UnlockRectangle(0);

        
         // de-quantization parameters

         m_UnqNeighborhoods_Scale = new List<float>(8);
         m_UnqNeighborhoods_Mean = new List<float>(8);

         for (int c = 0; c < q.quantized().numComp(); c++)
         {
            m_UnqNeighborhoods_Scale.Add(q.radius(c));
            m_UnqNeighborhoods_Mean.Add(q.center(c));
         }
         qn_mean_0_3 = new Vector4(m_UnqNeighborhoods_Mean[0],m_UnqNeighborhoods_Mean[1],m_UnqNeighborhoods_Mean[2],m_UnqNeighborhoods_Mean[3]);
         qn_scale_0_3 = new Vector4(m_UnqNeighborhoods_Scale[0],m_UnqNeighborhoods_Scale[1],m_UnqNeighborhoods_Scale[2],m_UnqNeighborhoods_Scale[3]);
        
         // expression in the shader is 
         // (v*2.0-1.0)*UnqNeighborhoods_Scale_0_3 + UnqNeighborhoods_Mean_0_3
         // => this is baked into the ants to reduce work load
         qn_mean_0_3 = qn_mean_0_3 - qn_scale_0_3;
         qn_scale_0_3 = qn_scale_0_3 * 2.0f;

      }
      unsafe void similaritySetsToD3DTexture(Exemplar a, int l)
      {

         // Indices
         {
            // . create texture
            m_d3dSimilaritySet = new Texture(BRenderDevice.getDevice(),
                  a.stack(l).getWidth(),
                  a.stack(l).getHeight(), 1, 0, Format.A8R8G8B8, Pool.Managed);


            // . fill in 
            GraphicsStream texstream = m_d3dSimilaritySet.LockRectangle(0, LockFlags.None);
            byte* data = (byte*)texstream.InternalDataPointer;
            int rectPitch = a.stack(l).getWidth() * 4;

            byte* dataI = data;// (byte*)rectI.pBits;
            int pitch = rectPitch;
            int[] c_ki = new int[2] { 2, 0 };
            int[] c_kj = new int[2] { 1, 3 };

            int kn = Math.Min(2, Globals.K_NEAREST); // TODO: extend to ohter values of K ?
            Globals.Assert(kn <= 2);
            int width = a.stack(l).getWidth();
            int exsz = width;

            for (int j = 0; j < width; j++)
            {
               for (int i = 0; i < width; i++)
               {
                  int n = a.similaritySetSize(l, i, j);

                  for (int k = 0; k < kn; k++)
                  {
                     if (k < n)
                     {
                        Pair<int, int> kij = a.similarPixel(l, i, j, k);
                        int vi = kij.first;
                        int vj = kij.second;

                        //dataI[i*4+j*pitch  + c_ki[k]]=vi & 255;            // simsets as indirections
                        //dataI[i*4+j*pitch  + c_kj[k]]=vj & 255;

                        Globals.Assert(Math.Abs(vi - i) < exsz);
                        Globals.Assert(Math.Abs(vj - j) < exsz);

                        int oi = (vi - i) + 128;
                        int oj = (vj - j) + 128;

                        Globals.Assert(oi >= 0 && oi <= 255);
                        Globals.Assert(oj >= 0 && oj <= 255);

                        dataI[i * 4 + j * pitch + c_ki[k]] = (byte)(oi & 255); // simsets as offsets
                        dataI[i * 4 + j * pitch + c_kj[k]] = (byte)(oj & 255);
                     }
                     else
                     {
                        dataI[i * 4 + j * pitch + c_ki[k]] = 128;
                        dataI[i * 4 + j * pitch + c_kj[k]] = 128;
                     }
                  }
               }
            }
            m_d3dSimilaritySet.UnlockRectangle(0);
         }
      }
      
      // . unquantize, recolored exemplar
      public Vector4 qr_mean_0_3 = new Vector4();
      public Vector4 qr_scale_0_3 = new Vector4();
      public Vector4 qn_mean_0_3 = new Vector4();
      public Vector4 qn_scale_0_3 = new Vector4();

   };
   #endregion
}