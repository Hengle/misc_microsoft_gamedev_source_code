using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace RemoteMemory
{
   public class SymbolInfo
   {
      class LineDesc
      {
         public uint mAddress;
         public short mLine;
         public uint mFilenameIndex;
      }

      class SymDesc
      {
         public uint mAddress;
         public uint mSize;
         public uint mNameIndex;
      }

      List<String> mFileNames = new List<string>();
      List<SymDesc> mSymbolDat = new List<SymDesc>();
      List<String> mSymbolNames = new List<string>();

      List<String> mIgnoreList = new List<String>();

      UInt32 hProcess = 0x0007BEEF;
      UInt64 pvModuleBase = 0;

      bool mDoEnumerate = true;//

     
      SortedAddressHashTree mAddressHash = new SortedAddressHashTree();

      //============================================================================
      // load
      //============================================================================
      public bool init(int procHandle,string filename)
      {
         hProcess = (uint)procHandle;

         bool dointrusive = false;
         if (mDoEnumerate)
         {
            dointrusive = false;
            DbgHelp.SymSetOptions(DbgHelp.SymOpt.UNDNAME);
         }
         else
         {
            dointrusive = false;// true;
            DbgHelp.SymSetOptions(DbgHelp.SymOpt.LOAD_LINES);
         }

         string pathToPDB = Path.GetDirectoryName(filename);
         if (!DbgHelp.SymInitialize(hProcess, pathToPDB, dointrusive))
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }

         IntPtr pZero = new IntPtr(0);

         DbgHelp.MODLOAD_DATA modLoadDat = new DbgHelp.MODLOAD_DATA();

         ///////////////////////////////////////////////
         //load module
         pvModuleBase = (UInt64)DbgHelp.SymLoadModuleEx(hProcess, pZero, filename, null, 0, 0, modLoadDat, 0);
         if (pvModuleBase == 0)
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }

         pvModuleBase = (UInt64)((UInt32)(pvModuleBase));


         /////////////////ENUMERATION

         if (mDoEnumerate)
         {
            ///////////////////////////////////////////////
            //enumerate lines
            bool OK = DbgHelp.SymEnumLines(hProcess, pvModuleBase, null, null, enumLinesCallback, pZero);
            if (!OK)
            {
               Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
               return false;
            }

            mAddressHash.sortList();
            ///////////////////////////////////////////////
            //enumerate symbols
            //OK = DbgHelp.SymEnumSymbols(hProcess, pvModuleBase, null, enumSymbolsCallback, pZero);//pvModuleBase);
            //if (!OK)
            //{
            //   Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            //   return false;
            //}
         }
      


         return true;
      }
      //============================================================================
      // close
      //============================================================================
      public void deinit()
      {
         DbgHelp.SymUnloadModule64(hProcess, pvModuleBase);
         DbgHelp.SymCleanup(hProcess);
         mAddressHash = null;
      }


      //============================================================================
      // enumLinesCallback
      //============================================================================
      bool enumLinesCallback(ref DbgHelp.SRCCODE_INFO LineInfo, IntPtr UserContext)
      {

         int index = mFileNames.IndexOf(LineInfo.FileName);

         if(index ==-1)
         {
            index = mFileNames.Count;
            mFileNames.Add(LineInfo.FileName);
         }

         LineDesc ld = new LineDesc();
         ld.mAddress = (uint)LineInfo.Address;
         ld.mFilenameIndex = (uint)index;
         ld.mLine = (short)LineInfo.LineNumber;



         return mAddressHash.addAddress((uint)LineInfo.Address, ld);
      }

      //============================================================================
      // enumSymbolsCallback
      //============================================================================
      bool enumSymbolsCallback(ref  DbgHelp.SYMBOL_INFO pSymInfo, uint SymbolSize, IntPtr UserContext)
      {

         int index = mSymbolNames.IndexOf(pSymInfo.Name);

         if(index ==-1)
         {
            index = mSymbolNames.Count;
            mSymbolNames.Add(pSymInfo.Name);
         }


         SymDesc sd = new SymDesc();
         sd.mAddress = (uint)(pSymInfo.Address >> 32);
         sd.mSize = SymbolSize;
         sd.mNameIndex = (uint)index;// pSymInfo.Name;// +4;

         //insert based upon address
         for (int i = 0; i < mSymbolDat.Count; i++)
         {
            if (mSymbolDat[i].mAddress < sd.mAddress)
            {
               mSymbolDat.Insert(i, sd);
               break;
            }
         }


         //gXDBFileBuilder.addSymbol((DWORD)(pSymInfo->Address >> 32U), SymbolSize, pSymInfo->Name + 4);

         return true;
      }

      //============================================================================
      // LookupInfo
      //============================================================================
      public class LookupInfo
      {
         public UInt32 mAddress;
         public int mLine;
         public bool mFoundSymbol;
         public bool mFoundLine;
         public string mSymbol;
         public string mFilename;
      };

      //============================================================================
      // lookup
      //============================================================================
      public bool lookup(UInt32 address,ref LookupInfo info)
      {
         if (mDoEnumerate)
            return lookupEnumerated(address, ref info);

         return lookupImmediate(address, ref info);
      }


      public bool lookupEnumerated(UInt32 address,ref LookupInfo info)
      {
         try
         {
            info.mAddress = address;
            info.mLine = -1;
            info.mFilename = "?";
            info.mSymbol = "?";
            info.mFoundLine = false;
            info.mFoundSymbol = false;
            
             
            for(int i = 0 ; i < mSymbolDat.Count;i++)
            {
               if(mSymbolDat[i].mAddress == address)
               {
                  info.mFoundSymbol = true;
                  info.mSymbol = mSymbolNames[(int)mSymbolDat[i].mNameIndex];
                  break;
               }
            }

          
            {

               LineDesc ld = (LineDesc)mAddressHash.getObjNearAddress(address);

               if(ld != null)
               {
                  info.mFoundLine = true;
                  info.mLine = ld.mLine;
                  info.mFilename = mFileNames[(int)ld.mFilenameIndex];
                  return true;
               }
            }
            

         }
         catch(Exception e)
         {

         }
         return false;

      }

      public bool lookupImmediate(UInt32 address,ref LookupInfo info)
      {
         info.mAddress = address;
         info.mLine = -1;
         info.mFilename = "?";
         info.mSymbol = "?";
         info.mFoundLine = false;
         info.mFoundSymbol = false;


         ///////////////////////////////////////////////////
         //lookup filename
         byte[] fName = new byte[1000];

         uint displacement = 0;
         DbgHelp._IMAGEHLP_LINE64 lineinfo = new DbgHelp._IMAGEHLP_LINE64();
         lineinfo.SizeOfStruct = (uint)DbgHelp._IMAGEHLP_LINE64.getSize();


         unsafe
         {
            fixed (byte* pCst = fName)
            {
               //IntPtr ptr = new IntPtr()
               lineinfo.FileName = new IntPtr(pCst);
               info.mFoundLine = DbgHelp.SymGetLineFromAddr64(hProcess, (UInt64)address, ref displacement, ref lineinfo);
            }

         }

         if (!info.mFoundLine)
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }
         else
         {
            info.mLine = (int)lineinfo.LineNumber;

            info.mFilename = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(lineinfo.FileName);
         }

         



         ///////////////////////////////////////////////////
         //lookup symbol
         ulong pdwDisplacment = 0;
         DbgHelp.SYMBOL_INFO symbInfo = new DbgHelp.SYMBOL_INFO();
         symbInfo.SizeOfStruct = (uint)DbgHelp.SYMBOL_INFO.getSize();
         symbInfo.MaxNameLen = (int)DbgHelp.SYMBOL_INFO.getmaxCharSize();

         info.mFoundSymbol = DbgHelp.SymFromAddr(hProcess, address, ref pdwDisplacment, ref symbInfo);
         if (!info.mFoundSymbol)
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }
         else
         {
            info.mSymbol = symbInfo.Name;
            info.mFoundSymbol = true;

         }

         

      
        

        

	      
	      
         return true;
      }


      //============================================================================
      // addIgnoreSymbol
      //============================================================================
      public void addIgnoreSymbol(string ignoreSymb)
      {
         mIgnoreList.Add(ignoreSymb);
      }

      //============================================================================
      // isIgnoreSymbol
      //============================================================================
      public bool isIgnoreSymbol(string ignoreSymb)
      {
        return mIgnoreList.Contains(ignoreSymb);
      }

   }


   //============================================================================
   // SortedBinaryTree
   //============================================================================
   class SortedBinaryTree
   {
      class SortedBinaryTreeNode
      {
         public uint mValue=0;
         public object mObj = null;
      };

      List<SortedBinaryTreeNode> mObjectDat = new List<SortedBinaryTreeNode>();
      Hashtable mInsertionTable = new Hashtable();
      public void clear()
      {
         mObjectDat.Clear();
      }
      //============================================================================
      // AddressBucket
      //============================================================================
      public SortedBinaryTree()
      {

      }

      //============================================================================
      // giveIndex
      //============================================================================
      int giveIndex(uint valToFind)
      {
         if (mObjectDat.Count == 0)
            return -1;

         if(mObjectDat.Count < 16)
         {
            return giveIndexLinear(valToFind);
         }

         return giveIndexBinary(valToFind);
      }
      //============================================================================
      // giveIndex
      //============================================================================
      int giveIndexLinear(uint valToFind)
      {
         for(int i = 1 ; i < mObjectDat.Count;i++)
         {
            if (mObjectDat[i].mValue >= valToFind)
               return i;
         }
         return -1;
      }
      //============================================================================
      // giveIndex
      //============================================================================
      int giveIndexBinary(uint valToFind)
      {
         int l = 0;
         int h = mObjectDat.Count - 1;
         while (h >= l)
         {
            int m = (l + h) >> 1;

            uint addr0 = mObjectDat[m].mValue;
            uint addr1 = 0;

            if (m + 1 < mObjectDat.Count)
               addr1 = mObjectDat[m + 1].mValue;
            else
               addr1 = addr0 + 65536 ;

            if (valToFind >= addr0 && valToFind < addr1)
            {
              
               return m;
            }

            if (valToFind < addr0)
               h = m - 1;
            else
               l = m + 1;
         }

         return -1; //NOT FOUND
      }

      //============================================================================
      // addAddress
      //============================================================================
      public void addValue(uint val, object obj)
      {
         if (mInsertionTable.Contains(val))
            return;

         SortedBinaryTreeNode sbtn = new SortedBinaryTreeNode();
         sbtn.mValue = val;
         sbtn.mObj = obj;

         mObjectDat.Add(sbtn);
         mInsertionTable.Add(val, val);
      }
      //============================================================================
      // addAddress
      //============================================================================
      public void sortList()
      {
         mObjectDat.Sort(delegate(SortedBinaryTreeNode p1, SortedBinaryTreeNode p2) { return p1.mValue.CompareTo(p2.mValue); });

      }
      //============================================================================
      // getObjectNearValue
      //============================================================================
      public object getObjectNearValue(uint val)
      {
         int idx = giveIndex(val);
         if (idx != -1)
            return mObjectDat[idx].mObj;

         return null;
      }
      //============================================================================
      // addAddress
      //============================================================================
      public int count()
      {
         return mObjectDat.Count;
      }

   };

   //============================================================================
   // SortedHashTree
   //============================================================================
   public class SortedAddressHashTree
   {
      uint mMinValue = uint.MaxValue;
      uint mMaxValue = uint.MinValue;
      uint mNumBuckets = 256;
      uint mRangePerBucket = 256;

      Hashtable mAddressBuckets = new Hashtable(); //KEY is MINADDRESS, value is SortedBinaryTree

      uint mMinBucket = uint.MaxValue;
      uint mMaxBucket = uint.MinValue;
      uint mMinSearched = uint.MaxValue;
      uint mMaxSearched = uint.MinValue;
      uint mMaxEntriesInBucket = uint.MinValue;
      //============================================================================
      // SortedAddressHashTree
      //============================================================================
      public SortedAddressHashTree()
      {
         mMinValue = 0;
         mMaxValue = 0xFFFFFFFF;// 0x00DC8000;
         
         mNumBuckets = 16;// mMaxValue / mRangePerBucket;
         mRangePerBucket = mMaxValue / mNumBuckets;//0xFF ;
         
         //initialize our address buckets
         {
            uint minAddr = 0;

            uint i = 0;
          //  for (i = 0; i < mNumBuckets - 1; i++)
          //     mAddressBuckets.Add(mRangePerBucket * i, new SortedBinaryTree());

          //  mAddressBuckets.Add(mRangePerBucket * i, new SortedBinaryTree());
         }
      }
      //============================================================================
      // addAddress
      //============================================================================
      public void sortList()
      {
         IDictionaryEnumerator _enum = mAddressBuckets.GetEnumerator();
         while(_enum.MoveNext())
            ((SortedBinaryTree)_enum.Value).sortList();
      }

      //============================================================================
      // addAddress
      //============================================================================
      uint getHashKey(uint address)
      {
         uint hashKey = (address / mRangePerBucket);
         return hashKey;
      }
      //============================================================================
      // addAddress
      //============================================================================
      public bool addAddress(uint addr, object obj)
      {
         uint hashKey = getHashKey(addr);
         if (!mAddressBuckets.Contains(hashKey))
         {
            mAddressBuckets.Add(hashKey, new SortedBinaryTree());
            if (hashKey < mMinBucket) mMinBucket = hashKey;
            if (hashKey > mMaxBucket) mMaxBucket = hashKey;
            if(mAddressBuckets.Count >= mNumBuckets)
                  return false;
         }

         SortedBinaryTree ab = (SortedBinaryTree)mAddressBuckets[hashKey];
         ab.addValue(addr, obj);
         if (ab.count() >= mRangePerBucket)
            return false;
         if (addr < mMinSearched) mMinSearched = addr;
         if (addr > mMaxSearched) mMaxSearched = addr;
         if (ab.count() > mMaxEntriesInBucket)
            mMaxEntriesInBucket = (uint)ab.count();

         return true;
      }

      //============================================================================
      // getObjNearAddress
      //============================================================================
      public object getObjNearAddress(uint address)
      {
         uint hashKey = getHashKey(address);

         SortedBinaryTree ab = (SortedBinaryTree)mAddressBuckets[hashKey];
         if (ab == null)
            return null;
         
         return ab.getObjectNearValue(address);
            
         
      }
   };
}
