
#include <windows.h> // (timeGetTime)

#include "CAnalyser.h"

#include <iostream>
#include <fstream>
#include <string>

#include <float.h>

#include "CExemplar.h"
#include "CSimpleTimer.h"
#include "CGlobalParameters.h"

#include "CQuantizer.h"

#include <strsafe.h>

// ------------------------------------------------------------------------

extern int g_iNEIGHBORHOOD_RADIUS_SIMSET;

// ------------------------------------------------------------------------

NeighborhoodSynth::OffsetTableInitializer NeighborhoodSynth::s_OffsetTable;

// ------------------------------------------------------------------------

Analyser::~Analyser()
{
  if (m_Exemplar != NULL)
    delete (m_Exemplar);
}


// ------------------------------------------------------------------------


void Analyser::setExemplar(MultiDimFloatTexture *ex_clr,
                           MultiDimFloatTexture *ex_cstr,
                           bool toroidal,
                           int  periodx,
                           int  periody,
                           bool terrain)
{
  CTexture::convert_functor *convert=new CTexture::convert_std();

  // Create the exemplar

  if (NULL == ex_cstr)
    m_Exemplar = new Exemplar(0,this,ex_clr,toroidal,periodx,periody,terrain);
  else
    m_Exemplar = new Exemplar(0,this,ex_clr,ex_cstr,toroidal,periodx,periody,terrain);
  m_iNbLevels=m_Exemplar->nbLevels();

  delete (convert);

}

// ------------------------------------------------------------------------

std::string Analyser::rootName() const
{
  if (exemplar() == NULL)
    return (std::string("[unknown]"));
  else
  {
    string rootname;
    const char *n=strrchr(exemplar()->stack(0)->getName(),'\\');
    if (n == NULL)
      n=exemplar()->stack(0)->getName();
    else
      n=n+1;
    static char str[MAX_PATH];
    StringCchCopyA(str,MAX_PATH,n);
    char *p=strchr(str,'.');
    if (p!=NULL)
      *p='\0';
    return (std::string(str));
  }
}

  
// ------------------------------------------------------------------------


void Analyser::computeExemplarsNeighborhoods() const
{
  // pre-compute exemplar neighborhoods
  m_Exemplar->computeRuntimeNeighborhoods(pcaSynth());
}


// ------------------------------------------------------------------------


void Analyser::analyse()
{
  long tmstart=timeGetTime();

  // ======================================================
  // -> starting analyse
  cerr << endl;
  cerr << "    -= Starting Analyse =- " << endl;
  cerr << endl;

  analyse_recolor();
  analyse_similarity_sets();
  analyse_PCA_runtime();
  analyse_PCA_runtime_4D();

  // -> done
  int timetotal=(timeGetTime()-tmstart);
  cerr << "Total analyse time:  (" << timetotal/1000 << " s. " << (timetotal % 1000) << " ms)" << endl;
  m_iTimeTotal=timetotal;

  // ======================================================
  // -> analyse is complete

  // SAT: took out writing of extra data
  /*
  // -> write variance graph data
  for (int l=0;l<m_iNbLevels;l++) {
    static char str[1024];
    // exemplar transform
    StringCchPrintfA(str,1024,"%s.%d.graph.trsf.txt",rootName().c_str(),l);
    ofstream pca_recolor_graph(str);
    for (int i=0;i<m_RecolorVarianceCapturedGraph[l].size();i++)
      pca_recolor_graph << m_RecolorVarianceCapturedGraph[l][i] << endl;
    pca_recolor_graph.close();
    // runtime synthesis
    StringCchPrintfA(str,1024,"%s.%d.graph.runtime.txt",rootName().c_str(),l);
    ofstream pca_runtime_graph(str);
    for (int i=0;i<m_RuntimeVarianceCapturedGraph[l].size();i++)
      pca_runtime_graph << m_RuntimeVarianceCapturedGraph[l][i] << endl;
    pca_runtime_graph.close();
    // variance lost by other schemes
    StringCchPrintfA(str,1024,"%s.%d.reconstruction.txt",rootName().c_str(),l);
    ofstream variance_loss(str);
    variance_loss << hform("pca error %1.4f, constant neighborhood error %1.4f, ratio: %f\n",
      m_StandardSchemeReconstructionError[l][0],
      m_StandardSchemeReconstructionError[l][1],
      m_StandardSchemeReconstructionError[l][2]);
    variance_loss.close();
  }
  */

}


