//#define DEBUG_DATA_DUMP

using System;
using System.Net.Sockets;
using System.IO;
using System.Timers;
using System.Threading;
using System.Collections;
using System.Net;


namespace RemoteTools
{
	/// <summary>
	/// This object contains a connection to a client
	/// </summary>
	public class NetServiceConnection : IDisposable
	{
	   
      private Socket mSocket = null;
      private NetworkStream mStream = null;
      private Thread ReadThread = null;
      private Thread DispatchThread = null;

      private System.Timers.Timer mDispatchTimer = null;
      private bool mLostConnection = false;
		public string mClientName = "";
      
      public bool mbUseDispatchTimer = false; //This allows switching between a dispatch timer or thread for io

      const int cMaxPacketSize = 100000;
      static int mHackyTempFileCounter = 0;
      const int cBatchSize = 25;  // some day this could be handled smartly

      string mServiceName = "";
      public string ServiceName
      {
         get
         {
            return mServiceName;
         }
         set
         {
            mServiceName = value;
         }

      }
      int mServiceID;
      public int ServiceID
      {
         get
         {
            return mServiceID;
         }
         set
         {
            mServiceID = value;
         }
      }
      
      // Packet queue for sending.
      //public ArrayList mPacketQueue = new ArrayList();
      //private class BPacketEntry
      //{
      //   //public BPacketType mType;
      //   public int mType;
      //   public byte[] mPayload;
      //   public int mPayloadSize;
      //}
      
      // Status
      //public enum BStatus
      //{
      //   cStatusIdle,
      //   cStatusWorking,
      //   cStatusWorkingOwner,
      //}
      //private int mNumProcs = -1;
      //private string mFilename = "";
      //private FileStream mFile;
      //private BStatus mStatus = BStatus.cStatusIdle;
      //private SClientConnection mWorkingForClient;
      //private int mTotalTriCount = 0;
      //private int mNextWorkTriIndex = 0;
      //private bool mWantToWork = true;
      //private ArrayList mClientsWorking = new ArrayList();
      
      //private ArrayList mResultFiles = new ArrayList();
      

      //ugh, keep this static to avoid alot of signature changes until we need more than one client..
      //      This is the packet parsing code is given a stream instead of the binary writer
      //          GetBinaryWriter gets called inside of each packet's writing code.
      static public bool mbIsXbox = false;
      static public bool mbIsAnsiBStringClient = true;
      static public BinaryWriter GetBinaryWriter(Stream s)
      {
         if(mbIsXbox)
         {
            return new XBoxBinaryWriter(s);
         }
         else if(mbIsAnsiBStringClient)
         {
            return new BinaryWriter(s, System.Text.Encoding.ASCII);
         }
         else 
         {
            return new BinaryWriter(s, System.Text.Encoding.Unicode);
         }  
      }
      static public BinaryReader GetBinaryReader(Stream s)
      {

         if(mbIsXbox)
         {
            return new XBoxBinaryReader(s);
         }
         else if(mbIsAnsiBStringClient)
         {
            return new BinaryReader(s, System.Text.Encoding.ASCII);
         }
         else 
         {
            return new BinaryReader(s, System.Text.Encoding.Unicode);
         }  

      }

      //From msdn
      //private static Socket ConnectSocket(string server, int port)
      //{
      //   Socket s = null;
      //   IPHostEntry hostEntry = null;

      //   // Get host related information.
      //   hostEntry = Dns.GetHostEntry(server);

      //   // Loop through the AddressList to obtain the supported AddressFamily. This is to avoid
      //   // an exception that occurs when the host IP Address is not compatible with the address family
      //   // (typical in the IPv6 case).
      //   foreach (IPAddress address in hostEntry.AddressList)
      //   {
      //      IPEndPoint ipe = new IPEndPoint(address, port);
      //      Socket tempSocket =
      //          new Socket(ipe.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

      //      tempSocket.Connect(ipe);

      //      if (tempSocket.Connected)
      //      {
      //         s = tempSocket;
      //         break;
      //      }
      //      else
      //      {
      //         continue;
      //      }
      //   }
      //   return s;
      //}

