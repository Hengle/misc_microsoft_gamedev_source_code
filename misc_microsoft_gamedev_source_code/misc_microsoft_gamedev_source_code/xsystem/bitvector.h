//==============================================================================
// bitvector.h
//
// Copyright (c) 1997-2001 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
//==============================================================================

// Forward declarations
class BChunkReader;
class BChunkWriter;
//==============================================================================
// Const declarations



//==============================================================================
// Simple array for AND testing.
/*
const DWORD BBitVectorAND[] = 
{
   0x00000001, 0x00000002, 0x00000004, 0x00000008,
   0x00000010, 0x00000020, 0x00000040, 0x00000080,
   0x00000100, 0x00000200, 0x00000400, 0x00000800,
   0x00001000, 0x00002000, 0x00004000, 0x00008000,
   0x00010000, 0x00020000, 0x00040000, 0x00080000,
   0x00100000, 0x00200000, 0x00400000, 0x00800000,
   0x01000000, 0x02000000, 0x04000000, 0x08000000,
   0x10000000, 0x20000000, 0x40000000, 0x80000000 
};

//==============================================================================
class BBitVector
{
public:
   BBitVector( void ) : mValue(0) {  }

   long  getValue( void ) const { return(mValue); }
   void  zero( void ) { mValue=0; }
   bool  isSet( long n ) const { if ((n >= 0) && (n < 32) && ((mValue&BBitVectorAND[n]) > 0)) return true; return false; }
   void  set( long n ) { if ((n >= 0) && (n < 32)) mValue=mValue|BBitVectorAND[n]; }
   void  setAll( long v ) { mValue=v; }
   void  unset( long n ) { if ((n >= 0) && (n < 32)) mValue=mValue&~BBitVectorAND[n]; }

protected:
   long   mValue;

}; // BBitVector
*/

class BBitVector
{
public:
   BBitVector( void ) : mValue(0) {  }
   
   long  getValue( void ) const { return(mValue); }
   void  zero( void ) { mValue=0; }
   bool  isSet( long n ) const { if ((n >= 0) && (n < 32) && ((mValue&(1<<n)) > 0)) return true; return false; }
   void  set( long n ) { if ((n >= 0) && (n < 32)) mValue=mValue|(1<<n); }
   void  setAll( long v ) { mValue=v; }
   void  unset( long n ) { if ((n >= 0) && (n < 32)) mValue=mValue&~(1<<n); }
   
protected:
   long   mValue;
   
}; // BBitVector

template <long Size> class UTBitVector
{
public:
   UTBitVector( void ) { zero(); }
   
   char  getValue( long index=0 ) const 
   {
      if ((index < 0) || ((index<<3) >= Size))
      {
         BASSERT(0);
         return(false);
      }      
      return(mValue[index]); 
   }
   
   void  zero( void ) 
   { 
      Utils::FastMemSet(mValue, 0, sizeof(mValue));
   }

   void  setToOnes( void ) 
   { 
      Utils::FastMemSet(mValue, 0xFF, sizeof(mValue));
   }
   
   // 15aug01 - ham - since this is called a million times, 
   //    we're not going to waste time to convert this to a lowercase bool

   __forceinline BOOL isSet( long n ) const 
   { 
#ifdef _DEBUG
      if ((n < 0) || (n >= Size))
      {
         BASSERT(0);
         return 0;
      }
#endif
      return (mValue[n>>3])&(1<<(n%8)); 
   }
   
   __forceinline bool checkAndSet( long n )
   { 
#ifdef _DEBUG
      if ((n < 0) || (n >= Size))
      {
         BASSERT(0);
         return 0;
      }
#endif
      DWORD mask=(1<<(n%8));
      DWORD byte=n>>3;
      if(!(mValue[byte]&mask))
      {
         mValue[byte] |= mask;
         return(false);
      }
      return(true);
   }

   void  set( long n ) 
   { 
      if ((n < 0) || (n >= Size))
      {
         BASSERT(0);
         return;
      }
      mValue[n>>3]|=(1<<(n%8)); 
   }
   
   void  setAll( char v ) 
   { 
      memset(mValue, v, sizeof(mValue)); 
   }
   
   void  unset( long n ) 
   { 
      if ((n < 0) || (n >= Size))
      {
         BASSERT(0);
         return;
      }
      mValue[n>>3]&=~(1<<(n%8)); 
   }

   long getSize(void) const { return(Size); }
   
   const char *getRawValue(void) const {return(mValue);}
   
   bool load(BChunkReader* chunkReader);
   bool save(BChunkWriter* chunkWriter) const;

protected:
   char  mValue[(Size>>3) + ((Size%8)?1:0)];
}; // BBitVector

typedef UTBitVector<8>  BBitVector8;
typedef UTBitVector<16> BBitVector16;
typedef UTBitVector<32> BBitVector32;
typedef UTBitVector<64> BBitVector64;
typedef UTBitVector<96> BBitVector96;

//==============================================================================
#include "bitvector.inl"

//==============================================================================
// eof: bitvector.h
//==============================================================================
