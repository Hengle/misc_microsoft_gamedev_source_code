//==============================================================================
// xstokenizer.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSTOKENIZER_H_
#define _XSTOKENIZER_H_

//==============================================================================
//  DCP NOTES:
//  * Still basically the same code as before with the change that the tokenizing
//    is now done up front to enable more reasonable pushing/popping of tokens.
//  * May eventually have to beef this class up with full data parameterization
//    if other uses don't have the same basic tokens or parsing rules.
//==============================================================================


//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BXSCompiler;
class BXSMessenger;

//==============================================================================
// Const declarations


//==============================================================================
class BXSTokenizer
{
   public:
      enum 
      {
         cUnknown=0,  // for initializing variables 
         cError,     
         cName,      // the compiler distinguishes names using symbol tables 
         cLeftParen, // (
         cRightParen,// )
         cLT,        // <
         cLE,        // <=
         cEQ,        // ==
         cNE,        // !=
         cGE,        // >=
         cGT,        // >
         cAdd,       // +
         cSub,       // -
         cMult,      // *
         cDiv,       // /
         cMod,       // %
         cAssign,    // =
         cPlusPlus,  // ++
         cMinusMinus,// --
         cColon,     // :
         cSemiColon, // ;
         cComma,     // ,
         cLeftBrace, // {
         cRightBrace,// }
         cPeriod,    // .
         cWhiteSpace,// DUH:)
         cAnd,       // &&
         cOr,        // ||
         cBitAnd,    // &
         cBitOr,     // |
         cInclude,   // 
         cSwitch,    // 
         cCase,      // 
         cWhile,     // 
         cBreak,     // 
         cDefault,   // 
         cRule,      //
         cIf,        //
         cThen,      //
         cElse,      //
         cGoto,      //
         cLabel,     //
         cFor,       //
         cDBG,       //
         cILL,       //
         cIRL,       //
         cReturn,    //
         cVoid,      //
         cInteger,   //
         cFloat,     //
         cBool,      // true or false.  Helpful to compiling and parsing to categorize them as specific token types.
         cVector,    // 
         cString,    // "..."
         cConst,     //
         cPriority,  //
         cMinInterval,//
         cMaxInterval,//
         cHighFrequency,//
         cActive,    //
         cInactive,  //
         cGroup,     //
         cBreakpoint,
         cStatic,
         cContinue,
         cExtern,
         cExport,
         cRunImmediately,
         cMutable,
         cClass,
         cEOS
      };
      enum
      {
         cMaxTokenSize=127
      };

      //Ctor/Dtor.
      BXSTokenizer( BXSMessenger *messenger );
      ~BXSTokenizer( void );

      //Simple lookups.
      long                       getFileID( void ) const { return(mFileID); }
      void                       setFileID( long v ) { mFileID=v; }
      const BSimString&             getFilename( void ) const { return(mFilename); };
      const BSimString&             getFilenameWithoutExtension( void ) const { return(mFilenameWithoutExtension); };
      const char*                getSource( void ) const { return(mSource); }
      long                       getSourceLength( void ) const { return(mSourceLength); }
      long                       getNumberLines( void ) const { return(mNumberLines); }

      //Init methods.
      void                       initialize( void );

      //Source methods.
      bool                       setSource( const BSimString &filename, const char *source, long sourceLength );

      //High level token methods.  NOTE:
      //  getToken() gets the NEXT token.
      //  getCurrentToken() gets the current token.
      //  getFutureToken( long n ) returns the Nth token past the current token W/O CHANGING THE CURRENT TOKEN.
      //  getDarkSpaceToken() gets the NEXT non-whitespace token.
      bool                       tokenize( void );
      long                       getToken( BXSCompiler *compiler );
      long                       getCurrentToken( void );
      long                       getCurrentTokenNumber( void ) const { return(mCurrentToken); }
      long                       getFutureToken( long numberTokensAhead, bool countWhiteSpace );
      long                       getDarkSpaceToken( BXSCompiler *compiler );
      //Current token methods.  NOTE: getCurrentTokenText that doesn't take any args uses a
      //static scratch space var in this class, so subsequent calls to it overwrite previous
      //results.
      void                       decrementCurrentToken( BXSCompiler *compiler );
      void                       incrementCurrentToken( BXSCompiler *compiler );
      char*                      getCurrentTokenText( void );
      bool                       getCurrentTokenText( char **text, long maxLength );
      long                       getCurrentTokenLineNumber( void ) const;

      //Debug.
      const char*                getTokenTypeName( long token );
      void                       setDebug( bool v ) { mDebug=v; }

   protected:
      long                       mFileID;
      BSimString                    mFilename;
      BSimString                    mFilenameWithoutExtension;
      BXSMessenger*              mMessenger;
      const char*                mSource;
      long                       mSourceLength;
      long                       mNumberLines;

      class BTokenEntry
      {
         public:
            long                 token;
            long                 start;
            long                 length;
            long                 lineNumber;
            #ifdef _DEBUG
            char                 string[cMaxTokenSize+1];
            #endif
      };
      typedef BDynamicSimArray<BTokenEntry> BTokenArray;
      BTokenArray                mTokens;

      long                       mCurrentToken;

      long                       mStartStates[256];
      bool                       mInAName[256];
      static char                mScratchString[cMaxTokenSize+1];
      bool                       mDebug;
      long                       mCurrentLineNumber;

      enum TokenizeStates
      {
         cStartState,
         cErrorState,
         cWhiteSpaceState,
         cCPPCommentState,
         cCPPCommentState2,
         cNameStartState,
         cNameInState,
         cSubState,
         cIntegerState,
         cFirstFractDigitState,
         cNextFractDigitState,
         cExpSignState,
         cStartingPeriodState,
         cFirstExpDigitState,
         cNextExpDigitState,
         cStringStartState,
         cStringInState,
         cLParenState,
         cRParenState,
         cLessState,
         cEqualState,
         cGreaterState,
         cBangState,
         cAddState,
         cMultState,
         cDivState,
         cModState,
         cColonState,
         cSemiColonState,
         cCommaState,
         cLeftBraceState,
         cRightBraceState,
         cAndState,
         cOrState,
         cEOSState
      };
}; // BXSTokenizer
typedef BDynamicSimArray<BXSTokenizer*> BXSTokenizerArray;


//==============================================================================
#endif // _XSTOKENIZER_H_
