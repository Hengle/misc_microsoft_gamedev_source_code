//============================================================================
//
// File: tlsValue.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once


//============================================================================
// class BTLSValue
//============================================================================
template<typename ValueType>
class BTLSValue
{
public:
   BTLSValue() : 
      mIndex(TlsAlloc())
   {
      if (TLS_OUT_OF_INDEXES == mIndex)
      {
         BASSERT(0);
      }
   }
   
   ~BTLSValue()
   {
      TlsFree(mIndex);
   }
   
   void set(ValueType* pVal)
   {
      if (!TlsSetValue(mIndex, pVal))
      {
         BASSERT(0);
      }
   }
   
   ValueType* get(void) const
   {
      return static_cast<ValueType*>(TlsGetValue(mIndex));
   }
   
private:
   DWORD mIndex;

   BTLSValue(const BTLSValue&);
   BTLSValue& operator= (const BTLSValue&);
};