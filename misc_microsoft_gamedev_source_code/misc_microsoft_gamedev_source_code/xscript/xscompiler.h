//==============================================================================
// xscompiler.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================
#ifndef _XSCOMPILER_H_
#define _XSCOMPILER_H_


//==============================================================================
// Includes
#include "xssymboltable.h"
#include "xsdefines.h"
#include "xsfileentry.h"
#include "xsfunctionentry.h"
#include "xsquad.h"
#include "xsruleentry.h"
#include "xssyscallmodule.h"
#include "xstokenizer.h"
#include "xsvariableentry.h"

//==============================================================================
// Forward declarations
class BXSCompiler;
class BXSData;
class BXSEmitter;
class BXSMessenger;
class BXSOptimizer;
class BXSSource;
class BXSParseTreeNode;

//==============================================================================
// Const declarations


//==============================================================================
class BXSCompiler
{
   public:
      //Ctor/Dtor.
      BXSCompiler( BXSMessenger *messenger, BXSData *data, BXSSource *source, BXSSyscallModule *sm );
      virtual ~BXSCompiler( void );

      //Init.
      bool                       initialize( void );
      bool                       initialize(long baseDirID, long baseUWDirID);

      //Syscalls.
      BXSSyscallModule*          getSyscalls( void ) const { return(mSyscalls); }
      //Source.
      BXSSource*                 getSource( void ) const { return(mSource); }
      //Data.
      BXSData*                   getData( void ) const { return(mData); }

      //Module management.
      long                       getNumberModules( void ) const { return(mModuleFilenames.getNumber()); }
      const BSimString&             getModuleFilename( long index ) const;
      bool                       addModuleFilename( const BSimString &filename );       
      bool                       removeModuleFilename( const BSimString &filename );       

      //Compilation.
      bool                       addExternalVariable( const char *source );
      bool                       compileModules( const BSimString &name );
      bool                       compileFile( const BSimString &filename, const BSimString &qualifiedFilename,
                                    const BSimString &listPrefix, const char *source, long sourceLength );
      //Compilation options.
      bool                       getListing( void ) const { return(mListing); }
      void                       setListing( bool v ) { mListing=v; };
      bool                       getDebugTokenizer( void ) const { return(mDebugTokenizer); }
      void                       setDebugTokenizer( bool v ) { mDebugTokenizer=v; }
      bool                       getDebugCode( void ) const { return(mDebugCode); }
      void                       setDebugCode( bool v ) { mDebugCode=v; }

      //"Immediate" parsing.  Takes a string, parses it, and immediately executes it.  This obviously
      //doesn't support things like variables and what not.
      bool                       parseImmediate( const char *source, long sourceLength );

      //Misc.
      void                       setLabelPosition( long labelID, long labelPosition );
      void                       addLineNumberQuad( long lineNumber );
      void                       addFileNumberQuad( long fileNumber );

      //Misc.
      const BSimString&             getIncludePath( void ) const { return(mIncludePath); }
      void                       setIncludePath( const BSimString &v );
      void                       dumpTables( BFile *dumpFile );
      const char*                getSyscallName( long id );
      const char*                getFunctionName( long id );
      const char*                getVariableName( long id );
      const char*                getVariableTypeName( long variableType );
      const char*                getLabelName( long id );

   protected:
      //Internal file compilation.
      bool                       prepareCompilation( const BSimString &listPrefix, const BSimString &filename, const BSimString &qualifiedFilename );
      bool                       finishCompilation( const BSimString &listPrefix, const BSimString &filename );
      bool                       parseSource( const BSimString &qualifiedFilename, const BSimString &filename, const char *source, long sourceLength );
      BXSTokenizer*              getTokenizer( void ) const;
      BXSTokenizer*              getTokenizer( const BSimString &filename ) const;
      BXSTokenizer*              createTokenizer( void );
      bool                       popTokenizer( void );