// ------------------------------------------------------------------------


void Analyser::analyse_recolor()
{
  // ======================================================
  // -> recoloring exemplar (computing appearence vectors)
  if (!m_Exemplar->recoloredStackIsPresent()) { 
    cerr << "Recoloring ... " << endl;
    long tmstart=timeGetTime();

    m_Exemplar->recolor();

    cerr << endl;
    cerr << endl;

    cerr << "done.";
    int tmrecolor=(timeGetTime()-tmstart);
    cerr << "  (" << tmrecolor/1000 << " s. " << (tmrecolor % 1000) << " ms)" << endl;
    m_iTimeRecolor=tmrecolor;
  } else {
    cerr << "Recoloring skipped: imported from external source." << endl;
    m_iTimeRecolor=0;
  }
}


// ------------------------------------------------------------------------


void Analyser::analyse_similarity_sets()
{
  // ======================================================
  // -> similarity sets
  cerr << "Computing similarity sets ... " << endl;
  long tmstart=timeGetTime();

  m_Exemplar->computeSimilaritySets();

  cerr << endl;
  cerr << endl;

  cerr << "done.";
  m_iTimeKNearests=(timeGetTime()-tmstart);
  cerr << "  (" << m_iTimeKNearests/1000 << " s. " << (m_iTimeKNearests % 1000) << " ms)" << endl;
}


// ------------------------------------------------------------------------


void Analyser::analyse_PCA_runtime()
{
  // ======================================================
  // -> computing pca on neighborhoods for synthesis: 8D version
  cerr << "Computing PCA (synthesis neighborhoods) ... ";
  long tmstart=timeGetTime();
  m_PCASynth.resize(m_iNbLevels);
  // -> pca on every level
  for (int l=0;l<m_Exemplar->nbLevels();l++)
  {
    // -> build neighborhoods
    int w=m_Exemplar->recoloredStack(l)->getWidth();
    int h=m_Exemplar->recoloredStack(l)->getHeight();
    assertx(w == m_Exemplar->stack(l)->getWidth() && h == m_Exemplar->stack(l)->height());
    assertx(m_Exemplar->recoloredStack(l)->numComp() == NUM_RECOLORED_PCA_COMPONENTS);
    m_PCASynth[l].setNumDim(
      NeighborhoodSynth_NUM_NEIGHBORS*m_Exemplar->recoloredStack(l)->numComp());
    for (int i=0;i<w;i++) {
      for (int j=0;j<h;j++) {
        NeighborhoodSynth nsynth(
          m_Exemplar->recoloredStack(l),
          exemplar_accessor(l),
          (!m_Exemplar->isToroidal()) && l < (m_iNbLevels - NUM_LEVELS_WITHOUT_BORDER),
//          (!m_Exemplar->isToroidal()) && l < FIRST_LEVEL_WITH_BORDER,
          l,i,j);
        if (m_Exemplar->isToroidal() || !nsynth.isCrossing())
          m_PCASynth[l].addSampleByCopy(nsynth);
      }
    }
  }

  // -> compute PCA
  cerr << endl;
  for (int l=0;l<m_iNbLevels;l++)
  {
    m_PCASynth[l].computePCA();
    m_RuntimeVarianceCaptured[l]=m_PCASynth[l].percentEnergyInNDimensions(NUM_RUNTIME_PCA_COMPONENTS);
    m_PCASynth[l].listPercentEnergyInDimensions(m_PCASynth[l].getNumDim(),m_RuntimeVarianceCapturedGraph[l]);
    cerr << "     level " << l+1;
    cerr << " (percent energy in " 
      << NUM_RUNTIME_PCA_COMPONENTS << "/" << m_PCASynth[l].getNumDim() 
      << " comp.: " << m_RuntimeVarianceCaptured[l] << "%)"
      << endl;
  }

  // -> done
  cerr << "     done.";
  int timepcasynth=(timeGetTime()-tmstart);
  cerr << "  (" << timepcasynth/1000 << " s. " << (timepcasynth % 1000) << " ms)" << endl;
  m_iTimePCASynth=timepcasynth;
}


