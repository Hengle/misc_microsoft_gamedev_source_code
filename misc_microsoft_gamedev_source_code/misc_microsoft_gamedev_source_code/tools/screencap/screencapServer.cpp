// File: screencapServer.cpp
#include "screencapServer.h"

#include "xcore.h"
#include "threading\win32Event.h"
#include "containers\nameValueMap.h"
#include "containers\staticArray.h"

#include "..\shared\tcpClient.h"

#include "screenCapProtocol.h"
#include "d3dScreenCap.h"

const uint cMaxScreenWidth = 2048;
const uint cMaxScreenHeight = 2048;

static bool                      gInitialized;
static BDynamicArray<BString>    gHostAddresses;

static bool                      gCapturing;

static BWin32Event               gTermServer;
static HANDLE                    gServerThreadHandle = INVALID_HANDLE_VALUE;

static BString                   gServerIP;
static float                     gServerUpdateRate;
static BTcpClient*               gpTcpClient;
static BTcpListener*             gpTcpListener;
static BD3DScreenCapture*        gpD3DScreenCapture;

unsigned __stdcall serverThreadFunc(void* pArguments);

bool BScreenCapServer::init(void)
{
   if (gInitialized)
      return true;
      
   if (!BTcpClient::winsockSetup())
      return false;
      
   if (BTcpClient::getHostAddresses(gHostAddresses) != cTCE_Success)
      return false;
   
   gInitialized = true;
 
   return true;
}

bool BScreenCapServer::deinit(void)
{
   if (!gInitialized)
      return true;
      
   endCapture();      
      
   BTcpClient::winsockCleanup();
   
   gInitialized = false;
   
   return true;
}

bool BScreenCapServer::getInitialized(void) 
{
   return gInitialized;
}

unsigned int BScreenCapServer::getNumServerAddresses(void)
{
   return gHostAddresses.getSize();
}

const char* BScreenCapServer::getServerAddress(unsigned int index)
{
   return gHostAddresses[index];
}

bool BScreenCapServer::beginCapture(const char* pServerAddress, float updateRate)
{
   BDEBUG_ASSERT(pServerAddress);
   
   if (!gInitialized)
      return false;
      
   gServerIP.set(pServerAddress);
   gServerUpdateRate = updateRate;
   
   uintptr_t result = _beginthreadex(NULL, 0, serverThreadFunc, NULL, 0, NULL);
   BVERIFY(result != 0);

   gServerThreadHandle = (HANDLE)result;
   
   return true;   
}

bool BScreenCapServer::endCapture(void)
{
   if (!gInitialized)
      return false;

   if (gServerThreadHandle != INVALID_HANDLE_VALUE)
   {
      gTermServer.set();
      
      WaitForSingleObject(gServerThreadHandle, INFINITE);
      
      gTermServer.reset();
      
      CloseHandle(gServerThreadHandle);
      gServerThreadHandle = INVALID_HANDLE_VALUE;
   }
   
   return true;
}

bool BScreenCapServer::getCapturing(void)
{
   return gServerThreadHandle != INVALID_HANDLE_VALUE;
}

static void logMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);
   
   OutputDebugStringA(buf);
}

static void logErrorMessage(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   OutputDebugStringA(buf);
}

static void errorMessageBox(const char* pMsg, ...)
{
   const uint cMaxTextBufSize = 4096;

   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);
   
   MessageBoxA(NULL, buf, "ScreenCap Error", MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
}

static bool sendPacket(ScreenCapProtocol::ePacketTypes packetType, BYTE flags, void* pData, uint dataLen, bool wait)
{
   BStaticArray<BYTE, 4096> buf;

   uint crc32 = 0;
   if ((pData) && (dataLen)) 
      crc32 = calcCRC32(pData, dataLen);

   ScreenCapProtocol::BPacketHeader packetHeader(dataLen, crc32, (BYTE)packetType, flags, true);
         
   if ((pData) && (dataLen))
   {
      eTcpClientError status = gpTcpClient->send(&packetHeader, sizeof(packetHeader), false);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;
      }

      status = gpTcpClient->send(pData, dataLen, true);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;      
      }
   }  
   else
   {
      eTcpClientError status = gpTcpClient->send(&packetHeader, sizeof(packetHeader), true);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: send() failed\n");
         return false;
      }
   }

   if (wait)
   {
      eTcpClientError status = gpTcpClient->sendFlush(INFINITE);
      if (status != cTCE_Success)
      {
         logErrorMessage("sendPacket: sendFlush() failed\n");
         return false;
      }
   }         

   return true;
}

