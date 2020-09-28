//==============================================================================
// bitarray.h
//
// Copyright (c) 1998-2007, Ensemble Studios
//==============================================================================

#pragma once

#ifndef _BITARRAY_H_
#define _BITARRAY_H_

//==============================================================================
// Forward declarations
class BChunkWriter;
class BChunkReader;

//==============================================================================
// Const declarations


//=============================================================================
class BBitArray
{
   public:
                           BBitArray() : mBits(NULL), mNumber(0), mNumberBytes(0) {}
                           BBitArray(const BBitArray& ba) : mBits(NULL), mNumber(0), mNumberBytes(0) 
                           {
                              // prep the general size
                              setNumber(ba.getNumber());
                              clear();

                              // make sure all the same bits are set
                              /*DWORD i;
                              for (i=0; i < mNumber; i++)
                              {
                                 if (isBitSet(i))
                                    setBit(i);
                              }*/                              
                              memcpy(mBits, ba.mBits, mNumberBytes);
                           }
                           BBitArray(const unsigned char* bitfield, long number) : mBits(NULL), mNumber(0), mNumberBytes(0) 
                           { reset(bitfield, number); }
                           
                           ~BBitArray(void);

      void                 reset(const unsigned char* bitfield, long number)
                           {
                              // prep the general size
                              setNumber(number);
                              clear();
                              // make sure all the same bits are set
                              DWORD i;
                              for (i=0; i < mNumber; i++)
                              {
                                 if (isBitSet(bitfield, i))
                                    setBit(i);
                              }                              
                           }
                           

      bool                 setNumber(long newNumber, bool force=false);
      long                 getNumber() const { return mNumber; }
      long                 getNumberBytes() const { return mNumberBytes; }

      void                 clear()
                           {
                              if(mBits)
                                 memset(mBits, 0x00, mNumberBytes);
                           }

      void                 setAll()
                           {
                              if(mBits)
                                 memset(mBits, 0xFF, mNumberBytes);
                           }

      void                 setBit(DWORD bitNumber)
                           {
                              #ifdef _DEBUG
                              if (bitNumber >= mNumber)
                              {
                                 BASSERT(0);
                                 return;
                              }
                              #endif
                              mBits[(bitNumber >> 3)] |= (1 << (bitNumber & 7));
                           }
                                 
      void                 clearBit(DWORD bitNumber)
                           {
                              #ifdef _DEBUG
                              if (bitNumber >= mNumber)
                              {
                                 BASSERT(0);
                                 return;
                              }
                              #endif

                              mBits[(bitNumber >> 3)] &= ~(1 << (bitNumber & 7));
                           }

      __forceinline DWORD  isBitSet(DWORD bitNumber) const
                           {
                              //DCP TODO 06/05/01: This check needs to always be on for now.
                              //#ifdef _DEBUG
                              if (bitNumber >= mNumber)
                              {
                                 BASSERT(0);
                                 return(0);
                              }
                              //#endif

                              return(mBits[(bitNumber >> 3)] & (1 << (bitNumber & 7)));
                           }
      DWORD                isBitSet(const unsigned char* bitfield, DWORD bitNumber) const
                           {
                              return(bitfield[(bitNumber >> 3)] & (1 << (bitNumber & 7)));
                           }

      bool                 checkAndSet(DWORD bitNumber)
                           {
                              #ifdef _DEBUG
                              if (bitNumber >= mNumber)
                              {
                                 BASSERT(0);
                                 return(0);
                              }
                              #endif

                              // If bit already set, return true.
                              if(mBits[(bitNumber >> 3)] & (1 << (bitNumber & 7)))
                                 return(true);
                              
                              // Otherwise set it and return false.
                              mBits[(bitNumber >> 3)] |= (1 << (bitNumber & 7));
                              return(false);
                           }
                           
      bool                 checkAndClear(DWORD bitNumber)
                           {
#ifdef _DEBUG
                              if (bitNumber >= mNumber)
                              {
                                 BASSERT(0);
                                 return(0);
                              }
#endif
                              const unsigned char mask = static_cast<unsigned char>(1 << (bitNumber & 7));
                              const DWORD byteOfs = bitNumber >> 3;
                              
                              const bool isSetFlag = (mBits[byteOfs] & mask) != 0;
                              
                              mBits[byteOfs] &= ~mask;
                              
                              return isSetFlag;
                           }
                           
      bool                 areAllBitsSet() const
                           {
                              if (!mBits)
                                 return false;

                              const uint bytesToCheck = mNumber >> 3;
                              for (uint i = 0; i < bytesToCheck; i++)
                                 if (mBits[i] != 0xFF)
                                    return false;

                              const uint bitsToCheck = mNumber & 7;
                              if (bitsToCheck)
                              {
                                 const uint mask = (1 << bitsToCheck) - 1;
                                 if ((mBits[bytesToCheck] & mask) != mask)
                                    return false;
                              }

                              return true;
                           }

      bool                 areAllBitsClear() const
                           {
                              if (!mBits)
                                 return false;

                              const uint bytesToCheck = mNumber >> 3;
                              for (uint i = 0; i < bytesToCheck; i++)
                                 if (mBits[i] != 0x00)
                                    return false;

                              const uint bitsToCheck = mNumber & 7;
                              if (bitsToCheck)
                              {
                                 const uint mask = (1 << bitsToCheck) - 1;
                                 if ((mBits[bytesToCheck] & mask) != 0)
                                    return false;
                              }

                              return true;
                           }

      void                 and( const BBitArray& a );
      void                 or ( const BBitArray& a );

      BBitArray&           operator=(const BBitArray &val)
                           {
                              if (this == &val)
                              {
                                 // if you are hitting this, you are trying to copy to yourself!!
                                 BASSERT(0);
                                 return(*this);
                              }
                              if (val.getNumber() == getNumber())
                              {
                                 if (getNumber() > 0)
                                 {
                                    // May leave some bytes uninitialized at end of destination
                                    // array but that's ok since they won't be used
                                    memcpy(mBits, val.mBits, (mNumber + 7) >> 3);
                                 }
                              }
                              else
                              {
                                 DWORD b;
                                 setNumber(val.getNumber());
                                 clear();
                                 for (b=0; b < mNumber; b++)
                                 {
                                    if (val.isBitSet(b))
                                       setBit(b);   
                                 }
                              }                                 
                              return(*this);
                           }


      void                 asText(char *buffer, DWORD max)
      {
         unsigned long i;
         if (mNumber < max)
            max = mNumber;
         for (i=0; i < max-1; i++)
         {
            if (isBitSet((max-1)-i))
               buffer[i] = '1';
            else
               buffer[i] = '0';
         }
         buffer[i] = '\0';
      }

      const unsigned char* getBits() const { return mBits; }


      //Save/load.
      static bool             writeVersion(BChunkWriter *chunkWriter);
      static bool             readVersion(BChunkReader *chunkReader);
      bool                    save(BChunkWriter *chunkWriter) const;
      bool                    load(BChunkReader *chunkReader);

   protected:
      uchar*                  mBits;
      uint32                  mNumber;
      uint32                  mNumberBytes;

      //Static savegame stuff.
      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
};

//==============================================================================
#endif // _BITARRAY_H_

//==============================================================================
// eof: bitarray.h
//==============================================================================

