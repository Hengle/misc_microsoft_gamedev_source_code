//==============================================================================
// serialbuffer.cpp
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "serialbuffer.h"

//==============================================================================
// Defines


//==============================================================================
// BSerialBuffer::BSerialBuffer
//==============================================================================
BSerialBuffer::BSerialBuffer() :
   mBufferSize(0),
   mWritePointer(0),
   mReadPointer(0),
   mBuffer(0),
   mUserBuffer(FALSE)
{
   resetDestination(); // set to serialize our own buffer
} // BSerialBuffer::BSerialBuffer

//==============================================================================
// 
//==============================================================================
BSerialBuffer::BSerialBuffer(const BSerialBuffer& src) :
   mBufferSize(0),
   mWritePointer(0),
   mReadPointer(0),
   mBuffer(0),
   mUserBuffer(FALSE)
{
   *this = src;
}

//==============================================================================
// BSerialBuffer::BSerialBuffer
//==============================================================================
BSerialBuffer::BSerialBuffer(void* buffer, int32 size, bool isSource /*= true*/) :
   mBufferSize(0),
   mWritePointer(0),
   mReadPointer(0),
   mBuffer(0),
   mUserBuffer(FALSE)
{
   isSource ? resetSource(buffer, size) : resetDestination(buffer, size);
} // BSerialBuffer::BSerialBuffer

//==============================================================================
// BSerialBuffer::~BSerialBuffer
//==============================================================================
BSerialBuffer::~BSerialBuffer()
{   
   if (!mUserBuffer && mBuffer) 
      delete [] mBuffer;
   mBuffer = NULL;
} // BSerialBuffer::~BSerialBuffer

//==============================================================================
// BSerialBuffer::addSizedData
//==============================================================================
void BSerialBuffer::addSizedData(const void* data, int32 size)
{  
   add(size);
   add(data, size);
} // BSerialBuffer::addSizedData 

//==============================================================================
// BSerialBuffer::addData
//==============================================================================
void BSerialBuffer::addData(const void* data, int32 size)
{
   BASSERT(data);
   BASSERT(size >= 0);
   //ProfileStartEventTime(EVENT_SERIALBUFFER_ADDDATA);

   // If the buffer is full, fail
   if (mUserBuffer && (mWritePointer+(unsigned)size > mBufferSize))
   {
      BASSERT(0);
      return;  
   }
   else if (mWritePointer+(unsigned)size > mBufferSize)
   {
      // reallocate buffer
      if (!mBufferSize)
         mBufferSize = cDefaultBufferSize;
      int32 tempsize = max(cMinBufferSize, max(mWritePointer+(unsigned)size+1, mBufferSize*2));
      int8* temp = new int8 [tempsize];
      if (!temp)
      {
         BASSERT(0);
         return;
      }
      if (mBuffer)
         Utils::FastMemCpy(temp, mBuffer, mBufferSize);
      mBufferSize = tempsize;
      if (mBuffer)
         delete [] mBuffer;      
      mBuffer = temp;      
   }

   // copy in the data
   if (!mBuffer)
   {
      BASSERT(0);
      return;
   }
   Utils::FastMemCpy( (((int8*)mBuffer)+mWritePointer), (int8*)data, size );

   // advance the write pointer
   mWritePointer+=size;

   //ProfileStopEventTime(EVENT_SERIALBUFFER_ADDDATA);
} // BSerialBuffer::addData

//template<typename StringType>
//void BSerialBuffer::add(StringType& string) 
//{ 
//   int32 realSize = string.length()*sizeof(BCHAR_T);
//   // Write out an int 7 bits at a time. The high bit of the byte,
//   // when on, tells reader to continue reading more bytes.
//   uint32 v = (uint32)realSize; // support negative numbers
//   while (v >= 0x80) 
//   {
//      add((byte) (v | 0x80));
//      v >>= 7;
//   }
//   add((byte)v);
//
//   addData((void*)string.getPtr(), realSize);
//}

//==============================================================================
void BSerialBuffer::addString(char* data)
{ 
   int32 l;
   if (data)         
      l = strlen(data)+1;                     
   else         
      l = 0;

   add(l); 
   if (l)
      add(data, l);          
}

//==============================================================================
// BSerialBuffer::getBuffer
//==============================================================================
const void* BSerialBuffer::getBuffer() const
{ 
   return mBuffer;    
} // BSerialBuffer::getBuffer