      //High level parsing methods.
                                 //parse.  Top level parsing function.
      void                       parse( void );
                                 //parseVarDecl.  Variable declaration.  Forces assignment for a default
                                 //value.  Requires type name to be NEXT token.  Strips a semicolon if stripSemiColon
                                 //is true.  If parameter is true, will add this "variable" to the current function
                                 //as a parameter instead of a variable.  If forceConstantInitialValue is true, the
                                 //variable must have a constant (i.e. non-expression) initial value.
      bool                       parseVarDecl( bool incrementTokenFirst, bool stripSemiColon, bool parameter, bool forceConstantInitialValue );
                                 //parseFunctionDecl.  Function declaration.  Parses everything for a function
                                 //definition.  Requires return type to be NEXT token.  Requires code to be braced.
                                 //Strings everything off up to the last brace.  The current token will be '}' upon
                                 //exit.
      bool                       parseFunctionDecl( bool incrementTokenFirst );
                                 //parseConditionDecl.  Parses an IF-THEN-(ELSE) statement.  Expects the 'if' token
                                 //to be the current token at the time of call.  Eats everything up through the
                                 //last brace (if any).
      bool                       parseConditionDecl( void );
                                 //parseLabelDecl.  Defines a label.  Expects the 'label' token to be the current token
                                 //at the time of call.  Eats everything up through AND including the ';'.
      bool                       parseLabelDecl( void );
                                 //parseGotoDecl.  Defines a goto.  Expects the 'goto' token to be the current token at the
                                 //time of call.  Eats everything up through AND including the ';'.
      bool                       parseGotoDecl( void );
                                 //parseClassDefinition.  Defines a user class.  Expects the 'class' token to be the current token
                                 //at the time of call.  Eats everything up through AND including the ';'.
      bool                       parseClassDefinition( bool incrementTokenFirst );
                                 //parseClassDeclaration.  User Class declaration.  Requires type name to be NEXT token.
                                 //Strips a semicolon if stripSemiColon is true.  If parameter is true, will add this
                                 //"variable" to the current function as a parameter instead of a variable.
      bool                       parseClassDeclaration( bool incrementTokenFirst, bool stripSemiColon, bool parameter );
                                 //parseVariableAssignment.  Simple variable assignment of form 'var=X'  Strips the
                                 //semicolon if directed to.
      bool                       parseVariableAssignment( long token, char *tokenText, bool stripSemiColon, bool forceAssignment );
                                 //parseForLoop.  Expects the 'for' token to be the current token at the time of call.
                                 //Strips everything up to AND including any single statement terminator or ending brace.
      bool                       parseForLoop( void );
                                 //parseDBG.  Debug opcode parsing.  Doesn't mess with tokenizer.  Expects the passed
                                 //token to reference a variable.
      bool                       parseDBG( long token, char *tokenText );
                                 //parseILL.  Infinite loop limit parsing.  Doesn't mess with tokenizer.
      bool                       parseILL( void );
                                 //parseIRL.  Infinite recursion limit parsing.  Doesn't mess with tokenizer.
      bool                       parseIRL( void );
                                 //parseReturn.  Expects 'return' to be the current token at the time of call.  Strips
                                 //everything up to AND including the ';' on the return statement.
      bool                       parseReturn( void );
                                 //parseInclude.  Strips everything up to AND including the ';' on the include statement.
      bool                       parseInclude( bool incrementTokenFirst );
      bool                       parseSwitch( bool incrementTokenFirst );
      bool                       parseWhile( bool incrementTokenFirst );
      bool                       parseBreak( bool incrementTokenFirst );
      bool                       parseContinue( bool incrementTokenFirst );
      bool                       parseBreakpoint( void );
      bool                       parseRule( bool incrementTokenFirst );

      //Mid level parsing methods.
                                 //parseCode can handle a single line of code or braced code.  If singleStatement
                                 //is false, the code must be braced.  If braced, the '}' will be the current token
                                 //upon exit.
      bool                       parseCode( bool singleStatement, bool topLevel, bool *returnAdded );
                                 //parseSysFuncCall.  Strips everything up to but NOT including the terminating ';'.
      BXSParseTreeNode*          parseSysFuncCall( const BXSSymbolEntry *pEntry );
      bool                       parseAtomic( long token, char* tokenText, long *variableIndex, long *variableValue, long *variableType,
                                    long *variableOffset, bool *immediateVariable );
      bool                       parseExpression( bool incrementTokenFirst, bool stopOnParen, bool stopOnSemiColon,
                                    bool stopOnComma, bool stopOnColon, bool stripStopToken, long resultType );
      BXSParseTreeNode*          parseExpression2( bool incrementTokenFirst, bool stopOnParen, bool stopOnSemiColon,
                                    bool stopOnComma, bool stopOnColon, bool stripStopToken, long resultType );

