//---------------------------------------------------------------------------
#include "OSAdapter.h"
#include "CLibTextureException.h"
#include <string.h>
//---------------------------------------------------------------------------
void OSAdapter::convertName(char *name)
{
  for (;*name != '\0';name++)
  {
    if ((*name == '/') || (*name == '\\'))
      *name=OS_FILE_SEPARATOR;
  }
}
//---------------------------------------------------------------------------
const char *OSAdapter::convertName(const char *str)
{
  static char ptr[1024];
  char       *name=ptr;
  
  if (strlen(str) > 1023)
	  throw CLibTextureException("OSAdapter::String too long");
  strcpy(ptr,str);
  for (;*name != '\0';name++)
  {
    if ((*name == '/') || (*name == '\\'))
      *name=OS_FILE_SEPARATOR;
  }
  return (ptr);
}
//---------------------------------------------------------------------------
