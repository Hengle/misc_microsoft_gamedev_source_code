//==============================================================================
// random.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "random.h"

// Included for ASSERT_THREAD.
#include "threading\eventDispatcher.h"

//==============================================================================
// Defines

//==============================================================================
// Return random DWORD
//==============================================================================
DWORD BRandom3::getRandomDWORD() 
{ 
   DWORD result = 0;
   mCurrent = mCurrent * 214013L + 2531011L;
   if (mCurrent < 0)
   {
      result = (DWORD)((-mCurrent >> 16) & 0x7fff);
   }
   else
   {
      result = (DWORD)((mCurrent >> 16) & 0x7fff);
   }
   return (result);
}

//==============================================================================
// BRandomManager::BRandomManager
//==============================================================================
BRandomManager::BRandomManager(void) :
   mSyncFunction(NULL),
   mFinalReleaseSyncFunction(NULL)
{   
   mRandomGenerator.setNumber(cRandAmount);

   mRandomGenerator[cSoundRand].setSync(false);
   mRandomGenerator[cUIRand].setSync(false);
   mRandomGenerator[cSimRand].setSync(true);
   mRandomGenerator[cRMRand].setSync(true);
   mRandomGenerator[cUnsyncedRand].setSync(false);
   mRandomGenerator[cMixRand].setSync(true);
   mRandomGenerator[cEditorRand].setSync(false);
   mRandomGenerator[cAIRand].setSync(true);
}

//==============================================================================
// BRandomManager::~BRandomManager
//==============================================================================
BRandomManager::~BRandomManager(void)
{
} // BRandomManager::~BRandomManager

//==============================================================================
// BRandomManager::_setSeed
//==============================================================================
void BRandomManager::_setSeed(long tag, long seed)
{ 
   ASSERT_THREAD(cThreadIndexSim);
   
   //long seed1, seed2;
   //seed1 = seed % 31328;
   //seed2 = (seed << 8) % 30081;
   //mRandomGenerator[tag].initialize(seed1, seed2);
   mRandomGenerator[tag].setSeed(seed);
}

//==============================================================================
// BRandomManager::_getSeed
//==============================================================================
long BRandomManager::_getSeed(long tag) const
{ 
   ASSERT_THREAD(cThreadIndexSim);
   return mRandomGenerator[tag].getSeed();
}

//==============================================================================
// BRandomManager::_setGlobalSeed
//==============================================================================
void BRandomManager::_setGlobalSeed(long seed)
{ 
   ASSERT_THREAD(cThreadIndexSim);
   for (long i=0;i<cRandAmount;i++)
      _setSeed(i, seed);
}

//==============================================================================
// BRandomManager::_getRand
//==============================================================================
long BRandomManager::_getRand(long tag)
{
   ASSERT_THREAD(cThreadIndexSim);
   static long r;
   //r = mRandomGenerator[tag].getRandomLong(0, cLongMax); 
   r=mRandomGenerator[tag].getRandom();
   return r;
}

//==============================================================================
// BRandomManager::_getRand
//==============================================================================
long BRandomManager::_getRand(long tag, long max)
{
   ASSERT_THREAD(cThreadIndexSim);
   if(max<=0)
   {
      // jce [7/24/2002] -- Passing a negative number in here is probably bad news... so if we're <=0 we better be ==0
      BASSERT(max==0);

      return(0);
   }

   long r;
   //r = mRandomGenerator[tag].getRandomLong(0, max); 
   //r=mRandomGenerator[tag].getRandom()%(max+1);
   
   // jce [2/15/2008] -- this way is much better than % since low order bits have less randomness
   r=mRandomGenerator[tag].getRandom()/(32767/(max+1) + 1);
   
   // jce [4/21/2008] -- supposedly even better way of doing this from http://www.azillionmonkeys.com/qed/random.html
   // jce [5/12/2008] -- BLOWS UP IF MAX>MAXRANDOM, needs to be fixed first
   /*
   max++;   // max for how this is built is non-inclusive but our max is inclusive, so increment by one here
   long rInvRange = (long)((mRandomGenerator[tag].getMaxRandom()+1)/max);
   do 
   {
      r = mRandomGenerator[tag].getRandom();
   } while (r >= max * rInvRange);
   r /= rInvRange;
   */

   return r;
}

//==============================================================================
// BRandomManager::_getRand
//==============================================================================
DWORD BRandomManager::_getRand(long tag, DWORD max)
{
   ASSERT_THREAD(cThreadIndexSim);

   long r = 0;
   //long inMax = (long)max;
   //r = mRandomGenerator[tag].getRandomDWORD() % (max + 1);

   // jce [2/15/2008] -- this way is much better than % since low order bits have less randomness
   r=mRandomGenerator[tag].getRandom()/(32767/(max+1) + 1);

   // jce [4/21/2008] -- better way of doing this as explained at: http://www.azillionmonkeys.com/qed/random.html
   /*
   inMax++;   // max for how this is built is non-inclusive but our max is inclusive, so increment by one here   
   long randMax = mRandomGenerator[tag].getMaxRandom();
   // Halwes - 6/12/2008 - Clamp so that we don't divide by 0.
   inMax = Math::Min(inMax, randMax);
   long rInvRange = (randMax + 1) / inMax;
   do 
   {
      r = mRandomGenerator[tag].getRandom();
   } while (r >= (inMax * rInvRange));
   r /= rInvRange;
   */

   return ((DWORD)r);
}