static bool sendPacket(ScreenCapProtocol::ePacketTypes packetType, const BNameValueMap& nameValueMap, bool wait)
{
   BStaticArray<BYTE, 4096> buf;

   const uint dataSize = nameValueMap.getSerializeSize();
   buf.resize(dataSize);

   uchar* pEnd = (uchar*)nameValueMap.serialize(buf.getPtr(), true);
   const uint serializeSize = pEnd - buf.getPtr();
   BVERIFY(dataSize == serializeSize);
   
   return sendPacket(packetType, ScreenCapProtocol::BPacketHeader::cFlagDataIsNameValueMap, buf.getPtr(), buf.getSize(), wait);
}

static bool receivePacket(bool& receivedPacked, ScreenCapProtocol::BPacketHeader& header, BByteArray& data, bool waitForData)
{
   receivedPacked = false;

   uint bytesReceived;
   if (gpTcpClient->receive(&header, sizeof(header), bytesReceived, BTcpClient::cReceivePeek) < 0)
   {
      logErrorMessage("receivePacket: receive() failed\n");
      return false;
   }

   if (bytesReceived < sizeof(header))
   {
      if (gpTcpClient->getConnectionStatus() == BTcpClient::cClosedGracefully)
      {
         logErrorMessage("receivePacket: Connection closed prematurely\n");
         return false;
      }

      return true;
   }      

   if (header.mSig != ScreenCapProtocol::BPacketHeader::cSig)
   {
      logErrorMessage("receivePacket: Bad packet header sig\n");
      return false;
   }    

   if (header.mHeaderCRC16 != header.computeHeaderCRC16())   
   {
      logErrorMessage("receivePacket: Bad packet header CRC\n");
      return false;
   }

   if (waitForData)
   {
      uint totalDataSize = header.mDataSize;
      if (totalDataSize > 128U * 1024U * 1024U)
      {
         logErrorMessage("receivePacket: Packet data is too big: %i\n", totalDataSize);
         return false;
      }

      uint bytesAvailable;
      if (gpTcpClient->getReceiveBytesAvail(bytesAvailable, false) < 0)
      {
         logErrorMessage("receivePacket: getReceiveBytesAvail() failed\n");
         return false;
      }

      if (bytesAvailable < (totalDataSize + sizeof(header)))
      {
         if (gpTcpClient->getConnectionStatus() == BTcpClient::cClosedGracefully)
         {
            logErrorMessage("receivePacket: Connection closed prematurely\n");
            return false;
         }
         else
            return true;
      }

      if (gpTcpClient->receive(&header, sizeof(header), bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }

      if (bytesReceived != sizeof(header))
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }

      data.resize(totalDataSize);   
      if (gpTcpClient->receive(data.getPtr(), totalDataSize, bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }

      if (bytesReceived != totalDataSize)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }

      uint crc32 = cInitCRC32;
      if (totalDataSize)      
         crc32 = calcCRC32(data.getPtr(), totalDataSize);

      if (crc32 != header.mDataCRC32)
      {
         logErrorMessage("receivePacket: Bad data CRC\n");
         return false;
      }
   }
   else
   {
      if (gpTcpClient->receive(&header, sizeof(header), bytesReceived, 0) < 0)
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;
      }

      if (bytesReceived != sizeof(header))
      {
         logErrorMessage("receivePacket: receive() failed\n");
         return false;  
      }
   }

   receivedPacked = true;

   return true;
}

enum eProcessConnectionStatus
{
   cPCSSucceeded,
   cPCSFailed,
   cPCSTerminate,
   cPCSNewConnection
};

