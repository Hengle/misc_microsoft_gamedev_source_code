using System;
using System.Collections.Generic;
using System.Text;
using System.Net;


namespace RemoteTools
{


   //class NetServiceMain
   //{

   //   private NetServiceHost cmgr = null;
   //   private NetServiceHost debuggerCMGR = null;

   //   public List<NetServiceConnection> mConnectedServices = new List<NetServiceConnection>();

   //   NetServiceConnection mMasterConection = null;


   //   public NetServiceMain()
   //   {
   //      //cmgr = new NetServiceManager(1337);
   //      //cmgr.mConnectedDelegates += new NetServiceManager.ConnectedDelegate(OnNewClientConnection);

  
   //   }
   //   public void OnNewClientConnection(NetServiceConnection clientConnection)
   //   {
   //      mActiveClient = clientConnection;

   //      // a New client has arrived, hook up the delagates.
   //      mActiveClient.mReadDelegates += new NetServiceConnection.ReadDelegate(readData);
   //      mActiveClient.mDisconnectedDelegates += new NetServiceConnection.DisconnectedDelegate(OnClientDisconnected);
   //      mActiveClient.startRead();

   //   }

   //   public void OnNewDebuggerClientConnection(NetServiceConnection clientConnection)
   //   {
   //      mActiveClient = clientConnection;
   //      mDebuggerClients.Add(clientConnection);

   //      // a New client has arrived, hook up the delagates.
   //      mActiveClient.mReadDelegates += new NetServiceConnection.ReadDelegate(readData);
   //      mActiveClient.mDisconnectedDelegates += new NetServiceConnection.DisconnectedDelegate(OnClientDisconnected);
   //      mActiveClient.startRead();

   //   }

   //   public void OnClientDisconnected(NetServiceConnection clientConnection)
   //   {
   //      //mGamesListWindow.listBox1.Items.Remove(clientConnection.mConnectionName);
   //      ////-- If we get down to zero, reEnabled the listbox.
   //      //if (mGamesListWindow.listBox1.Items.Count <= 0)
   //      //{
   //      //   mGamesListWindow.sessionReset();
   //      //}
   //      //since everything uses client 0 we must clear the list so that the next valid connection will be client 0 again
   //      mConnectedClients.Clear();
   //      mDebuggerClients.Clear();

   //      //this.Text = "RemoteGameDebugger: Disconnected";
   //   }
   //   bool mbAttached = false;
   //   public void readData(NetServiceConnection clientConnection, byte type, BinaryReader reader)
   //   {
   //      if (mbAttached == true)
   //         return;
   //      reader = NetServiceConnection.GetBinaryReader(reader.BaseStream);

   //      if (type == (byte)GameAppDebuggerPacketTypes.cDebuggerDettach)
   //      {
   //         //-- game has gone away, dettach thyself.
   //         sessionReset();

   //      }
   //      else if (type == (byte)GameAppDebuggerPacketTypes.cAdvertiseGame)
   //      {
   //         AdvertiseGame p = new AdvertiseGame(reader);
   //         String s = p.mGameName;

   //         if (p.mAdvertise == true)
   //         {
   //            clientConnection.mClientName = p.mGameName;
   //            //mGamesListWindow.listBox1.Items.Add(clientConnection.mConnectionName);
   //            mConnectedClients.Add(clientConnection);
   //            //if (mGamesListWindow.listBox1.Items.Count == 1)
   //            //{
   //            //   mGamesListWindow.listBox1.SelectedIndex = 0;
   //            //}
   //            mbAttached = true;
   //         }

   //         //int index = mGamesListWindow.listBox1.FindStringExact(p.mGameName);
   //         ////-- didn't find game, add it.
   //         //if (index == ListBox.NoMatches)
   //         //{
   //         //   if (p.mAdvertise == true)
   //         //   {
   //         //      clientConnection.mConnectionName = p.mGameName;
   //         //      mGamesListWindow.listBox1.Items.Add(clientConnection.mConnectionName);
   //         //      mConnectedClients.Add(clientConnection);
   //         //      if (mGamesListWindow.listBox1.Items.Count == 1)
   //         //      {
   //         //         mGamesListWindow.listBox1.SelectedIndex = 0;
   //         //      }
   //         //   }
   //         //}
   //         //else
   //         //{
   //         //   //-- we found a match. remove if advert is false
   //         //   if (p.mAdvertise == false)
   //         //   {
   //         //      mGamesListWindow.listBox1.Items.RemoveAt(index);
   //         //   }
   //         //}
   //      }     
   //      else
   //      {
   //         byte[] bytes = reader.ReadBytes(100);