// ------------------------------------------------------------------------


void Analyser::analyse_PCA_runtime_4D()
{
  // ======================================================
  // -> computing pca on neighborhoods for synthesis: 4D version
  cerr << "Computing PCA (synthesis neighborhoods 4D) ... ";
  long tmstart=timeGetTime();
  m_PCASynth_4Drecoloring.resize(m_iNbLevels);
  // -> pca on every level
  for (int l=0;l<m_Exemplar->nbLevels();l++)
  {
    // -> keep only 4 dimension from recolored exemplar
    MultiDimFloatTexture *level_4D=new MultiDimFloatTexture(
      m_Exemplar->recoloredStack(l)->width(),
      m_Exemplar->recoloredStack(l)->height(),
      m_Exemplar->recoloredStack(l)->numComp());
    int w=level_4D->getWidth();
    int h=level_4D->getHeight();
    assertx(w == m_Exemplar->stack(l)->getWidth() && h == m_Exemplar->stack(l)->height());
    assertx(level_4D->numComp() == NUM_RECOLORED_PCA_COMPONENTS);
    for (int i=0;i<w;i++) {
      for (int j=0;j<h;j++) {
        // -> copy first four channels
        for (int c=0;c<4;c++)
          level_4D->set(i,j,c)=m_Exemplar->recoloredStack(l)->get(i,j,c);
        // -> zero out all channels above 4
        for (int c=4;c<level_4D->numComp();c++)
          level_4D->set(i,j,c)=0;
      }
    }
    // -> build neighborhoods
    m_PCASynth_4Drecoloring[l].setNumDim(NeighborhoodSynth_NUM_NEIGHBORS*level_4D->numComp());
    for (int i=0;i<w;i++) {
      for (int j=0;j<h;j++) {
        NeighborhoodSynth nsynth(
          level_4D, // neighborhood is build on 4d version
          exemplar_accessor(l),
          (!m_Exemplar->isToroidal()) && l < (m_iNbLevels - NUM_LEVELS_WITHOUT_BORDER),
//          (!m_Exemplar->isToroidal()) && l < FIRST_LEVEL_WITH_BORDER,
          l,i,j);
        if (m_Exemplar->isToroidal() || !nsynth.isCrossing())
          m_PCASynth_4Drecoloring[l].addSampleByCopy(nsynth);
      }
    }
  }

  // -> compute PCA
  cerr << endl;
  for (int l=0;l<m_iNbLevels;l++)
  {
    m_PCASynth_4Drecoloring[l].computePCA();
    float varianceRetained=m_PCASynth_4Drecoloring[l].percentEnergyInNDimensions(NUM_RUNTIME_PCA_COMPONENTS);
    cerr << "     level " << l+1;
    cerr << " (percent variance in " 
      << NUM_RUNTIME_PCA_COMPONENTS << "/" << m_PCASynth_4Drecoloring[l].getNumDim() 
      << " comp.: " << varianceRetained << "%)"
      << endl;
  }
}

// ------------------------