static eProcessConnectionStatus processConnection(void)
{
   BNameValueMap nameValueMap;
   nameValueMap.add("ScreenCapVersion", (uint)ScreenCapProtocol::cScreenCapVersion);
   nameValueMap.add("ProtocolVersion", (uint)ScreenCapProtocol::cProtocolVersion);
   nameValueMap.add("Width", gpD3DScreenCapture->getWidth());
   nameValueMap.add("Height", gpD3DScreenCapture->getHeight());
   
   if (!sendPacket(ScreenCapProtocol::cPTHello, nameValueMap, true))
   {
      errorMessageBox("sendPacket() failed!");
      return cPCSFailed;
   }
   
   BByteArray frameBuf;
         
   for ( ; ; )
   {
      if (FAILED(gpD3DScreenCapture->capture()))
      {
         errorMessageBox("Failed capturing desktop!");
         return cPCSFailed;
      }

      const uint width = gpD3DScreenCapture->getWidth();
      BDEBUG_ASSERT((width & 3) == 0);
      const uint widthDiv4 = width >> 2;
      const uint height = gpD3DScreenCapture->getHeight();
      frameBuf.resize(gpD3DScreenCapture->getWidth() * 3 * gpD3DScreenCapture->getHeight());
      
      const DWORD* pBits = NULL;
      uint pitch = 0;
      
      if (!gpD3DScreenCapture->getBits(pBits, pitch))
      {
         errorMessageBox("Failed capturing desktop!");
         return cPCSFailed;
      }
                        
      for (uint y = 0; y < height; y++)
      {
         const DWORD* pSrc = (const DWORD*)((const BYTE*)pBits + pitch * y);
         BYTE* pDst = frameBuf.getPtr() + (y * width * 3);
         
         // A8R8G8B8
         
         for (uint x = widthDiv4; x > 0; x--)
         {
            const DWORD x0 = pSrc[0];
            const DWORD x1 = pSrc[1];
            const DWORD x2 = pSrc[2];
            const DWORD x3 = pSrc[3];
            
            pDst[0] = (BYTE)x0;
            pDst[1] = (BYTE)(x0 >> 8);
            pDst[2] = (BYTE)(x0 >> 16);
            
            pDst[3] = (BYTE)x1;
            pDst[4] = (BYTE)(x1 >> 8);
            pDst[5] = (BYTE)(x1 >> 16);
            
            pDst[6] = (BYTE)x2;
            pDst[7] = (BYTE)(x2 >> 8);
            pDst[8] = (BYTE)(x2 >> 16);
            
            pDst[9] = (BYTE)x3;
            pDst[10] = (BYTE)(x3 >> 8);
            pDst[11] = (BYTE)(x3 >> 16);
            
            pSrc += 4;
            pDst += 12;
         }                  
      }
      
      nameValueMap.clear();
      nameValueMap.add("Width", width);      
      nameValueMap.add("Height", height);  
      nameValueMap.add("DataSize", width * height * 3);   
      
      if (!sendPacket(ScreenCapProtocol::cPTFrame, nameValueMap, false))
      {
         errorMessageBox("sendPacket() failed!");
         return cPCSFailed;  
      }
      
      eTcpClientError result = gpTcpClient->send(frameBuf.getPtr(), frameBuf.getSizeInBytes(), true);
      if (result != cTCE_Success)
      {
         errorMessageBox("sendPacket() failed!");
         return cPCSFailed;    
      }
      
      HANDLE waitHandles[4] = 
      { 
         gTermServer.getHandle(), 
         gpTcpListener->getEvent().getHandle(),
         gpTcpClient->getSendEvent().getHandle(),
         gpTcpClient->getReceiveEvent().getHandle()
      };
      
      const uint waitTime = 10;
      const DWORD startTime = GetTickCount();
      
      for ( ; ; )
      {
         DWORD waitResult = WaitForMultipleObjects(4, waitHandles, FALSE, waitTime);
         
         switch (waitResult)
         {
            case WAIT_TIMEOUT:
               break;
            case WAIT_OBJECT_0:
               return cPCSTerminate;
            case WAIT_OBJECT_0 + 1:
               return cPCSNewConnection;
            case WAIT_OBJECT_0 + 2:
            case WAIT_OBJECT_0 + 3:
            {
               if (cTCE_Success != gpTcpClient->sendFlush(0))
               {
                  errorMessageBox("sendFlush() failed!");
                  return cPCSFailed;
               }
               break;
            }
            default:
            {
               errorMessageBox("WaitForMultipleObjects() failed!");
               return cPCSFailed;    
            }
         }
         
         if ((GetTickCount() - startTime) > waitTime)
            break;
      }         
   }
   
   return cPCSSucceeded;
}

