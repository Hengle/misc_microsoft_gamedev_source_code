//==============================================================================
// xsparsetree.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsparsetree.h"
#include "xsdefines.h"
#include "xsmessenger.h"
#include "xsopcodes.h"

//==============================================================================
// Defines
//#define DEBUGOUTPUTQUADS


//==============================================================================
// BXSParseTreeNode::BXSParseTreeNode
//==============================================================================
BXSParseTreeNode::BXSParseTreeNode(void) :
   mNodeType(-1),
   mResultType(-1),
   mValue(-1),
   mOffset(0),
   mVisited(false),
   mLocked(false),
   mParent(NULL),
   mLeft(NULL),
   mRight(NULL),
   mNumberParameters(0)
{
   memset(mParameters, 0, sizeof(BXSParseTreeNode*)*BXSSyscallModule::cMaximumNumberSyscallParms);
}

//==============================================================================
// BXSParseTreeNode::~BXSParseTreeNode
//==============================================================================
BXSParseTreeNode::~BXSParseTreeNode(void)
{
   if (mLeft != NULL)
   {
      delete mLeft;
      mLeft=NULL;
   }
   if (mRight != NULL)
   {
      delete mRight;
      mRight=NULL;
   }
   for (long i=0; i < mNumberParameters; i++)
      delete mParameters[i];
   memset(mParameters, 0, sizeof(BXSParseTreeNode*)*BXSSyscallModule::cMaximumNumberSyscallParms);
}

//==============================================================================
// BXSParseTreeNode::print
//==============================================================================
void BXSParseTreeNode::print(BXSMessenger *m, bool all)
{
   if (all == true)
   {
      //Left.
      if (mLeft != NULL)
         mLeft->print(m, all);
      //Right.
      if (mRight != NULL)
         mRight->print(m, all);
   }

   //Parms.
   for (long i=0; i < mNumberParameters; i++)
      mParameters[i]->print(m, all);

   //This.
   if (mNodeType == BXSBinary::cOpcode)
   {
      if ((mValue == BXSQuadOpcode::cSUB) &&
         ((mRight == NULL) || (mLeft == NULL)) )
         m->infoMsg("      OPCODE: NEG.");
      else
         m->infoMsg("      OPCODE: %s.", BXSQuadOpcode::getName(mValue));
   }
   else if (mNodeType == BXSBinary::cVariable)
   {
      m->infoMsg("      VARINDEX: %d, OFFSET=%d.", mValue, mOffset);
   }
   else
      m->infoMsg("      VALUE: %d, OFFSET=%d.", mValue, mOffset);
}

//==============================================================================
// BXSParseTreeNode::setVisitedAll
//==============================================================================
void BXSParseTreeNode::setVisitedAll(bool v)
{
   mVisited=v;
   if (mLeft != NULL)
      mLeft->setVisitedAll(v);
   if (mRight != NULL)
      mRight->setVisitedAll(v);
}

//==============================================================================
// BXSParseTreeNode::setLocked
//==============================================================================
void BXSParseTreeNode::setLocked(bool v)
{
   mLocked=v;
   if (mLeft != NULL)
      mLeft->setLocked(v);
   if (mRight != NULL)
      mRight->setLocked(v);
}

//==============================================================================
// BXSParseTreeNode::validate
//==============================================================================
bool BXSParseTreeNode::validate(long &resultType)
{

   //If we have an opcode, we have to validate our subtrees and then check those
   //in conjuction with our opcode.
   if (mNodeType == BXSBinary::cOpcode)
   {
      long leftType=BXSVariableEntry::cInvalidVariable;
      long rightType=BXSVariableEntry::cInvalidVariable;

      if (mLeft != NULL)
      {
         if (mLeft->validate(leftType) == false)
            return(false);
      }
      if (mRight != NULL)
      {
         if (mRight->validate(rightType) == false)
            return(false);
      }

      //If we don't have valid variable types now, we have a problem.
      if ((leftType == BXSVariableEntry::cInvalidVariable) || (rightType == BXSVariableEntry::cInvalidVariable))
         return(false);

      //Check the validity of this opcode and these two operands.
      if (validOpcode(mValue, leftType, rightType, resultType) == false)
         return(false);

      return(true);
   }

   //If we have anything else, we just care about what the return type from the syscall/function
   //is going to be or what the type of the variable is.
   resultType=mResultType;
   return(true);
}

