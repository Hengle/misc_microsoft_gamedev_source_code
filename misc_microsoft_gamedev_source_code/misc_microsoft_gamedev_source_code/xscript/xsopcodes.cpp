//==============================================================================
// xsopcodes.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes.
#include "xscript.h"
#include "xsopcodes.h"

//==============================================================================
char* BXSQuadOpcode::getName(long opcode)
{
   switch (opcode)
   {
      case BXSQuadOpcode::cNOP:     return("NOP");
      case BXSQuadOpcode::cRET:     return("RET");
      case BXSQuadOpcode::cJUMP:    return("JUMP");
      case BXSQuadOpcode::cJUMPZ:   return("JUMPZ");
      case BXSQuadOpcode::cJUMPNZ:  return("JUMPNZ");
      case BXSQuadOpcode::cLABEL:   return("LABEL");
      case BXSQuadOpcode::cPUSH:    return("PUSH");
      case BXSQuadOpcode::cPUSHI:   return("PUSHI");
      case BXSQuadOpcode::cPUSHADD: return("PUSHADD");
      case BXSQuadOpcode::cPOP:     return("POP");
      case BXSQuadOpcode::cPOPADD:  return("POPADD");
      case BXSQuadOpcode::cCALLS:   return("CALLS");
      case BXSQuadOpcode::cCALLF:   return("CALLF");
      case BXSQuadOpcode::cNOT:     return("NOT");
      case BXSQuadOpcode::cAND:     return("AND");
      case BXSQuadOpcode::cOR:      return("OR");
      case BXSQuadOpcode::cGT:      return("GT");
      case BXSQuadOpcode::cGE:      return("GE");
      case BXSQuadOpcode::cNE:      return("NE");
      case BXSQuadOpcode::cEQ:      return("EQ");
      case BXSQuadOpcode::cLE:      return("LE");
      case BXSQuadOpcode::cLT:      return("LT");
      case BXSQuadOpcode::cNEG:     return("NEG");
      case BXSQuadOpcode::cADD:     return("ADD");
      case BXSQuadOpcode::cSUB:     return("SUB");
      case BXSQuadOpcode::cMUL:     return("MUL");
      case BXSQuadOpcode::cDIV:     return("DIV");
      case BXSQuadOpcode::cMOD:     return("MOD");
      case BXSQuadOpcode::cASS:     return("ASS");

      case BXSQuadOpcode::cDBG:     return("DBG");
      case BXSQuadOpcode::cSEP:     return("SEP");
      case BXSQuadOpcode::cILL:     return("ILL");
      case BXSQuadOpcode::cIRL:     return("IRL");
      case BXSQuadOpcode::cLINE:    return("LINE");
      case BXSQuadOpcode::cFILE:    return("FILE");
      case BXSQuadOpcode::cBPNT:    return("BPNT");
   }

   return("???");
}

//==============================================================================
long BXSQuadOpcode::getPrecedence(long opcode)
{
   //Precedence rules:
   //   /, *, %
   //   +, -
   //   >, <, >=, <=
   //   ==, !=
   //   &&
   //   ||
   //   =
   //  So, we'll return 0 for '=' and go up one for each level above it.

   switch (opcode)
   {
      case BXSQuadOpcode::cASS:
         return(0);
      case BXSQuadOpcode::cOR:
         return(1);
      case BXSQuadOpcode::cAND:
         return(2);
      case BXSQuadOpcode::cEQ:
      case BXSQuadOpcode::cNE:
         return(3);
      case BXSQuadOpcode::cGT:
      case BXSQuadOpcode::cGE:
      case BXSQuadOpcode::cLT:
      case BXSQuadOpcode::cLE:
         return(4);
      case BXSQuadOpcode::cADD:
      case BXSQuadOpcode::cSUB:
         return(5);
      case BXSQuadOpcode::cMUL:
      case BXSQuadOpcode::cDIV:
      case BXSQuadOpcode::cMOD:
         return(6);
   }

   return(-1);
}
