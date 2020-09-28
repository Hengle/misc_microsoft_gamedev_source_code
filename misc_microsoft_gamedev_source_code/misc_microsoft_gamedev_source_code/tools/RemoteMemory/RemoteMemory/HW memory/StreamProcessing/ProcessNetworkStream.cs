using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Threading;

using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace RemoteMemory
{
   partial class AllocLogStream
   {
      

      static Thread mNetworkReaderThread;
      

      //============================================================================
      // initNetworkReaderThread
      //============================================================================
      static void initNetworkReaderThread(string tmpFile, NetworkStream pStream, TcpClient pClient)
      {
         networkThreadStartParams parms = new networkThreadStartParams();
         parms.mNetworkBinaryFilename = tmpFile;
         parms.mNetworkStream = pStream;
         parms.mTCPClient = pClient;

         mNetworkReaderThread = new Thread(new ParameterizedThreadStart(readNetworkStream));
         mNetworkReaderThread.Start(parms);
      }

      class networkThreadStartParams
      {
         public TcpClient mTCPClient;
         public NetworkStream mNetworkStream;
         public string mNetworkBinaryFilename;
      };
      //============================================================================
      // ListenForClients
      //============================================================================
      static private void readNetworkStream(object obj)
      {
         networkThreadStartParams pParms = (networkThreadStartParams)obj;

        

         FileStream lfs = null;
         try
         {
            lfs = new FileStream(pParms.mNetworkBinaryFilename, FileMode.OpenOrCreate, FileAccess.Write, FileShare.ReadWrite);
         }
         catch(Exception ex)
         {

         }
         
         BinaryWriter bw = new BinaryWriter(lfs);

         int cMaxSizeToRead = 0x00FFFFFF;
         byte[] buff = new byte[cMaxSizeToRead];
         mNumBytesWritten = 0;
         //spinloop on the network stream and write the data to a temp file.
         while (mIsRunning > 0)
         {
            try
            {
               if (mIsPaused == 1)
                  continue;

               int totalBytesRead = pParms.mNetworkStream.Read(buff, 0, cMaxSizeToRead);
               bw.Write(buff, 0, totalBytesRead);

               
               //CLM this isn't atomic, or threadsafe, but worst case is the reader skips 
               // a read cycle, which isn't bad at all.
               mNumBytesWritten += (ulong)totalBytesRead;
               
            }
            catch (Exception e)
            {

            }
         }

         bw.Flush();
         lfs.Close();
         bw.Close();
      }

      


      //============================================================================
      // fillBufferFromStream
      //============================================================================
      static public bool fillBufferFromStream(BinaryReader pStream, byte[] buff, int sizeToRead, TcpClient pClient)
      {
         int totalBytesRead = 0;
         while (totalBytesRead < sizeToRead)
         {
               totalBytesRead += pStream.Read(buff, totalBytesRead, sizeToRead - totalBytesRead);

            if (mIsRunning == 0 || pClient.Connected == false)
               return false;
         }
         return true;
      }

      //============================================================================
      // processNetworkStream
      //============================================================================
      static public void processNetworkStream(string IPAddy, NetworkStream pStream, string pdbFilename, TcpClient pClient)
      {
         forceOnConnectEvent(IPAddy, pdbFilename);
         Interlocked.Exchange(ref mIsRunning, 1);


         string cTempNetworkFile ="C:\\_tmpNetworkLog.bin";
         if (File.Exists(cTempNetworkFile))
            File.Delete(cTempNetworkFile);

         initNetworkReaderThread(cTempNetworkFile, pStream, pClient);

         FileStream mLoggedFileStreamReader = null;
         try
         {
            mLoggedFileStreamReader = new FileStream(cTempNetworkFile, FileMode.OpenOrCreate, FileAccess.Read, FileShare.ReadWrite);
         }catch (Exception ex)
         {
            GlobalErrors.addError("Error opening temp network stream file : \n" + ex.InnerException.ToString());
            Interlocked.Exchange(ref mIsRunning, 0);
         }

         BinaryReader br = new BinaryReader(mLoggedFileStreamReader);


         byte[] header = new byte[4];

         mNumBytesRead = 0;
         while (mIsRunning > 0)
         {
            try
            {
               if (mIsPaused == 1 || mNumBytesRead  >= mNumBytesWritten)
                  continue;

               if (!fillBufferFromStream(br, header, 4, pClient))
               {
                  GlobalErrors.addError("Error reading buffer from streams");
                  break;
               }

               byte packetStart = header[0];
               byte packetType = header[1];
               int packetSize = (int)Xbox_EndianSwap.endSwapI16(BitConverter.ToUInt16(header, 2));


               if (packetStart != HaloWarsMem.cALPacketPrefix)
               {
                  GlobalErrors.addError("Invalid Packet recieved in network stream. Disconnecting..");
                  break;
               }

               //modify our packet size since we've already read in 4 bytes..
               packetSize -= 4; // -= sizeof the data we've already read in..
               if (packetSize < 0) packetSize = 0;






               //read in the rest of the packet data
               byte[] packetData = null;
               if (packetSize > 0)
               {
                  //read the packet
                  packetData = new byte[packetSize];
                  if (!fillBufferFromStream(br, packetData, packetSize, pClient))
                  {
                     GlobalErrors.addError("Error reading buffer from streams");
                     break;
                  }

               }


               recieveProcessPacket(header, packetType, packetSize,ref packetData);

               int incAmt = 4 + packetSize;
              
               
               mNumBytesRead += (ulong)incAmt;
               


            }
            catch (Exception e)
            {
               GlobalErrors.addError(e.InnerException.ToString());
               break;
            }
         }
         stopProcessing();

         mLoggedFileStreamReader.Close();
         br.Close();

         forceOnDisconnectEvent();

      }

   }
}
