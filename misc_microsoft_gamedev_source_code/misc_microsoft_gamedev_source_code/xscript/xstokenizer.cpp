//==============================================================================
// xstokenizer.cpp
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xstokenizer.h"
#include "xscompiler.h"
#include "xsmessenger.h"


//==============================================================================
// BXSTokenizer::mScratchString
//==============================================================================
char BXSTokenizer::mScratchString[BXSTokenizer::cMaxTokenSize+1];


//==============================================================================
// BXSTokenizer::BXSTokenizer
//==============================================================================
BXSTokenizer::BXSTokenizer(BXSMessenger *messenger) :
   mFileID(-1),
   //mFilename doesn't need any ctor args.
   //mFilenameWithoutExtension doesn't need any ctor args.
   mMessenger(messenger),
   mSource(NULL),
   mSourceLength(0),
   mNumberLines(0),
   //mTokens doesn't need any ctor args.
   mCurrentToken(0),
   //mStartStates doesn't need any ctor args.
   //mInAName doesn't need any ctor args.
   mDebug(false),
   mCurrentLineNumber(-1)
{
   mFilename.set(B("None"));
   mFilenameWithoutExtension.set(B("None"));
}

//==============================================================================
// BXSTokenizer::~BXSTokenizer
//==============================================================================
BXSTokenizer::~BXSTokenizer(void)
{
}

//==============================================================================
// BXSTokenizer::initialize
//==============================================================================
void BXSTokenizer::initialize(void)
{
   //Names.
   for (long i=0 ; i < 256; i++)
   {
      if (isalpha(static_cast<unsigned char>(i)))
         mStartStates[i]=cNameStartState;
      else
         mStartStates[i]=cErrorState;
   }
   mStartStates['_']=cNameStartState;
   
   //Whitespace.
   mStartStates[' ']=cWhiteSpaceState;
   mStartStates['\t']=cWhiteSpaceState;
   mStartStates['\n']=cWhiteSpaceState;
   mStartStates['\r']=cWhiteSpaceState;

   //Subtraction (maybe integer)
   mStartStates['-']=cSubState;

   //Integer.
   mStartStates['0']=cIntegerState;
   mStartStates['1']=cIntegerState;
   mStartStates['2']=cIntegerState;
   mStartStates['3']=cIntegerState;
   mStartStates['4']=cIntegerState;
   mStartStates['5']=cIntegerState;
   mStartStates['6']=cIntegerState;
   mStartStates['7']=cIntegerState;
   mStartStates['8']=cIntegerState;
   mStartStates['9']=cIntegerState;

   //Start of a float.
   mStartStates['.']=cStartingPeriodState;
   //String.
   mStartStates['"']=cStringStartState;
   //EOS.
   mStartStates['\0']=cEOSState;
   //LParen.
   mStartStates['(']=cLParenState;
   // RParen
   mStartStates[')']=cRParenState;
   // < (maybe <=)
   mStartStates['<']=cLessState;
   // > (maybe >=)
   mStartStates['>']=cGreaterState;
   //=(maybe ==)
   mStartStates['=']=cEqualState;
   // ! (should be !=)
   mStartStates['!']=cBangState;
   // + 
   mStartStates['+']=cAddState;
   // * 
   mStartStates['*']=cMultState;
   // / (maybe comment) 
   mStartStates['/']=cDivState;
   // %  
   mStartStates['%']=cModState;
   // :
   mStartStates[':']=cColonState;
   // ;
   mStartStates[';']=cSemiColonState;
   // ,
   mStartStates[',']=cCommaState;
   // {
   mStartStates['{']=cLeftBraceState;
   // }
   mStartStates['}']=cRightBraceState;
   // &&
   mStartStates['&']=cAndState;
   // ||
   mStartStates['|']=cOrState;

   //Now setup the mInAName array.
   for (long i=0; i < 256; i++)
   {
      if (isdigit(static_cast<unsigned char>(i)))
         mInAName[i]=true;
      else if (isalpha(i))
         mInAName[i]=true;
      else if (i == '_')
         mInAName[i]=true;
      else
         mInAName[i]=false;
   }

}

