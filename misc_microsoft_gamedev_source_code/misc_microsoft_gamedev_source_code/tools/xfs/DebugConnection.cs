using System;
using System.Collections;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;

namespace xfs
{
   public delegate void TextCallback(String message);

   public class DebugConnection
   {
      const byte cCommandPrefix = (byte)'@';
      const byte cCommandSuffix = (byte)'#';

      enum CommandType : byte
      {
         FuncHello,
         FuncOutput,
         FuncGoodbye,
         FuncCommand
      };
      
      private Socket mSocket = null;
      
      private const int cRecvBufferSize = 1024;
      private byte[] mRecvBuffer = new byte[cRecvBufferSize];
      private uint mRecvBufferLen = 0;
      private uint mRecvPacketOfs = 0;
      private uint mRecvPacketLeft = 0;
      
      const int cSendBufferSize = 1024;
      private byte[] mSendBuffer = new byte[cSendBufferSize];
      private uint mSendBufferLen = 0;
      
      private Thread mThread = null;
      private bool mActive = false;
      
      private String mXboxName = "";
      private AutoResetEvent mExitThread = new AutoResetEvent(false);
            
      private AutoResetEvent mCommandReady = new AutoResetEvent(false);
      private Queue mCommandQueue = new Queue();
      
      private TextCallback mTextCallback = null;
      
      private int mID = 0;
      private String mIP = "";

      private bool mIsSelectedForCommands = true;
     

      public DebugConnection(TextCallback textCallback, int id)
      {
         mTextCallback = textCallback;
         mID = id;
      }
      
      public int getID()
      {
         return mID;
      }
      
      public String getIP()
      {
         if (!mActive)
            return null;
            
         return mIP;
      }
      
      public bool connect(Socket socket)
      {
         mActive = true;
         mSocket = socket;
         
         mIP = ((IPEndPoint)mSocket.RemoteEndPoint).Address.ToString();
         
         mThread = new Thread(new ParameterizedThreadStart(helperThread));
         mThread.IsBackground = true;
         mThread.Start(this);
         
         return true;
      }
      
      public static void helperThread(object parameter)
      {
         DebugConnection connection = (DebugConnection)parameter;
         
         connection.mRecvBufferLen = 0;
                  
         connection.mSocket.BeginReceive( connection.mRecvBuffer, 0, connection.mRecvBuffer.Length, 0, new AsyncCallback(ReadCallBack), connection);
         
         for ( ; ; )
         {
            try
            {
               WaitHandle[] waitHandles = { connection.mExitThread, connection.mCommandReady };
               int waitHandleIndex = WaitHandle.WaitAny(waitHandles);
               
               if (waitHandleIndex == 0)
                  break;
               
               connection.sendQueuedCommands();
            }
            catch
            {
               break; 
            }
         }
         
         connection.mSocket.Close();

         if (connection.mTextCallback != null)
            connection.mTextCallback("<debug>Debug connection [" + connection.mID.ToString() + "] disconnected: " + connection.getXboxName() + " (" + connection.getIP() + ") </debug>");

         DebugConnectionManager.removeConnection(connection);            
      }         
      
      public void disconnect()
      {
         if (!mActive)
            return;
            
         mExitThread.Set();
         
         mActive = false;
      }
            
      private void setXboxName(String name)
      {
         lock (mXboxName)
         {
            mXboxName = name;
         }
      }

      public String getXboxName()
      {
         String ret;
         lock (mXboxName)
         {
            ret = mXboxName;
         }
         return ret;
      }
      
      private void handleSocketError()
      {
         mExitThread.Set();
      }
     
      private void sendBegin(CommandType func) 
      {
         mSendBuffer[0] = cCommandPrefix;
         mSendBuffer[1] = (byte)func;
         mSendBuffer[2] = 0;
         mSendBuffer[3] = 0;
         mSendBufferLen = 4;
      }
      
      private void sendBYTE(int c)
      {
         mSendBuffer[mSendBufferLen] = (byte)c;
         mSendBufferLen++;
      }
      
      private void sendWORD(int c)
      {
         sendBYTE(c & 0xFF);
         sendBYTE(c >> 8);
      }
      
      private void sendString(String s)
      {
         sendWORD(s.Length);
         
         for (int i = 0; i < s.Length; i++)
            sendBYTE(s[i]);
      }
      
      private bool sendEnd()
      {
         sendBYTE(cCommandSuffix);
         mSendBuffer[2] = (byte)(mSendBufferLen & 0xFF);
         mSendBuffer[3] = (byte)(mSendBufferLen >> 8);
         
         try
         {
            mSocket.Send(mSendBuffer, (int)mSendBufferLen, 0);  
         }
         catch (SocketException)
         {
            handleSocketError();
            return false;
         }
         
         mSendBufferLen = 0;
         
         return true;
      }
     
