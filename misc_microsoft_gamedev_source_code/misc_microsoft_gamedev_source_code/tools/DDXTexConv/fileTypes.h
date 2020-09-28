// fileTypes.h
#pragma once

enum eFileType
{
   cFileTypeInvalid = -1,
   cFileTypeDDX,
   cFileTypeDDS,
   cFileTypeDDT,   
   cFileTypeBMP,
   cFileTypeTGA,
   cFileTypePNG,
   cFileTypeJPG,
   cFileTypeHDR,
   cFileTypeTIF,

   cFileTypeCount
};

class BFileType
{
public:
   static const char* getExtension(eFileType fileType);
   static eFileType determineFromFilename(const char* pFilename);
   static eFileType determineFromExtension(const char* pExtension);
};
