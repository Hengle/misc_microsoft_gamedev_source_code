//============================================================================
//
//  vertexDeclUtils.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

class BVertexDeclUtils
{
public:
   static int getVertexDeclarationTypeSize(DWORD type);
   static const char* getVertexDeclarationTypeName(DWORD type);
   static const char* getVertexDeclarationUsageName(DWORD usage);
   static int setVertexDeclarationOffsets(D3DVERTEXELEMENT9* pElements);
   static uint getVertexDeclarationStreamVertexSize(const D3DVERTEXELEMENT9* pElements, uint streamIndex);
   static void dumpVertexDeclaration(BTextDispatcher& dispatcher, D3DVERTEXELEMENT9* pElements);
};
