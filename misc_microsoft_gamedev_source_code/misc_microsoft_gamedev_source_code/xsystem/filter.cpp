//==============================================================================
// filter.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "filter.h"

//==============================================================================
// Defines


//==============================================================================
// BFilter::BFilter
//==============================================================================
BFilter::BFilter(void) :
   mWeights(NULL),
   mSizeX(0),
   mSizeZ(0),
   mRecipTotalWeight(0.0f)
{
} // BFilter::BFilter


//==============================================================================
// BFilter::BFilter
//==============================================================================
BFilter::BFilter(float w0, float w1, float w2, float w3, float w4, float w5, float w6, float w7, float w8)
{
   // Initialize to 3x3.
   bool ok=init(3, 3);
   if(!ok)
   {
      BASSERT(0);
      return;
   }

   mWeights[0]=w0;
   mWeights[1]=w1;
   mWeights[2]=w2;
   mWeights[3]=w3;
   mWeights[4]=w4;
   mWeights[5]=w5;
   mWeights[6]=w6;
   mWeights[7]=w7;
   mWeights[8]=w8;

   computeTotalWeight();
}


//==============================================================================
// BFilter::BFilter
//==============================================================================
BFilter::BFilter(float w0, float w1, float w2, float w3, float w4, 
                 float w5, float w6, float w7, float w8, float w9,
                 float w10, float w11, float w12, float w13, float w14,
                 float w15, float w16, float w17, float w18, float w19,
                 float w20, float w21, float w22, float w23, float w24)
{
   // Initialize to 5x5.
   bool ok=init(5, 5);
   if(!ok)
   {
      BASSERT(0);
      return;
   }

   // Yes, this IS a fine function.
   mWeights[0]=w0;
   mWeights[1]=w1;
   mWeights[2]=w2;
   mWeights[3]=w3;
   mWeights[4]=w4;
   mWeights[5]=w5;
   mWeights[6]=w6;
   mWeights[7]=w7;
   mWeights[8]=w8;
   mWeights[9]=w9;
   mWeights[10]=w10;
   mWeights[11]=w11;
   mWeights[12]=w12;
   mWeights[13]=w13;
   mWeights[14]=w14;
   mWeights[15]=w15;
   mWeights[16]=w16;
   mWeights[17]=w17;
   mWeights[18]=w18;
   mWeights[19]=w19;
   mWeights[20]=w20;
   mWeights[21]=w21;
   mWeights[22]=w22;
   mWeights[23]=w23;
   mWeights[24]=w24;

   computeTotalWeight();
}


//==============================================================================
// BFilter::~BFilter
//==============================================================================
BFilter::~BFilter(void)
{
   clear();
}


//==============================================================================
// BFilter::init
//==============================================================================
bool BFilter::init(long sizeX, long sizeZ)
{
   // Clear out anything existing.
   clear();

   // Check for bad size.
   if(sizeX<=0 || sizeZ<=0)
   {
      BASSERT(0);
      return(false);
   }

   // Init sizes.
   mSizeX=sizeX;
   mSizeZ=sizeZ;

   // Allocate.
   long numEntries=mSizeX*mSizeZ;
   mWeights=new float[numEntries];
   if(!mWeights)
      return(false);

   // Initialize for good measure.
   for(long i=numEntries-1; i>=0; i--)
      mWeights[i]=0.0f;

   return(true);
}


//==============================================================================
// BFilter::setWeight
//==============================================================================
void BFilter::setWeight(long x, long z, float weight)
{
   // Check for bad coordinates.
   if(x<0 || x>=mSizeX || z<0 || z>=mSizeZ)
   {
      BASSERT(0);
      return;
   }

   // Set it.
   mWeights[x*mSizeZ+z]=weight;
   computeTotalWeight();
}


//==============================================================================
// BFilter::getWeight
//==============================================================================
float BFilter::getWeight(long x, long z) const
{
   // Check for bad coordinates.
   if(x<0 || x>=mSizeX || z<0 || z>=mSizeZ)
   {
      BASSERT(0);
      return(0.0f);
   }

   // Return it.
   return(mWeights[x*mSizeZ+z]);
}


//==============================================================================
// BFilter::clear
//==============================================================================
void BFilter::clear(void)
{
   // Nuke weights.
   if(mWeights)
   {  
      delete []mWeights;
      mWeights=NULL;
   }
   mSizeX=0;
   mSizeZ=0;
   mRecipTotalWeight=0.0f;
}


//==============================================================================
// BFilter::computeTotalWeight
//==============================================================================
void BFilter::computeTotalWeight(void)
{
   // Need weights before this makes sense.
   if(!mWeights)
   {
      BASSERT(0);
      mRecipTotalWeight=0.0f;
      return;
   }

   // Add up all the weights.
   float total=0.0f;
   for(long i= (mSizeX*mSizeZ) - 1; i>=0; i--)
      total+=mWeights[i];

   // Save off reciprocal.
   mRecipTotalWeight=1.0f/total;
}


//==============================================================================
// eof: filter.cpp
//==============================================================================
