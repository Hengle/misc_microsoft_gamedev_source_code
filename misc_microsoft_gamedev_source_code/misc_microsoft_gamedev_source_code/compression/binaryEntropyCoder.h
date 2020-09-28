// File: binaryentropycoder.h
#pragma once

template<uint NumSymbols>
class BSymbolContext
{
public:
   BSymbolContext()
   {
      reset();
   }

   uint numSymbols(void) const { return NumSymbols; }

   void update(uint symbol)
   {
      assert(symbol < NumSymbols);
      mTotalSymbols++;
      mFreq[symbol]++;

      if (mTotalSymbols == 65535)
         rescale();            
   }

   void reset(void)
   {
      mTotalSymbols = NumSymbols;
      std::fill(mFreq, mFreq + NumSymbols, 1);
   }

   void rescale(void)
   {
      uint total = 0;
      do 
      {
         for (uint i = 0; i < NumSymbols; i++)
         {
            mFreq[i] = Math::Max<ushort>(1U, mFreq[i] >> 1U);
            total += mFreq[i];
         }
      } while (total > 65535);

      mTotalSymbols = static_cast<ushort>(total);
   }

   uint getTotalSymbols(void) const
   {
      return mTotalSymbols;
   }

   uint getFreq(uint symbol) const
   {
      assert(symbol < NumSymbols);
      return mFreq[symbol];
   }

   uint getCumFreq(uint symbol) const
   {
      assert(symbol < NumSymbols);
      uint cumFreq = 0;
      for (int i = 0; i < static_cast<int>(symbol) - 1; i++)
         cumFreq += mFreq[i];
      return cumFreq;
   }

private:
   ushort mTotalSymbols;
   ushort mFreq[NumSymbols];
};

template<uint NumSymbols>
class BSymbolContexts
{
public:
   BSymbolContexts(uint numContexts) : mContexts(numContexts)
   {
      reset();
   }

   uint numContexts(void) const { return mContexts.size(); }
   uint numSymbols(void) const { return NumSymbols; }

   const BSymbolContext<NumSymbols>& operator[](uint context) const  { return mContexts[context]; }
         BSymbolContext<NumSymbols>& operator[](uint context)        { return mContexts[context]; }

   void reset(void)
   {
      for (uint i = 0; i < mContexts.size(); i++)
         mContexts[i].reset();
   }

private:
   BDynamicArray< BSymbolContext<NumSymbols> > mContexts;
};

class BBitContext
{
public:
   BBitContext();

   void update(uint symbol);
   void reset(void);
   void rescale(void);
   uint getTotalSymbols(void) const;
   uint getNumZeroSymbols(void) const;

private:
   ushort mTotalSymbols;
   ushort mNumZeroSymbols;
};

class BBitContexts
{
public:
   BBitContexts(uint numContexts);
         
   uint numContexts(void) const { return mContexts.size(); }
      
   const BBitContext& operator[](uint context) const  { return mContexts[context]; }
         BBitContext& operator[](uint context)        { return mContexts[context]; }

   void reset(void);
   
private:
   BDynamicArray<BBitContext> mContexts;
};

class BBinaryEntropyCoder
{
public:
   BBinaryEntropyCoder();

   void encodeStart(void);
   void encodeBit(BBitContext& context, uint bit);
   void encodeBit(uint bit);
   void encodeCode(uint code, uint codeSize);
      
   template<typename T>
   void encodeSymbol(T& context, uint symbol)
   {
      mRangeCoder.encode(context.getCumFreq(symbol), context.getFreq(symbol), context.getTotalSymbols());
      context.update(symbol);         
   }
   
   void encodeEnd(void);
   
   void decodeStart(uint ofs = 0);
   uint decodeBit(BBitContext& context);
   uint decodeCode(uint codeSize);
   uint decodeBit(void);
      
   template<typename T>
   uint decodeSymbol(T& context)
   {
      const uint decodedFreq = mRangeCoder.getFreq(context.getTotalSymbols());

      uint sym = 0, cumFreq = 0;
      for ( ; sym < NumSymbols; sym++)
      {
         const uint symFreq = context.getFreq(sym);

         if (decodedFreq < (cumFreq + symFreq))
            break;

         cumFreq += symFreq;
      }

      mRangeCoder.decode(cumFreq, context.getFreq(sym), context.getTotalSymbols());  

      context.update(sym);         

      return sym;
   }
   
   void decodeEnd(void);
   
   void setBuf(BByteArray* pBits);
      
   BByteArray* getBuf(void) { return mRangeCoder.getTraits().getBuf(); }

private:
   BByteArray mCodedBits;
   
   class BRangerCoderStreamTraits
   {
   public:
      BRangerCoderStreamTraits(BByteArray* pBuf) : 
         mpBuf(pBuf)
      {
      }

      bool outputByte(uchar c)
      {
         mpBuf->pushBack(c);
         return true;
      }

      int inputByte(void)
      {
         if (mCurOfs >= mpBuf->size())
            return 0;
         return (*mpBuf)[mCurOfs++];
      }
      
      void setBuf(BByteArray* pBuf)
      {
         mpBuf = pBuf;
      }
      
      BByteArray* getBuf(void) const
      {
         return mpBuf;
      }
      
      void setOfs(uint ofs) 
      {
         mCurOfs = ofs;
      }
      
      uint getOfs(void)
      {
         return mCurOfs;
      }

   private:
      BByteArray* mpBuf;
      uint mCurOfs;
   };

   BRangeCoder<BRangerCoderStreamTraits> mRangeCoder;
};