// DEBUG
/*
  // test quantizer by comparing old and new quantizations

  cerr << "Quantizer sanity check ... ";
  
  m_Exemplar->computeQuantizedNeighborhoods(m_PCART);

  Quantizer q(m_Exemplar->recoloredStack(0),8,97.0f);
  int numdiff=0;
  for (int j=0;j<m_Exemplar->recoloredStack(0)->height();j++)
  {
    for (int i=0;i<m_Exemplar->recoloredStack(0)->width();i++)
    {
      const Exemplar::quantized& qn=m_Exemplar->getQuantizedNeighborhood(0,i,j);
      for (int c=0;c<m_Exemplar->recoloredStack(0)->numComp();c++)
      {
        if (q.quantized()->get(i,j,c) != qn.tbl[c])
        {
          //cerr << q.quantized()->get(i,j,c) << " =? " << qn.tbl[c] << endl;
          numdiff++;
        }
      }
    }
  }
  cerr << "number of differences = " << numdiff << endl;

  cerr << "mean quantizer = ";
  for (int c=0;c<m_Exemplar->recoloredStack(0)->numComp();c++)
    cerr << q.mean(c) << ',';
  cerr << endl;
  
  cerr << "mean ref = ";
  for (int c=0;c<m_Exemplar->recoloredStack(0)->numComp();c++)
    cerr << m_PCART[0].getProjAverage(c) << ',';
  cerr << endl;
  
  cerr << "stddev quantizer = ";
  for (int c=0;c<m_Exemplar->recoloredStack(0)->numComp();c++)
    cerr << q.stddev(c) << ',';
  cerr << endl;
  
  cerr << "stddev ref = ";
  for (int c=0;c<m_Exemplar->recoloredStack(0)->numComp();c++)
    cerr << m_PCART[0].getStdDev(c) << ',';
  cerr << endl;

  CTexture *errtex = q.error();
  CTexture::saveTexture(errtex,"quantize_error.png");
  delete (errtex);

  cerr << "done." << endl;
*/


// ------------------------------------------------------------------------


pair<float,float> Analyser::eigenVectorNonSquareCompMinMaxMean(PCA_Synth& pca,int v,float& _mean)
{
  float compmin [3];
  float compmax [3];
  float compmean[3];

  for (int c=0;c<3;c++)
  {
    compmin[c]  =  FLT_MAX;
    compmax[c]  = -FLT_MAX;
    compmean[c] = 0.0f;
  }

  // find min / max   - only on three first comp
  for (int j=0;j<NeighborhoodSynth_NUM_NEIGHBORS;j++)
  {
      float  comp[3];

      for (int c=0;c<3;c++)
        comp[c] = pca.getEigenVect(v,j*NeighborhoodSynth_NUM_COMP+c);

      for (int c=0;c<3;c++)
      {
        compmin[c]  =  min(compmin[c],comp[c]);
        compmax[c]  =  max(compmax[c],comp[c]);
        compmean[c] += comp[c];
      }
  }
  float allcompmin =  FLT_MAX;
  float allcompmax = -FLT_MAX;
  float allmean    = 0.0f;
  for (int c=0;c<3;c++)
  {
    allcompmin   = min(compmin[c],allcompmin);
    allcompmax   = max(compmax[c],allcompmax);
    compmean[c] /= NeighborhoodSynth_NUM_NEIGHBORS;
    allmean += compmean[c];
  }
  allmean /= (float)3;

  _mean = allmean;

  return make_pair(allcompmin,allcompmax);
}


// ------------------------------------------------------------------------


