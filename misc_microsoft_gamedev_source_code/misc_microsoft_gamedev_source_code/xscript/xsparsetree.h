//==============================================================================
// xsparsetree.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSPARSETREE_H_
#define _XSPARSETREE_H_

//==============================================================================
// Includes
#include "xsquad.h"
#include "xssyscallmodule.h"

//==============================================================================
// Const declarations.

//==============================================================================
// Forward declarations.
class BXSMessenger;


//==============================================================================
class BXSParseTreeNode
{
   public:
      BXSParseTreeNode( void );
      ~BXSParseTreeNode( void );

      //Print.
      void                       print( BXSMessenger *m, bool all );

      //Node Type.  This is the type of thing this is (opcode, syscall, variable, immediate variable, etc.).
      long                       getNodeType( void ) const { return(mNodeType); }
      void                       setNodeType( long v ) { mNodeType=v; }
      //Result Type.  This is the type of variable this represents (int, bool, etc.).
      long                       getResultType( void ) const { return(mResultType); }
      void                       setResultType( long v ) { mResultType=v; }
      //Value.  This is the node-type specific 'ID'.  It could be an index, an immediate variable value, etc.
      long                       getValue( void ) const { return(mValue); }
      void                       setValue( long v ) { mValue=v; }
      //Offset.  Index offset for user variables that have more than one base variable in them.  Zero for all base types.
      long                       getOffset( void ) const { return(mOffset); }
      void                       setOffset( long v ) { mOffset=v; }
      //Visited.
      bool                       getVisited( void ) const { return(mVisited); }
      void                       setVisited( bool v ) { mVisited=v; }
      void                       setVisitedAll( bool v );
      //Locked.
      bool                       getLocked( void ) const { return(mLocked); }
      void                       setLocked( bool v );

      //Trial evaluate (to check validity of expression).
      bool                       validate( long &resultType );
      bool                       validOpcode( long opcode, long type1, long type2, long &resultType ) const;

      //Tree stuff.
      BXSParseTreeNode*          getParent( void ) const { return(mParent); }
      BXSParseTreeNode*          getLeft( void ) const { return(mLeft); }
      void                       setLeft( BXSParseTreeNode *node );
      BXSParseTreeNode*          getRight( void ) const { return(mRight); }
      void                       setRight( BXSParseTreeNode *node );
      BXSParseTreeNode*          getRoot( void );

      //Parameters.
      long                       getNumberParameters( void ) const { return(mNumberParameters); }
      bool                       addParameter( BXSParseTreeNode *node );

      //Output Quads.
      bool                       outputQuads( BXSQuadArray &quads, BXSMessenger *m );

   protected:
      long                       mNodeType;
      long                       mResultType;
      long                       mValue;
      long                       mOffset;
      bool                       mVisited;
      bool                       mLocked;

      //Parent, Left, Right.  Standard Unary/Binary parse tree structure.
      BXSParseTreeNode*          mParent;
      BXSParseTreeNode*          mLeft;
      BXSParseTreeNode*          mRight;
      //Parameters are grafted in here to simply allow syscalls and functions
      //to be put into the same parse tree w/o grubbing up the simple to grok
      //Right and Left system.
      long                       mNumberParameters;
      BXSParseTreeNode*          mParameters[BXSSyscallModule::cMaximumNumberSyscallParms];
};


//==============================================================================
#endif // _XSPARSETREE_H_