//==============================================================================
// BSerialBuffer::getBufferSize
//==============================================================================
int32 BSerialBuffer::getBufferSize() const
{ 
   return mWritePointer;
} // BSerialBuffer::getBufferSize

//==============================================================================
// BSerialBuffer::resetDestination
//==============================================================================
void BSerialBuffer::resetDestination()
{  
   mWritePointer = mReadPointer = 0;   
   if (mUserBuffer)  
   { 
      mBufferSize = 0;      
      mBuffer = 0;           
   }
   mUserBuffer = FALSE;
} // BSerialBuffer::reset

//==============================================================================
// BSerialBuffer::resetDestination
//==============================================================================
void BSerialBuffer::resetDestination(const uchar* pBuffer, const int32 size)
{
   if (!mUserBuffer && mBuffer)
      delete [] mBuffer;
   mWritePointer = mReadPointer = 0;
   mBufferSize = size;
   mBuffer = (int8*)(pBuffer);
   mUserBuffer = TRUE;
} // BSerialBuffer::BSerialBuffer

//==============================================================================
// BSerialBuffer::resetDestination
//==============================================================================
void BSerialBuffer::resetDestination(const void* pBuffer, const int32 size)
{
   if (!mUserBuffer && mBuffer)
      delete [] mBuffer;
   mWritePointer = mReadPointer = 0;
   mBufferSize = size;
   mBuffer = (int8 *)pBuffer;
   mUserBuffer = TRUE;
} // BSerialBuffer::BSerialBuffer

//==============================================================================
// BSerialBuffer::resetSource
//==============================================================================
void BSerialBuffer::resetSource(const void* buffer, const int32 size)
{
   if ((!mUserBuffer) && (mBuffer)) 
      delete [] mBuffer;
   mReadPointer = 0;
   mWritePointer = mBufferSize = size;
   mBuffer = (int8 *)buffer;
   mUserBuffer = TRUE;
} // BSerialBuffer::reset

