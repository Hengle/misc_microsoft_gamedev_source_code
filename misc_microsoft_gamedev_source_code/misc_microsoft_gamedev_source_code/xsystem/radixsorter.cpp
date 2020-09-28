//==============================================================================
// radixsorter.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "RadixSorter.h"
//#include "memory.h"

#define BYTE unsigned char

//==============================================================================
// Construction/Destruction
//==============================================================================

BRadixSorter::BRadixSorter(BMemoryHeap* pHeap) : 
   mpHeap(pHeap),
   mIndices(NULL), 
   mIndices2(NULL), 
   mSize(0)
{

}

BRadixSorter::~BRadixSorter()
{
	HEAP_DELETE_ARRAY(mIndices, *mpHeap);
	HEAP_DELETE_ARRAY(mIndices2, *mpHeap);
}


//==============================================================================
// MACROS
//
// CHECK_PASS_VALIDITY (checks to seee if all values for a given bucket has the
// same byte entry)  If so, then we can skip that byte because it will not
// affect the order. 
//==============================================================================
#define CHECK_PASS_VALIDITY(pass, ofs)													    \
	const unsigned long* count = &mHistogram[pass<<8];												\
	bool doPass = true;																			\
	const BYTE unique = *(((BYTE*)input)+ofs);														\
	if(count[unique]== num) doPass=false;														\


//==============================================================================
// BRadixSorter::resize
//==============================================================================
void BRadixSorter::resize( long num )
{
   HEAP_DELETE_ARRAY(mIndices, *mpHeap);
   HEAP_DELETE_ARRAY(mIndices2, *mpHeap);
   
	mIndices = HEAP_NEW_ARRAY(unsigned long, num, *mpHeap);
	mIndices2 = HEAP_NEW_ARRAY(unsigned long, num, *mpHeap);
	mSize = num;

	resetIndices();
}

//==============================================================================
// BRadixSorter::freeIndices
//==============================================================================
void BRadixSorter::freeIndices(void)
{
   HEAP_DELETE_ARRAY(mIndices, *mpHeap);
   HEAP_DELETE_ARRAY(mIndices2, *mpHeap);
   
   mIndices = NULL;
   mIndices2 = NULL;
   
   mSize = 0;
}


//==============================================================================
// BRadixSorter::resetIndices
//==============================================================================
void BRadixSorter::resetIndices( void )
{
	for(unsigned long i=0;i<mSize;i++)	
		mIndices[i] = i;
}


//==============================================================================
// BRadixSorter::createHistograms
// -- size MUST be set before you call this
//==============================================================================
bool BRadixSorter::createHistograms( const float *buffer, long num)
{
   BASSERT(mIndices);

	//-- clear previous histogram																		
	memset(mHistogram,0, 256*4*sizeof(long));												
																								
	//-- store off values for Temporal Coherence													
	float prev = (float)buffer[mIndices[0]];													
	bool bSorted = true;
	const unsigned long *indices = mIndices;
																								
	//-- count	
	//-- cast the float away
	const BYTE* p = (BYTE*)buffer;	
	//-- get the last pointer in the aray 
	const BYTE* pe = &p[num*4];																

#ifdef XBOX
   unsigned long* h3= &mHistogram[0];	//-- first pass			
   unsigned long* h2= &mHistogram[256];	//-- second pass				
   unsigned long* h1= &mHistogram[512];	//-- third pass			
   unsigned long* h0= &mHistogram[768];	//-- fourth pass 				
#else	
	unsigned long* h0= &mHistogram[0];	//-- first pass			
	unsigned long* h1= &mHistogram[256];	//-- second pass				
	unsigned long* h2= &mHistogram[512];	//-- third pass			
	unsigned long* h3= &mHistogram[768];	//-- fourth pass 				
#endif	
																								
	while(p!=pe)																				
	{																							
		//-- read input buffer									
		float val = (float)buffer[*indices++];													
		//-- if this is true then we  need sorting										
		if(val<prev)	
		{ 
			bSorted = false; 
			break; 
		} 					
		
		prev = val;																			
																								
		//-- create histograms																
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;											
	}

																								
	//-- if we get here then 1 of 2 things has happened.  We need to sort,
	//-- or we don't.  If we don't just get out.  The histograms will be
	//-- in the same order as what we were given. Return true to signal
	//-- already sorted.
	if(bSorted)
		return (true);
																								
	//-- compute histograms (for real this time)			
	while(p!=pe)																				
	{																							
		//-- easy to create them now (cause we know we need to sort)								
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;	
	}

	//-- we need to sort
	return (false);

}


