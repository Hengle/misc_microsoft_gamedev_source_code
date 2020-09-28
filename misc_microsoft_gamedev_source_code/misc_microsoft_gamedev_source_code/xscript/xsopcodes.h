//==============================================================================
// xsopcodes.h - XS intermediate language definitions
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSOPCODES_H_
#define _XSOPCODES_H_


//==============================================================================
class BXSQuadOpcode
{
   public:
      enum codes 
      {
         //ERROR.  For initialization.  Doesn't do anything.
         cERROR=-1,

         //NOP.  Doesn't do anything.
         cNOP,

         //RET.  Return from a function.
         cRET,

         //Jump.  Sets the PC to the jump address.
         //F1: Jump address.
         cJUMP,
         //JUMPZ.  Sets the PC to the jump address if the top stack element is 0.  Pops TOS.
         //F1: Jump address.
         cJUMPZ,
         //JUMPNZ.  Sets the PC to the jump address if the top stack element is NOT 0.  Pops TOS.
         //F1: Jump address.
         cJUMPNZ,
         //Label.  Signifies a labeled point in the code.  Doesn't do anything.
         cLABEL,

         //FUNCTION/SYSCALL opcodes.
         //PUSH.  Pushes a full copy (address onto stack and value onto heap) of a variable.
         //F1:  Index of the variable to push.
         cPUSH,
         //PUSHADD.  Pushes a variable's address onto the stack.
         //F1:  Index of the variable to push.
         cPUSHADD,
         //PUSHI.  Pushes an immediate "variable" value.  Only done for simple types (<= 4 bytes).
         //F1:  Type of variable.
         //F2:  Value.
         cPUSHI,
         //POP.  Pops X things off of the stack AND heap.
         //F1:  Number of things to pop.
         cPOP,
         //POPADD.  Pops X things off of JUST the stack.
         //F1:  Number of things to pop.
         cPOPADD,
         //CALLS.  Calls a syscall.
         //F1:  Syscall ID.
         cCALLS,
         //CALLF.  Calls an XS-defined function.
         //F1:  Function ID.
         cCALLF,
      
         //BOOLEAN OPCODES.
         //NOT.  Inverts the bool value on the top of the stack.
         cNOT,
         //AND.  Takes the bool values on the top of the stack, ANDs them together, pops them, and pushes a single
         //bool result.
         cAND,
         //OR.  Takes the bool values on the top of the stack, ORs them together, pops them, and pushes a single
         //bool result.
         cOR,

         //COMPARISON OPCODES.
         //ALL.  Compare top two variables (via addresses) on the stack, pops the top two values,
         //and pushes a bool result onto the stack.
         cGT,
         cGE,
         cNE,
         cEQ,
         cLE,
         cLT,

         //MATH OPCODES.
         //NEG.  Negates the variable value pointed to by the top of the stack.
         cNEG,
         //ADD.  Add two values.  Uses the top two stack addresses for operands.
         //Result value is pushed on the heap.
         cADD,
         //SUB.  Subtract two values.  Uses the top two stack addresses for operands.
         //Result value is pushed on the heap.
         cSUB,
         //MUL.  Multiply two values.  Uses the top two stack addresses for operands.
         //Result value is pushed on the heap.
         cMUL,
         //DIV.  Divide two values.  Uses the top two stack addresses for operands.
         //Result value is pushed on the heap.
         cDIV,
         //MOD.  Mod two values.  Uses the top two stack addresses for operands.
         //Result value is pushed on the heap.
         cMOD,
         //ASS.  Assign a value to a variable.  Expects the value address to be at
         //the top of the stack and the variable address to be just below that.
         cASS,

         //DBG.  Debug opcode.  Spits out a variable value.
         //F1: Register storing the address of variable to print out.
         cDBG,
         //SEP.  Used for debug output.
         cSEP,
         //ILL.  Used to set the infinite loop limit in the interpreter.
         cILL,
         //IRL.  Used to set the infinite recursion limit in the interpreter.
         cIRL,
         //LINE.  Used to set the current line number in the interpreter (for debugging and whatnot).
         cLINE,
         //FILE.  Used to set the current file number in the interpreter (for debugging and whatnot).
         cFILE,
         //BPNT.  Breakpoint.
         cBPNT,

         cNumberOpcodes
      };

      
      static char*               getName( long opcode );
      static long                getPrecedence( long opcode );
};


//==============================================================================
#endif // _XSOPCODES_H_
