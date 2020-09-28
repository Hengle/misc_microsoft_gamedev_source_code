// -----------------------------------------------------

#include "CExemplar.h"
#include "CAnalyser.h"

#include "Geometry.h"
#include "Spatial.h"

#include "Stat.h"

#include <windows.h>
#include <string>
#include <fstream>
#include <strsafe.h>

#include "filetools.h"

#include "CSimpleTimer.h"

#include "CMultiDimFloatTexture.h"
#include "CGlobalParameters.h"
#include "mem_tools.h"

#ifndef PCTS_NO_ANALYSIS
#ifdef USE_ANN_LIBRARY
#include <ANN.h>
#endif
#endif

// #define SAVE_STACK

#include "CGlobalParameters.h"
#include "TClustering.h"

// -----------------------------------------------------

#define FLAG_CONSTRAINT    1
#define FLAG_FMAP          2
#define FLAG_PRTCOLORMAP   4

#define GAUSS_KERNEL_SIGMA 1.0f/3.0f

// -----------------------------------------------------

float Exemplar::s_fPrecision=1.0f;
float Exemplar::s_fEpsilon  =0.0f;


int   Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET = 2;
bool  Exemplar::s_bUseRecoloredForSimset      = false;

// -----------------------------------------------------

Exemplar::Exemplar(int id,
                   Analyser *analyser,
                   MultiDimFloatTexture *image,
                   bool toroidal,int px,int py,bool terrain)
{
  m_iExemplarId =id;
  m_bToroidal   = toroidal;
  m_iPeriodX    = px;
  m_iPeriodY    = py;
  m_bTerrain    = terrain;
  m_Analyser    = analyser;
  m_PRTColorMap    = NULL;
  m_PRTReflectance = NULL;

  MultiDimFloatTexture *tmp=new MultiDimFloatTexture(image);

#ifdef SAVE_STACK
  CTexture *test=enlargeTexture(image,0);
  CTexture::saveTexture(test,"__enlarged.png");
  vector<CTexture *> lvls;
  computeStackLevels(test,lvls);
  return;
#endif

  m_iNbLevels=computeStackLevels(image,toroidal,m_Stack);
  computePyramidLevels(image,m_Pyramid);

  m_Stack[0]->setName(image->getName());

}


// -----------------------------------------------------


Exemplar::Exemplar(int id,
                   Analyser       *analyser,
                   MultiDimFloatTexture *image,
                   MultiDimFloatTexture *constraint,
                   bool toroidal,int px,int py,bool terrain)
{
  m_iExemplarId = id;
  m_bToroidal   = toroidal;
  m_iPeriodX    = px;
  m_iPeriodY    = py;
  m_bTerrain    = terrain;
  m_Analyser    = analyser;
  m_PRTColorMap    = NULL;
  m_PRTReflectance = NULL;

  if (constraint->getWidth() != image->getWidth() 
    || constraint->getHeight() != image->getHeight())
    throw SynthesisException("Exemplar - texture %s and constraint %s differs in size !",image->getName(),constraint->getName());

  MultiDimFloatTexture *tmp=new MultiDimFloatTexture(image);

  m_iNbLevels=computeStackLevels(image     , toroidal, m_Stack);
  int       n=computeStackLevels(constraint, toroidal, m_Constraints);
  assert(m_iNbLevels == n);
  computePyramidLevels(image,                          m_Pyramid);

  m_Stack[0]      ->setName(image->getName());
  m_Constraints[0]->setName(constraint->getName());
}


// -----------------------------------------------------

// load from stream
Exemplar::Exemplar(Analyser *analyser,istream& i)
{
  SimpleTimer tm("\n[Exemplar::Exemplar(istream& i)] %02d min %02d sec %d ms\n");

  m_Analyser = analyser;
  m_PRTColorMap = NULL;
  m_PRTReflectance = NULL;

  string name;
  // read id
  i >> m_iExemplarId;
  // read exemplar flags
  int flags;
  i >> flags;

  m_bToroidal = (flags & (int)EXEMPLAR_TOROIDAL_FLAG) != 0 ? true : false;
  m_bTerrain  = (flags & (int)EXEMPLAR_TERRAIN_FLAG)  != 0 ? true : false;    

  cerr << "\n\n\n TOROIDAL = " << m_bToroidal << "    TERRAIN = " << m_bTerrain << "\n\n\n";

  // read period along x
  i >> m_iPeriodX;
  // read period along y
  i >> m_iPeriodY;
  //m_iPeriodX=1;
  //m_iPeriodY=1;

  // read number of levels
  i >> m_iNbLevels;
  // read exemplar name
  i >> name;
  MultiDimFloatTexture *image=new MultiDimFloatTexture(name.c_str());
  {
    SimpleTimer tm("\n[Exemplar::Exemplar(istream& i) - computeLevels - m_Stack] %02d min %02d sec %d ms\n");
    assertx(computeStackLevels(image, m_bToroidal, m_Stack) == m_iNbLevels);
    computePyramidLevels(image,                    m_Pyramid);
  }
  m_Stack[0]->setName(image->getName());
  // read if constraint or feature map is present
  int present;
  i >> present;
  if (present & FLAG_CONSTRAINT)
  {
    // read constraint texture name
    i >> name;
    MultiDimFloatTexture *constraint=new MultiDimFloatTexture(name.c_str());
    {
      SimpleTimer tm("\n[Exemplar::Exemplar(istream& i) - computeLevels - m_Constraints] %02d min %02d sec %d ms\n");
      assertx(computeStackLevels(constraint, m_bToroidal, m_Constraints) == m_iNbLevels);
    }
    m_Constraints[0]->setName(constraint->getName());
  }
  if (present & FLAG_FMAP)
  {
    // NOTE: feature map is no longer saved
    //       this is done for backward comp.
    i >> name;
//    CTexture *distmap=CTexture::loadTexture(name.c_str());
//    computePyramidLevels(distmap,m_FeatureDistanceMaps);
//    delete (distmap);
  }
  if (present & FLAG_PRTCOLORMAP)
  {
    i >> name;
    m_PRTColorMap=CTexture::loadTexture(name.c_str());
  }

  {
    SimpleTimer tm("\n[Exemplar::Exemplar(istream& i) - knearests tables] %02d min %02d sec %d ms\n");

    // read knearests tables
    m_SimilaritySet.resize(m_iNbLevels);
    for (int l=0;l<m_iNbLevels;l++)
    {
      int nbkn;
      i >> nbkn;
      m_SimilaritySet[l].resize(stack(l)->getWidth()*stack(l)->getHeight());
      for (int k=0;k<nbkn;k++)
      {
        i >> name;
        if (k < K_NEAREST)
        {
          CTexture *kns=CTexture::loadTexture(name.c_str());
          for (int ti=0;ti<kns->getWidth();ti++)
          {
            for (int tj=0;tj<kns->getHeight();tj++)
            {
              if (kns->get(ti,tj,2) < 255)
                m_SimilaritySet[l][ti+tj*stack(l)->getWidth()]
                .push_back( knearest_id(l, kns->get(ti,tj,0), kns->get(ti,tj,1), m_iExemplarId ) );
            }
          }
          delete (kns);
        }
      }
    }
  }

  // load recolored exemplar
  m_RecoloredStack.clear();
  for (int l=0;l<nbLevels();l++)
  {
    i >> name;
    MultiDimFloatTexture *recolored = new MultiDimFloatTexture(name);
    m_RecoloredStack.push_back(recolored);
  }

}


// -----------------------------------------------------