//==============================================================================
// BXSTokenizer::setSource
//==============================================================================
bool BXSTokenizer::setSource(const BSimString &filename, const char *source, long sourceLength)
{
   //The NULL terminator requirement is causing problems.  I can't recall why I added
   //it, so I'm going to turn it off.  Everything seems to be fine with it off:)
   //if ((source == NULL) || (sourceLength <= 0) || (source[sourceLength-1] != '\0'))
   if ((source == NULL) || (sourceLength <= 0))
      return(false);
   
   //Copy the name.
   if (filename.length() > 0)
   {
      mFilename=filename;
      mFilenameWithoutExtension=filename;
      mFilenameWithoutExtension.removeExtension();
   }
   else
   {
      mFilename.set(B("None"));
      mFilenameWithoutExtension.set(B("None"));
   }

   //NOTE: We do not copy source since it's prolly pretty long.  Easy to change later
   //if need be.
   mSource=source;
   mSourceLength=sourceLength;
   mNumberLines=0;
   //Set the number of tokens back to 0.
   mTokens.setNumber(0);
   mCurrentToken=0;
   //Reset the current line number.
   mCurrentLineNumber=0;
   //Reset our file ID, too.
   mFileID=-1;
   return(true);
}

//==============================================================================
// BXSTokenizer::tokenize
//==============================================================================
bool BXSTokenizer::tokenize(void)
{
   //Bomb checks.
   if ((mSource == NULL) || (mSourceLength <= 0))
      return(false);

   //Set the number of tokens back to 0.
   mTokens.setNumber(0);
   mCurrentToken=0;

   //Setup the vars.
   mNumberLines=0;
   long sourcePosition=0;
   long state=cStartState;
   long token=cUnknown;
   long tokenStart=0;
   long tokenLength=0;
   long numberTokenDigits=0;

   //Go into the token loop.
   while (token != cEOS)
   {
      //Get the next two chars.  If we're past the end of the source buffer,
      //then we set them to the NULL terminator.
      char c;
      if (sourcePosition >= mSourceLength)
         c='\0';
      else
      {
         c=mSource[sourcePosition];
         
         // rg [11/13/04] - fix in case user uses weird chars in text file by accident (can happen when you copy & paste examples from Outlook)
         if (c < 0)
            c = '\0';
      }
      //Next char.
      char cN;
      if (sourcePosition >= mSourceLength-1)
         cN='\0';
      else
         cN=mSource[sourcePosition+1];

      //Update source pos.
      sourcePosition++;

      //Advance the line counter when we see a line feed.
      if (c == '\n')
         mNumberLines++;

      //If we're in the start state, get the next state based on the char we're reading.
      //Else, inc the current token length.
      if (state == cStartState)
      {
         state=mStartStates[c];
         //Remember the token start.
         if (state != cStartState)
         {
            tokenStart=sourcePosition-1;
            tokenLength=1;
         }
      }
      else
         tokenLength++;

      //Handle the state.
      switch(state)
      {
         case cStartState:
            break;
         case cErrorState:
            mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0001: unexpected character '%c'.", c); 
            token=cError;
            break;
         case cWhiteSpaceState:
            if (mStartStates[cN] != cWhiteSpaceState)
               token=cWhiteSpace;
            break;
         case cCPPCommentState:
            if (c == '\n')
               token=cWhiteSpace;
            break;
         case cCPPCommentState2:
            if ((c == '*') && (cN == '/'))
            {
               //Skip the /.
               sourcePosition++;
               tokenLength++;
               token=cWhiteSpace;
            }
            break;
         case cNameStartState:
            state=cNameInState;
            break;
         case cNameInState:
            if (!mInAName[c])
            {
               //Backup.
               sourcePosition--;
               tokenLength--;
               if (c == '\n')
                  mNumberLines--;
               if (tokenLength > cMaxTokenSize)
               {
                  mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0002: name is too long.");
                  tokenLength=cMaxTokenSize;
                  token=cError;
               }
               else
                  token=cName;
            }
            break;
         case cSubState:
            if (cN == '.')
               state=cStartingPeriodState;
            else if (cN == '-')
            {
               token=cMinusMinus;
               sourcePosition++;
               tokenLength++;
            }
            else if (isdigit(static_cast<unsigned char>(cN)))
            {
               //If the previous token is not an operand or a delimiter, then we have a sub.  Else,
               //we have a negative number.
               long lastToken=cUnknown;
               if (mTokens.getNumber() > 0)
                  lastToken=mTokens[mTokens.getNumber()-1].token;
               if ((lastToken == cName) ||
                  (lastToken == cRightParen) ||
                  (lastToken == cInteger) ||
                  (lastToken == cFloat) ||
                  (lastToken == cBool) ||
                  (lastToken == cVector) ||
                  (lastToken == cString))
                  token=cSub;
               else
                  state=cIntegerState;
            }
            else
               token=cSub;
            break;
         case cIntegerState:
            if (isdigit(c))
               numberTokenDigits++;
            else if (c == '.')
               state=cNextFractDigitState;
            else if (c == 'e' || c == 'E')
               state=cExpSignState;
            else
            {
               //Backup.
               sourcePosition--;
               tokenLength--;
               if (c == '\n')
                  mNumberLines--;
               //Check the length - we only allow 9 digits - TBD: do a better overflow check.
               if (numberTokenDigits > 9)
               {
                  mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0003: more than 9 digits in the integer constant.");
                  token=cError;
               }
               else
                  token=cInteger;
            }
            break;
         case cStartingPeriodState:
            state=cFirstFractDigitState;
            break;
         case cFirstFractDigitState:
            if (isdigit(c))
               state=cNextFractDigitState;
            else
            {
               //If the previous token is a name, this is a period.
               long lastToken=cUnknown;
               if (mTokens.getNumber() > 0)
                  lastToken=mTokens[mTokens.getNumber()-1].token;
               if (lastToken == cName)
               {
                  //Backup.
                  sourcePosition--;
                  tokenLength--;
                  if (c == '\n')
                     mNumberLines--;
                  token=cPeriod;
               }
               else
               {
                  mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0004: bad floating point constant or class specifier.");
                  token=cError;
               }
            }
            break;
         case cNextFractDigitState:
            if (isdigit(c))
               ;
            else if (c == 'e' || c == 'E')
               state=cExpSignState;
            else
            {
               //Backup.
               sourcePosition--;
               tokenLength--;
               if (c == '\n')
                  mNumberLines--;
               token=cFloat;
            }
            break;
         case cExpSignState:
            if (c == '+' || c == '-')
               ;
            else
            {
               //Backup.
               sourcePosition--;
               tokenLength--;
               if (c == '\n')
                  mNumberLines--;
            }
            //Look for exponent digits.
            state=cFirstExpDigitState;
            break;
         case cFirstExpDigitState:
            if (isdigit(c))
               state=cNextExpDigitState;
            else
            {
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0005: bad floating point constant - missing exponent digits.");
               token=cError;
            }
            break;
         case cNextExpDigitState:
            if (isdigit(c))
               ;
            else
            {
               //Backup.
               sourcePosition--;
               tokenLength--;
               if (c == '\n')
                  mNumberLines--;
               token=cFloat;
            }
            break;
         case cStringStartState:
            state=cStringInState;
            //Skip the starting double quote.
            tokenStart=sourcePosition;
            tokenLength--;
            break;
         case cStringInState:
            if ((c == '\\') && (cN == '"'))
            {
               //Consume the '\"' "character".
               sourcePosition++;
               tokenLength++;
            }
            //DCP TODO 11/10/01: This doesn't work.  Looks like it's going to have to
            //scan the token once it's "created" down below.
            else if ((c == '\\') && (cN == '\\'))
            {
               //Convert the '\\' "character" to a single '\'.
               sourcePosition++;
               tokenLength++;
            }
            else if (c == '"')
            {
               //Skip the ending double quote.
               tokenLength--;
               if (tokenLength > cMaxTokenSize)
               {
                  mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0006: string is too long.");
                  tokenLength=cMaxTokenSize;
                  token=cError;
               }
               else
                  token=cString;
            }
            else if (c == '\n')
            {
               //Backup
               sourcePosition--;
               tokenLength--;
               mNumberLines--;
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0007: line feed in string.");
               token=cError;
            }
            else if (c == '\0')
            {
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0008: string not terminated.");
               token=cError;
            }
            break;
         case cLParenState:
            token=cLeftParen;
            break;
         case cRParenState:
            token=cRightParen;
            break;
         case cLessState:
            if (cN == '=')
            {
               //Advance to consume the equal.
               sourcePosition++;
               tokenLength++;
               token=cLE;
            }
            else
               token=cLT;
            break;
         case cEqualState:
            if (cN == '=')
            {
               //Advance to consume the equal.
               sourcePosition++;
               tokenLength++;
               token=cEQ;
            }
            else
               token=cAssign;
            break;
         case cGreaterState:
            if (cN == '=')
            {
               //Advance to consume the equal.
               sourcePosition++;
               tokenLength++;
               token=cGE;
            }
            else
               token=cGT;
            break;
         case cBangState:
            if (cN == '=')
            {
               //Advance to consume the equal.
               sourcePosition++;
               tokenLength++;
               token=cNE;
            }
            else
            {
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0009: expected '=' after '!'.");
               token=cError;
            }
            break;
         case cAddState:
            if (cN == '+')
            {
               token=cPlusPlus;
               sourcePosition++;
               tokenLength++;
            }
            else
               token=cAdd;
            break;
         case cMultState:
            token=cMult;
            break;
         case cDivState:
            if (cN == '/')
            {
               //Advance to consume the slash.
               sourcePosition++;
               tokenLength++;
               state=cCPPCommentState;
            }
            else if (cN == '*')
            {
               //Advance to consume the slash.
               sourcePosition++;
               tokenLength++;
               state=cCPPCommentState2;
            }
            else
               token=cDiv;
            break;         
         case cModState:
            token=cMod;
            break;
         case cColonState:
            token=cColon;
            break;
         case cSemiColonState:
            token=cSemiColon;
            break;
         case cCommaState:
            token=cComma;
            break;
         case cLeftBraceState:
            token=cLeftBrace;
            break;
         case cRightBraceState:
            token=cRightBrace;
            break;
         case cEOSState:
            if (sourcePosition != mSourceLength)
            {
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0010: end of string character found before buffer end.");
               token=cError;
            }
            else
               token=cEOS;
            break;
         case cAndState:
            if (cN == '&')
            {
               token=cAnd;
               sourcePosition++;
               tokenLength++;
            }
            else
               token=cBitAnd;
            break;
         case cOrState:
            if (cN == '|')
            {
               token=cOr;
               sourcePosition++;
               tokenLength++;
            }
            else
               token=cBitOr;
            break;
         default:
            BASSERT(0);
            mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0011: unexpected state=%d.", state);
            break;
      }

      //If we have a known, non-whitespace token, add it.
      if (token != cUnknown)
      {
         if (token != cWhiteSpace)
         {
            long nT=mTokens.getNumber();
            if (mTokens.setNumber(nT+1) == false)
            {
               mMessenger->sourceErrorMsg(mFilename.getPtr(), mNumberLines+1, "Token Error 0012: cannot add token #%d.", nT);
               return(false);
            }

            //If we have the "true" or "false" cName token, convert that to a cBool token or a cVector.
            if (token == cName)
            {
               StringCchCopyNExA(mScratchString, sizeof(mScratchString), mSource+tokenStart, tokenLength, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
               mScratchString[tokenLength]='\0';
               if ((stricmp("true", mScratchString) == 0) ||
                  (stricmp("false", mScratchString) == 0) ||
                  (stricmp("bool", mScratchString) == 0))
                  token=cBool;
               else if (stricmp("vector", mScratchString) == 0)
                  token=cVector;
               else if (stricmp("include", mScratchString) == 0)
                  token=cInclude;
               else if (stricmp("switch", mScratchString) == 0)
                  token=cSwitch;
               else if (stricmp("case", mScratchString) == 0)
                  token=cCase;
               else if (stricmp("while", mScratchString) == 0)
                  token=cWhile;
               else if (stricmp("break", mScratchString) == 0)
                  token=cBreak;
               else if (stricmp("default", mScratchString) == 0)
                  token=cDefault;
               else if (stricmp("rule", mScratchString) == 0)
                  token=cRule;
               else if (stricmp("if", mScratchString) == 0)
                  token=cIf;
               else if (stricmp("then", mScratchString) == 0)
                  token=cThen;
               else if (stricmp("else", mScratchString) == 0)
                  token=cElse;
               else if (stricmp("goto", mScratchString) == 0)
                  token=cGoto;
               else if (stricmp("label", mScratchString) == 0)
                  token=cLabel;
               else if (stricmp("for", mScratchString) == 0)
                  token=cFor;
               else if (stricmp("dbg", mScratchString) == 0)
                  token=cDBG;
               else if (stricmp("return", mScratchString) == 0)
                  token=cReturn;
               else if (stricmp("void", mScratchString) == 0)
                  token=cVoid;
               else if (stricmp("int", mScratchString) == 0)
                  token=cInteger;
               else if (stricmp("float", mScratchString) == 0)
                  token=cFloat;
               else if (stricmp("string", mScratchString) == 0)
                  token=cString;
               else if (stricmp("const", mScratchString) == 0)
                  token=cConst;
               else if (stricmp("priority", mScratchString) == 0)
                  token=cPriority;
               else if (stricmp("minInterval", mScratchString) == 0)
                  token=cMinInterval;
               else if (stricmp("maxInterval", mScratchString) == 0)
                  token=cMaxInterval;
               else if (stricmp("highFrequency", mScratchString) == 0)
                  token=cHighFrequency;
               else if (stricmp("active", mScratchString) == 0)
                  token=cActive;
               else if (stricmp("inactive", mScratchString) == 0)
                  token=cInactive;
               else if (stricmp("highFrequency", mScratchString) == 0)
                  token=cHighFrequency;
               else if (stricmp("group", mScratchString) == 0)
                  token=cGroup;
               else if (stricmp("infiniteLoopLimit", mScratchString) == 0)
                  token=cILL;
               else if (stricmp("infiniteRecursionLimit", mScratchString) == 0)
                  token=cIRL;
               else if (stricmp("breakpoint", mScratchString) == 0)
                  token=cBreakpoint;
               else if (stricmp("static", mScratchString) == 0)
                  token=cStatic;
               else if (stricmp("continue", mScratchString) == 0)
                  token=cContinue;
               else if (stricmp("extern", mScratchString) == 0)
                  token=cExtern;
               else if (stricmp("export", mScratchString) == 0)
                  token=cExport;
               else if (stricmp("runImmediately", mScratchString) == 0)
                  token=cRunImmediately;
               else if (stricmp("mutable", mScratchString) == 0)
                  token=cMutable;
               else if (stricmp("class", mScratchString) == 0)
                  token=cClass;
            }

            mTokens[nT].token=token;
            mTokens[nT].start=tokenStart;
            mTokens[nT].length=tokenLength;
            mTokens[nT].lineNumber=mNumberLines;
            #ifdef _DEBUG
            if (tokenLength > cMaxTokenSize)
            {
               StringCchCopyNExA(mTokens[nT].string, sizeof(mTokens[nT].string), mSource+tokenStart, cMaxTokenSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
               mTokens[nT].string[cMaxTokenSize]='\0';
            }
            else
            {
               StringCchCopyNExA(mTokens[nT].string, sizeof(mTokens[nT].string), mSource+tokenStart, tokenLength, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
               mTokens[nT].string[tokenLength]='\0';
            }
            #endif

            //Debug output.
            if (mDebug == true)
            {
               char outputText[256];
               if (token != cWhiteSpace)
               {
                  char foo[cMaxTokenSize+1];
                  StringCchCopyNExA(foo, cMaxTokenSize+1, mSource+tokenStart, tokenLength, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
                  foo[tokenLength]='\0';
                  bsnprintf(outputText, sizeof(outputText), "'%s', Type=%s (#%d).\n", foo, getTokenTypeName(token), token);
               }
               else
                  bsnprintf(outputText, sizeof(outputText), "WhiteSpace (#%d).\n", token);
               
               OutputDebugStringA(outputText);
            }
         }

         //If we don't have an EOS token, go back to the start of finding a token.
         if (token != cEOS)
         {
            state=cStartState;
            token=cUnknown;
            tokenLength=0;
            tokenStart=0;
            numberTokenDigits=0;
         }
      }

      //DCP/RF:  Catch-all failure.
      if (sourcePosition >= mSourceLength)
         break;
   }

   return(true);
}

//==============================================================================
// BXSTokenizer::getToken
//==============================================================================
long BXSTokenizer::getToken(BXSCompiler *compiler)
{
   mCurrentToken++;
   if ((mCurrentToken < -1) || (mCurrentToken >= mTokens.getNumber()) )
      return(cUnknown);

   //If the compiler is doing debug code, spit out the line number if it's new.
   if (compiler->getDebugCode() == true)
   {
      if (mTokens[mCurrentToken].lineNumber != mCurrentLineNumber)
      {
         mCurrentLineNumber=mTokens[mCurrentToken].lineNumber;
         compiler->addLineNumberQuad(mCurrentLineNumber);
      }
   }

   //If we're in a debug build, set the scratch text for debugging help.
   #ifdef _DEBUG
   getCurrentTokenText();
   #endif
   return(mTokens[mCurrentToken].token);
}

//==============================================================================
// BXSTokenizer::getCurrentToken
//==============================================================================
long BXSTokenizer::getCurrentToken(void)
{
   if ((mCurrentToken < 0) || (mCurrentToken >= mTokens.getNumber()) )
      return(cUnknown);
   //If we're in a debug build, set the scratch text for debugging help.
   #ifdef _DEBUG
   getCurrentTokenText();
   #endif
   return(mTokens[mCurrentToken].token);
}

//==============================================================================
// BXSTokenizer::getFutureToken
//==============================================================================
long BXSTokenizer::getFutureToken(long numberTokensAhead, bool countWhiteSpace)
{
   long count=0;
   for (long i=mCurrentToken; i < mTokens.getNumber(); i++)
   {
      if ((mTokens[i].token != cWhiteSpace) || (countWhiteSpace == true))
      {
         count++;
         if (count >= numberTokensAhead)
            return(mTokens[i].token);
      }
   }
   return(cEOS);
}

//==============================================================================
// BXSTokenizer::getDarkSpaceToken
//==============================================================================
long BXSTokenizer::getDarkSpaceToken(BXSCompiler *compiler)
{
   for (long i=mCurrentToken+1; i < mTokens.getNumber(); i++)
   {
      if (mTokens[i].token != cWhiteSpace)
      {
         mCurrentToken=i;

         //If the compiler is doing debug code, spit out the line number if it's new.
         if (compiler->getDebugCode() == true)
         {
            if (mTokens[mCurrentToken].lineNumber != mCurrentLineNumber)
            {
               mCurrentLineNumber=mTokens[mCurrentToken].lineNumber;
               compiler->addLineNumberQuad(mCurrentLineNumber);
            }
         }

         //If we're in a debug build, set the scratch text for debugging help.
         #ifdef _DEBUG
         getCurrentTokenText();
         #endif
         return(mTokens[i].token);
      }
   }
   return(cUnknown);
}

//==============================================================================
// BXSTokenizer::decrementCurrentToken
//==============================================================================
void BXSTokenizer::decrementCurrentToken(BXSCompiler * /*compiler*/)
{
   mCurrentToken--;
   if (mCurrentToken < 0)
      mCurrentToken=0;

   //DCP 06/02/01: This is screwing stuff up.  Once we go to a line, there really
   //isn't a reason to go backwards.  This is just confusing the LINE opcode output
   //by putting bogus switches in, so I'll just take it out.
   //If the compiler is doing debug code, spit out the line number if it's new.
   /*if (compiler->getDebugCode() == true)
   {
      if (mTokens[mCurrentToken].lineNumber != mCurrentLineNumber)
      {
         mCurrentLineNumber=mTokens[mCurrentToken].lineNumber;
         compiler->addLineNumberQuad(mCurrentLineNumber);
      }
   }*/

   //If we're in a debug build, set the scratch text for debugging help.
   #ifdef _DEBUG
   getCurrentTokenText();
   #endif
}

//==============================================================================
// BXSTokenizer::incrementCurrentToken
//==============================================================================
void BXSTokenizer::incrementCurrentToken(BXSCompiler *compiler)
{
   mCurrentToken++;
   if (mCurrentToken >= mTokens.getNumber())
      mCurrentToken=mTokens.getNumber()-1;

   //If the compiler is doing debug code, spit out the line number if it's new.
   if (compiler->getDebugCode() == true)
   {
      if (mTokens[mCurrentToken].lineNumber != mCurrentLineNumber)
      {
         mCurrentLineNumber=mTokens[mCurrentToken].lineNumber;
         compiler->addLineNumberQuad(mCurrentLineNumber);
      }
   }

   //If we're in a debug build, set the scratch text for debugging help.
   #ifdef _DEBUG
   getCurrentTokenText();
   #endif
}

//==============================================================================
// BXSTokenizer::getCurrentTokenText
//==============================================================================
char* BXSTokenizer::getCurrentTokenText(void)
{
   if ((mCurrentToken < 0) || (mCurrentToken >= mTokens.getNumber()) )
      return(NULL);
   //strncpy(mScratchString, mSource+mTokens[mCurrentToken].start, mTokens[mCurrentToken].length);
   //mScratchString[mTokens[mCurrentToken].length]='\0';

   //Copy the token.  If we have a string, we have to strip down the '\"' chars.
   if (mTokens[mCurrentToken].token != cString)
   {
      StringCchCopyNExA(mScratchString, sizeof(mScratchString), mSource+mTokens[mCurrentToken].start, mTokens[mCurrentToken].length, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
      mScratchString[mTokens[mCurrentToken].length]='\0';
   }
   else
   {
      long cp=0;
      for (long i=mTokens[mCurrentToken].start; i < mTokens[mCurrentToken].start+mTokens[mCurrentToken].length; )
      {
         if ((mSource[i] == '\\') && (mSource[i+1] == '"'))
         {
            mScratchString[cp]='"';
            cp++;
            i+=2;
         }
         else
         {
            mScratchString[cp]=mSource[i];
            cp++;
            i++;
         }
      }

      mScratchString[cp]='\0';
   }
   return(mScratchString);
}

//==============================================================================
// BXSTokenizer::getCurrentTokenText
//==============================================================================
bool BXSTokenizer::getCurrentTokenText(char **text, long maxLength)
{
   if ((mCurrentToken < 0) || (mCurrentToken >= mTokens.getNumber()) || (text == NULL) || (maxLength <= 0))
      return(false);
   //strncpy(*text, mSource+mTokens[mCurrentToken].start, mTokens[mCurrentToken].length);
   //*text[mTokens[mCurrentToken].length]='\0';

   //Copy the token.  If we have a string, we have to strip down the '\"' chars.
   if (mTokens[mCurrentToken].token != cString)
   {
      StringCchCopyNExA(*text, maxLength, mSource+mTokens[mCurrentToken].start, mTokens[mCurrentToken].length, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
      *text[mTokens[mCurrentToken].length]='\0';
   }
   else
   {
      long cp=0;
      for (long i=mTokens[mCurrentToken].start; (i < mTokens[mCurrentToken].start+mTokens[mCurrentToken].length) && (cp < maxLength); )
      {
         if ((mSource[i] == '\\') && (mSource[i+1] == '"'))
         {
            *text[cp]='"';
            cp++;
            i+=2;
         }
         else
         {
            *text[cp]=mSource[i];
            cp++;
            i++;
         }
      }

      *text[cp]='\0';
   }

   return(true);
}

//==============================================================================
// BXSTokenizer::getCurrentTokenLineNumber
//==============================================================================
long BXSTokenizer::getCurrentTokenLineNumber(void) const
{
   if ((mCurrentToken < 0) || (mCurrentToken >= mTokens.getNumber()) )
      return(-1);
   return(mTokens[mCurrentToken].lineNumber+1);
}

//==============================================================================
// BXSTokenizer::getTokenTypeName
//==============================================================================
const char* BXSTokenizer::getTokenTypeName(long token)
{
   static char sUnknown[]="Unknown";
   static char sError[]="Error";
   static char sInteger[]="Int";
   static char sName[]="Name";
   static char sString[]="String";
   static char sLeftParen[]="LeftParen";
   static char sRightParen[]="RightParen";
   static char sLT[]="LT";
   static char sLE[]="LE";
   static char sEQ[]="EQ";
   static char sNE[]="NE";
   static char sGE[]="GE";
   static char sGT[]="GT";
   static char sAdd[]="Add";
   static char sSub[]="Sub";
   static char sMult[]="Mult";
   static char sDiv[]="Div";
   static char sMod[]="Mod";
   static char sAssign[]="Assign";
   static char sPlusPlus[]="++";
   static char sMinusMinus[]="--";
   static char sColon[]="Colon";
   static char sSemiColon[]="SemiColon";
   static char sComma[]="Comma";
   static char sLeftBrace[]="LeftBrace";
   static char sRightBrace[]="RightBrace";
   static char sPeriod[]="Period";
   static char sWS[]="WS";
   static char sAnd[]="AND";
   static char sOr[]="OR";
   static char sBitAnd[]="BitAND";
   static char sBitOr[]="BitOR";
   static char sBool[]="Bool";
   static char sEOS[]="EOS";

   switch (token)
   {
      case cUnknown:
         return(sUnknown);
      case cError:
         return(sError);
      case cInteger:
         return(sInteger);
      case cName:
         return(sName);
      case cString:
         return(sString);
      case cLeftParen:
         return(sLeftParen);
      case cRightParen:
         return(sRightParen);
      case cLT:
         return(sLT);
      case cLE:
         return(sLE);
      case cEQ:
         return(sEQ);
      case cNE:
         return(sNE);
      case cGE:
         return(sGE);
      case cGT:
         return(sGT);
      case cAdd:
         return(sAdd);
      case cSub:
         return(sSub);
      case cMult:
         return(sMult);
      case cDiv:
         return(sDiv);
      case cMod:
         return(sMod);
      case cAssign:
         return(sAssign);
      case cPlusPlus:
         return(sPlusPlus);
      case cMinusMinus:
         return(sMinusMinus);
      case cColon:
         return(sColon);
      case cSemiColon:
         return(sSemiColon);
      case cComma:
         return(sComma);
      case cLeftBrace:
         return(sLeftBrace);
      case cRightBrace:
         return(sRightBrace);
      case cPeriod:
         return(sPeriod);
      case cWhiteSpace:
         return(sWS);
      case cAnd:
         return(sAnd);
      case cOr:
         return(sOr);
      case cBitAnd:
         return(sBitAnd);
      case cBitOr:
         return(sBitOr);
      case cBool:
         return(sBool);
      case cEOS:
         return(sEOS);
   }
   
   return (sUnknown);
}
