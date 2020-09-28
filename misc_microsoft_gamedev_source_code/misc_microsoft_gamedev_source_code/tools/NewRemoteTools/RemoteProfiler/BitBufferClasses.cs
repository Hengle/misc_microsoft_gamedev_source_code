using System;
using System.Collections;



namespace EnsembleStudios.RemoteGameDebugger
{
   //replace template with interface...
   public interface ICoderBuffer
   {
      bool outputByte(byte c);
      int inputByte();
      uint getPos();
      uint getSize();
      /*const*/ byte[] getBuf();
      void setPos(uint pos);
   }

   public class BStaticCoderBuf : ICoderBuffer
   {
      public BStaticCoderBuf(byte[] pBuf, uint bufSize)
      {
         setBuf(pBuf,bufSize);
      }

      public BStaticCoderBuf()
      {
         setBuf(null,0);
      }

      public void setBuf(byte[] pBuf, uint bufSize)
      {
         mpBuf = pBuf;
         mBufSize = bufSize;
         mBufPos = 0;
      }

      // returns false on failure
      public bool outputByte(byte c)
      {
         if (mBufPos >= mBufSize)
            return false;

         mpBuf[mBufPos++] = c;
         return true;
      }

      public int inputByte() 
      {
         if (mBufPos >= mBufSize)
            return 0;
         return mpBuf[mBufPos++];
      }

      public uint getPos()// const
      {
         return mBufPos;
      }

      public uint getSize()// const
      {
         return mBufSize;
      }

      //      public const byte[] getBuf() //const 
      //      {
      //         return mpBuf;
      //      }

      public byte[] getBuf() 
      {
         return mpBuf;
      }

      public void setPos(uint pos) 
      {
         mBufPos = pos;
      }

   
      private byte[] mpBuf;
      private uint mBufSize;
      private uint mBufPos;
   }

   //---------------------------------------------------------------------------------------------------
   // class BDynamicCoderBuf
   // This buffer uses a container that grows as bytes are written.
   //---------------------------------------------------------------------------------------------------
   public class BDynamicCoderBuf : ICoderBuffer
   {

      public BDynamicCoderBuf(int size)
      {
         mBufPos = 0;
         //mBuf.resize(size);
         resize(ref mBuf,size);
      }

      // returns false on failure
      public bool outputByte(byte c)
      {
         if (mBufPos >= size(mBuf))
         {
            //mBuf.resize(mBufPos + 1);
            resize(ref mBuf,(int)mBufPos + 1);
         }
         mBuf[mBufPos++] = c;
         return true;
      }

      // returns -1 on failure
      public int inputByte()
      {
         if (mBufPos >= size(mBuf))
            return 0;
         return mBuf[mBufPos++];
      }

      public uint getPos()// const
      {
         return mBufPos;
      }

      public uint getSize()// const
      {  
         return size(mBuf);
      }

      public /*const*/ byte[] getBuf() //const 
      {
         //return &mBuf[0];
         return mBuf;
      }

      public void setPos(uint pos) 
      {
         mBufPos = pos;
      }
  
      private byte[] mBuf;
      private uint mBufPos;
      
      private void resize(ref byte[] buffer, int size)
      {
         if(buffer != null)
         {
            byte[] newBuf = new byte[size];
            buffer.CopyTo(newBuf,0);
            buffer = newBuf;
         }
         else
         {
            buffer = new byte[size];
         }
         //throw new System.Exception("implement this");
      }
      private uint size(byte[] buffer)
      {
         return (uint)buffer.Length;
      }
   }


   //---------------------------------------------------------------------------------------------------
   // class BBitPacker
   // BufferType is the type of buffer object to be used to read/write bytes (for example BStaticCoderBuf or BDynamicCoderBuf). I could have 
   // made this an abstract interface but I wanted to avoid the overhead of virtual dispatching.
   //---------------------------------------------------------------------------------------------------
   //template<typename BufferType>
   public class BBitPacker
   {
      //typedef BufferType bufferType;      
      public BBitPacker()
      {
         mBitBuf = 0;
         mNumBufBits = 0;
         mpBuf = null;    
      }
      public BBitPacker(/*ref*/ ICoderBuffer pBuf)
      {
         mBitBuf = 0;
         mNumBufBits = 0;
         mpBuf = pBuf;       
      }
      
      public ICoderBuffer getBuffer() 
      { 
         return mpBuf; 
      }
      
      public void setBuffer(/*ref*/ ICoderBuffer pBuf)
      {
         mpBuf = pBuf;
      }

      // Starts encoding bits.   
      public void encodeStart()
      {
         mBitBuf = 0;
         mNumBufBits = 0;
      }
      
