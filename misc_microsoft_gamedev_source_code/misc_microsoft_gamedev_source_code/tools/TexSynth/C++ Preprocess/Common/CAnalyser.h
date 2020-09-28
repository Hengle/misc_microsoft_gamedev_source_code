/**

class Analyser

Analyse a given set of images for texture synthesis

2004-06-24 Sylvain Lefebvre - (c) Microsoft Corp.

*/

// -----------------------------------------------------
#ifndef __ANALYSER__
#define __ANALYSER__
// -----------------------------------------------------

#include "TNeighborhood.h"
#include "TPCA.h"
#include "CTexture.h"

// -----------------------------------------------------

class Exemplar;

// -----------------------------------------------------

#define ANALYSER_DONE           0
#define ANALYSER_DATA_EXISTS    1
#define ANALYSER_DATA_NOT_FOUND 2

#define MAX_NUM_LEVELS 16

// -----------------------------------------------------

class Analyser
{
protected:

  // Exemplar
  Exemplar                                    *m_Exemplar;
  // PCA analysis of the neighborhoods of appearence vectors
  vector<PCA_Synth >                           m_PCASynth;
  vector<PCA_Synth >                           m_PCASynth_4Drecoloring;
  // Number of levels
  int                                          m_iNbLevels;
  // runtime PCA computation time
  int                                          m_iTimePCASynth;
  // k-nearests computation time
  int                                          m_iTimeKNearests;
  // transform PCA computation time
  int                                          m_iTimeRecolor;
  // total time
  int                                          m_iTimeTotal;
  // variance per-level captured by recoloring pca
  int                                          m_RecolorNumDimUsed[MAX_NUM_LEVELS];
  float                                        m_RecolorVarianceCaptured[MAX_NUM_LEVELS];
  TableAllocator<float>                        m_RecolorVarianceCapturedGraph[MAX_NUM_LEVELS];
  // variance per-level lost by standard schemes
  double                                       m_StandardSchemeReconstructionError[MAX_NUM_LEVELS][3];
  // variance per-level captured by runtime pca
  float                                        m_RuntimeVarianceCaptured[MAX_NUM_LEVELS];
  TableAllocator<float>                        m_RuntimeVarianceCapturedGraph[MAX_NUM_LEVELS];

  // Creates a texture to visualize an eigen vector for a non square neighborhood
  CTexture *eigenVectorNonSquare2Texture(PCA_Synth& pca,int v,bool rescale);
  // Compute min and max over all components for a given eigenvector of non squared neighborhood
  pair<float,float> eigenVectorNonSquareCompMinMaxMean(PCA_Synth& pca,int v,float& _mean);

  void analyse_recolor();
  void analyse_similarity_sets();
  void analyse_PCA_runtime();
  void analyse_PCA_runtime_4D();

public:

  Analyser() : m_iNbLevels(0), m_Exemplar(NULL) {}

  ~Analyser();

  // set the exemplar
  void setExemplar(
    MultiDimFloatTexture *ex_clr,
    MultiDimFloatTexture *ex_cstr,
    bool toroidal=false,
    int periodx=1,
    int periody=1,
    bool terrain=false);

  // Start analysis
  void analyse();

  // Creates analysis report in directory
  void report(const char *);

  // Save analyser to disk
  void save(const char *fname=NULL);

  // Load analyser from disk
  int  load(const char *fname=NULL);

  // Compute exemplar neighborhoods
  void computeExemplarsNeighborhoods() const;

  // Return 'data set' name
  std::string rootName() const;

  // Accessors

  Exemplar                  *exemplar()     const {return (m_Exemplar);}

  const vector<PCA_Synth>&   pcaSynth()     const;  

  int                        nbLevels()     const {return (m_iNbLevels);}

  friend class Exemplar;
};

// -----------------------------------------------------
#endif
// -----------------------------------------------------
