//==============================================================================
// xsemitter.cpp
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsemitter.h"
#include "xscompiler.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsmessenger.h"
#include "xsopcodes.h"
#include "xssource.h"

//==============================================================================
//#define DEBUGLISTHEXINFO


//==============================================================================
// Defines
char BXSEmitter::mScratchString[132];

//==============================================================================
// BXSEmitter::BXSEmitter
//==============================================================================
BXSEmitter::BXSEmitter(BXSMessenger *messenger) :
   mMessenger(messenger),
   mListFile(NULL)
{
}

//==============================================================================
// BXSEmitter::~BXSEmitter
//==============================================================================
BXSEmitter::~BXSEmitter(void)
{
   clear();
}

//==============================================================================
// BXSEmitter::emitQuads
//==============================================================================
bool BXSEmitter::emitQuads(BXSQuadArray &quads, BXSSource *source,
   long *offset, BXSCompiler *compiler)
{
   //Initialize the offset.
   *offset=-1;

   //Bail if we don't have anything to output.
   if (quads.getNumber() <= 0)
      return(true);

   //Save the starting count.
   long startCount=source->getCodeSize();
   if (emitTheBytes(quads, source, compiler) == false)
      return(false);

   //If any bytes were output set the offset to the start.
   long endCount=source->getCodeSize();
   if (endCount > startCount)
      *offset=startCount;

   return(true);   
}

//==============================================================================
// BXSEmitter::getNumberBytesEmitted
//==============================================================================
long BXSEmitter::getNumberBytesEmitted(long opcode)
{
   //NOTE: These numbers are hand-counted based on what the emitter actually emits
   //in the 'emitTheBytes' method below.  They obviously have to stay synced up.

   switch (opcode)
   {
      case BXSQuadOpcode::cNOP:
         return(0);

      case BXSQuadOpcode::cRET:
         return(1);

      case BXSQuadOpcode::cJUMP:
         return(6);
      case BXSQuadOpcode::cJUMPZ:
      case BXSQuadOpcode::cJUMPNZ:
         return(6);
      case BXSQuadOpcode::cLABEL:
         return(1);

      case BXSQuadOpcode::cPUSH:
      case BXSQuadOpcode::cPUSHADD:
         return(5);
      case BXSQuadOpcode::cPUSHI:
         return(6);
      case BXSQuadOpcode::cPOP:
      case BXSQuadOpcode::cPOPADD:
         return(2);
      case BXSQuadOpcode::cCALLS:
      case BXSQuadOpcode::cCALLF:
         return(3);

      case BXSQuadOpcode::cNOT:
      case BXSQuadOpcode::cAND:
      case BXSQuadOpcode::cOR:
         return(1);

      case BXSQuadOpcode::cGT:
      case BXSQuadOpcode::cGE:
      case BXSQuadOpcode::cNE:
      case BXSQuadOpcode::cEQ:
      case BXSQuadOpcode::cLE:
      case BXSQuadOpcode::cLT:
         return(1);

      case BXSQuadOpcode::cNEG:
      case BXSQuadOpcode::cADD:
      case BXSQuadOpcode::cSUB:
      case BXSQuadOpcode::cMUL:
      case BXSQuadOpcode::cDIV:
      case BXSQuadOpcode::cMOD:
      case BXSQuadOpcode::cASS:
         return(1);

      case BXSQuadOpcode::cDBG:
         return(1);
      case BXSQuadOpcode::cSEP:
         return(0);
      case BXSQuadOpcode::cILL:
         return(5);
      case BXSQuadOpcode::cIRL:
         return(5);
      case BXSQuadOpcode::cLINE:
         return(5);
      case BXSQuadOpcode::cFILE:
         return(5);
      case BXSQuadOpcode::cBPNT:
         return(1);

      default:
         BASSERT(0);
         return(0);
   }
}

//==============================================================================
// BXSEmitter::backpatchLong
//==============================================================================
bool BXSEmitter::backpatchLong(long position, long value, BXSSource *source)
{
   return(source->overwriteCode(position, (SBYTE*)&value, sizeof(long)) );
}

