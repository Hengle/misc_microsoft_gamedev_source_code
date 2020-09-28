using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using RemoteTools;
//Thunk!

namespace EnsembleStudios.RemoteGameDebugger
{
   public class MainForm
   {
      public List<NetServiceConnection> mDebuggerClients = new List<NetServiceConnection>();
   }

#region  Networking stuff

   //public class CTSPacketHeader
   //{

   //   static public int swap(int val)
   //   {
   //      long data = 0;
   //      data += (val & 0xff000000) >> 24;
   //      data += (val & 0x00ff0000) >> 8;
   //      data += (val & 0x0000ff00) << 8;
   //      data += (val & 0x000000ff) << 24;
   //      return (int)data;
   //   }

   //   public bool mbXboxDetected = false;
   //   public CTSPacketHeader(BinaryReader reader)
   //   {

   //      //mSize = reader.ReadInt32()-1; // minus one for the type byte below
   //      //mSize = ReadInt32(reader)-1;

   //      int size = reader.ReadInt32();
   //      if ((uint)size > (uint)(0xffff))
   //      {
   //         mbXboxDetected = true;
   //         mSize = swap(size) - 1;
   //      }
   //      else
   //      {
   //         mSize = size - 1;
   //      }
   //      mType = reader.ReadByte();
   //   }

   //   public CTSPacketHeader(BinaryWriter writer, byte type)
   //   {
   //      mType = type;
   //      mSize = 0;
   //      writer.Write(mSize);
   //      writer.Write(type);
   //   }

   //   public void writeSize(BinaryWriter writer)
   //   {
   //      mSize = (int)writer.Seek(0, SeekOrigin.End);
   //      writer.Seek(0, SeekOrigin.Begin);
   //      //int size = swap(mSize);
   //      writer.Write(mSize);
   //   }

   //   public int mSize = 0;
   //   public byte mType = 0;
   //}
   //public class SClientConnection
   //{
   //   public void write(Stream s)
   //   {

   //   }

   //   //static public BinaryWriter GetBinaryWriter(Stream stream)
   //   //{
   //   //   return new BinaryWriter(stream);
   //   //}
   //   static public bool mbIsXbox = false;
   //   static public bool mbIsAnsiBStringClient = true;
   //   static public BinaryWriter GetBinaryWriter(Stream s)
   //   {
   //      if (mbIsXbox)
   //      {
   //         return new XBoxBinaryWriter(s);
   //      }
   //      else if (mbIsAnsiBStringClient)
   //      {
   //         return new BinaryWriter(s, System.Text.Encoding.ASCII);
   //      }
   //      else
   //      {
   //         return new BinaryWriter(s, System.Text.Encoding.Unicode);
   //      }
   //   }
   //   static public BinaryReader GetBinaryReader(Stream s)
   //   {

   //      if (mbIsXbox)
   //      {
   //         return new XBoxBinaryReader(s);
   //      }
   //      else if (mbIsAnsiBStringClient)
   //      {
   //         return new BinaryReader(s, System.Text.Encoding.ASCII);
   //      }
   //      else
   //      {
   //         return new BinaryReader(s, System.Text.Encoding.Unicode);
   //      }

   //   }
   //}


   //public class XBoxBinaryReader : BinaryReader
   //{
   //   public XBoxBinaryReader(Stream s)
   //      : base(s, System.Text.Encoding.ASCII)
   //   {

