using System;
using System.Windows.Forms;
using System.Collections;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;

namespace GetBuild
{
   //============================================================================
   // Server
   //============================================================================
   static public class Server
   {
      const int cSocketPort = 1010;
      const int cMaxConnections = 100;

      static public string mName = null;
      static public string mIP = null;
      static private ReaderWriterLock mServerRWLock = new ReaderWriterLock();
      static private int mServerLockCount = 0;
      static private volatile bool mActive = false;
      static private volatile bool mKillAcceptConnectionsThread = false;
      static private Thread mThread = null;
      static private TcpListener mTcpListener = null;
      static private int mNextClientID = 0;
      static private ArrayList mClientList = null;
      static private ReaderWriterLock mClientListRWLock = new ReaderWriterLock();
      static private int mNextConnectionID = 0;
      static private ArrayList mConnectionList = null;
      static private ReaderWriterLock mConnectionListRWLock = new ReaderWriterLock();
      static private int mFileListIndex = 0;
      static private Hashtable mFileList = null;
      static private ReaderWriterLock mFileListRWLock = new ReaderWriterLock();
      static private Dictionary<string, int> mValidFiles = null;
      static private int mConnectionCounter = 0;
      static private ReaderWriterLock mConnectionCounterRWLock = new ReaderWriterLock();

      static public ArrayList mEvents1 = null;
      static public ArrayList mEvents2 = null;
      static public ArrayList mCurrentEvents = null;
      static public List<string> mIPs = new List<string>();
      static public bool mDisableTimeout = false;
      static public bool mEnableNoDelay = true;

      //============================================================================
      // Server.setup
      //============================================================================
      public static bool setup(string defaultIP)
      {

         mClientList = new ArrayList();
         mConnectionList = new ArrayList();
         mFileList = new Hashtable();
         mFileListIndex = 0;
         mValidFiles = new Dictionary<string, int>();
         mEvents1 = new ArrayList();
         mEvents2 = new ArrayList();
         mCurrentEvents = mEvents1;

         try
         {
            mName = Dns.GetHostName();
            IPAddress[] ips = Dns.GetHostAddresses(mName);
            if(defaultIP != null)
            {
               foreach (IPAddress ip in ips)
               {
                  if(ip.ToString()==defaultIP)
                  {
                     mIP = defaultIP;
                     break;
                  }
               }
            }
            foreach (IPAddress ip in ips)
            {
               mIPs.Add(ip.ToString());
               if (mIP == null)
                  mIP = ip.ToString();
            }
            mTcpListener = new TcpListener(IPAddress.Any, cSocketPort);
            mTcpListener.ExclusiveAddressUse = false;
            mTcpListener.Start();
         }
         catch (Exception ex)
         {
            logException(ex);
            return false;
         }

         return true;
      }

      //============================================================================
      // Server.start
      //============================================================================
      public static void start()
      {
         mActive = true;
         mKillAcceptConnectionsThread = false;
         mThread = new Thread(new ThreadStart(acceptConnections));
         mThread.IsBackground = true;
         mThread.Start();
      }

      //============================================================================
      // Server.lockServer
      //============================================================================
      private static bool lockServer()
      {
         if (!mActive)
            return false;

         Interlocked.Increment(ref mServerLockCount);

         return true;
      }

      //============================================================================
      // Server.unlockServer
      //============================================================================
      private static void unlockServer()
      {
         Interlocked.Decrement(ref mServerLockCount);
      }

      //============================================================================
      // Server.acceptConnections
      //============================================================================
      public static void acceptConnections()
      {
         try
         {
            for(;;)
            {
               TcpClient tcpClient = mTcpListener.AcceptTcpClient();
               if (tcpClient == null)
                  break;
               
               // rg [6/13/06] - Use a separate flag (instead of mActive) to kill the accept thread so we can safely 
               // reset the server without loosing this worker thread.
               if (mKillAcceptConnectionsThread)
                  break;
                  
               if(lockServer())
               {
                  try
                  {
                     onConnect(tcpClient);
                  }
                  finally
                  {
                     unlockServer();
                  }
               }                  
            }
         }
         catch
         {
         }
         
         mKillAcceptConnectionsThread = false;
      }

