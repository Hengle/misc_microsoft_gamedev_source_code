using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;

namespace xfs
{
   //============================================================================
   // Connection
   //============================================================================
   public class Connection
   {
      const int cRecvBufferSize=1024000;
      const int cSendBufferSize=1024000;
      const int cOpenInitialRead=4;

      const byte cCommandPrefix=(byte)'@';
      const byte cCommandSuffix=(byte)'#';

      enum CommandType : byte
      {
         CreateFile=1,
         CloseHandle,
         ReadFile,
         WriteFile,
         SetFilePointer,
         CreateDirectory,
         GetFileAttributes,
         FindFiles,
         FlushFileBuffers,
         Test,
         Reset,
         GetFileAttributesEx,
      };

      const byte cErrorResponse=255;

      const uint cAccessQuery                   = 0x00000000;
      const uint cAccessGenericRead             = 0x80000000;
      const uint cAccessGenericWrite            = 0x40000000;
                                   
      const uint cFileShareRead                 = 0x00000001;
      const uint cFileShareWrite                = 0x00000002;
      const uint cFileShareDelete               = 0x00000004;

      const int cDispositionCreateNew           = 1;
      const int cDispositionCreateAlways        = 2;
      const int cDispositionOpenExisting        = 3;
      const int cDispositionOpenAlways          = 4;
      const int cDispositionTruncateExisting    = 5;

      const uint cFileAttributeReadOnly         = 0x00000001;
      const uint cFileAttributeHidden           = 0x00000002;
      const uint cFileAttributeSystem           = 0x00000004;
      const uint cFileAttributeDirectory        = 0x00000010;
      const uint cFileAttributeArchive          = 0x00000020;
      const uint cFileAttributeDevice           = 0x00000040;
      const uint cFileAttributeNormal           = 0x00000080;
      const uint cFileAttributeTemporary        = 0x00000100;

      const uint cFileFlagWriteThrough          = 0x80000000;
      const uint cFileFlagOverlapped            = 0x40000000;
      const uint cFileFlagNoBuffering           = 0x20000000;
      const uint cFileFlagRandomAccess          = 0x10000000;
      const uint cFileFlagSequentialScan        = 0x08000000;
      const uint cFileFlagDeleteOnClose         = 0x04000000;
      const uint cFileFlagBackupSemantics       = 0x02000000;

      private int mID = -1;
      private Client mClient = null;
      private ReaderWriterLock mConnectionRWLock = new ReaderWriterLock();
      private int mConnectionLockCount = 0;
      private bool mActive = false;
      private Thread mThread = null;
      private TcpClient mTcpClient = null;
      private NetworkStream mStream = null;
      BinaryReader mReader = null;
      BinaryWriter mWriter = null;
      private byte[] mRecvBuffer = null;
      private byte[] mSendBuffer = null;
      private ArrayList mFileIndexList = null;
      public string mIP = null;
      private static int mThreadNumber = 0;

      //============================================================================
      // Connection constructor
      //============================================================================
      public Connection(int connectionID, Client client)
      {
         mID = connectionID;
         mClient = client;
      }

      //============================================================================
      // Connection.getClient
      //============================================================================
      public Client getClient()
      {
         return mClient;
      }

      //============================================================================
      // Connection.connect
      //============================================================================
      public bool connect(TcpClient tcpClient)
      {
         if(tcpClient==null)
            return false;

         tcpClient.NoDelay = Server.mEnableNoDelay;
         tcpClient.SendBufferSize=32768;
         tcpClient.ReceiveBufferSize=32768;
                  
         mTcpClient = tcpClient;
         mStream = mTcpClient.GetStream();
         mReader = new BinaryReader(mStream);
         mWriter = new BinaryWriter(mStream);
         mRecvBuffer = new byte[cRecvBufferSize];
         mSendBuffer = new byte[cSendBufferSize];
         mFileIndexList = new ArrayList();
         mIP = ((IPEndPoint)tcpClient.Client.RemoteEndPoint).Address.ToString();

         mClient.addConnection(this);
         
         return true;
      }

