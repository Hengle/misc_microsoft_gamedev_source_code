/**

class PCA

Computes PCA of given samples
Enables projection on a given number of eigenvectors

2004-06-24 Sylvain Lefebvre - (c) Microsoft Corp.
2005-12    modified for anisotexsyn project
*/

// -----------------------------------------------------
#ifndef __PCA__
#define __PCA__
// -----------------------------------------------------

#include <vector>
#include <list>
#include <map>

// -----------------------------------------------------

using namespace std;

// -----------------------------------------------------

#include "Hh.h"
#include "Principal.h"
#include "Random.h"
#include "CSynthesisException.h"
#include "mem_tools.h"

// -----------------------------------------------------

#define log2(x) (log(x)/log(2.0))

// -----------------------------------------------------

template <class T_Sample> class PCA
{
protected:

  // -------------------------------------------
  /// Sample container - wrapper for user samples
  class SampleContainer
  {
  protected:

    /// user defined sample
    const T_Sample&       m_Sample;

  public:

    SampleContainer() : m_Sample(T_Sample()) {}

    SampleContainer(const T_Sample& s) : m_Sample(s) 
    { }

    SampleContainer(const SampleContainer& sc) : m_Sample(sc.m_Sample)
    { }

    const T_Sample& sample()                       {return (m_Sample);}

    // for compatitbility with VC++ std::vector
    const SampleContainer& operator = (const SampleContainer& ) 
    {throw SynthesisException("SampleContainer::operator =  !! INVALID OPERATION !!"); return (*this);}
  };

  // -------------------------------------------

  /// number of dimensions
  int                     m_iNumDim;
  /// flag to check wether computePCA has been called or not
  bool                    m_bReady;
  /// total variance in dataset
  double                  m_dTotalVariance;
  /// vector of sample constainers
  vector<SampleContainer> m_Samples;
  /// list of locally stored samples
  list<T_Sample>          m_LocalSamples;
  /// Mean value of samples
  TableAllocator<float>   m_Mean;
  /// eigen vectors from PCA
  vector<vector<float> >  m_EigenVects;
  /// eigen values from PCA                // NOTE: these are actually sqrt(eigenval) (standard deviation) // TODO: rename
  TableAllocator<float>   m_EigenVals;

public:

  // -------------------------------------------
  /// constructor
  PCA()	
  {	
    m_iNumDim=0;
    m_bReady=false;
    m_dTotalVariance=0.0;
  }

  // -------------------------------------------
  /// set number of dimensions
  void setNumDim(int n)
  {
    m_iNumDim=n;
    m_dTotalVariance=0.0;
    m_EigenVals.allocate(n);
    m_Mean.allocate(n);
  }

  int getNumDim() const {return (m_iNumDim);}

  // -------------------------------------------
  /// clear
  void clear()
  {
    m_Samples.clear();
    m_LocalSamples.clear();
  }

  // -------------------------------------------
  /// add a sample (stored by reference)
  void addSampleByRef(const T_Sample& s)
  {
    assertx(s.numDim() == m_iNumDim);
    // add
    m_Samples.push_back(SampleContainer(s));
  }

  // -------------------------------------------
  /// add a sample (local copy - T_Sample operator = must be defined )
  void addSampleByCopy(const T_Sample& s)
  {
    assertx(s.numDim() == m_iNumDim);
    // add
    m_LocalSamples.push_back(s);
    m_Samples.push_back(SampleContainer((m_LocalSamples.back())));
  }

  // -------------------------------------------
  /// compute PCA from samples
  bool computePCA(int empca=0)
  {
    assertx(!m_Samples.empty());
    m_bReady=true;

    // compute mean
    m_Mean.zero();
    for (int c=0;c<(int)m_Samples.size();c++) {
      for (int d=0;d<m_iNumDim;d++)
        m_Mean[d]+=m_Samples[c].sample().getValue(d);
    }
    for (int d=0;d<m_iNumDim;d++)
      m_Mean[d]/=(float)m_Samples.size();
    // compute total variance
    m_dTotalVariance=0.0;
    for (int c=0;c<(int)m_Samples.size();c++) {
      float dist=0.0f;
      for (int d=0;d<m_iNumDim;d++) {
        float diff=m_Samples[c].sample().getValue(d) - m_Mean[d];
        dist += diff*diff;
      }
      m_dTotalVariance += dist;
    }
    m_dTotalVariance/=(double)m_Samples.size();
    // compute PCA
    bool valid  =((int)m_Samples.size() > m_iNumDim);
    bool success=false;
    if (valid) {
      // store samples in matrix
      Matrix<float> msmpls((int)m_Samples.size(),m_iNumDim);
      for (int s=0;s<(int)m_Samples.size();s++) {
        for (int i=0;i<m_iNumDim;i++)
          msmpls[s][i]=(float)m_Samples[s].sample().getValue(i);
      }
      // subtract mean from samples
      SubtractMean(msmpls);
      // PCA
      Matrix<float> evects; 
      Array<float>  evals;
      if (empca <= 0) {
        // Standard PCA
        PrincipalComponents(msmpls,evects,evals);
        success=true;
        // update eigenvectors
        m_EigenVects.resize(m_iNumDim);
        for (int j=0;j<m_iNumDim;j++) {
          m_EigenVects[j].resize(m_iNumDim);
          for (int i=0;i<m_iNumDim;i++) {
            m_EigenVects[j][i]=evects[j][i];
          }
        }
        // update eigenvalues
        for (int i=0;i<m_iNumDim;i++) {
          m_EigenVals[i]=evals[i];
        }
      } else {
        // Fast PCA
        int numEMPCAVectors=min(20,m_iNumDim);   // TODO: numEMPCAVectors should be a parameter!!
        cerr << hform("(empca niter=%d, numvec=%d)",empca,numEMPCAVectors);
        evects.init(numEMPCAVectors,m_iNumDim);
        success=EMPrincipalComponents(msmpls,evects,evals,empca);
        if (success) {
          // update eigenvectors
          m_EigenVects.resize(m_iNumDim);
          for (int j=0;j<m_iNumDim;j++) {
            m_EigenVects[j].resize(m_iNumDim);
            if (j >= numEMPCAVectors) {
              for (int i=0;i<m_iNumDim;i++)     m_EigenVects[j][i]=0;
            } else {
              for (int i=0;i<m_iNumDim;i++)     m_EigenVects[j][i]=evects[j][i];
            }
          }
          // update eigenvalues
          for (int i=0;i<m_iNumDim;i++) {
            if (i >= numEMPCAVectors) m_EigenVals[i]=0;
            else                      m_EigenVals[i]=evals[i];
          }
        } // success
      } // empca
    } // valid
    
    if (!valid || !success) {
      if (!valid)   cerr << "PCA::computePCA() - WARNING - not enough samples" << cerr;
      if (!success) cerr << "PCA::computePCA() - WARNING - could not compute PCA" << endl;
      // -> creates identity basis
      m_EigenVects.resize(m_iNumDim);
      for (int j=0;j<m_iNumDim;j++)  {
        m_EigenVects[j].resize(m_iNumDim);
        for (int i=0;i<m_iNumDim;i++)  {
          if (i == j)  m_EigenVects[j][i]=1.0f;
          else         m_EigenVects[j][i]=0.0f;
        }
      }
      for (int i=0;i<m_iNumDim;i++) {
        m_EigenVals[i]=0;
      }
    }

    return (true);
  }

  // -------------------------------------------
  /// projects a sample on n first eigen vectors
  T_Sample project(const T_Sample& smp,int n) const
  {
    T_Sample        ret;
    partial(smp,n,ret);
    return (ret);
  }

  // -------------------------------------------
  /// projects a sample on n first eigen vectors
  void project(const T_Sample& smp,int n,T_Sample& ret) const
  {
    assertx(n <= int(m_EigenVects.size()));
    for (int d=0;d<m_iNumDim;d++)
      ret.setValue(d,0.0f);
    for (int i=0;i<n;i++) {
      assertx(int(m_EigenVects[i].size()) == m_iNumDim);
      float proj=0.0;
      for (int j=0;j<m_iNumDim;j++)
        proj+=m_EigenVects[i][j]*smp.getValue(j);
      ret.setValue(i,proj);
    }
  }

  // -------------------------------------------
  /// unprojects a sample using n first eigen vectors
  void unproject(const T_Sample& smp,int n,T_Sample& ret) const
  {
    assertx(n <= int(m_EigenVects.size()));
    for (int j=0;j<m_iNumDim;j++) {
      float unproj=0.0;
      for (int i=0;i<n;i++)
        unproj+=m_EigenVects[i][j]*smp.getValue(i);
      ret.setValue(j,unproj);
    }
  }

  // -------------------------------------------
  /// add mean to sample
  void addMean(const T_Sample& smp,T_Sample& ret) const
  {
    for (int j=0;j<m_iNumDim;j++) {
      ret.setValue(j,smp.getValue(j)+getMean(j));
    }
  }

  // -------------------------------------------
  /// subtract mean from sample
  void subMean(const T_Sample& smp,T_Sample& ret) const
  {
    for (int j=0;j<m_iNumDim;j++) {
      ret.setValue(j,smp.getValue(j)-getMean(j));
    }
  }

  // -------------------------------------------
  /// accessors
  float    getMean(int i)         const {checkAccess(i); return (m_Mean[i]);        }
  float    getStdDev(int i)       const {checkAccess(i); return (m_EigenVals[i]);   }
  float    getEigenVal(int i)     const {checkAccess(i); return (m_EigenVals[i]);   }
  double   getTotalVariance()     const {checkAccess();  return (m_dTotalVariance); }
  
  float    getEigenVect(int v,int c) const 
  {
    checkAccess(0); 
    assertx(v < (int)m_EigenVects.size() && c < (int)m_EigenVects[v].size()); 
    return (m_EigenVects[v][c]);
  }

  // -------------------------------------------
  // Computes the percentage of energy in N first dimensions
  float    percentEnergyInNDimensions(int n)
  {
    checkAccess();
    double accumw   =0.0;
    for (int i=0;i<min(n,getNumDim());i++) {
      double e   = getStdDev(i);
      accumw    += e*e;
    }
    if (m_dTotalVariance > 1e-9f)  return float(accumw*100.0/m_dTotalVariance);
    else                           return (100.0f);
  }

  // -------------------------------------------
  // List the percentage of energy in dimensions 0..n
  void    listPercentEnergyInDimensions(int n,TableAllocator<float>& result)
  {
    result.allocate(n+1);
    for (int i=0;i<=n;i++) {
      result[i]=percentEnergyInNDimensions(i);
      // this is very innefficient: should perform a running sum
      // ... but it does not have to be fast
    }
  }

  // -------------------------------------------
  /// copy basis from another PCA
  template <class P> void copyBasisFrom(const P& pca)
  {
    pca.checkAccess();
    m_bReady=true;
    if (pca.m_iNumDim == m_iNumDim) {
      for (int i=0;i<m_iNumDim;i++) {
        m_Mean[i]  =pca.getMean(i);
        m_EigenVals[i]=pca.getEigenVal(i);
        for (int j=0;j<m_iNumDim;j++)
          m_EigenVects[i][j]=pca.getEigenVect(i,j);
      }
    }
    else
      throw SynthesisException("PCA::copyBasisFrom - PCA basis size differs");
  }

  // -------------------------------------------
  /// save to stream
  void save(ostream& o) const
  {
    checkAccess();

    o.precision(10);
    // dimensions
    o << m_iNumDim << endl;
    o << 0 << endl; // backward comp.
    // mean
    for (int i=0;i<m_iNumDim;i++)
      o << m_Mean[i] << ' ';
    o << endl;
    // eigen values
    for (int i=0;i<m_iNumDim;i++)
      o << m_EigenVals[i] << ' ';
    o << endl;
    // eigen vectors
    o << (int)m_EigenVects.size() << endl;
    for (int e=0;e<(int)m_EigenVects.size();e++)  {
      o << (int)m_EigenVects[e].size() << ' ';
      for (int i=0;i<(int)m_EigenVects[e].size();i++)
        o << m_EigenVects[e][i] << ' ';
      o << endl;
    }
  }

  // -------------------------------------------
  /// load from stream
  bool load(istream& is)
  {
    m_bReady=true;
    // dimensions
    is >> m_iNumDim;
    if (m_iNumDim == 0) return false;
    assertx(m_iNumDim > 0);
    setNumDim(m_iNumDim);
    // for backward compatibility
    int tmp; is >> tmp;
    // mean
    for (int i=0;i<m_iNumDim;i++)
      is >> m_Mean[i];
    // for backward compatibility
    float f; for (int i=0;i<tmp;i++) is >> f;
    // eigen values
    for (int i=0;i<m_iNumDim;i++)
      is >> m_EigenVals[i];
    // eigen vectors
    int nb,sz;
    is >> nb;
    m_EigenVects.resize(nb);
    for (int e=0;e<nb;e++) {
      is >> sz;
      assertx(sz == m_iNumDim);
      m_EigenVects[e].resize(sz);
      for (int i=0;i<(int)m_EigenVects[e].size();i++)
        is >> m_EigenVects[e][i];
    }
    return (true);
  }

  // ----------------------------------------------
  
  void checkAccess(int i=0) const
  {
    if (!m_bReady)
      throw SynthesisException("PCA::checkAccess - PCA basis not initialized (call computePCA or copyBasis)");
    if (i < 0 || i > (int)m_iNumDim)
      throw SynthesisException("PCA::checkAccess - access violation");
  }

  // ----------------------------------------------

  bool isReady() const
  {
    return (m_bReady);
  }

};

// -----------------------------------------------------
#endif
// -----------------------------------------------------