static void disconnectAndExit(bool failed)
{
   if ((gpTcpClient) && (gpTcpClient->getConnectionStatus() == BTcpClient::cOpen))
   {
      sendPacket(ScreenCapProtocol::cPTBye, 0, NULL, 0, true);
   }

   if (gpTcpClient)      
   {
      gpTcpClient->close();
      delete gpTcpClient;
      gpTcpClient = NULL;
   }

   if (gpTcpListener)
   {
      gpTcpListener->close();
      delete gpTcpListener;
      gpTcpListener = NULL;
   }
   
   if (gpD3DScreenCapture)
   {
      delete gpD3DScreenCapture;  
      gpD3DScreenCapture = NULL;
   }

   _endthreadex(failed ? FALSE : TRUE);
}

unsigned __stdcall serverThreadFunc(void* pArguments)
{
   BDEBUG_ASSERT(!gpD3DScreenCapture);
   gpD3DScreenCapture = new BD3DScreenCapture;
   if (FAILED(gpD3DScreenCapture->init()))
   {
      errorMessageBox("Unable to create D3D9 device!");
      disconnectAndExit(true);
   }
   
   if ((gpD3DScreenCapture->getWidth() > cMaxScreenWidth) || (gpD3DScreenCapture->getHeight() > cMaxScreenHeight))
   {  
      errorMessageBox("Desktop resolution is too big!");
      disconnectAndExit(true);
   }
   
   BDEBUG_ASSERT(!gpTcpListener);
   gpTcpListener = new BTcpListener();
   
   eTcpClientError result = gpTcpListener->listen(gServerIP.getPtr(), ScreenCapProtocol::cPort);
   if (result != cTCE_Success)
   {
      errorMessageBox("Unable to listen for connections at %s:%u!", gServerIP.getPtr(), ScreenCapProtocol::cPort);
      disconnectAndExit(true);
   }
   
   for ( ; ; )
   {
      HANDLE waitHandles[2] = { gTermServer.getHandle(), gpTcpListener->getEvent().getHandle() };
      
      DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
      
      if (waitResult == WAIT_OBJECT_0)
         break;
      else if (waitResult == (WAIT_OBJECT_0 + 1))
      {
         for ( ; ; )
         {
            SOCKET socket;
                     
            result = gpTcpListener->accept(&socket, NULL, 20000);
            if (result != cTCE_Success)
            {
               errorMessageBox("Failed accepting connection!");
               break;
            }
                                    
            BDEBUG_ASSERT(!gpTcpClient);
            gpTcpClient = new BTcpClient;

            const uint cBufSize = 16U * 1024U * 1024U;

            eTcpClientError result = gpTcpClient->attach(socket, cBufSize, true);
            if (result != cTCE_Success)
            {
               errorMessageBox("Failed attaching to connection!");
               
               delete gpTcpClient;
               gpTcpClient = NULL;
               
               break;
            }
            
            gpTcpClient->setReceiveTimeout(30000);
            gpTcpClient->setSendTimeout(30000);
            
            eProcessConnectionStatus status = processConnection();
            
            if (gpTcpClient->getConnectionStatus() == BTcpClient::cOpen)
            {
               sendPacket(ScreenCapProtocol::cPTBye, 0, NULL, 0, false);
            }                  
            
            gpTcpClient->close(10000, 0);
            
            delete gpTcpClient;
            gpTcpClient = NULL;
            
            if (status == cPCSTerminate)
               disconnectAndExit(false);
            else if (status != cPCSNewConnection)
               break;
         }            
      }
      else
      {
         errorMessageBox("WaitForMultipleObjects() failed");
         disconnectAndExit(true);
      }
   }
         
   disconnectAndExit(false);
   
   return FALSE;
}
