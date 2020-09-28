/*
  class Exemplar

  Contains all required information about exemplars

  2004-06-24 Sylvain Lefebvre - (c) Microsoft Corp.

*/
// -----------------------------------------------------
#ifndef __EXEMPLAR__
#define __EXEMPLAR__
// -----------------------------------------------------

#include "CTexture.h"
#include "TNeighborhood.h"

#include "CSynthesisException.h"

#include "CMultiDimFloatTexture.h"

#include "config.h"
#include "assert.h"

#include "filetools.h"

#include <vector>

using namespace std;

// -----------------------------------------------------

#define EXEMPLAR_TOROIDAL_FLAG 1
#define EXEMPLAR_TERRAIN_FLAG  2

// -----------------------------------------------------

class Analyser;

// -----------------------------------------------------

class Exemplar
{
public:

protected:

  class knearest_id
  {
  public:
    knearest_id() : l(-1), i(0), j(0), s(0) {}
    knearest_id(int ll,int ii,int jj,int ss) : l(ll), i(ii), j(jj), s(ss) {}
    int i;
    int j;
    int s;
    int l;
    bool operator == (const knearest_id& id) const {return (id.l==l && id.i==i && id.j==j && id.s==s);}
  };

  // class knearest_id_cmp is used to compare knearest ids (coordinates in exemplar)
  // the purpose is to prevent selection of close knearests (in image space)
  class knearest_id_cmp
  {
  public:
    double operator ()(const knearest_id& id0,const knearest_id& id1)
    {
      assertx(id0.l == id1.l);
      exemplar_accessor access(id0.l);
      double d0i,d0j;
      double d1i,d1j;
      access.level_pos(id0.i,id0.j,d0i,d0j);
      access.level_pos(id1.i,id1.j,d1i,d1j);
      double di=d0i - d1i;
      double dj=d0j - d1j;
      return (sqrt(di*di + dj*dj));
    }
  };

  // Analyser
  Analyser                               *m_Analyser;
  // Store all levels - stack
  vector<MultiDimFloatTexture *>          m_Stack;
  // Store all levels - pyramid
  vector<MultiDimFloatTexture *>          m_Pyramid;
  // Store recolored stack
  vector<MultiDimFloatTexture *>          m_RecoloredStack;
  // Store constraint values per pixel (may be empty) - stack
  vector<MultiDimFloatTexture *>          m_Constraints;
  // Store k-nearests information
  vector<vector<vector<knearest_id> > >   m_SimilaritySet;
  // Store synth neighborhoods (after computeSynthNeighborhoods has been called)
  vector<vector<NeighborhoodSynth> >      m_SynthNeighborhoods;
  // Store projected synth neighborhoods (after computeProjectedSynthNeighborhoods has been called)
  vector<vector<NeighborhoodSynth> >      m_ProjectedSynthNeighborhoods;
  // Exemplar id
  int                                     m_iExemplarId;
  // Number of levels
  int                                     m_iNbLevels;
  // Is exemplar toroidal ?
  bool                                    m_bToroidal;
  // Is exemplar a terrain ?
  bool                                    m_bTerrain;
  // Exemplar period along x axis
  int                                     m_iPeriodX;
  // Exemplar period along y axis
  int                                     m_iPeriodY;
  // Optionnal data for color PRT
  const MultiDimFloatTexture             *m_PRTReflectance;
  const CTexture                         *m_PRTColorMap;

  // Creates a texture from k-nearests table
  void kNearestsTableTextures(int level,vector<CTexture *>& _texs) const;

  // Computes the set of RT neighbourhoods
  void computeRTNeighborhoods();

  // Computes the set of synth neighbourhoods
  void computeSynthNeighborhoods();

  // Compute variance in a set of neighborhoods
  double computeVarianceInNeighborhoods(int numComp,const vector<NeighborhoodRecolor>& neighborhoods);

public:

  Exemplar(Analyser *analyser,istream& i);

  Exemplar(int id,
    Analyser       *analyser,
    MultiDimFloatTexture *image,
    bool toroidal,int px=1,int py=1,bool terrain=false);

  Exemplar(int id,
    Analyser       *analyser,
    MultiDimFloatTexture *image,
    MultiDimFloatTexture *constraint,
    bool toroidal,int px=1,int py=1,bool terrain=false);

  ~Exemplar();

  // Computes similarity sets. 
  void computeSimilaritySets();

  // Computes the set of projected synth neighbourhoods
  void computeProjectedSynthNeighborhoods(const vector<PCA_Synth>&);