      public NetServiceConnection(string serverIP, int serverPort)
      {
         IPAddress addr;
         if (IPAddress.TryParse(serverIP, out  addr) == false)
            return;
         IPEndPoint endpoint = new IPEndPoint(new IPAddress(addr.GetAddressBytes()), serverPort);

         Socket s = new Socket(endpoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

         s.Connect(endpoint);

         if(s.Connected)
         {
            InitFromSocket(s);            
         }
      }

		public NetServiceConnection(Socket s)
		{
         InitFromSocket(s);         
		}   
   
      private void InitFromSocket(Socket s)
      {
         mSocket = s;
         mStream = new NetworkStream(s);
         //mSocket.ReceiveTimeout = 3000;

         // Look up the host name.
         try
         {
            IPHostEntry hostEntry = System.Net.Dns.GetHostEntry(((IPEndPoint)s.RemoteEndPoint).Address.ToString());
            mClientName = hostEntry.HostName;
         }
         catch (System.Exception e)
         {
            mClientName = "DNS:Unknown Host Name";
         }

#if DEBUG_DATA_DUMP
            fileID++;
            mStream = File.Create(fileID.ToString() + "blah.stream");//,1000);
            mWriter = new BinaryWriter(mStream);
#endif

      }


#if DEBUG_DATA_DUMP
      static int fileID = 0;
      long dataWritten = 0;
      FileStream mStream;
      BinaryWriter mWriter;
#endif

      public void DispatchThreadFunction()
      {
         bool active = true;
         while(active)
         {
            lock (this)
            {         
               if (mLostConnection)
               { 
                  if (mDisconnectedDelegates != null)
                     mDisconnectedDelegates(this);

                  active = false;
               }

               object [] packets = null;

               lock (mIncomingPackets)
               {
                  packets = mIncomingPackets.ToArray();
                  mIncomingPackets.Clear();
               }
               
               if (packets != null && packets.Length !=0)
               {           
                  foreach (SIncomingPacket packet in packets)
                  {
                     //Console.WriteLine("processing packet type " + packet.header.mType.ToString() + ", size "+packet.header.mSize.ToString());
                     
                     // set up a memory stream to dispatch the bytes
                     MemoryStream memStream = null;
                     BinaryReader reader = null;
                     if(packet.header.mSize > 0)
                     {
                        memStream = new MemoryStream(packet.buffer);
                        //reader = new BinaryReader(memStream, System.Text.Encoding.Unicode);
                        reader = GetBinaryReader(memStream);
                     }

                     mReadDelegates(this, packet.header.mType, reader);


#if DEBUG_DATA_DUMP
                     mWriter.Write(packet.header.mSize);
                     mWriter.Write(packet.header.mType);
                     mWriter.Write(packet.buffer,0,packet.buffer.Length);
                     dataWritten = dataWritten + 5 + packet.buffer.Length;
#endif

                  }
               }
               
               // Send any pending packets.
               //lock(mPacketQueue)
               //{
               //   if(!mLostConnection && mSocket.Connected)
               //   {
               //      foreach(BPacketEntry entry in mPacketQueue)
               //         sendPacketInternal(entry.mType, entry.mPayload, entry.mPayloadSize);
               //   }
               //   mPacketQueue.Clear();
               //}
            }
            System.Threading.Thread.Sleep(100);
         }
      }

      public void startRead()
      {
         ReadThread = new Thread(new ThreadStart(ReadThreadStart));
         ReadThread.Name = "Network Read Thread";
			ReadThread.Start();

         //if(mbUseDispatchTimer == true)
         //{
         //   mDispatchTimer = new System.Timers.Timer(100);
         //   mDispatchTimer.Elapsed += new System.Timers.ElapsedEventHandler(dispatchTimerElapsed);
         //   mDispatchTimer.Start();
         //}
         //else
         {
            DispatchThread = new Thread(new ThreadStart(DispatchThreadFunction));
            DispatchThread.Name = "DispatchThread";
            DispatchThread.Start();
         }
      }

		public void stopRead()
		{
		   mSocket.Close();
			ReadThread.Abort();
			mLostConnection = true;
         if (mDispatchTimer != null)
			   mDispatchTimer.Enabled = false;
		}

      public Socket getSocket() { return mSocket; }

      public void write(Stream stream)
      {
         try
         {
            //lock(this)
            {
               const int bufferSize = 1024;
               byte [] buffer = new byte[bufferSize];
               stream.Seek(0, SeekOrigin.Begin);

               int bytesRead = stream.Read(buffer, 0, bufferSize);
               while (bytesRead > 0)
               {
                  mStream.Write(buffer, 0, bytesRead);
                  bytesRead = stream.Read(buffer, 0, bufferSize); // <-- shouldn't we read from a bytesRead offset?  Test!
               }
            }
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
         }
      }

      public delegate void ReadDelegate(NetServiceConnection clientConnection, byte type, BinaryReader reader);
      public ReadDelegate mReadDelegates;

      public delegate void DisconnectedDelegate(NetServiceConnection cc);
      public DisconnectedDelegate mDisconnectedDelegates;

      public class SIncomingPacket
      {
         // note this ctor blocks until the packet is fully read
         public SIncomingPacket(NetworkStream stream)
         {
            //BinaryReader headerReader = new BinaryReader(stream, System.Text.Encoding.Unicode);            
            BinaryReader reader = new BinaryReader(stream);
            header = new CTSPacketHeader(reader);

            //check that we have valid information...  It would be better to change the protocol to have some leading dummy bytes that we could check for 
            if(header.mSize < 0 || header.mSize > cMaxPacketSize)
            {
               throw new System.Exception(String.Format("Invalid Packet Size:{0} Type:{1}",header.mSize,header.mType)); 
            }

            //buffer = new byte[header.mSize];
            //stream.Read(buffer, 0, header.mSize);
            if(header.mSize > 0)
               buffer = reader.ReadBytes(header.mSize);
         }
         public CTSPacketHeader header = null;
         public byte [] buffer = null;
      }

      public ArrayList mIncomingPackets = new ArrayList();
  
      public void ReadThreadStart()
      {
         bool busted = false;
         while (true)
         {
            // block reading a packet off the stream
            SIncomingPacket packet = null;
            try
            {   
               if(busted == false)
               {
                  if(mStream.CanRead == true)
                  {
                     packet = new SIncomingPacket(mStream);
                     if(packet.header.mbXboxDetected == true)
                     {
                        mbIsXbox = true;
                     }
                  }
                  else
                  {
                     continue;
                  }
               }
            }
            catch (System.IO.EndOfStreamException ex)
            {
               mLostConnection = true;
               ex.ToString();
               break;
            }
            catch (System.IO.IOException ex)
            {
               ErrorHandler.Error("IO Network Error");
               ErrorHandler.Error(ex.ToString());
               // FIXME: Better filter for disconnect exception vs. other exceptions
               mLostConnection = true;
               return; // was break
            }
            catch (System.Exception exAny)
            {
               ErrorHandler.Error("Network Error");
               ErrorHandler.Error(exAny.ToString());
               busted = true;
            }

            if (packet != null)
            {        
               lock (mIncomingPackets)
               {
                  mIncomingPackets.Add(packet);
               }
            }
         }
      }

      //public void sendPacket(SClientConnection destClient, BPacketType type, byte []payload, int payloadSize)
      //{
      //   // Queue it.
      //   BPacketEntry entry = new BPacketEntry();
      //   entry.mType = type;
      //   entry.mPayloadSize = payloadSize;
      //   entry.mPayload = null;
      //   if(payloadSize > 0)
      //   {
      //      entry.mPayload = new byte[payloadSize];
      //      Array.Copy(payload, entry.mPayload, payloadSize);
      //   }
      //   lock(destClient.mPacketQueue)
      //   {
      //      destClient.mPacketQueue.Add(entry);
      //   }
      //}
      
      
      //private void sendPacketInternal(BPacketType type, byte []payload, int payloadSize)
      //{
      //   try
      //   {
      //      // Create a writer 
      //      BinaryWriter writer = new BinaryWriter(mStream);

      //      // First the size of the packet (adding one for the type)
      //      writer.Write(payloadSize+1);
            
      //      // Now the type byte
      //      writer.Write((byte)type);
            
      //      // Now the payload.
      //      if(payload != null)
      //         writer.Write(payload, 0, payloadSize);
      //   }
      //   catch (System.IO.IOException ex)
      //   {
      //      ErrorHandler.Error("IO Network Error");
      //      ErrorHandler.Error(ex.ToString());
      //      // FIXME: Better filter for disconnect exception vs. other exceptions
      //      mLostConnection = true;
      //      return; // was break
      //   }
      //}
     
      //public void workFor(SClientConnection owningClient)
      //{
      //   // Tell owner of job to send us the file to work on.
      //   sendFile(owningClient.mFilename);
      //   lock(owningClient.mClientsWorking)
      //   {
      //      owningClient.mClientsWorking.Add(this);
      //   }
         
      //   // Update our status.
      //   mStatus = BStatus.cStatusWorking;
      //   mWorkingForClient = owningClient;
         
      //   // Notification.
      //   if(mUpdateDelegates != null)
      //      mUpdateDelegates(this, BUpdateMessage.cStatusUpdate);
            
      //   // Get our first blob of work.
      //   owningClient.readyForWork(this);
      //}
      
      //public bool readyForWork(SClientConnection worker)
      //{
      //   // If there's nothing else to do, tell the client to send results.
      //   if(mNextWorkTriIndex >= mTotalTriCount)
      //   {
      //      sendPacket(worker, BPacketType.cPacketRequestResults, null, 0);
      //      return(false);
      //   }
         
      //   // Give out the next batch of triangles.
      //   int numTris = Math.Min(cBatchSize, mTotalTriCount-mNextWorkTriIndex);
      //   worker.sendWorkRequest(mNextWorkTriIndex, numTris);
      //   mNextWorkTriIndex += numTris;
         
      //   // Successfully handed out some work.
      //   return(true);
      //}

      //public void sendWorkRequest(int startIndex, int triCount)
      //{
      //   MemoryStream payloadStream = new MemoryStream();
      //   BinaryWriter writer = new BinaryWriter(payloadStream);
      //   writer.Write(startIndex);
      //   writer.Write(triCount);
      //   sendPacket(this, BPacketType.cPacketWorkRequest, payloadStream.GetBuffer(), payloadStream.GetBuffer().Length);
      //}
      
      //public void sendFile(string filename)
      //{
      //   FileStream file;
      //   file = File.Open(filename, FileMode.Open, FileAccess.Read);
         
      //   Console.WriteLine("sendFile " + filename + " to " + mClientName);
      //   // File start packet.
      //   sendPacket(this, BPacketType.cPacketFileStart, null, 0);
         
      //   // Make sure we're at the beginning of the file.
      //   file.Seek(0, SeekOrigin.Begin);
         
      //   // Send the file contents.
      //   const int cBuffSize = 16384;
      //   byte[] buff = new byte[cBuffSize];
      //   int bytesRead = file.Read(buff, 0, cBuffSize);
      //   while(bytesRead>0)
      //   {
      //      sendPacket(this, BPacketType.cPacketFileData, buff, bytesRead);
      //      bytesRead = file.Read(buff, 0, cBuffSize);
      //   }
         
      //   // File end packet.
      //   sendPacket(this, BPacketType.cPacketFileEnd, null, 0);
         
      //   // Clean up.
      //   file.Close();
      //}
      
      //public void addResultFile(string filename)
      //{
      //   mResultFiles.Add(filename);
      //}


      #region IDisposable Members

      public void Dispose()
      {
         stopRead();
      
      }

      #endregion
   }

