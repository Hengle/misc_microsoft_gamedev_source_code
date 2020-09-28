using System;
using System.Net.Sockets;
using System.IO;
using System.Net;

using System.Threading;
using System.Collections;
using System.Collections.Generic;
using System.Timers;

namespace RemoteTools
{
   public class NetServiceManager
   {
      private TcpListener TcpListener = null;
      private Thread ListenThread;

      private ArrayList mClientConnections = new ArrayList();

      public NetServiceManager(int listenPort)
      {
         TcpListener = new TcpListener(System.Net.IPAddress.Any, listenPort);
         TcpListener.Start();
         ListenThread = new Thread(new ThreadStart(listenThreadStart));
         ListenThread.Start();

         mListenDispatchTimer.Elapsed += new System.Timers.ElapsedEventHandler(mListenDispatchTimer_Elapsed);
         mListenDispatchTimer.Start();
      }

      ~NetServiceManager()
      {
         TcpListener.Stop();
         ListenThread.Abort();
      }

      public System.Timers.Timer mListenDispatchTimer = new System.Timers.Timer(100);

      private ArrayList mIncomingClients = new ArrayList();
      public void mListenDispatchTimer_Elapsed(object sender, ElapsedEventArgs e)
      {
         object[] incomingClients = null;
         if (mIncomingClients.Count > 0)
         {
            lock (mIncomingClients)
            {
               incomingClients = mIncomingClients.ToArray();
               mIncomingClients.Clear();
            }
         }

         if (incomingClients != null)
         {
            foreach (Socket client in incomingClients)
            {
               NetServiceConnection cc = new NetServiceConnection(client);
               mClientConnections.Add(cc);
               if (mConnectedDelegates != null)
                  mConnectedDelegates(cc);
            }
         }
      }

      public delegate void ConnectedDelegate(NetServiceConnection clientConnection);
      public ConnectedDelegate mConnectedDelegates;

      public void listenThreadStart()
      {
         while (true)
         {
            Socket socket = TcpListener.AcceptSocket();
            lock (mIncomingClients)
            {
               mIncomingClients.Add(socket);
            }
         }
      }

      public void closeClientConnections()
      {
         lock (this)
         {
            foreach (Socket client in mIncomingClients)
            {
               client.Close();
            }
            mIncomingClients.Clear();

            ListenThread.Abort();
            TcpListener.Stop();
         }
      }


   }

   public interface IConnectionHandler
   {
      void SetConnection(NetServiceConnection connection);
      void HandleMessage(byte type, BinaryReader reader);

   }


   #region SeriviceListStuff

   public class NetServiceListClient
   {   
      string mHostIP;
      NetServiceConnection mListServerConnection = null;
      Dictionary<int, ServiceInfo> mServiceList = new Dictionary<int, ServiceInfo>();

      public NetServiceListClient(string hostIP, int hostPort)
      {
         mListServerConnection = new NetServiceConnection(hostIP, hostPort);
         mHostIP = hostIP;

         mListServerConnection.mReadDelegates += new NetServiceConnection.ReadDelegate(ReadData);
         mListServerConnection.mDisconnectedDelegates += new NetServiceConnection.DisconnectedDelegate(OnClientDisconnected);
         mListServerConnection.startRead();

         mListServerConnection.write(WriteListCommand());
      }

      public Dictionary<int, ServiceInfo> ListServices()
      {
         return null;
      }
      public void ReadData(NetServiceConnection clientConnection, byte type, BinaryReader reader)
      {
         if (type == (byte)ServiceListPacketTypes.cServiceList)
         {
            ReadServiceList(reader);
         }
      }
      public void OnClientDisconnected(NetServiceConnection clientConnection)
      {

      }


      public NetServiceConnection ConnectToService(int id)
      {
         if (mServiceList.ContainsKey(id) == false)
         {
            return null;
         }
         int connectToPort = mServiceList[id].mPort;

         return new NetServiceConnection(mHostIP, connectToPort);
      }

      enum ServiceListPacketTypes
      {
         cCommand,
         cServiceList
      };

      public void ReadServiceList(BinaryReader reader)
      {
         try
         {
            mServiceList.Clear();
            int numGames = reader.ReadInt32();
            for (int i = 0; i < numGames; i++)
            {
               ServiceInfo service = new ServiceInfo();
               service.mName = reader.ReadString();
               service.mID = reader.ReadInt32();
               service.mAvailable = reader.ReadBoolean();
               service.mPort = reader.ReadInt32();

               mServiceList[service.mID] = service;
            }
         }
         catch (Exception e)
         {
            Console.WriteLine(e.ToString());
         }
      }
    
