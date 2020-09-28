//==============================================================================
// serialbuffer.h
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
#include "estypes.h"

//==============================================================================
class BSerialBuffer
{
      // SINGLE INSTANCE USAGE:
      // most common usage - for when you want to use the serial buffer once

      // On the send side, create a BSerialBuffer()
      // call add to add your elements
      // call getBuffer() and getBufferSize() to get a pointer to the buffer to send

      // On the recv side, create a BSerialBuffer(incomingBuffer, incomingSize)
      // then call get to get your elements


      // SHARED INSTANCE USAGE:
      // for when you want to use the same serial buffer multiple times (like BPacket does)
      // create a serial buffer using the empty ctor

      // On the send side, call resetDestination to set the destination buffer for serialization

      // On the recv side, call resetSource(buffer, size) then call get to get your elements      

   public:
      // Constructors

      // use the internal buffer
      BSerialBuffer();

      BSerialBuffer(const BSerialBuffer& src);

      // user supplied buffer
      BSerialBuffer(void* buffer, int32 size, bool isSource = true);

      // Destructors
      ~BSerialBuffer();

      BSerialBuffer& operator=(const BSerialBuffer& source)
      {
         if (this==&source)
            return *this;

         mBufferSize = source.mBufferSize;
         mWritePointer = source.mWritePointer;
         mReadPointer = source.mReadPointer;

         if (!mUserBuffer && mBuffer)
            delete[] mBuffer;

         mUserBuffer = source.mUserBuffer;

         if (mUserBuffer)
         {
            mBuffer = source.mBuffer;
         }
         else
         {
            mBuffer = new int8[mBufferSize];

            Utils::FastMemCpy(mBuffer, source.mBuffer, mBufferSize);
         }
         return *this;
      }

      // Functions

      // These functions only apply to a read/write serialbuffer 
      // They won't work after the buffer has been serialized (they will fail silently)
      void                       addData(const void* data, int32 size);      
      // this function is the same as addData, but it auto-inserts a size (int32) also, 
      // so that on the receiving side you can call getSizedData() 
      // and it will retrieve the correct size automagically
      // it costs one extra int32
      void                       addSizedData(const void* data, int32 size); 

      // USE THESE ADD AND GET METHODS 
      // Todo : JAR put these in a namespace, pass by reference, remove extraneous NULL checks
      void                       add(void* data, int32 size) { addData(data, size); }
      void                       add(const void* data, int32 size) { addData(data, size); }
      void                       add(char* data, int32 size) { addData(data, size); }
      void                       add(const char* data, int32 size) { addData(reinterpret_cast<const void*>(data), size); }
      void                       add(WCHAR* data, int32 size) { addData(data, size*sizeof(WCHAR)); }
      void                       add(uint8* data, int32 size) { addData((void *)data, size); }
      void                       add(int8 data) { addData((void *)&data, sizeof(data)); }
      void                       add(uint8 data) { addData((void *)&data, sizeof(data)); }
      void                       add(float data) { addData((void *)&data, sizeof(data)); }
      void                       add(int64 data) { addData((void *)&data, sizeof(data)); }
      void                       add(uint64 data) { addData((void *)&data, sizeof(data)); }
      void                       add(int16 data) { addData((void *)&data, sizeof(data)); }
      void                       add(uint16 data) { addData((void *)&data, sizeof(data)); }
      void                       add(int32 data) { addData((void *)&data, sizeof(data)); }
      void                       add(uint32 data) { addData((void *)&data, sizeof(data)); }
      void                       add(long data) { addData((void *)&data, sizeof(data)); }
      void                       add(bool data) { addData((void *)&data, sizeof(int8)); }
      void                       add(const GUID& data) { addData((void *)&data, sizeof(data)); }
      template<typename StringType>
      void                       add(const StringType& string)
      { 
         int32 realSize = string.length()*StringType::cBytesPerChar;
         // Write out an int 7 bits at a time. The high bit of the byte,
         // when on, tells reader to continue reading more bytes.
         uint32 v = (uint32)realSize; // support negative numbers
         while (v >= 0x80) 
         {
            add((byte) (v | 0x80));
            v >>= 7;
         }
         add((byte)v);

         addData((void*)string.getPtr(), realSize);
      }

      void                       addString(char* data);
      
      // These functions only apply to a read-only serialbuffer
      // They won't work before a buffer has been serialized (they will return null data)
      // Todo : JAR put these in a namespace, pass by reference, remove extraneous NULL checks
      void                       get(char* data);
      void                       get(uint8* data);
      void                       get(float* data);
      void                       get(int32* data);
      void                       get(uint32* data);
      void                       get(long* data);
      void                       get(int64* data);
      void                       get(uint64* data);
      void                       get(int16* data);
      void                       get(uint16* data);
      void                       get(bool* data);
      void                       get(GUID* data);
      void                       get(uint8* data, int32 size);
      void                       get(char** data, int32 size);
      void                       get(WCHAR** data, int32 size);
      void                       get(uint8** data, int32 size);
      void                       get(void** data, int32 size); 
      // this one has wierd param order but thats so it wouldn't interfere with the one above
      void                       getString(char** data, int32 maxSize);
      template<typename StringType>
      void                       get(StringType* string)
      {
         BASSERT(string);

         // Read out an int 7 bits at a time. The high bit
         // of the byte when on means to continue reading more bytes.
         int count = 0;
         int shift = 0;
         byte b = 0;
         do 
         {
            get(&b);
            count |= (b & 0x7F) << shift;
            shift += 7;
         } 
         while ((b & 0x80) != 0);

         //count>>=1;//(count/2)
         //count /= sizeof(BCHAR_T);
         count /= sizeof(StringType::charType);
         StringType::charType* pStr = new StringType::charType[count+1];
         pStr[0] = 0;
         pStr[count] = 0;
         get(&pStr, count);
         (*string).append(pStr, count);
         delete[] pStr;
      }
      // note: these functions return a pointer into the serialbuffer, not a newly allocated buffer
      // pass in size == 0 to get a pointer to the whole rest of the buffer
      void                       getPointer(const char** data, int32* size) { getPointer(reinterpret_cast<const void**>(data), size); }
      void                       getPointer(const void** data, int32* size);

      // set the serialization destination
      void                       resetDestination(); // serialize into internal buffer
      void                       resetDestination(const uchar* pBuffer, const int32 size); // serialize into user-supplied buffer
      void                       resetDestination(const void* pBuffer, const int32 size); // serialize into user-supplied buffer
            
      // deserialize from a user-supplied buffer
      void                       resetSource(const void* data, const int32 size);
      
      // get the serialized buffer and it's size
      const void *                getBuffer() const; // todo : JAR 20080225 - why is this returning a non-const void * to an internal buffer on a const method?
      int32                       getBufferSize() const;
      uint32                      getDataAvailable() const { return (mWritePointer - mReadPointer); }

      // Variables

   protected:

      // Enums

      enum
      {
         cMinBufferSize = 512,
         cDefaultBufferSize = 2048     // Static limit on the amount of serialized data, can be extended by just changing this number
      };

      // Variables

      uint32              mBufferSize;
      uint32              mWritePointer;
      uint32              mReadPointer;
      int8*               mBuffer;
      BOOL                mUserBuffer;
}; // BSerialBuffer