//==============================================================================
// BRadixSorter::Sort
//==============================================================================
BRadixSorter& BRadixSorter::sort(const float* values, unsigned long num)
{
	// Checkings
	if(!values || !num)	return *this;


	unsigned long* input = (unsigned long*)values;

	if(num > mSize)	
		resize(num);												
	else						
		resetIndices();											

	//-- figure out our histograms and use these to control the passes
	if (createHistograms(values, num) == true)
      return (*this);

	//-- number of negatives
	long nNegatives = 0;
	
	//-- nice optimization
	// An efficient way to compute the number of negatives values we'll have to deal with is simply to sum the 128
	// last values of the last histogram. Last histogram because that's the one for the Most Significant Byte,
	// responsible for the sign. 128 last values because the 128 first ones are related to positive numbers.
//-- FIXING PREFIX BUG ID 469
	const unsigned long* h3= &mHistogram[768];
//--
	for(unsigned long i=128;i<256;i++)	nNegatives += h3[i];	// 768 for last histogram, 128 for negative part

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(unsigned long j=0;j<4;j++)
	{
#ifdef XBOX
      const unsigned long ofs = 3 - j;
#else	
	   const unsigned long ofs = j;
#endif	   
	   
		// Here we deal with positive values only
	    CHECK_PASS_VALIDITY(j, ofs);

		// Should we care about negative values?
		if(j != 3)
		{
         if(doPass)
			{
				// Create offsets
				
            unsigned long curOfs = 0;
            for(long i=0;i<256;i++)		
            {
               mOffset[i] = curOfs;
               curOfs += count[i];
            }

				// Perform Radix Sort
				const BYTE* const inputBytes	= (BYTE*)input + ofs;
//-- FIXING PREFIX BUG ID 462
				const unsigned long* indices		= mIndices;
//--
								
            unsigned long num4 = num >> 2;
            
            for (uint i = num4; i > 0; i--)
				{
               const long id0 = indices[0];
               const long id1 = indices[1];
               const long id2 = indices[2];
               const long id3 = indices[3];
               indices += 4;

               const uint c0 = inputBytes[id0 << 2];
               const uint c1 = inputBytes[id1 << 2];
               const uint c2 = inputBytes[id2 << 2];
               const uint c3 = inputBytes[id3 << 2];

               if ((c0 == c1) && (c1 == c2) && (c2 == c3))
               {
                  unsigned long destOfs = mOffset[c0];
                  mOffset[c0] += 4;

                  mIndices2[destOfs + 0] = id0;
                  mIndices2[destOfs + 1] = id1;
                  mIndices2[destOfs + 2] = id2;
                  mIndices2[destOfs + 3] = id3;
               }
               else
               {
					   mIndices2[mOffset[c0]++] = id0;
					   mIndices2[mOffset[c1]++] = id1;
					   mIndices2[mOffset[c2]++] = id2;
					   mIndices2[mOffset[c3]++] = id3;
               }
				}

            for (uint i = num & 3; i > 0; i--)
            {
					long id = *indices++;
               uint c = inputBytes[id << 2];
               unsigned long destOfs = mOffset[c]++;
					mIndices2[destOfs] = id;
            }

            std::swap(mIndices, mIndices2);
			}
		}
		else
		{
			if(doPass)
			{
				// Create biased offsets, in order for negative numbers to be sorted as well
            unsigned long curOfs = nNegatives;
            for(long i=0;i<256;i++)		
            {
               mOffset[i] = curOfs;
               curOfs += count[i];
            }

				// We must reverse the sorting order for negative numbers!
            curOfs = 0;
            for (uint i = 255; i >= 128; i--)
            {
               mOffset[i] = curOfs;
               curOfs += count[i];
            }
				
            for(unsigned long i=128;i<256;i++)	
               mOffset[i] += count[i];							// Fixing the wrong place for negative values*/

            const BYTE* const inputBytes = (BYTE*)input + ofs;
//-- FIXING PREFIX BUG ID 465
            const unsigned long* indices = mIndices;
//--
				
            unsigned long num4 = num >> 2;
            
            for (uint i = num4; i > 0; i--)
				{
               const long id0 = indices[0];
               const long id1 = indices[1];
               const long id2 = indices[2];
               const long id3 = indices[3];
               indices += 4;
               
               
               const uint c0 = inputBytes[id0 << 2];
               const uint c1 = inputBytes[id1 << 2];
               const uint c2 = inputBytes[id2 << 2];
               const uint c3 = inputBytes[id3 << 2];
               
               if ((c0 == c1) && (c1 == c2) && (c2 == c3))
               {
                  unsigned long destOfs = mOffset[c0];
                  if (c0 < 128)
                  {
                     mOffset[c0] += 4;
                     mIndices2[destOfs + 0] = id0;
                     mIndices2[destOfs + 1] = id1;
                     mIndices2[destOfs + 2] = id2;
                     mIndices2[destOfs + 3] = id3;
                  }
                  else
                  {
                     mOffset[c0] -= 4;
                     mIndices2[destOfs - 1] = id0;
                     mIndices2[destOfs - 2] = id1;
                     mIndices2[destOfs - 3] = id2;
                     mIndices2[destOfs - 4] = id3;
                  }                        
               }
               else
               {  
                  if (c0 < 128)
					      mIndices2[mOffset[c0]++] = id0;
                  else
                     mIndices2[--mOffset[c0]] = id0;

                  if (c1 < 128)
					      mIndices2[mOffset[c1]++] = id1;
                  else
                     mIndices2[--mOffset[c1]] = id1;
                  
                  if (c2 < 128)
					      mIndices2[mOffset[c2]++] = id2;
                  else
                     mIndices2[--mOffset[c2]] = id2;
                  
                  if (c3 < 128)
					      mIndices2[mOffset[c3]++] = id3;
                  else
                     mIndices2[--mOffset[c3]] = id3;
               }
				}

            for (uint i = num & 3; i > 0; i--)
            {
					long id = *indices++;

               uint c = inputBytes[id << 2];

               unsigned long destOfs;
               if (c < 128)
                  destOfs = mOffset[c]++;
               else
                  destOfs = --mOffset[c];
                  
               mIndices2[destOfs] = id;
            }
				
            std::swap(mIndices, mIndices2);
			}
			else
			{
				// The pass is useless, yet we still have to reverse the order of current list if all values are negative.
				if(unique>=128)
				{
					for(unsigned long i=0;i<num;i++)	
                  mIndices2[i] = mIndices[num-i-1];
					
               std::swap(mIndices, mIndices2);
				}
			}
		}
	}

#if 0
   for (uint i = 1; i < num; i++)
   {
      float prevValue = values[mIndices[i - 1]];
      float curValue = values[mIndices[i]];
      BDEBUG_ASSERT(curValue >= prevValue);
   }
#endif

	return *this;
}