      // Encodes 0 to 16 bits.
      // false on failure
      public bool encodeSmall(ushort bits, int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
         //BDEBUG_ASSERT(mpBuf != null);
         
         mBitBuf |= (uint)(bits << mNumBufBits);
         mNumBufBits += numBits;
         
         while (mNumBufBits >= 8)
         {
            if (!mpBuf.outputByte((byte)(mBitBuf)))
               return false;
               
            mBitBuf >>= 8;
            mNumBufBits -= 8;
         }

         return true;
      }
      
      // Encodes 0-32 bits.
      // false on failure
      public bool encode(uint bits, int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 32));
               
         if (numBits <= 16)
            return encodeSmall((ushort)(bits), numBits);
         
         if (!encodeSmall((ushort)(bits), 16))
            return false;
            
         return encodeSmall((ushort)(bits >> 16), numBits - 16);
      }
      
      // Encodes 0-64 bits.
      // false on failure
      public bool encode(UInt64 bits, int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 64));

         if (numBits <= 32)
            return encode((uint)(bits), numBits);

         if (!encode((uint)(bits), 32))
            return false;

         return encode((uint)(bits >> 32), numBits - 32);
      }

      // Ends bit encoding by flushing the bit buffer to the next byte boundary, if necessary.
      // false on failure
      public bool encodeEnd()
      {
         if (!encodeSmall(0, 7))
            return false;
            
         mBitBuf = 0;
         mNumBufBits = 0;
         return true;
      }
      
      // Starts decoding by priming the bit buffer.
      // false on failure
      public bool decodeStart()
      {
         //BDEBUG_ASSERT(mpBuf != null);
         
         mBitBuf = 0;
         mNumBufBits = 0;
         
         for (int i = 0; i < 4; i++)
         {
            int b = mpBuf.inputByte();
            if (b < 0)
               return false;
               
            mBitBuf |= (uint)(b << mNumBufBits);
            mNumBufBits += 8;
         }
         
         mNumBufBits = 16;
         
         return true;
      }
            
      // Removes 0-16 bits from the stream.
      // false on failure.
      public bool decodeRemoveBits(int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
         //BDEBUG_ASSERT(mpBuf != null);
         
         if ((mNumBufBits -= numBits) >= 0)
            mBitBuf >>= numBits;
         else
         {
            mBitBuf >>= (numBits + mNumBufBits);

            /*const*/ int b0 = mpBuf.inputByte();
            if (b0 < 0)
               return false;
               
            /*const*/ int b1 = mpBuf.inputByte();
            if (b1 < 0)
               return false;
               
            mBitBuf |= (uint)((b0 << 16) | (b1 << 24));

            mBitBuf >>= (-mNumBufBits);

            mNumBufBits += 16;
         }
         
         return true;
      }
      
      // Syncs up the decoder to the next byte boundary
      // false on failure
      public bool decodeSyncToNextByte()
      {
         if ((mNumBufBits & 7) != 0)
            return decodeRemoveBits( 7 - (mNumBufBits & 7) );
         return true;
      }
            
      // Decodes and removes the next 0-16 bits.
      // false on failure
      public bool decodeSmall(out uint bits, int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
         
         bits = mBitBuf & (uint)((1 << numBits) - 1);

         return decodeRemoveBits(numBits);
      }
      
      // Look ahead the next 0-16 bits.
      public uint decodeLookahead(int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));

         return mBitBuf & (uint)((1 << numBits) - 1);
      }
      
      // Decodes and removes 0-32 bits.
      // false on failure
      public bool decode(out uint bits, int numBits)
      {
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 32));
         
         if (numBits <= 16)
            return decodeSmall(out bits, numBits);
                          
         if (!decodeSmall(out bits, 16))
            return false;
         
         uint upper;
         if (!decodeSmall(out upper, numBits - 16))
            return false;
         
         bits |= (upper << 16);
         
         return true;  
      }
      
      // Decodes and removes 0-64 bits.
      // false on failure
      public bool decode(out UInt64 bits, int numBits)
      {
         bits = 0;
         //BDEBUG_ASSERT((numBits >= 0) && (numBits <= 64));

         uint l, h;
         if (numBits <= 32)
         {
            if (!decode(out l, numBits))
               return false;
            
            bits = l;
            return true;
         }
         
         if (!decode(out l, 32))
            return false;
            
         if (!decode(out h, numBits - 32))
            return false;
         
         bits = l | (((UInt64)(h)) << 32);

         return true;  
      }

      //private BufferType* mpBuf;
      private ICoderBuffer mpBuf;
      private uint mBitBuf;
      private int mNumBufBits;

      void BDEBUG_ASSERT(bool val)
      {
         if(val == false)
            throw new System.Exception("//BDEBUG_ASSERT");
      }
   }   
   
   public class BitPacker : BBitPacker
   {
      public BitPacker(/*ref*/ ICoderBuffer pBuf) : base(pBuf)
      {
      }
   }
}