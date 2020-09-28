using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   public class SymbolInfo
   {
      class LineDesc
      {
         public uint mAddress;
         public short mLine;
         public string mFilename;
      }

      class SymDesc
      {
         public uint mAddress;
         public uint mSize;
         public string mNameOffset;
      }
      List<LineDesc> mLineDat = new List<LineDesc>();
      List<SymDesc> mSymbolDat = new List<SymDesc>();

      IntPtr hProcess = new IntPtr(0);
      UInt64 pvModuleBase = 0;

      //============================================================================
      // load
      //============================================================================
      public bool load(string filename)
      {
         DbgHelp.SymSetOptions(DbgHelp.SymOpt.UNDNAME);

         hProcess = new IntPtr(0);
         if(!DbgHelp.SymInitialize(hProcess, "", false))
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }

         IntPtr pZero = new IntPtr(0);

         DbgHelp.MODLOAD_DATA modLoadDat = new DbgHelp.MODLOAD_DATA();

         pvModuleBase = (UInt64)DbgHelp.SymLoadModuleEx(hProcess, pZero, filename, null, 0, 0, modLoadDat, 0);
         if (pvModuleBase == 0)
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }

         pvModuleBase = (UInt64)((UInt32)(pvModuleBase));


         //enumerate symbols
         DbgHelp.SymEnumSymbolsProc enumSymbolsCB = new DbgHelp.SymEnumSymbolsProc(this.enumSymbolsCallback);

         System.Runtime.InteropServices.GCHandle gch = System.Runtime.InteropServices.GCHandle.Alloc(enumSymbolsCB);
         IntPtr ip = System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(enumSymbolsCB);
         DbgHelp.SymEnumSymbolsProc cb = null;
         bool OK = true;
            unsafe
            {
           //    cb = (ip.ToPointer() as DbgHelp.SymEnumSymbolsProc);
          //    OK = DbgHelp.SymEnumSymbols(hProcess, pvModuleBase, null, cb, pZero);//pvModuleBase);
            }


         
         if (!OK)
         {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
            return false;
         }
        while (true) { };

         //enumerate lines
         //OK = DbgHelp.SymEnumLines(hProcess, pvModuleBase, null, null, enumLinesCallback, hProcess);
         //if (!OK)
         //{
         //   Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
         //   return false;
         //}


         return true;
      }
      //============================================================================
      // close
      //============================================================================
      public void close()
      {
         DbgHelp.SymUnloadModule64(hProcess, pvModuleBase);
         DbgHelp.SymCleanup(hProcess);

      }


      //============================================================================
      // enumLinesCallback
      //============================================================================
      bool enumLinesCallback(ref DbgHelp.SRCCODE_INFO LineInfo, IntPtr UserContext)
      {
       //  UserContext;

        // gXDBFileBuilder.addLine((DWORD)LineInfo->Address, LineInfo->LineNumber, LineInfo->FileName);
         LineDesc ld = new LineDesc();
         ld.mAddress = (uint)LineInfo.Address;
         ld.mFilename = LineInfo.FileName;
         ld.mLine = (short)LineInfo.LineNumber;
         mLineDat.Add(ld);

         return true;
      }

      //============================================================================
      // enumSymbolsCallback
      //============================================================================
      bool enumSymbolsCallback(ref DbgHelp.SYMBOL_INFO pSymInfo, uint SymbolSize, IntPtr UserContext)
      {
         SymDesc sd = new SymDesc();
         sd.mAddress = (uint)(pSymInfo.Address >> 32);
         sd.mSize = SymbolSize;
         sd.mNameOffset = pSymInfo.Name;// +4;

         //gXDBFileBuilder.addSymbol((DWORD)(pSymInfo->Address >> 32U), SymbolSize, pSymInfo->Name + 4);

         return true;
      }

      //============================================================================
      // enumSymbolsCallback
      //============================================================================
      void getProcForAddress()
      {

      }

      //============================================================================
      // getLineForAddress
      //============================================================================
      public void getLineForAddress(uint pAddr, ref string filePath, ref int lineNum)
      {
         
         char[] ByteArray = new char[10000];
         DbgHelp._IMAGEHLP_LINE64 lineinfo = new DbgHelp._IMAGEHLP_LINE64();

         uint displacement = 0;

         bool result = true;
         unsafe
         {
            fixed (char* p = ByteArray)
            {
               IntPtr MyIntPtr = (IntPtr)p;

               lineinfo.SizeOfStruct = (uint)DbgHelp._IMAGEHLP_LINE64.getSize();
               lineinfo.FileName = MyIntPtr;
               result = DbgHelp.SymGetLineFromAddr64(hProcess, (UInt64)pAddr, ref displacement, ref lineinfo);
            }
         }

         

	      if(result)
	      {
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            sb.Append(ByteArray);
            filePath = sb.ToString();
		      lineNum = (int)lineinfo.LineNumber;
	      }
	      else
	      {
            Int32 errCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
		      filePath = "[unknown]";
		      lineNum = 0;
	      }
      }



   }
}
