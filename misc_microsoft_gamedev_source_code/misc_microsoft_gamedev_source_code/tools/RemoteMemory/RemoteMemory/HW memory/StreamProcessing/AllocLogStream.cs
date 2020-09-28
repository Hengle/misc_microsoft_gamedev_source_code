using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Threading;

namespace RemoteMemory
{
   //============================================================================
   // ITcpServerListener
   //============================================================================
   public interface ITcpServerListener
   {
      void onMessageRecieved(PacketWrapper packet);
      void onClientConnected(string clientName, string pdbFile);
      void onClientDisconnected();
   };

   //============================================================================
   // packetWrapper
   //============================================================================
   public class PacketWrapper
   {
      public byte mPacketType;
      public int mPacketSize;
      public byte[] mPacketData;
   };


   //============================================================================
   // AllocLogStream
   //============================================================================
   partial class AllocLogStream
   {
      static int mIsRunning = 0;
      static int mIsPaused = 0;
      static string cTempLoggingFile = AppDomain.CurrentDomain.BaseDirectory + "\\_tmpLog.bin";
      static FileStream mLoggedFileStream = null;
      static BinaryWriter mBinaryWriter = null;

      static UInt64 mNumBytesWritten = 0;
      static UInt64 mNumBytesRead = 0;

      //============================================================================
      // saveStreamAs
      //============================================================================
      static public void saveStreamAs(string filename)
      {
         if (!File.Exists(cTempLoggingFile))
            return;

         if (File.Exists(filename))
            File.Delete(filename);

         File.Copy(cTempLoggingFile, filename);
      }

      //============================================================================
      // forceOnConnectEvent
      //============================================================================
      static void forceOnConnectEvent(string clientName, string pdbFilename)
      {
         if (File.Exists(cTempLoggingFile))
            File.Delete(cTempLoggingFile);



         mLoggedFileStream = new FileStream(cTempLoggingFile, FileMode.CreateNew);
         mBinaryWriter = new BinaryWriter(mLoggedFileStream);

         for (int i = 0; i < mListeners.Count; i++)
            mListeners[i].onClientConnected(clientName, pdbFilename);
      }

      //============================================================================
      // forceOnDisconnectEvent
      //============================================================================
      static void forceOnDisconnectEvent()
      {
         for (int i = 0; i < mListeners.Count; i++)
            mListeners[i].onClientDisconnected();

         mBinaryWriter.Flush();
         mLoggedFileStream.Close();
         mBinaryWriter.Close();
      }


      //============================================================================
      // stopProcessing
      //============================================================================
      static public void stopProcessing()
      {
         Interlocked.Exchange(ref mIsRunning, 0);
      }


      //============================================================================
      // isProcessing
      //============================================================================
      static public bool isProcessing()
      {
         return mIsRunning == 0 ? false : true;
      }

      //============================================================================
      // pauseProcessing
      //============================================================================
      static public void pauseProcessing()
      {
         Interlocked.Exchange(ref mIsPaused, 1);
      }

      //============================================================================
      // unpauseProcessing
      //============================================================================
      static public void unpauseProcessing()
      {
         Interlocked.Exchange(ref mIsPaused, 0);
      }
      //============================================================================
      // addListener
      //============================================================================
      private static void recieveProcessPacket(byte[] header, byte packetType, int packetSize,ref byte[] packetData)
      {
         PacketWrapper pkt = new PacketWrapper();
         pkt.mPacketType = packetType;
         pkt.mPacketSize = packetSize;
         pkt.mPacketData = null;
         if (packetSize != 0)
         {
            pkt.mPacketData = new byte[packetSize];
            packetData.CopyTo(pkt.mPacketData, 0);
         }



         if (((HaloWarsMem.eALPacketType)packetType) == HaloWarsMem.eALPacketType.cALEOF)
         {
            stopProcessing();
         }



         for (int i = 0; i < mListeners.Count; i++)
            mListeners[i].onMessageRecieved(pkt);



         //whatever the stream is, log it out so we can use the 'save' commands properly.
         mBinaryWriter.Write(header, 0, 4);
         if (packetData != null)
            mBinaryWriter.Write(packetData, 0, packetSize);
      }
      //============================================================================
      // addListener
      //============================================================================
      static public void addListener(ITcpServerListener list)
      {
         mListeners.Add(list);
      }

      //============================================================================
      // clearListeners
      //============================================================================
      static public void clearListeners()
      {
         mListeners.Clear();
      }

      static List<ITcpServerListener> mListeners = new List<ITcpServerListener>();
   }
}