      //============================================================================
      // Server.shutdown
      //============================================================================
      public static void shutdown()
      {
         output("Shutting down");

         // safe to set these outside a lock
         mActive = false;
         mKillAcceptConnectionsThread = true;

         while (mServerLockCount > 0)
            Thread.Sleep(100);

         mConnectionListRWLock.AcquireWriterLock(-1);
         try
         {
            foreach (Connection connection in mConnectionList)
            {
               if (connection != null)
                  connection.disconnect();
            }
            mConnectionList.Clear();
            mConnectionCounterRWLock.AcquireWriterLock(-1);
            mConnectionCounter = 0;
            mConnectionCounterRWLock.ReleaseWriterLock();
         }
         finally
         {
            mConnectionListRWLock.ReleaseWriterLock();
         }

         mClientListRWLock.AcquireWriterLock(-1);
         try
         {
            mClientList.Clear();
         }
         finally
         {
            mClientListRWLock.ReleaseWriterLock();
         }

         mFileListRWLock.AcquireWriterLock(-1);
         try
         {
            foreach (FileStream fs in mFileList.Values)
            {
               if (fs != null)
                  fs.Close();
            }
            mFileList.Clear();
         }
         finally
         {
            mFileListRWLock.ReleaseWriterLock();
         }

         lock (((ICollection)mValidFiles).SyncRoot)
         {
            mValidFiles.Clear();
         }

         if (mTcpListener != null)
         {
            mTcpListener.Stop();
            mTcpListener = null;
         }

         if(mThread!=null)
         {
            mThread.Abort();
            mThread=null;
         }
      }

      public static bool hasActiveConnections()
      {
         // .Count is thread safe
         return (mConnectionList.Count > 0);
      }

      //============================================================================
      // Server.reset
      //============================================================================
      public static bool reset(bool permitTimeout)
      {
         // safe to set this outside a lock
         mActive = false;

         int retryCount = 0;
         while (mServerLockCount > 0)
         {
            Thread.Sleep(100);
            
            retryCount++;
            if ((permitTimeout) && (retryCount > 180))
               return false;
         }

         mConnectionListRWLock.AcquireWriterLock(-1);
         try
         {
            foreach (Connection connection in mConnectionList)
            {
               if (connection != null)
               {
                  if (connection.lockConnection())
                  {
                     string ip = connection.getIP();
                     connection.disconnect();
                     connection.unlockConnection();
                     addEvent(NetEvent.EventType.ConnectionClose, ip);
                  }
               }
            }
            mConnectionList.Clear();
         }
         finally
         {
            mConnectionListRWLock.ReleaseWriterLock();
         }

         mClientListRWLock.AcquireWriterLock(-1);
         try
         {
            foreach (Client client in mClientList)
               addEvent(NetEvent.EventType.ClientDisconnect, client.getIP());
            mClientList.Clear();
         }
         finally
         {
            mClientListRWLock.ReleaseWriterLock();
         }

         mFileListRWLock.AcquireWriterLock(-1);
         try
         {
            foreach (FileStream fs in mFileList.Values)
            {
               if (fs != null)
                  fs.Close();
            }
            mFileList.Clear();
         }
         finally
         {
            mFileListRWLock.ReleaseWriterLock();
         }

         lock (((ICollection)mValidFiles).SyncRoot)
         {
            mValidFiles.Clear();
         }

         mNextClientID = 0;
         mNextConnectionID = 0;

         mActive = true;
         
         return true;
      }

