/**

class Clustering

Computes k-mean clustering of given samples (Lloyd clustering)

2006-01-02 Sylvain Lefebvre - (c) Microsoft Corp.

*/

// -----------------------------------------------------
#ifndef __CLUSTERING__
#define __CLUSTERING__
// -----------------------------------------------------

#include <vector>
#include <list>
#include <algorithm>

// -----------------------------------------------------

using namespace std;

// -----------------------------------------------------

#include "Hh.h"
#include "Random.h"
#include "CSynthesisException.h"
#include "mem_tools.h"
#include "CSimpleTimer.h"

// -----------------------------------------------------

template <class T_Sample> class Clustering
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

  typedef TableAllocator<float,false,false> t_cluster_center;

  // -------------------------------------------

  // squared distance between samples
  /*float distance2(const T_Sample& s0,const T_Sample& s1) 
  {
    float dist=0.0f;
    for (int i=0;i<m_iNumDim;i++) {
      float diff = s0.getValue(i) - s1.getValue(i);
      dist += diff*diff;
    }
    return (dist);
  }*/

  // squared distance between sample and cluster center
  float distance2_interrupt(const T_Sample& s,const t_cluster_center& ctr,float currentmin) 
  {
    float dist=0.0f;
    for (int i=0;i<m_iNumDim;i++) {
      float diff = s.getValue(i) - ctr[i];
      dist += diff*diff;
      if (dist > currentmin)
        return (dist);
    }
    return (dist);
  }

  // squared distance between sample and cluster center
  float distance2(const T_Sample& s,const t_cluster_center& ctr) 
  {
    float dist=0.0f;
    for (int i=0;i<m_iNumDim;i++) {
      float diff = s.getValue(i) - ctr[i];
      dist += diff*diff;
    }
    return (dist);
  }

  // squared distance between two cluster centers
  float distance2(const t_cluster_center& ctr0,const t_cluster_center& ctr1) 
  {
    float dist=0.0f;
    for (int i=0;i<m_iNumDim;i++) {
      float diff = ctr0[i] - ctr1[i];
      dist += diff*diff;
    }
    return (dist);
  }

  // -------------------------------------------

  /// number of dimensions
  int                     m_iNumDim;
  /// number of clusters
  int                     m_iNumClusters;
  /// minimum number of clusters
  int                     m_iMinNumClusters;
  /// flag to check wether compute has been called or not
  bool                    m_bReady;
  /// vector of sample constainers
  vector<SampleContainer> m_Samples;
  /// list of locally stored samples
  list<T_Sample>          m_LocalSamples;

  vector<t_cluster_center>          m_Centers;
  vector<vector<int> >              m_Clusters;
  TableAllocator<int,false,true>    m_SampleOwner;
  float                             m_fTargetIntraClusterDist;