      private void sendQueuedCommands()
      {
         lock (mCommandQueue.SyncRoot)
         {
            while (mCommandQueue.Count > 0)
            {
               sendBegin(CommandType.FuncCommand);
               
               String s = (String)mCommandQueue.Dequeue();
               sendString(s);
               
               if (!sendEnd())
                  break;
            }
         }
      }
      
      public void sendCommand(String command)
      {
         lock (mCommandQueue.SyncRoot)
         {
            mCommandQueue.Enqueue(command);
         }
         
         mCommandReady.Set();
      }
      
      private int readBYTE()
      {
         if (mRecvPacketLeft < 1)
            return -1;
         
         int c = mRecvBuffer[mRecvPacketOfs];
         mRecvPacketOfs++;
         mRecvPacketLeft--;
         
         return c;
      }
      
      private int readWORD()
      {
         if (mRecvPacketLeft < 2)
            return -1;
         
         int c = mRecvBuffer[mRecvPacketOfs] + (mRecvBuffer[mRecvPacketOfs + 1] << 8);
         mRecvPacketOfs += 2;
         mRecvPacketLeft -= 2;
         
         return c;
      }
      
      private bool readString(out String str)
      {
         str = "";
         
         int l = readWORD();
         if (l < 0)
            return false;
         
         if (mRecvPacketLeft < l)
            return false;
         
         StringBuilder sb = new StringBuilder();
         
         for (int i = 0; i < l; i++)
            sb.Append((char)mRecvBuffer[mRecvPacketOfs + i]);
            
         mRecvPacketOfs += (uint)l;
         mRecvPacketLeft -= (uint)l;
            
         str = sb.ToString();
         
         return true;
      }
                  
      private bool processHelloPacket()
      {
         String name;
         if (!readString(out name))
            return false;
         setXboxName(name);
            
         if (mTextCallback != null)
            mTextCallback("<debug>Debug connection [" + mID.ToString() + "] connected: " + getXboxName() + " (" + mIP + ") </debug>");            
                  
         return true;
      }
      
      private bool processTextPacket()
      {
         String text;
         if (!readString(out text))
            return false;

         if (mTextCallback != null)
            mTextCallback(text);
         
         return true;
      }

      private bool processGoodbyePacket()
      {
         mExitThread.Set();
         
         return true;
      }
   
      private bool processPacket(CommandType func)
      {   
         bool status = false;
         
         switch (func)
         {
            case CommandType.FuncHello:   status = processHelloPacket(); break;
            case CommandType.FuncOutput:  status = processTextPacket(); break;
            case CommandType.FuncGoodbye: status = processGoodbyePacket(); break;
         }
         
         return status;
      }
                 
      private void processReadCallBack(IAsyncResult ar)
      {
         int bytesRead;
         try
         {
            bytesRead = mSocket.EndReceive(ar);
         }
         catch
         {
            handleSocketError();
            return;   
         }
         
         if (bytesRead == 0)
            return;
            
         mRecvBufferLen += (uint)bytesRead;
         
         uint curOfs = 0;
         
         while ((mRecvBufferLen - curOfs) > 4)
         {
            uint prefix = mRecvBuffer[curOfs + 0];
            CommandType func = (CommandType)mRecvBuffer[curOfs + 1];
            uint len = (uint)mRecvBuffer[curOfs + 2] + ((uint)mRecvBuffer[curOfs + 3] << 8);
            
            if (prefix != cCommandPrefix)
            {
               handleSocketError();
               return;   
            }
            
            if (len > mRecvBuffer.Length)
            {
               handleSocketError();
               return;
            }
            
            if ((mRecvBufferLen - curOfs) >= len)
            {
               mRecvPacketOfs = (uint)curOfs + 4;
               mRecvPacketLeft = (uint)len - 4;
               
               bool status = processPacket(func);
                              
               if (!status)
               {
                  handleSocketError();
                  return;
               }
               
               if (readBYTE() != cCommandSuffix)            
               {
                  handleSocketError();
                  return;
               }
               
               curOfs += len;
            }
            else
            {
               break;
            }
         }      
         
         if (curOfs > 0)
         {
            uint bytesLeft = mRecvBufferLen - curOfs;
            
            for (uint i = 0; i < bytesLeft; i++)
               mRecvBuffer[i] = mRecvBuffer[curOfs + i];
            
            mRecvBufferLen = bytesLeft;
         }      

         try
         {
            mSocket.BeginReceive(mRecvBuffer, (int)mRecvBufferLen, (int)(mRecvBuffer.Length - mRecvBufferLen), 0, new AsyncCallback(ReadCallBack), this);
         }
         catch
         {
            handleSocketError();
         }
      }
      
      private static void ReadCallBack(IAsyncResult ar)
      {
         DebugConnection connection = (DebugConnection) ar.AsyncState;
         
         connection.processReadCallBack(ar);
      }