//==============================================================================
// BXSParseTreeNode::validOpcode
//==============================================================================
bool BXSParseTreeNode::validOpcode(long opcode, long type1, long type2, long &resultType) const
{
   //Init.
   resultType=BXSVariableEntry::cInvalidVariable;

   //Common stuff.
   bool anyBools=false;
   bool anyStrings=false;
   bool anyVectors=false;
   if ((type1 == BXSVariableEntry::cBoolVariable) || (type2 == BXSVariableEntry::cBoolVariable))
      anyBools=true;
   if ((type1 == BXSVariableEntry::cStringVariable) || (type2 == BXSVariableEntry::cStringVariable))
      anyStrings=true;
   if ((type1 == BXSVariableEntry::cVectorVariable) || (type2 == BXSVariableEntry::cVectorVariable))
      anyVectors=true;
   bool sameTypes=false;
   if (type1 == type2)
      sameTypes=true;

   switch (opcode)
   {
      case BXSQuadOpcode::cADD:
      case BXSQuadOpcode::cSUB:
      case BXSQuadOpcode::cMUL:
      case BXSQuadOpcode::cDIV:
      case BXSQuadOpcode::cMOD:
         if (type1 == BXSVariableEntry::cVectorVariable)
         {
            if ( (type2 == BXSVariableEntry::cVectorVariable) ||
               ((type2 == BXSVariableEntry::cIntegerVariable) ||
               (type2 == BXSVariableEntry::cFloatVariable)) && 
               ((opcode == BXSQuadOpcode::cMUL) || (opcode == BXSQuadOpcode::cDIV)) )
            {
               resultType=BXSVariableEntry::cVectorVariable;
               return(true);
            }
            return(false);
         }
         if (anyStrings == true)
         {
            if (opcode == BXSQuadOpcode::cADD)
            {
               resultType=BXSVariableEntry::cStringVariable;
               return(true);
            }
            return(false);
         }
         if ((anyBools == true) || (anyVectors == true))
            return(false);

         resultType=type1;
         return(true);

      case BXSQuadOpcode::cNEG:
         if ((type1 == BXSVariableEntry::cBoolVariable) || (type1 == BXSVariableEntry::cStringVariable))
            return(false);
         resultType=type1;
         return(true);

      case BXSQuadOpcode::cASS:
         if ( ((anyStrings == true) || (anyVectors == true)) && (sameTypes == false))
            return(false);
         resultType=type1;
         return(true);

      case BXSQuadOpcode::cLT:
      case BXSQuadOpcode::cLE:
      case BXSQuadOpcode::cGT:
      case BXSQuadOpcode::cGE:
      case BXSQuadOpcode::cEQ:
      case BXSQuadOpcode::cNE:
         if ( ((anyBools == true) || (anyStrings == true) || (anyVectors == true)) && (sameTypes == false))
            return(false);
         resultType=BXSVariableEntry::cBoolVariable;
         return(true);

      case BXSQuadOpcode::cNOT:
         if (type1 != BXSVariableEntry::cBoolVariable)
            return(false);
         resultType=BXSVariableEntry::cBoolVariable;
         return(true);

      case BXSQuadOpcode::cAND:
      case BXSQuadOpcode::cOR:
         if ((anyBools == false) || (sameTypes == false))
            return(false);
         resultType=BXSVariableEntry::cBoolVariable;
         return(true);
   }

   BASSERT(0);
   return(false);
}

//==============================================================================
// BXSParseTreeNode::setLeft
//==============================================================================
void BXSParseTreeNode::setLeft(BXSParseTreeNode *node)
{
   if (node == NULL)
      return;
   mLeft=node;
   node->mParent=this;
}

//==============================================================================
// BXSParseTreeNode::setRight
//==============================================================================
void BXSParseTreeNode::setRight(BXSParseTreeNode *node)
{
   if (node == NULL)
      return;
   mRight=node;
   node->mParent=this;
}

//==============================================================================
// BXSParseTreeNode::getRoot
//==============================================================================
BXSParseTreeNode* BXSParseTreeNode::getRoot(void)
{
   BXSParseTreeNode *root=this;
   while (root->mParent != NULL)
      root=root->mParent;
   return(root);
}

