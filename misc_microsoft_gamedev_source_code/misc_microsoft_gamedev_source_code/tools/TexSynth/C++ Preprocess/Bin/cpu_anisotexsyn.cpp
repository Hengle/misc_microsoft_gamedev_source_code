/*

Simple app to call Analyser and Synthesizer

2004-06-26 Sylvain Lefebvre - (c) Microsoft Corp.
2005-10    Rewritten for anisotexsyn / featurespace projects

*/

/* -------------------------------------------------------- */
#ifdef WIN32
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#endif
/* -------------------------------------------------------- */
#define SCREEN_SIZE 512
/* -------------------------------------------------------- */
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <direct.h>
#include <strsafe.h>
/* -------------------------------------------------------- */

#include "config.h"

/* -------------------------------------------------------- */

#include <list>
#include <vector>
#include <set>

using namespace std;

/* -------------------------------------------------------- */

#include "TPCA.h"

#include "CExemplar.h"
#include "CAnalyser.h"
#include "CGlobalParameters.h"

#include "mem_tools.h"
#include "cmdline_tools.h"


#include "utils.h"


/* -------------------------------------------------------- */
/*
char          g_szOutputName[1024];
char          g_szParamFile[1024];
char          g_szReportDir[1024];
bool          g_bToroidal=false;
*/

CTexture     *g_Tex=NULL;

// -----------------------------------

vector<CTexture *>                           g_UserConstraint;

Analyser                                     g_Analyser;


#define MAX_STR_LEN              80

class BuildProperties
{
public:
   char g_szOutputName[1024];
   char g_szParamFile[1024];
   char g_szReportDir[1024];
   bool g_bToroidal;


   char outputDirectory[MAX_STR_LEN];
   char prtMapName[MAX_STR_LEN];
   char featureMapName[MAX_STR_LEN];
   float featureMapWeight;

   char texName[MAX_STR_LEN];
   char rgbImageName[MAX_STR_LEN];
   char rgbCstrName[MAX_STR_LEN];

   int periodx;
   int periody;

   bool tor;

   bool window;
   bool terrain;
   bool report;

   float precision;
   bool export_recolor;
   bool import_recolor;
   char import_fname[1024];
   int  import_numcomp;

   int rsimset;

   bool YUY;
   bool HSV;

   bool order;
   int order_v0;
   int order_v1;
   int order_v2;


   BuildProperties()
   {
      g_bToroidal=false;

      window=false;
      periodx=1,periody=1;
      terrain=false;
      report=false;
      //isprt=false;
      precision = 0.9f;
      export_recolor=false;
      import_recolor=false;
      import_fname[1024];
      import_numcomp=NUM_RECOLORED_PCA_COMPONENTS;
      featureMapWeight=1.0f;

      rsimset = -1;

      YUY = false;
      HSV = false;

      order = false;
      order_v0 = -1;
      order_v1 = -1;
      order_v2 = -1;


      strcpy(texName, "null");
      strcpy(rgbImageName, "null");
      strcpy(rgbCstrName, "null");

      strcpy(outputDirectory, "\0");
      strcpy(prtMapName, "\0");
      strcpy(featureMapName, "\0");
   }

};

/* -------------------------------------------------------- */

void computeLevels(CTexture *tex,vector<CTexture *>& _levels,int maxl=-1);

bool processCmdLineArgsHoppe(int argc, char **argv, BuildProperties *properties);
bool processCmdLineArgsSimple(int argc, char **argv, BuildProperties *properties);
void convert(const BuildProperties *properties);

//void resizeImageIfNeeded(CTexture **texture);

/* -------------------------------------------------------- */

void computeLevels(CTexture *tex,vector<CTexture *>& _levels,int maxl)
{
  CTexture *lvl=tex;
  _levels.clear();
  while (1)
  {
    _levels.push_back(lvl);
    if (lvl->getWidth() == 1 || lvl->getHeight() == 1)
      break;
    if (_levels.size() == maxl)
      break;
    lvl=lvl->computeNextMIPMapLevel(CTexture::mipmap_std());
  }
}

/* -------------------------------------------------------- */


// from FilterImage (2005-09-12)

class ucRGBA
{
public:
  unsigned char v[4];
  ucRGBA()                     {v[0]=0;v[1]=0;v[2]=0;v[3]=0;}
  ucRGBA(unsigned char *rgba)  {memcpy(v,rgba,4);}
  ucRGBA(unsigned char r,unsigned char g,unsigned char b,unsigned char a) {v[0]=r;v[1]=g;v[2]=b;v[3]=a;}
};

static ucRGBA g_rgbaBlack(0,0,0,0);
static ucRGBA g_rgbaWhite(255,255,255,0);

inline bool rgb_equal(const ucRGBA& uc1, const ucRGBA& uc2)
{
  return (
    uc1.v[0]==uc2.v[0] &&
    uc1.v[1]==uc2.v[1] &&
    uc1.v[2]==uc2.v[2]);
}


// ------