void Exemplar::save(const Analyser *a,ostream& o) const
{
  static char  str[1024];

  string rootname=a->rootName();

  // save id
  o << m_iExemplarId << endl;
  // save toroidal
  o << ((m_bToroidal?EXEMPLAR_TOROIDAL_FLAG:0) | (m_bTerrain?EXEMPLAR_TERRAIN_FLAG:0)) << endl;
  // period along x
  o << m_iPeriodX << endl;
  // period along y
  o << m_iPeriodY << endl;
  // save number of levels
  o << nbLevels() << endl;
  // save exemplar
  StringCchPrintfA(str,1024,"%s.ex.float",rootname.c_str());
  if (m_PRTReflectance == NULL) {
    m_Stack[0]->save(str);
  } else {
    m_PRTReflectance->save(str); // replace by reflectance
  }
  o << str << endl;
  // save contraint and feature map
  int present=0;
  if (!m_Constraints.empty())         present=present | FLAG_CONSTRAINT;
//  if (!m_FeatureDistanceMaps.empty()) present=present|2; // NOTE: feature map is no longer saved
  if (m_PRTColorMap!=NULL)            present=present | FLAG_PRTCOLORMAP;
  o << present << endl;
  // -> constraint
  if (!m_Constraints.empty())
  {
    StringCchPrintfA(str,1024,"%s.cstr.float",rootname.c_str());
    m_Constraints[0]->save(str);
    o << str << endl;
  }
  /*
  // -> feature map
  if (!m_FeatureDistanceMaps.empty())
  {
    sprintf(str,"%s.fmap.png",rootname.c_str());
    CTexture::saveTexture(m_FeatureDistanceMaps[0],str);
    o << str << endl;
  }
  */
  // -> PRT color map
  if (m_PRTColorMap != NULL)
  {
    StringCchPrintfA(str,1024,"%s.albedo.png",rootname.c_str());
    CTexture::saveTexture(m_PRTColorMap,str);
    o << str << endl;
  }
  // save k-nearests tables
  for (int l=0;l<nbLevels();l++)
  {
    vector<CTexture *> kns;
    kNearestsTableTextures(l,kns);
    o << (int)kns.size() << endl;
    for (int k=0;k<(int)kns.size();k++)
    {
       // SAT: changed to use .tga instead
       /*
       StringCchPrintfA(str,1024,"%s.kns.%d.%d.png",rootname.c_str(),l,k);
       CTexture::saveTexture(kns[k],str);

       StringCchPrintfA(str,1024,"%s.kns.%d.%d.tga",rootname.c_str(),l,k);
       CTexture::saveTexture(kns[k],str);
       */

       // SAT: changed to save as binary instead
       {
          StringCchPrintfA(str,1024,"%s.kns.%d.%d.bin",rootname.c_str(),l,k);
          FILE *file=fopen(str,"wb");

          if (file == NULL)
             throw CLibTextureException("Unable to save %s",str);

          cerr << "Saving BIN image " << str << " ... " << std::endl;;

          int memsizebytes = kns[k]->getHeight() * kns[k]->getWidth() * kns[k]->numComp();

          fwrite(&memsizebytes, sizeof(int), 1, file);

          // need to swap Red and Blue channels
          bool bSwapReadBlue = false;
          if(bSwapReadBlue)
          {
             /****************************/
             /* Swap red and blue colors */
             /****************************/
             BYTE *texels = new BYTE [memsizebytes];
             if (texels != NULL)
             {
                int width = kns[k]->getWidth();
                int height = kns[k]->getHeight();
                int numComp = kns[k]->numComp();
                int i;

                BYTE *ptr1 = kns[k]->getData();
                BYTE *ptr2 = texels;
                for (i=0; i<(width*height); i++)
                {
                   ptr2[0] = ptr1[2];
                   ptr2[1] = ptr1[1];
                   ptr2[2] = ptr1[0];

                   if (numComp == 3)
                   {
                      ptr1 += 3;
                      ptr2 += 3;
                   }
                   else if (numComp == 4)
                   {
                      ptr2[3] = ptr1[3];
                      ptr1 += 4;
                      ptr2 += 4;
                   }
                }
             }
             fwrite(texels, 1, memsizebytes, file);
          }
          else
          {
             fwrite(kns[k]->getData(), 1, memsizebytes, file);
          }

          fclose(file);
       }

      o << str << endl;
      delete (kns[k]);
    }
  }
  // save recolored stack
  for (int l=0;l<nbLevels();l++)
  {
      StringCchPrintfA(str,1024,"%s.recolored.%d.float",rootname.c_str(),l);
      m_RecoloredStack[l]->save(str);
      o << str << endl;
/*
      // sanity check
      MultiDimFloatTexture *test=new MultiDimFloatTexture(str);
      assertx(test->width() == m_RecoloredStack[l]->width()
        && test->height() == m_RecoloredStack[l]->height()
        && test->numComp() == m_RecoloredStack[l]->numComp());
      for (int j=0;j<test->height();j++)
        for (int i=0;i<test->width();i++)
          for (int c=0;c<test->numComp();c++)
            assertx(test->get(i,j,c) == m_RecoloredStack[l]->get(i,j,c));
      delete (test);
*/
  }
}


// -----------------------------------------------------


Exemplar::~Exemplar()
{
  for (int i=0;i<(int)m_Stack.size();i++)
    delete (m_Stack[i]);
  for (int i=0;i<(int)m_Pyramid.size();i++)
    delete (m_Pyramid[i]);
  for (int i=0;i<(int)m_Constraints.size();i++)
    delete (m_Constraints[i]);
  for (int i=0;i<(int)m_RecoloredStack.size();i++)
    delete (m_RecoloredStack[i]);
  if (m_PRTColorMap) delete (m_PRTColorMap);
}


// -----------------------------------------------------


int Exemplar::computeStackLevels(MultiDimFloatTexture *tex,bool toroidal,
                                 vector<MultiDimFloatTexture *>& _lvls)
{
  MultiDimFloatTexture *large=enlargeTexture(tex,toroidal ? 0 : 1);
  int nlvl=0;
  if (GLOBALPARAMETERS.isDefined("gauss") || FORCE_GAUSS)
  {
    cerr << "(using Gauss filter)" << endl;
    nlvl = computeStackLevels_Gauss(large,_lvls);
  }
  else
    nlvl=computeStackLevels_Box(large,_lvls);
  delete (large);
  return (nlvl);
}


// -----------------------------------------------------


int Exemplar::computePyramidLevels(MultiDimFloatTexture *tex,vector<MultiDimFloatTexture *>& _lvls)
{
  int nlvl=0;
  if (GLOBALPARAMETERS.isDefined("gauss") || FORCE_GAUSS)
  {
    cerr << "(using Gauss filter)" << endl;
    nlvl = computePyramidLevels_Gauss(tex,_lvls);
  }
  else
    nlvl=computePyramidLevels_Box(tex,_lvls);
  return (nlvl);
}


// -----------------------------------------------------

MultiDimFloatTexture *Exemplar::enlargeTexture(const MultiDimFloatTexture *ex,int type)
{
  SimpleTimer tm("\n[Exemplar::enlargeTexture] %02d min %02d sec %d ms\n");

  int w=ex->getWidth();
  int h=ex->getHeight();
  MultiDimFloatTexture *large=new MultiDimFloatTexture(w*2,h*2,ex->numComp());

  for (int j=0;j<h*2;j++)
  {
    for (int i=0;i<w*2;i++)
    {
      int ri,rj;

      // where to read ?
      if (type == 0)
      {
        // Toroidal
        // ri
        if (i < w/2)
          ri=(i+w/2)%w;
        else if (i < w+w/2)
          ri=i-w/2;
        else
          ri=((i-w/2) % w);
        // rj
        if (j < h/2)
          rj=(j+h/2)%h;
        else if (j < h+h/2)
          rj=j-h/2;
        else
          rj=((j-h/2) % h);
      }
      else
      {
        // Mirror
        // ri
        if (i < w/2)
          ri=(w/2 - i);
        else if (i < w+w/2)
          ri=i-w/2;
        else
          ri=(w-1 - (i-(w/2+w)));
        // rj
        if (j < h/2)
          rj=(h/2 - j);
        else if (j < h+h/2)
          rj=j-h/2;
        else
          rj=(h-1 - (j-(h/2+h)));
      }

      for (int c=0;c<ex->numComp();c++)
        large->set(i,j,c)=ex->get(ri,rj,c);
    }
  }
  return (large);
}


// -----------------------------------------------------

int Exemplar::computeStackLevels_Box(MultiDimFloatTexture *tex,
                                     vector<MultiDimFloatTexture *>& _lvls)
{
  static char str[256];
  SimpleTimer tm("\n[Exemplar::computeStackLevels] %02d min %02d sec %d ms\n");

  vector<MultiDimFloatTexture *> tmplvls;

  // compute stack of averages on large texture

  // -> add unmodifed finest level
  tmplvls.clear();
  tmplvls.push_back(tex);
  int l=0;

  MultiDimFloatTexture *lvl=tex;
  int num=(int)(0.01 + log2((float)lvl->getWidth()));

  do  // TODO: speed this up - extremely unefficient !!! (not critical though)
  {
    const stack_accessor_v2 access(l); // l is destination level for children
                                       // (children *are* at level l)

    MultiDimFloatTexture *stackl=new MultiDimFloatTexture(
      lvl->getWidth(),
      lvl->getHeight(),
      lvl->numComp());

    TableAllocator<float,true> avg(lvl->numComp());

    for (int i=0;i<stackl->getWidth();i++)
    {
      for (int j=0;j<stackl->getHeight();j++)
      {
        avg.zero();

        for (int li=0;li<2;li++)
        {
          for (int lj=0;lj<2;lj++)
          {
            int ci,cj;
            access.child_ij(i,j,li,lj,ci,cj);
            float *pix;
            pix=lvl->getpixmod(ci,cj);
            for (int c=0;c<stackl->numComp();c++)
              avg[c] += pix[c];
          }
        }
        
        float *opix=stackl->getpix(i,j);
        for (int c=0;c<stackl->numComp();c++)
          opix[c]=avg[c]/4.0f;
      }
    }

    lvl=stackl;
    tmplvls.push_back(lvl);
    l++;

  }  while ((1 << l) <= lvl->getWidth());

  // clamp stack
  int w=tex->getWidth()/2;
  int h=tex->getHeight()/2;
  _lvls.clear();

  l=0;
  int sz=w;
  for (vector<MultiDimFloatTexture *>::iterator L=tmplvls.begin();L!=tmplvls.end();L++)
  {
    int oi=0,oj=0;
    if (m_bTerrain)
    {
      // offset levels to reproduce primal subdivision stack, 
      // while still following dual subdivision child links
      if (l > 0)
      {
        oi = -(1 << (l-1));
        oj = -(1 << (l-1));
      }
      else
      {
        oi = 0;
        oj = 0;
      }
    }

    MultiDimFloatTexture *clamped=(*L)->extract<MultiDimFloatTexture>(w/2+oi,h/2+oj,w,h);
    _lvls.push_back(clamped);
#ifdef SAVE_STACK
    sprintf(str,"__stack_box_%d.png",l);
    CTexture *tmp=clamped->toRGBTexture();
    CTexture::saveTexture(tmp,str);
    delete (tmp);
#endif
    l++;
    sz >>= 1;
    if (sz < 1)
      break;
  }

  // free memory
  for (vector<MultiDimFloatTexture *>::iterator L=tmplvls.begin();L!=tmplvls.end();L++)
    if ((*L) != tex) 
      delete ((*L));

  return ((int)_lvls.size());    
}


