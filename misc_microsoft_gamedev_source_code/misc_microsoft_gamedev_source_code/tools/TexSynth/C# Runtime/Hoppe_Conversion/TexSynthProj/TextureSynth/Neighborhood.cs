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
using EditorCore;

namespace TextureSynthesis
{

   //CLM we no longer have pyramids, just use stack accessors
   class accessor 
   {
      // TODO find a way around 'virtual': this is slow
     public virtual void neigh_ij(int i,int j,int li,int lj,ref int _i,ref int _j)
     {
     }
     public virtual void child_ij(int i,int j,int li,int lj,ref int _i,ref int _j)
     {
     }
     public virtual void level_pos(int i,int j,ref double _i,ref double _j)
     {
     }
     public virtual bool comparable(int i,int j,int x,int y)
     {
        return false;
     }
   };
   class stack_accessor_v2 : accessor
   {
   
     protected int m_iLOffs;
     protected int m_iLvl;
   
     public stack_accessor_v2(int l) {m_iLOffs=(1 << l); m_iLvl=l;}
      public override void neigh_ij(int i, int j, int li, int lj, ref int _i, ref int _j) 
     {
       _i = i + li * m_iLOffs;
       _j = j + lj * m_iLOffs;
     }
      public override void child_ij(int i, int j, int li, int lj, ref int _i, ref int _j) 
     {
       _i = i + (int)Math.Floor(((float)li - .5f) * m_iLOffs); 
       _j = j + (int)Math.Floor(((float)lj - .5f) * m_iLOffs);
     }
      public override void level_pos(int i, int j, ref double _i, ref double _j) 
     {
       _i = i;
       _j = j;
     }
      public override bool comparable(int i, int j, int x, int y) 
     {
       return ( ((i % m_iLOffs) == (x % m_iLOffs))&& ((j % m_iLOffs) == (y % m_iLOffs)));
     }
   };


   //CLM there were problems with this class being staticly initalized during duplication processes in C#
   class OffsetTableInitializer
  {
  
    public int [][]m_OffsetTable=null;

    public OffsetTableInitializer(int numNeighbors)
    {
       m_OffsetTable = new int[numNeighbors][];
       for(int i=0;i<numNeighbors;i++)
          m_OffsetTable[i] = new int[2];

      neighborhood2OffsetTable();  
    }

    /**
    Builds an offset list from a neighborhood shape
    specified as a table of chars:
    {' ','x',' ',
    'x',' ','x',
    ' ','x',' '};
    Everything different from ' ' is considered a neighbor.
    The neighborhood size must be specified, 3 in the exemple above.
    */
    void neighborhood2OffsetTable( )
      {
        int n     =0;
        for (int j = 0; j < Globals.NeighborhoodSynth_SIZE; j++)
        {
           for (int i = 0; i < Globals.NeighborhoodSynth_SIZE; i++)
          {
             if (Globals.NeighborhoodSynth_SHAPE[i + j * Globals.NeighborhoodSynth_SIZE] != ' ')
            {
               m_OffsetTable[n][0] = i - (Globals.NeighborhoodSynth_SIZE>>1);
               m_OffsetTable[n][1] = j - (Globals.NeighborhoodSynth_SIZE>>1);
             
              // sanity check
              //Globals.Assert(
              //     m_OffsetTable[n][0] >= -(radius)
              //  && m_OffsetTable[n][0] <= (radius)
              //  && m_OffsetTable[n][1] >= -(radius)
              //  && m_OffsetTable[n][1] <= (radius));
              n++;
            }
          }
        }
      
      }    
  };

   class NeighborhoodSynth
   {
      ~NeighborhoodSynth()
      {
         s_OffsetTable = null;
         m_Values = null;
      }

      //CLM this was static origionally.. 
      public OffsetTableInitializer s_OffsetTable = null;


      int T_iNumNeighbors = Globals.NeighborhoodSynth_NUM_NEIGHBORS;
      int T_iC = Globals.NeighborhoodSynth_NUM_COMP;
      int e_numdim = Globals.NeighborhoodSynth_NUM_NEIGHBORS * Globals.NeighborhoodSynth_NUM_COMP;

      // constructs a neighborhood from a list of offsets
     public void construct(MultiDimFloatTexture t,accessor access,bool  checkcrossing,int l,int i,int j)
     {
       //CLM this used to be static...
       s_OffsetTable = new OffsetTableInitializer(Globals.NeighborhoodSynth_NUM_NEIGHBORS);
       

       m_iI=(short)i;
       m_iJ = (short)j;
       m_bCrossing=false;

       int w=t.getWidth();
       int h=t.getHeight();
       int n=0;

       for (int k=0;k<T_iNumNeighbors;k++)
       {
         int offsi = s_OffsetTable.m_OffsetTable[k][0];
         int offsj = s_OffsetTable.m_OffsetTable[k][1];

         int ni=0,nj=0;
         access.neigh_ij(i,j,offsi,offsj,ref ni,ref nj);
         ni = ((ni % w) + w) % w; // FIXME TODO make this faster
         nj = ((nj % h) + h) % h;

         for (int c=0;c<T_iC;c++)
         {
           float v=t.get(ni,nj,c);
           m_Values[n++]=v;
         }
       }
   
       // border detection
       // . if the neighborhood crosses the boundary of a non-toroidal texture,
       //    it will not be used as a candidate. Therefore, synthesis will not use it.
       if (checkcrossing) //l < FIRST_LEVEL_WITH_BORDER) // FIXME TODO: how to choose this automatically ?
       {
         int hl=(1<<l);
         if (i < Globals.NeighborhoodSynth_SIZE / 2 * hl || i >= w - Globals.NeighborhoodSynth_SIZE / 2 * hl)
           m_bCrossing=true;
        if (j < Globals.NeighborhoodSynth_SIZE / 2 * hl || j >= h - Globals.NeighborhoodSynth_SIZE / 2 * hl)
           m_bCrossing=true;
       }

       Globals.Assert(n == e_numdim);
     }

      
     float         []m_Values = new float[Globals.NeighborhoodSynth_NUM_NEIGHBORS * Globals.NeighborhoodSynth_NUM_COMP];
     bool          m_bCrossing;
     short         m_iI;
     short         m_iJ;

     public float  getValue(int i)         {return (m_Values[i]);}
     public void   setValue(int i,float d)       {m_Values[i]=d;}
     public int    numDim()                {return (e_numdim);}
     public int    i()                     {return (m_iI);}
     public int    j()                     {return (m_iJ);}
     public bool   isCrossing()            {return (m_bCrossing);}
     public void   setIJ(int i,int j)            {m_iI=(short)i;m_iJ=(short)j;}
   }
}