      //============================================================================
      // Connection::disconnect
      //============================================================================
      public void disconnect()
      {
         mActive = false;

         if (mTcpClient != null)
         {
            mTcpClient.Close();
            mTcpClient = null;
         }

         // closing the tcpclient will not close the stream
         if (mStream != null)
         {
            mStream.Close();
            mStream = null;
         }

         mReader = null;
         mWriter = null;
         mRecvBuffer = null;
         mSendBuffer = null;

         if (mFileIndexList != null)
         {
            foreach (int index in mFileIndexList)
               Server.closeFile(index);
            mFileIndexList.Clear();
            mFileIndexList = null;
         }

         mThread = null;

         if (mClient != null)
         {
            mClient.removeConnection(this);
            mClient = null;
         }
      }

      //============================================================================
      // Connection::start
      //============================================================================
      public void start()
      {
         mActive = true;
         mThread = new Thread(new ParameterizedThreadStart(acceptCommands));
         mThread.IsBackground = true;
         mThread.Name = String.Format("ConnectionThread.{0}", Interlocked.Increment(ref mThreadNumber));
         mThread.Start(this);
      }

      //============================================================================
      // Connection.acceptCommands
      //============================================================================
      public static void acceptCommands(object parameter)
      {
         Connection connection = (Connection)parameter;

         for (; ; )
         {
            CommandType command;
            try
            {
               byte prefix = connection.readByte();
               
               if (prefix != cCommandPrefix)
               {
//                  Server.onDisconnect(connection);
                  break;
               }
               command = (CommandType)connection.readByte();
            }
            catch(EndOfStreamException)
            {
//               Server.onDisconnect(connection);
               break;
            }
            catch
            {
               break;
            }
            if (!Server.lockConnection(connection))
               break;
            try
            {
               bool retval = connection.onCommand(command);
               if (!retval)
                  break;
            }
            catch (System.Exception ex)
            {
               Server.logException(ex);
               break;
            }
            finally
            {
               Server.unlockConnection(connection);
            }
         }
         
         Server.onDisconnect(connection);
      }

      //============================================================================
      // Connection.readByte
      //============================================================================
      byte readByte()
      {
         return mReader.ReadByte();
      }

      //============================================================================
      // Connection.onCommand
      //============================================================================
      bool onCommand(CommandType command)
      {
         bool retval = false;
         
         try
         {
            // rg [6/13/06] - The entire server is locked inside here, so make sure we complete the operation in a reasonable period of time.
            // Otherwise XFS will lock up if the user exits the game while a large I/O is taking place.
            // This could make debugging the 360 side of XFS difficult, so it should be an option.
            // This is a hack, but we're using binary streams instead of the lower-level socket API.
            // jce [10/19/06] - Added hacky option
            if(!Server.mDisableTimeout)
            {
               mTcpClient.ReceiveTimeout = 15000;
               mTcpClient.SendTimeout = 15000;
            }
            
            switch (command)
            {
               case CommandType.CreateFile         : retval = handleCreateFile(); break;
               case CommandType.CloseHandle        : retval = handleCloseHandle(); break;
               case CommandType.ReadFile           : retval = handleReadFile(); break;
               case CommandType.WriteFile          : retval = handleWriteFile(); break;
               case CommandType.SetFilePointer     : retval = handleSetFilePointer(); break;
               case CommandType.CreateDirectory    : retval = handleCreateDirectory(); break;
               case CommandType.GetFileAttributes  : retval = handleGetFileAttributes(); break;
               case CommandType.FindFiles          : retval = handleFindFiles(); break;
               case CommandType.FlushFileBuffers   : retval = handleFlushFileBuffers(); break;
               case CommandType.Test               : retval = handleTest(); break;
               case CommandType.Reset              : retval = handleReset(); break;
               case CommandType.GetFileAttributesEx: retval = handleGetFileAttributesEx(); break;
               default:
                  {
                     Server.output("ERROR - Invalid command");
                     break;
                  }
            }
         }
         finally
         {
            if (mActive)
            {
               mTcpClient.ReceiveTimeout = 0;
               mTcpClient.SendTimeout = 0;
            }
         }            
         
         return retval;
      }

      //============================================================================
      // Connection.getIP
      //============================================================================
      public string getIP()
      {
         return mIP;
      }