CTexture *Analyser::eigenVectorNonSquare2Texture(PCA_Synth& pca,int v,bool rescale)
{
  CTexture *res=new CTexture(NeighborhoodSynth_SIZE,NeighborhoodSynth_SIZE,false);

  float allmean=0.0f;
  pair<float,float> minmax = Analyser::eigenVectorNonSquareCompMinMaxMean(pca,v,allmean);

  const char tbl[NeighborhoodSynth_SIZE*NeighborhoodSynth_SIZE]=NeighborhoodSynth_SHAPE;

  int num=0;
  for (int j=0;j<res->getHeight();j++)
  {
    for (int i=0;i<res->getWidth();i++)
    {
      float  comp[3]; // limit to 3 first components
      for (int c=0;c<3;c++)
        comp[c]=0.0f;

      if (tbl[i+j*NeighborhoodSynth_SIZE] == ' ')
        continue;

      for (int c=0;c<3;c++)
        comp[c] = pca.getEigenVect(v , num*NeighborhoodSynth_NUM_COMP+c);

      float r=0,g=0,b=0;

      if (rescale)
      {
        r=(comp[0]-minmax.first)/(minmax.second-minmax.first);
        g=(comp[1]-minmax.first)/(minmax.second-minmax.first);
        b=(comp[2]-minmax.first)/(minmax.second-minmax.first);
      }
      else
      {
        // search min/max over all components of all vectors
        float allmin =  FLT_MAX;
        float allmax = -FLT_MAX;
        for (int k=0;k<NeighborhoodSynth::e_numdim;k++)
        {
          float tmp=0.0f;
          pair<float,float> mM = Analyser::eigenVectorNonSquareCompMinMaxMean(pca,k,tmp);
          allmin=min(mM.first ,allmin);
          allmax=max(mM.second,allmax);
        }
        // map 0 to 0.5
        r=0.5f + 0.5f*(comp[0])/(allmax-allmin);
        g=0.5f + 0.5f*(comp[1])/(allmax-allmin);
        b=0.5f + 0.5f*(comp[2])/(allmax-allmin);
      }

      res->getData()[(i+j*NeighborhoodSynth_SIZE)*3  ]=(unsigned char)min(max((r*255.0f),0.0f),255.0f);
      res->getData()[(i+j*NeighborhoodSynth_SIZE)*3+1]=(unsigned char)min(max((g*255.0f),0.0f),255.0f);
      res->getData()[(i+j*NeighborhoodSynth_SIZE)*3+2]=(unsigned char)min(max((b*255.0f),0.0f),255.0f);

      num++;
    }
  }
  assertx(num == NeighborhoodSynth_NUM_NEIGHBORS);
  return (res);
}


// ------------------------------------------------------------------------


