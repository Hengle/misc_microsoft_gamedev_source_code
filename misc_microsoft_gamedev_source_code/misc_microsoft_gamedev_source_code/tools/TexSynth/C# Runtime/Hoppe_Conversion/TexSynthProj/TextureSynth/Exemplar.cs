/*===================================
  Exemplar.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis (MGS Ensemble studios) [12.01.06]
===================================*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Rendering;
using System.IO;
using EditorCore;

namespace TextureSynthesis
{
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   class knearest_id
   {
      public knearest_id() { l = (-1); i = (0); j = (0); s = (0); }
      public knearest_id(int ll, int ii, int jj, int ss) { l = (ll); i = (ii); j = (jj); s = (ss); }
      public int i;
      public int j;
      public int s;
      public int l;
      public bool cmp(knearest_id id) { return (id.l == l && id.i == i && id.j == j && id.s == s); }
   };

   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   //CLM
   //This class stores the runtime information needed for similaity sets and projected synth neighborhoods
   class KNearestSimilartySet
   {
      public KNearestSimilartySet(StreamReader f, Exemplar owner,int numLevels)
      {
         mOwner = owner;
         mNumLevels = numLevels;
         load(f);
      }
      ~KNearestSimilartySet()
      {
         m_SimilaritySet.Clear();
         mOwner = null;
         m_SynthNeighborhoods = null;
         m_ProjectedSynthNeighborhoods = null;
      }
      void load(StreamReader f)
      {
         // read knearests tables
         m_SimilaritySet = new List<List<List<knearest_id>>>();
         for (int ml = 0; ml < mNumLevels; ml++)
            m_SimilaritySet.Add(new List<List<knearest_id>>());


         for (int l = 0; l < mNumLevels; l++)
         {
            int nbkn = Convert.ToInt32(f.ReadLine());
            int w = mOwner.stack(l).width();
            int h = mOwner.stack(l).height();
            m_SimilaritySet[l] = new List<List<knearest_id>>();//= new knearest_id[w * h][];
            for (int ml = 0; ml < w * h; ml++)
               m_SimilaritySet[l].Add(new List<knearest_id>());//.Add(new List<List<knearest_id>>());

            for (int k = 0; k < nbkn; k++)
            {
               String name = f.ReadLine();
               name = Path.ChangeExtension(name, ".bin");// +".KNS";

               Globals.Assert(File.Exists(name));

               if (k < Globals.K_NEAREST)
               {
                  BinaryReader br = new BinaryReader(File.Open(name, FileMode.Open, FileAccess.Read));
                  int memsize = br.ReadInt32();
                  Byte[] buff = br.ReadBytes(memsize);
                  br.Close();
                  int numComp = 3;

                  for (int ti = 0; ti < w; ti++)
                  {
                     for (int tj = 0; tj < h; tj++)
                     {
                        if (buff[(ti + tj * w) * numComp + 2] < 255)
                           m_SimilaritySet[l][ti + tj * w].Add(new knearest_id(l, buff[(ti + tj * w) * numComp + 0], buff[(ti + tj * w) * numComp + 1], mOwner.getID()));

                     }
                  }
               }
            }
         }

      }

      public void computeRuntimeNeighborhoods(List<PCA/*_Synth*/> pcaSynth)
      {
         m_SynthNeighborhoods = null;
         m_ProjectedSynthNeighborhoods = null;
         computeSynthNeighborhoods();
         computeProjectedSynthNeighborhoods(pcaSynth);
         m_SynthNeighborhoods = null;
      }


