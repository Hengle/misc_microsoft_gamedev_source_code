//==============================================================================
// xsemitter.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSEMITTER_H_
#define _XSEMITTER_H_

//==============================================================================
// Includes.
#include "xsquad.h"

//==============================================================================
// Forward declarations.
class BXSCompiler;
class BXSSource;
class BXSMessenger;
class BFile;

//==============================================================================
class BXSEmitter
{
   public:
      //Ctor and Dtor.
      BXSEmitter( BXSMessenger *messenger );
      ~BXSEmitter( void );

      //Emit methods.
      bool                       emitQuads( BXSQuadArray &quads, BXSSource *source,
                                    long *offset, BXSCompiler *compiler );
      
      //Misc.
      long                       getNumberBytesEmitted( long opcode );

      //Backpatch.
      bool                       backpatchLong( long position, long value, BXSSource *source );
      
      //Debug.
      void                       setListFile( BFile *file ) { mListFile=file; }
      void                       addListLine( bool lineFeed, const char *output, ... );
      void                       addListLine2( bool lineFeed, const char *output, ... );

      //Misc.
      void                       clear( void );

   protected:
      //The "Big" emit method.
      bool                       emitTheBytes( BXSQuadArray &quads, BXSSource *source, BXSCompiler *compiler );

      //Low level emit methods.
      bool                       emit1BYTE( long value, BXSSource *source );
      bool                       emit2BYTE( long value, BXSSource *source );
      bool                       emit4BYTE( long value, BXSSource *source );

      //Variables.
      BXSMessenger*              mMessenger;
      BFile*                     mListFile;
      char                       mListingCodeBytes[132];
      static char                mScratchString[132];
}; // BXSEmitter


//==============================================================================
#endif // _XSEMITTER_H_