// -----------------------------------------------------


int Exemplar::computeStackLevels_Gauss(MultiDimFloatTexture *tex,
                                       vector<MultiDimFloatTexture *>& _lvls)
{
  static char str[256];
  SimpleTimer tm("\n[Exemplar::computeStackLevels] %02d min %02d sec %d ms\n");

  vector<MultiDimFloatTexture *> tmplvls;

  // compute stack of averages on large texture

  // -> finest level
  MultiDimFloatTexture *first=tex;
  MultiDimFloatTexture *lvl  =first;
  tmplvls.clear();
  tmplvls.push_back(tex);

  int numlvl=(int)(0.01 + log2((float)lvl->width()));
  int l=0;
  // -> compute successive filtered versions - FIXME: stack alignement ??
  do
  {
    MultiDimFloatTexture *next=first->applyGaussianFilter_Separable((1 << l)*2+1); // FIXME size ?
    if (lvl != first)
      delete (lvl);
    lvl=next;

    tmplvls.push_back(lvl);

    l++;
  }  while ((1 << l) < lvl->width());

  // clamp stack
  int w=tex->getWidth()/2;
  int h=tex->getHeight()/2;
  _lvls.clear();

  int sz=w;
  l=0;
  for (vector<MultiDimFloatTexture *>::iterator L=tmplvls.begin();L!=tmplvls.end();L++)
  {
    int oi=0,oj=0;
    if (m_bTerrain)
    {
      // offset levels to reproduce primal subdivision stack, 
      // while still following dual subdivision child links
      if (l > 0)
      {
        oi = -(1 << (l-1));
        oj = -(1 << (l-1));
      }
      else
      {
        oi = 0;
        oj = 0;
      }
    }
    else
    {
      // FIXME TODO: add offsets to respect stack alignement
    }

    MultiDimFloatTexture *clamped=(*L)->extract<MultiDimFloatTexture>(w/2+oi,h/2+oj,w,h);
    _lvls.push_back(clamped);
#ifdef SAVE_STACK
    CTexture *tmp=clamped->toRGBTexture();
    sprintf(str,"__stack_gauss_clamped_%d.png",l);
    CTexture::saveTexture(tmp,str);
    delete (tmp);
#endif
    l++;
    sz >>= 1;
    if (sz < 1)
      break;
  }

  // free memory
  for (vector<MultiDimFloatTexture *>::iterator L=tmplvls.begin();L!=tmplvls.end();L++)
    if ((*L) != tex) 
      delete ((*L));

  return ((int)_lvls.size());    
}


// -----------------------------------------------------


int Exemplar::computePyramidLevels_Box(MultiDimFloatTexture *tex,
                                       vector<MultiDimFloatTexture *>& _lvls)
{
  _lvls.clear();
  MultiDimFloatTexture *lvl=new MultiDimFloatTexture(tex);
  while (1)
  {
    _lvls.push_back(lvl);

    if (lvl->getWidth() == 1 || lvl->getHeight() == 1)
      break;

    lvl=lvl->computeNextMIPMapLevel_BoxFilter();

  }
  /*
  // DEBUG
  for (int l=0;l<(int)_lvls.size();l++)
  {
  static char str[256];
  sprintf(str,"__pyr_box_%d.png",l);
  CTexture::saveTexture(_lvls[l],str);
  }
  */
  return ((int)_lvls.size());
}


// -----------------------------------------------------


int Exemplar::computePyramidLevels_Gauss(MultiDimFloatTexture *tex,
                                         vector<MultiDimFloatTexture *>& _lvls)
{
  _lvls.clear();
  MultiDimFloatTexture *lvl=new MultiDimFloatTexture(tex);
  int l=0;
  while (1)
  {
    /*
    // DEBUG
    CTexture *texlvl = lvl->toRGBTexture();
    static char str[256];
    sprintf(str,"__pyr_gauss_%d.png",(int)_lvls.size());
    CTexture::saveTexture(texlvl,str);
    delete (texlvl);
    */
    _lvls.push_back(lvl);
    if (lvl->width() == 1 || lvl->height() == 1)
      break;

    lvl=lvl->computeNextMIPMapLevel_GaussianFilter(3); //(1 << l)*2+1);
    l++;
  }
  return ((int)_lvls.size());
}


// -----------------------------------------------------


void Exemplar::computeRuntimeNeighborhoods(const vector<PCA_Synth>& pcaSynth)
{
  m_SynthNeighborhoods.clear();
  m_ProjectedSynthNeighborhoods.clear();
  computeSynthNeighborhoods();
  computeProjectedSynthNeighborhoods(pcaSynth);
  m_SynthNeighborhoods.clear();
}


// -----------------------------------------------------


void Exemplar::computeSynthNeighborhoods()
{
  SimpleTimer tm("\n[computeSynthNeighborhoods] %02d min %02d sec %d ms\n");

  m_SynthNeighborhoods.resize(m_iNbLevels);
  // foreach level
  for (int l=0;l<m_iNbLevels;l++)
  {
    exemplar_accessor access(l);

    const MultiDimFloatTexture *recolored_level=NULL;

    if (GLOBALPARAMETERS.isDefined("4D")) {
      // -> keep only 4 dimension from recolored exemplar
      MultiDimFloatTexture *level_4D=new MultiDimFloatTexture(
        recoloredStack(l)->width(),
        recoloredStack(l)->height(),
        recoloredStack(l)->numComp());
      int w=level_4D->getWidth();
      int h=level_4D->getHeight();
      assertx(w == stack(l)->getWidth() && h == stack(l)->height());
      assertx(level_4D->numComp() == NUM_RECOLORED_PCA_COMPONENTS);
      for (int i=0;i<w;i++) {
        for (int j=0;j<h;j++) {
          // -> copy first four channels
          for (int c=0;c<4;c++)
            level_4D->set(i,j,c)=recoloredStack(l)->get(i,j,c);
          // -> zero out all channels above 4
          for (int c=4;c<level_4D->numComp();c++)
            level_4D->set(i,j,c)=0;
        }
      }
      recolored_level=level_4D;
    } else {
      // -> keep all dimensions
      recolored_level=recoloredStack(l);
    }

    m_SynthNeighborhoods[l].resize(recolored_level->width()*recolored_level->height());
    for (int j=0;j<recolored_level->height();j++) {
      for (int i=0;i<recolored_level->width();i++)  {
        m_SynthNeighborhoods[l][i+j*recolored_level->width()].construct(
          recolored_level,
          access,
          (!m_bToroidal) && l < (m_iNbLevels - NUM_LEVELS_WITHOUT_BORDER),
          //(!m_bToroidal) && l < FIRST_LEVEL_WITH_BORDER,
          l,i,j);
      }
    }

  }
}


// -----------------------------------------------------


void Exemplar::computeProjectedSynthNeighborhoods(const vector<PCA_Synth>& pcaSynth)
{
  bool clean_neigh=false;

  // if synth neighborhoods not available, compute them
  if (m_SynthNeighborhoods.empty())
  {
    computeSynthNeighborhoods();
    clean_neigh=true;
  }

  {
    SimpleTimer tm("\n[computeProjectedSynthNeighborhoods] %02d min %02d sec %d ms\n");

    m_ProjectedSynthNeighborhoods.resize(m_iNbLevels);
    // foreach level
    for (int l=0;l<m_iNbLevels;l++)  {
      m_ProjectedSynthNeighborhoods[l].resize(m_RecoloredStack[l]->getWidth()*m_RecoloredStack[l]->getHeight());
      for (int j=0;j<m_RecoloredStack[l]->getHeight();j++) {
        for (int i=0;i<m_RecoloredStack[l]->getWidth();i++) {
          const NeighborhoodSynth& neigh=m_SynthNeighborhoods[l][i+j*m_RecoloredStack[l]->getWidth()];
          NeighborhoodSynth&       proj =m_ProjectedSynthNeighborhoods[l][i+j*m_RecoloredStack[l]->getWidth()];
          pcaSynth[l].project(neigh,NUM_RUNTIME_PCA_COMPONENTS, proj);
          proj.setIJ(i,j);
        }
      }
    }
  }

  // clean RT neighborhoods
  if (clean_neigh)
    m_SynthNeighborhoods.clear();
}


// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------


bool Exemplar::acceptNeighbor(int l,int w,int h,
                              int i,int j,bool c_crossing,
                              int x,int y,bool q_crossing)
{
  // Check for alignement with exemplar period
  if ((1 << l) >= m_iPeriodX/4)
    if ( (x % m_iPeriodX) != (i % m_iPeriodX) )
      return (false);
  if ((1 << l) >= m_iPeriodY/4)
    if ( (y % m_iPeriodY) != (j % m_iPeriodY) )
      return (false);

  // Do not accept if neighbor on edge
  if (!m_bToroidal && c_crossing)
    return (false);

  // Check class (experimental)
  if (isConstrained())
  {
    int classc=(int)m_Constraints[l]->get(i,j,0);
    int classq=(int)m_Constraints[l]->get(x,y,0);
    if (classc > 128)
      return (false);
  }

  // Do not jump close to the border (only if not toroidal)
  if ((!m_bToroidal) && l < (m_iNbLevels - NUM_LEVELS_WITHOUT_BORDER))
  //if ((!m_bToroidal) && l < FIRST_LEVEL_WITH_BORDER)
  {
    // the following is mandatory with C[S[p+delta]]-delta instead of C[S[p+delta]-delta]
    int hl = (1<<l);
    if (i < hl*NeighborhoodSynth_SIZE/2 || i >= w - hl*NeighborhoodSynth_SIZE/2)
      return (false);
    if (j < hl*NeighborhoodSynth_SIZE/2 || j >= h - hl*NeighborhoodSynth_SIZE/2)
      return (false);
  }

  return (true);
}


// -----------------------------------------------------