void Analyser::report(const char *dir)
{
  static char str[1024];

  // output non-square eigen vectors
  for (int l=0;l<m_iNbLevels;l++)
  {
    CTexture *evtex=new CTexture(
      (NeighborhoodSynth_SIZE+2)*(NeighborhoodSynth::e_numdim+1),
      (NeighborhoodSynth_SIZE+2)*2,false);
    for (int i=0;i<NeighborhoodSynth::e_numdim;i++)
    {
      CTexture *tex=eigenVectorNonSquare2Texture(m_PCASynth[l],i,true);
      evtex->copy(i*(NeighborhoodSynth_SIZE+2),0,tex);
      delete (tex);
      tex=eigenVectorNonSquare2Texture(m_PCASynth[l],i,false);
      evtex->copy(i*(NeighborhoodSynth_SIZE+2),(NeighborhoodSynth_SIZE+2),tex);
      delete (tex);
    }
    StringCchPrintfA(str,1024,"%s\\eigenvects.%d.png",dir,l);
    CTexture::saveTexture(evtex,str);   
    delete (evtex);
  }

/*
  // output recolored exemplar
  for (int l=0;l<m_iNbLevels;l++)
  {
    CTexture *ch02=m_Exemplar->recoloredStack(l)->toRGBTexture(0,128.0f);
    sprintf(str,"%s\\recolored.%d_02.png",dir,l);
    CTexture::saveTexture(ch02,str);
    delete (ch02);
    CTexture *ch35=m_Exemplar->recoloredStack(l)->toRGBTexture(3,128.0f);
    sprintf(str,"%s\\recolored.%d_35.png",dir,l);
    CTexture::saveTexture(ch35,str);
    delete (ch35);
    CTexture *ch67=m_Exemplar->recoloredStack(l)->toRGBTexture(6,128.0f);
    sprintf(str,"%s\\recolored.%d_67.png",dir,l);
    CTexture::saveTexture(ch67,str);
    delete (ch67);
  }
*/

  // output feature distance
  if (m_Exemplar->stack(0)->numComp() == 4) {
    float minv=999999.0f,maxv=0.0f;
    for (int j=0;j<m_Exemplar->stack(0)->height();j++) {
      for (int i=0;i<m_Exemplar->stack(0)->width();i++) {
        minv=min(minv,m_Exemplar->stack(0)->get(i,j,3));
        maxv=max(maxv,m_Exemplar->stack(0)->get(i,j,3));
      }
    }
    CTexture *dist=new CTexture(m_Exemplar->stack(0)->width(),m_Exemplar->stack(0)->height(),false);
    for (int j=0;j<m_Exemplar->stack(0)->height();j++) {
      for (int i=0;i<m_Exemplar->stack(0)->width();i++) {
        unsigned char v=unsigned char(255.0f*(m_Exemplar->stack(0)->get(i,j,3)-minv)/(maxv-minv));
        for (int c=0;c<2;c++)
          dist->set(i,j,c)=v;
        if (m_Exemplar->stack(0)->get(i,j,3) >= 0) {
          dist->set(i,j,2)=0;
        } else {
          dist->set(i,j,2)=255;
        }
      }
    }
    StringCchPrintfA(str,1024,"%s\\featuredist.png",dir);
    CTexture::saveTexture(dist,str);
    delete (dist);
  }

  // create HTML page
  StringCchPrintfA(str,1024,"%s\\analyse.html",dir);
  ofstream report(str);

  int w=256;

  GLOBALPARAMETERS.list(report);

  report << "<UL>" << endl;
  report << hform("<LI> Simset neighborhood size: %dx%d\n",
    Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1,Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1);
  if (Exemplar::s_bUseRecoloredForSimset)
    report << hform("<LI> <font color=\"#ff0000\"><b> Used recolored exemplar to compute simsets </b></font>");
  report << hform("<LI> Recoloring neighborhood size: %dx%d\n",
    NEIGHBORHOOD_RADIUS_RECOLOR*2+1,NEIGHBORHOOD_RADIUS_RECOLOR*2+1);
  report << "<LI> Transform computation time  " 
    << m_iTimeRecolor/60000 << " m. " << (m_iTimeRecolor/1000)%60 << " s. " 
    << (m_iTimeRecolor % 1000) << " ms." << endl;
  report << "<LI> Run-time PCA computation time  " 
    << m_iTimePCASynth/60000 << " m. " << (m_iTimePCASynth/1000)%60 << " s. " 
    << (m_iTimePCASynth % 1000) << " ms." << endl;
  report << "<LI> K-nearests computation time  " 
    << m_iTimeKNearests/60000 << " m. " << (m_iTimeKNearests/1000)%60 << " s. " 
    << (m_iTimeKNearests % 1000) << " ms." << endl;
  report << "<LI> <font color=\"#ff0000\"><b> precision = " << Exemplar::s_fPrecision << "</b></font>" << endl;
  report << "<LI> Total pre-computation time  " 
    << m_iTimeTotal/60000 << " m. " << (m_iTimeTotal/1000)%60 << " s. " 
    << (m_iTimeTotal % 1000) << " ms." << endl;
  report << "</UL>" << endl;

  report << "<hr><br>" << endl;
  report << "<h2>Exemplars</h2><br>" << endl;
  report << "<TABLE>" << endl;

  report << "<TR>" << endl;
  StringCchPrintfA(str,1024,"%s\\%s.png",dir,rootName().c_str());
  CTexture *tmp=m_Exemplar->stack(0)->toRGBTexture();
  CTexture::saveTexture(tmp,str);
  delete (tmp);
  report << "<TD><a href=\"" << rootName() << ".png\"><img src=\"" << rootName() << ".png\"></a></TD>" << endl;
  report << "<TD> periodx = " << m_Exemplar->getPeriodX() 
    << " periody = " << m_Exemplar->getPeriodY() 
    << (m_Exemplar->isToroidal()?" toroidal ":"") << "</TD>" << endl;

  if (m_Exemplar->stack(0)->numComp() == 4) {
    report << "<TD><a href=\".featuredist.png\"><img src=\"featuredist.png\"></a></TD>" << endl;
  }

  if (m_Exemplar->isConstrained())
  {
    StringCchPrintfA(str,1024,"%s\\%s.png",dir,rootName().c_str());
    CTexture *tmp=m_Exemplar->constraint(0)->toRGBTexture();
    CTexture::saveTexture(tmp,str);
    delete (tmp);
    report << "<TD><a href=\"" << rootName() << ".cstr.png\"><img src=\"" << rootName() << ".cstr.png\"></a></TD>" << endl;
  }
  report << "</TR>" << endl;    
  report << "</TABLE>" << endl;

  report << "<TABLE>" << endl;
  for (int i=0;i<m_iNbLevels;i++)
  {
    report << "<TR>";
    report << hform("<TD> level %d, variance recoloring %f (%d/%d) </TD>",i,
      m_RecolorVarianceCaptured[i],m_RecolorNumDimUsed[i],
       (Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)
      *(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)
      *m_Exemplar->stack(0)->numComp());
    report << hform("<TD>           variance runtime    %f (%d/%d) </TD>",
      m_RuntimeVarianceCaptured[i],NUM_RUNTIME_PCA_COMPONENTS,NeighborhoodSynth_NUM_NEIGHBORS*m_RecolorNumDimUsed[i]);
    for (int n=0;n<m_RecolorVarianceCapturedGraph[i].size();n++) {
      report << hform("<TD>%d %f</TD>",n,m_RecolorVarianceCapturedGraph[i][n]);
    }
    report << "</TR>\n";
  }
  report << "</TABLE>" << endl;

  report << "<hr><br>" << endl;
  report << "<h2>Eigen vectors</h2><br>" << endl;
  report << "<TABLE>" << endl;
  for (int i=0;i<m_iNbLevels;i++)
  {
    report << "<TR>" << endl;
    report << "<TD><a href=\"eigenvects." << i << ".png\"><img src=\"eigenvects." << i << ".png\" height=\"32\"></a></TD>" << endl;
    report << "</TR>" << endl;    
  }
  report << "</TABLE>" << endl;

  report.close();
}