      void computeSynthNeighborhoods()
      {
         ScopeTimer tm = new ScopeTimer("[computeSynthNeighborhoods]");

         m_SynthNeighborhoods = new NeighborhoodSynth[mNumLevels][];

         // foreach level
         for (int level = 0; level < mNumLevels; level++)
         {
            MultiDimFloatTexture recolored_level = null;

            if (Globals.isDefined("4D"))
            {
               // . keep only 4 dimension from recolored exemplar
               MultiDimFloatTexture level_4D = new MultiDimFloatTexture(mOwner.recoloredStack(level).width(), mOwner.recoloredStack(level).height(), mOwner.recoloredStack(level).numComp());
               int w = level_4D.getWidth();
               int h = level_4D.getHeight();
               Globals.Assert(w ==mOwner.stack(level).getWidth() && h == mOwner.stack(level).height());
               Globals.Assert(level_4D.numComp() == Globals.NUM_RECOLORED_PCA_COMPONENTS);
               for (int i = 0; i < w; i++)
               {
                  for (int j = 0; j < h; j++)
                  {
                     // . copy first four channels
                     for (int c = 0; c < 4; c++)
                        level_4D.set(mOwner.recoloredStack(level).get(i, j, c), i, j, c);
                     // . zero out all channels above 4
                     for (int c = 4; c < level_4D.numComp(); c++)
                        level_4D.set(0, i, j, c);
                  }
               }
               recolored_level = level_4D;
            }
            else
            {
               // . keep all dimensions
               recolored_level = mOwner.recoloredStack(level);
            }

            m_SynthNeighborhoods[level] = new NeighborhoodSynth[recolored_level.width() * recolored_level.height()];

            stack_accessor_v2 access = new stack_accessor_v2(level);


            for (int j = 0; j < recolored_level.height(); j++)
            {
               for (int i = 0; i < recolored_level.width(); i++)
               {
                  int index = i + j * recolored_level.width();
                  m_SynthNeighborhoods[level][index] = new NeighborhoodSynth();
                  m_SynthNeighborhoods[level][index].construct(
                                                 recolored_level,
                                                 access,
                                                 (!mOwner.isToroidal()) && level < (mNumLevels - Globals.NUM_LEVELS_WITHOUT_BORDER),
                     //(!m_bToroidal) && l < FIRST_LEVEL_WITH_BORDER,
                                                 level, i, j);
               }
            }
         }

         tm.destroy();
         tm = null;

      }

      void computeProjectedSynthNeighborhoods(List<PCA/*_Synth*/> pcaSynth)
      {

         bool clean_neigh = false;

         // if synth neighborhoods not available, compute them
         if (m_SynthNeighborhoods == null)
         {
            computeSynthNeighborhoods();
            clean_neigh = true;
         }

         {
            ScopeTimer tm = new ScopeTimer("[computeProjectedSynthNeighborhoods]");

            m_ProjectedSynthNeighborhoods = new NeighborhoodSynth[mNumLevels][];
            // foreach level
            for (int l = 0; l < mNumLevels; l++)
            {
               m_ProjectedSynthNeighborhoods[l] = new NeighborhoodSynth[mOwner.recoloredStack(l).getWidth() * mOwner.recoloredStack(l).getHeight()];
               for (int j = 0; j < mOwner.recoloredStack(l).getHeight(); j++)
               {
                  for (int i = 0; i < mOwner.recoloredStack(l).getWidth(); i++)
                  {
                     m_ProjectedSynthNeighborhoods[l][i + j *mOwner.recoloredStack(l).getWidth()] = new NeighborhoodSynth();
                     NeighborhoodSynth neigh = m_SynthNeighborhoods[l][i + j * mOwner.recoloredStack(l).getWidth()];
                     NeighborhoodSynth proj = m_ProjectedSynthNeighborhoods[l][i + j * mOwner.recoloredStack(l).getWidth()];
                     pcaSynth[l].project(neigh, Globals.NUM_RUNTIME_PCA_COMPONENTS, ref proj);
                     proj.setIJ(i, j);
                  }
               }
            }
            tm.destroy();
            tm = null;
         }


         // clean RT neighborhoods
         if (clean_neigh)
            m_SynthNeighborhoods = null;

      }

      
      #region MEMBERS

      Exemplar mOwner = null;

      int mNumLevels = 0;
      
      NeighborhoodSynth[][] m_SynthNeighborhoods = null;// Store synth neighborhoods (after computeSynthNeighborhoods has been called)

      //projected neighborhoods
      NeighborhoodSynth[][] m_ProjectedSynthNeighborhoods = null;// Store projected synth neighborhoods (after computeProjectedSynthNeighborhoods has been called)
      public NeighborhoodSynth getProjectedSynthNeighborhood(int l, int i, int j)
      {
         return (m_ProjectedSynthNeighborhoods[l][i + j * mOwner.stack(l).getWidth()]);
      }