void Exemplar::computeSimilaritySets()
{
#ifndef PCTS_NO_ANALYSIS

  cerr << "  -> computing similarity sets" << endl;
  cerr << "     * simsets neighborhood is " << (Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1) << "^2" << endl;
  if (s_bUseRecoloredForSimset)
    cerr << "     * using recolored exemplar" << endl;

  const vector<MultiDimFloatTexture *> *ex_stack=NULL;
  if (s_bUseRecoloredForSimset) ex_stack=&(m_RecoloredStack);
  else                          ex_stack=&(m_Stack);

  m_SimilaritySet.resize(m_iNbLevels);

  // foreach level
  for (int l=0;l<m_iNbLevels-COARSE_LEVELS_SKIP_ANALYSE;l++)
  {
    //STAT(Ratio);
    //STAT(D3);
    //STAT(DAll);

    float ann_epsilon=0.0f;
    int   num_ann_dim=0;
    int   num_project=0;

    int w=(*ex_stack)[l]->getWidth();
    int h=(*ex_stack)[l]->getHeight();

    cerr << "     -> level " << l << endl;
    m_SimilaritySet[l].resize(w*h);

    // progress bar vars
    int pb_displayed=0,pb_n=0;

    // --------------------------------
    // precompute neighborhoods and PCA

    int tmstart=timeGetTime();
    PCA<NeighborhoodSimset> pca;

    vector<NeighborhoodSimset>  neightbl;
    vector<NeighborhoodSimset>  nprojtbl;

    // -> Gauss kernel
    MultiDimFloatTexture *kernel=MultiDimFloatTexture::makeGaussKernel(
      Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1,GAUSS_KERNEL_SIGMA);
    // DEBUG
    //CTexture::saveTexture(kernel->toRGBTexture(0,255.0/kernel->get(kernel->width()/2,kernel->width()/2,0)),"kernel.simset.png");
    // -> create neighbors
    neightbl.resize(w*h);
    for (int y=0;y<h;y++) {
      for (int x=0;x<w;x++) {
        neightbl[x+y*w].construct(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET,
          (*ex_stack)[l],exemplar_accessor(l),
          (!m_bToroidal) && l < (m_iNbLevels - NUM_LEVELS_WITHOUT_BORDER),
//          (!m_bToroidal) && l < FIRST_LEVEL_WITH_BORDER,
          l,x,y,
          kernel);
      }
    }
    delete (kernel);

    if (s_fPrecision < 1.0f)
    {
      cerr << "        PCA ... ";// << endl;

      // -> add samples
      //cerr << "        adding samples " << endl;
      pca.setNumDim(
        (Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)
        *(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)
        *(*ex_stack)[0]->numComp());

      for (int y=0;y<h;y++)
        for (int x=0;x<w;x++)
          pca.addSampleByRef(neightbl[x+y*w]);

      // -> compute PCA
      //cerr << "        computing " << endl;
      pca.computePCA();
      pca.clear();

      cerr << " done.";
      int tmpca=timeGetTime() - tmstart;
      cerr << "  (" << tmpca/1000 << " s. " << (tmpca % 1000) << " ms)" << endl;

      /*
      // PCA sanity check
      cerr << "        PCA sanity check ... ";
      for (int y=0;y<h;y++)
      {
      for (int x=0;x<w;x++)
      {
      NeighborhoodSimset proj(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET,(*ex_stack)[l]->numComp());
      pca.project(neightbl[x+y*w],proj);
      NeighborhoodSimset unproj(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET,(*ex_stack)[l]->numComp());
      pca.unproject(proj,unproj);
      assertx(unproj.distance(neightbl[x+y*w]) < 1e-6);
      }
      }
      cerr << " done." << endl;
      */

      // --------------------------------
      // -> compute total std energy
      double totalw=0.0;
      for (int v=0;v<pca.getNumDim();v++)
      {
        float e=pca.getEigenVal(v);
        totalw+=e*e;
      }
      // -> select components until reaching 99% of totalw
      int    num_comp_prec;
      double accumw=0.0;
      for (num_comp_prec=0;num_comp_prec<pca.getNumDim();num_comp_prec++)
      {
        float e=pca.getEigenVal(num_comp_prec);
        accumw+=e*e;
        if (accumw > totalw*s_fPrecision)
          break;
      }
      num_comp_prec=min(num_comp_prec+1,(int)pca.getNumDim());
      
      ann_epsilon=(float)(sqrt(accumw)*s_fEpsilon);

      // --------------------------------
      // -> compute partially projected neighborhoods

      num_project = num_comp_prec;
      num_ann_dim = num_project;

      nprojtbl.resize(w*h);
      for (int y=0;y<h;y++)
      {
        for (int x=0;x<w;x++)
        {
          nprojtbl[x+y*w].setShape(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET,(*ex_stack)[l]->numComp());
          pca.project(neightbl[x+y*w],num_project,nprojtbl[x+y*w]);
        }
      }
    }
    else
    {
      ann_epsilon = 0.0f;
      num_project = 
        (Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)*(Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET*2+1)
        *(*ex_stack)[l]->numComp();
      num_ann_dim = num_project;
    }

    // --------------------------------
    // add neighborhoods to search structure

    ANNpointArray	ann_data_pts;		// data points
    ANNpoint		  ann_query_pt;		// query point
    ANNidxArray		ann_nn_idx;			// near neighbor indices
    ANNdistArray	ann_dists;			// near neighbor distances
    ANNkd_tree	 *ann_tree;

    ann_query_pt = annAllocPt(num_ann_dim);       // allocate query point
    ann_data_pts = annAllocPts(w*h, num_ann_dim); // allocate data points
    ann_nn_idx   = new ANNidx[w*h];  		          // allocate space for near neigh indices
    ann_dists    = new ANNdist[w*h];              // allocate space for near neighbor dists

    tmstart=timeGetTime();
    cerr << "        precision                       : " << s_fPrecision << endl;
    cerr << "        number of dimensions for search : " << num_ann_dim << endl;
    if (pca.isReady())
    {
      cerr << "        total number of dimensions      : " << pca.getNumDim() << endl;
      cerr << "        percentage of total energy      : " << pca.percentEnergyInNDimensions(num_ann_dim) << endl;
    }
    cerr << "        building search structure ... ";

    // compute min/max on every comp (usefull stat) // DEBUG
    vector<float> components_min(num_ann_dim),components_max(num_ann_dim);
    for (int c=0;c<num_ann_dim;c++)
    {
      components_min[c]= 999999.0f;
      components_max[c]=-999999.0f;
    }

    // build ann data points

    vector<NeighborhoodSimset>  *samplestbl=NULL;
    if (s_fPrecision < 1.0f)
      samplestbl=&nprojtbl;
    else
      samplestbl=&neightbl;

    for (int y=0;y<h;y++)
    {
      for (int x=0;x<w;x++)
      {
        int id=x+y*w;

        // -> fill in neighborhoods
        for (int c=0;c<num_project;c++)
        {
          ann_data_pts[id][c]=(*samplestbl)[id].getValue(c);
          
          // DEBUG
          components_min[c]=min(components_min[c],(float)ann_data_pts[id][c]);
          components_max[c]=max(components_max[c],(float)ann_data_pts[id][c]);
        }
      }
    }
    // DEBUG
    // display comp min/max
    //for (int c=0;c<num_ann_dim;c++)
    //  cerr << "[" << components_min[c] << "," << components_max[c] << "] ";
    //cerr << endl;

    // Build search structure
    ann_tree = new ANNkd_tree(ann_data_pts,			// the data points
      w*h,              // number of points
      num_ann_dim);

    cerr << " done.";
    int tmstruct=timeGetTime() - tmstart;
    cerr << "  (" << tmstruct/1000 << " s. " << (tmstruct % 1000) << " ms)" << endl;

    // Parse pixels and search for k-nearests

    double min_inter_dist=((float)w)*MIN_K_NEAREST_DISTANCE;
    int nbfound   =0;
    int nbrejected=0;
    int redoavg   =0;
    int startnavg =0;
    int k_mul=1;
    tmstart=timeGetTime();
    cerr << "        [";
    for (int y=0;y<h;y++)
    {
      for (int x=0;x<w;x++)
      {  
        const NeighborhoodSimset& nquery=neightbl[x+y*w];
        
        // ************ SEARCH with ANN

        // Query point
        for (int c=0;c<num_ann_dim;c++)
          ann_query_pt[c]=ann_data_pts[x+y*w][c];
        // Number to search for
        int ksearch=min(k_mul*K_NEAREST,w*h);
        //cerr << ksearch << " ===== " << w*h << endl;

        NeighborhoodSimset *valids[K_NEAREST];
        int             nvalids = 0;
        int  loopcnt            = 0;
        bool redo               = false;
        bool toomany            = false;
        do
        {
          if (loopcnt>0)
            redo=true;
          else
            startnavg+=ksearch;
          nvalids=0;
          // add identity
          if ((m_bToroidal || !nquery.isCrossing()) && 
            acceptNeighbor(l,w,h,x,y,nquery.isCrossing(),x,y,nquery.isCrossing()))
            valids[nvalids++]=&(neightbl[x+y*w]);
          // Search 
          ksearch=min(ksearch,w*h);
          ann_tree->annkSearch(
            ann_query_pt,	           // query point
            ksearch,                 // number of near neighbors
            ann_nn_idx,				       // nearest neighbors (returned)
            ann_dists,				       // distance (returned)
            ann_epsilon);            // epsilon FIXME: how to set this value ??
          nbfound   +=ksearch;
          nbrejected+=ksearch;
          // Select valids only
          for (int k=0;k<ksearch;k++)
          {
            // neighbor data
            NeighborhoodSimset      *neigh = &(neightbl[ann_nn_idx[k]]);
            int                      i = neigh->i();
            int                      j = neigh->j();

            // Identity already inserted
            if (i == x && j == y)
              continue;

            // Accept ?
            if (!acceptNeighbor(l,w,h,i,j,neigh->isCrossing(),x,y,nquery.isCrossing()))
              continue;

            // ------
            // check distance with those already present
            double mindist=-1;
            knearest_id kn_id=knearest_id(l,i,j,m_iExemplarId);
            for (int v=0;v<nvalids;v++)
            {
              knearest_id present_id=knearest_id(l,valids[v]->i(),valids[v]->j(),m_iExemplarId);
              knearest_id_cmp cmp;
              double dist=cmp(kn_id,present_id);
              if (dist < mindist || mindist < 0.0)
                mindist=dist;
            }
            if (mindist > 0.0 && mindist < min_inter_dist)
              continue;

            // ------
            // check distance with neighbor's k-nearests      NOTE: uses neighbors (-1,0) (0,-1) (-1,-1)
            // (avoids coherence within similarity set)
            int hl = (1 << l);
            static const int np[3][2]={{-1,0},{-1,-1},{0,-1}};
            bool too_close=false;
            for (int ni=0;ni<3 && !too_close;ni++)
            {
              int nx = x + np[ni][0]*hl;
              int ny = y + np[ni][1]*hl;
              if (nx >= 0 && ny >= 0 && nx < w && ny < h)
              {
                for (int v=0;v<(int)m_SimilaritySet[l][nx+ny*w].size();v++)
                {
                  knearest_id neigh_id=m_SimilaritySet[l][nx+ny*w][v];
                  int d = max(
                    abs((neigh_id.i - kn_id.i) - np[ni][0]*hl),
                    abs((neigh_id.j - kn_id.j) - np[ni][1]*hl)
                    );
                  int r = hl; // distance threshold
                  if (d <= r)
                  {
                    too_close=true;
                    break;
                  }
                }
              }
            }
            if (too_close)
              continue;   // reject

            // add
            valids[nvalids++]=neigh;
            nbrejected--;

            if (nvalids >= K_NEAREST)
            {
              if (k < ksearch/4)
                toomany=true;
              break;
            }
          }
          // for next search, twice more neighbors
          ksearch*=2;
          loopcnt++;
        } while (nvalids < min(K_NEAREST,w*h) && ksearch < w*h );

        // adapt starting ksearch
        if (redo)
          k_mul<<=1;
        if (toomany)
          k_mul=min(1,k_mul >> 1);
        redoavg+=loopcnt;
        // add k-nearests
        for (int v=0;v<nvalids;v++)
          m_SimilaritySet[l][x+y*w].push_back(knearest_id(l,valids[v]->i(),valids[v]->j(),m_iExemplarId));

        // progress bar
        pb_n++;
        int pb_percent=(20*pb_n)/(w*h);
        while (pb_displayed < pb_percent)
        {
          cerr << '.';
          pb_displayed++;
          if (pb_displayed % 5 == 0)
            cerr << (100*pb_displayed)/(20) << '%';
        }	

        if (K_NEAREST < w*h && !nquery.isCrossing() && (m_SimilaritySet[l][x+y*w].size() < K_NEAREST))
          cerr << 'e' << endl;

      }
    }
    cerr << ']';
    int tmkn=timeGetTime() - tmstart;
    cerr << "  (" << tmkn/1000 << " s. " << (tmkn % 1000) << " ms)" << endl;

    cerr << "         redo avg           = " << redoavg/(float)(w*h) << endl;
    cerr << "         start ksearch avg  = " << startnavg/(float)(w*h) << endl;
    cerr << "         num extracted      = " << nbfound 
      << "  nbrejected = " << nbrejected << ": " << nbrejected*100.0f/nbfound << '%' << endl;
    cerr << "         num comp. used     = " << num_project << endl;

    // free memory

    delete [](ann_dists);
    delete [](ann_nn_idx);
    delete (ann_tree);
    annDeallocPts(ann_data_pts);
    annDeallocPt(ann_query_pt);

  }

  // fill in last levels
  for (int l=m_iNbLevels-COARSE_LEVELS_SKIP_ANALYSE;l<m_iNbLevels;l++)
  {
    int w=(*ex_stack)[l]->getWidth();
    int h=(*ex_stack)[l]->getHeight();
    m_SimilaritySet[l].resize(w*h);
    for (int i=0;i<w;i++)
      for (int j=0;j<h;j++)
        for (int k=0;k<K_NEAREST;k++)
          m_SimilaritySet[l][i+j*w].push_back(knearest_id(l,i,j,m_iExemplarId));
  }

  /*
  // DEBUG
  // output exemplar stack through similarity set
  // useful to visually check similarity sets make sense
  for (int l=0;l<m_iNbLevels;l++)
  {
    const MultiDimFloatTexture *lvl=m_Stack[l];
    int w=lvl->getWidth();
    int h=lvl->getHeight();
    CTexture *textons=new CTexture(w,h,false);
    for (int j=0;j<h;j++) {
      for (int i=0;i<w;i++) {
        int u=m_SimilaritySet[l][i+j*w][1].i;
        int v=m_SimilaritySet[l][i+j*w][1].j;
        for (int c=0;c<3;c++)
          textons->set(i,j,c)=lvl->get(u,v,c);
      }
    }
    static char str[256];
    sprintf(str,"dbg_simset_lvl%d.png",l);
    CTexture::saveTexture(textons,str);
    delete (textons);
  }
  */

#endif PCTS_NO_ANALYSIS
}


// -----------------------------------------------------


double Exemplar::computeVarianceInNeighborhoods(int numComp,
                                                const vector<NeighborhoodRecolor>& neighborhoods)
{
  // -> mean
  NeighborhoodRecolor mean;
  mean.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,numComp);
  int nsz =NEIGHBORHOOD_RADIUS_RECOLOR*2+1;
  // -> for all neighborhoods
  for (int n=0;n<int(neighborhoods.size());n++) {
    // compute average
    const NeighborhoodRecolor& neigh=neighborhoods[n];
    for (int nj=0;nj<nsz;nj++) {
      for (int ni=0;ni<nsz;ni++) {
        for (int c=0;c<numComp;c++) {
          int v=(ni + nj*nsz)*numComp+c;
          mean.setValue(v,mean.getValue(v) + neigh.getValue(v));
        }
      } // ni
    } // nj
  } // n
  for (int v=0;v<mean.numDim();v++) {
    mean.setValue(v,mean.getValue(v)/float(neighborhoods.size()));
  }
