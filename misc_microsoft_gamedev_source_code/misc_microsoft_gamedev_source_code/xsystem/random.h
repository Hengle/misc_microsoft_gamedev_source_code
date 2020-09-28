//==============================================================================
// random.h
//
// Copyright (c) 1999-2000, Ensemble Studios
//==============================================================================
#pragma once

#ifndef _RANDOM_H_
#define _RANDOM_H_

// rg [7/21/06] - None of this is thread-safe. Asserts if called from any thread but sim.

// control whether we want to record file and line information for all rands.. this 
// wants to be OFF in a final release build
#ifdef _DEBUG
   #define RAND_DEBUG
#endif


//==============================================================================
// INSTRUCTIONS
// If you want to add a new random tag:
// 1. add it to the BRandomTag enum below
// 2. add a creation line to BRandomManager::BRandomManager() (random.cpp)

//==============================================================================
// Random tag enum
enum
{
   cSoundRand,
   cUIRand,
   cSimRand,
   cRMRand,
   cUnsyncedRand,
   cMixRand,
   cEditorRand,
   cAIRand,
   cRandAmount
};

//==============================================================================
// Random macros
//#ifdef RAND_DEBUG // FIXME: This only wants to be turned off in final release, not just non _DEBUG
   #define getRand(tag)                           gRandomManager._getRandNS(tag)
   #define getRandDistribution(tag)               gRandomManager._getRandDistributionNS(tag)
   #define getRandRange(tag, minVal, maxVal)      gRandomManager._getRandRangeNS(tag, minVal, maxVal)
   #define getRandRangeDWORD(tag, minVal, maxVal) gRandomManager._getRandRangeDWORDNS(tag, minVal, maxVal)
   #define getRandRangeFloat(tag, minVal, maxVal) gRandomManager._getRandRangeFloatNS(tag, minVal, maxVal)
   #define getRandGaussian(tag, mean, stddev)     gRandomManager._getRandGaussianNS(tag, mean, stddev)
   #define getRandMax(tag, max)                   gRandomManager._getRandNS(tag, max)
   #define setRandSeed(tag, v)                    gRandomManager._setSeedNS(tag, v)
   #define getRandSeed(tag)                       gRandomManager._getSeed(tag)
/*#else // in release build, we dont want to record file and line, it will slow things down
   #define getRand(tag)                   game->getRandomManager()->_getRand(tag)
   #define getRandMax(tag, max)           game->getRandomManager()->_getRand(tag, max)
   #define setRandSeed(tag, v)            game->getRandomManager()->_setSeed(tag, v)
   #define getRandSeed(tag)               game->getRandomManager()->_getSeed(tag)
#endif*/


//==============================================================================
//lint -e1927 -e1401

class BRandom1
{
   public:
      BRandom1(long s = GetTickCount())
      {
         s;
      }

      void _setSeed(long s)
      {
         seed = s;
      }

      double getNext() 
      {
        long k;
        double uniform;
        k = seed / 54532;
        seed = 39373 * (seed - k * 54532) - k * 1481;
        if (seed < 0)
	      seed += 2147483647;
        uniform = double(seed) * 4.656612E-10;
        return uniform;
      }

      long getRandom(void) { return (long)(getNext() * 32767.0f); }

      void setSeed(long s) { seed = s; }
      long getSeed() const {return seed;}

      bool                    getSync(void) const {return(mSync);}
      void                    setSync(bool sync) {mSync=sync;}

      bool                 mSync;      
private:
      long seed;


}; // BRandom1

//lint +e1927 +e1401
//==============================================================================
//lint -e1927

//==============================================================================
// class BRandom2
// 
// jce 7/18/2000 -- this sucks royally... for example, it will consistently produce 
// results that when %'ed with 256 are either 128 or 0
//==============================================================================
class BRandom2
{
   public:
      BRandom2(long dw1 = -1, long dw2 = -1)
      {
         if (dw1 < 0)
            dw1 = GetTickCount() % 31328;

         if (dw2 < 0)
            dw2 = (GetTickCount() << 8) % 30081;

         initialize(dw1, dw2);
         mSync=false;
      }


      double u[97],c,cd,cm;
      long i97,j97;

      void initialize(long ij,long kl)
      {
         double s,t;
         long ii,i,j,k,l,jj,m;

         /*
            Handle the seed range errors
               First random number seed must be between 0 and 31328
               Second seed must have a value between 0 and 30081
         */
         if (ij < 0 || ij > 31328 || kl < 0 || kl > 30081) {
            ij = 1802;
            kl = 9373;
         }

         i = (ij / 177) % 177 + 2;
         j = (ij % 177)       + 2;
         k = (kl / 169) % 178 + 1;
         l = (kl % 169);

         for (ii=0; ii<97; ii++) {
            s = 0.0;
            t = 0.5;
            for (jj=0; jj<24; jj++) {
               m = (((i * j) % 179) * k) % 179;
               i = j;
               j = k;
               k = m;
               l = (53 * l + 1) % 169;
               if (((l * m % 64)) >= 32)
                  s += t;
               t *= 0.5;
            }
            u[ii] = s;
         }

         c    = 362436.0 / 16777216.0;
         cd   = 7654321.0 / 16777216.0;
         cm   = 16777213.0 / 16777216.0;
         i97  = 97;
         j97  = 33;
      }