   //The Error Handler should be in a separate library.  Perhaps this can be done when the 
   //dxmanagedcontrol is added to a separate library
   #region Error Handler
   public delegate void ErrorEvent(string error);
   public class ErrorHandler
   {
      static public void Error(string error)
      {
         try
         {
            if(mStrategy != null)
            {
               mStrategy.Error(error);
            }
            if(OnError != null)
            {
               OnError(error);
            }
         }
         catch(System.Exception ex)
         {
            ex.ToString();
         }
      }
      static ErrorHandlerStrategy mStrategy = new ErrorHandlerLogStrategy();
      public static event ErrorEvent OnError = null;
   }   
   interface ErrorHandlerStrategy
   {
      void Error(string error);      
   }
   class ErrorHandlerLogStrategy : ErrorHandlerStrategy
   {
      string mFilename = "ErrorLog.txt";
      public ErrorHandlerLogStrategy()
      {
         if (!Directory.Exists("Logs"))
            Directory.CreateDirectory("Logs");
            mFilename = "Logs\\Log " + System.DateTime.Now.ToString().Replace(":","_").Replace("/","_") +".txt";
      }
      public void Error(string error)
      {
         lock(this)
         {
            StreamWriter writer = new StreamWriter(mFilename,true);
            writer.WriteLine(System.DateTime.Now.ToString() + " " + error);
            writer.Close();
         }
      }
   } 
   #endregion



