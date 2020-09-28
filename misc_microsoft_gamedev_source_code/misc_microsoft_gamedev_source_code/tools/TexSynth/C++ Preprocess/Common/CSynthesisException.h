#ifndef __SynthesisException__
#define __SynthesisException__

#include <cstdarg>

#ifdef WIN32
#include <windows.h>
#include <strsafe.h>
#endif

class SynthesisException
{
protected:
  char m_szMsg[512];
public:

  SynthesisException(){m_szMsg[0]='\0';}

  SynthesisException(char *msg,...)
  {
    va_list args;
    va_start(args, msg);

    StringCchVPrintfA(m_szMsg,512,msg,args);

#ifdef WIN32
    OutputDebugStringA(m_szMsg);
    OutputDebugStringA("\n");
#endif

    va_end(args);
  }

  const char *getMsg() const {return (m_szMsg);}
};

#endif