// from FilterImage (2005-09-12)
static void euclidean_distance_map(Matrix<Vector>& mvec)
{
  // Compute Euclidean distance map.
  //  See original work:
  //   Per-Erik Danielsson.  Euclidean Distance Mapping.
  //    Computer Graphics and image Processing 14:227-248, 1980.
  //  And description in:
  //   F.S. Nooruddin and Greg Turk.  Simplification and repair of
  //    polygonal models using volumetric techniques.
  //    Georgia Tech TR GIT-GVU-99-37, 1999.
  // Implementation note:
  // currently uses Vector=float[3]; for optimization should use int[2].
  ForIndexLU(y,1,mvec.ysize()-1) {
    ForIndex(x,mvec.xsize()) {
      Vector vt=mvec[y-1][x]+Vector(-1,0,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
    ForIndexLU(x,1,mvec.xsize()-1) {
      Vector vt=mvec[y][x-1]+Vector(0,-1,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
    For (int x=mvec.xsize()-2;x>=0;x--) {
      Vector vt=mvec[y][x+1]+Vector(0,+1,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
  } EndFor;
  For (int y=mvec.ysize()-2;y>=0;y--) {
    ForIndex(x,mvec.xsize()) {
      Vector vt=mvec[y+1][x]+Vector(+1,0,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
    ForIndexLU(x,1,mvec.xsize()-1) {
      Vector vt=mvec[y][x-1]+Vector(0,-1,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
    For (int x=mvec.xsize()-2;x>=0;x--) {
      Vector vt=mvec[y][x+1]+Vector(0,+1,0);
      if (mag2(vt)<mag2(mvec[y][x])) mvec[y][x]=vt;
    } EndFor;
  } EndFor;
}


// ------


// from FilterImage (2005-09-12)
static void do_featureoffsets(MultiDimFloatTexture *image,
                              bool scalar,
                              const ucRGBA& emptycolor)
{
  // ysize!=xsize is OK
  Matrix<Vector> mvec(image->getHeight(),image->getWidth());
  ForIndex(y,mvec.ysize()) {
    ForIndex(x,mvec.xsize()) {
      ucRGBA pix;
      pix.v[0]=(unsigned char)image->get(y,x,0);
      pix.v[1]=(unsigned char)image->get(y,x,1);
      pix.v[2]=(unsigned char)image->get(y,x,2);
      pix.v[3]=0;
      bool isundef=rgb_equal(pix,emptycolor);
      float vecy  =isundef ? BIGFLOAT: 0.f;
      mvec[y][x]  =Vector(vecy,0,0);
    } EndFor;
  } EndFor;
  euclidean_distance_map(mvec);
  
  ForIndex(y,image->getHeight()) {
    ForIndex(x,image->getWidth()) {
      assertx(mvec[y][x][0]<image->getHeight()); // no more BIGFLOAT values
      if (!scalar)  {
        ForIndex(c,2) {
          int i=(int)(mvec[y][x][c]);
          image->set(y,x,c)=float(i);
        } EndFor;
        image->set(y,x,2)=0;
      } else {
        float l=sqrt(mvec[y][x][0]*mvec[y][x][0] + mvec[y][x][1]*mvec[y][x][1]);
        image->set(y,x,0)=l;
        image->set(y,x,1)=l;
        image->set(y,x,2)=l;
      }
    } EndFor;
  } EndFor;
}

// ------


MultiDimFloatTexture *computeFeatureDistanceMap(CTexture *fmap,bool scalar)
{
  if (fmap == NULL)
    return (NULL);

  bool fdist_binary=GLOBALPARAMETERS.isDefined("fdist_binary");
  if (fdist_binary) {
    cerr << " * using binary feature distance" << endl;
    MultiDimFloatTexture *distmap=new MultiDimFloatTexture(fmap);
    return (distmap);
  }
  bool fdist_tanh=GLOBALPARAMETERS.isDefined("fdist_tanh");
  if (fdist_tanh) {
    cerr << " * using tanh for feature distance" << endl;
  }
  bool fdist_clerp=GLOBALPARAMETERS.isDefined("fdist_clerp");
  if (fdist_clerp) {
    cerr << " * using cubic lerp for feature distance" << endl;
  }
  bool  fdist_clamp=GLOBALPARAMETERS.isDefined("fdist_clamp");
  float fdist_clamp_max=0;
  if (fdist_clamp) {
    fdist_clamp_max=(float)atof(GLOBALPARAMETERS.getValue("fdist_clamp"));
    cerr << " * clamping feature distance to " << fdist_clamp_max << endl;
  }
  bool fdist_one_sided=GLOBALPARAMETERS.isDefined("fdist_one_sided");

  MultiDimFloatTexture *distmap_b=new MultiDimFloatTexture(fmap);
  do_featureoffsets(distmap_b,scalar,g_rgbaBlack);
  MultiDimFloatTexture *distmap_w=new MultiDimFloatTexture(fmap);
  do_featureoffsets(distmap_w,scalar,g_rgbaWhite);

  //CTexture::saveTexture(distmap_b->toRGBTexture(0,16),"distmap_b.png");
  //CTexture::saveTexture(distmap_w->toRGBTexture(0,16),"distmap_w.png");

  // compute max length
  float maxl_b=-1e16f,minl_b=1e16f;
  float maxl_w=-1e16f,minl_w=1e16f;

  ForIndex(y,fmap->getHeight()) {
    ForIndex(x,fmap->getWidth()) {
      maxl_b=max(maxl_b,distmap_b->get(x,y,0));
      minl_b=min(minl_b,distmap_b->get(x,y,0));
      maxl_w=max(maxl_w,distmap_w->get(x,y,0));
      minl_w=min(minl_w,distmap_w->get(x,y,0));
    } EndFor;
  } EndFor;

  cerr << hform("fmaps min / max [%f,%f]  [%f,%f]\n",minl_b,maxl_b,minl_w,maxl_w);

  if (fdist_one_sided) {
    // one-sided feature distance
    for (int j=0;j<distmap_b->height();j++){
      for (int i=0;i<distmap_b->width();i++){
        float l = distmap_b->get(i,j,0);
        float v=0;
        if (fdist_clamp) { 
          l      = min(l,fdist_clamp_max);
          maxl_b = min(maxl_b,fdist_clamp_max);
        }
        float lunit             = (l-minl_b)/(maxl_b-minl_b);
        if      (fdist_tanh)  v = tanh((lunit-0.5f)*4.0f);
        else if (fdist_clerp) v = lunit*lunit*(3.0f-2.0f*lunit);
        else                  v = lunit;
        for (int c=0;c<3;c++)
          distmap_b->set(i,j,c)=v*255.0f;
      }
    }
  } else {
    // two-sided feature distance
    float maxl=max(maxl_b,-minl_w+1.0f);
    float minl=min(minl_b,-maxl_w+1.0f);
    for (int j=0;j<distmap_w->height();j++){
      for (int i=0;i<distmap_w->width();i++){
        float l = distmap_b->get(i,j,0);
        if (fmap->get(i,j,0) == 255 && fmap->get(i,j,1) == 255 && fmap->get(i,j,2) == 255) {
          l = -distmap_w->get(i,j,0)+1.0f;
        }
        if (fdist_clamp) { 
          l      = min(l, fdist_clamp_max);
          l      = max(l,-fdist_clamp_max);
          maxl   = min(maxl, fdist_clamp_max);
          minl   = max(minl,-fdist_clamp_max);
        }
        float v=0;
        float lunit             = (l-minl)/(maxl-minl);
        if      (fdist_tanh)  v = tanh((lunit-0.5f)*4.0f);
        else if (fdist_clerp) v = lunit*lunit*(3.0f-2.0f*lunit);
        else                  v = lunit;
        for (int c=0;c<3;c++)
          distmap_b->set(i,j,c)=v*255.0f;
      }
    }
  }

  return (distmap_b);
}


/* -------------------------------------------------------- */
/*
int main(int argc, char **argv) 
{
  static CTexture::convert_std     rgb;
  static CTexture::convert_RGB_HSV hsv;
  static CTexture::convert_RGB_YUV yuv;
  static CTexture::convert_order  *order=NULL;
  static char                      str[1024];
  bool                             window=false;
  int                              periodx=1,periody=1;
  bool                             terrain=false;
  bool                             report=false;
  bool                             isprt=false;
  bool                             export_recolor=false;
  bool                             import_recolor=false;
  static char                      import_fname[1024];
  int                              import_numcomp=NUM_RECOLORED_PCA_COMPONENTS;
  CTexture                        *featuremask=NULL;
  MultiDimFloatTexture            *featuremap=NULL;
  float                            featuremap_weight=1.0f;
  CTexture                        *prtcolormap=NULL;
  MultiDimFloatTexture            *prtreflectance=NULL;

#ifndef _DEBUG
  try
#endif
  {

    // print doc
    cerr << endl;
    cerr << " -------------------------------------------------------" << endl;
    cerr << endl;
    cerr << "synthtex " << endl;
    cerr << "         -rd   <dirname>   Specifies directory name for" << endl;
    cerr << "                           outputing reports and data." << endl;
    cerr << "                           If analysis data is present in" << endl;
    cerr << "                           the directory and compatible" << endl;
    cerr << "                           it will be automatically loaded." << endl;
    cerr << "         -fmap <filename>  Specifies feature map image filename" << endl;
    cerr << "         -report           Output HTML reports." << endl;
    cerr << "         -tor              Treat exemplars as toroidals." << endl;
    cerr << "         -terrrain         Treat exemplar as terrain heightfield." << endl;
    cerr << "         -periodx <int>    Specify exemplar x period." << endl;
    cerr << "         -periody <int>    Specify exemplar y period." << endl;
    cerr << "         -simset_use_recolored  Simset computation will use recolored exemplar." << endl;
    cerr << "         -rsimset <int>    Specify simset neighborhood radius (size=r*2+1). defaults to 3 (7x7)." << endl;
    cerr << "         -space [HSV|YUV]  Color space to be used" << endl;
    cerr << "         -order [012|120|...] Order of components, usefull" << endl;
    cerr << "                           if less than 3 components are used" << endl;
    cerr << "         -precision [0,1]  Precision. Less -> Faster. Typically 0.99" << endl;
    cerr << "         -epsilon [0,1]    Epsilon for nearest neighbor search. More -> Faster." << endl;
    cerr << "                           Typically 0.1" << endl;
    cerr << "         -export_recolor   Export recoloring data for external computation" << endl;
    cerr << "         -import_recolor   Import recolored data from external computation" << endl;
    cerr << endl;
    cerr << "synthtex [PARAM_LIST] MAP EXEMPLAR CONSTRAINT" << endl;
    cerr << "         MAP           : <filename> | null" << endl;
    cerr << "         EXEMPLAR      : <filename>" << endl;
    cerr << "         CONSTRAINT    : <filename> | null" << endl;
    cerr << endl;
    cerr << " Exemples:" << endl;
    cerr << endl;
    cerr << "  anisotexsyn -rd .\\texture\\ null texture.png null" << endl;
    cerr << endl;
    cerr << " Supported image formats: PNG, JPG." << endl;
    cerr << " Also supports prt files as input for analysis." << endl;
    cerr << endl;
    cerr << " -------------------------------------------------------" << endl;
    cerr << endl;
#ifdef USE_ANN_LIBRARY
    cerr << " -------------------------------------------------------" << endl;
    cerr << " ********* SEARCH performed with ANN library ***********" << endl;
    cerr << endl;
    cerr << endl;
    cerr << " WARNING: ANN library should **NOT** be included in any " << endl;
    cerr << "          commercial product !!!!!!!!!!!!!!!!!!!!!!!!!! " << endl;
    cerr << endl;
    cerr << endl;
    cerr << " -------------------------------------------------------" << endl;
    cerr << endl;
#endif

    if (argc < 2)
      return (1);

    // default parameter values
    int rsimset=-1;
    StringCchCopyA(g_szOutputName,1024,"synth.png");
    StringCchCopyA(g_szParamFile,1024,"threshold.txt");
    StringCchCopyA(g_szReportDir,1024,"synthtex");
    for (int i=1;i<argc;i++)
    {
      StringCchCatA(g_szReportDir,1024,"_");
      StringCchCatA(g_szReportDir,1024,argv[i]);
    }
    int l=strlen(g_szReportDir);
    for (int c=0;c<l;c++)
    {
      g_szReportDir[c]=g_szReportDir[c]==' ' ? '_' : g_szReportDir[c];
      g_szReportDir[c]=g_szReportDir[c]=='\\' ? '_' : g_szReportDir[c];
    }
    CTexture::convert_functor *clrspace=&rgb;

    // parse options
    int argshift=1;
    while (argshift<argc)    
    {
      if (argv[argshift][0] != '-')
        break;
      else if (!strcmp("-o",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -o <filename> : specifies output file." << endl;
          exit (-1);
        }
        StringCchCopyA(g_szOutputName,1024,argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-p",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -p <filename> : specifies parameter file." << endl;
          exit (-1);
        }
        StringCchCopyA(g_szParamFile,1024,argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-rd",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -rd <dirname> : specifies report directory." << endl;
          exit (-1);
        }
        StringCchCopyA(g_szReportDir,1024,argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-fmap",argv[argshift])) {
        if (argshift+2 >= argc)  {
          cerr << "option -fmap <filename> <weight[1.0]> : specifies feature map image and weight." << endl;
          exit (-1);
        }
        featuremask=CTexture::loadTexture(argv[argshift+1]);
        featuremap_weight=(float)atof(argv[argshift+2]);
        assertx(featuremap_weight >= 0.0f);
        argshift+=3;

        cerr << endl << endl << "**********************************************" << endl;
        cerr << "**********************************************" << endl;
        cerr << "      FMAP present - weight = " << featuremap_weight << endl;
        cerr << "**********************************************" << endl;
        cerr << "**********************************************" << endl << endl;

      }  else if (!strcmp("-prtcolor",argv[argshift]))  {
        if (argshift+2 >= argc) {
          cerr << "option -prtcolor <filename>: specifies albedo map for PRT analysis." << endl;
          exit (-1);
        }
        prtcolormap=CTexture::loadTexture(argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-export_recolor",argv[argshift])) {
        export_recolor=true;
        argshift++;
      } else if (!strcmp("-import_recolor",argv[argshift]) && argshift+1 < argc) {
        import_recolor=true;
        StringCchCopyA(import_fname,1024,argv[argshift+1]);
        cerr << "* recoloring data will be imported from files lvlXX." << import_fname << endl;
        argshift+=2;
      } else if (!strcmp("-import_numcomp",argv[argshift]) && argshift+1 < argc) {
        import_numcomp=atoi(argv[argshift+1]);
        cerr << "* importing " << import_numcomp << " dimensions" << endl;
        argshift+=2;
      } else if (!strcmp("-window",argv[argshift])) {
        window=true;
        argshift++;
      } else if (!strcmp("-report",argv[argshift])) {
        report=true;
        argshift++;
      } else if (!strcmp("-terrain",argv[argshift])) {
        terrain=true;
        argshift++;
      } else if (!strcmp("-order",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -order [012|021|...]" << endl;
          exit (-1);
        }
        int v0=argv[argshift+1][0]-'0'; 
        if (v0 < 0) {
          cerr << "option -order [012|021|...]" << endl;
          exit (-1);
        }
        int v1=argv[argshift+1][1]-'0';
        if (v1 < 0) {
          cerr << "option -order [012|021|...]" << endl;
          exit (-1);
        }
        int v2=argv[argshift+1][2]-'0';
        if (v2 < 0) {
          cerr << "option -order [012|021|...]" << endl;
          exit (-1);
        }
        order=new CTexture::convert_order(v0,v1,v2);
        argshift+=2;
      } else if (!strcmp("-space",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -space [HSV|YUV]  Color space to be used" << endl;
          exit (-1);
        }
        if (!strcmp(argv[argshift+1],"YUV"))
          clrspace=&yuv;
        else if (!strcmp(argv[argshift+1],"HSV"))
          clrspace=&hsv;
        else {
          cerr << "option -space [HSV|YUV]  Color space to be used" << endl;
          exit (-1);
        }
        argshift+=2;
      } else if (!strcmp("-tor",argv[argshift])) {
        g_bToroidal=true;
        argshift++;
      } else if (!strcmp("-simset_use_recolored",argv[argshift])) {
        cerr << "* simset computation will use recolored exemplar"  << endl;
        Exemplar::s_bUseRecoloredForSimset=true;
        argshift++;
      } else if (!strcmp("-prepare",argv[argshift])) {
        // no longer relevant
        argshift++;
      } else if (!strcmp("-periodx",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -periodx <int>" << endl;
          exit (-1);
        }
        periodx=atoi(argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-periody",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -periody <int>" << endl;
          exit (-1);
        }
        periody=atoi(argv[argshift+1]);
        argshift+=2;
      } else if (!strcmp("-rsimset",argv[argshift]) && argshift+1 < argc) {
        rsimset=atoi(argv[argshift+1]);
        cerr << "* simset neighborhood radius set to: " << rsimset << endl;
        argshift+=2;
      } else if (!strcmp("-precision",argv[argshift])) {
        if (argshift+1 >= argc)
        {
          cerr << "option -precision [0,1]" << endl;
          exit (-1);
        }
        Exemplar::s_fPrecision=(float)atof(argv[argshift+1]);
        argshift+=2;
        cerr << " -------------------------------------------------------" << endl;
        cerr << " ********* Precision set to " << Exemplar::s_fPrecision   << endl;
        cerr << " -------------------------------------------------------" << endl;
        cerr << endl;
      } else if (!strcmp("-epsilon",argv[argshift])) {
        if (argshift+1 >= argc) {
          cerr << "option -epsilon [0,1]" << endl;
          exit (-1);
        }
        Exemplar::s_fEpsilon=(float)atof(argv[argshift+1]);
        argshift+=2;
        cerr << " -------------------------------------------------------" << endl;
        cerr << " ********* Epsilon set to " << Exemplar::s_fEpsilon   << endl;
        cerr << " -------------------------------------------------------" << endl;
        cerr << endl;
      } else if (argv[argshift][0]=='-')  {
        GLOBALPARAMETERS.addParameter(&(argv[argshift][1]));
        if (argshift+1 < argc && argv[argshift+1][0] != '-') {
          GLOBALPARAMETERS.setValue(&(argv[argshift][1]),argv[argshift+1]);
          argshift+=2;
        } else {
          GLOBALPARAMETERS.setValue(&(argv[argshift][1]),"1");
          argshift++;
        }
      } else {
        // unknow parameter - ignore
        cerr << "WARNING: ignoring parameter " << argv[argshift] << endl;
        argshift++; 
      }
    }
    
    GLOBALPARAMETERS.addParameter("recolor_rgb");
    GLOBALPARAMETERS.addParameter("recolor_3channels");
    GLOBALPARAMETERS.addParameter("recolor_4channels");
    GLOBALPARAMETERS.list(cerr);

    assertx(!(export_recolor && import_recolor));

    if (argc-argshift < 2) {
      cerr << "No exemplar specified !\n" << endl;
      return (1);
    }

    if (order != NULL)
      clrspace=new CTexture::convert_compose(*clrspace,*order);

    // must feature map be made toroidal?
    if (featuremask != NULL) {
      if (g_bToroidal) {
        // enlarge to make sure distances are correct
        int w=featuremask->getWidth();
        int h=featuremask->getHeight();
        CTexture *torfmap=new CTexture(w*3,h*3,false);
        torfmap->copy(0  ,0,featuremask);
        torfmap->copy(w  ,0,featuremask);
        torfmap->copy(w*2,0,featuremask);
        torfmap->copy(0  ,h,featuremask);
        torfmap->copy(w  ,h,featuremask);
        torfmap->copy(w*2,h,featuremask);
        torfmap->copy(0  ,h*2,featuremask);
        torfmap->copy(w  ,h*2,featuremask);
        torfmap->copy(w*2,h*2,featuremask);
        delete (featuremask);
        featuremask=torfmap;
      }
      // create feature map from feature mask
      featuremap=computeFeatureDistanceMap(featuremask,true);
      delete (featuremask);
      featuremask=NULL;
    }

    // create report directory
    mkdir(g_szReportDir);

    // load target constraint map
    if (!strcmp("null",argv[argshift]))
      g_Tex=NULL;
    else  {
      g_Tex=CTexture::loadTexture(argv[argshift]);
      computeLevels(g_Tex,g_UserConstraint);
    }
    argshift++;

    // exemplar image
    CTexture *rgb_image=NULL;
    
    if (strstr(argv[argshift],".prt") != NULL)
      isprt=true;
    else
      rgb_image=CTexture::loadTexture(argv[argshift]);


    // exemplar constraint
    CTexture *rgb_cstr=NULL;
    if (strcmp(argv[argshift+1],"null"))
      rgb_cstr=CTexture::loadTexture(argv[argshift+1]);
    else
      cerr << "  -> no constraint" << endl;

    // convert to float textures

    MultiDimFloatTexture *image = NULL;

    if (isprt) {
      LPD3DXPRTBUFFER ppBuffer=NULL;

      //SAT:  Took out d3dx9 lib.  No way to load PRT Buffers at the moment.
      assert(0);

      //D3DXLoadPRTBufferFromFile(argv[argshift],&ppBuffer);
      assertx(ppBuffer != NULL);

      cerr << "=============================================================" << endl;
      cerr << "                   Reading PRT data" << endl;
      cerr << "=============================================================" << endl;
      cerr << "Num. samples : " << ppBuffer->GetNumSamples()  << endl;
      cerr << "Num. channels: " << ppBuffer->GetNumChannels() << endl;
      cerr << "Num. coeffs. : " << ppBuffer->GetNumCoeffs()   << endl;
      int texres = (int)(sqrt((double)ppBuffer->GetNumSamples()));
      cerr << "Texture size (assumed squared): " << texres << endl;
      cerr << "=============================================================" << endl;
      assertx(1 == ppBuffer->GetNumChannels());
      if (prtcolormap != NULL) {
        cerr << "=============================================================" << endl;
        cerr << "  a color map is present!" << endl;
        cerr << "=============================================================" << endl;
        // -> expand with color map
        assertx(texres == prtcolormap->getWidth() && texres == prtcolormap->getHeight());
        image = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs()*3);
        prtreflectance = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs());
        FLOAT *data=NULL;
        ppBuffer->LockBuffer(0,ppBuffer->GetNumSamples(),&data);
        for (int j=0;j<texres;j++) {
          for (int i=0;i<texres;i++) {
            assertx(i+j*texres < (int)ppBuffer->GetNumSamples());
            int offset=i+j*texres;
            for (int k=0;k<3;k++) { // for each color channel (RGB)
              for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++) // for each monochrome prt coeff
                image->set(i,j,k*ppBuffer->GetNumCoeffs()+c)
                  =data[offset*ppBuffer->GetNumCoeffs() + c] * float(prtcolormap->get(i,j,k))/255.0f;
            }
            for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++)
              prtreflectance->set(i,j,c)=data[offset*ppBuffer->GetNumCoeffs() + c];
          }
        }
        ppBuffer->UnlockBuffer();
        image->setName(argv[argshift]);
        prtreflectance->setName(argv[argshift]);
      } else {
        // -> no color map
        // creating float texture
        image = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs());
        FLOAT *data=NULL;
        ppBuffer->LockBuffer(0,ppBuffer->GetNumSamples(),&data);
        for (int j=0;j<texres;j++) {
          for (int i=0;i<texres;i++) {
            assertx(i+j*texres < (int)ppBuffer->GetNumSamples());
            int offset=i+j*texres;
            for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++)
              image->set(i,j,c)=data[offset*ppBuffer->GetNumCoeffs() + c];
          }
        }
        ppBuffer->UnlockBuffer();
        image->setName(argv[argshift]);
      } // color map
    } else if (featuremap != NULL) {
      // feature map is present
      if (featuremap->getWidth() < rgb_image->getWidth() || featuremap->getHeight() < rgb_image->getHeight()) {
        throw SynthesisException("feature map is smaller than exemplar !");
      }
      // -> only keep center part if larger than exemplar
      if (featuremap->getWidth() > rgb_image->getWidth() || featuremap->getHeight() > rgb_image->getHeight()) {
        cerr << " * cropping feature map" << endl;
        int l=(featuremap->getWidth() - rgb_image->getWidth())/2;
        int r=(featuremap->getHeight() - rgb_image->getHeight())/2;
        MultiDimFloatTexture *subfmap=featuremap->extract<MultiDimFloatTexture>(
          l,r,rgb_image->getWidth(),rgb_image->getHeight());
        delete (featuremap);
        featuremap=subfmap;
      }
      // -> built multichannel exemplar
      image = new MultiDimFloatTexture(rgb_image->getWidth(),rgb_image->getHeight(),4);
      for (int j=0;j<image->height();j++) {
        for (int i=0;i<image->width();i++) {
          // rgb
          for (int c=0;c<3;c++)
            image->set(i,j,c)=rgb_image->get(i,j,c);
          // feature dist
          image->set(i,j,3)=featuremap->get(i,j,0)*featuremap_weight;
        }
      }
      image->setName(rgb_image->getName());
    }
    else
      image = new MultiDimFloatTexture(rgb_image);

    // size of neighborhood for simset analyse
    if (rsimset < 0) {
      // default values
      if (image->width() > 64)  Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=3;
      else                      Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=2;
    } else
      Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=rsimset;


    MultiDimFloatTexture *cstr  = NULL;
    if (rgb_cstr != NULL)
      cstr = new MultiDimFloatTexture(rgb_cstr);

    // add to analyser
    g_Analyser.setExemplar(image,cstr,g_bToroidal,periodx,periody,terrain);
    if (prtreflectance != NULL) {
      assertx(prtcolormap != NULL);
      g_Analyser.exemplar()->setColorPRTData(prtcolormap,prtreflectance);
    }

    if (import_recolor)
      g_Analyser.exemplar()->importRecolorData(import_fname,import_numcomp);

    // export recoloring data?
    if (export_recolor) {
      StringCchPrintfA(str,1024,"%s.nontrsf.txt",g_Analyser.rootName().c_str());
      g_Analyser.exemplar()->exportRecolorData(str);
      cerr << endl << endl;
      cerr << "* Recoloring data exported => exiting." << endl;
      exit (0);
    }

    // Analyse data
    getcwd(str,1024);
    chdir(g_szReportDir);

    int rcode=g_Analyser.load();
    if (rcode != ANALYSER_DONE)
    {
      if ((rcode == ANALYSER_DATA_EXISTS) && window)
      {
        if (IDNO == MessageBox(
            NULL,
            "Data exists but is incompatible. Recompute ?",
            "What should I do ?",
            MB_YESNO | MB_ICONQUESTION))
          exit (-1);
      }
      g_Analyser.analyse();
      g_Analyser.save();

      chdir(str);
      getcwd(str,1024);
      cerr << "in directory " << str << endl;

      if (report)
        g_Analyser.report(g_szReportDir);
    }
    else
    {
      chdir(str);
      getcwd(str,1024);
      cerr << "in directory " << str << endl;
      cerr << endl << endl;
      cerr << " ------------------------------------ " << endl;
      cerr << "           Using saved data           " << endl;
      cerr << " ------------------------------------ " << endl;
      cerr << endl << endl;
    }
    cerr << g_Analyser.nbLevels() << " levels." << endl;

    // done.
  }
#ifndef _DEBUG
  catch (CLibTextureException& e)
  {
    if (window)
      MessageBox(NULL,e.getMsg(),"Texture error",MB_OK | MB_ICONSTOP);
    else
      cerr << e.getMsg() << endl;
  }
  catch (SynthesisException& e)
  {
    if (window)
      MessageBox(NULL,e.getMsg(),"Synthesis error",MB_OK | MB_ICONSTOP);
    else
      cerr << e.getMsg() << endl;
  }
  catch (MemoryException& e)
  {
    if (window)
      MessageBox(NULL,e.getMsg(),"Memory access error",MB_OK | MB_ICONSTOP);
    else
      cerr << e.getMsg() << endl;
  }
  catch (...)
  {
    if (window)
      MessageBox(NULL,"Unknown error","Generic Error",MB_OK | MB_ICONSTOP);
    else
      cerr << "Unknown error" << endl;
  }
#endif
  return (0);
}
*/
/* -------------------------------------------------------- */



int main(int argc, char **argv) 
{
   BuildProperties   properties;

   processCmdLineArgsSimple(argc, argv, &properties);
   convert(&properties);
}





bool processCmdLineArgsHoppe(int argc, char **argv, BuildProperties *properties)
{

   // print doc
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
   cerr << "synthtex " << endl;
   cerr << "         -rd   <dirname>   Specifies directory name for" << endl;
   cerr << "                           outputing reports and data." << endl;
   cerr << "                           If analysis data is present in" << endl;
   cerr << "                           the directory and compatible" << endl;
   cerr << "                           it will be automatically loaded." << endl;
   cerr << "         -fmap <filename>  Specifies feature map image filename" << endl;
   cerr << "         -report           Output HTML reports." << endl;
   cerr << "         -tor              Treat exemplars as toroidals." << endl;
   cerr << "         -terrrain         Treat exemplar as terrain heightfield." << endl;
   cerr << "         -periodx <int>    Specify exemplar x period." << endl;
   cerr << "         -periody <int>    Specify exemplar y period." << endl;
   cerr << "         -simset_use_recolored  Simset computation will use recolored exemplar." << endl;
   cerr << "         -rsimset <int>    Specify simset neighborhood radius (size=r*2+1). defaults to 3 (7x7)." << endl;
   cerr << "         -space [HSV|YUV]  Color space to be used" << endl;
   cerr << "         -order [012|120|...] Order of components, usefull" << endl;
   cerr << "                           if less than 3 components are used" << endl;
   cerr << "         -precision [0,1]  Precision. Less -> Faster. Typically 0.99" << endl;
   cerr << "         -epsilon [0,1]    Epsilon for nearest neighbor search. More -> Faster." << endl;
   cerr << "                           Typically 0.1" << endl;
   cerr << "         -export_recolor   Export recoloring data for external computation" << endl;
   cerr << "         -import_recolor   Import recolored data from external computation" << endl;
   cerr << endl;
   cerr << "synthtex [PARAM_LIST] MAP EXEMPLAR CONSTRAINT" << endl;
   cerr << "         MAP           : <filename> | null" << endl;
   cerr << "         EXEMPLAR      : <filename>" << endl;
   cerr << "         CONSTRAINT    : <filename> | null" << endl;
   cerr << endl;
   cerr << " Exemples:" << endl;
   cerr << endl;
   cerr << "  anisotexsyn -rd .\\texture\\ null texture.png null" << endl;
   cerr << endl;
   cerr << " Supported image formats: PNG, JPG." << endl;
   cerr << " Also supports prt files as input for analysis." << endl;
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
#ifdef USE_ANN_LIBRARY
   cerr << " -------------------------------------------------------" << endl;
   cerr << " ********* SEARCH performed with ANN library ***********" << endl;
   cerr << endl;
   cerr << endl;
   cerr << " WARNING: ANN library should **NOT** be included in any " << endl;
   cerr << "          commercial product !!!!!!!!!!!!!!!!!!!!!!!!!! " << endl;
   cerr << endl;
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
#endif

   if (argc < 2)
      return (false);

   // default parameter values

   // SAT: took out
   //int rsimset=-1;

   StringCchCopyA(properties->g_szOutputName,1024,"synth.png");
   StringCchCopyA(properties->g_szParamFile,1024,"threshold.txt");
   StringCchCopyA(properties->g_szReportDir,1024,"synthtex");
   for (int i=1;i<argc;i++)
   {
      StringCchCatA(properties->g_szReportDir,1024,"_");
      StringCchCatA(properties->g_szReportDir,1024,argv[i]);
   }
   int l=strlen(properties->g_szReportDir);
   for (int c=0;c<l;c++)
   {
      properties->g_szReportDir[c]=properties->g_szReportDir[c]==' ' ? '_' : properties->g_szReportDir[c];
      properties->g_szReportDir[c]=properties->g_szReportDir[c]=='\\' ? '_' : properties->g_szReportDir[c];
   }

   // SAT: took out
   //CTexture::convert_functor *clrspace=&rgb;

   // parse options
   int argshift=1;
   while (argshift<argc)    
   {
      if (argv[argshift][0] != '-')
         break;
      else if (!strcmp("-o",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -o <filename> : specifies output file." << endl;
            exit (-1);
         }
         StringCchCopyA(properties->g_szOutputName,1024,argv[argshift+1]);
         argshift+=2;
      } else if (!strcmp("-p",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -p <filename> : specifies parameter file." << endl;
            exit (-1);
         }
         StringCchCopyA(properties->g_szParamFile,1024,argv[argshift+1]);
         argshift+=2;
      } else if (!strcmp("-rd",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -rd <dirname> : specifies report directory." << endl;
            exit (-1);
         }
         StringCchCopyA(properties->g_szReportDir,1024,argv[argshift+1]);
         argshift+=2;
      } else if (!strcmp("-fmap",argv[argshift])) {
         if (argshift+2 >= argc)  {
            cerr << "option -fmap <filename> <weight[1.0]> : specifies feature map image and weight." << endl;
            exit (-1);
         }
         strncpy(properties->featureMapName, argv[argshift+1], MAX_STR_LEN);
         properties->featureMapWeight=(float)atof(argv[argshift+2]);
         assertx(properties->featureMapWeight >= 0.0f);
         argshift+=3;

         cerr << endl << endl << "**********************************************" << endl;
         cerr << "**********************************************" << endl;
         cerr << "      FMAP present - weight = " << properties->featureMapWeight << endl;
         cerr << "**********************************************" << endl;
         cerr << "**********************************************" << endl << endl;

      }  else if (!strcmp("-prtcolor",argv[argshift]))  {
         if (argshift+2 >= argc) {
            cerr << "option -prtcolor <filename>: specifies albedo map for PRT analysis." << endl;
            exit (-1);
         }
         strncpy(properties->prtMapName, argv[argshift+1], MAX_STR_LEN);
         argshift+=2;
      } else if (!strcmp("-export_recolor",argv[argshift])) {
         properties->export_recolor=true;
         argshift++;
      } else if (!strcmp("-import_recolor",argv[argshift]) && argshift+1 < argc) {
         properties->import_recolor=true;
         StringCchCopyA(properties->import_fname,1024,argv[argshift+1]);
         cerr << "* recoloring data will be imported from files lvlXX." << properties->import_fname << endl;
         argshift+=2;
      } else if (!strcmp("-import_numcomp",argv[argshift]) && argshift+1 < argc) {
         properties->import_numcomp=atoi(argv[argshift+1]);
         cerr << "* importing " << properties->import_numcomp << " dimensions" << endl;
         argshift+=2;
      } else if (!strcmp("-window",argv[argshift])) {
         properties->window=true;
         argshift++;
      } else if (!strcmp("-report",argv[argshift])) {
         properties->report=true;
         argshift++;
      } else if (!strcmp("-terrain",argv[argshift])) {
         properties->terrain=true;
         argshift++;
      } else if (!strcmp("-order",argv[argshift])) {
         properties->order = true;
         if (argshift+1 >= argc) {
            cerr << "option -order [012|021|...]" << endl;
            exit (-1);
         }
         properties->order_v0=argv[argshift+1][0]-'0'; 
         if (properties->order_v0 < 0) {
            cerr << "option -order [012|021|...]" << endl;
            exit (-1);
         }
         properties->order_v1=argv[argshift+1][1]-'0';
         if (properties->order_v1 < 0) {
            cerr << "option -order [012|021|...]" << endl;
            exit (-1);
         }
         properties->order_v2=argv[argshift+1][2]-'0';
         if (properties->order_v2 < 0) {
            cerr << "option -order [012|021|...]" << endl;
            exit (-1);
         }
         //order=new CTexture::convert_order(v0,v1,v2);
         argshift+=2;
      } else if (!strcmp("-space",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -space [HSV|YUV]  Color space to be used" << endl;
            exit (-1);
         }
         if (!strcmp(argv[argshift+1],"YUV"))
            properties->YUY = true;
            //clrspace=&yuv;
         else if (!strcmp(argv[argshift+1],"HSV"))
            properties->HSV = true;
            //clrspace=&hsv;
         else {
            cerr << "option -space [HSV|YUV]  Color space to be used" << endl;
            exit (-1);
         }
         argshift+=2;
      } else if (!strcmp("-tor",argv[argshift])) {
         properties->g_bToroidal=true;
         argshift++;
      } else if (!strcmp("-simset_use_recolored",argv[argshift])) {
         cerr << "* simset computation will use recolored exemplar"  << endl;
         Exemplar::s_bUseRecoloredForSimset=true;
         argshift++;
      } else if (!strcmp("-prepare",argv[argshift])) {
         // no longer relevant
         argshift++;
      } else if (!strcmp("-periodx",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -periodx <int>" << endl;
            exit (-1);
         }
         properties->periodx=atoi(argv[argshift+1]);
         argshift+=2;
      } else if (!strcmp("-periody",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -periody <int>" << endl;
            exit (-1);
         }
         properties->periody=atoi(argv[argshift+1]);
         argshift+=2;
      } else if (!strcmp("-rsimset",argv[argshift]) && argshift+1 < argc) {
         properties->rsimset=atoi(argv[argshift+1]);
         cerr << "* simset neighborhood radius set to: " << properties->rsimset << endl;
         argshift+=2;
      } else if (!strcmp("-precision",argv[argshift])) {
         if (argshift+1 >= argc)
         {
            cerr << "option -precision [0,1]" << endl;
            exit (-1);
         }
         //Exemplar::s_fPrecision=(float)atof(argv[argshift+1]);
         properties->precision = (float)atof(argv[argshift+1]);
         argshift+=2;
         cerr << " -------------------------------------------------------" << endl;
         cerr << " ********* Precision set to " << Exemplar::s_fPrecision   << endl;
         cerr << " -------------------------------------------------------" << endl;
         cerr << endl;
      } else if (!strcmp("-epsilon",argv[argshift])) {
         if (argshift+1 >= argc) {
            cerr << "option -epsilon [0,1]" << endl;
            exit (-1);
         }
         Exemplar::s_fEpsilon=(float)atof(argv[argshift+1]);
         argshift+=2;
         cerr << " -------------------------------------------------------" << endl;
         cerr << " ********* Epsilon set to " << Exemplar::s_fEpsilon   << endl;
         cerr << " -------------------------------------------------------" << endl;
         cerr << endl;
      } else if (argv[argshift][0]=='-')  {
         GLOBALPARAMETERS.addParameter(&(argv[argshift][1]));
         if (argshift+1 < argc && argv[argshift+1][0] != '-') {
            GLOBALPARAMETERS.setValue(&(argv[argshift][1]),argv[argshift+1]);
            argshift+=2;
         } else {
            GLOBALPARAMETERS.setValue(&(argv[argshift][1]),"1");
            argshift++;
         }
      } else {
         // unknow parameter - ignore
         cerr << "WARNING: ignoring parameter " << argv[argshift] << endl;
         argshift++; 
      }
   }

   assertx(!(properties->export_recolor && properties->import_recolor));

   if (argc-argshift < 2) {
      cerr << "No exemplar specified !\n" << endl;
      return (false);
   }




   // load target constraint map
   strncpy(properties->texName, argv[argshift], MAX_STR_LEN);
   argshift++;

   // exemplar image
   strncpy(properties->rgbImageName, argv[argshift], MAX_STR_LEN);

   // exemplar constraint
   strncpy(properties->rgbCstrName, argv[argshift+1], MAX_STR_LEN);


   return(true);
}


bool processCmdLineArgsSimple(int argc, char **argv, BuildProperties *properties)
{
   // print doc
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
   cerr << "anisotexsyn  EXEMPLAR" << endl;
   cerr << "         EXEMPLAR      : <filename>" << endl;
   cerr << endl;
   cerr << " Examples:" << endl;
   cerr << endl;
   cerr << "  anisotexsyn texture.tga" << endl;
   cerr << endl;
   cerr << " Supported image formats: TGA." << endl;
   cerr << " Also supports prt files as input for analysis." << endl;
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
#ifdef USE_ANN_LIBRARY
   cerr << " -------------------------------------------------------" << endl;
   cerr << " ********* SEARCH performed with ANN library ***********" << endl;
   cerr << endl;
   cerr << endl;
   cerr << " WARNING: ANN library should **NOT** be included in any " << endl;
   cerr << "          commercial product !!!!!!!!!!!!!!!!!!!!!!!!!! " << endl;
   cerr << endl;
   cerr << endl;
   cerr << " -------------------------------------------------------" << endl;
   cerr << endl;
#endif

   if (argc < 2)
   {      
      cerr << "Nothing to process." << endl;
      exit (-1);
   }

   // default parameter values
   StringCchCopyA(properties->g_szOutputName,1024,"synth.png");
   StringCchCopyA(properties->g_szParamFile,1024,"threshold.txt");
   StringCchCopyA(properties->g_szReportDir,1024,"synthtex");
   for (int i=1;i<argc;i++)
   {
      StringCchCatA(properties->g_szReportDir,1024,"_");
      StringCchCatA(properties->g_szReportDir,1024,argv[i]);
   }
   int l=strlen(properties->g_szReportDir);
   for (int c=0;c<l;c++)
   {
      properties->g_szReportDir[c]=properties->g_szReportDir[c]==' ' ? '_' : properties->g_szReportDir[c];
      properties->g_szReportDir[c]=properties->g_szReportDir[c]=='\\' ? '_' : properties->g_szReportDir[c];
   }


   // parse options
   int argshift=1;


   // texture to convert
   char fullTextureName[MAX_STR_LEN];
   char fullTextureFMapName[MAX_STR_LEN];
   char directoryName[MAX_STR_LEN];
   char textureName[MAX_STR_LEN];
   char textureNameBase[MAX_STR_LEN];

   char workDirectoryName[MAX_STR_LEN];

   strncpy(fullTextureName, argv[argshift], MAX_STR_LEN);


   // Check if texture passed in exists
   FILE *file=fopen(fullTextureName, "rb");
   if(!file)
   {
      cerr << "File does no exist:  " << fullTextureName << endl;
      exit (-1);
   }
   fclose(file);



   // Create texture name and diretory name
   //

   char *pLastSlash = strrchr(fullTextureName, '\\');
   if(pLastSlash)
   {
      int lastSlashPos = (int)(pLastSlash - fullTextureName);

      strncpy(directoryName, fullTextureName, lastSlashPos + 1);
      directoryName[lastSlashPos + 1] = '\0';
      strcpy(textureName, pLastSlash + 1);
   }
   else
   {
      strcpy(directoryName, ".\\");
      strcpy(textureName, fullTextureName);
   }


   // Create base name
   // 

   // remove extension
   char *pPeriodPos = strrchr(textureName, '.');
   int perioudPos = (int)(pPeriodPos - textureName);
   if(!pPeriodPos || stricmp(pPeriodPos, ".tga") != 0)
   {
      cerr << "Invalid file type (" << pPeriodPos << ")" << endl;
      exit (-1);
   }

   strncpy(textureNameBase, textureName, perioudPos);
   textureNameBase[perioudPos] = '\0';

   // remove type if exist (_df)
   char *pUnderscorePos = strrchr(textureNameBase, '_');
   if(pUnderscorePos && 
      (strncmp(pUnderscorePos, "_df", 3) == 0))
   {
      int underscorePos = (int)(pUnderscorePos - textureNameBase);
      textureNameBase[underscorePos] = '\0';
   }



   // Create work directory
   //

   strcpy(workDirectoryName, directoryName);
   strcat(workDirectoryName, textureNameBase);
   strcat(workDirectoryName, "\\");

   // make work directory
   mkdir(workDirectoryName);
   
   // Set property name
   strcpy(properties->g_szReportDir, workDirectoryName);



   // Build exemplar image (128x128) in work directory
   //

   char fullTextureName128[MAX_STR_LEN];
   strcpy(fullTextureName128, workDirectoryName);
   strcat(fullTextureName128, textureNameBase);
   strcat(fullTextureName128, ".ex.tga");

   resizeImage(fullTextureName, fullTextureName128);

   // Set property name
   strcpy(properties->rgbImageName, fullTextureName128);



   // Build feature map image (128x128) in work directory
   //

   strcpy(fullTextureFMapName, directoryName);
   strcat(fullTextureFMapName, textureNameBase);
   strcat(fullTextureFMapName, "_fm.tga");

   if(FILE *file=fopen(fullTextureFMapName, "rb"))
   {
      fclose(file);

      char fullTextureFMapName128[MAX_STR_LEN];
      strcpy(fullTextureFMapName128, workDirectoryName);
      strcat(fullTextureFMapName128, textureNameBase);
      strcat(fullTextureFMapName128, ".fmap.tga");

      resizeImage(fullTextureFMapName, fullTextureFMapName128);

      // Set property name
      strcpy(properties->featureMapName, fullTextureFMapName128);
   }



   // Always be toroidal
   properties->g_bToroidal = true;
   
   return(true);
}


void convert(const BuildProperties *properties)
{
   static CTexture::convert_std     rgb;
   static CTexture::convert_RGB_HSV hsv;
   static CTexture::convert_RGB_YUV yuv;
   static CTexture::convert_order  *order=NULL;

   bool                             isprt=false;
   static char                      str[1024];

   CTexture                        *featuremask=NULL;
   MultiDimFloatTexture            *featuremap=NULL;
   CTexture                        *prtcolormap=NULL;
   MultiDimFloatTexture            *prtreflectance=NULL;


   Exemplar::s_fPrecision = properties->precision;

   if(properties->order)
   {
      order=new CTexture::convert_order(properties->order_v0,properties->order_v1,properties->order_v2);
   }


   CTexture::convert_functor *clrspace=&rgb;
   if(properties->YUY)
      clrspace=&yuv;
   else if (properties->HSV)
      clrspace=&hsv;


   // load feature map
   if(strcmp(properties->featureMapName, "\0") != 0)
   {
      cerr << "=============================================================" << endl;
      cerr << "                   Using Feature Map: " << properties->featureMapName << endl;
      cerr << "=============================================================" << endl;

      featuremask=CTexture::loadTexture(properties->featureMapName);
   }

   // load radiance transfer map
   if(strcmp(properties->prtMapName, "\0") != 0)
   {
      cerr << "=============================================================" << endl;
      cerr << "                   Using Radiance Transfer Map: " << properties->prtMapName << endl;
      cerr << "=============================================================" << endl;

      prtcolormap=CTexture::loadTexture(properties->prtMapName);
   }




   GLOBALPARAMETERS.addParameter("recolor_rgb");
   GLOBALPARAMETERS.addParameter("recolor_3channels");
   GLOBALPARAMETERS.addParameter("recolor_4channels");
   GLOBALPARAMETERS.list(cerr);

   if (order != NULL)
      clrspace=new CTexture::convert_compose(*clrspace,*order);

   // must feature map be made toroidal?
   if (featuremask != NULL) {
      if (properties->g_bToroidal) {
         // enlarge to make sure distances are correct
         int w=featuremask->getWidth();
         int h=featuremask->getHeight();
         CTexture *torfmap=new CTexture(w*3,h*3,false);
         torfmap->copy(0  ,0,featuremask);
         torfmap->copy(w  ,0,featuremask);
         torfmap->copy(w*2,0,featuremask);
         torfmap->copy(0  ,h,featuremask);
         torfmap->copy(w  ,h,featuremask);
         torfmap->copy(w*2,h,featuremask);
         torfmap->copy(0  ,h*2,featuremask);
         torfmap->copy(w  ,h*2,featuremask);
         torfmap->copy(w*2,h*2,featuremask);
         delete (featuremask);
         featuremask=torfmap;
      }
      // create feature map from feature mask
      featuremap=computeFeatureDistanceMap(featuremask,true);
      delete (featuremask);
      featuremask=NULL;
   }


   /*
   // create report directory
   mkdir(properties->g_szReportDir);
   */



   // load target constraint map
   if (!strcmp("null",properties->texName))
      g_Tex=NULL;
   else  {
      g_Tex=CTexture::loadTexture(properties->texName);
      computeLevels(g_Tex,g_UserConstraint);
   }

   // exemplar image
   CTexture *rgb_image=NULL;

   if (strstr(properties->rgbImageName,".prt") != NULL)
      isprt=true;
   else
      rgb_image=CTexture::loadTexture(properties->rgbImageName);

   // exemplar constraint
   CTexture *rgb_cstr=NULL;
   if (strcmp(properties->rgbCstrName,"null"))
      rgb_cstr=CTexture::loadTexture(properties->rgbCstrName);
   else
      cerr << "  -> no constraint" << endl;

   // convert to float textures

   MultiDimFloatTexture *image = NULL;

   if (isprt) {
      LPD3DXPRTBUFFER ppBuffer=NULL;

      //SAT:  Took out d3dx9 lib.  No way to load PRT Buffers at the moment.
      assert(0);

      //D3DXLoadPRTBufferFromFile(argv[argshift],&ppBuffer);
      assertx(ppBuffer != NULL);

      cerr << "=============================================================" << endl;
      cerr << "                   Reading PRT data" << endl;
      cerr << "=============================================================" << endl;
      cerr << "Num. samples : " << ppBuffer->GetNumSamples()  << endl;
      cerr << "Num. channels: " << ppBuffer->GetNumChannels() << endl;
      cerr << "Num. coeffs. : " << ppBuffer->GetNumCoeffs()   << endl;
      int texres = (int)(sqrt((double)ppBuffer->GetNumSamples()));
      cerr << "Texture size (assumed squared): " << texres << endl;
      cerr << "=============================================================" << endl;
      assertx(1 == ppBuffer->GetNumChannels());
      if (prtcolormap != NULL) {
         cerr << "=============================================================" << endl;
         cerr << "  a color map is present!" << endl;
         cerr << "=============================================================" << endl;
         // -> expand with color map
         assertx(texres == prtcolormap->getWidth() && texres == prtcolormap->getHeight());
         image = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs()*3);
         prtreflectance = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs());
         FLOAT *data=NULL;
         ppBuffer->LockBuffer(0,ppBuffer->GetNumSamples(),&data);
         for (int j=0;j<texres;j++) {
            for (int i=0;i<texres;i++) {
               assertx(i+j*texres < (int)ppBuffer->GetNumSamples());
               int offset=i+j*texres;
               for (int k=0;k<3;k++) { // for each color channel (RGB)
                  for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++) // for each monochrome prt coeff
                     image->set(i,j,k*ppBuffer->GetNumCoeffs()+c)
                     =data[offset*ppBuffer->GetNumCoeffs() + c] * float(prtcolormap->get(i,j,k))/255.0f;
               }
               for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++)
                  prtreflectance->set(i,j,c)=data[offset*ppBuffer->GetNumCoeffs() + c];
            }
         }
         ppBuffer->UnlockBuffer();
         image->setName(properties->rgbImageName);
         prtreflectance->setName(properties->rgbImageName);
      } else {
         // -> no color map
         // creating float texture
         image = new MultiDimFloatTexture(texres,texres,ppBuffer->GetNumCoeffs());
         FLOAT *data=NULL;
         ppBuffer->LockBuffer(0,ppBuffer->GetNumSamples(),&data);
         for (int j=0;j<texres;j++) {
            for (int i=0;i<texres;i++) {
               assertx(i+j*texres < (int)ppBuffer->GetNumSamples());
               int offset=i+j*texres;
               for (int c=0;c<(int)ppBuffer->GetNumCoeffs();c++)
                  image->set(i,j,c)=data[offset*ppBuffer->GetNumCoeffs() + c];
            }
         }
         ppBuffer->UnlockBuffer();
         image->setName(properties->rgbImageName);
      } // color map
   } else if (featuremap != NULL) {
      // feature map is present
      if (featuremap->getWidth() < rgb_image->getWidth() || featuremap->getHeight() < rgb_image->getHeight()) {
         throw SynthesisException("feature map is smaller than exemplar !");
      }
      // -> only keep center part if larger than exemplar
      if (featuremap->getWidth() > rgb_image->getWidth() || featuremap->getHeight() > rgb_image->getHeight()) {
         cerr << " * cropping feature map" << endl;
         int l=(featuremap->getWidth() - rgb_image->getWidth())/2;
         int r=(featuremap->getHeight() - rgb_image->getHeight())/2;
         MultiDimFloatTexture *subfmap=featuremap->extract<MultiDimFloatTexture>(
            l,r,rgb_image->getWidth(),rgb_image->getHeight());
         delete (featuremap);
         featuremap=subfmap;
      }
      // -> built multichannel exemplar
      image = new MultiDimFloatTexture(rgb_image->getWidth(),rgb_image->getHeight(),4);
      for (int j=0;j<image->height();j++) {
         for (int i=0;i<image->width();i++) {
            // rgb
            for (int c=0;c<3;c++)
               image->set(i,j,c)=rgb_image->get(i,j,c);
            // feature dist
            image->set(i,j,3)=featuremap->get(i,j,0)*properties->featureMapWeight;
         }
      }
      image->setName(rgb_image->getName());
   }
   else
      image = new MultiDimFloatTexture(rgb_image);

   // size of neighborhood for simset analyse
   if (properties->rsimset < 0) {
      // default values
      if (image->width() > 64)  Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=3;
      else                      Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=2;
   } else
      Exemplar::s_iNEIGHBORHOOD_RADIUS_SIMSET=properties->rsimset;


   MultiDimFloatTexture *cstr  = NULL;
   if (rgb_cstr != NULL)
      cstr = new MultiDimFloatTexture(rgb_cstr);

   // add to analyser
   g_Analyser.setExemplar(image,cstr,properties->g_bToroidal,properties->periodx,properties->periody,properties->terrain);
   if (prtreflectance != NULL) {
      assertx(prtcolormap != NULL);
      g_Analyser.exemplar()->setColorPRTData(prtcolormap,prtreflectance);
   }

   if (properties->import_recolor)
      g_Analyser.exemplar()->importRecolorData(properties->import_fname,properties->import_numcomp);

   // export recoloring data?
   if (properties->export_recolor) {
      StringCchPrintfA(str,1024,"%s.nontrsf.txt",g_Analyser.rootName().c_str());
      g_Analyser.exemplar()->exportRecolorData(str);
      cerr << endl << endl;
      cerr << "* Recoloring data exported => exiting." << endl;
      exit (0);
   }

   // Analyse data
   getcwd(str,1024);
   chdir(properties->g_szReportDir);

   int rcode=g_Analyser.load();
   if (rcode != ANALYSER_DONE)
   {
      if ((rcode == ANALYSER_DATA_EXISTS) && properties->window)
      {
         if (IDNO == MessageBox(
            NULL,
            "Data exists but is incompatible. Recompute ?",
            "What should I do ?",
            MB_YESNO | MB_ICONQUESTION))
            exit (-1);
      }
      g_Analyser.analyse();
      g_Analyser.save();

      chdir(str);
      getcwd(str,1024);
      cerr << "in directory " << str << endl;

      if (properties->report)
         g_Analyser.report(properties->g_szReportDir);
   }
   else
   {
      chdir(str);
      getcwd(str,1024);
      cerr << "in directory " << str << endl;
      cerr << endl << endl;
      cerr << " ------------------------------------ " << endl;
      cerr << "           Using saved data           " << endl;
      cerr << " ------------------------------------ " << endl;
      cerr << endl << endl;
   }
   cerr << g_Analyser.nbLevels() << " levels." << endl;


   return;
}


/*
void resizeImageIfNeeded(CTexture **texture)
{
   if(((*texture)->getWidth() <= 128) || ((*texture)->getHeight() <= 128))
      return;

   char savedName[1024];
   strcpy(savedName, (*texture)->getName());

   CTexture::saveTexture(*texture,"temp_hoppe1.png");
   free(*texture);

   system( "FilterImage.exe temp_hoppe1.png -scaletox 128 > temp_hoppe2.png");

   *texture = CTexture::loadTexture("temp_hoppe2.png");
   (*texture)->setName(savedName);

   system( "del temp_hoppe1.png temp_hoppe2.png");
}
*/