/*
  // DEBUG
  CTexture *tex=new CTexture(nsz,nsz,false);
  for (int nj=0;nj<nsz;nj++) {
    for (int ni=0;ni<nsz;ni++) {
      for (int c=0;c<numComp;c++) {
        tex->set(ni,nj,c)=mean.getValue(c+ni*numComp+nj*numComp*nsz)*255.0f;
      }
    }
  }
  static char str[256];
  static int dbg=0;
  sprintf(str,"debug_%d.png",dbg++);
  CTexture::saveTexture(tex,str);
  delete (tex);
*/
  // -> variance
  double neigh_variance=0.0;
  // -> for all neighborhoods
  for (int n=0;n<int(neighborhoods.size());n++) {
    const NeighborhoodRecolor& neigh=neighborhoods[n];
    neigh_variance += double(mean.distanceSq(neigh));
  }
  neigh_variance /= double(neighborhoods.size());
  return (neigh_variance);
}


// -----------------------------------------------------


void Exemplar::recolor()
{
  m_RecoloredStack.clear();

  vector<PCA_Recolor> pca_levels;
  pca_levels.resize(m_iNbLevels);
  int lastvalidpca=0;
  int numrecoloreddim=NUM_RECOLORED_PCA_COMPONENTS;
  // check for global recoloring parameters
  bool recolor_rgb=GLOBALPARAMETERS.isDefined("recolor_rgb");
  if (recolor_rgb) {
    cerr << "**********************************************" << endl;
    cerr << "  Recoloring disabled: RGB color used instead" << endl;
    cerr << "*********************************************" << endl;
  }
  bool recolor_3channels=GLOBALPARAMETERS.isDefined("recolor_3channels");
  if (recolor_3channels) {
    cerr << "**********************************************" << endl;
    cerr << "  Recoloring limited to 3 channels" << endl;
    cerr << "*********************************************" << endl;
    numrecoloreddim=3;
  }
  bool recolor_4channels=GLOBALPARAMETERS.isDefined("recolor_4channels");
  if (recolor_4channels) {
    cerr << "**********************************************" << endl;
    cerr << "  Recoloring limited to 4 channels" << endl;
    cerr << "*********************************************" << endl;
    numrecoloreddim=4;
  }
  assertx(
    !(recolor_3channels && recolor_rgb)
    && !(recolor_4channels && recolor_rgb)
    && !(recolor_3channels && recolor_4channels));
  bool use_empca=GLOBALPARAMETERS.isDefined("empca");
  int empca=0;
  if (use_empca) {
    empca=atoi(GLOBALPARAMETERS.getValue("empca"));
    cerr << "**********************************************" << endl;
    cerr << "  Using EM-PCA;  niter=" << empca << endl;
    cerr << "*********************************************" << endl;
  }

  // foreach level
  for (int l=0;l<m_iNbLevels;l++)
  {
    exemplar_accessor access(l);

    // Gauss kernel
    MultiDimFloatTexture *kernel=MultiDimFloatTexture::makeGaussKernel(
      NEIGHBORHOOD_RADIUS_RECOLOR*2+1,GAUSS_KERNEL_SIGMA);
    // DEBUG
    //CTexture::saveTexture(kernel->toRGBTexture(0,
    // 255.0/kernel->get(kernel->width()/2,kernel->width()/2,0)),
    // "kernel.recolor.png");
    // build neighborhoods for recoloring
    vector<NeighborhoodRecolor> neighborhoods;
    neighborhoods.resize(m_Stack[l]->getWidth()*m_Stack[l]->getHeight());
    for (int j=0;j<m_Stack[l]->getHeight();j++) {
      for (int i=0;i<m_Stack[l]->getWidth();i++) {
        neighborhoods[i+j*m_Stack[l]->getWidth()].construct(
          NEIGHBORHOOD_RADIUS_RECOLOR,
          m_Stack[l],
          access,
          false,
          l,i,j,
          kernel);
      }
    }
    vector<NeighborhoodRecolor> projected_neighborhoods;
    if (!recolor_rgb) {
      // -> computing pca on color neighborhoods
      cerr << "   computing PCA ... ";

      pca_levels[l].setNumDim(
        (NEIGHBORHOOD_RADIUS_RECOLOR*2+1)
        *(NEIGHBORHOOD_RADIUS_RECOLOR*2+1)
        *m_Stack[l]->numComp());

      int w=stack(l)->getWidth();
      int h=stack(l)->getHeight();
      for (int i=0;i<w;i++) {
        for (int j=0;j<h;j++) {
          const NeighborhoodRecolor& neigh=neighborhoods[i+j*m_Stack[l]->getWidth()];
          if (isToroidal() || !neigh.isCrossing())
            pca_levels[l].addSampleByRef(neigh);
        }
      }
      pca_levels[l].computePCA(empca);

      cerr << hform("* total variance from PCA is %f\n",pca_levels[l].getTotalVariance());

      // SAT:  Took out writing of extra data
      /*
      // DEBUG
      {
        static char str[512];
        StringCchPrintfA(str,512,"%s.%d.pca.txt",m_Analyser->rootName().c_str(),l);
        ofstream opca(str);
        pca_levels[l].save(opca);
        opca.close();
      }
      */

      // display variance retained in these firsts components
      m_Analyser->m_RecolorVarianceCaptured[l]=pca_levels[l].percentEnergyInNDimensions(numrecoloreddim);
      m_Analyser->m_RecolorNumDimUsed[l]=numrecoloreddim;
      cerr << " (energy in " 
        << NUM_RECOLORED_PCA_COMPONENTS << '/' << pca_levels[l].getNumDim() << " comp.: " 
        << m_Analyser->m_RecolorVarianceCaptured[l] << "%)"
        << endl;
      pca_levels[l].listPercentEnergyInDimensions(
        pca_levels[l].getNumDim(),
        m_Analyser->m_RecolorVarianceCapturedGraph[l]);

      // project neighborhoods
      cerr << "projecting ... ";
      projected_neighborhoods.resize(m_Stack[l]->getWidth()*m_Stack[l]->getHeight());
      for (int j=0;j<m_Stack[l]->getHeight();j++)
      {
        for (int i=0;i<m_Stack[l]->getWidth();i++)
        {
          const NeighborhoodRecolor& neigh=neighborhoods[i+j*m_Stack[l]->getWidth()];
          NeighborhoodRecolor&       proj=projected_neighborhoods[i+j*m_Stack[l]->getWidth()];
          proj.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,NUM_RECOLORED_PCA_COMPONENTS);
          pca_levels[l].project(neigh,NUM_RECOLORED_PCA_COMPONENTS,  proj);
          proj.setIJ(i,j);
        }
      }

      // compute reconstruction error for PCA and standard synthesis schemes (only on RGB exemplars)
      if (m_Stack[l]->numComp() == 3) {
        cerr << endl;
        // compute reconstruction error on PCA
        //Stat ErrPCA;
        //ErrPCA.setprint(1); ErrPCA.setname("ErrPCA");
        //Stat ErrCST;
        //ErrCST.setprint(1); ErrCST.setname("ErrCST");
        double err_pca=0.0;
        for (int n=0;n<int(neighborhoods.size());n++) {
          // compute projection error
          const NeighborhoodRecolor& neigh=neighborhoods[n];
          NeighborhoodRecolor centered;
          centered.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l]->numComp());
          pca_levels[l].subMean(neigh,centered);
          NeighborhoodRecolor proj;
          proj.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l]->numComp());
          pca_levels[l].project(centered,NUM_RECOLORED_PCA_COMPONENTS,  proj);
          NeighborhoodRecolor unproj_centered;
          unproj_centered.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l]->numComp());
          pca_levels[l].unproject(proj,NUM_RECOLORED_PCA_COMPONENTS, unproj_centered);
          NeighborhoodRecolor unproj;
          unproj.setShape(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l]->numComp());
          pca_levels[l].addMean(unproj_centered,unproj);
          double err=neigh.distanceSq(unproj);
          err_pca+=err;
          //ErrPCA.enter(err);
        }
        err_pca/=double(neighborhoods.size());
        //ErrPCA.terminate();
        // compute reconstruction error of a constant color neighborhood
        double err_cst=0.0;
        int    nsz  = NEIGHBORHOOD_RADIUS_RECOLOR*2+1;
        float  kctr = kernel->get(NEIGHBORHOOD_RADIUS_RECOLOR,NEIGHBORHOOD_RADIUS_RECOLOR,0);
        for (int j=0;j<m_Stack[l]->getHeight();j++) {
          for (int i=0;i<m_Stack[l]->getWidth();i++) {
            // fetch neighborhood
            const NeighborhoodRecolor& neigh=neighborhoods[i+j*m_Stack[l]->getWidth()];
            // compute difference with constant neighborhood
            double err=0.0;
            for (int c=0;c<m_Stack[l]->numComp();c++) {
              // value at center
              float ctr = m_Stack[l]->get(i,j,c)/255.0f;
              for (int nj=0;nj<nsz;nj++) {
                for (int ni=0;ni<nsz;ni++) {
                  // weight
                  float w=kernel->get(ni,nj,0)/kctr;
                  // value in neighborhood
                  int   v=c+(ni + nj*nsz)*m_Stack[l]->numComp();
                  float nval= neigh.getValue(v);
                  // compute difference
                  if (ni == NEIGHBORHOOD_RADIUS_RECOLOR && nj == NEIGHBORHOOD_RADIUS_RECOLOR) {
                    assertx(ctr == nval);
                  }
                  float diff=ctr*w - nval;
                  err+=diff*diff;
                } // ni
              } // nj
            } // c
            //ErrCST.enter(err);
            err_cst+=err;
          } // i
        } // j
        err_cst/=double(neighborhoods.size());
        //ErrCST.terminate();
        cerr << endl;
        //cerr << hform("Reconstruction error PCA                  : %f\n",err_pca);
        //cerr << hform("Reconstruction error constant neighborhood: %f\n",err_cst);
        m_Analyser->m_StandardSchemeReconstructionError[l][0]=err_pca;
        m_Analyser->m_StandardSchemeReconstructionError[l][1]=err_cst;
        m_Analyser->m_StandardSchemeReconstructionError[l][2]=err_cst/err_pca;
      } else {
        m_Analyser->m_StandardSchemeReconstructionError[l][0]=0.0f;
        m_Analyser->m_StandardSchemeReconstructionError[l][1]=0.0f;
        m_Analyser->m_StandardSchemeReconstructionError[l][2]=0.0f;
      } // end of reconstruction error comparison

    } // if !recolored_rgb

    delete (kernel);

    // build recolored exemplar
    cerr << "recoloring ... ";
    MultiDimFloatTexture *recolored=new MultiDimFloatTexture(
      stack(l)->getWidth(),
      stack(l)->getHeight(),
      NUM_RECOLORED_PCA_COMPONENTS);
    for (int j=0;j<stack(l)->getHeight();j++)  {
      for (int i=0;i<stack(l)->getWidth();i++)  {
        //pair<int,int> kij = similarPixel(l,i,j,0); // avoid neighborhoods on non-toroidal boundary
        //int ki = i;//kij.first;
        //int kj = j;//kij.second;
        const NeighborhoodRecolor& proj = projected_neighborhoods[i+j*m_Stack[l]->getWidth()];
        // transfer into recolored stack
        if (recolor_3channels) {
          // For comparison btw recoloring and no recoloring
          // -> 3 channels recoloring
          for (int c=0;c<3;c++)
            recolored->set(i,j,c) = proj.getValue(c);
          for (int c=3;c<recolored->numComp();c++)
            recolored->set(i,j,c) = 0.0;
        } else if (recolor_4channels) {
          // -> 4 channels recoloring
          for (int c=0;c<4;c++)
            recolored->set(i,j,c) = proj.getValue(c);
          for (int c=4;c<recolored->numComp();c++)
            recolored->set(i,j,c) = 0.0;
        } else if (recolor_rgb) {
          // For comparison btw recoloring and no recoloring
          // -> rgb
          for (int c=0;c<3;c++)
            recolored->set(i,j,c) = stack(l)->get(i,j,c);
          for (int c=3;c<recolored->numComp();c++)
            recolored->set(i,j,c) = 0.0;
        } else {
          // Standard approach
          // -> 8 channels recoloring
          for (int c=0;c<recolored->numComp();c++)
            recolored->set(i,j,c) = proj.getValue(c);
        }
      }
    }
    cerr << "done." << endl;

    /*if (l==0) {
      ofstream fdbg("debug.txt");
      fdbg << hform(" *** eigenvalues: %f %f %f %f %f %f %f %f",
        pca_levels[l].getEigenVal(0)*pca_levels[l].getEigenVal(0),
        pca_levels[l].getEigenVal(1)*pca_levels[l].getEigenVal(1),
        pca_levels[l].getEigenVal(2)*pca_levels[l].getEigenVal(2),
        pca_levels[l].getEigenVal(3)*pca_levels[l].getEigenVal(3),
        pca_levels[l].getEigenVal(4)*pca_levels[l].getEigenVal(4),
        pca_levels[l].getEigenVal(5)*pca_levels[l].getEigenVal(5),
        pca_levels[l].getEigenVal(6)*pca_levels[l].getEigenVal(6),
        pca_levels[l].getEigenVal(7)*pca_levels[l].getEigenVal(7)
        ) << endl;
      for (int c=0;c<recolored->numComp();c++) {
        fdbg << hform(" *** eigenvector %d : ",c);
        for (int d=0;d<pca_levels[l].getNumDim();d++) 
          fdbg << ' ' << pca_levels[l].getEigenVect(c,d);
        fdbg << endl;
      }
      for (int c=0;c<recolored->numComp();c++) {
        for (int j=0;j<stack(l)->getHeight();j++)  {
          for (int i=0;i<stack(l)->getWidth();i++)  {
            fdbg << recolored->get(i,j,c) << ' ';
          }
        }
        fdbg << endl;
      }
    }*/
    // store
    m_RecoloredStack.push_back(recolored);
  }
}


