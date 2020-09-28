// File: fileTypes.cpp
#include "xcore.h"
#include "fileTypes.h"

#include <d3d9.h>
#include <d3dx9.h>

//---------------------------------------------------------------------------------------------------------------
// BFileType::getFileTypeExtension
//---------------------------------------------------------------------------------------------------------------
const char* BFileType::getExtension(eFileType fileType)
{
   static const char *formatExt[]=
   {
         "ddx",
         "dds",
         "ddt",
         "bmp",
         "tga",
         "png",
         "jpg",
         "hdr",
         "tif"
   };

   BDEBUG_ASSERT((sizeof(formatExt) / sizeof(formatExt[0])) == cFileTypeCount);

   if ((fileType >= cFileTypeCount))
      return NULL;

   return formatExt[fileType];
}

//---------------------------------------------------------------------------------------------------------------
// determineType
//---------------------------------------------------------------------------------------------------------------
eFileType BFileType::determineFromFilename(const char* pFilename)
{
   char srcExt[_MAX_EXT];
   _splitpath_s(pFilename, NULL, 0, NULL, 0, NULL, 0, srcExt, sizeof(srcExt));

   if (srcExt[0] == '.')
      memmove(srcExt, srcExt + 1, strlen(srcExt + 1) + 1);

   return determineFromExtension(srcExt);
}

//---------------------------------------------------------------------------------------------------------------
// determineExtensionType
//---------------------------------------------------------------------------------------------------------------
eFileType BFileType::determineFromExtension(const char* pExtension)   
{
   for (uint i = 0; i < cFileTypeCount;i++)
      if(0 == _stricmp(pExtension, getExtension(static_cast<eFileType>(i))))
         return (eFileType)i;

   return cFileTypeInvalid;
}