//==============================================================================
// BXSParseTreeNode::addParameter
//==============================================================================
bool BXSParseTreeNode::addParameter(BXSParseTreeNode *node)
{
   if (node == NULL)
      return(false);
   if (mNumberParameters >= BXSSyscallModule::cMaximumNumberSyscallParms)
      return(false);

   mParameters[mNumberParameters]=node;
   mNumberParameters++;
   return(true);
}

//==============================================================================
// BXSParseTreeNode::outputQuads
//==============================================================================
bool BXSParseTreeNode::outputQuads(BXSQuadArray &quads, BXSMessenger *m)
{
   //If we have a parent, this call is invalid.
   if (mParent != NULL)
      return(false);
   //Make sure we haven't visited anything.
   setVisitedAll(false);

   //Start at the root (us).
   BXSParseTreeNode *cur=this;

   //Create the opcode stream to put the parse tree on the stack.
   #ifdef DEBUGOUTPUTQUADS
   m->infoMsg("Output Quads:");
   #endif
   while (cur != NULL)
   {
      //Go down the left side.
      if ((cur->getLeft() != NULL) && (cur->getLeft()->getVisited() == false))
         cur=cur->getLeft();
      //Right side next.
      else if ((cur->getRight() != NULL) && (cur->getRight()->getVisited() == false))
         cur=cur->getRight();
      //Visit this level's root and then pop up a level.
      else
      {
         if (cur->getNodeType() == BXSBinary::cOpcode)
         {
            if ((cur->getValue() == BXSQuadOpcode::cSUB) &&
               ((cur->getRight() == NULL) || (cur->getLeft() == NULL)) )
            {
               #ifdef DEBUGOUTPUTQUADS
               m->infoMsg("  OPCODE: NEG.");
               #endif
               if (quads.add(BXSQuad(BXSQuadOpcode::cNEG)) == -1)
                  return(false);
            }
            else
            {
               #ifdef DEBUGOUTPUTQUADS
               m->infoMsg("  OPCODE  : %s.", BXSQuadOpcode::getName(cur->getValue()));
               #endif
               if (cur->getValue() < 0)
               {
                  BASSERT(0);
                  return(false);
               }
               if (quads.add(BXSQuad(cur->getValue())) == -1)
                  return(false);
            }
         }
         else if ((cur->getNodeType() == BXSBinary::cSyscall) || (cur->getNodeType() == BXSBinary::cFunction))
         {
            //Output parms.  It's a problem if we have parms and we don't have a CALLF or CALLS opcode.
            for (long i=0; i < cur->mNumberParameters; i++)
            {
               if (cur->mParameters[i]->outputQuads(quads, m) == false)
                  return(false);
               //m->infoMsg("  PARM  : %d.", ->getValue());
            }

            if (cur->getNodeType() == BXSBinary::cSyscall)
            {
               #ifdef DEBUGOUTPUTQUADS
               m->infoMsg("  SYSCALL: %d.", cur->getValue());
               #endif
               if (quads.add(BXSQuad(BXSQuadOpcode::cCALLS, cur->getValue())) == -1)
                  return(false);
            }
            else
            {
               #ifdef DEBUGOUTPUTQUADS
               m->infoMsg("  FUNCTION: %d.", cur->getValue());
               #endif
               if (quads.add(BXSQuad(BXSQuadOpcode::cCALLF, cur->getValue())) == -1)
                  return(false);
            }
         }
         else if (cur->getNodeType() == BXSBinary::cVariable)
         {
            #ifdef DEBUGOUTPUTQUADS
            m->infoMsg("  VARINDEX: %d.", cur->getValue());
            #endif
            if (quads.add(BXSQuad(BXSQuadOpcode::cPUSH, cur->getValue(), cur->getOffset())) == -1)
               return(false);
         }
         else if (cur->getNodeType() == BXSBinary::cImmediateVariable)
         {
            #ifdef DEBUGOUTPUTQUADS
            m->infoMsg("  IMMVAR: %d.", cur->getValue());
            #endif
            if (quads.add(BXSQuad(BXSQuadOpcode::cPUSHI, cur->getResultType(), cur->getValue())) == -1)
               return(false);
         }
         else
            BASSERT(0);

         //We've been here now.
         cur->setVisited(true);
         //Pop to our parent.
         cur=cur->getParent();
      }
   }

   return(true);
}

