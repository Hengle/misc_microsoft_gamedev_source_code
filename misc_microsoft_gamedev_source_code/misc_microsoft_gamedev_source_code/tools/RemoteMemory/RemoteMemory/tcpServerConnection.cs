using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace RemoteMemory
{


   //============================================================================
   // TcpServerConnection
   //============================================================================
   class TcpServerConnection
   {
      TcpListener tcpListener;
      Thread listenThread;

      TcpClient mTCPClient;


      string mPDBFilename = "";
      //============================================================================
      // waitFor360ToConnect
      //============================================================================
      public void waitFor360ToConnect(string pdbFilename)
      {
         mPDBFilename = pdbFilename;

         tcpListener = new TcpListener(IPAddress.Any, 2887);
         listenThread = new Thread(new ThreadStart(ListenForClients));

         listenThread.Start();
      }

      //============================================================================
      // ListenForClients
      //============================================================================
      private void ListenForClients()
      {
         try
         {
            this.tcpListener.Start();
            this.tcpListener.Server.SetSocketOption( SocketOptionLevel.IP, SocketOptionName.ReuseAddress, true);
            //blocks until a client has connected to the server
            
            mTCPClient = this.tcpListener.AcceptTcpClient();
         }
         catch(Exception e)
         {
            GlobalErrors.addError("SHIT!");
         }
         


         //create a thread to handle communication with connected client
         Thread clientThread = new Thread(new  ParameterizedThreadStart(HandleClientComm));
         clientThread.Start(mTCPClient);
         
      }

      //============================================================================
      // stopListener
      //============================================================================
      public void stopListener()
      {
         
         tcpListener.Stop();
      }

      //============================================================================
      // HandleClientComm
      //============================================================================
      private void HandleClientComm(object client)
      {
         TcpClient tcpClient = (TcpClient)client;
         if (tcpClient == null)
         {
            stopListener();
            return;
         }

         NetworkStream clientStream = tcpClient.GetStream();

         
         IPEndPoint ipend = (IPEndPoint)(tcpClient.Client.RemoteEndPoint);


         AllocLogStream.processNetworkStream(ipend.Address.ToString(), clientStream, mPDBFilename, tcpClient);

         stopListener();
         tcpClient.Close();

      }

    
   }
}