public:

  // -------------------------------------------
  /// constructor
  Clustering()
  {	
    m_iNumDim             =0;
    m_iNumClusters        =1;
    m_iMinNumClusters     =1;
    m_fTargetIntraClusterDist=1.0f;
    m_bReady              =false;
  }
  
  // -------------------------------------------
  /// init
  void init(int numdim,float maxdist,int minclusters)	
  {
    m_iNumDim             =numdim;
    m_iNumClusters        =1;
    m_iMinNumClusters     =1;
    m_bReady              =false;
    m_iMinNumClusters     =max(1,minclusters);
    m_fTargetIntraClusterDist=maxdist;
    clear();
  }
  
  // -------------------------------------------
  
  int numDim()      const {return (m_iNumDim);}
  int numClusters() const {return (m_iNumClusters);}

  // -------------------------------------------
  /// clear
  void clear()
  {
    m_Samples.clear();
    m_LocalSamples.clear();
  }

  // -------------------------------------------
  /// add a sample (stored by reference)
  void addSample(const T_Sample& s)
  {
    assertx(s.numDim() == m_iNumDim);
    // add
    m_Samples.push_back(SampleContainer(s));
  }

  // -------------------------------------------
  /// add a sample (local copy)
  void giveSample(const T_Sample& s)
  {
    assertx(s.numDim() == m_iNumDim);
    // add
    m_LocalSamples.push_back(s);
    m_Samples.push_back(SampleContainer((m_LocalSamples.back())));
  }

  // -------------------------------------------
  /// find which cluster center is the closest to a sample
  int findClosestCenter(const T_Sample& s)
  {
    checkAccess();

    int    argmin=-1;
    float distmin=1e16f;
    for (int c=0;c<m_iNumClusters;c++) {
      float dist=distance2_interrupt(s,m_Centers[c],distmin);
      if (dist < distmin) {
        argmin=c;
        distmin=dist;
      }
    }
    assertx(argmin >= 0);
    return (argmin);
  }

  // -------------------------------------------
  /// distribute samples in clusters
  int distribute()
  {
//    SimpleTimer tm("\n[Clustering::distribute] %02d min %02d sec %d ms\n");

    // clear previous clusters
    m_Clusters.resize(m_iNumClusters);
    for (int c=0;c<m_iNumClusters;c++) {
      m_Clusters[c].clear();
      m_Clusters[c].reserve(m_Samples.size()/m_iNumClusters);
    }
    // foreach sample, find closest center
    int num_changes=0;
    for (int s=0;s<int(m_Samples.size());s++) {
      int closest=findClosestCenter(m_Samples[s].sample()); // expensive...
      // update cluster
      m_Clusters[closest].push_back(s);
      // update sample owner table, count changes
      if (m_SampleOwner[s] != closest)
        num_changes++;
      m_SampleOwner[s]=closest;
//      assertx(closest >= 0 && closest < m_iNumClusters);
    }
    return (num_changes);
  }

  // -------------------------------------------
  /// update cluster centers
  void updateCenters()
  {
//    SimpleTimer tm("\n[Clustering::updateCenters] %02d min %02d sec %d ms\n");

    static TableAllocator<int,true,false> counters;
    // recompute centers from samples
    // -> zeros centers
    counters.reallocate(m_iNumClusters);
    for (int c=0;c<m_iNumClusters;c++) {
      m_Centers[c].zero();
    }
    // -> compute averages
    for (int s=0;s<int(m_Samples.size());s++) {
      int o=m_SampleOwner[s];
      for (int d=0;d<m_iNumDim;d++)
        m_Centers[o][d] += m_Samples[s].sample().getValue(d);
      counters[o]++;
    }
    for (int c=0;c<m_iNumClusters;c++) {
      if (counters[c] > 0) {
        for (int d=0;d<m_iNumDim;d++)
         m_Centers[c][d] = m_Centers[c][d]/float(counters[c]);
      }
    }
  }

  // -------------------------------------------
  /// split clusters if their radius exceeds user target
  bool split()
  {
//    SimpleTimer tm("\n[Clustering::split] %02d min %02d sec %d ms\n");

    // -> check whether a cluster is too large
    vector<int> to_be_split;
    to_be_split.reserve(m_iNumClusters/4);
    for (int c=0;c<m_iNumClusters;c++) {
      float maxdist=0;
      if (!m_Clusters[c].empty()) {
        for (int e=0;e<int(m_Clusters[c].size());e++) {
          maxdist=max(maxdist,sqrt(distance2(m_Samples[m_Clusters[c][e]].sample(),m_Centers[c])));
        }
      }
      if (maxdist > m_fMaxIntraClusterDist) {
        to_be_split.push_back(c);
      }
    }
    // -> add clusters
    for (int c=0;c<int(to_be_split.size());c++) {
      m_Clusters.push_back(vector<int>());
      t_cluster_center center;
      center.allocate(m_iNumDim);
      // pick an element of the splited cluster at random
      int e=Random::G.getuint() % m_Clusters[to_be_split[c]].size();
      // copy into center
      int s=m_Clusters[to_be_split[c]][e];
      for (int d=0;d<m_Samples[s].sample().numDim();d++)
        center[d]=m_Samples[s].sample().getValue(d);
      // store new center
      m_Centers.push_back(center);
    }
    m_iNumClusters=int(m_Clusters.size());
    assertx(m_Clusters.size() == m_Centers.size());
    return (!to_be_split.empty());
  }
  // -------------------------------------------

  typedef pair<int,float> t_pair_int_float;

  class cmp_pair_int_float {
  public:
    bool operator ()(const t_pair_int_float& a,const t_pair_int_float& b) const {
      return (a.second < b.second);
    }
  };

  // -------------------------------------------
  /// merge clusters if resulting radius does not exceeds user target
  bool merge()
  {
//    SimpleTimer tm("\n[Clustering::merge_closest] %02d min %02d sec %d ms\n");
    // -> compute clusters radius
    static TableAllocator<float> cradius;
    cradius.reallocate(m_iNumClusters);
    for (int c=0;c<m_iNumClusters;c++) {
      float avgdist=0;
      if (!m_Clusters[c].empty()) {
        for (int e=0;e<int(m_Clusters[c].size());e++) {
          avgdist+=distance2(m_Samples[m_Clusters[c][e]].sample(),m_Centers[c]);
        }
        avgdist /= float(m_Clusters[c].size());
        cradius[c]=sqrt(avgdist);
      } else
        cradius[c]=0;
    }
    // -> foreach cluster
    vector<t_pair_int_float> priority_queue;
    for (int c=0;c<m_iNumClusters;c++) {
      // -> find distance to nearest cluster
      int   argmin =-1;
      float distmin=-1;
      for (int o=c+1;o<m_iNumClusters;o++) {
        float dist=distance2(m_Centers[c],m_Centers[o]);
        if (dist < distmin || distmin < 0) {
          argmin=o;
          distmin=dist;
        }
      }
      float radius=0;
      if (argmin>=0) radius = distmin + cradius[c] + cradius[argmin];
      if (argmin>=0 && radius < m_fTargetIntraClusterDist) {
        // add to priority queue
        priority_queue.push_back(make_pair(c,radius));
      } 
    }
    // remove flagged clusters by order of est. resulting radius
    sort(priority_queue.begin(),priority_queue.end(),cmp_pair_int_float());
    vector<int> to_be_removed;
    for (int r=0;r<int(priority_queue.size());r++) {
      if (m_iNumClusters - int(to_be_removed.size()) <= m_iMinNumClusters)
        break;
      to_be_removed.push_back(priority_queue[r].first);
    }
    // remove from centers
    if (!to_be_removed.empty()) {
      sort(to_be_removed.begin(),to_be_removed.end());
      for (int r=int(to_be_removed.size())-1;r>=0;r--) {
        assertx(to_be_removed[r] >= 0 && to_be_removed[r] < int(m_Centers.size()));
        m_Centers[to_be_removed[r]]=m_Centers.back();
        m_Centers.pop_back();
        assertx(!m_Centers.empty());
      }
      m_iNumClusters=int(m_Centers.size());
      m_Clusters.clear();
      return (true);
    } else
      return (false);
  }

  // -------------------------------------------
  /// remove empty clusters
  void clean()
  {
//    SimpleTimer tm("\n[Clustering::clean] %02d min %02d sec %d ms\n");
    vector<int> to_be_removed;
    for (int c=0;c<m_iNumClusters;c++) {
      if (m_Clusters[c].empty())
        to_be_removed.push_back(c);
    }
    if (!to_be_removed.empty()) {
      sort(to_be_removed.begin(),to_be_removed.end());
      for (int r=int(to_be_removed.size())-1;r>=0;r--) {
        assertx(to_be_removed[r] >= 0 && to_be_removed[r] < int(m_Centers.size()));
        m_Centers[to_be_removed[r]]=m_Centers.back();
        m_Centers.pop_back();
        assertx(!m_Centers.empty());
      }
      m_iNumClusters=int(m_Centers.size());
      m_Clusters.clear();
      // redistribute samples
      distribute();
      cerr << hform("(removed %d empty clusters)",int(to_be_removed.size())) << endl;
    }
  }

  // -------------------------------------------
  /// compute k-mean clustering from samples 
  bool compute()
  {
    m_bReady=true;

    m_iNumClusters=int(m_Samples.size());

    // allocate
    m_Centers.resize(m_iNumClusters);
    ForTable(m_Centers,i) {
      m_Centers[i].reallocate(m_iNumDim);
    }
    m_SampleOwner.reallocate(unsigned int(m_Samples.size()));
    
    // init cluster centers
    // -> one cluster per sample
    for (int c=0;c<m_iNumClusters;c++) {
      // copy into center
      for (int d=0;d<m_Samples[c].sample().numDim();d++)
        m_Centers[c][d]=m_Samples[c].sample().getValue(d);
    }

    /*
    // -> randomly chose samples to init clusters
    TableAllocator<int> chosen;
    chosen.allocate(m_iNumClusters);
    for (int c=0;c<m_iNumClusters;c++) {
      // init with a random point
      int s=-1;
      while (1) {
        s=Random::G.getuint() % m_Samples.size();
        // make sure it has not been already chosen
        int i;
        for (i=0;i<c;i++) {
          if (chosen[i]==s) break;
        }
        if (i == c) break;
      }
      // copy into center
      for (int d=0;d<m_Samples[s].sample().numDim();d++)
        m_Centers[c][d]=m_Samples[s].sample().getValue(d);
    }
    */

    int niter=0;
    while (1) {
      int num_changes=distribute();    // recompute clusters
      cerr << hform(" iteration %d: %d changes, %d clusters\n",niter++,num_changes,m_iNumClusters);
      updateCenters();                 // recompute centers
      int before=m_iNumClusters;
      bool modified=false;
      if (m_iNumClusters > m_iMinNumClusters) {
        modified = merge(); // merge clusters
        cerr << hform("               %d clusters after merge (%d removed)\n",m_iNumClusters,before-m_iNumClusters);
      }
      if (num_changes == 0 && !modified)  break;
    };
    clean();

    // detect empty clusters (must not be any after clean !)
    int nsup=0;
    for (int c=0;c<m_iNumClusters;c++) {
      if (m_Clusters[c].empty()) {
        cerr << hform("WARNING cluster %d is empty\n",c);
        assertx(false);
        nsup++;
      }
    }

    return (true);
  }

  // -------------------------------------------
  /// accessors
  float clusterCenter(int c,int d) const {return (m_Centers[c][d]);}
  int   sampleOwner(int s)         const {return (m_SampleOwner[s]);}

  // -------------------------------------------
  /// save to stream
  void save(ostream& o) const
  {
    checkAccess();

    o.precision(10);
    // sample dimensions
    o << m_iNumDim << endl;
    // number of clusters
    o << m_iNumClusters << endl;
    // cluster centers
    for (int c=0;c<m_iNumClusters;c++) {
      ForTable(m_Centers[c],d) {
        o << m_Centers[c][d] << ' ';
      }
      o << endl;
    }
    // sample owners
    o << int(m_SampleOwner.size()) << endl;
    for (int s=0;s<int(m_SampleOwner.size());s++) {
      o << m_SampleOwner[s] << ' ';
    }
    o << endl;
  }

  // -------------------------------------------
  /// load from stream
  void load(istream& is)
  {
    m_bReady=true;
    // sample dimensions
    is >> m_iNumDim;
    // number of clusters
    is >> m_iNumClusters;
    cerr << hform("reading cluster, numdim=%d, numclusters=%d\n",m_iNumDim,m_iNumClusters);
    // cluster centers
    m_Clusters.clear();
    m_Centers.resize(m_iNumClusters);
    for (int c=0;c<m_iNumClusters;c++) {
      m_Centers[c].reallocate(m_iNumDim);
      ForTable(m_Centers[c],d) {
        float f;
        is >> f;
        m_Centers[c][d]=f;
      }
    }
    // sample owners
    int sz;
    is >> sz;
    m_SampleOwner.allocate(sz);
    ForTable(m_SampleOwner,s) {
      is >> m_SampleOwner[s];
    }
  }

  // ----------------------------------------------
  
  void checkAccess(int i=0) const
  {
    if (!m_bReady)
      throw SynthesisException("Clustering::checkAccess - Clustering not computed !");
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
