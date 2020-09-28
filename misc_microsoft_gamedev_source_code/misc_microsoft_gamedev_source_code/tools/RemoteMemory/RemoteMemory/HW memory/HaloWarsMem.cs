using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   //we mimic the structs here that we'll receive from the allocation logger.

   //==============================================================================
   // HaloWarsMemoryEventListener
   //==============================================================================
   public interface HaloWarsMemoryEventListener
   {
      void onHeapRegister(uint mPtr, int flags, string name);
      void onNew(uint mpHeap, uint mSize, uint mpBlock, uint mBlockSize, HaloWarsMem.BALContext context);
      void onResize(uint mpHeap, uint mpOrigBlock, uint mNewSize, uint mpNewBlock, HaloWarsMem.BALContext context);
      void onDelete(uint mpHeap, uint mpBlock, HaloWarsMem.BALContext context);
      void onConnect();
      void onDisconnect();
   };


   //==============================================================================
   // HaloWarsMem
   //==============================================================================
   public class HaloWarsMem
   {
      //==============================================================================
      // Packet structs
      //==============================================================================
      public static uint cALStreamVersion = 0xDEAD0001;
      public static uint cALPacketPrefix = 0x7F;

      //==============================================================================
      // eALPacketType
      //==============================================================================
      public enum eALPacketType
      {
         cALVersion,
         cALRegisterHeap,  //size = 44
         cALNew,           //size = 96
         cALResize,        
         cALDelete,        //size = 84
         cALSnapshot,
         cALEOF,
         cALFrame,
         cALIgnoreLeaf,
            
         cALNumPacketTypes
      };

      //==============================================================================
      // BALPacketBase
      //==============================================================================
      public class BALPacketBase
      {
         public byte mPacketPrefix;
         public byte mPacketType;
         public short mPacketSize;
         
         public BALPacketBase(eALPacketType type)
         {
            mPacketPrefix=(byte)cALPacketPrefix;
            mPacketType=(byte)type;
            mPacketSize = 0;// (int)allocLogPacketGetSize((uint)type); //CLM do we need this, since we'll be getting it from the 360?
         }
            
      };

      //==============================================================================
      // BALPacketVersion
      //==============================================================================
      public class BALPacketVersion 
      {
         public int mVersion;
         public int mXEXChecksum;
         public int mXEXBaseAddress;
         public Int64 mTimerFreq;

         public BALPacketVersion(int packetSize, byte[] packetData)
         {
            mVersion = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mXEXChecksum = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 4));
            mXEXBaseAddress = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 8));
            mTimerFreq = Xbox_EndianSwap.endSwapI64(BitConverter.ToInt64(packetData, 12));
         }
         
      };

      //==============================================================================
      // BALPacketRegisterHeap
      //==============================================================================
      public class BALPacketRegisterHeap 
      {
         public uint mPtr;

         public enum eFlags
         {
            cFlagPhysical = 1,
         };

         public int mFlags;

         public string mName = "";

         public BALPacketRegisterHeap(int packetSize, byte[] packetData)
         {
            mPtr = (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mFlags = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 4));

            int plus = 8;

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 32; i++)
            {
               char ch = (char)packetData[i + plus];
               if (ch == '\0')
               {
                  break;
               }
               sb.Append(ch);
            }

            mName = sb.ToString();

         }
         
      };

      //==============================================================================
      // BALContext
      //==============================================================================
      public class BALContext
      {
         public byte mThreadIndex;
         public byte mBackTraceSize;
         public Int64 mTime;

         enum eFlags{ cMaxBackTrace = 16 };
         public uint[] mBackTrace = new uint[(int)eFlags.cMaxBackTrace];

         public void frompacket(int packetSize, byte[] packetData, int startByte)
         {
            mThreadIndex = packetData[startByte]; 
            startByte++;
            mBackTraceSize = packetData[startByte];
            startByte++;

            mTime = Xbox_EndianSwap.endSwapI64((Int64)BitConverter.ToUInt64(packetData, startByte));
            startByte += 8;

            for (int i = 0; i < (int)eFlags.cMaxBackTrace; i++)
            {
               mBackTrace[i] = (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, startByte));
               startByte += 4;
            }
         }

         public void copyTo(ref BALContext rhs)
         {
            rhs.mThreadIndex = mThreadIndex;
            rhs.mBackTraceSize = mBackTraceSize;
            rhs.mTime = mTime;

            for (int i = 0; i < (int)eFlags.cMaxBackTrace; i++)
               rhs.mBackTrace[i] = mBackTrace[i];
            
         }
      };

      //==============================================================================
      // BALPacketNew
      //==============================================================================
      public class BALPacketNew
      {
         public uint mpHeap;    //void*
         public uint mSize;
         public uint mpBlock;   //void*
         public uint mBlockSize;

         public BALContext mContext = new BALContext();

         public BALPacketNew(int packetSize, byte[] packetData)
         {
            mpHeap =       (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mSize =        (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 4));
            mpBlock =      (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 8));
            mBlockSize =   (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 12));

            mContext.frompacket(packetSize,packetData,16);
         }
         
         
         
      };

      //==============================================================================
      // BALPacketResize
      //==============================================================================
      public class BALPacketResize
      {
         public uint mpHeap;       //void*
         public uint mpOrigBlock;  //void*
         public uint mNewSize;
         public uint mpNewBlock;   //void*

         public BALContext mContext = new BALContext();

         public BALPacketResize(int packetSize, byte[] packetData)
         {
            mpHeap =       (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mpOrigBlock =  (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 4));
            mNewSize =     (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 8));
            mpNewBlock =   (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 12));

            mContext.frompacket(packetSize, packetData, 16);
         }
      };

      //==============================================================================
      // BALPacketDelete
      //==============================================================================
      public class BALPacketDelete 
      {
         public uint mpHeap;    //void*
         public uint mpBlock;   //void*

         public BALContext mContext = new BALContext();

         public BALPacketDelete(int packetSize, byte[] packetData)
         {
            mpHeap =    (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mpBlock =   (uint)Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 4));
            
            mContext.frompacket(packetSize, packetData, 8);
         } 
         
      };

      //==============================================================================
      // BALPacketEOF
      //==============================================================================
      public class BALPacketEOF 
      {
         public BALPacketEOF() { }
      };

      //==============================================================================
      // BALPacketSnapshot
      //==============================================================================
      public class BALPacketSnapshot
      {
         public int mIndex;

         public BALPacketSnapshot(int packetSize, byte[] packetData) 
         {
            mIndex = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
         }

      };

      //==============================================================================
      // BALPacketFrame
      //==============================================================================
      public class BALPacketFrame 
      {
         public int mIndex;
         public Int64 mTime;

         public BALPacketFrame(int packetSize, byte[] packetData)
         {
            mIndex = Xbox_EndianSwap.endSwapI32(BitConverter.ToInt32(packetData, 0));
            mTime = Xbox_EndianSwap.endSwapI64(BitConverter.ToInt64(packetData, 4));
         }
      };

      //==============================================================================
      // BALPacketIgnoreLeaf
      //==============================================================================
      public class BALPacketIgnoreLeaf 
      {
         //public byte[] mSymbol = new byte[64];
         public string mSymbolName = "";

         public BALPacketIgnoreLeaf(int packetSize, byte[] packetData)
         {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < packetData.Length; i++)
            {
               char ch = (char)packetData[i];
               if (ch == '\0')
               {
                  break;
               }
               sb.Append(ch);
            }

            mSymbolName = sb.ToString();
         }
         
      };


      //==============================================================================
      // loadSymbolInfo
      //==============================================================================
      static SymbolInfo mSymbolInfo = new SymbolInfo();
      static public void loadSymbolInfo(uint procHandle, string exeName)
      {
         mSymbolInfo.init((int)procHandle, exeName);
      }
      //==============================================================================
      // loadSymbolInfo
      //==============================================================================
      static public void closeSymbolInfo()
      {
         mSymbolInfo.deinit();
      }

      //==============================================================================
      // getSymbolInfo
      //==============================================================================
      static public SymbolInfo getSymbolInfo()
      {
         return mSymbolInfo;
      }

   };

   //---------------------------------------
   //---------------------------------------
   class Xbox_EndianSwap
   {
      //-----------------------------------------------------------------------------
      public static ushort endSwapI16(ushort i)
      {
         return (ushort)((i << 8) | (i >> 8));
      }
      //-----------------------------------------------------------------------------
      public static int endSwapI32(int i)
      {
         return endSwapI16((ushort)(i & 0x0000FFFF)) << 16 | endSwapI16((ushort)(i >> 16));
      }
      //-----------------------------------------------------------------------------
      public static Int64 endSwapI64(Int64 i)
      {
         Int64 big = endSwapI32((Int32)(i & 0x00000000FFFFFFFF));
         big = big << 32;
         Int32 small = endSwapI32((Int32)(i >> 32));
         return (Int64)(big | small);
      }

      //-----------------------------------------------------------------------------
      public static float endSwapF32(float f)
      {
         byte[] b = BitConverter.GetBytes(f);
         Array.Reverse(b);

         return BitConverter.ToSingle(b, 0);
      }
   }
}