      //============================================================================
      // Connection.lockConnection
      //============================================================================
      public bool lockConnection()
      {
         // this is ok to check mActive outside the lock
         // even with the lock, we still have the edge case that it could be
         // set to false after our check and we've already decided to return true
         if (!mActive)
            return false;

         Interlocked.Increment(ref mConnectionLockCount);

         return true;
      }

      //============================================================================
      // Connection.unlockConnection
      //============================================================================
      public void unlockConnection()
      {
         Interlocked.Decrement(ref mConnectionLockCount);
      }

      //============================================================================
      // Connection.handleCreateFile
      //============================================================================
      bool handleCreateFile()
      {
         // Read in parameters
         short fileLen=mReader.ReadInt16();
         char[] fileName=mReader.ReadChars(fileLen);
         uint desiredAccess=mReader.ReadUInt32();
         uint shareMode=mReader.ReadUInt32();
         int creationDisposition=mReader.ReadInt32();
         uint flagsAndAttributes=mReader.ReadUInt32();
         byte suffix=mReader.ReadByte();

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Default some values.
         int fileIndex=-1;
         long fileSize=0;
         FileStream fs=null;
         long fileTime=0;

         // Remap the path from remote to local
         string path=Server.remapPath(fileName, fileLen);

         // Set the file mode
         System.IO.FileMode mode=System.IO.FileMode.Open;
         switch(creationDisposition)
         {
            case cDispositionCreateNew        : mode=System.IO.FileMode.CreateNew; break;
            case cDispositionCreateAlways     : mode=System.IO.FileMode.Create; break;
            case cDispositionOpenExisting     : mode=System.IO.FileMode.Open; break;
            case cDispositionOpenAlways       : mode=System.IO.FileMode.OpenOrCreate; break;
            case cDispositionTruncateExisting : mode=System.IO.FileMode.Truncate; break;
         }

         // Set the file access
         System.IO.FileAccess access=System.IO.FileAccess.Read;
         if((desiredAccess&cAccessGenericRead)!=0 && (desiredAccess&cAccessGenericWrite)!=0)
            access=System.IO.FileAccess.ReadWrite;
         else if((desiredAccess&cAccessGenericRead)!=0)
            access=System.IO.FileAccess.Read;
         else if((desiredAccess&cAccessGenericWrite)!=0)
            access=System.IO.FileAccess.Write;

         // Set the file share
         System.IO.FileShare share=System.IO.FileShare.None;
         if((shareMode&cFileShareRead)!=0 && (shareMode&cFileShareWrite)!=0)
            share=System.IO.FileShare.ReadWrite;
         else if((shareMode&cFileShareRead)!=0)
            share=System.IO.FileShare.Read;
         else if((shareMode&cFileShareWrite)!=0)
            share=System.IO.FileShare.Write;

         // Open the file
         fileIndex = Server.openFile(path, mode, access, share, mClient.getID());

         Server.output("  Handle "+fileIndex.ToString());

         if(fileIndex != -1)
         {
            mFileIndexList.Add(fileIndex);

            // Get the file stream
            fs = Server.getFileStream(fileIndex);

            try
            {
               // Get the file size
               if(fs!=null)
                  fileSize=fs.Length;

               // Get the file time
               fileTime=File.GetLastWriteTime(path).ToFileTime();
            }
            catch (Exception ex)
            {
               Server.logException(ex);
            }
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.CreateFile);
         mWriter.Write(fileIndex);
         mWriter.Write(fileSize);
         mWriter.Write(fileTime);

         byte data=0;
         if(creationDisposition==cDispositionOpenExisting && fs!=null && fileSize>=cOpenInitialRead)
            data=1;
         mWriter.Write(data);

         if(data==1)
         {
            fs.Read(mSendBuffer, 0, cOpenInitialRead);
            mWriter.Write(mSendBuffer, 0, cOpenInitialRead);
         }

         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleCloseHandle
      //============================================================================
      bool handleCloseHandle()
      {
         // Read in parameters
         int fileIndex=mReader.ReadInt32();
         byte suffix = mReader.ReadByte();

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         Server.closeFile(fileIndex);

         mFileIndexList.Remove(fileIndex);

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.CloseHandle);
         mWriter.Write((byte)1);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleReadFile
      //============================================================================
      bool handleReadFile()
      {
         // Read in parameters
         int fileIndex=mReader.ReadInt32();
         long fileOffset=mReader.ReadUInt32();
         int bytesToRead=mReader.ReadInt32();

         // Verify the command suffix
         byte suffix=mReader.ReadByte();
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Make sure the passed in file index points to an actual open file
         FileStream fs=Server.getFileStream(fileIndex);
         if(fs==null)
         {
            Server.output("ERROR - invalid file handle");
            return false;
         }

         // Check the file position
         if(fileOffset!=fs.Position)
            fs.Position=fileOffset;

         // Send the data
         int bytesLeftToRead=bytesToRead;

         for(;;)
         {
            mWriter.Write((byte)cCommandPrefix);
            mWriter.Write((byte)CommandType.ReadFile);

            int packetBytesRequest;
            if(bytesLeftToRead<=cSendBufferSize)
               packetBytesRequest=bytesLeftToRead;
            else
               packetBytesRequest=cSendBufferSize;

            int packetBytesActual=fs.Read(mSendBuffer, 0, packetBytesRequest);

            bytesLeftToRead-=packetBytesActual;

            bool done=(packetBytesActual<packetBytesRequest || bytesLeftToRead==0);
            if(done)
               mWriter.Write((byte)0);
            else
               mWriter.Write((byte)1);

            //mWriter.Write((byte)0);
            mWriter.Write(packetBytesActual);
            mWriter.Write(mSendBuffer, 0, packetBytesActual);
            mWriter.Write((byte)cCommandSuffix);
            mWriter.Flush();

            if(done)
               break;
         }

         return true;
      }

      //============================================================================
      // Connection.handleWriteFile
      //============================================================================
      bool handleWriteFile()
      {
         // Read in parameters
         int fileIndex=mReader.ReadInt32();
         long fileOffset=mReader.ReadUInt32();
         int bytesToWrite=mReader.ReadInt32();

         // Make sure the passed in file index points to an actual open file
         FileStream fs=Server.getFileStream(fileIndex);
         if(fs==null)
         {
            Server.output("ERROR - invalid file handle");
            return false;
         }

         // Check the file position
         if(fileOffset!=fs.Position)
            fs.Position=fileOffset;

         // Write data
         int bytesLeft=bytesToWrite;
         int bytesWritten=0;
         for(;;)
         {
            int packetBytes=bytesLeft;
            if(packetBytes>cRecvBufferSize)
               packetBytes=cRecvBufferSize;
            int bytes = mReader.BaseStream.Read(mRecvBuffer, 0, packetBytes);
            if(bytes==0)
               return false;
            fs.Write(mRecvBuffer, 0, bytes);
            bytesLeft-=bytes;
            bytesWritten+=bytes;
            if(bytesLeft==0)
               break;
         }

         // Verify the command suffix
         byte suffix=mReader.ReadByte();
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.WriteFile);
         mWriter.Write(bytesWritten);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleSetFilePointer
      //============================================================================
      bool handleSetFilePointer()
      {
         // Read in parameters
         int fileIndex=mReader.ReadInt32();
         long fileOffset=mReader.ReadUInt32();
         int distanceToMove=mReader.ReadInt32();
         uint moveMethod=mReader.ReadUInt32();

         Server.output("SetFilePointer "+fileIndex.ToString());

         // Verify the command suffix
         byte suffix=mReader.ReadByte();
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Make sure the passed in file index points to an actual open file
         FileStream fs=Server.getFileStream(fileIndex);
         if(fs==null)
         {
            Server.output("ERROR - invalid file handle");
            return false;
         }

         // Check the file position if setting the file pointer based on the current position
         if(moveMethod==1)
         {
            if(fileOffset!=fs.Position)
               fs.Position=fileOffset;
         }

         // Set the file position
         System.IO.SeekOrigin origin;
         switch(moveMethod)
         {
            case 0: origin=System.IO.SeekOrigin.Begin; break;
            case 1: origin=System.IO.SeekOrigin.Current; break;
            case 2: origin=System.IO.SeekOrigin.End; break;
            default:
            {
               Server.output("ERROR - invalid move method");
               return false;
            }
         }
         Int64 filePos=fs.Seek(distanceToMove, origin);

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.SetFilePointer);
         mWriter.Write(filePos);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleCreateDirectory
      //============================================================================
      bool handleCreateDirectory()
      {
         // Read in parameters
         short dirLen=mReader.ReadInt16();
         char[] dirName=mReader.ReadChars(dirLen);
         byte suffix=mReader.ReadByte();

         Server.output("CreateDirectory "+new string(dirName));

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Remap the path from remote to local
         string path=Server.remapPath(dirName, dirLen);

         byte retval=1;
         try
         {
            if (!Directory.Exists(path)) 
            {
               DirectoryInfo di = Directory.CreateDirectory(path);
            }
         }
         catch (Exception ex)
         {
            Server.logException(ex);
            retval=0;
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.CreateDirectory);
         mWriter.Write((byte)retval);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleGetFileAttributes
      //============================================================================
      bool handleGetFileAttributes()
      {
         // Read in parameters
         short fileLen=mReader.ReadInt16();
         char[] fileName=mReader.ReadChars(fileLen);
         byte suffix=mReader.ReadByte();

         Server.output("GetFileAttributes "+new string(fileName));

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Remap the path from remote to local
         string path=Server.remapPath(fileName, fileLen);

         FileAttributes attr=0;
         try
         {
            attr=File.GetAttributes(path);
         }
         catch (Exception ex)
         {
            Server.logException(ex);
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.GetFileAttributes);
         mWriter.Write((int)attr);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      [StructLayout(LayoutKind.Explicit, Size = 36)]
      private struct WIN32_FILE_ATTRIBUTE_DATA
      {
         [FieldOffset(0)]public uint dwFileAttributes;
         [FieldOffset(4)]public UInt64 ftCreationTime;
         [FieldOffset(12)]public UInt64 ftLastAccessTime;
         [FieldOffset(20)]public UInt64 ftLastWriteTime;
         [FieldOffset(28)]public uint nFileSizeHigh;
         [FieldOffset(32)]public uint nFileSizeLow;
      }

      private enum GET_FILEEX_INFO_LEVELS
      {
         GetFileExInfoStandard,
         GetFileExMaxInfoLevel
      }

      #region DLLImports
      [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
      [return: MarshalAs(UnmanagedType.Bool)]
      static extern bool GetFileAttributesEx(string lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId, out WIN32_FILE_ATTRIBUTE_DATA fileData);
      #endregion
      
      //============================================================================
      // Connection.handleGetFileAttributesEx
      //============================================================================
      bool handleGetFileAttributesEx()
      {
         // Read in parameters
         short fileLen = mReader.ReadInt16();
         char[] fileName = mReader.ReadChars(fileLen);
         byte suffix = mReader.ReadByte();

         Server.output("GetFileAttributesEx " + new string(fileName));

         // Verify the command suffix
         if (suffix != cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Remap the path from remote to local
         string path = Server.remapPath(fileName, fileLen);

         WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
         fileAttributeData.dwFileAttributes = 0;
         fileAttributeData.ftCreationTime = 0;
         fileAttributeData.ftLastAccessTime = 0;
         fileAttributeData.ftLastWriteTime = 0;
         fileAttributeData.nFileSizeHigh = 0;
         fileAttributeData.nFileSizeLow = 0;
         
         bool success = false;
         
         try
         {
            success = GetFileAttributesEx(path, GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard, out fileAttributeData);
         }
         catch (Exception ex)
         {
            Server.logException(ex);
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.GetFileAttributesEx);
         mWriter.Write((byte)(success ? 1 : 0));
         mWriter.Write((int)fileAttributeData.dwFileAttributes);
         mWriter.Write((UInt64)fileAttributeData.ftCreationTime);
         mWriter.Write((UInt64)fileAttributeData.ftLastAccessTime);
         mWriter.Write((UInt64)fileAttributeData.ftLastWriteTime);
         mWriter.Write((uint)fileAttributeData.nFileSizeHigh);
         mWriter.Write((uint)fileAttributeData.nFileSizeLow);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleFindFiles
      //============================================================================
      bool handleFindFiles()
      {
         // Read in parameters
         short fileLen=mReader.ReadInt16();
         char[] fileName=mReader.ReadChars(fileLen);
         byte suffix=mReader.ReadByte();

         Server.output("FindFiles "+new string(fileName));

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Remap the path from remote to local
         string path=Server.remapPath(fileName, fileLen);

         int loc=path.LastIndexOf('\\');
         string dirPath;
         string searchPattern;
         if(loc==-1)
         {
            dirPath=@".";
            searchPattern=path;
         }
         else
         {
            dirPath=path.Substring(0, loc+1);
            searchPattern=path.Substring(loc+1, path.Length-loc-1);
         }

         DirectoryInfo[] dirInfoList = null;
         FileInfo[] fileInfoList = null;
         try
         {
            DirectoryInfo dirInfo = new DirectoryInfo(dirPath);
            dirInfoList = dirInfo.GetDirectories();
            fileInfoList = dirInfo.GetFiles(searchPattern);
         }
         catch (Exception ex)
         {
            Server.logException(ex);
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.FindFiles);
         int dirCount = 0;
         if (dirInfoList != null)
            dirCount = dirInfoList.Length;
         int fileCount = 0;
         if (fileInfoList != null)
            fileCount = fileInfoList.Length;
         int totalCount = dirCount + fileCount;
         if(totalCount>0)
         {
            mWriter.Write(totalCount);
            
            if (fileInfoList != null)
            {
               foreach (FileInfo fileInfo in fileInfoList)
               {
                  byte[] bytes = Encoding.UTF8.GetBytes(fileInfo.Name);
                  mWriter.Write((int)bytes.Length);
                  mWriter.Write(bytes, 0, bytes.Length);
                  mWriter.Write((int)fileInfo.Attributes);
                  mWriter.Write(fileInfo.LastWriteTime.ToFileTime());
                  mWriter.Write(fileInfo.Length);
               }
            }
            
            if (dirInfoList != null)
            {
               foreach (DirectoryInfo dirInfo in dirInfoList)
               {
                  byte[] bytes = Encoding.UTF8.GetBytes(dirInfo.Name);
                  mWriter.Write((int)bytes.Length);
                  mWriter.Write(bytes, 0, bytes.Length);
                  mWriter.Write((int)dirInfo.Attributes);
                  mWriter.Write(dirInfo.LastWriteTime.ToFileTime());
                  mWriter.Write((long)0);
               }
            }
         }
         else
            mWriter.Write((int)0);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleFlushFileBuffers
      //============================================================================
      bool handleFlushFileBuffers()
      {
         // Read in parameters
         int fileIndex=mReader.ReadInt32();

         Server.output("FlushFileBuffers "+fileIndex.ToString());

         // Verify the command suffix
         byte suffix=mReader.ReadByte();
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Make sure the passed in file index points to an actual open file
         FileStream fs=Server.getFileStream(fileIndex);
         if(fs==null)
         {
            Server.output("ERROR - invalid file handle");
            return false;
         }

         int retval=1;
         try
         {
            fs.Flush();
         }
         catch (Exception ex)
         {
            Server.logException(ex);
            retval=0;
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.FlushFileBuffers);
         mWriter.Write(retval);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleTest
      //============================================================================
      bool handleTest()
      {
         // Read in parameters
         byte suffix=mReader.ReadByte();

         Server.output("Test");

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.Test);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }

      //============================================================================
      // Connection.handleReset
      //============================================================================
      bool handleReset()
      {
         // Read in parameters
         byte suffix=mReader.ReadByte();

         Server.output("Reset");

         // Verify the command suffix
         if(suffix!=cCommandSuffix)
         {
            Server.output("ERROR - missing command suffix");
            return false;
         }

         Server.resetClient(this);
         DebugConnectionManager.resetClient(mIP);

         // Send the response
         mWriter.Write((byte)cCommandPrefix);
         mWriter.Write((byte)CommandType.Reset);
         mWriter.Write((byte)1);
         mWriter.Write((byte)cCommandSuffix);
         mWriter.Flush();

         return true;
      }
     
                    
   }
}