      public bool getIsSelectedForCommands()
      {
         return mIsSelectedForCommands;
      }
      public bool setIsSelectedForCommands(bool truefalse)
      {
         return mIsSelectedForCommands = truefalse;
      }
   };

   static public class DebugConnectionManager
   {
      const int cSocketPort = 1001;
      const int cMaxConnections = 100;

      static bool mActive = false;
      
      static private ArrayList mConnectionList = new ArrayList();
      static private TcpListener mTcpListener = null;
            
      static private TextCallback mTextCallback;
                  
      public static bool setup()
      {
         try
         {
            reset();
         
            mTcpListener = new TcpListener(IPAddress.Any, cSocketPort);
            mTcpListener.Start();
         }
         catch
         {
            return false;
         }

         return true;
      }
      
      public static void setTextCallback(TextCallback textCallback)
      {
         mTextCallback = textCallback;
      }
      
      public static bool start()
      {
         try
         {
            mTcpListener.BeginAcceptSocket(acceptCallback, null);
         }
         catch
         {
            return false;
         }
         
         mActive = true;
         
         return true;
      }
           
      public static bool reset()
      {
         if (mTextCallback != null)
            mTextCallback("<debug>Disconnecting all debug connections</debug>");
            
         lock (mConnectionList.SyncRoot)
         {
            foreach(DebugConnection connection in mConnectionList)
            {
               connection.disconnect();
            }
            
            mConnectionList.Clear();
         }
         
         return true;
      }
      
      public static void removeConnection(DebugConnection c)
      {
         lock (mConnectionList.SyncRoot)
         {
            int i = mConnectionList.IndexOf(c);
            if (i > 0)
            {
               mTextCallback("<debug>Deleting debug connection [" + c.getID().ToString() + "]</debug>");
                  
               mConnectionList.RemoveAt(i);
            }
         }
      }
      
      public static void resetClient(String ip)
      {
         mTextCallback("<debug>Disconnecting all debug connections from IP " + ip + "</debug>");
            
         lock (mConnectionList.SyncRoot)
         {
            foreach(DebugConnection c in mConnectionList)
            {
               String connectionIP = c.getIP();
               if (connectionIP != null)
               {
                  if (connectionIP == ip)
                     c.disconnect();
               }
            }
         }        
      }
      
      public static bool shutdown()
      {
         if (!mActive)
            return false;
            
         reset();
         
         if (mTcpListener != null)
         {
            mTcpListener.Stop();
            mTcpListener = null;
         }
         
         mActive = false;
         
         return true;
      }
      
      public static bool hasActiveConnections()
      {
         // .Count is thread safe
         return (mConnectionList.Count > 0);
      }
      
      private static void acceptCallback(IAsyncResult ar)
      {
         try
         {
            Socket socket = mTcpListener.EndAcceptSocket(ar);
                        
            onConnect(socket);
         }
         catch
         {
         }

         try
         {
            if (mTcpListener != null)
               mTcpListener.BeginAcceptSocket(acceptCallback, null);           
         }
         catch
         {
         
         }               
      }
      
      public static bool onConnect(Socket socket)
      {
         if (!mActive)
            return false;
                        
         lock (mConnectionList.SyncRoot)
         {
            if (mConnectionList.Count >= cMaxConnections)
               return false;
               
            DebugConnection connection = new DebugConnection(mTextCallback, mConnectionList.Count);
            if (!connection.connect(socket))
               return false;
            
            if (!mActive)
               return false;
               
            mConnectionList.Add(connection);
         }

         return true;
      }
      
      public static void sendCommand(String command)
      {
         lock (mConnectionList.SyncRoot)
         {
            if (mConnectionList.Count == 0)
            {
               if (mTextCallback != null)
                  mTextCallback("<console>Error: No current debug connections!</debug>");
            }
            else
            {
               foreach(DebugConnection c in mConnectionList)
               {
                  if (c.getIsSelectedForCommands())
                  {
                     c.sendCommand(command);

                     if (mTextCallback != null)
                        mTextCallback("<debug>Command sent to [" + c.getID().ToString() + "] " + c.getXboxName() + " (" + c.getIP() + ") </debug>");
                  }
               }
            }               
         }         
      }

      public static void sendCommandToIP(string IP, String command)
      {
         lock (mConnectionList.SyncRoot)
         {
            if (mConnectionList.Count == 0)
            {
               if (mTextCallback != null)
                  mTextCallback("<console>Error: No current debug connections!</debug>");
            }
            else
            {
               foreach (DebugConnection c in mConnectionList)
               {
                  if (IP.Equals(c.getIP().ToString()) && c.getIsSelectedForCommands())
                  {
                     c.sendCommand(command);

                     if (mTextCallback != null)
                        mTextCallback("<debug>Command sent to [" + c.getID().ToString() + "] " + c.getXboxName() + " (" + c.getIP() + ") </debug>");
                     break;
                  }
               }
            }
         }
      }
   	
   };

}