// ------------------------------------------------------------------------


void Analyser::save(const char *fname)
{
  static char str[MAX_PATH];

  if (fname == NULL)
    StringCchCopyA(str,MAX_PATH,(rootName()+".analyse").c_str());
  else
    StringCchCopyA(str,MAX_PATH,fname);

  ofstream otest(str);
  // saving parameters for compatibility check
  otest << K_NEAREST << endl;
#ifdef K_NEAREST_DIVERSITY
  otest << K_NEAREST_DIVERSITY << endl;
#else
  otest << 0 << endl;
#endif
  otest << NEIGHBORHOOD_RADIUS_RECOLOR << endl;
  otest << Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET << endl;
  otest << NUM_RECOLORED_PCA_COMPONENTS << endl;
  otest << NUM_RUNTIME_PCA_COMPONENTS  << endl;
#ifdef USE_STACK_EXEMPLAR
  otest << 1 << endl;
#else
  otest << 0 << endl;
#endif
#ifdef USE_CENTER_PIXEL
  otest << 1 << endl;
#else
  otest << 0 << endl;
#endif

  // saving ...
  otest << m_iNbLevels << endl;
  otest << (int)1 << endl;  // for backward compatibility
  // pca synth
  for (int l=0;l<(int)m_PCASynth.size();l++)
    m_PCASynth[l].save(otest);
  // exemplar
  m_Exemplar->save(this,otest);
  // pca synth - 4D
  for (int l=0;l<(int)m_PCASynth_4Drecoloring.size();l++)
    m_PCASynth_4Drecoloring[l].save(otest);
  // done
  otest.close();
}


// ------------------------------------------------------------------------


