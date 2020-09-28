#include "CLibTextureException.h"

#include <cstdarg>
#include <strsafe.h>

CLibTextureException::CLibTextureException(char *msg,...)
{
  va_list args;
  va_start(args, msg);

  StringCchVPrintfA(m_szMsg,512,msg,args);

  va_end(args);
}
