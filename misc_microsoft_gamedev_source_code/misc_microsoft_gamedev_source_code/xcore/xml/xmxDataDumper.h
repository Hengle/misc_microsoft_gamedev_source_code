//============================================================================
//
// File: xmxDataDumper.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "xmxData.h"

class BXMXDataDumper
{
   BXMXDataDumper(const BXMXDataDumper&);
   BXMXDataDumper& operator= (const BXMXDataDumper&);
   
public:
   BXMXDataDumper();
   
   bool dump(BTextDispatcher& textDispatcher, const BConstDataBuffer& buf);
   
   void setIndentSize(uint size) { mIndentSize = size; }
   
private:
   // This class can only dump NATIVE endianness XMX data!
   typedef BXMXData<true, cBigEndianNative> BXMXDataType;
   
   BXMXDataType*     mpXMXData;
   BTextDispatcher*  mpTextDispatcher;
   uint              mIndent;
   uint              mIndentSize;
   
   bool dumpNode(uint nodeIndex);
};