      double getRandomUniform(void)
      {
         double uni;

         /* Make sure the initialisation routine has been called */

         uni = u[i97-1] - u[j97-1];
         if (uni <= 0.0)
            uni++;
         u[i97-1] = uni;
         i97--;
         if (i97 == 0)
            i97 = 97;
         j97--;
         if (j97 == 0)
            j97 = 97;
         c -= cd;
         if (c < 0.0)
            c += cm;
         uni -= c;
         if (uni < 0.0)
            uni++;

         return(uni);
      }

      double getRandomGaussian(double mean,double stddev)
      {
         double  q,fu,v,x,y;

         /*  
            Generate P = (fu,v) uniform in rect. enclosing acceptance region 
            Make sure that any random numbers <= 0 are rejected, since
            gaussian() requires uniforms > 0, but RandomUniform() delivers >= 0.
         */
         do {
            fu = getRandomUniform();
            v = getRandomUniform();
            if (fu <= 0.0 || v <= 0.0) {
                fu = 1.0;
                v = 1.0;
            }
            v = 1.7156 * (v - 0.5);

            /*  Evaluate the quadratic form */
            x = fu - 0.449871;
            y = fabs(v) + 0.386595;
            q = x * x + y * (0.19600 * y - 0.25472 * x);

            /* Accept P if inside inner ellipse */
            if (q < 0.27597)
               break;

            /*  Reject P if outside outer ellipse, or outside acceptance region */
          } while ((q > 0.27846) || (v * v > -4.0 * log(fu) * fu * fu));

          /*  Return ratio of P's coordinates as the normal deviate */
          return (mean + stddev * v / fu);
      }

      long getRandom(void) { return getRandomLong(0, 32767); }
      long getRandomLong(long lower, long upper)
      {
         return((long)(getRandomUniform() * (upper - lower + 1)) + lower);
      }

      double getRandomDouble(double lower, double upper)
      {
         return((upper - lower) * getRandomUniform() + lower);
      }

      void                    setSeed(long seed) {seed;}
      long                    getSeed() const {return 0;}

      bool                    getSync(void) const {return(mSync);}
      void                    setSync(bool sync) {mSync=sync;}

      bool                 mSync;      
}; // BRandom2

//lint +e1927


//==============================================================================
// BRandom3
//
// jce 7/18/2000 -- this simple one is ripped directly from the rand() CRT source code,
// but encapsulated into a class so that there can be multiple instances.  Returns
// random number between 0 and 32767.
//==============================================================================
class BRandom3
{
   public:
                              BRandom3(void) : mSavedCurrent(1), mCurrent(1), mSync(false) {}
      long                    getSeed(void) const { return mCurrent; }
      void                    setSeed(long seed) {mCurrent=seed;}
      long                    getRandom(void) 
      {
         //mCurrent=((mCurrent*214013L+2531011L) >> 16) & 0x7fff; 
         return(((mCurrent = mCurrent * 214013L + 2531011L) >> 16) & 0x7fff);
         //return(mCurrent);
      }
      DWORD                   getRandomDWORD();
      long                    getMaxRandom(void) const {return(32767);}

      bool                    getSync(void) const {return(mSync);}
      void                    setSync(bool sync) {mSync=sync;}   
      void                    saveSeed(void) { mSavedCurrent = mCurrent; }
      void                    restoreSeed(void) { mCurrent = mSavedCurrent; }

   protected:
      long                    mCurrent;
      bool                    mSync;
      long                    mSavedCurrent;
};


typedef void (*RAND_SYNC_PROC)(const char *str, long data, const char *file, long line);

//==============================================================================
class BRandomManager
{
   public:
      enum { cLongMax = 1073741824 };
      BRandomManager(void);
      virtual ~BRandomManager(void);
            
      void                       _setSeed(long tag, long seed);
      long                       _getSeed(long tag) const;
      long                       _getRand(long tag);
      long                       _getRand(long tag, long max);
      DWORD                      _getRand(long tag, DWORD max);
      float                      _getRandDistribution(long tag);
      float                      _getRandRangeFloat(long tag, float minVal, float maxVal);
      float                      _getRandGaussian(long tag, float center, float offset);
      long                       _getRandRange(long tag, long minVal, long maxVal);
      DWORD                      _getRandRange(long tag, DWORD minVal, DWORD maxVal);
      void                       _setGlobalSeed(long seed);

      void                       _setSeedNS(long tag, long seed);
      long                       _getRandNS(long tag);
      long                       _getRandNS(long tag, long max);
      float                      _getRandDistributionNS(long tag);
      float                      _getRandRangeFloatNS(long tag, float minVal, float maxVal);
      float                      _getRandGaussianNS(long tag, float center, float offset);
      long                       _getRandRangeNS(long tag, long minVal, long maxVal);
      DWORD                      _getRandRangeDWORDNS(int tag, DWORD minVal, DWORD maxVal);

      // returns a value between baseVal - delta and baseVal + delta (inclusive)
      float                      getRandomOffset(long tag, float baseVal, float delta);

      void                       testRandom(void);

      void                       syncAll(void);       // meant for final release syncing
      bool                       getSync(long tag) const;

      void                       setSyncFunction(RAND_SYNC_PROC fn) {mSyncFunction=fn;}
      void                       setFinalReleaseSyncFunction(RAND_SYNC_PROC fn) {mFinalReleaseSyncFunction=fn;}

   protected:
      BDynamicSimArray<BRandom3> mRandomGenerator;

      // Callbacks to support syncing if desired.
      RAND_SYNC_PROC             mSyncFunction;
      RAND_SYNC_PROC             mFinalReleaseSyncFunction;
};

extern BRandomManager gRandomManager;

#endif

//==============================================================================
// eof: random.h
//==============================================================================
