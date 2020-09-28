//==============================================================================
// xsfileeentry.h
//
// Copyright (c) 2001-2003, Ensemble Studios
//==============================================================================

#ifndef _XSFILEENTRY_H_
#define _XSFILEENTRY_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif


//==============================================================================
class BXSFileEntry
{
   public:
      BXSFileEntry( void );
      ~BXSFileEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Filename.
      const char*                getFilename( void ) const { return(mFilename); }
      long                       getFilenameLength( void ) const { return(mFilenameLength); }
      bool                       setFilename( const char *filename );
      //Source (can be NULL).
      const char*                getSource( void ) const { return(mSource); }
      long                       getNumberSourceLines( void ) const { return(mSourceLines.getNumber()); }
      const char*                getSourceLine( long lineNumber ) const;
      long                       getSourceLength( void ) const { return(mSourceLength); }
      bool                       setSource( const char *source );

      //Save and Load.
      #ifdef _BANG
      static bool             writeVersion( BChunkWriter *chunkWriter );
      static bool             readVersion( BChunkReader *chunkReader );
      static void             setVersion( DWORD v ) { msLoadVersion=v; }
      bool                    save( BChunkWriter* chunkWriter );
      bool                    load( BChunkReader* chunkReader );
      #endif

   protected:
      long                       mID;
      char*                      mFilename;
      long                       mFilenameLength;
      char*                      mSource;
      BDynamicSimLongArray           mSourceLines;
      long                       mSourceLength;

      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
};
typedef BDynamicSimArray<BXSFileEntry*> BXSFileEntryArray;


//==============================================================================
#endif // _XSFILEENTRY_H_