int Analyser::load(const char *fname)
{
  SimpleTimer tm("\n[Analyser::load] %02d min %02d sec %d ms\n");

  static char str[MAX_PATH];

  if (fname == NULL)
  {
    WIN32_FIND_DATAA FindFileData;
    HANDLE hFind;

    hFind = FindFirstFileA("*.analyse", &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
    {
      return (ANALYSER_DATA_NOT_FOUND);
    } 
    else 
    {
      StringCchCopyA(str,MAX_PATH,FindFileData.cFileName);
      FindClose(hFind);
    }
  }
  else
    StringCchCopyA(str,MAX_PATH,fname);

  bool updated=false;

  ifstream itest(str);
  if (!itest)
    return (ANALYSER_DATA_NOT_FOUND);
  // compatibility check
  int v;
  itest >> v;
  if (v < K_NEAREST)
    return (ANALYSER_DATA_EXISTS);
  itest >> v;
#ifdef K_NEAREST_DIVERSITY
  if (v < K_NEAREST_DIVERSITY)
    return (ANALYSER_DATA_EXISTS);
#else
  if (v != 0)
    return (ANALYSER_DATA_EXISTS);
#endif
  itest >> v;
  if (v != NEIGHBORHOOD_RADIUS_RECOLOR)
    return (ANALYSER_DATA_EXISTS);
  itest >> v;
  //if (v != Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET) // no longer compilation dependent
  //  return (ANALYSER_DATA_EXISTS);
  itest >> v;
  if (v != NUM_RECOLORED_PCA_COMPONENTS)
    return (ANALYSER_DATA_EXISTS);
  itest >> v;
  if (v != NUM_RUNTIME_PCA_COMPONENTS)
    return (ANALYSER_DATA_EXISTS);
  itest >> v;
#ifdef USE_STACK_EXEMPLAR
  if (v != 1)
    return (ANALYSER_DATA_EXISTS);
#else
  if (v != 0)
    return (ANALYSER_DATA_EXISTS);  
#endif
  itest >> v;
#ifdef USE_CENTER_PIXEL
  if (v != 1)
    return (ANALYSER_DATA_EXISTS);
#else
  if (v != 0)
    return (ANALYSER_DATA_EXISTS);  
#endif

  // loading ...
  itest >> m_iNbLevels;

  m_PCASynth.resize(m_iNbLevels);
  m_PCASynth_4Drecoloring.resize(m_iNbLevels);

  int nbex;
  itest >> nbex; // for backward compatibility
  assertx(nbex == 1);

  {
    SimpleTimer tm("\n[Analyser::load PCA Synth] %02d min %02d sec %d ms\n");
    // pca
    for (int l=0;l<(int)m_PCASynth.size();l++) {
      bool success=m_PCASynth[l].load(itest);
      assertx(success);
    }
  }

  {
    SimpleTimer tm("\n[Analyser::load Exemplar] %02d min %02d sec %d ms\n");
    // exemplar
    m_Exemplar=new Exemplar(this,itest);
  }

  {
    SimpleTimer tm("\n[Analyser::load PCA Synth 4D] %02d min %02d sec %d ms\n");
    // pca 4D
    bool recompute=false;
    for (int l=0;l<(int)m_PCASynth_4Drecoloring.size();l++) {
      bool success=m_PCASynth_4Drecoloring[l].load(itest);
      if (!success) {
        if (l == 0) {
          recompute = true;
          break;
        } else {
          assertx(false);
        }
      }
    }
    if (recompute) {
      analyse_PCA_runtime_4D();
      updated=true;
    }
  }

  // done
  itest.close();
  // if updated, save again
  if (updated)
    save(fname);
  // done
  return (ANALYSER_DONE);
}

// ------------------------------------------------------------------------

const vector<PCA_Synth>&   Analyser::pcaSynth() const
{
  if (GLOBALPARAMETERS.isDefined("4D")) {
    return (m_PCASynth_4Drecoloring);
  } else {
    return (m_PCASynth);
  }
}

// ------------------------------------------------------------------------

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// ------------------------------------------------------------------------