//==============================================================================
void BSerialBuffer::get(char* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(char) <= mBufferSize) 
   { 
      *data = *reinterpret_cast<char*>(mBuffer+mReadPointer);
      mReadPointer+=sizeof(char);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(uint8* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(int8) <= mBufferSize)
   {
      *data = *reinterpret_cast<int8*>(mBuffer+mReadPointer);
      mReadPointer+=sizeof(int8);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(float* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(float) <= mBufferSize) 
   {
      //*data = *reinterpret_cast<float *>(mBuffer+mReadPointer);
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(float));
      mReadPointer+=sizeof(float);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(int32* data)
{
   BASSERT(data);
   if (mReadPointer+sizeof(int32) <= mBufferSize)
   {
      //*data = *reinterpret_cast<int32 *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(int32));
      mReadPointer+=sizeof(int32);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(uint32* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(uint32) <= mBufferSize) 
   {
      //*data = *reinterpret_cast<uint32 *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(uint32));
      mReadPointer+=sizeof(uint32);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(long* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(long) <= mBufferSize) 
   {
      //*data = *reinterpret_cast<uint32 *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(long));
      mReadPointer+=sizeof(long);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(int64 *data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(int64) <= mBufferSize) 
   { 
      //*data = *reinterpret_cast<int *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(int64));
      mReadPointer+=sizeof(int64); 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(uint64* data)
{
   BASSERT(data);
   if (mReadPointer+sizeof(uint64) <= mBufferSize)
   {
      //*data = *reinterpret_cast<int *>(mBuffer+mReadPointer);
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(uint64));
      mReadPointer+=sizeof(uint64);
   }
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(int16* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(int16) <= mBufferSize) 
   { 
      //*data = *reinterpret_cast<int16 *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(int16));
      mReadPointer+=sizeof(int16); 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(uint16* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(uint16) <= mBufferSize) 
   { 
      //*data = *reinterpret_cast<uint16 *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(uint16));
      mReadPointer+=sizeof(uint16); 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(bool* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(int8) <= mBufferSize) 
   { 
      *data = *reinterpret_cast<int8*>(mBuffer+mReadPointer)!=0; 
      mReadPointer+=sizeof(int8); 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(GUID* data) 
{ 
   BASSERT(data);   
   if (mReadPointer+sizeof(GUID) <= mBufferSize) 
   { 
      //*data = *reinterpret_cast<GUID *>(mBuffer+mReadPointer); 
      // rg [7/11/05] - to avoid Xbox misalignment issues
      Utils::FastMemCpy(data, mBuffer+mReadPointer, sizeof(GUID));
      mReadPointer+=sizeof(GUID); 
   } 
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(uint8* data, int32 size) 
{ 
   BASSERT(data);   
   BASSERT(size >= 0);
   BASSERT(mReadPointer <= (mReadPointer + (unsigned)size));
   BASSERT((unsigned)size <= (mWritePointer - mReadPointer));
   if (mReadPointer+(unsigned)size <= mBufferSize) 
   { 
      Utils::FastMemCpy(data, mBuffer+mReadPointer, size); 
      mReadPointer+=size; 
   } 
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(char** data, int32 size) 
{ 
   BASSERT(data);   
   BASSERT(size >= 0);
   BASSERT(mReadPointer <= (mReadPointer + (unsigned)size));
   BASSERT((unsigned)size <= (mWritePointer - mReadPointer));
   if (mReadPointer+(unsigned)size <= mBufferSize) 
   { 
      Utils::FastMemCpy(*data, mBuffer+mReadPointer, size); 
      mReadPointer+=size; 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(WCHAR** data, int32 size) 
{ 
   BASSERT(data); 
   BASSERT(size >= 0);
   BASSERT(mReadPointer <= (mReadPointer + (unsigned)size));
   BASSERT((unsigned)size <= (mWritePointer - mReadPointer));
   if (mReadPointer+(unsigned)(size*sizeof(WCHAR)) <= mBufferSize) 
   { 
      Utils::FastMemCpy(*data, mBuffer+mReadPointer, (size*sizeof(WCHAR))); 
      mReadPointer+=(size*sizeof(WCHAR)); 
   } 
   else
      BASSERT(0);
}

//==============================================================================
void BSerialBuffer::get(uint8** data, int32 size) 
{ 
   BASSERT(data);   
   BASSERT(size >= 0);
   BASSERT(mReadPointer <= (mReadPointer + (unsigned)size));
   BASSERT((unsigned)size <= (mWritePointer - mReadPointer));
   if (mReadPointer+(unsigned)size <= mBufferSize) 
   { 
      Utils::FastMemCpy(*data, mBuffer+mReadPointer, size); 
      mReadPointer+=size; 
   } 
   else
      BASSERT(0);
}      

//==============================================================================
void BSerialBuffer::get(void** data, int32 size) 
{ 
   BASSERT(data);     
   BASSERT(size >= 0);
   BASSERT(mReadPointer <= (mReadPointer + (unsigned)size));
   BASSERT((unsigned)size <= (mWritePointer - mReadPointer));
   if (mReadPointer+(unsigned)size <= mBufferSize) 
   {
      Utils::FastMemCpy(*data, mBuffer+mReadPointer, size); 
      mReadPointer+=size; 
   }
   else BASSERT(0);
}

//==============================================================================
// this one has wierd param order but thats so it wouldn't interfere with the one above
void BSerialBuffer::getString(char** data, int32 maxSize) 
{ 
   BASSERT(data);   
   BASSERT(maxSize >= 0);
   int32 l=0;

   get(&l); 
   if (l > maxSize)
      l = maxSize;                  
   if (l)
      get(data, l);          
}

//==============================================================================
//template<typename StringType>
//void BSerialBuffer::get(StringType* string) 
//{
//   BASSERT(string);
//
//   // Read out an int 7 bits at a time. The high bit
//   // of the byte when on means to continue reading more bytes.
//   int count = 0;
//   int shift = 0;
//   byte b = 0;
//   do 
//   {
//      get(&b);
//      count |= (b & 0x7F) << shift;
//      shift += 7;
//   } 
//   while ((b & 0x80) != 0);
//
//   //count>>=1;//(count/2)
//   count /= sizeof(BCHAR_T);
//   BCHAR_T *str = new BCHAR_T[count+1];
//   str[0] = 0;
//   str[count] = 0;
//   get(&str, count);
//   (*string).append(str, count);
//   delete[] str;
//}

//==============================================================================
// note: these functions return a pointer into the serialbuffer, not a newly allocated buffer
// pass in size == 0 to get a pointer to the whole rest of the buffer
void BSerialBuffer::getPointer(const void** data, int32* size) 
{
   BASSERT(data);
   if (!size)
      return;

   if (*size <= 0)
      *size = mBufferSize-mReadPointer; // set size to be the remaining portion of the buffer, if we're passed 0

   if (mReadPointer+(unsigned)*size <= mBufferSize)
   {
      *data = static_cast<void *>(mBuffer+mReadPointer); mReadPointer+=*size;
   }
   else
      BASSERT(0);
}