   public class XBoxBinaryReader : BinaryReader
   {
      public XBoxBinaryReader(Stream s) : base (s, System.Text.Encoding.ASCII)
      {

      }
      public override short ReadInt16()
      {
         return System.BitConverter.ToInt16(ReverseReadNBytes(2),0);
      }
      public override int ReadInt32()
      {
         return System.BitConverter.ToInt32(ReverseReadNBytes(4),0);
      }       
      public override long ReadInt64()
      {
         return System.BitConverter.ToInt64(ReverseReadNBytes(8),0);
      }
      public override ushort ReadUInt16()
      {
         return System.BitConverter.ToUInt16(ReverseReadNBytes(2),0);
      }
      public override uint ReadUInt32()
      {
         return System.BitConverter.ToUInt32(ReverseReadNBytes(4),0);
      }       
      public override ulong ReadUInt64()
      {
         return System.BitConverter.ToUInt64(ReverseReadNBytes(8),0);
      }
      public override float ReadSingle()
      {
         return System.BitConverter.ToSingle(ReverseReadNBytes(4),0);
      }
      public override double ReadDouble()
      {
         return System.BitConverter.ToDouble(ReverseReadNBytes(8),0);
      }

      //We could use "System.BitConverter.IsLittleEndian = false;", but being global and static scares me..
      public byte[] ReverseReadNBytes(int count)
      {
         byte[] data = new byte[count];
         data = base.ReadBytes(count);
         Array.Reverse(data,0,count);
         return data;
      }

