/**

class Neighbors
class Neighborhood

Store information on pixel neighborhood.

2004-06-24 Sylvain Lefebvre

*/

// -----------------------------------------------------
#ifndef __NEIGHBORHOOD__
#define __NEIGHBORHOOD__
// -----------------------------------------------------

#include <vector>

using namespace std;

// -----------------------------------------------------

#include "Hh.h"

#include "config.h"

#include "CMultiDimFloatTexture.h"

#include "mem_tools.h"

// -----------------------------------------------------

// configuration of non-square neighborhoods
#define NeighborhoodSynth_NUM_COMP      NUM_RECOLORED_PCA_COMPONENTS
#define NeighborhoodSynth_RADIUS        2
#define NeighborhoodSynth_SIZE          (NeighborhoodSynth_RADIUS*2+1)

#define NeighborhoodSynth_NUM_NEIGHBORS 4
#define NeighborhoodSynth_SHAPE         {' ',' ',' ',' ',' ', \
                                         ' ','x',' ','x',' ', \
                                         ' ',' ',' ',' ',' ', \
                                         ' ','x',' ','x',' ', \
                                         ' ',' ',' ',' ',' '};


/*
#define NeighborhoodSynth_NUM_NEIGHBORS 9
#define NeighborhoodSynth_SHAPE         {' ','x',' ','x',' ', \
                                         'x',' ',' ',' ','x', \
                                         ' ',' ','x',' ',' ', \
                                         'x',' ',' ',' ','x', \
                                         ' ','x',' ','x',' '};
*/

/*
#define NeighborhoodSynth_NUM_NEIGHBORS 9
#define NeighborhoodSynth_SHAPE         {' ',' ',' ',' ',' ', \
                                         ' ','x','x','x',' ', \
                                         ' ','x','x','x',' ', \
                                         ' ','x','x','x',' ', \
                                         ' ',' ',' ',' ',' '};
*/

// -----------------------------------------------------
// Neighborhoods can be built in synthesis space (pyramid)
// or in the exemplar (stack). The accessors below
// are used by the neighborhood class to access neighbors.
// They are also used to build the stack and during synthesis
// on the CPU version.

class accessor 
{
public: // TODO find a way around 'virtual': this is slow
  virtual void neigh_ij(int i,int j,int li,int lj,int& _i,int& _j) const = 0;
  virtual void child_ij(int i,int j,int li,int lj,int& _i,int& _j) const = 0;
  virtual void level_pos(int i,int j,double& _i,double& _j) const = 0;
  virtual bool comparable(int i,int j,int x,int y) const = 0;
};

// -------

class pyramid_accessor : public accessor
{
protected:
  int m_iLvl;
public:
  pyramid_accessor(int l) {m_iLvl=l;}
  inline void neigh_ij(int i,int j,int li,int lj,int& _i,int& _j) const
  {
    _i = i + li;
    _j = j + lj;
  }
  inline void child_ij(int i,int j,int li,int lj,int& _i,int& _j) const
  {
    _i = (i << 1) + li;
    _j = (j << 1) + lj;
  }
  inline void level_pos(int i,int j,double& _i,double& _j) const
  {
    _i = i + 0.5*(1 << m_iLvl);
    _j = j + 0.5*(1 << m_iLvl);
  }
  virtual bool comparable(int i,int j,int x,int y) const
  {
    return (true);
  }
};

// -------

class stack_accessor_v2 : public accessor
{
protected:
  int m_iLOffs;
  int m_iLvl;
public:
  stack_accessor_v2(int l) {m_iLOffs=(1 << l); m_iLvl=l;}
  inline void neigh_ij(int i,int j,int li,int lj,int& _i,int& _j) const
  {
    _i = i + li * m_iLOffs;
    _j = j + lj * m_iLOffs;
  }
  inline void child_ij(int i,int j,int li,int lj,int& _i,int& _j) const
  {
    // level is considered as DESTINATION level  
    // (not very coherent with neigh_ij -- FIXME ?)
/*
    if (m_iLvl > 0)
    {
      int dli = 2*li - 1;
      int dlj = 2*lj - 1;
      _i = i + dli * (m_iLOffs >> 1);
      _j = j + dlj * (m_iLOffs >> 1);
    }
    else
    {
      _i = i + li;
      _j = j + lj;
    }
*/
    _i = i + (int)floor(((float)li - .5f) * m_iLOffs); 
    _j = j + (int)floor(((float)lj - .5f) * m_iLOffs);
  }
  inline void level_pos(int i,int j,double& _i,double& _j) const
  {
    _i = i;
    _j = j;
  }
  virtual bool comparable(int i,int j,int x,int y) const
  {
    return ( ((i % m_iLOffs) == (x % m_iLOffs))
      && ((j % m_iLOffs) == (y % m_iLOffs)));
  }
};

#ifdef USE_STACK_EXEMPLAR
#define exemplar_accessor stack_accessor_v2
#else
ERROR__USE_STACK_EXEMPLAR_must_be_defined
// #define exemplar_accessor pyramid_accessor
#endif



// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------
// Neighborhood class for a single level


class Neighborhood
{
protected:

  int                   m_iN;
  int                   m_iC;
  int                   m_iNumDim;

  TableAllocator<float,true> m_Values;
  bool                       m_bCrossing;
  short                      m_iI;
  short                      m_iJ;

public:

  Neighborhood()
  {
    m_iC=0;
    m_iN=0;
    m_iNumDim=0;
    m_bCrossing=false;
    m_iI=0;
    m_iJ=0;
  }

  Neighborhood(int n,int c)
  {
    setShape(n,c);
    m_bCrossing=false;
    m_iI=0;
    m_iJ=0;
  }

  void setShape(int n,int c)
  {
    m_iC=c;
    m_iN=n;
    m_iNumDim=(2*n+1)*(2*n+1)*c;
    m_Values.allocate(m_iNumDim);
    m_Values.zero();
  }

  Neighborhood(const Neighborhood& n)
  {
    if (n.m_iNumDim == 0) {
      m_iC=0;
      m_iN=0;
      m_iNumDim=0;
      m_bCrossing=false;
      m_iI=0;
      m_iJ=0; 
    }  else  {
      setShape(n.m_iN,n.m_iC);
      for (int i=0;i<m_iNumDim;i++)
        m_Values[i]=n.m_Values[i];
      m_bCrossing=false;
      m_iI=n.m_iI;
      m_iJ=n.m_iJ;
    }
  }

  Neighborhood(
    int   n,
    const MultiDimFloatTexture *t,
    const accessor& access,
    bool  checkcrossing,
    int l,int i,int j,
    const MultiDimFloatTexture *kernel)
  {
    construct(n,t,access,checkcrossing,l,i,j,kernel);
  }

  // constructs a regular square neighborhood from an RGB texture
  void construct(
    int   n_radius,
    const MultiDimFloatTexture *t,
    const accessor& access,
    bool  checkcrossing,
    int l,int i,int j,
    const MultiDimFloatTexture *kernel)
  {
    setShape(n_radius,t->numComp());

    m_iI=i;
    m_iJ=j;
    m_bCrossing=false;

    int w=t->getWidth();
    int h=t->getHeight();
    int n=0;

    float kctr=kernel->get(m_iN,m_iN,0);

    for (int sj=-m_iN;sj<=m_iN;sj++) // NOTE: this is NOT the same order as Sig05 - this has no impact on synthesis
      // quality, but do *not* copy this code in paratexsyn or the runtime projection 
      // (GPU) will no longer work (it is using column-major order) !!
    {
      for (int si=-m_iN;si<=m_iN;si++)
      {
        int ni,nj;
        access.neigh_ij(i,j,si,sj,
          ni,nj);
        ni = ((ni % w) + w) % w; // FIXME TODO make this faster
        nj = ((nj % h) + h) % h;

        for (int c=0;c<m_iC;c++)
        {
          float v=t->get(ni,nj,c);
          // kernel ? (eg. Gauss attenuation kernel)
          if (kernel != NULL) v=v*kernel->get(si+m_iN,sj+m_iN,0)/kctr;
          m_Values[n++]=((float)v)/(255.0f);  // stores colors btw 0...1 (for compatibility with shader)
        }

      }
    }

    // border detection
    // -> if the neighborhood crosses the boundary of a non-toroidal texture,
    //    it will not be used as a candidate. Therefore, synthesis will not use it.
    if (checkcrossing)
    {
      int hl=(1<<l);
      if (i < m_iN*hl || i >= w-m_iN*hl)
        m_bCrossing=true;
      if (j < m_iN*hl || j >= h-m_iN*hl)
        m_bCrossing=true;
    }

    assertx(n == m_iNumDim);
  }

  // computes distance between this neighborhood and another
  double distanceSq(const Neighborhood& ne) const
  {
    assertx(m_iNumDim == ne.m_iNumDim && m_iC == ne.m_iC);
    double dist=0.0;
    for (int n=0;n<m_iNumDim;n++) {
      double diff=(ne.m_Values[n]-m_Values[n]);
      dist += diff*diff;
    }
    return (dist);
  }

  inline float  getValue(int i) const {return (m_Values[i]);}
  inline void   setValue(int i,float d) {m_Values[i]=d;}
  inline int    numDim()        const {return (m_iNumDim);}

  inline int    i() const {return (m_iI);}
  inline int    j() const {return (m_iJ);}
  inline void   setIJ(int i,int j) {m_iI=i; m_iJ=j;}
  inline bool   isCrossing() const {return (m_bCrossing);}

};


// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------
// Sparse neighborhood class - 2005-09-22
//
// This neighborhood class can handle sparse neighborhoods.
// It reads data from MultiDimFloatTexture textures.
// 
//


template <int T_iNumNeighbors,int T_iC> 
class SparseNeighborhood
{
public:

  enum {e_numdim = T_iNumNeighbors*T_iC};

protected:

  float         m_Values[e_numdim];
  bool          m_bCrossing;
  short         m_iI;
  short         m_iJ;

protected:

  class OffsetTableInitializer
  {
  public:

    int m_OffsetTable[T_iNumNeighbors][2];

  public:

    OffsetTableInitializer()
    {
      const char tbl[NeighborhoodSynth_SIZE*NeighborhoodSynth_SIZE]=NeighborhoodSynth_SHAPE;
      neighborhood2OffsetTable(tbl,NeighborhoodSynth_SIZE,m_OffsetTable);  
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
    void neighborhood2OffsetTable(
      const char *neighborhood_shape,
      int         neighborhood_size,
      int         _neighbors_offsets[T_iNumNeighbors][2]
      )
      {
        int n     =0;
        int radius=neighborhood_size/2;
        for (int j=0;j<neighborhood_size;j++)
        {
          for (int i=0;i<neighborhood_size;i++)
          {
            if (neighborhood_shape[i + j*neighborhood_size] != ' ')
            {
              _neighbors_offsets[n][0] = i-radius;
              _neighbors_offsets[n][1] = j-radius;
              n++;
              // sanity check
              assertx(
                   _neighbors_offsets[n][0] >= -(radius)
                && _neighbors_offsets[n][0] <=  (radius)
                && _neighbors_offsets[n][1] >= -(radius)
                && _neighbors_offsets[n][1] <=  (radius));
            }
          }
        }
        // sanity check
        assertx(n == T_iNumNeighbors);
      }    
  };

public:

  static OffsetTableInitializer s_OffsetTable;

  SparseNeighborhood()
  {
    for (int i=0;i<e_numdim;i++)
      m_Values[i]=0.0;
    m_bCrossing=false;
    m_iI=0;
    m_iJ=0;
  }

  SparseNeighborhood(const SparseNeighborhood& n)
  {
    for (int i=0;i<e_numdim;i++)
      m_Values[i]=n.m_Values[i];
    m_bCrossing=false;
    m_iI       =n.m_iI;
    m_iJ       =n.m_iJ;
  }

  SparseNeighborhood(
    const MultiDimFloatTexture *t,
    const accessor& access,
    bool  checkcrossing,
    int l,int i,int j)
  {
    construct(t,access,checkcrossing,l,i,j);
  }

  // constructs a neighborhood from a list of offsets
  void construct(
    const MultiDimFloatTexture *t,
    const accessor& access,
    bool  checkcrossing,
    int l,int i,int j)
  {
    m_iI=i;
    m_iJ=j;
    m_bCrossing=false;

    int w=t->getWidth();
    int h=t->getHeight();
    int n=0;

    for (int k=0;k<T_iNumNeighbors;k++)
    {
      int offsi = s_OffsetTable.m_OffsetTable[k][0];
      int offsj = s_OffsetTable.m_OffsetTable[k][1];

      int ni,nj;
      access.neigh_ij(i,j,offsi,offsj,
        ni,nj);
      ni = ((ni % w) + w) % w; // FIXME TODO make this faster
      nj = ((nj % h) + h) % h;

      for (int c=0;c<T_iC;c++)
      {
        float v=t->get(ni,nj,c);
        m_Values[n++]=v;
      }

    }

    // border detection
    // -> if the neighborhood crosses the boundary of a non-toroidal texture,
    //    it will not be used as a candidate. Therefore, synthesis will not use it.
    if (checkcrossing) //l < FIRST_LEVEL_WITH_BORDER) // FIXME TODO: how to choose this automatically ?
    {
      int hl=(1<<l);
      if (i < NeighborhoodSynth_SIZE/2*hl || i >= w-NeighborhoodSynth_SIZE/2*hl)
        m_bCrossing=true;
      if (j < NeighborhoodSynth_SIZE/2*hl || j >= h-NeighborhoodSynth_SIZE/2*hl)
        m_bCrossing=true;
    }

    assertx(n == e_numdim);
  }

  inline float  getValue(int i)         const {return (m_Values[i]);}
  inline void   setValue(int i,float d)       {m_Values[i]=d;}
  inline int    numDim()                const {return (e_numdim);}
  inline int    i()                     const {return (m_iI);}
  inline int    j()                     const {return (m_iJ);}
  inline bool   isCrossing()            const {return (m_bCrossing);}
  inline void   setIJ(int i,int j)            {m_iI=i;m_iJ=j;}

};


// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------
// Defines for the two types of neighborhoods: analysis for similarity sets and run-time

typedef Neighborhood  NeighborhoodRecolor; 

typedef Neighborhood  NeighborhoodSimset;

typedef SparseNeighborhood<NeighborhoodSynth_NUM_NEIGHBORS,NeighborhoodSynth_NUM_COMP> NeighborhoodSynth;
// neighborhoods used at runtime

// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------
// Defines for PCA analysis of these neighborhoods

#include "TPCA.h"

typedef PCA<NeighborhoodRecolor> PCA_Recolor;
typedef PCA<NeighborhoodSynth  > PCA_Synth;

// -----------------------------------------------------
#endif
// -----------------------------------------------------