      //Low level, very simple parsing methods.
                                 //parseVariableTypeName.  Takes the given token and returns the variable type
                                 //for it.  Does not touch the tokenizer.
      BYTE                       parseVariableTypeName( long token, bool voidAllowed );
      bool                       parseIntegerConstant( long *value );
      bool                       parseFloatConstant( float *value );
      bool                       parseBoolConstant( bool *value );
      bool                       parseVectorConstant( BVector *value );
      bool                       parseVector( BVector *value );

      //Label methods.
      bool                       addLabelQuad( long labelID, bool pushLabelUp );
      bool                       fixupLabelsAndOverloads( void );
      long                       createLabel( char *name, bool useScope );

      //Function overload methods.
      bool                       addFunctionOverload( long oldFID, long newFID );

      //Misc Functions.
      long                       createTemporaryVariable( long variableType, void *data );
      bool                       createIncrementVariableCode( long variableIndex, long incrementValue );
      bool                       createVariableAssignCode( long lValueIndex, long rValueIndex );
      bool                       createVariableAssignCodeImmediate( long lValueIndex, long type, long value );

      long                       createRuleGroup( char *name );
      long                       getDefaultLongVariableValue( long variableID );
      const BXSSymbolEntry*      getSymbolEntry( char *name, bool useGlobalScope );
      long                       getOperatorOpcode( long token );
      bool                       isHigherPrecedence( long opcode1, long opcode2 );
      bool                       isLowerPrecedence( long opcode1, long opcode2 );
   
      BXSFunctionEntry           *getCurrentFunctionEntry(void);

      //Error Functions.
      void                       srcErrMsg( const char *message, long arg1 );
      void                       srcErrMsg( const char *message, long arg1, const char *arg2 );
      void                       srcErrMsg( const char *message, const char *arg1 );
      void                       srcErrMsg( const char *message, const char *arg1, const char *arg2 );
      void                       srcErrMsg( const char *message );
      
      //Output Functions.
      bool                       emitTheLists( BXSQuadArray &quads, long *offset );

      //Clean up.
      void                       cleanUp( void );

      //General/Subsystem stuff.
      BXSTokenizer*              mBaseTokenizer;
      BXSTokenizerArray          mTokenizers;
      BXSTokenizerArray          mUnusedTokenizers;
      BXSEmitter*                mEmitter;
      BXSOptimizer*              mOptimizer;

      //Switches.
      bool                       mListing;
      bool                       mDebugTokenizer;
      bool                       mDebugCode;

      //The rest of this is data that's particular to the compiled code from a single
      //set of source files.
      char                       mCurrentScope[BXSTokenizer::cMaxTokenSize+2];
      long                       mCurrentFunctionID;
      long                       mCurrentClassID;
      //Function overloads.
      BXSFunctionOverloadArray   mFunctionOverloads;
      //Labels.
      BXSLabelArray              mLabels;
      //Break label IDs.
      BDynamicSimLongArray           mBreakLabelIDs;
      //Continue label IDs.
      BDynamicSimLongArray           mContinueLabelIDs;
      //Quads.
      BXSQuadArray               mQuads;
      //Misc.
      BSimString                    mIncludePath;
      long                       mBaseDirID;
      long                       mBaseUWDirID;
      BFile*                     mListFile;
      //Modules.
      BDynamicSimArray<BSimString>      mModuleFilenames;

      //Syscalls.
      BXSSyscallModule*          mSyscalls;
      //Source.
      BXSSource*                 mSource;
      //XSData.  Where all of the instance-specific data results go.  Built up as the code compiles.
      BXSData*                   mData;
      //Messenger.
      BXSMessenger*              mMessenger;

      //Console/Immediate parsing stuff.
      char                       mParseImmediateStrings[BXSSyscallModule::cMaximumNumberSyscallParms][BXSTokenizer::cMaxTokenSize+1];
      BVector                    mParseImmediateVectors[BXSSyscallModule::cMaximumNumberSyscallParms];
};


//==============================================================================
#endif // _XSCOMPILER_H_