// -----------------------------------------------------


void Exemplar::exportRecolorData(const char *fname)
{
  static char str[1024];

  // foreach level
  for (int l=0;l<m_iNbLevels;l++)
  {
    exemplar_accessor access(l);

    FILE *fout;
    StringCchPrintfA(str,1024,"lvl%02d.%s",l,fname);
    fout=fopen(str,"w");
    if (fout == NULL) {
      cerr << endl << endl;
      cerr << "ERROR: Cannot save data for external recoloring! (" << str << ')' << endl;
      cerr << endl << endl;
      exit (-1);
    }

    // Gauss kernel
    MultiDimFloatTexture *kernel=MultiDimFloatTexture::makeGaussKernel(
      2*NEIGHBORHOOD_RADIUS_RECOLOR+1,GAUSS_KERNEL_SIGMA);
    // build neighborhoods for recoloring
    vector<NeighborhoodRecolor> neighborhoods;
    neighborhoods.resize(m_Stack[l]->getWidth()*m_Stack[l]->getHeight());
    for (int j=0;j<m_Stack[l]->getHeight();j++) {
      for (int i=0;i<m_Stack[l]->getWidth();i++) {
        neighborhoods[i+j*m_Stack[l]->getWidth()].construct(
        NEIGHBORHOOD_RADIUS_RECOLOR,
        m_Stack[l],
        access,
        false,
        l,i,j,
        kernel);
      }
    }
    delete (kernel);

    int numdim=neighborhoods[0].numDim();

    // clustering
    cerr << "Clustering :" << endl;
    float max_cluster_radius=0.0f;
    // determine cluster radius
    // -> compute variance
    TableAllocator<float,true> mean;
    mean.allocate(numdim);
    for (int j=0;j<m_Stack[l]->getHeight();j++) {
      for (int i=0;i<m_Stack[l]->getWidth();i++) {
        const NeighborhoodRecolor& n   =neighborhoods[i+j*m_Stack[l]->getWidth()];
        for (int d=0;d<numdim;d++) {
          mean[d]+=n.getValue(d);
        }
      }
    }
    for (int d=0;d<numdim;d++) {
      mean[d] /= float((m_Stack[l]->getHeight()*m_Stack[l]->getWidth()));
    }
    float var=0.0;
    for (int j=0;j<m_Stack[l]->getHeight();j++) {
      for (int i=0;i<m_Stack[l]->getWidth();i++) {
        const NeighborhoodRecolor& n   =neighborhoods[i+j*m_Stack[l]->getWidth()];
        float dist=0.0f;
        for (int d=0;d<numdim;d++) {
          dist += (mean[d]-n.getValue(d))*(mean[d]-n.getValue(d));
        }
        var+=dist;
      }
    }
    var /= float((m_Stack[l]->getHeight()*m_Stack[l]->getWidth()));
    // -> determine max cluster radius
    max_cluster_radius=sqrt(var)*0.01f; // 0.05f
    // -> do clustering
    cerr << "    - variance           = " << var << endl;
    cerr << "    - standard deviation = " << sqrt(var) << endl;
    cerr << "    - max cluster radius = " << max_cluster_radius << endl;
    Clustering<NeighborhoodRecolor> clusters;
    clusters.init(numdim,max_cluster_radius,int(0.1*m_Stack[l]->getHeight()*m_Stack[l]->getWidth()));
    // -> add samples
    for (int j=0;j<m_Stack[l]->getHeight();j++) {
      for (int i=0;i<m_Stack[l]->getWidth();i++) {
        const NeighborhoodRecolor& n   =neighborhoods[i+j*m_Stack[l]->getWidth()];
        clusters.addSample(n);
      }
    }
    // -> compute
    clusters.compute();
    cerr << "    - number of clusters: " << clusters.numClusters() << endl;
    // -> save cluster centers
    fprintf(fout,"%% File automatically generated on %s\n",__TIMESTAMP__);
    fprintf(fout,"%% %dx%d exemplar, %d dimensions per sample, level %d\n",
      m_Stack[l]->getWidth(),m_Stack[l]->getHeight(),m_Stack[l]->numComp(),l);
    fprintf(fout,"%% %dx%d neighborhoods for a total of %d dimensions per 'appearence vector'\n\n",
      NEIGHBORHOOD_RADIUS_RECOLOR*2+1,NEIGHBORHOOD_RADIUS_RECOLOR*2+1,numdim);
    fprintf(fout,"%% %d clusters, max radius of %f\n\n",
      clusters.numClusters(),max_cluster_radius);
    for (int v=0;v<numdim;v++) {    
      for (int c=0;c<clusters.numClusters();c++) {
        fprintf(fout,"%f ",clusters.clusterCenter(c,v));
        //const NeighborhoodRecolor& n   =neighborhoods[c];
        //fprintf(fout,"%f ",n.getValue(v));
        //fprintf(fout,"%1.16g ",n.getValue(v));
      }
      fprintf(fout,"\n");
    }
    fclose(fout);
    // -> save clustering data
    StringCchPrintfA(str,1024,"clusters.%d.%s",l,m_Analyser->rootName().c_str());
    ofstream f(str);
    clusters.save(f);
    f.close();
/*
// DEBUG
    static TableAllocator<float> colors;
    if (colors.size() == 0){
      colors.allocate(3*4096);
      for (int i=0;i<4096;i++) {
        colors[i*3+0]=Random::G.unif();
        colors[i*3+1]=Random::G.unif();
        colors[i*3+2]=Random::G.unif();
        float l=sqrt(colors[i*3+0]*colors[i*3+0]+colors[i*3+1]*colors[i*3+1]+colors[i*3+2]*colors[i*3+2]);
        colors[i*3+0]/=l;
        colors[i*3+1]/=l;
        colors[i*3+2]/=l;
      }
    }
    CTexture *dbg=new CTexture(
      stack(l)->getWidth(),
      stack(l)->getHeight(),
      false);
    for (int j=0;j<dbg->getHeight();j++) {
      for (int i=0;i<dbg->getWidth();i++) {
        // build a neighborhood
        NeighborhoodRecolor n;
        n.construct(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l],access,false,l,i,j);
        // find cluster
        int c=clusters.sampleOwner(i+j*m_Stack[l]->getWidth());
        //int c=clusters.findClosestCenter(n);
        for (int d=0;d<3;d++)
          dbg->set(i,j,d) = colors[(c%4096)*3+d]*255;
      }
    }
    static char dbgfile[512];
    StringCchPrintfA(dbgfile,512,"clusters.%d.png",l);
    CTexture::saveTexture(dbg,dbgfile);
*/
  /* // without clustering
    fprintf(fout,"%% File automatically generated on %s\n",__TIMESTAMP__);
    fprintf(fout,"%% %dx%d exemplar, %d dimensions per sample, level %d\n",
      m_Stack[l]->getWidth(),m_Stack[l]->getHeight(),m_Stack[l]->numComp(),l);
    fprintf(fout,"%% %dx%d neighborhoods for a total of %d dimensions per 'appearence vector'\n\n",
      NEIGHBORHOOD_RADIUS_RECOLOR*2+1,NEIGHBORHOOD_RADIUS_RECOLOR*2+1,numdim);
    // save neighborhoods
    for (int v=0;v<numdim;v++) {
      for (int j=0;j<m_Stack[l]->getHeight();j++) {
        for (int i=0;i<m_Stack[l]->getWidth();i++) {
          const NeighborhoodRecolor& n   =neighborhoods[i+j*m_Stack[l]->getWidth()];
            fprintf(fout,"%1.16g ",n.getValue(v));
        }
      }
      fprintf(fout,"\n");
    }
    fclose(fout);
  */
  }
}