      //similar set
      List<List<List<knearest_id>>> m_SimilaritySet;// Store k-nearests information
      public int similaritySetSize(int l, int i, int j)
      {
         return ((int)m_SimilaritySet[l][i + j * mOwner.stack(l).getWidth()].Count);
      }
      public Pair<int, int> similarPixel(int l, int i, int j, int k)
      {
         knearest_id nfo = m_SimilaritySet[l][i + j * mOwner.stack(l).getWidth()][k];
         if (nfo == null)
            return new Pair<int, int>(0, 0);
         return (new Pair<int, int>(nfo.i, nfo.j));
      }


      #endregion
   };


   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   //CLM [12.06.06]
   //We define exemplars to have specific synth parameters that are different from other exemplars
   class ExemplarSynthParams
   {
      public ExemplarSynthParams()
      {
         mPerLevelJitterRandomness = new int[Globals.cNumLevels] { 0, 0, 0, 0, 0, 0, 0, 0 };
         mPerLevelDoCorrection = new bool[Globals.cNumLevels] { true, true, true, true, true, true, true, true };
         mConstraintThres = 99;
         mCoherenceThres = 0;
         mCutAvoidance = 0;
         TextureScale = 0.55000001f;
      }
      public int[]   mPerLevelJitterRandomness;
      public bool[]  mPerLevelDoCorrection;
      public int     mConstraintThres;
      public int     mCoherenceThres;
      public float   mCutAvoidance;

      float   mTextureScale;
      public float TextureScale
      {
         get
         {
            return mTextureScale;
         }
         set
         {
            mTextureScale = value;
            if (mTextureScale > 1) mTextureScale = 1;
            if (mTextureScale < 0) mTextureScale = 0;
         }
      }

      public int mStartAtLevel = Globals.cNumLevels - 1;
     
   }

   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------
   // -----------------------------------------------------



   class Exemplar
   {
      String stripToRootName(String fname)
      {
         String name = Path.GetFileNameWithoutExtension(fname);
         int k = name.LastIndexOf('.');
         if (k != -1)
            name = name.Substring(0, k);

         return name;
      }

      public Exemplar(String filename)
      {
         //we're given a filename
         mOrigionalExemplarTexture = BRenderDevice.getTextureManager().getTexture(filename);

         String rootFilename = stripToRootName(filename);
         m_name = rootFilename;

         
         StreamReader f = new StreamReader(File.Open(rootFilename+ ".analyse", FileMode.Open, FileAccess.Read));

         loadAnalysisFile(f);

         f.Close();

         createInternalExemplar();
      }
      ~Exemplar()
      {
         for (int i = 0; i < (int)m_Stack.Length; i++)
            m_Stack[i]=null;
         m_Stack=null;

         if (m_Constraints!=null)
         {
            for (int i = 0; i < (int)m_Constraints.Length; i++)
               m_Constraints[i] = null;
            m_Constraints = null;
         }
         
        for (int i = 0; i < (int)m_RecoloredStack.Length; i++)
            m_RecoloredStack[i]=null;
         m_RecoloredStack = null;

         for (int i = 0; i < (int)m_LevelsData.Length; i++)
            m_LevelsData[i] = null;
         m_LevelsData = null;

         if (mRandomTexture!=null)
         {
            mRandomTexture.Dispose();
            mRandomTexture = null;
         }
         if (mInternalExemplarTexture!=null)
         {
            mInternalExemplarTexture.Dispose();
            mInternalExemplarTexture = null;
         }
         
         mHighResExemplarTexture = null;
         mOrigionalExemplarTexture = null;

         mKNS = null;
         m_PCASynth.Clear();
         m_PCASynth = null;
         m_PCASynth_4Drecoloring.Clear();
         m_PCASynth_4Drecoloring = null;
      }

