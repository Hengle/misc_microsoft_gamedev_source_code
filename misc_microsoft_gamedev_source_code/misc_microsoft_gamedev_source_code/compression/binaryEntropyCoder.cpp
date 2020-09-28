#include "compression.h"
#include "xcore.h"
#include "rangecoder.h"
#include "containers\dynamicarray.h"
#include "binaryentropycoder.h"

BBitContext::BBitContext()
{
   reset();
}
      
void BBitContext::update(uint symbol)
{
   mTotalSymbols++;
   if (0 == symbol)
      mNumZeroSymbols++;

   if (mTotalSymbols == 65535)
      rescale();            
}
      
void BBitContext::reset(void)
{
   mTotalSymbols = 2;
   mNumZeroSymbols = 1;
}
      
void BBitContext::rescale(void)
{
   mTotalSymbols >>= 1;
   if (mTotalSymbols < 2)
      mTotalSymbols = 2;
      
   mNumZeroSymbols >>= 1;
   if (mNumZeroSymbols < 1)
      mNumZeroSymbols = 1;

   if (mNumZeroSymbols == mTotalSymbols)      
      mTotalSymbols++;
}
      
uint BBitContext::getTotalSymbols(void) const
{
   return mTotalSymbols;
}
      
uint BBitContext::getNumZeroSymbols(void) const
{
   return mNumZeroSymbols;
}

BBitContexts::BBitContexts(uint numContexts) :
   mContexts(numContexts)
{
}

void BBitContexts::reset(void)
{
   for (uint i = 0; i < mContexts.size(); i++)
      mContexts[i].reset();
}         
            
BBinaryEntropyCoder::BBinaryEntropyCoder() :
   mRangeCoder(BRangerCoderStreamTraits(&mCodedBits))
{
}
   
void BBinaryEntropyCoder::encodeStart(void)
{
   mRangeCoder.encodeStart();
}
         
void BBinaryEntropyCoder::encodeBit(BBitContext& context, uint bit)
{
   assert(bit < 2);
   
   if (bit)
      mRangeCoder.encode(context.getNumZeroSymbols(), context.getTotalSymbols() - context.getNumZeroSymbols(), context.getTotalSymbols());
   else
      mRangeCoder.encode(0, context.getNumZeroSymbols(), context.getTotalSymbols());
      
   context.update(bit);         
}

void BBinaryEntropyCoder::encodeBit(uint bit)
{
   assert(bit < 2);

   mRangeCoder.encode(bit, 1, 2);
}

void BBinaryEntropyCoder::encodeCode(uint code, uint codeSize)
{
   assert(code < (1U << codeSize));
   assert((codeSize >= 1) && (codeSize <= 16));

   mRangeCoder.encode(code, 1, 1 << codeSize);
}
   
void BBinaryEntropyCoder::encodeEnd(void)
{
   mRangeCoder.encodeEnd();
}

void BBinaryEntropyCoder::decodeStart(uint ofs)
{
   mRangeCoder.getTraits().setOfs(ofs);
   mRangeCoder.decodeStart();
}

uint BBinaryEntropyCoder::decodeBit(BBitContext& context)
{
   uint freq = mRangeCoder.getFreq(context.getTotalSymbols());
   
   uint result = 0;
   if (freq >= context.getNumZeroSymbols())
   {
      result = 1;
      
      mRangeCoder.decode(context.getNumZeroSymbols(), context.getTotalSymbols() - context.getNumZeroSymbols(), context.getTotalSymbols());
   }
   else
   {
      mRangeCoder.decode(0, context.getNumZeroSymbols(), context.getTotalSymbols());
   }
      
   context.update(result);
   
   return result;      
}

uint BBinaryEntropyCoder::decodeBit(void)
{
   uint freq = mRangeCoder.getFreq(2);

   uint result = 0;
   if (freq >= 1)
      result = 1;

   mRangeCoder.decode(result, 1, 2);
   
   return result;      
}

uint BBinaryEntropyCoder::decodeCode(uint codeSize)
{
   assert((codeSize >= 1) && (codeSize <= 16));
   uint freq = mRangeCoder.getFreq(1 << codeSize);
   
   mRangeCoder.decode(freq, 1, 1 << codeSize);

   return freq;      
}
   
void BBinaryEntropyCoder::decodeEnd(void)
{
}

void BBinaryEntropyCoder::setBuf(BByteArray* pBits)
{
   if (pBits)
      mRangeCoder.getTraits().setBuf(pBits);
   else
      mRangeCoder.getTraits().setBuf(&mCodedBits);
}

#if 0
{
   BBitContexts contexts(256);
   BBinaryEntropyCoder coder;

   coder.encodeStart();
   srand(200);
   for (uint l = 0; l < 12265536; l++)
   {
      uint sym = rand() & 1;
      uint context = rand() & (contexts.numContexts()-1);
      coder.encodeBit(contexts[context], sym);
   }
   coder.encodeEnd();   

   contexts.reset();

   coder.decodeStart();
   srand(200);
   for (uint l = 0; l < 12265536; l++)
   {
      uint sym = rand() & 1;
      uint context = rand() & (contexts.numContexts()-1);
      uint decodedSym = coder.decodeBit(contexts[context]);
      assert(sym == decodedSym);
   }
   coder.decodeEnd();
}
#endif