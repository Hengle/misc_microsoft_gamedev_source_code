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
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Rendering;
using System.IO;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using EditorCore;

namespace TextureSynthesis
{
   class PCA
   {
      int m_iNumDim = 0;               /// number of dimensions
      bool m_bReady = false;           /// flag to check wether computePCA has been called or not
      double m_dTotalVariance = 0.0;   /// total variance in dataset
      List<float> m_Mean;              /// Mean value of samples
      List<List<float>> m_EigenVects;     /// eigen vectors from PCA
      List<float> m_EigenVals;         /// eigen values from PCA                // NOTE: these are actually sqrt(eigenval) (standard deviation) // TODO: rename
      public Vector4 [][]mGroupedEigenVecs = null;

      public PCA()
      {
         m_iNumDim = 0;
         m_bReady = false;
         m_dTotalVariance = 0.0;
      }

      ~PCA()
      {
         for (int i = 0; i < m_EigenVects.Count; i++)
            m_EigenVects.Clear();
         m_Mean.Clear();
         m_EigenVals.Clear();
      }

      public bool load(StreamReader f)
      {
         m_bReady = true;
         // dimensions
         m_iNumDim = Convert.ToInt32(f.ReadLine());
         if (m_iNumDim == 0)
            return false;
         Globals.Assert(m_iNumDim > 0);
         setNumDim(m_iNumDim);

         // for backward compatibility
         int tmp = Convert.ToInt32(f.ReadLine());

         char[] seps = { ' ' };

         // mean
         String v = f.ReadLine();
         String[] splitStrings = v.Split(seps);
         for (int i = 0; i < m_iNumDim; i++)
         {
            float g = Convert.ToSingle(splitStrings[i]);
            m_Mean.Add(g);
         }


         // for backward compatibility
         float tf;
         for (int i = 0; i < tmp; i++)
            tf = Convert.ToSingle(f.ReadLine());

         // eigen values
         v = f.ReadLine();
         splitStrings = v.Split(seps);
         for (int i = 0; i < m_iNumDim; i++)
            m_EigenVals.Add(Convert.ToSingle(splitStrings[i]));

         // eigen vectors
         int nb, sz;
         nb = Convert.ToInt32(f.ReadLine());
         m_EigenVects = new List<List<float>>(nb);

         for (int e = 0; e < nb; e++)
         {
            v = f.ReadLine();
            splitStrings = v.Split(seps);

            sz = Convert.ToInt32(splitStrings[0]);
            Globals.Assert(sz == m_iNumDim);

            m_EigenVects.Add(new List<float>(sz));

            for (int i = 0; i < sz; i++)
               m_EigenVects[e].Add(Convert.ToSingle(splitStrings[1 + i]));

         }
         eigenVectorsToD3D();
         return (true);
      }
      // -------------------------------------------
      /// set number of dimensions
      void setNumDim(int n)
      {
         m_iNumDim = n;
         m_dTotalVariance = 0.0;
         m_EigenVals = new List<float>(n);
         m_Mean = new List<float>(n);
      }

      int getNumDim() { return (m_iNumDim); }


      
     // -------------------------------------------
     /// projects a sample on n first eigen vectors
      public void project(NeighborhoodSynth smp, int n, ref NeighborhoodSynth ret)
     {
       Globals.Assert(n <= m_EigenVects.Count);
       for (int d=0;d<m_iNumDim;d++)
         ret.setValue(d,0.0f);
       for (int i=0;i<n;i++) 
       {
         Globals.Assert(m_EigenVects[i].Count == m_iNumDim);
         float proj=0.0f;
         for (int j=0;j<m_iNumDim;j++)
           proj+=m_EigenVects[i][j]*smp.getValue(j);
         ret.setValue(i,proj);
       }
     }

      public float    getEigenVect(int v,int c) 
      {
        // checkAccess(0); 
         Globals.Assert(v < (int)m_EigenVects.Count && c < (int)m_EigenVects[v].Count); 
         return (m_EigenVects[v][c]);
      }
      void eigenVectorsToD3D()
      {
         mGroupedEigenVecs = new Vector4[Globals.NUM_RUNTIME_PCA_COMPONENTS][];
         for (int v = 0; v < Globals.NUM_RUNTIME_PCA_COMPONENTS; v++)
         {
            mGroupedEigenVecs[v] = new Vector4[18];
            for (int n = 0; n < Globals.NeighborhoodSynth_NUM_NEIGHBORS; n++)
            {
               mGroupedEigenVecs[v][n * 2 + 0] = new Vector4(getEigenVect(v, n * 8 + 0),getEigenVect(v, n * 8 + 1),getEigenVect(v, n * 8 + 2),getEigenVect(v, n * 8 + 3));
               mGroupedEigenVecs[v][n * 2 + 1] = new Vector4(getEigenVect(v, n * 8 + 4),getEigenVect(v, n * 8 + 5),getEigenVect(v, n * 8 + 6),getEigenVect(v, n * 8 + 7));
            }
         }
      }

   }
}