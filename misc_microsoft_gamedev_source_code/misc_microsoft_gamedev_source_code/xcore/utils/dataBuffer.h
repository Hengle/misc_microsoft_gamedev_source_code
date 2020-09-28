//============================================================================
//
// File: dataBuffer.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

class BDataBuffer
{
public:
   BDataBuffer() { }
   BDataBuffer(void* pData, uint dataLen) : mpData(pData), mDataLen(dataLen) { }

   void set(void* pData, uint dataLen) { mpData = pData; mDataLen = dataLen; }
   
   BYTE* getPtr(void) const { return static_cast<BYTE*>(mpData); }
   void setPtr(void* pData) { mpData = pData; }

   uint getLen(void) const { return mDataLen; }
   void setLen(uint dataLen) { mDataLen = dataLen; }
   
   void clear(void)
   {
      mpData = NULL;
      mDataLen = 0;
   }

protected:   
   void* mpData;
   uint mDataLen;
};

class BConstDataBuffer
{
public:
   BConstDataBuffer() { }
   BConstDataBuffer(const BDataBuffer& b) : mpData(b.getPtr()), mDataLen(b.getLen()) { }
   BConstDataBuffer(const void* pData, uint dataLen) : mpData(pData), mDataLen(dataLen) { }

   void set(const void* pData, uint dataLen) { mpData = pData; mDataLen = dataLen; }

   const BYTE* getPtr(void) const { return static_cast<const BYTE*>(mpData); }
   void setPtr(const void* pData) { mpData = pData; }

   uint getLen(void) const { return mDataLen; }
   void setLen(uint dataLen) { mDataLen = dataLen; }
   
   void clear(void)
   {
      mpData = NULL;
      mDataLen = 0;
   }

protected:   
   const void* mpData;
   uint mDataLen;
};

template<class T>
class BTypedDataBuffer
{
public:
   BTypedDataBuffer() { }
   BTypedDataBuffer(const BTypedDataBuffer& b) : mpData(b.getPtr()), mDataLen(b.getLen()) { }
   BTypedDataBuffer(T* pData, uint dataLen) : mpData(pData), mDataLen(dataLen) { }

   void set(T* pData, uint dataLen) { mpData = pData; mDataLen = dataLen; }

   T* getPtr(void) const { return mpData; }
   void setPtr(T* pData) { mpData = pData; }

   uint getLen(void) const { return mDataLen; }
   void setLen(uint dataLen) { mDataLen = dataLen; }

   void clear(void)
   {
      mpData = NULL;
      mDataLen = 0;
   }

private:
   T* mpData;
   uint mDataLen;
};

template<class T>
class BTypedConstDataBuffer
{
public:
   BTypedConstDataBuffer() { }
   BTypedConstDataBuffer(const BTypedDataBuffer<T>& b) : mpData(b.getPtr()), mDataLen(b.getLen()) { }
   BTypedConstDataBuffer(const BTypedConstDataBuffer& b) : mpData(b.getPtr()), mDataLen(b.getLen()) { }
   BTypedConstDataBuffer(const T* pData, uint dataLen) : mpData(pData), mDataLen(dataLen) { }

   void set(const T* pData, uint dataLen) { mpData = pData; mDataLen = dataLen; }

   const T* getPtr(void) const { return mpData; }
   void setPtr(const T* pData) { mpData = pData; }

   uint getLen(void) const { return mDataLen; }
   void setLen(uint dataLen) { mDataLen = dataLen; }

   void clear(void)
   {
      mpData = NULL;
      mDataLen = 0;
   }

private:
   const T* mpData;
   uint mDataLen;
};