//==============================================================================
// BRandomManager::_getRandDistribution
//==============================================================================
float BRandomManager::_getRandDistribution(long tag)
{
   static float cOneOver32k = 1.0f / 32767.0f;
   long randBase = _getRand(tag) % 32768;
   float retval = static_cast<float>(randBase) * cOneOver32k;
   return(retval);
}

//==============================================================================
// BRandomManager::_getRandGaussian
//==============================================================================
float BRandomManager::_getRandGaussian(long tag, float mean, float stddev)
{
   double  q,fu,v,x,y;

   /*  
      Generate P = (fu,v) uniform in rect. enclosing acceptance region 
      Make sure that any random numbers <= 0 are rejected, since
      gaussian() requires uniforms > 0, but RandomUniform() delivers >= 0.
   */
   do {
      fu = _getRandRangeFloat(tag, 0.0, 1.0);
      v = _getRandRangeFloat(tag, 0.0, 1.0);
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
    return (float)(mean + stddev * v / fu);
}

//==============================================================================
// BRandomManager::_getRandRange
//==============================================================================
long BRandomManager::_getRandRange(long tag, long minVal, long maxVal)
{
   //long delta=maxVal-minVal;
   //return(minVal+_getRand(tag, delta, file, line));
   return(minVal+_getRand(tag, maxVal-minVal));
}

//==============================================================================
// BRandomManager::_getRandRange
//==============================================================================
DWORD BRandomManager::_getRandRange(long tag, DWORD minVal, DWORD maxVal)
{
   return (minVal + _getRand(tag, maxVal - minVal));
}

//==============================================================================
// BRandomManager::_getRandRangeFloat
//==============================================================================
float BRandomManager::_getRandRangeFloat(long tag, float minVal, float maxVal)
{
   // ajl 5/15/02 - Changed this code to one line to fix issue where debug and release builds would
   // return slightly different values sometimes. This was happening while generating random map areas.
   // The difference appears to be caused by the compiler optimizer.
   //float dist = _getRandDistribution(tag, file, line);
   //float delta = maxVal - minVal;
   //float val = minVal+(delta * dist);
   //return(val);
   return(minVal + ((maxVal - minVal) * _getRandDistribution(tag)));
}

//==============================================================================
// BRandomManager::getRandomOffset
//==============================================================================
float BRandomManager::getRandomOffset(long tag, float baseVal, float delta)
{
    //float val;
   //val = baseVal - delta;
   //val = val + _getRandRangeFloat(tag, 0.0f, 2.0f * delta);
   //return(val);
   return(baseVal - delta + _getRandRangeFloat(tag, 0.0f, 2.0f * delta));
}

//==============================================================================
// BRandomManager::testRandom
//==============================================================================
void BRandomManager::testRandom(void)
{
   long n;
   long range =  32768; // 100;
   long samples = 1000;
   BDynamicSimArray<long> nums;
   nums.setNumber(range);
   for (n=0; n < range * samples; n++)
   {
      long num;
      num = getRand(cSimRand) % 32768;
      //num = rand() % 32768;
      //num = getRandomRange(cSimRand, 0, range);
      //num = (long)getRandomOffset(cSimRand, 50.0f, 50.0f);
      //num = (long)(getRandomDistribution(cSimRand) * 100.0f);
      nums[num] = nums[num] + 1;
   }
   for (n=0; n < nums.getNumber(); n++)
   {
      blog("Random Number %d came up %d times",n,nums[n]);
   }
}

//==============================================================================
// BRandomManager::syncAll
//==============================================================================
void BRandomManager::syncAll(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   for(long i=0; i<mRandomGenerator.getNumber(); i++)
   {
      if(mRandomGenerator[i].getSync() && mFinalReleaseSyncFunction)
      {
         long num=getRand(i);
         mFinalReleaseSyncFunction("SA", num, __FILE__, __LINE__);
      }
   }
}


//==============================================================================
// BRandomManager::
//==============================================================================
bool BRandomManager::getSync(long tag) const
{
   ASSERT_THREAD(cThreadIndexSim);
   return mRandomGenerator[tag].getSync();
}

//==============================================================================
// Non-Sync functions below. The assert isn't defined in the macro because it 
// makes the macro hard to use when you slap a ; in it.
void BRandomManager::_setSeedNS(long tag, long seed)
{
   //BFATAL_ASSERT(!getSync(tag));
   _setSeed(tag, seed);
}

long BRandomManager::_getRandNS(long tag)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRand(tag));
}

long BRandomManager::_getRandNS(long tag, long max)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRand(tag, max));
}

float BRandomManager::_getRandDistributionNS(long tag)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRandDistribution(tag));
}

float BRandomManager::_getRandRangeFloatNS(long tag, float minVal, float maxVal)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRandRangeFloat(tag, minVal, maxVal));
}

float BRandomManager::_getRandGaussianNS(long tag, float center, float offset)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRandGaussian(tag, center, offset));
}

long BRandomManager::_getRandRangeNS(long tag, long minVal, long maxVal)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRandRange(tag, minVal, maxVal));
}

DWORD BRandomManager::_getRandRangeDWORDNS(int tag, DWORD minVal, DWORD maxVal)
{
   //BFATAL_ASSERT(!getSync(tag));
   return(_getRandRange(tag, minVal, maxVal));
}

//==============================================================================
// BRandomManager::
//==============================================================================

//==============================================================================
// eof: random.cpp
//==============================================================================
