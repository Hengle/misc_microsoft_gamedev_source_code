// File: buildClient.h
#pragma once
#include "..\shared\tcpClient.h"
#include "buildSystemProtocol.h"
#include "containers\nameValueMap.h"
#include "utils\commandLineParser.h"

class BBuildClient
{
public:
   BBuildClient();
   ~BBuildClient();
   
   bool run(BCommandLineParser::BStringArray& args);

private:
   BCommandLineParser::BStringArray mCommandLineArgs;
   
   uint           mNumCPUs;
   BTcpListener   mListener;
   BString        mListenAddress;
   bool           mEnableExecUpdate;
         
   SOCKET         mServerSocket;
   sockaddr       mServerAddress;
   
   HANDLE         mHelperThread;
   
   // Helper thread
   BTcpClient     mClient;
   
   DWORD          mLastKeepAliveSent;
   DWORD          mLastKeepAliveReceived;
      
   BWin32Event    mExitEvent;
   
   uint16         mServerProtocolVersion;
   uint16         mServerSystemVersion;
   
   volatile bool  mReceivedAutoUpdatePacket;
   
   enum { cMaxHelperProcesses  = 8 };
   
   struct BProcessState
   {
      BProcessState() { clear(); }
      
      void clear()
      {
         mID = UINT64_MAX;
         mHandle = INVALID_HANDLE_VALUE;
         mCurDir.empty();
         mExec.empty();
         mArgs.empty();
         mOutputFiles.clear();
      }
      
      uint64                  mID;
      HANDLE                  mHandle;
      BString                 mCurDir;
      BString                 mExec;
      BString                 mArgs;
      BDynamicArray<BString>  mOutputFiles;
   };
   
   BProcessState mProcesses[cMaxHelperProcesses];
   
   void clearProcesses();
   uint getNumActiveProcesses() const;
   bool getFreeProcessIndex(uint& index) const;
   bool findProcessByHandle(HANDLE handle, uint& index) const;
            
   bool sendPacket(BuildSystemProtocol::ePacketTypes packetType, const BNameValueMap& nameValueMap, bool wait);
   bool sendPacket(BuildSystemProtocol::ePacketTypes packetType, void* pData, uint dataLen, bool wait);
   bool receivePacket(bool& receivedPacked, BuildSystemProtocol::BPacketHeader& header, BByteArray& data, bool waitForData);
   
   bool tickConnection();
   
   bool processByePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
   bool processAutoUpdatePacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
   bool processKillAllProcessesPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
   bool processExecuteProcessPacket(const BuildSystemProtocol::BPacketHeader& packetHeader, const BByteArray& packetData);
   bool processPackets(BuildSystemProtocol::ePacketTypes packetType, bool& receivedRequestedPacket, BByteArray* pPacketData, BNameValueMap* pNameValueMap);
   
   bool terminateAllProcesses();
   bool handleProcessCompletion(HANDLE handle);
   bool waitForPacket(BuildSystemProtocol::ePacketTypes packetType, BByteArray* pPacketData, BNameValueMap* pNameValueMap);
   
   void disconnectAndExit(bool failed);
      
   unsigned threadMethod();

   void terminateHelperThread();
      
   bool handleConnection();
   
   void logMessage(const char* pMsg, ...);
   void logErrorMessage(const char* pMsg, ...);   

   bool checkExecutable(bool force);
   bool handleAutoUpdate(void);
   bool parseCommandLineArgs(BCommandLineParser::BStringArray& args);
   bool runInternal(BCommandLineParser::BStringArray& args);
         
   static unsigned __stdcall threadFunc(void* pArguments);
};
