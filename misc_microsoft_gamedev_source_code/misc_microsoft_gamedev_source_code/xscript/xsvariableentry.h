//==============================================================================
// xsvariableentry.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSVARIABLEENTRY_H_
#define _XSVARIABLEENTRY_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BXSUserClassEntry;
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
class BXSData;
#endif


//==============================================================================
class BXSVariableEntry
{
   public:
      enum
      {
         cInvalidVariable     =255,
         cVoidVariable        =0x00000001,
         cIntegerVariable     =0x00000002,
         cFloatVariable       =0x00000004,
         cBoolVariable        =0x00000008,
         cStringVariable      =0x00000010,
         cVectorVariable      =0x00000020,
         cUserClassVariable   =0x00000040
      };
      enum
      {
         cIntegerDataSize=sizeof(long),
         cFloatDataSize=sizeof(float),
         cBoolDataSize=sizeof(long), //sizeof(bool),
         cVectorDataSize=sizeof(BVector),
         cUserHeapDataSize=sizeof(long),
         cTempStringConversionMaxSize=256
      };
      enum
      {
         cConst   =0x01,
         cExtern  =0x02,
         cStatic  =0x04,
         cExport  =0x08,
         cTemp    =0x10,
      };

      BXSVariableEntry( void );
      ~BXSVariableEntry( void );

      //Symbol.
      long                       getSymbolID( void ) const { return(mSymbolID); }
      void                       setSymbolID( long v ) { mSymbolID=v; }
      //Type.
      long                       getType( void ) const { return(mType); }
      void                       setType( long v ) { mType=v; }
      //Const.  Const is just a compile side thing that the compiler has the responsibility to check.
      //As such, it's not factored into any of the size or output.  If the compiler creates an opcode
      //sequence that assigns a value into the variable, it will work when interpreted.
      bool                       getConst( void ) const { if (mModifiers&cConst) return(true); return(false); }
      void                       setConst( bool v );
      //Extern.  True if this is an extern variable (one that can be referenced from multiple source
      //files).
      bool                       getExtern( void ) const { if (mModifiers&cExtern) return(true); return(false); }
      void                       setExtern( bool v );
      //Static.  True if this is a static variable (one that maintains its value across function
      //calls).
      bool                       getStatic( void ) const { if (mModifiers&cStatic) return(true); return(false); }
      void                       setStatic( bool v );
      //Export.  True if this is a export variable (one that is 'exported' outside the runtime for
      //potential initial value override).
      bool                       getExport( void ) const { if (mModifiers&cExport) return(true); return(false); }
      void                       setExport( bool v );
      //Temp.  True if this is a temporary variable (created by the compiler during compilation to
      //actually make the interp work).
      bool                       getTemp( void ) const { if (mModifiers&cTemp) return(true); return(false); }
      void                       setTemp( bool v );

      //Data.
      long                       getDataLength( void ) const;
      BYTE*                      getData( void ) const;
      bool                       setData( void *d );
      //All.
      bool                       setAll( long s, long t, void *d );
      //String.
      bool                       setString( void *d );
      long                       getStringLength( void ) const;
      //User Class.
      bool                       initUserClass( BXSUserClassEntry *uce );
      bool                       setUserClass( void *d );
      long                       getUserClassLength( void ) const;

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter, BXSData *data );
      bool                       load( BChunkReader* chunkReader, BXSData *data );
      #endif

      //Copy.
      bool                       copy( BXSVariableEntry *v );

   protected:
      long                       mSymbolID;
      long                       mType;
      BYTE*                      mData;
      BYTE                       mModifiers;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSVariableEntry*> BXSVariableEntryArray;


//==============================================================================
#endif // _XSVARIABLEENTRY_H_