      //============================================================================
      // Server.onConnect
      //============================================================================
      public static bool onConnect(TcpClient tcpClient)
      {
         mConnectionListRWLock.AcquireWriterLock(-1);
         try
         {
            if (mConnectionList.Count >= cMaxConnections)
               return false;

            // Get or create the client
            Client client = null;
            string clientIP = ((IPEndPoint)tcpClient.Client.RemoteEndPoint).Address.ToString();
            int availClientID = mNextClientID;
            mClientListRWLock.AcquireReaderLock(-1);
            try
            {
               foreach (Client clientItem in mClientList)
               {
                  if (clientItem.getIP() == clientIP)
                  {
                     client = clientItem;
                     break;
                  }
               }
               if(client == null)
               {
                  for(int checkID = 0; checkID < mNextClientID; checkID++)
                  {
                     bool idInUse=false;
                     foreach (Client clientItem in mClientList)
                     {
                        if(clientItem.getID()==checkID)
                        {
                           idInUse = true;
                           break;
                        }
                     }
                     if(!idInUse)
                     {
                        availClientID = checkID;
                        break;
                     }
                  }
               }
            }
            finally
            {
               mClientListRWLock.ReleaseReaderLock();
            }

            bool newClient = false;
            if (client == null)
            {
               int clientID = availClientID;
               if(availClientID == mNextClientID)
                  mNextClientID++;
               client = new Client(clientID, clientIP);
               mClientListRWLock.AcquireWriterLock(-1);
               try
               {
                  mClientList.Add(client);
               }
               finally
               {
                  mClientListRWLock.ReleaseWriterLock();
               }
               
               newClient = true;
            }

            // Setup the new connection
            int connectionID = mNextConnectionID;
            mNextConnectionID++;
            Connection connection = new Connection(connectionID, client);
            if (!connection.connect(tcpClient))
            {
               return false;
            }

            if(newClient)
               addEvent(NetEvent.EventType.ClientConnect, client.getIP());

            addEvent(NetEvent.EventType.ConnectionOpen, connection.getIP());

            mConnectionList.Add(connection);
            connection.start();

            mConnectionCounterRWLock.AcquireWriterLock(-1);
            mConnectionCounter++;
            mConnectionCounterRWLock.ReleaseWriterLock();
         }
         finally
         {
            mConnectionListRWLock.ReleaseWriterLock();
         }

         return true;
      }

      //============================================================================
      // Server.onDisconnect
      //============================================================================
      public static void onDisconnect(Connection connection)
      {
         if (!lockServer())
            return;
         
         try
         {
            mConnectionListRWLock.AcquireWriterLock(-1);
            try
            {
               mConnectionList.Remove(connection);

               mConnectionCounterRWLock.AcquireWriterLock(-1);
               mConnectionCounter--;
               mConnectionCounterRWLock.ReleaseWriterLock();

               addEvent(NetEvent.EventType.ConnectionClose, connection.getIP());

               Client client = connection.getClient();

               connection.disconnect();

               mClientListRWLock.AcquireWriterLock(-1);
               try
               {
                  if (client != null && client.getConnectionCount() == 0)
                  {
                     mClientList.Remove(client);
                     addEvent(NetEvent.EventType.ClientDisconnect, client.getIP());
                  }
               }
               finally
               {
                  mClientListRWLock.ReleaseWriterLock();
               }
            }
            finally
            {
               mConnectionListRWLock.ReleaseWriterLock();
            }
         }
         finally
         {
            unlockServer();
         }
      }

      //============================================================================
      // Server.lockConnection
      //============================================================================
      public static bool lockConnection(Connection connection)
      {
         if (!lockServer())
            return false;

         mConnectionListRWLock.AcquireReaderLock(-1);
         try
         {
            if (!connection.lockConnection())
            {
               unlockServer();
               return false;
            }
         }
         finally
         {
            mConnectionListRWLock.ReleaseReaderLock();
         }

         return true;
      }

