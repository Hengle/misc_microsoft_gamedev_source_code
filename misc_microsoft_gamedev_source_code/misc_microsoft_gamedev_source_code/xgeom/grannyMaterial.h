// File: granny_material.h
#pragma once

// local
#include "unigeom.h"

struct granny_material;

class BMaterialBuilder
{
public:
   BMaterialBuilder()
   {
   }

   bool create(
      Unigeom::BMaterial& dstMaterial,
      int accessoryIndex, 
      const granny_material* pGrannyMaterial,
      BTextDispatcher* pLog);
      
private:
   bool findStringParam(const granny_material* pGrannyMaterial, const char* pName, BString& str);
   bool findIntParam(const granny_material* pGrannyMaterial, const char* pName, int& val);
   bool findFloatParam(const granny_material* pGrannyMaterial, const char* pName, float& val);
   
};