   //   }
   //   public override short ReadInt16()
   //   {
   //      return System.BitConverter.ToInt16(ReverseReadNBytes(2), 0);
   //   }
   //   public override int ReadInt32()
   //   {
   //      return System.BitConverter.ToInt32(ReverseReadNBytes(4), 0);
   //   }
   //   public override long ReadInt64()
   //   {
   //      return System.BitConverter.ToInt64(ReverseReadNBytes(8), 0);
   //   }
   //   public override ushort ReadUInt16()
   //   {
   //      return System.BitConverter.ToUInt16(ReverseReadNBytes(2), 0);
   //   }
   //   public override uint ReadUInt32()
   //   {
   //      return System.BitConverter.ToUInt32(ReverseReadNBytes(4), 0);
   //   }
   //   public override ulong ReadUInt64()
   //   {
   //      return System.BitConverter.ToUInt64(ReverseReadNBytes(8), 0);
   //   }
   //   public override float ReadSingle()
   //   {
   //      return System.BitConverter.ToSingle(ReverseReadNBytes(4), 0);
   //   }
   //   public override double ReadDouble()
   //   {
   //      return System.BitConverter.ToDouble(ReverseReadNBytes(8), 0);
   //   }

   //   //We could use "System.BitConverter.IsLittleEndian = false;", but being global and static scares me..
   //   public byte[] ReverseReadNBytes(int count)
   //   {
   //      byte[] data = new byte[count];
   //      data = base.ReadBytes(count);
   //      Array.Reverse(data, 0, count);
   //      return data;
   //   }

   //   //use this if the encoding type is not ASCII
   //   public string ReadAsciString(BinaryReader reader)
   //   {
   //      byte length = reader.ReadByte();
   //      byte[] bytes = reader.ReadBytes(length);
   //      return System.Text.Encoding.ASCII.GetString(bytes, 0, length);
   //   }
   //}
   //public class XBoxBinaryWriter : BinaryWriter
   //{
   //   public XBoxBinaryWriter(Stream s)
   //      : base(s, System.Text.Encoding.ASCII)
   //   {

   //   }
   //   public XBoxBinaryWriter(Stream s, System.Text.Encoding e)
   //      : base(s, e)
   //   {

   //   }
   //   public override void Write(int value)
   //   {
   //      base.Write(swap(value));
   //   }
   //   public override void Write(ulong value)
   //   {
   //      //hmm, I wonder if this is slower or faster than swap()?

   //      //Should I try some unsafe code?
   //      MemoryStream m = new MemoryStream();
   //      BinaryWriter b = new BinaryWriter(m);
   //      b.Write(value);
   //      b.Close();
   //      byte[] bytes = m.ToArray();
   //      Array.Reverse(bytes);
   //      base.Write(bytes, 0, bytes.Length);
   //   }

   //   static public int swap(int val)
   //   {
   //      long data = 0;
   //      data += (val & 0xff000000) >> 24;
   //      data += (val & 0x00ff0000) >> 8;
   //      data += (val & 0x0000ff00) << 8;
   //      data += (val & 0x000000ff) << 24;
   //      return (int)data;
   //   }


   //}


#endregion

   //#region Error Handler
   //public delegate void ErrorEvent(string error);
   //public class ErrorHandler
   //{
   //   static public void Error(string error)
   //   {
   //      try
   //      {
   //         if (mStrategy != null)
   //         {
   //            mStrategy.Error(error);
   //         }
   //         if (OnError != null)
   //         {
   //            OnError(error);
   //         }
   //      }
   //      catch (System.Exception ex)
   //      {
   //         ex.ToString();
   //      }
   //   }
   //   static ErrorHandlerStrategy mStrategy = new ErrorHandlerLogStrategy();
   //   public static event ErrorEvent OnError = null;
   //}
   //interface ErrorHandlerStrategy
   //{
   //   void Error(string error);
   //}
   //class ErrorHandlerLogStrategy : ErrorHandlerStrategy
   //{
   //   string mFilename = "ErrorLog.txt";
   //   public ErrorHandlerLogStrategy()
   //   {
   //      mFilename = "Log " + System.DateTime.Now.ToString().Replace(":", "_").Replace("/", "_") + ".txt";
   //   }
   //   public void Error(string error)
   //   {
   //      lock (this)
   //      {
   //         StreamWriter writer = new StreamWriter(mFilename, true);
   //         writer.WriteLine(System.DateTime.Now.ToString() + " " + error);
   //         writer.Close();
   //      }
   //   }
   //}
   //#endregion


}