// -----------------------------------------------------


void Exemplar::importRecolorData(const char *fname,int numcomp)
{
  static char str[1024];

  cerr << "Reading recolored data from external source ..." << endl;
  cerr << "  * file rootname = " << fname << endl;
  cerr << "  * expecting dim reduction into " << numcomp << "D space" << endl;
    
  assertx(m_RecoloredStack.empty());
  // foreach level
  for (int l=0;l<m_iNbLevels;l++)
  {
    exemplar_accessor access(l);

    // -> load clustering data
    Clustering<NeighborhoodRecolor> clusters;
    StringCchPrintfA(str,1024,"clusters.%d.%s",l,m_Analyser->rootName().c_str());
    ifstream f(str);
    clusters.load(f);
    f.close();

    cerr << hform("(%d) ",l);
    cerr << hform("%d clusters loaded\n",clusters.numClusters());

    // -> load transform centered
    StringCchPrintfA(str,1024,"lvl%02d.%s",l,fname);
    ifstream fin(str);
    if (!fin) {
      cerr << endl << endl;
      cerr << "ERROR: Cannot load data from external recoloring! (" << str << ')' << endl;
      cerr << endl << endl;
      exit (-1);
    }

    TableAllocator<TableAllocator<float> > trsf_centers;
    trsf_centers.allocate(clusters.numClusters());
    for (int d=0;d<numcomp;d++) {
      ForTable(trsf_centers,c) {
        if (trsf_centers[c].size() == 0)
          trsf_centers[c].allocate(numcomp);
        fin >> trsf_centers[c][d];
      }
    }
    fin.close();

    // -> transfer to recolored exemplar
    MultiDimFloatTexture *recolored=new MultiDimFloatTexture(
      stack(l)->getWidth(),
      stack(l)->getHeight(),
      NUM_RECOLORED_PCA_COMPONENTS);
    for (int j=0;j<recolored->getHeight();j++) {
      for (int i=0;i<recolored->getWidth();i++) {
        // build a neighborhood
        //NeighborhoodRecolor n;
        //n.construct(NEIGHBORHOOD_RADIUS_RECOLOR,m_Stack[l],access,false,l,i,j);
        // find cluster
        //int c=clusters.findClosestCenter(n);
        int c=clusters.sampleOwner(i+j*recolored->getWidth());
        // cerr << "closest = " << c << endl;
        assertx(c>=0);
        for (int d=0;d<numcomp;d++)
          recolored->set(i,j,d) = trsf_centers[c][d];
        for (int d=numcomp;d<recolored->numComp();d++)
          recolored->set(i,j,d) = 0.0f;
      }
    }
    // store
    m_RecoloredStack.push_back(recolored);
  }

/* // without clustering
  // foreach level
  for (int l=0;l<m_iNbLevels;l++)
  {
    exemplar_accessor access(l);

    StringCchPrintfA(str,1024,"lvl%02d.%s",l,fname);
    ifstream fin(str);
    if (!fin) {
      cerr << endl << endl;
      cerr << "ERROR: Cannot load data from external recoloring! (" << str << ')' << endl;
      cerr << endl << endl;
      exit (-1);
    }
    cerr << hform("(%d)",l);
    MultiDimFloatTexture *recolored=new MultiDimFloatTexture(
      stack(l)->getWidth(),
      stack(l)->getHeight(),
      NUM_RECOLORED_PCA_COMPONENTS);
    for (int c=0;c<recolored->numComp();c++) {
      for (int j=0;j<recolored->getHeight();j++) {
        for (int i=0;i<recolored->getWidth();i++) {
          float v;
          fin >> v;
          recolored->set(i,j,c) = v;
          //cerr << v << ' ';
        }
      }
      //cerr << endl;
    }
    fin.close();
    // store
    m_RecoloredStack.push_back(recolored);
  }
  cerr << "done." << endl;
  */
}


// -----------------------------------------------------


void Exemplar::kNearestsTableTextures(int level,vector<CTexture *>& _texs) const
{
  _texs.clear();

  for (int k=0;k<K_NEAREST;k++)
  {
    CTexture *tex=new CTexture(stack(level)->getWidth(),stack(level)->getHeight(),false);
    for (int i=0;i<tex->getWidth();i++)
    {
      for (int j=0;j<tex->getHeight();j++)
      {
        int nb=(int)m_SimilaritySet[level][i+j*stack(level)->getWidth()].size();
        if (k < nb)
        {
          tex->set(i,j,0)=m_SimilaritySet[level][i+j*stack(level)->getWidth()][k].i;
          tex->set(i,j,1)=m_SimilaritySet[level][i+j*stack(level)->getWidth()][k].j;
          tex->set(i,j,2)=m_SimilaritySet[level][i+j*stack(level)->getWidth()][k].s;
        }
        else
        {
          tex->set(i,j,0)=0;
          tex->set(i,j,1)=0;
          tex->set(i,j,2)=255;
        }
      }
    }
    _texs.push_back(tex);
  }
}


// -----------------------------------------------------