      //============================================================================
      // Server.unlockConnection
      //============================================================================
      public static void unlockConnection(Connection connection)
      {
         mConnectionListRWLock.AcquireReaderLock(-1);
         try
         {
            connection.unlockConnection();
         }
         finally
         {
            mConnectionListRWLock.ReleaseReaderLock();
         }

         unlockServer();
      }

      //============================================================================
      // Server.resetClient
      //============================================================================
      public static void resetClient(Connection skipConnection)
      {
         mConnectionListRWLock.AcquireWriterLock(-1);
         try
         {
            Client client = skipConnection.getClient();

            //bool anyReset = false;
            for (int i = mConnectionList.Count - 1; i >= 0; i--)
            {
               Connection connection = (Connection)mConnectionList[i];
               if (connection != skipConnection && connection.getClient() == client)
               {
                  if (connection.lockConnection())
                  {
                     string ip = connection.getIP();
                     connection.disconnect();
                     connection.unlockConnection();
                     addEvent(NetEvent.EventType.ConnectionReset, ip);
                  }

                  mConnectionList.RemoveAt(i);
                  mConnectionCounterRWLock.AcquireWriterLock(-1);
                  mConnectionCounter--;
                  mConnectionCounterRWLock.ReleaseWriterLock();
               }
            }
         }
         finally
         {
            mConnectionListRWLock.ReleaseWriterLock();
         }

         addEvent(NetEvent.EventType.ResetClient, "");
      }

      //============================================================================
      // 
      //============================================================================
      public static void logException(System.Exception ex)
      {
         addEvent(NetEvent.EventType.Exception, ex.ToString());
      }

      //============================================================================
      // Server.output
      //============================================================================
      public static void output(string text)
      {
         addEvent(NetEvent.EventType.General, text);
      }

      //============================================================================
      // Server.openFile
      //============================================================================
      public static int openFile(string path, FileMode mode, FileAccess access, FileShare share, int connectionID)
      {
         bool wasPreviouslyOpened;

         lock (((ICollection)mValidFiles).SyncRoot)
         {
            wasPreviouslyOpened = mValidFiles.ContainsKey(path);
         }

         bool writeMode = false;
         if (mode == System.IO.FileMode.Create || access == System.IO.FileAccess.Write || access == System.IO.FileAccess.ReadWrite)
            writeMode = true;
         
         // Attempt to open the file
         FileStream fs=null;
                  
         if (!writeMode)
         {
            // If the file was previously opened, it most likely still exists so it's OK to retry opening it for a few secs.
            // Retrying may be needed if a tool is currently writing the file.
            // If the file hasn't been previously opened, only try to open it if it exists. 
            // We don't want to retry nonexisting files because the game would take forever to load due to missing files.

            if ((File.Exists(path)) || (wasPreviouslyOpened))
            {
               // rg [6/3/06] - Retry for a few seconds in case the file is currently being written by a tool during a reload.
               const int cTimesToRetry = 30;
               for (int i = 0; i < cTimesToRetry; i++)
               {
                  try
                  {
                     fs = File.Open(path, mode, access, share);
                  }
                  catch (System.Exception ex)
                  {
                     addEvent(NetEvent.EventType.Exception, String.Format("path:{0}\n{1}", path, ex.ToString()));
                  }
                  
                  if (fs != null)
                     break;

                  Thread.Sleep(100);
               }
            }
         }
         else 
         {
            try
            {
               fs = File.Open(path, mode, access, share);
            }
            catch
            {
               // To support multiple connections, create a unique name for file creations.
               //if (mode == FileMode.Create)
               {
                  // Attempt to open the file with a unique name
                  string dir = Path.GetDirectoryName(path);
                  if (dir.Length > 0)
                     dir += "\\";
                  string name = Path.GetFileNameWithoutExtension(path);
                  string ext = Path.GetExtension(path);
                  path = dir + name + connectionID + ext;
                  try
                  {
                     fs = File.Open(path, mode, access, share);
                  }
                  catch (Exception ex)
                  {
                     addEvent(NetEvent.EventType.Exception, String.Format("path:{0}\n{1}", path, ex.ToString()));
                  }
               }
            }
         }
         if (fs == null)
         {
            output("Open failed " + path);
            return -1;
         }
         else
            addEvent(NetEvent.EventType.FileOpen, fs.Name);

         if (!wasPreviouslyOpened)
         {
            lock (((ICollection)mValidFiles).SyncRoot)
            {
               // ajl 6/27/06 - Added check here because the path could have been
               // added by another thread after our wasPreviouslyOpened value was set.
               // The program was hanging here without this check.
               if (!mValidFiles.ContainsKey(path))
                  mValidFiles.Add(path, 0);
            }
         }

         // Add the file to the file list
         int index = -1;
         mFileListRWLock.AcquireWriterLock(-1);
         try
         {
            // find the next available file index
            // first start by trying the last known file list index + 1
            index = mFileListIndex;
            while (true)
            {
               if (index == int.MaxValue)
                  index = 0;
               if (!mFileList.ContainsKey(index))
                  break;
               index += 1;
            }
            mFileList[index] = fs;
            mFileListIndex = index + 1;
         }
         finally
         {
            mFileListRWLock.ReleaseWriterLock();
         }

         return index;
      }

