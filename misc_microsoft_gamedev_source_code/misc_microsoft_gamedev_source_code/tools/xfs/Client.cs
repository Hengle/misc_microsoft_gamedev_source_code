using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace xfs
{
   //============================================================================
   // Client
   //============================================================================
   public class Client
   {
      private int mID = -1;
      private string mIP = null;
      private ArrayList mConnectionList = null;

      //============================================================================
      // Client constructor
      //============================================================================
      public Client(int clientID, string ip)
      {
         mID = clientID;
         mIP = ip;
         mConnectionList = new ArrayList();
      }

      //============================================================================
      // Client.getID
      //============================================================================
      public int getID()
      {
         return mID;
      }

      //============================================================================
      // Client.getIP
      //============================================================================
      public string getIP()
      {
         return mIP;
      }

      //============================================================================
      // Client.getConnectionCount
      //============================================================================
      public int getConnectionCount()
      {
         int count = 0;
         lock (mConnectionList.SyncRoot)
         {
            count = mConnectionList.Count;
         }
         return count;
      }

      //============================================================================
      // Client.addConnection
      //============================================================================
      public void addConnection(Connection connection)
      {
         lock (mConnectionList.SyncRoot)
         {
            mConnectionList.Add(connection);
         }
      }

      //============================================================================
      // Client.removeConnection
      //============================================================================
      public void removeConnection(Connection connection)
      {
         lock (mConnectionList.SyncRoot)
         {
            mConnectionList.Remove(connection);
         }
      }
   }
}