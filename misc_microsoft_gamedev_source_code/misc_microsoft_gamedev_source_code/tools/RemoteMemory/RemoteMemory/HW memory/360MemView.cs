using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   class XBOXMemView
   {

      static public int TotalPhysicalMemory
      {
         get
         {
            return 512 * (1024 * 1024); //512MB
         }
      }
      static public uint TotalVirtualMemory
      {
         get
         {
            return UInt32.MaxValue; //4GB
         }
      }
     

      //============================================================================
      // vIRTUAL MEMORY ADDRESS RANGES
      //============================================================================
      public static ulong MM_VIRTUAL_4KB_BASE = ((ulong)0x00000000);
      public static ulong MM_VIRTUAL_4KB_END = ((ulong)0x3FFFFFFF);
      public static ulong MM_VIRTUAL_64KB_BASE = ((ulong)0x40000000);
      public static ulong MM_VIRTUAL_64KB_END = ((ulong)0x7FFFFFFF);
      public static ulong MM_IMAGE_64KB_BASE = ((ulong)0x80000000);
      public static ulong MM_IMAGE_64KB_END = ((ulong)0x8BFFFFFF);
      public static ulong MM_ENCRYPTED_64KB_BASE = ((ulong)0x8C000000);
      public static ulong MM_ENCRYPTED_64KB_END = ((ulong)0x8DFFFFFF);
      public static ulong MM_IMAGE_4KB_BASE = ((ulong)0x90000000);
      public static ulong MM_IMAGE_4KB_END = ((ulong)0x9FFFFFFF);
      public static ulong MM_PHYSICAL_64KB_BASE = ((ulong)0xA0000000);
      public static ulong MM_PHYSICAL_64KB_END = ((ulong)0xBFFFFFFF);
      public static ulong MM_PHYSICAL_16MB_BASE = ((ulong)0xC0000000);
      public static ulong MM_PHYSICAL_16MB_END = ((ulong)0xDFFFFFFF);
      public static ulong MM_PHYSICAL_4KB_BASE = ((ulong)0xE0000000);
      public static ulong MM_PHYSICAL_4KB_END = ((ulong)0xFFFFFFFF);



      //============================================================================
      // convertVirtualAddrToPhysicalAddr
      //============================================================================
      static public uint convertVirtualAddrToPhysicalAddr(uint pMemAddr)
      {
         // AND with this value as part of the calculation to convert from a 
         // virtual memory pointer to a physical address that can be used by the GPU:
         uint GPU_ADDRESS_MASK = (uint)(TotalPhysicalMemory - 1);

         return pMemAddr & GPU_ADDRESS_MASK;
      }

      //============================================================================
      // VirtualAddressInfo
      //============================================================================
      public class VirtualAddressInfo
      {

         public enum eMemRegion
         {
            eVirtual = 0,
            eImage = 1,
            ePhysical = 2,
            eEncrypted = 3,
         };

         public enum ePageSize
         {
            e4k = 4096,
            e64k = 65536,
            e16m = 16777216
         }

         public uint pAddress = 0;

         

         public eMemRegion mMemRegion = eMemRegion.eVirtual;
         public ePageSize mSizeOfContainingPage = ePageSize.e4k;

         public uint mPhysicalAddress = 0;
      };
      //============================================================================
      // convertVirtualAddrToPhysicalAddr
      //============================================================================
      static public VirtualAddressInfo getVirtualAddressInfo(uint pMemAddr)
      {
         VirtualAddressInfo vai = new VirtualAddressInfo();
         vai.pAddress = pMemAddr;

         ulong ulAddress = (ulong)pMemAddr;
         if (ulAddress <= MM_VIRTUAL_4KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.ePhysical;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e4k;
         }
         else if (ulAddress <= MM_VIRTUAL_64KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.eVirtual;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e64k;
         }
         else if (ulAddress <= MM_IMAGE_64KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.eImage;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e64k;
         }
         else if (ulAddress <= MM_ENCRYPTED_64KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.eEncrypted;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e64k;
         }
         else if (ulAddress <= MM_IMAGE_4KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.eImage;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e4k;
         }
         else if (ulAddress <= MM_PHYSICAL_64KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.ePhysical;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e64k;
         }
         else if (ulAddress <= MM_PHYSICAL_16MB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.ePhysical;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e16m;
         }
         else if (ulAddress <= MM_PHYSICAL_4KB_END)
         {
            vai.mMemRegion = VirtualAddressInfo.eMemRegion.ePhysical;
            vai.mSizeOfContainingPage = VirtualAddressInfo.ePageSize.e4k;
         }

         vai.mPhysicalAddress = convertVirtualAddrToPhysicalAddr(pMemAddr);

         return vai;
      }


   }
}