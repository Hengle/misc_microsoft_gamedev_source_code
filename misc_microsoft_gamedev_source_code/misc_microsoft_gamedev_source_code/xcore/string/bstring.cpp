//============================================================================
//
//  BUString.cpp
//
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"

BStringHeapAllocator gStringHeapStringAllocator  (&gStringHeap);
BStringHeapAllocator gPrimaryHeapStringAllocator (&gPrimaryHeap);
BStringHeapAllocator gRenderHeapStringAllocator  (&gRenderHeap);
BStringHeapAllocator gSimHeapStringAllocator     (&gSimHeap);

BStringFixedHeapAllocator gStringFixedHeapAllocator;

BString sEmptyString("");
BUString sEmptyUString(L"");
BSimString sEmptySimString("");

#ifndef BUILD_FINAL
void BStringTest(void)
{

   BStringTemplate<WCHAR> unicodeString;
   BStringTemplate<char> ansiString;
   
   //unicodeString.asANSI();
   //unicodeString.asUnicode();
   //ansiString.asANSI();
   //ansiString.asUnicode();
   
   ansiString.asUnicode(unicodeString);
   unicodeString.asANSI(ansiString);
   
   ansiString.set("AA");
   unicodeString.set("AA");
   ansiString.set(L"AA");
   unicodeString.set(L"AA");

   unicodeString.set(unicodeString);
   ansiString.set(ansiString);
   
   //unicodeString.set(ansiString.asUnicode());
   //ansiString.set(unicodeString.asANSI());
   
   ansiString.remove('A');
   unicodeString.remove(L'A');
      
   ansiString = BString(unicodeString);
   unicodeString = BString(ansiString);
      
   ansiString.empty();
   ansiString.setNULL();
   ansiString.copy("blah");
   ansiString.copyTok(BString("ksjkls"));
   ansiString.append(BString("AAAA"));
   ansiString.append("AAAA");
   ansiString.prepend(BString("AAA"));
   ansiString.insert(0, BString("AAAA"));
   ansiString.insert(0, 32);

   ansiString.remove           (0, BString(""));
   ansiString.remove           (0,2);
   ansiString.remove           (32);
   ansiString.crop             (20, 30);
   ansiString.format           ("%s", "blah");
   if (0)
      ansiString.format           ("%s", NULL);
   ansiString.trimLeft         ();
   ansiString.trimRight        ();
   ansiString.toLower          ();
   ansiString.toUpper          ();
   ansiString.removeExtension  ();
   ansiString.endianSwap();
   
   unicodeString.empty();
   unicodeString.setNULL();
   unicodeString.copy(L"blah");
   unicodeString.copyTok(BUString(L"ksjkls"));
   unicodeString.append(BUString(L"AAAA"));
   unicodeString.append(L"AAAA");
   unicodeString.prepend(BUString(L"AAA"));
   unicodeString.insert(0, BUString(L"AAAA"));
   unicodeString.insert(0, 32);

   unicodeString.remove           (0, BUString(L""));
   unicodeString.remove           (0,2);
   unicodeString.remove           (32);
   unicodeString.crop             (20, 30);
   unicodeString.format           (L"%s", L"blah");
   if (0)
      unicodeString.format           (L"%s", NULL);
   unicodeString.trimLeft         ();
   unicodeString.trimRight        ();
   unicodeString.toLower          ();
   unicodeString.toUpper          ();
   unicodeString.removeExtension  ();
   unicodeString.endianSwap();
   
   BString blah(BUString(L""));
   BUString blah2(BString(""));
   
   ansiString.set(blah);
   ansiString.set(blah2);
   unicodeString.set(blah);
   unicodeString.set(blah2);
   
//   BString blah6;
//   printf("%s", blah6);
}

#endif