//==============================================================================
// BXSEmitter::addListLine
//==============================================================================
void BXSEmitter::addListLine(bool lineFeed, const char *output, ...)
{
   if (mListFile == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, output);
   char temp1[1024];
   bvsnprintf(temp1, sizeof(temp1), output, args);
   va_end(args);

   if (lineFeed == true)
      mListFile->fprintf("%s\n", temp1);
   else
      mListFile->fprintf("%s", temp1);
}

//==============================================================================
// BXSEmitter::addListLine2
//==============================================================================
void BXSEmitter::addListLine2(bool lineFeed, const char *output, ...)
{
   if (mListFile == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, output);
   char temp1[1024];
   bvsnprintf(temp1, sizeof(temp1), output, args);
   va_end(args);

   if (lineFeed == true)
      mListFile->fprintf("%-40s\n", temp1);
   else
      mListFile->fprintf("%-40s", temp1);
}

//==============================================================================
// BXSEmitter::clear
//==============================================================================
void BXSEmitter::clear(void)
{
   mListFile=NULL;
}

//==============================================================================
// BXSEmitter::emitTheBytes
//==============================================================================
bool BXSEmitter::emitTheBytes(BXSQuadArray &quads, BXSSource *source, BXSCompiler *compiler)
{
   //NOTE: The output bytes from this method are depended on by 'getNumberBytesEmitted'.  If
   //a change is made to the output format, it needs to be reflected in that function, too.

   //Initialize the listing string.
   mListingCodeBytes[0]='\0';

   //Go into the big output loop.
   for (long i=0; i < quads.getNumber(); i++)
   {
      //Extract the info.
      long opcode=quads[i].mOpcode;
      long f1=quads[i].mF1;
      long f2=quads[i].mF2;
      long f3=quads[i].mF3;

      //Skip the NOP.
      if (opcode == BXSQuadOpcode::cNOP)
         continue;
      //Do the SEP processing.
      if (opcode == BXSQuadOpcode::cSEP)
      {
         addListLine(true, "");
         continue;
      }

      //Initialize the listing string.
      mListingCodeBytes[0]='\0';

      //Emit the opcode.
      if (emit1BYTE(opcode, source) == false)
         return(false);
   
      //Emit the operands if any.
      switch (opcode)
      {
         //RETURN opcode.
         case BXSQuadOpcode::cRET:
            //Debug.
            addListLine2(false, "%-8s ", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%-8s ", BXSQuadOpcode::getName(opcode));
            break;

         //JUMP and LABEL opcodes.
         case BXSQuadOpcode::cJUMP:
            //Debug.
            addListLine2(false, "%-8s Check=%d, %s", BXSQuadOpcode::getName(opcode), f1, compiler->getLabelName(f2));
            addListLine(true, "//%s, %d, %d.", BXSQuadOpcode::getName(opcode), f1, f2);
            //Emit whether or not this is a jump that can/should be checked for infinite loops.  We don't
            //do that check for the JUMPZ or JUMPNZ because those aren't used by gotos or at the bottom of
            //a for or a while loop to bounce it back to the top.
            if (emit1BYTE(f1, source) == false)
               return(false);
            //Emit the jump label.
            if (emit4BYTE(f2, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cJUMPZ:
         case BXSQuadOpcode::cJUMPNZ:
            //Debug.
            addListLine2(false, "%-8s Pop=%d, %s", BXSQuadOpcode::getName(opcode), f1, compiler->getLabelName(f2));
            addListLine(true, "//%s, %d, %d.", BXSQuadOpcode::getName(opcode), f1, f2);
            //Emit the pop stack tag.
            if (emit1BYTE(f1, source) == false)
               return(false);
            //Emit the jump label.
            if (emit4BYTE(f2, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cLABEL:
            //Debug.
            addListLine2(false, "%-8s %s", BXSQuadOpcode::getName(opcode), compiler->getLabelName(f1));
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //The position that we're at now is the location for this label.
            //It needs to be saved.  F1 is the compiler's ID for this label.
            compiler->setLabelPosition(f1, source->getCodeSize());
            break;

         case BXSQuadOpcode::cPUSH:
            //Debug.
            addListLine2(false, "%-8s %s.%d", BXSQuadOpcode::getName(opcode), compiler->getVariableName(f1), f2);
            addListLine(true, "//%s, %d, %d.", BXSQuadOpcode::getName(opcode), f1, f2);
            //Emit the variable ID.
            if (emit2BYTE(f1, source) == false)
               return(false);
            //Emit the offset.
            if (emit2BYTE(f2, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cPUSHADD:
            //Debug.
            addListLine2(false, "%-8s %s.%d", BXSQuadOpcode::getName(opcode), compiler->getVariableName(f1), f2);
            addListLine(true, "//%s, %d, %d.", BXSQuadOpcode::getName(opcode), f1, f2);
            //Emit the variable ID.
            if (emit2BYTE(f1, source) == false)
               return(false);
            //Emit the offset.
            if (emit2BYTE(f2, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cPUSHI:
            //Debug.
            if ((f1 == BXSVariableEntry::cIntegerVariable) || (f1 == BXSVariableEntry::cBoolVariable))
            {
               addListLine2(false, "%-8s %s, %d", BXSQuadOpcode::getName(opcode), compiler->getVariableTypeName(f1), f2);
               addListLine(true, "//%s, %d, %d.", BXSQuadOpcode::getName(opcode), f1, f2);
            }
            else
            {
               float v=*(float*)(&f2);
               addListLine2(false, "%-8s %s, %f", BXSQuadOpcode::getName(opcode), compiler->getVariableTypeName(f1), v);
               addListLine(true, "//%s, %d, %f.", BXSQuadOpcode::getName(opcode), f1, v);
            }
            //Emit the variable type.
            if (emit1BYTE(f1, source) == false)
               return(false);
            //Emit the value.
            if (emit4BYTE(f2, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cPOP:
            //Debug.
            addListLine2(false, "%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the number of things to pop.
            if (emit1BYTE(f1, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cPOPADD:
            //Debug.
            addListLine2(false, "%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the number of things to pop.
            if (emit1BYTE(f1, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cCALLS:
            //Debug.
            addListLine2(false, "%-8s %s", BXSQuadOpcode::getName(opcode), compiler->getSyscallName(f1));
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the syscall ID.
            if (emit2BYTE(f1, source) == false)
               return(false);
            break;
         case BXSQuadOpcode::cCALLF:
            //Debug.
            addListLine2(false, "%-8s %s", BXSQuadOpcode::getName(opcode), compiler->getFunctionName(f1));
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the function ID.
            if (emit2BYTE(f1, source) == false)
               return(false);
            break;

         case BXSQuadOpcode::cNOT:
         case BXSQuadOpcode::cAND:
         case BXSQuadOpcode::cOR:
            //Debug.
            addListLine2(false, "%-8s", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%s.", BXSQuadOpcode::getName(opcode));
            break;
      
         case BXSQuadOpcode::cGT:
         case BXSQuadOpcode::cGE:
         case BXSQuadOpcode::cEQ:
         case BXSQuadOpcode::cNE:
         case BXSQuadOpcode::cLE:
         case BXSQuadOpcode::cLT:
            //Debug.
            addListLine2(false, "%-8s", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%s.", BXSQuadOpcode::getName(opcode));
            break;
      
         case BXSQuadOpcode::cNEG:
         case BXSQuadOpcode::cADD:
         case BXSQuadOpcode::cSUB:
         case BXSQuadOpcode::cMUL:
         case BXSQuadOpcode::cDIV:
         case BXSQuadOpcode::cMOD:
         case BXSQuadOpcode::cASS:
            //Debug.
            addListLine2(false, "%-8s", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%s.", BXSQuadOpcode::getName(opcode));
            break;

         case BXSQuadOpcode::cDBG:
            //Debug.
            addListLine2(false, "%-8s", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%s.", BXSQuadOpcode::getName(opcode));
            break;

         case BXSQuadOpcode::cILL:
            //Debug.
            addListLine2(false, "%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the infinite loop limit.
            if (emit4BYTE(f1, source) == false)
               return(false);
            break;

         case BXSQuadOpcode::cIRL:
            //Debug.
            addListLine2(false, "%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            addListLine(true, "//%s, %d.", BXSQuadOpcode::getName(opcode), f1);
            //Emit the infinite recursion limit.
            if (emit4BYTE(f1, source) == false)
               return(false);
            break;

         case BXSQuadOpcode::cLINE:
            //Debug.
            addListLine2(true, "//%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            //Emit the line number.
            if (emit4BYTE(f1, source) == false)
               return(false);
            break;

         case BXSQuadOpcode::cFILE:
            //Debug.
            addListLine2(true, "//%-8s %d", BXSQuadOpcode::getName(opcode), f1);
            //Emit the file number.
            if (emit4BYTE(f1, source) == false)
               return(false);
            break;

         case BXSQuadOpcode::cBPNT:
            //Debug.
            addListLine2(false, "%-8s ", BXSQuadOpcode::getName(opcode));
            addListLine(true, "//%-8s ", BXSQuadOpcode::getName(opcode));
            break;

         default:
            mMessenger->errorMsg("Error - unknown opcode: %d    %d, %d, %d.", opcode, f1, f2, f3);
            return(false);
      }

      #ifdef DEBUGLISTHEXINFO
      if ((mListFile != NULL) && (code.getNumber() > startPosition))
         mListFile->fprintf("\t\t// %s\n", mListingCodeBytes);
      #endif
   }

   return(true);
}

//==============================================================================
// BXSEmitter::emit1BYTE
//==============================================================================
bool BXSEmitter::emit1BYTE(long value, BXSSource *source)
{
   #ifdef DEBUGLISTHEXINFO
   long startCount=code.getNumber();
   #endif

   if ((value > 127) || (value < -128))
   {
      mMessenger->errorMsg("Error - out of range problem adding one byte to the source array.");
      return(false);
   }

   BYTE byteVal=(BYTE)value;
   if ((source->addCode((SBYTE*)&byteVal, 1) == false))
   {
      mMessenger->errorMsg("Error - problem adding one byte to the source array.");
      return(false);
   }

   #ifdef DEBUGLISTHEXINFO
   if (mListFile != NULL)
   {
      BYTE foo=code[startCount];
      sprintf(mScratchString,"%02x ", foo);
      strcat(mListingCodeBytes, mScratchString);          
   }
   #endif
   return(true);
}

//==============================================================================
// BXSEmitter::emit2BYTE
//==============================================================================
bool BXSEmitter::emit2BYTE(long value, BXSSource *source)
{
   #ifdef DEBUGLISTHEXINFO
   long startCount=code.getNumber();
   #endif

   if ((value > 32767) || (value < -32768))
   {
      mMessenger->errorMsg("Error - problem adding two bytes to the source array.");
      return(false);
   }

   WORD wordVal=(WORD)value; 
   if ((source->addCode((SBYTE*)&wordVal, 2) == false))
   {
      mMessenger->errorMsg("Error - problem adding two bytes to the source array.");
      return(false);
   }

   #ifdef DEBUGLISTHEXINFO
   if (mListFile != NULL)
   {
      BYTE foo=code[startCount];
      sprintf(mScratchString,"%02x", foo);
      strcat(mListingCodeBytes, mScratchString);          
      foo=code[startCount+1];
      sprintf(mScratchString,"%02x ", foo);
      strcat(mListingCodeBytes, mScratchString);          
   }
   #endif
   return(true);   
}

//==============================================================================
// BXSEmitter::emit4BYTE
//==============================================================================
bool BXSEmitter::emit4BYTE(long value, BXSSource *source)
{
   #ifdef DEBUGLISTHEXINFO
   long startCount=code.getNumber();
   #endif

   if (source->addCode((SBYTE *)&value, 4) == false)
   {
      mMessenger->errorMsg("Error - problem adding four bytes to the source array.");
      return(false);
   }

   #ifdef DEBUGLISTHEXINFO
   if (mListFile != NULL)
   {
      // to list the bytes - we cast them as unsigned
      BYTE foo=code[startCount];
      sprintf(mScratchString,"%02x", foo);
      strcat(mListingCodeBytes, mScratchString);          
      foo=code[startCount+1];
      sprintf(mScratchString,"%02x", foo);
      strcat(mListingCodeBytes, mScratchString);          
      foo=code[startCount+2];
      sprintf(mScratchString,"%02x", foo);
      strcat(mListingCodeBytes, mScratchString);          
      foo=code[startCount+3];
      sprintf(mScratchString,"%02x ", foo);
      strcat(mListingCodeBytes, mScratchString);          
   }
   #endif
   return(true);   
}
