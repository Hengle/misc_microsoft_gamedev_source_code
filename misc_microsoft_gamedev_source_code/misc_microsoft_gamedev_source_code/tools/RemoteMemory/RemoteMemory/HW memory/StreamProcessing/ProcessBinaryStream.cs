using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Threading;

namespace RemoteMemory
{
   partial class AllocLogStream
   {
      //============================================================================
      // processStream
      //============================================================================
      static public void processBinaryStream(BinaryReader pStream, string pdbFilename, uint throttleAmout)
      {
         forceOnConnectEvent("127.0.0.1", pdbFilename);


         byte[] header = new byte[4];

         Interlocked.Exchange(ref mIsRunning, 1);

         mNumBytesWritten = (ulong)pStream.BaseStream.Length;
         mNumBytesRead= 0;

         int numPacketsRecieved = 0;
         while (mIsRunning > 0 && mNumBytesRead < mNumBytesWritten)
         {
            if (mIsPaused == 1)
               continue;

            try
            {
               pStream.Read(header, 0, 4); //read the header..

               byte packetStart = header[0];
               byte packetType = header[1];
               int packetSize = (int)Xbox_EndianSwap.endSwapI16(BitConverter.ToUInt16(header, 2));


               if (packetStart != HaloWarsMem.cALPacketPrefix)
                  continue;   //ERROR HERE??

               //modify our packet size since we've already read in 4 bytes..
               packetSize -= 4; // -= sizeof the data we've already read in..
               if (packetSize < 0) packetSize = 0;

               //read in the rest of the packet data
               byte[] packetData = null;
               if (packetSize > 0)
               {
                  //read the packet
                  packetData = new byte[packetSize];
                  int readSize = pStream.Read(packetData, 0, packetSize);

                  if (readSize != packetSize)
                     break; //ERROR HERE??
               }

               recieveProcessPacket(header, packetType, packetSize,ref packetData);

               numPacketsRecieved++;

               mNumBytesRead += (ulong)(4 + packetSize);

               //this is for reading from Disk. We throttle a bit to let the UI catch up..
               if (throttleAmout != 0)
               {
                  for (int i = 0; i < throttleAmout; i++) ;
                  //Thread.Sleep((int)throttleAmout);
               }
            }
            catch (Exception e)
            {
               GlobalErrors.addError(e.InnerException.ToString());
               break;
            }
         }

         forceOnDisconnectEvent();

      }


   }
}
