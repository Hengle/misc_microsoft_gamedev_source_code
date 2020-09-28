using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace ScnMemEst
{
   public class ECF
   {
      protected uint cXTD_ECFFileID = unchecked((uint)0x77826);
      protected uint cECFMaxChunks = 32768;
      protected uint cECFMaxChunkSize = (int)(1024U * 1024U * 1024U);
      protected uint cECFHeaderMagic = unchecked((uint)0xDABA7737);
      protected uint cECFInvertedHeaderMagic = unchecked((uint)0x3777BADA);
      protected uint cECFAdler32DWORDsToSkip = 3;

      [StructLayout(LayoutKind.Sequential)]
      public class ECFChunkHolder
      {
         public MemoryStream mDataMemStream;
         public void Close()
         {
            mDataMemStream.Close();
            mDataMemStream = null;
         }
      }

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct ECFHeader
      {
         public uint mHeaderMagic;
         public uint mHeaderSize;
         public uint mHeaderAdler32;

         public uint mFileSize;

         public ushort mNumChunks;
         public ushort mFlags;

         public uint mID;

         public ushort mChunkExtraDataSize;
         public ushort mPad0;

         public uint mPad1;

         public void endianSwap()
         {
            mHeaderMagic = (uint)Xbox_EndianSwap.endSwapI32((int)mHeaderMagic);
            mHeaderSize = (uint)Xbox_EndianSwap.endSwapI32((int)mHeaderSize);
            mHeaderAdler32 = (uint)Xbox_EndianSwap.endSwapI32((int)mHeaderAdler32);
            mFileSize = (uint)Xbox_EndianSwap.endSwapI32((int)mFileSize);// totalheaderSize;
            mNumChunks = (ushort)Xbox_EndianSwap.endSwapI16((ushort)mNumChunks);
            mFlags = (ushort)Xbox_EndianSwap.endSwapI16((ushort)mFlags);
            mID = (uint)Xbox_EndianSwap.endSwapI32((int)mID);
            mChunkExtraDataSize = 0;
         }
         public static int giveSize()
         {
            return sizeof(uint) * 6 + sizeof(ushort) * 4;
         }

      };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct BECFChunkHeader
      {
         public Int64 mID;
         public Int32 mOfs;
         public Int32 mSize;
         public Int32 mAdler32;

         public byte mFlags;
         public byte mAlignmentLog2;

         public Int16 mPad0;
         public void endianSwap()
         {
            mID = Xbox_EndianSwap.endSwapI64(mID);
            mOfs = Xbox_EndianSwap.endSwapI32(mOfs);
            mSize = Xbox_EndianSwap.endSwapI32(mSize);
            mAdler32 = Xbox_EndianSwap.endSwapI32(mAdler32);

            //public byte mFlags;
            //public byte mAlignmentLog2;

            //public Int16 mPad0;
         }
         public static int giveSize()
         {
            return sizeof(Int64) +
               (sizeof(Int32) * 3) +
               (2) +
               sizeof(Int16);
         }


      };

      [StructLayout(LayoutKind.Sequential)]
      public class BECFChunkData
      {

         public BECFChunkData(int id, MemoryStream binWriter, int size, bool calcAdler)
         {
            mID = id;
            mMemStream = binWriter;
            mLength = size;
            mHeaderAdler32 = 0;

            if (calcAdler)
            {
               BinaryReader br = new BinaryReader(mMemStream);
               mHeaderAdler32 = calcAdler32(br.ReadBytes((int)mLength), 0, (uint)mLength);
               mMemStream.Position = 0;
               //br.Close();
            }
         }
         ~BECFChunkData()
         {
            destroy();
         }

         public void destroy()
         {
            if (mMemStream != null)
            {
               mMemStream.Close();
               mMemStream = null;
            }
         }

         public int mID;
         public MemoryStream mMemStream;
         public long mLength;
         public uint mHeaderAdler32;
      };


      static public unsafe uint calcAdler32(byte[] p, uint start, uint len)
      {
         //CLM FROM XCORE\HASH\ADLER32.H ADLER=1!
         uint adler = 1;

         const long BASE = 65521L;
         const long NMAX = 5552;

         int Pbuf = (int)start;// (byte*)p;
         uint s1 = adler & 0xffff;
         uint s2 = (adler >> 16) & 0xffff;
         int k;

         if (p == null) return 1;

         while (len > 0)
         {
            k = (int)((len < NMAX) ? len : NMAX);

            len -= (uint)k;

            while (k >= 16)
            {
               //DO16(Pbuf);
               for (int i = 0; i < 16; i++)
               { s1 += p[Pbuf++]; s2 += s1; }

               k -= 16;
            }

            if (k != 0)
            {
               do
               {
                  s1 += p[Pbuf++];
                  s2 += s1;
               } while (--k > 0);
            }

            s1 %= (uint)BASE;
            s2 %= (uint)BASE;
         }

         return (s2 << 16) | s1;
      }


      static public byte[] StructToByteArray(object _oStruct)
      {
         try
         {
            // This function copys the structure data into a byte[] 

            //Set the buffer ot the correct size 
            byte[] buffer = new byte[Marshal.SizeOf(_oStruct)];

            //Allocate the buffer to memory and pin it so that GC cannot use the 
            //space (Disable GC) 
            GCHandle h = GCHandle.Alloc(buffer, GCHandleType.Pinned);

            // copy the struct into int byte[] mem alloc 
            Marshal.StructureToPtr(_oStruct, h.AddrOfPinnedObject(), false);

            h.Free(); //Allow GC to do its job 

            return buffer; // return the byte[]. After all thats why we are here 
            // right. 
         }
         catch (Exception ex)
         {
            throw ex;
         }
      }

   };

   public class ECFWriter : ECF
   {
      public ECFWriter()
      {

      }
      ~ECFWriter()
      {
         destroy();
      }
      public void destroy()
      {
         if (mChunks != null)
         {
            for (int i = 0; i < mChunks.Count; i++)
               mChunks[i] = null;
            mChunks.Clear();
            mChunks = null;
         }
      }

      private FileStream mTempChunkFile = null;
      private BinaryWriter mTempFileWriter = null;
      private string mStaticTempName = "_ecfExportTemp";

      private void openCreateTempFiles()
      {
         if (File.Exists(mStaticTempName))
            File.Delete(mStaticTempName);
         mTempChunkFile = File.Open(mStaticTempName, FileMode.OpenOrCreate, FileAccess.ReadWrite);
         mTempFileWriter = new BinaryWriter(mTempChunkFile);


      }
      private void cleanTempFiles()
      {
         if (mTempChunkFile != null)
         {
            mTempChunkFile.Close();
            mTempChunkFile = null;
         }
         if (mTempFileWriter != null)
         {
            mTempFileWriter.Close();
            mTempFileWriter = null;

         }
         if (File.Exists(mStaticTempName))
            File.Delete(mStaticTempName);
      }


      public unsafe bool writeToFile(string filename)
      {
         if (File.Exists(filename))
            File.Delete(filename);
         FileStream s = File.Open(filename, FileMode.OpenOrCreate, FileAccess.ReadWrite);
         BinaryWriter f = new BinaryWriter(s);
         BinaryReader r = new BinaryReader(s);

         //open reading from our temp file..
         mTempChunkFile.Position = 0;
         BinaryReader tbr = new BinaryReader(mTempChunkFile);


         //WRITE OUR HEADER
         mHeader = new ECFHeader();
         mHeader.mHeaderMagic = cECFHeaderMagic;
         mHeader.mHeaderSize = (uint)sizeof(ECFHeader);                  //THIS IS JUST THE SIZE OF THIS HEADER 


         f.Write(StructToByteArray(mHeader));




         /////////////////////////////////////////
         //WRITE OUR CHUNK HEADERS (Dummies!)
         BECFChunkHeader[] headers = new BECFChunkHeader[mChunks.Count];
         ushort padVal = 0;
         Int32 padVal32 = 0;

         BECFChunkHeader ch = new BECFChunkHeader();
         for (int i = 0; i < mChunks.Count; i++)
         {
            headers[i].mID = mChunks[i].mID;
            headers[i].mOfs = (int)f.BaseStream.Position;
            headers[i].mSize = (int)mChunks[i].mLength;
            headers[i].mAdler32 = (int)mChunks[i].mHeaderAdler32;//THIS IS THE ADLER FOR THE ACTUAL CHUNK DATA WE REPRESENT (Post endiean converted!)

            headers[i].mFlags = 0;
            headers[i].mAlignmentLog2 = 2;
            headers[i].mPad0 = (short)padVal;
            //headers[i].endianSwap();//not yet....

            f.Write(StructToByteArray(headers[i]));
         }






         ////////////////////////////////////////////////
         //WRITE OUR CHUNK BLOCKS

         for (int i = 0; i < mChunks.Count; i++)
         {


            //CLM [03.13.07] ECF Changes
            //we need to ensure each chunk is aligned to the boundry defined by ch.mAlignmentLog2
            //so check to see if our position pointer is a multiple of 4 (2^2) 
            //if it's not, write the number of bytes required to MAKE it that multiple.
            long pos = f.BaseStream.Position;
            long modPos = pos & 3;
            if (modPos != 0)
            {
               long numBytesToWrite = 4 - modPos;
               byte[] b = new byte[numBytesToWrite];
               f.Write(b);
            }

            long streampos = f.BaseStream.Length;

            f.Write(tbr.ReadBytes((int)mChunks[i].mLength));

            //fill in our header data
            headers[i].mOfs = (int)streampos;


            //seek back to our header and update our position pointer
            //f.Seek(sizeof(ECFHeader) + (sizeof(BECFChunkHeader) * i) + sizeof(Int64), SeekOrigin.Begin);
            //f.Write(Xbox_EndianSwap.endSwapI32((int)streampos));
            //f.Seek(0, SeekOrigin.End);
         }





         //REWRITE OUR HEADER WITH THE PROPER DATA

         //write our actual file length back to the header
         mHeader.mFileSize = (uint)f.BaseStream.Length;// totalheaderSize;
         mHeader.mNumChunks = (ushort)mChunks.Count;
         mHeader.mFlags = 0;// mFlags;
         mHeader.mID = cXTD_ECFFileID;
         mHeader.mChunkExtraDataSize = 0;
         mHeader.endianSwap();

         //CLM we have to calculate the adler of the endianswapped data, then endian swap ourselves
         mHeader.mHeaderAdler32 = calcAdler32(StructToByteArray(mHeader),
                                     (uint)(sizeof(Int32) * cECFAdler32DWORDsToSkip),
                                     (uint)(sizeof(ECFHeader) - sizeof(Int32) * cECFAdler32DWORDsToSkip));
         mHeader.mHeaderAdler32 = (uint)Xbox_EndianSwap.endSwapI32((int)mHeader.mHeaderAdler32);

         f.Seek(0, SeekOrigin.Begin);
         f.Write(StructToByteArray(mHeader));
         // f.Seek(0, SeekOrigin.End);

         /////////////////////////////////////////
         //WRITE OUR CHUNK HEADERS (Real!)
         for (int i = 0; i < mChunks.Count; i++)
         {
            headers[i].endianSwap();
            f.Write(StructToByteArray(headers[i]));
         }



         headers = null;


         f.Close();
         f = null;
         s.Close();
         s = null;

         tbr.Close();
         tbr = null;

         clear();
         return true;
      }

      short mChunkExtraDataSize = 0;
      ECFHeader mHeader;
      List<BECFChunkData> mChunks = null;
      long mTotalSize = 0;

      public void clear()
      {
         if (mChunks != null)
         {
            for (int i = 0; i < mChunks.Count; i++)
            {
               mChunks[i].destroy();
               mChunks[i] = null;
            }

            mChunks.Clear();
            mChunks = null;
         }


         cleanTempFiles();
      }

      public int addChunk(int chunkID, ECFChunkHolder chunkHolder, long len)
      {
         if (mChunks == null)
            mChunks = new List<BECFChunkData>();

         if (mTempFileWriter == null)
            openCreateTempFiles();

         mTotalSize += len;

         //write this chunk to our temp file instead..
         chunkHolder.mDataMemStream.Position = 0;

         BECFChunkData dat = new BECFChunkData(chunkID, chunkHolder.mDataMemStream, (int)len, true);
         BinaryReader br = new BinaryReader(chunkHolder.mDataMemStream);
         mTempFileWriter.Write(br.ReadBytes((int)chunkHolder.mDataMemStream.Length));
         br.Close();

         dat.mMemStream.Close();
         dat.mMemStream = null;

         mChunks.Add(dat);


         return mChunks.Count - 1;
      }

   };
   public class ECFReader : ECF
   {
      FileStream mFileStream = null;
      BinaryReader mBinaryReader = null;

      ECFHeader mFileHeader;
      List<BECFChunkHeader> mChunkHeaders = new List<BECFChunkHeader>();

      public ECFReader()
      {
      }
      ~ECFReader()
      {
         destroy();
      }
      public void destroy()
      {
         if (mChunkHeaders != null)
         {
            mChunkHeaders.Clear();
            mChunkHeaders = null;
         }
      }

      //-------------------------
      void readHeader(BinaryReader f)
      {
         mFileHeader = new ECFHeader();
         mFileHeader.mHeaderMagic = (uint)f.ReadInt32();
         mFileHeader.mHeaderSize = (uint)f.ReadInt32();
         mFileHeader.mHeaderAdler32 = (uint)f.ReadInt32();
         mFileHeader.mFileSize = (uint)f.ReadInt32();
         mFileHeader.mNumChunks = (ushort)f.ReadInt16();
         mFileHeader.mFileSize = (ushort)f.ReadInt16();
         mFileHeader.mID = (uint)f.ReadInt32();
         mFileHeader.mChunkExtraDataSize = (ushort)f.ReadInt16();
         mFileHeader.mPad0 = (ushort)f.ReadInt16();
         mFileHeader.mPad1 = (uint)f.ReadInt32();

         mFileHeader.endianSwap();
      }
      BECFChunkHeader readChunkHeader(BinaryReader f)
      {
         BECFChunkHeader chunkHead = new BECFChunkHeader();
         chunkHead.mID = (Int64)f.ReadInt64();
         chunkHead.mOfs = (Int32)f.ReadInt32();
         chunkHead.mSize = (Int32)f.ReadInt32();
         chunkHead.mAdler32 = (Int32)f.ReadInt32();
         chunkHead.mFlags = (byte)f.ReadByte();
         chunkHead.mAlignmentLog2 = (byte)f.ReadByte();
         chunkHead.mPad0 = (Int16)f.ReadInt16();
         chunkHead.endianSwap();

         return chunkHead;
      }

      //-------------------------

      public bool openForRead(string filename)
      {
         if (!File.Exists(filename))
            return false;

         mFileStream = File.Open(filename, FileMode.Open, FileAccess.Read);
         mBinaryReader = new BinaryReader(mFileStream);

         readHeader(mBinaryReader);

         if (mFileHeader.mHeaderMagic != cECFHeaderMagic)
         {
            close();
            return false;
         }

         //read our chunk headers..
         for (int i = 0; i < mFileHeader.mNumChunks; i++)
            mChunkHeaders.Add(readChunkHeader(mBinaryReader));

         return true;
      }
      public void seekToChunk(int chunkIndex)
      {
         if (chunkIndex < 0 || chunkIndex >= mFileHeader.mNumChunks)
            return;

         int offset = mChunkHeaders[chunkIndex].mOfs;
         mFileStream.Seek(offset, SeekOrigin.Begin);
      }
      public void close()
      {
         mFileStream.Close();
         mFileStream = null;
         mBinaryReader.Close();
         mBinaryReader = null;
      }

      //-------------------------
      public BECFChunkHeader getChunkHeader(uint chunkIndex)
      {
         if (chunkIndex < 0 || chunkIndex >= mFileHeader.mNumChunks)
            return mChunkHeaders[0];

         return mChunkHeaders[(int)chunkIndex];
      }
      public uint getNumChunks()
      {

         return mFileHeader.mNumChunks;
      }
      //-------------------------
      public byte readByte()
      {
         return mBinaryReader.ReadByte();
      }
      public float readSingle()
      {
         return mBinaryReader.ReadSingle();
      }
      public double readDouble()
      {
         return mBinaryReader.ReadDouble();
      }
      public Int32 readInt32()
      {
         return mBinaryReader.ReadInt32();
      }
      public Int16 readInt16()
      {
         return mBinaryReader.ReadInt16();
      }
      public byte[] readBytes(int count)
      {
         return mBinaryReader.ReadBytes(count);
      }
   }

}
