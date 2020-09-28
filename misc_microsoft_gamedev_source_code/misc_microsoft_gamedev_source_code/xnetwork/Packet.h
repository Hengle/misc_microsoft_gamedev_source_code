//==============================================================================
// Packet.h
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "MaxSendSize.h"
#include "Serializable.h"
#include "SerialBuffer.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

// A Packet is an object that knows how to be passed around on the network - 
// it has serialize and deserialize methods that convert the object to and 
// from a serialized byte stream
//
// Optionally, it also has a "signature" which is a seperate portion that is stuck
// onto the front of the byte stream and can be used to distinguish one 
// object from another by peeking at the beginning of a byte stream - the
// serializeSignature, and deserializeSignature methods handle this functionality,
// and a child object can implement it's own static methods to peek a data stream and compare
// it against it's signature to determine a type match (see BTypedPacket::getType() for an example)
//
// The client code simply calls serializeInto and deserializeFrom normally.. 
//==============================================================================
class BPacket : public BSerializable
{
   public:      
      BPacket(void) : mSB(0,0) {}                  
      virtual ~BPacket(void) {}

      enum { cDefaultMaxSize = cMaxSendSize };
      virtual long            getMaxSerialSize(void) const { return cDefaultMaxSize; }
   
      virtual void            deserializeFrom(const void *data, const long size) { mSB.resetSource(data, size); deserialize(mSB); }    
      virtual void            serializeInto(void **data, long *size) { mSB.resetDestination(*data, *size); serialize(mSB); if (size) *size = mSB.getBufferSize(); } 
      virtual void            serializeInto(uchar* ppData, long* pSize) { mSB.resetDestination(ppData, *pSize); serialize(mSB); if (pSize) *pSize = mSB.getBufferSize(); } 

      virtual void            deserializeFrom(BSerialBuffer &sb) { deserialize(sb); }    
      virtual void            serializeInto(BSerialBuffer &sb) { serialize(sb); } 

      virtual void            serialize(BSerialBuffer &sb) {serializeSignature(sb);}
      virtual void            deserialize(BSerialBuffer &sb) {deserializeSignature(sb);}      

      virtual void            serializeSignature(BSerialBuffer &sb) {sb;}
      virtual void            deserializeSignature(BSerialBuffer &sb) {sb;}      

      static long             getSignatureSize(void) {return 0;}

      BSerialBuffer           mSB;
};

// The basic typed packet - includes a simple byte-length type field
// Notes:
// Packets usually have two ctors, a send ctor and a recv ctor
// The send ctor has ctor parameters to setup the packet with it's data
// you then send it on it's way (calling link->sendPacket or what have you)
// On the receive side, you test for type using BTypedPacket::getType (or BWhateverPacket::getType)
// and then create a corresponding object using the recv ctor, which is usually void of parameters
//==============================================================================
class BTypedPacket : public BPacket
{
   public:

      BTypedPacket(long type) { BASSERT(type < 255); mType = static_cast<unsigned char>(type); }
      BTypedPacket() : mType(0) {}

      virtual ~BTypedPacket() {}

      static long getType(const void* pData)
      {
         if (pData == 0)
            return(-1);
         return static_cast<long>(static_cast<const char*>(pData)[BPacket::getSignatureSize()]) & 0xFF;
      }

      static long getSignatureSize() { return BPacket::getSignatureSize()+sizeof(unsigned char); }

      virtual void serializeSignature(BSerialBuffer& sb)
      {
         BPacket::serializeSignature(sb);
         sb.add(mType);
      }

      virtual void deserializeSignature(BSerialBuffer& sb)
      {
         BPacket::deserializeSignature(sb);
         sb.get(&mType);
      }

   private:
      unsigned char mType;
};