      //use this if the encoding type is not ASCII
      public string ReadAsciString(BinaryReader reader)
      {
         byte length = reader.ReadByte();
         byte[] bytes = reader.ReadBytes(length);
         return System.Text.Encoding.ASCII.GetString(bytes,0,length);
      }
   }
   public class XBoxBinaryWriter : BinaryWriter
   {
      public XBoxBinaryWriter(Stream s) : base (s,System.Text.Encoding.ASCII)
      {
         
      }
      public XBoxBinaryWriter(Stream s, System.Text.Encoding e) : base (s,e)
      {
         
      }
      public override void Write(int value)
      {
         base.Write (swap(value));
      }
      public override void Write(ulong value)
      {
         //hmm, I wonder if this is slower or faster than swap()?

         //Should I try some unsafe code?
         MemoryStream m = new MemoryStream();
         BinaryWriter b = new BinaryWriter(m);
         b.Write(value);
         b.Close();
         byte[] bytes = m.ToArray();
         Array.Reverse(bytes);
         base.Write(bytes,0,bytes.Length);
      }

      static public int swap(int val)
      {
         long data = 0;
         data += (val & 0xff000000) >> 24;
         data += (val & 0x00ff0000) >> 8;
         data += (val & 0x0000ff00) << 8;
         data += (val & 0x000000ff) << 24;        
         return (int)data;
      }
   }


   public class CTSPacketHeader
   {

      static public int swap(int val)
      {
         long data = 0;
         data += (val & 0xff000000) >> 24;
         data += (val & 0x00ff0000) >> 8;
         data += (val & 0x0000ff00) << 8;
         data += (val & 0x000000ff) << 24;
         return (int)data;
      }

      public bool mbXboxDetected = false;
      public CTSPacketHeader(BinaryReader reader)
      {

         //mSize = reader.ReadInt32()-1; // minus one for the type byte below
         //mSize = ReadInt32(reader)-1;
         
         
         int size = reader.ReadInt32();
         if ((uint)size > (uint)(0xffff))
         {
            mbXboxDetected = true;
            mSize = swap(size) - 1;
         }
         else
         {
            mSize = size - 1;
         }
         mType = reader.ReadByte();
      }

      public CTSPacketHeader(BinaryWriter writer, byte type)
      {
         mType = type;
         mSize = 0;
         writer.Write(mSize);
         writer.Write(type);
      }

      public void writeSize(BinaryWriter writer)
      {
         mSize = (int)writer.Seek(0, SeekOrigin.End);
         writer.Seek(0, SeekOrigin.Begin);
         //int size = swap(mSize);
         writer.Write(mSize);
      }

      public int mSize = 0;
      public byte mType = 0;
   }

}
