//==============================================================================
// xsconfigmacros.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSCONFIGMACROS_H_
#define _XSCONFIGMACROS_H_

//==============================================================================
// Includes.


//==============================================================================
// Syscalls.
#ifndef _DEBUG
   #define XS_HELP(_help) \
      { \
         if (! sm->setSyscallHelp(_help)) \
         { \
             sm->clearSyscall(); \
         } \
         sm->finishSyscallAdd(); \
      }
#endif
#ifdef _DEBUG
   #define XS_HELP(_help) \
      { \
         if (! sm->setSyscallHelp(_help)) \
         { \
             sm->infoMsg("Syscall config error - Failed to register help for syscall '%s' (this is okay if this is a savegame load).", sm->getNewSyscallName()); \
             sm->clearSyscall(); \
         } \
         sm->finishSyscallAdd(); \
      }
#endif

#define XS_SYSCALL(_name, _type, _address, _ver) \
   { \
      if (sm->addSyscall(_name, _address, _type) == false) \
      { \
          sm->infoMsg("Syscall config error - Unable to add the '%s' syscall.", _name); \
          sm->clearSyscall(); \
      } \
   }
#define XS_CONTEXTSYSCALL(_name, _type, _address, _ver) \
   { \
      if (sm->addSyscall(_name, _address, _type) == false) \
      { \
          sm->infoMsg("Syscall config error - Unable to add the '%s' syscall.", _name); \
          sm->clearSyscall(); \
      } \
      if (sm->setSyscallContext(true) == false) \
      { \
          sm->clearSyscall(); \
      } \
   }
#define XS_CONSOLESYSCALL(_name, _address) \
   { \
      if (sm->addSyscall(_name, _address, XSBinary::cVoidVariable, 0) == false) \
      { \
          sm->infoMsg("Syscall config error - Unable to add the '%s' syscall.", _name); \
          sm->clearSyscall(); \
      } \
   }
#define XS_INTEGER_PARM(_defValue) \
   { \
      if (! sm->addSyscallIntegerParameter(_defValue)) \
      { \
          sm->infoMsg("Syscall config error - Unable to add integer parm %d for syscall '%s'.", _defValue, sm->getNewSyscallName()); \
          sm->clearSyscall(); \
      } \
   }
#define XS_FLOAT_PARM(_defValue) \
   { \
      if (! sm->addSyscallFloatParameter(_defValue)) \
      { \
          sm->infoMsg("Syscall config error - Unable to add float parm %f for syscall '%s'.", _defValue, sm->getNewSyscallName()); \
          sm->clearSyscall(); \
      } \
   }
#define XS_BOOL_PARM(_defValue) \
   { \
      if (! sm->addSyscallBoolParameter(_defValue)) \
      { \
          sm->infoMsg("Syscall config error - Unable to add bool parm %d for syscall '%s'.", _defValue, sm->getNewSyscallName()); \
          sm->clearSyscall(); \
      } \
   }
#define XS_STRING_PARM(_defValue) \
   { \
      if (! sm->addSyscallStringParameter(_defValue)) \
      { \
          sm->infoMsg("Syscall config error - Unable to add string parm %s for syscall '%s'.", _defValue, sm->getNewSyscallName()); \
          sm->clearSyscall(); \
      } \
   }
#define XS_VECTOR_PARM(_defValue) \
   { \
      if (! sm->addSyscallVectorParameter(_defValue)) \
      { \
          sm->infoMsg("Syscall config error - Unable to add vector parm (%f, %f, %f) for syscall '%s'.", _defValue.x, _defValue.y, _defValue.z, sm->getNewSyscallName()); \
          sm->clearSyscall(); \
      } \
   }


//==============================================================================
#endif // _XSCONFIGMACROS_H_