  // Computes run-time neighborhoods
  void computeRuntimeNeighborhoods(const vector<PCA_Synth>&);

  // Recolors the exemplar
  void recolor();

  // Export data for external recoloring (eg. using MatLab)
  void exportRecolorData(const char *fname);

  // Import data from external recoloring (eg. using MatLab)
  void importRecolorData(const char *fname,int numcomp);

  // Accept or reject candidate for k-nearests
  bool acceptNeighbor(int l,int w,int h,
    int i,int j,bool q_crossing,
    int x,int y,bool c_crossing);

  // Returns number of levels
  int nbLevels() const {return (m_iNbLevels);}

  // Accessors

  float stackPixel(int l,int i,int j,int c)      const
  {return (m_Stack[l]->getmod(i,j,c));}

  float pyramidPixel(int l,int i,int j,int c)    const
  {return (m_Pyramid[l]->getmod(i,j,c));}

  float constraintPixel(int l,int i,int j)       const
  {return (m_Constraints[l]->getmod(i,j,0));}

  const NeighborhoodSynth& getProjectedSynthNeighborhood(int l,int i,int j) const
  {return (m_ProjectedSynthNeighborhoods[l][i+j*m_Stack[l]->getWidth()]);}

  int exemplarId() const {return (m_iExemplarId);}

  int            similaritySetSize(int l,int i,int j) const 
  {return ((int)m_SimilaritySet[l][i+j*m_Stack[l]->getWidth()].size());}

  pair<int,int>  similarPixel(int l,int i,int j,int k) const
  {
    const knearest_id& nfo=m_SimilaritySet[l][i+j*m_Stack[l]->getWidth()][k];
    return (make_pair(nfo.i,nfo.j));
  }

  const MultiDimFloatTexture *constraint(int l)     const {return (m_Constraints[l]);}

  const MultiDimFloatTexture *stack(int l)          const {return (m_Stack[l]);}

  const MultiDimFloatTexture *pyramid(int l)        const {return (m_Pyramid[l]);}

  const MultiDimFloatTexture *recoloredStack(int l) const {return (m_RecoloredStack[l]);}

  bool recoloredStackIsPresent() const {return (!m_RecoloredStack.empty());}

  bool            isToroidal   ()      const {return (m_bToroidal);}

  bool            isConstrained()      const {return (!m_Constraints.empty());}

  int             getPeriodX()         const {return (m_iPeriodX);}

  int             getPeriodY()         const {return (m_iPeriodY);}

  const CTexture *prtColorMap()        const {return (m_PRTColorMap);}

  void            setColorPRTData(const CTexture *albedo,const MultiDimFloatTexture *reflectance)
  { m_PRTColorMap = albedo; m_PRTReflectance=reflectance; }

  void save(const Analyser *a,ostream& o) const;

  // Computes a stack
  int computeStackLevels(
    MultiDimFloatTexture *,
    bool toroidal,
    vector<MultiDimFloatTexture *>& _lvls);

  // Computes a pyramid
  int computePyramidLevels(MultiDimFloatTexture *,vector<MultiDimFloatTexture *>& _lvls);

  // Enlarge a given exemplar
  MultiDimFloatTexture *enlargeTexture(const MultiDimFloatTexture *ex,int type);

  // Computes a stack from an input texture - Box Filter version
  int computeStackLevels_Box(MultiDimFloatTexture *,vector<MultiDimFloatTexture *>& _lvls);

  // Computes a stack from an input texture - Gaussian Filter version
  int computeStackLevels_Gauss(MultiDimFloatTexture *tex,vector<MultiDimFloatTexture *>& _lvls);

  // Computes a pyramid from an input texture - Box Filter version
  int computePyramidLevels_Box(MultiDimFloatTexture *,vector<MultiDimFloatTexture *>& _lvls);

  // Computes a pyramid from an input texture - Gaussian Filter version
  int computePyramidLevels_Gauss(MultiDimFloatTexture *,vector<MultiDimFloatTexture *>& _lvls);

  // Precision for ANN search (controls number of dimension used for search in PCA space)
  static float s_fPrecision;
  // Epsilon for ANN search - in percentage of total energy
  static float s_fEpsilon;
  // Radius of the neighborhood used to compute similarity sets (size = r*2+1)
  static int   s_iNEIGHBORHOOD_RADIUS_SIMSET;
  // Determines whether to use original or recolored exemplar for simset computation
  static bool  s_bUseRecoloredForSimset;

  // DEBUG
  friend class Analyser;
};
// -----------------------------------------------------
#endif
// -----------------------------------------------------