      public class ServiceInfo
      {
         public string mName;
         public int mID;
         public bool mAvailable;
         public int mPort;
      }


      enum ListCommands
      {
         cGetList,
         cConnect
      };
     
      // read
      //public void ParseServiceCommand(BinaryReader reader)
      //{
      //   try
      //   {
      //      ListCommands command = (ListCommands)reader.ReadInt32();

      //      if (command == ListCommands.cConnect)
      //      {


      //         //mClientName = reader.ReadString();
      //         //mDebuggerAddress = reader.ReadString();
      //         //mDebuggerPort = reader.ReadInt32();
      //      }
      //   }
      //   catch (Exception e)
      //   {
      //      Console.WriteLine(e.ToString());
      //   }
      //}

      Stream WriteListCommand()
      {
         try
         {
            Stream stream = new MemoryStream();
            BinaryWriter writer = NetServiceConnection.GetBinaryWriter(stream);

            CTSPacketHeader header = new CTSPacketHeader(writer, (byte)ServiceListPacketTypes.cCommand);

            writer.Write((int)ListCommands.cGetList);

            //-- get the size
            header.writeSize(writer);
            return stream;
         }
         catch (Exception e)
         {
            Console.WriteLine(e.ToString());
            return null;
         }
      }
     
      //??
      Stream WriteConnectCommand(string debuggerAddress, Int32 debuggerPort, string clientName)
      {
         try
         {
            Stream stream = new MemoryStream();
            BinaryWriter writer = NetServiceConnection.GetBinaryWriter(stream);

            CTSPacketHeader header = new CTSPacketHeader(writer, (byte)ServiceListPacketTypes.cCommand);

            writer.Write((int)ListCommands.cConnect);
            writer.Write(clientName);
            writer.Write(debuggerAddress);
            writer.Write(debuggerPort);

            //-- get the size
            header.writeSize(writer);

            return stream;
         }
         catch (Exception e)
         {
            Console.WriteLine(e.ToString());
            return null;
         }
      }
   }

   #endregion

   /// <summary>
   /// 
   /// </summary>
   public class NetServiceHost
   {
      private TcpListener mTcpListener = null;
      private Thread mListenThread;

      private ArrayList mClientConnections = new ArrayList();

      public NetServiceHost(int listenPort)
      {
         mTcpListener = new TcpListener(System.Net.IPAddress.Any, listenPort);
         mTcpListener.Start();

         mListenThread = new Thread(new ThreadStart(listenThreadStart));
         mListenThread.Start();

         mListenDispatchTimer.Elapsed += new System.Timers.ElapsedEventHandler(mListenDispatchTimer_Elapsed);
         mListenDispatchTimer.Start();
      }   

      ~NetServiceHost()
      {
         mTcpListener.Stop();
         mListenThread.Abort();
      }

      public System.Timers.Timer mListenDispatchTimer = new System.Timers.Timer(100);

      private ArrayList mIncomingClients = new ArrayList();
      public void mListenDispatchTimer_Elapsed(object sender, ElapsedEventArgs e)
      {
         object[] incomingClients = null;
         if (mIncomingClients.Count > 0)
         {
            lock (mIncomingClients)
            {
               incomingClients = mIncomingClients.ToArray();
               mIncomingClients.Clear();
            }
         }

         if (incomingClients != null)
         {
            foreach (Socket client in incomingClients)
            {
               NetServiceConnection cc = new NetServiceConnection(client);
               mClientConnections.Add(cc);              
               if (mConnectedDelegates != null)
                  mConnectedDelegates(cc);
            }
         }
      }

      public delegate void ConnectedDelegate(NetServiceConnection clientConnection);
      public ConnectedDelegate mConnectedDelegates;

      public void listenThreadStart()
      {
         while (true)
         {
            Socket socket = mTcpListener.AcceptSocket();
            
            lock (mIncomingClients)
            {
               mIncomingClients.Add(socket);
            }
         }
      }

      public void closeClientConnections()
      {
         lock (this)
         {
            foreach (Socket client in mIncomingClients)
            {
               client.Close();
            }
            mIncomingClients.Clear();

            mListenThread.Abort();
            mTcpListener.Stop();
         }
      }
   }






}
