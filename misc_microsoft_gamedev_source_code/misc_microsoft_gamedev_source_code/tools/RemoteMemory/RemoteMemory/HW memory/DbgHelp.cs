using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace RemoteMemory
{
   public class DbgHelp
   {
      [Flags]
      public enum SymOpt : uint
      {
         CASE_INSENSITIVE = 0x00000001,
         UNDNAME = 0x00000002,
         DEFERRED_LOADS = 0x00000004,
         NO_CPP = 0x00000008,
         LOAD_LINES = 0x00000010,
         OMAP_FIND_NEAREST = 0x00000020,
         LOAD_ANYTHING = 0x00000040,
         IGNORE_CVREC = 0x00000080,
         NO_UNQUALIFIED_LOADS = 0x00000100,
         FAIL_CRITICAL_ERRORS = 0x00000200,
         EXACT_SYMBOLS = 0x00000400,
         ALLOW_ABSOLUTE_SYMBOLS = 0x00000800,
         IGNORE_NT_SYMPATH = 0x00001000,
         INCLUDE_32BIT_MODULES = 0x00002000,
         PUBLICS_ONLY = 0x00004000,
         NO_PUBLICS = 0x00008000,
         AUTO_PUBLICS = 0x00010000,
         NO_IMAGE_SEARCH = 0x00020000,
         SECURE = 0x00040000,
         SYMOPT_DEBUG = 0x80000000
      };

      [Flags]
      public enum SymFlag : uint
      {
         VALUEPRESENT = 0x00000001,
         REGISTER = 0x00000008,
         REGREL = 0x00000010,
         FRAMEREL = 0x00000020,
         PARAMETER = 0x00000040,
         LOCAL = 0x00000080,
         CONSTANT = 0x00000100,
         EXPORT = 0x00000200,
         FORWARDER = 0x00000400,
         FUNCTION = 0x00000800,
         VIRTUAL = 0x00001000,
         THUNK = 0x00002000,
         TLSREL = 0x00004000,
      }

      [Flags]
      public enum SymTagEnum : uint
      {
         Null,
         Exe,
         Compiland,
         CompilandDetails,
         CompilandEnv,
         Function,
         Block,
         Data,
         Annotation,
         Label,
         PublicSymbol,
         UDT,
         Enum,
         FunctionType,
         PointerType,
         ArrayType,
         BaseType,
         Typedef,
         BaseClass,
         Friend,
         FunctionArgType,
         FuncDebugStart,
         FuncDebugEnd,
         UsingNamespace,
         VTableShape,
         VTable,
         Custom,
         Thunk,
         CustomType,
         ManagedType,
         Dimension
      };

      [StructLayout(LayoutKind.Sequential)]
      public struct SYMBOL_INFO
      {
         public uint SizeOfStruct;
         public uint TypeIndex;
         public ulong Reserved1;
         public ulong Reserved2;
         public uint Reserved3;
         public uint Size;
         public ulong ModBase;
         public SymFlag Flags;
         public ulong Value;
         public ulong Address;
         public uint Register;
         public uint Scope;
         public SymTagEnum Tag;
         public int NameLen;
         public int MaxNameLen;

         [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
         public string Name;

         static public uint getSize()
         {
            return 88;
         }
         static public uint getmaxCharSize()
         {
            return 937;
         }
      };

      [StructLayout(LayoutKind.Sequential)]
      public struct SRCCODE_INFO 
      {
         public uint SizeOfStruct;
         public IntPtr Key;
         public UInt64 ModBase;

         [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 261)]
         public string Obj;//TCHAR Obj[MAX_PATH+1];               //The name of the object file within the module that contains the line.

         [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 261)]
         public string FileName; //TCHAR FileName[MAX_PATH+1];

         public uint LineNumber;
         public UInt64 Address;                        //The virtual address of the first instruction of the line.
      };

      [StructLayout(LayoutKind.Sequential)]
      public struct _IMAGEHLP_LINE64
      {
         public uint SizeOfStruct;
         public uint Key;
         public uint LineNumber;
         public IntPtr FileName;
         public ulong Address;

         static public uint getSize()
         {
            return 24;
         }
      };

      [StructLayout(LayoutKind.Sequential)]
      public struct MODLOAD_DATA 
      {
         public uint ssize;
         public uint ssig;
         public IntPtr data;
         public uint size;
         public uint flags;
      };

      public delegate bool SymEnumSymbolsProc(ref SYMBOL_INFO pSymInfo, uint SymbolSize, IntPtr UserContext);
      public delegate bool SymEnumLinesProc(ref SRCCODE_INFO pSymInfo, IntPtr UserContext);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymInitialize(UInt32 hProcess, string UserSearchPath, bool fInvadeProcess);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern uint SymSetOptions(SymOpt SymOptions);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern ulong SymLoadModule64(UInt32 hProcess, IntPtr hFile, string ImageName, string ModuleName, ulong BaseOfDll, uint SizeOfDll);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern ulong SymLoadModuleEx(UInt32 hProcess, IntPtr hFile, string ImageName, string ModuleName, ulong BaseOfDll, uint SizeOfDll, MODLOAD_DATA loadData, uint flags);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static unsafe extern bool SymEnumSymbols(UInt32 hProcess, ulong BaseOfDll, string Mask, SymEnumSymbolsProc EnumSymbolsCallback, IntPtr UserContext);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymGetLineFromAddr64(UInt32 hProcess, ulong dwAddr, ref uint pdwDisplacement, ref _IMAGEHLP_LINE64 Line);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymFromAddr(UInt32 hProcess, ulong dwAddr, ref ulong pdwDisplacement, ref SYMBOL_INFO symbolInfo);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymEnumSymbolsForAddr(UInt32 hProcess, ulong Address, SymEnumSymbolsProc EnumSymbolsCallback, IntPtr UserContext);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymUnloadModule64(UInt32 hProcess, ulong BaseOfDll);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymCleanup(UInt32 hProcess);

      [DllImport("dbghelp.dll", SetLastError = true)]
      public static extern bool SymEnumLines(UInt32 hProcess, UInt64 BaseOfDll, string Object, string File, SymEnumLinesProc EnumLinesCallback, IntPtr UserContext);


   }
}