      //============================================================================
      // Server.closeFile
      //============================================================================
      public static void closeFile(int index)
      {
         FileStream fs = null;

         mFileListRWLock.AcquireWriterLock(-1);
         try
         {
            fs = mFileList[index] as FileStream;
            mFileList.Remove(index);
         }
         finally
         {
            mFileListRWLock.ReleaseWriterLock();
         }

         if (fs != null)
         {
            addEvent(NetEvent.EventType.FileClose, fs.Name);
            fs.Close();
         }
      }

      //============================================================================
      // Server.getFileStream
      //============================================================================
      public static FileStream getFileStream(int index)
      {
         if(index==-1)
            return null;

         mFileListRWLock.AcquireReaderLock(-1);
         try
         {
            return mFileList[index] as FileStream;
         }
         finally
         {
            mFileListRWLock.ReleaseReaderLock();
         }
      }

      //============================================================================
      // Server.remapPath
      //============================================================================
      public static string remapPath(char[] fileName, short fileLen)
      {
         string path = new string(fileName);
         return path;

         /*
         int loc=-1;
         for(int i=0; i<fileLen; i++)
         {
            if(fileName[i]==':')
            {
               loc=i;
               if(loc<fileLen-2)
                  loc+=2;
               break;
            }
         }

         if(loc<1)
         {
            string path=new string(fileName);
            return path;
         }
         else
         {
            string path=new string(fileName, loc, fileLen-loc);
            return path;
         }
         */
      }

      public static ArrayList getEventList()
      {
         lock (mCurrentEvents.SyncRoot)
         {
            ArrayList list = mCurrentEvents;
            if (mCurrentEvents == mEvents1)
               mCurrentEvents = mEvents2;
            else
               mCurrentEvents = mEvents1;
            return list;
         }
      }

      public static void addEvent(NetEvent.EventType eventType, string eventText)
      {
         NetEvent netEvent = new NetEvent();
         netEvent.mType = eventType;
         netEvent.mText = eventText;
         lock (mCurrentEvents.SyncRoot)
         {
            mCurrentEvents.Add(netEvent);
         }
      }
      public static void forceClearWaitingEvents()
      {
         lock (mCurrentEvents.SyncRoot)
         {
            mCurrentEvents.Clear();
         }
      }

      //============================================================================
      // Server.haveConnection
      //============================================================================
      public static int getConnectionCounter()
      {
         mConnectionCounterRWLock.AcquireReaderLock(-1);
         int counter = mConnectionCounter;
         mConnectionCounterRWLock.ReleaseReaderLock();
         return counter;
      }
   }
}