      bool loadAnalysisFile(StreamReader f)
      {

         // compatibility check
         //CLM this seems like a bunch of shit......
         //this should be a per-exemplar dataset, not global?
         //and it should actually be goddamn used!
         //or alteast just a goddamn header version cehck?

         int v;
         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v >= Globals.K_NEAREST);//return (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);

         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == 0);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);

         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == Globals.NEIGHBORHOOD_RADIUS_RECOLOR);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);

         v = Convert.ToInt32(f.ReadLine());

         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == Globals.NUM_RECOLORED_PCA_COMPONENTS);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);

         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == Globals.NUM_RUNTIME_PCA_COMPONENTS);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);

         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == 1);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);
         v = Convert.ToInt32(f.ReadLine());
         Globals.Assert(v == 1);// (eAnalyserLoadResult.ANALYSER_DATA_EXISTS);


         //read number of levels
         m_iNbLevels = Convert.ToInt32(f.ReadLine());

         m_PCASynth = new List<PCA/*PCA_SYNTH*/>(m_iNbLevels);
         m_PCASynth_4Drecoloring = new List<PCA/*PCA_SYNTH*/>(m_iNbLevels);

         int nbex = Convert.ToInt32(f.ReadLine()); // for backward compatibility
         Globals.Assert(nbex == 1);
            //return false;// eAnalyserLoadResult.ANALYSER_DATA_EXISTS;

         //load our PCA Synth Vectors
         for (int i = 0; i < m_iNbLevels; i++)
         {
            m_PCASynth.Add(new PCA()/*PCA_synth*/);
            bool success = m_PCASynth[i].load(f);

            Globals.Assert(success);
         }

         //load our exemplar
         loadExemplarData(f);

         //load our PCA synth 4d recolored data
         bool recompute = false;
         bool updated = false;
         for (int l = 0; l < (int)m_iNbLevels; l++)
         {
            m_PCASynth_4Drecoloring.Add(new PCA()/*PCA_synth*/);
            bool success = m_PCASynth_4Drecoloring[l].load(f);
            Globals.Assert(success);
         }
         

         computeRuntimeNeighborhoods(pcaSynth());
         generateD3DLevelData();


         return true;// eAnalyserLoadResult.ANALYSER_DONE;
      }
      bool loadExemplarData(StreamReader f)
      {
         //grab our ID
         m_iExemplarId = Convert.ToInt32(f.ReadLine());

         // read exemplar flags
         int flags = Convert.ToInt32(f.ReadLine());
         m_bToroidal = (flags & (int)eExemplarFlags.EXEMPLAR_TOROIDAL_FLAG) != 0 ? true : false;

         // read periods
         m_iPeriodX = Convert.ToInt32(f.ReadLine());
         m_iPeriodY = Convert.ToInt32(f.ReadLine());

         // read number of levels
         m_iNbLevels = Convert.ToInt32(f.ReadLine());

         // read exemplar name & load our image stack
         String exname =f.ReadLine();
         MultiDimFloatTexture image = new MultiDimFloatTexture(exname);
         {
            bool succede = computeStackLevels(image, m_bToroidal, ref m_Stack) == m_iNbLevels;
            Globals.Assert(succede);
           
         }
         m_Stack[0].setName(image.getName());
          



         //read our presentation flagss
         String name = null;
         int present = Convert.ToInt32(f.ReadLine());
         if ((present & (int)eExemplarFlags.FLAG_CONSTRAINT) !=0)
         {
            // read constraint texture name
            name = f.ReadLine();
            MultiDimFloatTexture constraint=new MultiDimFloatTexture(name);
            {
               bool succede = (computeStackLevels(constraint, m_bToroidal, ref m_Constraints) == m_iNbLevels);
               Globals.Assert(succede);
            }
            m_Constraints[0].setName(constraint.getName());
         }
         if ((present & (int)eExemplarFlags.FLAG_FMAP) != 0)
         {
            // NOTE: feature map is no longer saved
            //       this is done for backward comp.
            name = f.ReadLine();
         }
         if ((present & (int)eExemplarFlags.FLAG_PRTCOLORMAP) != 0)
         {
            name = f.ReadLine();
            //m_PRTColorMap=CTexture::loadTexture(name.c_str());
         }


         //load our Knearest neighbors (see TONG et al. 2002 "Synthesis of bidirectional texture functions on arbitrary surfaces.")
         mKNS = new KNearestSimilartySet(f, this, m_iNbLevels);
         


       
        // load recolored exemplar (see HOPPE et al. 2006 "Appearance Space Texture Synthesis")
        m_RecoloredStack = new MultiDimFloatTexture[m_iNbLevels];
         for (int l = 0; l < m_iNbLevels; l++)
        {
           name = f.ReadLine();
          m_RecoloredStack[l] = new MultiDimFloatTexture(name);
        }

        loadHighResExemplarImg(m_name);
        loadRandomTexture(m_name);
         
         return true;
      }
      unsafe void createInternalExemplar()
      {
         // RGB exemplar texture
         mInternalExemplarTexture = new Texture(BRenderDevice.getDevice(), stack(0).getWidth(), stack(0).getHeight(), 1, 0, Format.A8R8G8B8, Pool.Managed);

         for (int l = 0; l < (int)Globals.cNumLevels; l++)
         {
            // . fill
            GraphicsStream texstream = mInternalExemplarTexture.LockRectangle(0, LockFlags.None);
            byte* data = (byte*)texstream.InternalDataPointer;
            int rectPitch = stack(0).getWidth() * 4;
            for (int i = 0; i < stack(0).getWidth(); i++)
            {
               for (int j = 0; j < stack(0).getHeight(); j++)
               {
                  data[i * 4 + j * rectPitch + 2] = Globals.FLOAT_2_UCHAR(stack(0).get(i, j, 0));
                  data[i * 4 + j * rectPitch + 1] = Globals.FLOAT_2_UCHAR(stack(0).get(i, j, 1));
                  data[i * 4 + j * rectPitch + 0] = Globals.FLOAT_2_UCHAR(stack(0).get(i, j, 2));
                  data[i * 4 + j * rectPitch + 3] = 0;
               }
            }
            mInternalExemplarTexture.UnlockRectangle(0);
         }
      }
      // -----------------------------------------------------
      public void loadHighResExemplarImg(String rootname)
      {
         if (File.Exists(rootname + ".highres.png"))
         {
            mHighResExemplarTexture = BRenderDevice.getTextureManager().getTexture(rootname + ".highres.png");
         }
         else
         {
            mHighResExemplarTexture = null;
         }

      }
      void loadRandomTexture(String rootname)
      {
         //do we have a random map?
         if (File.Exists(rootname + ".random.png"))
         {
            TextureHandle RandomTextureFile = BRenderDevice.getTextureManager().getTexture(rootname + ".random.png");
            mRandomTexture = RandomTextureFile.mTexture;

            //Huzzah! We have a random texture!
            //build our mipmap levels internal
            //Surface s = RandomTextureFile.mTexture.GetSurfaceLevel(0);

            //mRandomTexture = Texture(BRenderDevice.getDevice(), s.Description.Width, s.Description.Height, 1, Usage.None, Format.A8R8G8B8, Pool.Default);

            ////copy the data over from the file to our local file
            //GraphicsStream inputStream = RandomTextureFile.mTexture.LockRectangle(0, LockFlags.None);
            //byte* data = (byte*)inputStream.InternalDataPointer;
            //int rectPitch = s.Description.Format;
            //for (int j = 0; j < a.exemplar().recoloredStack(l).height(); j++)
            //{
            //   for (int i = 0; i < a.exemplar().recoloredStack(l).width(); i++)
            //   {
            //   }
            //}
            //RandomTextureFile.mTexture.UnlockRectangle(0);
            ////build our mip-map chain (higher levels contain min of lower 4 pixels)

            //s.Dispose();
            //s =null;
         }
         else
         {
            mRandomTexture = null;
         }
      }
      // -----------------------------------------------------


      int computeStackLevels(MultiDimFloatTexture tex, bool toroidal, ref MultiDimFloatTexture[] _lvls)
      {
        MultiDimFloatTexture large=enlargeTexture(tex,toroidal ? 0 : 1);
        int nlvl=0;
        if (Globals.isDefined("gauss") || Globals.FORCE_GAUSS)
        {
           if(Globals.PRINT_DEBUG_MSG)Debug.Print("(using Gauss filter)");
          nlvl = computeStackLevels_Gauss(large,ref _lvls);
        }
        else
          nlvl=computeStackLevels_Box(large,ref _lvls);
        large=null;
        return (nlvl);
      }
      int computeStackLevels_Box(MultiDimFloatTexture tex, ref MultiDimFloatTexture[] _lvls)
      {

        List<MultiDimFloatTexture> tmplvls = new List<MultiDimFloatTexture>();

        // compute stack of averages on large texture
        // . add unmodifed finest level
        tmplvls.Clear();
        tmplvls.Add(tex);
        int l=0;

        MultiDimFloatTexture lvl=tex;
        float []lvlDat = lvl.getData();
        int num=(int)(0.01 + BMathLib.log2((float)lvl.getWidth()));

        do  // TODO: speed this up - extremely unefficient !!! (not critical though)
        {
          stack_accessor_v2 access = new stack_accessor_v2(l); // l is destination level for children
                                                             // (children *are* at level l)

          MultiDimFloatTexture stackl=new MultiDimFloatTexture(lvl.getWidth(),lvl.getHeight(),lvl.numComp());

         List<float> avg = new List<float>();
            for(int q=0;q<lvl.numComp();q++)
               avg.Add(0);

          for (int i=0;i<stackl.getWidth();i++)
          {
            for (int j=0;j<stackl.getHeight();j++)
            {
              for(int q=0;q<avg.Count;q++)
                 avg[q]=0;

              for (int li=0;li<2;li++)
              {
                for (int lj=0;lj<2;lj++)
                {
                  int ci=0,cj=0;
                  access.child_ij(i,j,li,lj,ref ci,ref cj);
                   //CLM large amount of modification here.. check back w/ origional if problems occur
                  if (ci < 0 || ci >= lvl.getWidth() ||
                     cj < 0 || cj >= lvl.getHeight())
                     continue;

                  int idx =lvl.getpixmodIndex(ci, cj);
                  for (int c=0;c<stackl.numComp();c++)
                     avg[c] += lvlDat[idx + c];
                }
              }
            
              
              int opix=stackl.getpixIndex(i,j);
              for (int c=0;c<stackl.numComp();c++)
                 stackl.getData()[opix+c] = avg[c] / 4.0f;
               
            }
          }

          lvl=stackl;
          tmplvls.Add(lvl);
          l++;

        }  while ((1 << l) <= lvl.getWidth());

        // clamp stack
        int w=tex.getWidth()/2;
        int h=tex.getHeight()/2;

        _lvls = new MultiDimFloatTexture[m_iNbLevels];

        l=0;
        int sz=w;
         for(int i=0;i<tmplvls.Count;i++)
         {
            MultiDimFloatTexture ktex = tmplvls[i];
          MultiDimFloatTexture clamped=ktex.extract(w/2,h/2,w,h);
          _lvls[i]=clamped;

          l++;
          sz >>= 1;
          if (sz < 1)
            break;
        }

        for (int i = 0; i < tmplvls.Count; i++) 
           tmplvls[i] = null;
        
        tmplvls.Clear();

        return ((int)_lvls.Length);  

      }
      int computeStackLevels_Gauss(MultiDimFloatTexture tex, ref MultiDimFloatTexture[] _lvls)
      {
         Globals.Assert(false);
         /*
        static char str[256];
        //SimpleTimer tm("\n[Exemplar::computeStackLevels] %02d min %02d sec %d ms\n");

        List<MultiDimFloatTexture > tmplvls;

        // compute stack of averages on large texture

        // . finest level
        MultiDimFloatTexture first=tex;
        MultiDimFloatTexture lvl  =first;
        tmplvls.clear();
        tmplvls.push_back(tex);

        int numlvl=(int)(0.01 + log2((float)lvl.width()));
        int l=0;
        // . compute successive filtered versions - FIXME: stack alignement ??
        do
        {
          MultiDimFloatTexture *next=first.applyGaussianFilter_Separable((1 << l)*2+1); // FIXME size ?
          if (lvl != first)
            delete (lvl);
          lvl=next;

          tmplvls.push_back(lvl);

          l++;
        }  while ((1 << l) < lvl.width());

        // clamp stack
        int w=tex.getWidth()/2;
        int h=tex.getHeight()/2;
        _lvls.clear();

        int sz=w;
        l=0;
        for (vector<MultiDimFloatTexture *>::iterator L=tmplvls.begin();L!=tmplvls.end();L++)
        {
          int oi=0,oj=0;
          

          MultiDimFloatTexture *clamped=(*L).extract<MultiDimFloatTexture>(w/2+oi,h/2+oj,w,h);
          _lvls.push_back(clamped);
      #ifdef SAVE_STACK
          CTexture *tmp=clamped.toRGBTexture();
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
          * */
         return 0;
      }

      // -----------------------------------------------------

      MultiDimFloatTexture enlargeTexture(MultiDimFloatTexture ex,int type)
      {
        ScopeTimer tm = new ScopeTimer("[Exemplar::enlargeTexture]");

        int w=ex.getWidth();
        int h=ex.getHeight();
        MultiDimFloatTexture large=new MultiDimFloatTexture(w*2,h*2,ex.numComp());

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

            for (int c=0;c<ex.numComp();c++)
              large.set(ex.get(ri,rj,c),i,j,c);
          }
        }
        tm.destroy();
        tm = null;
        return (large);
      }

      //CLM this should be removed...
      public void computeRuntimeNeighborhoods(List<PCA/*_Synth*/> pcaSynth)
      {
         mKNS.computeRuntimeNeighborhoods(pcaSynth);
      }
      public void generateD3DLevelData()
      {
         m_LevelsData = new LevelDataD3DPacket[m_iNbLevels];
         //generate our level data
         for (int l = 0; l < (int)m_iNbLevels; l++)
            m_LevelsData[l]= new LevelDataD3DPacket(this, l);
        
      }

      #region MEMBERS
      enum eExemplarFlags
      {
         //types
         EXEMPLAR_TOROIDAL_FLAG = 1,

         //prsent
         FLAG_CONSTRAINT = 1,
         FLAG_FMAP = 2,
         FLAG_PRTCOLORMAP = 4,
      }

      MultiDimFloatTexture []m_Stack = null;// Store all levels - stack
      MultiDimFloatTexture []m_RecoloredStack = null;// Store recolored stack
      MultiDimFloatTexture []m_Constraints = null;// Store constraint values per pixel (may be empty) - stack
      LevelDataD3DPacket   []m_LevelsData = null;// Textures and level information for synthesis

      KNearestSimilartySet mKNS = null;

      int m_iExemplarId;
      int m_iNbLevels;

      bool m_bToroidal;// Is exemplar toroidal ?

      int m_iPeriodX;
      int m_iPeriodY;

      String m_name;

      // PCA analysis of the neighborhoods of appearence vectors
      List<PCA/*PCA_Synth*/ > m_PCASynth = null;
      List<PCA/*PCA_Synth*/ > m_PCASynth_4Drecoloring = null;
      public List<PCA/*_Synth*/> pcaSynth()
      {
         if (Globals.isDefined("4D"))
         {
            return (m_PCASynth_4Drecoloring);
         }
         else
         {
            return (m_PCASynth);
         }
      }

      public ExemplarSynthParams mSynthParams = new ExemplarSynthParams();

      //advanced accessors
      public NeighborhoodSynth getProjectedSynthNeighborhood(int l, int i, int j)
      {
         return mKNS.getProjectedSynthNeighborhood(l, i, j);
      }
      public int similaritySetSize(int l, int i, int j)
      {
         return mKNS.similaritySetSize(l, i, j);
      }
      public Pair<int, int> similarPixel(int l, int i, int j, int k)
      {
         return mKNS.similarPixel(l, i, j, k);
      }
      public bool isToroidal()
      {
         return m_bToroidal;
      }
      public LevelDataD3DPacket levelData(int l)
      {
         return m_LevelsData[l];
      }
      public int getID()
      {
         return m_iExemplarId;
      }
      public MultiDimFloatTexture recoloredStack(int l)
      {
         return m_RecoloredStack[l];
      }
      public MultiDimFloatTexture stack(int l)
      {
         return m_Stack[l];
      }
      public bool isConstrained()
      {
         if (m_Constraints == null)
            return false;

         return (m_Constraints.Length != 0);
      }

      // Exemplar randomness map
      Texture mRandomTexture = null;
      public Texture getRandomTexture()
      {
         return mRandomTexture;
      }

      TextureHandle mHighResExemplarTexture = null;
      public TextureHandle getHighResExemplar()
      {
         return mHighResExemplarTexture;
      }

      TextureHandle mOrigionalExemplarTexture = null;

    



      Texture mInternalExemplarTexture = null; //internal version used for shaders
      public Texture getInternalExemplarTexture()
      {
         return mInternalExemplarTexture;
      }

      #endregion

   }
}