   //         //MessageBox.Show(this, "bad packet");
   //      }

   //   }

   //   public NetServiceConnection ActiveClient
   //   {
   //      get
   //      {
   //         return mActiveClient;
   //      }
   //   }

   //   ///////////////////////////////////////////////////
   
   //   public void ConnectDebugger(int clientID)
   //   {
   //      if (mConnectedClients.Count <= clientID)
   //         return;
   //      NetServiceConnection client = mConnectedClients[clientID];
   
   //      //-- tell game we want to attach.
   //      MemoryStream memStream = new MemoryStream();
     
   //      // Determine the IPAddress of this machine
   //      string strHostName = "";
   //      // NOTE: DNS lookups are nice and all but quite time consuming.
   //      strHostName = Dns.GetHostName();
   //      IPHostEntry ipEntry = Dns.GetHostByName(strHostName);
   //      DebuggerAttach toGame = new DebuggerAttach(memStream, ipEntry.AddressList[0].ToString(), 1339, client.mClientName);
   //      client.write(memStream);
   //   }

   //   public void DebuggerDisconnect(int clientID)
   //   {
   //      if (mDebuggerClients.Count <= clientID)
   //         return;

   //      ////-- send disconnect packet to game.
   //      MemoryStream memStream = new MemoryStream();
   //      DettachDebugger setBP = new DettachDebugger(memStream);
   //      NetServiceConnection client = mDebuggerClients[clientID];
   //      client.write(memStream);

   //      ////-- reEnable the listbox.
   //      //sessionReset();
   //   }

   //   public void sessionReset()
   //   {
   //      //listBox1.Enabled = true;
   //      //mDebuggerDisconnect.Enabled = false;
   //      //((MainForm)this.ParentForm).ResetGameSessionData();
   //   }



   //   //////////////////////////////////////////////////////////////////////////



   //   #region IDisposable Members

   //   public void Dispose()
   //   {
   //      cmgr.closeClientConnections();
   //      cmgr = null;

   //      debuggerCMGR.closeClientConnections();
   //      debuggerCMGR = null;

   //      GC.Collect();
   //      GC.WaitForPendingFinalizers();
   //   }

   //   #endregion
   //}

   //enum GameAppDebuggerPacketTypes
   //{
   //   cFirstPacketType = 1,
   //   cAdvertiseGame = cFirstPacketType,
   //   cDebuggerAttach,
   //   cDebuggerDettach,
   //   cConsoleCommand,
   //   cNumberOfTypedPackets
   //};
   //public class AdvertiseGame
   //{
   //   public AdvertiseGame(BinaryReader reader)
   //   {
   //      try
   //      {

   //         mAdvertise = reader.ReadBoolean();
   //         mGameName = reader.ReadString();

   //      }
   //      catch (Exception e)
   //      {
   //         Console.WriteLine(e.ToString());
   //      }
   //   }

   //   public bool mAdvertise;
   //   public string mGameName;
   //}

   //public class DebuggerAttach
   //{
   //   // read
   //   public DebuggerAttach(BinaryReader reader)
   //   {
   //      try
   //      {

   //         mClientName = reader.ReadString();
   //         mDebuggerAddress = reader.ReadString();
   //         mDebuggerPort = reader.ReadInt32();
   //      }
   //      catch (Exception e)
   //      {
   //         Console.WriteLine(e.ToString());
   //      }
   //   }

   //   // write
   //   public DebuggerAttach(Stream stream, string debuggerAddress, Int32 debuggerPort, string clientName)
   //   {
   //      try
   //      {
   //         //BinaryWriter writer = new BinaryWriter(stream, System.Text.Encoding.Unicode);
   //         BinaryWriter writer = NetServiceConnection.GetBinaryWriter(stream);

   //         CTSPacketHeader header = new CTSPacketHeader(writer, (byte)GameAppDebuggerPacketTypes.cDebuggerAttach);

   //         writer.Write(clientName);
   //         writer.Write(debuggerAddress);
   //         writer.Write(debuggerPort);

   //         //-- get the size
   //         header.writeSize(writer);
   //      }
   //      catch (Exception e)
   //      {
   //         Console.WriteLine(e.ToString());
   //      }
   //   }

   //   public string mClientName;
   //   public string mDebuggerAddress;
   //   public Int32 mDebuggerPort;
   //}


}
