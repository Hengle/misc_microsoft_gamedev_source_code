//==============================================================================
// RemoteToolsHost.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//
#include "xsystem.h"

//#include "common.h"

#if defined(ENABLE_TIMELINE_PROFILER)
//#include "xcore.h"

#include "RemoteToolsHost.h"
#include "SerialBuffer.h"
#include "SocksReliableSocket.h"
#include "SocksHelper.h"
//#include "kb.h"
//#include "game.h"
//#include "database.h"
//#include "adefines.h"

//#include "world.h"
//#include "team.h"
//
//#include "player.h"
//#include "ai.h"
//#include "xsruntime.h"
//
//#include "xssource.h"

//#include "console.h"
#include "commheaders.h"
#include "commlog.h"


#include "RemoteProfiler.h" ///shit!!

BRemoteToolsHost gRemoteToolsHost;
BRemoteProfiler gRemoteProfiler;


extern void createProcess(const char* fileName, const char* arguments, bool waitProcessToExit);
//==============================================================================
//==============================================================================
BNetDebugGameLister::BNetDebugGameLister(const char* gameListserver, long port, BRemoteToolsHost* debugHook) :
   mpSocket( NULL ),
   mbConnected( false ),
	mpDebugHook(debugHook)
{
	// resolve the host name
	//
	u_long nRemoteAddr = inet_addr( gameListserver );
	if ( nRemoteAddr == INADDR_NONE ) 
	{
		// pcHost isn't a dotted IP, so resolve it through DNS
		hostent* pHE = gethostbyname( gameListserver );
		if ( pHE == 0 ) 
			return;

		nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
	}

	//-- setup the advertise address.
	memset(&mAdvertiseAddress, 0, sizeof(mAdvertiseAddress));
	mAdvertiseAddress.sin_family = AF_INET;
	mAdvertiseAddress.sin_addr.s_addr = nRemoteAddr;
	mAdvertiseAddress.sin_port = htons( (u_short)port );

	advertiseGame(true);
}

//==============================================================================
//==============================================================================
BNetDebugGameLister::~BNetDebugGameLister()  
{ 
	advertiseGame(false);

	if(mpSocket)
		mpSocket->setObserver( 0 ); 
	
	if(mpSocket)
		mpSocket->dispose();

	mpDebugHook = NULL;
}

//==============================================================================
//==============================================================================
HRESULT BNetDebugGameLister::send (const void* Buffer, const long Length)
{
   if ( !mbConnected || mpSocket == NULL)
      return 0;

   BSendBuffer * SendBuffer=0;
   HRESULT Result;

   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
   if (FAILED (Result))
   {
      nlog(cTransportNL,"BRemoteToolsHost::send: failed to allocate buffer");
      return Result;
   }

   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
   SendBuffer -> Length = Length;

   Result = mpSocket -> send (SendBuffer);

   return Result;
}

//==============================================================================
//==============================================================================
HRESULT BNetDebugGameLister::sendTo ( IN CONST VOID * Buffer, IN ULONG Length, IN CONST SOCKADDR_IN * RemoteAddress)
{
   if (!mbConnected || mpSocket == NULL)
   {
      nlog(cTransportNL, "BRemoteToolsHost::sendTo: can't send, not connected");
      return E_FAIL;
   }


   BSendBuffer * SendBuffer;
   HRESULT Result;

   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
   if (FAILED (Result))
   {
      nlog(cTransportNL, "BRemoteToolsHost::send: failed to allocate buffer");
      return Result;
   }

   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
   SendBuffer -> Length = Length;

   Result = mpSocket -> sendTo (SendBuffer, RemoteAddress);

   return Result;
}

//==============================================================================
//==============================================================================
HRESULT BNetDebugGameLister::sendPacket(BPacket &packet)
{
   if ( !mbConnected || mpSocket == NULL)
      return 0;  
   
   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
   packet.serialize(sb);

   return send(sb.getBuffer(), sb.getBufferSize());
}

//==============================================================================
//==============================================================================
HRESULT BNetDebugGameLister::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
{
   if ( !mbConnected || mpSocket == NULL)
      return 0;  
   
   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
   packet.serialize(sb);

   return sendTo(sb.getBuffer(), sb.getBufferSize(), RemoteAddress);
}

//==============================================================================
//==============================================================================
void BNetDebugGameLister::recvd (IN BSocket * Socket, IN const void * Buffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN * RemoteAddress)
{ 
   UNREFERENCED_PARAMETER (Socket);
	UNREFERENCED_PARAMETER (RemoteAddress);

   long type = BTypedPacket::getType(Buffer);
	BSerialBuffer sb(const_cast<void *>(Buffer), Length);
	switch(type)
	{
		case BRemoteToolsHost::cDebuggerAttach:
		{
			BDebuggerAttachPacket packet;
			packet.deserialize(sb);
			//-- attach a debugger if we aren't already attached.
			if(mpDebugHook && packet.mDebuggerAddress.length() > 0 && mpDebugHook->isConnected() == false)
			{
				mpDebugHook->attachDebugger(packet.mDebuggerAddress, packet.mDebuggerPort);
			}
		}
	}
}

//==============================================================================
//==============================================================================
void BNetDebugGameLister::disconnected (IN BSocket * Socket, IN DWORD Status)
{
	UNREFERENCED_PARAMETER (Socket);
	UNREFERENCED_PARAMETER (Status);

	mbConnected = false;
	if(mpSocket)
	{
		mpSocket->dispose();
		mpSocket=NULL;
	}
}

//==============================================================================
//==============================================================================
bool BNetDebugGameLister::advertiseGame(bool advert)
{
	if(mbConnected == false)
	{
		mpSocket = new BSocksReliableSocket(this);
		if(mpSocket == NULL)
			return(false);

		mpSocket->setObserver( this );
		if(mpSocket->connect(&mAdvertiseAddress, NULL) != S_OK)
		{
			mpSocket->dispose();
			mpSocket = NULL;
			mbConnected = false;
		}
		else
		{
			mbConnected = true;
		}
	}

	if(mbConnected == false)
		return(false);

	BAdvertisePacket out;
	out.mAdvertise = advert;
	out.mGameName += BSimString(BSocksHelper::addressToString(BSocksHelper::getLocalIP()));


	if(sendPacket(out) == S_OK)
		return(true);

	return(false);
}
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//

//BSocksReliableListener
//
//BNetServiceHost::BNetServiceHost(long port) :
//mpSocket( NULL ),
//mbConnected( false ),
//{
//   // resolve the host name
//   //
//   //u_long nRemoteAddr = inet_addr( gameListserver );
//   //if ( nRemoteAddr == INADDR_NONE ) 
//   //{
//   //   // pcHost isn't a dotted IP, so resolve it through DNS
//   //   hostent* pHE = gethostbyname( gameListserver );
//   //   if ( pHE == 0 ) 
//   //      return;
//
//   //   nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
//   //}
//
//   ////-- setup the advertise address.
//   //memset(&mAdvertiseAddress, 0, sizeof(mAdvertiseAddress));
//   //mAdvertiseAddress.sin_family = AF_INET;
//   //mAdvertiseAddress.sin_addr.s_addr = nRemoteAddr;
//   //mAdvertiseAddress.sin_port = htons( (u_short)port );
//
//   //advertiseGame(true);
//}
//
////==============================================================================
////==============================================================================
//BNetServiceHost::~BNetServiceHost()  
//{ 
//   advertiseGame(false);
//
//   if(mpSocket)
//      mpSocket->setObserver( 0 ); 
//
//   if(mpSocket)
//      mpSocket->dispose();
//
//   mpDebugHook = NULL;
//}
//
////==============================================================================
////==============================================================================
//HRESULT BNetServiceHost::send (const void* Buffer, const long Length)
//{
//   if ( !mbConnected || mpSocket == NULL)
//      return 0;
//
//   BSendBuffer * SendBuffer=0;
//   HRESULT Result;
//
//   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
//   if (FAILED (Result))
//   {
//      nlog(cTransportNL,"BRemoteToolsHost::send: failed to allocate buffer");
//      return Result;
//   }
//
//   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
//   SendBuffer -> Length = Length;
//
//   Result = mpSocket -> send (SendBuffer);
//
//   return Result;
//}
//
////==============================================================================
////==============================================================================
//HRESULT BNetServiceHost::sendTo ( IN CONST VOID * Buffer, IN ULONG Length, IN CONST SOCKADDR_IN * RemoteAddress)
//{
//   if (!mbConnected || mpSocket == NULL)
//   {
//      nlog(cTransportNL, "BRemoteToolsHost::sendTo: can't send, not connected");
//      return E_FAIL;
//   }
//
//
//   BSendBuffer * SendBuffer;
//   HRESULT Result;
//
//   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
//   if (FAILED (Result))
//   {
//      nlog(cTransportNL, "BRemoteToolsHost::send: failed to allocate buffer");
//      return Result;
//   }
//
//   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
//   SendBuffer -> Length = Length;
//
//   Result = mpSocket -> sendTo (SendBuffer, RemoteAddress);
//
//   return Result;
//}
//
////==============================================================================
////==============================================================================
//HRESULT BNetServiceHost::sendPacket(BPacket &packet)
//{
//   if ( !mbConnected || mpSocket == NULL)
//      return 0;  
//
//   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
//   packet.serialize(sb);
//
//   return send(sb.getBuffer(), sb.getBufferSize());
//}
//
////==============================================================================
////==============================================================================
//HRESULT BNetServiceHost::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
//{
//   if ( !mbConnected || mpSocket == NULL)
//      return 0;  
//
//   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
//   packet.serialize(sb);
//
//   return sendTo(sb.getBuffer(), sb.getBufferSize(), RemoteAddress);
//}
//
////==============================================================================
////==============================================================================
//void BNetServiceHost::recvd (IN BSocket * Socket, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN * RemoteAddress)
//{ 
//   UNREFERENCED_PARAMETER (Socket);
//   UNREFERENCED_PARAMETER (RemoteAddress);
//
//   long type = BTypedPacket::getType(Buffer);
//   BSerialBuffer sb(Buffer, Length);
//   switch(type)
//   {
//   case BRemoteToolsHost::cDebuggerAttach:
//      {
//         BDebuggerAttachPacket packet;
//         packet.deserialize(sb);
//         //-- attach a debugger if we aren't already attached.
//         if(mpDebugHook && packet.mDebuggerAddress.length() > 0 && mpDebugHook->isConnected() == false)
//         {
//            mpDebugHook->attachDebugger(packet.mDebuggerAddress, packet.mDebuggerPort);
//         }
//      }
//   }
//}
//
////==============================================================================
////==============================================================================
//void BNetServiceHost::disconnected (IN BSocket * Socket, IN DWORD Status)
//{
//   UNREFERENCED_PARAMETER (Socket);
//   UNREFERENCED_PARAMETER (Status);
//
//   mbConnected = false;
//   if(mpSocket)
//   {
//      mpSocket->dispose();
//      mpSocket=NULL;
//   }
//}
//
////==============================================================================
////==============================================================================
//bool BNetServiceHost::advertiseGame(bool advert)
//{
//   if(mbConnected == false)
//   {
//      mpSocket = new BSocksReliableSocket(this);
//      if(mpSocket == NULL)
//         return(false);
//
//      mpSocket->setObserver( this );
//      if(mpSocket->connect(&mAdvertiseAddress, NULL) != S_OK)
//      {
//         mpSocket->dispose();
//         mpSocket = NULL;
//         mbConnected = false;
//      }
//      else
//      {
//         mbConnected = true;
//      }
//   }
//
//   if(mbConnected == false)
//      return(false);
//
//   BAdvertisePacket out;
//   out.mAdvertise = advert;
//   out.mGameName += BSimString(BSocksHelper::addressToString(BSocksHelper::getLocalIP()));
//
//
//   if(sendPacket(out) == S_OK)
//      return(true);
//
//   return(false);
//}
//


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//
BRemoteToolsHost::BRemoteToolsHost() :
   mpSocket( NULL ),
	mbStarted(false),
	mbConnected(false),
	mpGameAdvertiser(NULL),
	mAdvertiseTimeOut(0)
{
   //startup("172.16.1.35", 50000);
   //startup("169.254.13.116", 50000);

}

BRemoteToolsHost::~BRemoteToolsHost()
{ 
}

//==============================================================================
//==============================================================================
void BRemoteToolsHost::startup(const char* gameListserver, long port)
{
   if(!networkStartup())
   {
      return;
   }

   //afo/// game->initCommLogging();

	mpGameAdvertiser = new BNetDebugGameLister(gameListserver, port, this);
	mbStarted=true;
	//if(gConfig.isDefined(cConfigRemoteToolsHostAutoStart) == true)
	//{
	//	BSimString path;
	//	gConfig.get(cConfigRemoteToolsHostAutoStart, path);
	//	createProcess(path.asANSI(), NULL, false);
	//}
}

//==============================================================================
//==============================================================================
void BRemoteToolsHost::shutdown(void)
{
	if(mpGameAdvertiser)
		delete mpGameAdvertiser;

	if(mpSocket)
		mpSocket->setObserver( 0 ); 

	if(mpSocket)
		mpSocket->dispose(); 

	mbConnected=false;
   if(mbStarted)
   {
      networkCleanup();
   	mbStarted=false;
   }

}

//==============================================================================
//==============================================================================
void BRemoteToolsHost::disconnect(bool detachDebugger)
{
	if(detachDebugger == true)
	{
		BTypedPacket packet(BRemoteToolsHost::cDebuggerDetach);
		sendPacket(packet);
	}

	if(mpSocket)
	{
		mpSocket->dispose(); 
		mpSocket = NULL;
	}
	mbConnected=false;


}

//==============================================================================
//==============================================================================
void BRemoteToolsHost::update(long currentTime, bool force)
{
	if(mpGameAdvertiser == NULL)
		return;

	if(((DWORD)currentTime>mAdvertiseTimeOut) || (force==true))
	{
		if(mbConnected == false)
		{
			mpGameAdvertiser->advertiseGame(true);
		}
		else
		{
			//-- send heartbeat?			
		}

		mAdvertiseTimeOut=currentTime+60000;
	}

}
BCriticalSection gSendCrit;

//==============================================================================
//==============================================================================
HRESULT BRemoteToolsHost::send(const void* Buffer, const long Length)
{
   if ( !mbConnected || mpSocket == NULL)
      return 0;

   gSendCrit.lock();

   BSendBuffer * SendBuffer=0;
   HRESULT Result;

   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
   if (FAILED (Result))
   {
      nlog(cTransportNL,"BRemoteToolsHost::send: failed to allocate buffer");
      gSendCrit.unlock();
      return Result;
   }

   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
   SendBuffer -> Length = Length;

   Result = mpSocket -> send (SendBuffer);

   gSendCrit.unlock();
   return Result;
}

//==============================================================================
//==============================================================================
HRESULT BRemoteToolsHost::sendTo (IN CONST VOID * Buffer, IN ULONG Length, IN CONST SOCKADDR_IN * RemoteAddress)
{
   if (!mbConnected || mpSocket == NULL)
   {
      nlog(cTransportNL, "BRemoteToolsHost::sendTo: can't send, not connected");
      return E_FAIL;
   }


   BSendBuffer * SendBuffer;
   HRESULT Result;

   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
   if (FAILED (Result))
   {
      nlog(cTransportNL, "BRemoteToolsHost::send: failed to allocate buffer");
      return Result;
   }

   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
   SendBuffer -> Length = Length;

   Result = mpSocket -> sendTo (SendBuffer, RemoteAddress);

   return Result;
}

BCriticalSection gSendPacketCrit;

//==============================================================================
//==============================================================================
HRESULT BRemoteToolsHost::sendPacket(BPacket &packet)
{
   if ( !mbConnected || mpSocket == NULL)
      return 0;  

   gSendPacketCrit.lock();

	BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
	packet.serialize(sb);

   HRESULT res = send(sb.getBuffer(), sb.getBufferSize());

   gSendPacketCrit.unlock();

	return res;
}
//==============================================================================
//==============================================================================
HRESULT BRemoteToolsHost::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
{
	if ( !mbConnected || mpSocket == NULL)
		return 0;  

	BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
	packet.serialize(sb);

	return sendTo(sb.getBuffer(), sb.getBufferSize(), RemoteAddress);
}

//==============================================================================
//==============================================================================
void BRemoteToolsHost::recvd(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN * RemoteAddress)
{ 
	BSerialBuffer aasdf;
	gRemoteProfiler.read(aasdf,-1);
		
	UNREFERENCED_PARAMETER (Socket);
	UNREFERENCED_PARAMETER (RemoteAddress);

	long type = BTypedPacket::getType(Buffer);
	BSerialBuffer sb(const_cast<void *>(Buffer), Length);
	//switch(type)
	//{
	//	default:
	//	{
			long numObs = mTypedObservers.getNumber();
			for(long i=0; i<numObs; i++)
			{
				if(mTypedObservers[i] == NULL)
					continue;

				//-- If an observer handled the packet, break out of the loop.
				if(mTypedObservers[i]->read(sb, type) == true)
				{
					break;
				}
			}
		//	break;
		//}
		//case BRemoteToolsHost::cDebuggerDetach:
		//{
		//	disconnect(true);
		//	break;
		//}
		//
		//case BRemoteToolsHost::cPausePacket:
		//{
		//	BPauseGamePacket packet;
		//	packet.deserialize(sb);
		//	if(game->getWorld() == NULL)
		//		break;

		//	game->getWorld()->setPaused(packet.mbPause);
		//	break;
		//}
		//case BRemoteToolsHost::cKBTimedStat:
		//{
		//	if(game->getWorld()== NULL)
		//		break;

		//	long numPlayers = game->getWorld()->getNumberPlayers();
		//	BKBTimedStat newTS;
		//	for(long i=1; i<numPlayers; i++)
		//	{
		//		if(game->getWorld()->getPlayer(i) == NULL || game->getWorld()->getPlayer(i)->getKB() == NULL)
		//			continue;

		//		newTS.update(game->getWorld()->getPlayer(i)->getKB());
		//		BPacketKBTimedStat out(game->getWorld()->getPlayer(i)->getKB(), &newTS);
		//		gRemoteToolsHost.sendPacket(out);
		//	}
		//	break;
		//	
		//}


////////////////
		//case BRemoteToolsHost::cConsoleCommand:
		//{
		//	BConsoleCommandPacket packet;
		//	packet.deserialize(sb);

  //       if(gConsole)
  //          gConsole->execute(packet.mCommand.asANSI());
		//	break;
		//}
	
	//}
}

//==============================================================================
//==============================================================================
bool BRemoteToolsHost::attachDebugger(const BSimString& debuggerAddress, long debuggerPort)
{
	if(mbConnected == true)
		return(false);

   //if (debuggerAddress.length() == 0)
   //   return(0);

   //long inaddr = inet_addr( debuggerAddress.getPtr() );
   //if ( inaddr == INADDR_NONE )
   //   return(0);


	mpSocket = new BSocksReliableSocket(this);
   if(mpSocket == NULL)
		return(false);

	mpSocket->setObserver( this );
	
	//////-- new some helper classes.
	////if(mpRemoteAIDebugger == NULL)
	////	mpRemoteAIDebugger = new BRemoteAIDebugger();
	////if(mpRemoteXSDebugger == NULL)
	////	mpRemoteXSDebugger = new BRemoteXSDebugger();

	memset(&mDebuggerAddress, 0, sizeof(mDebuggerAddress));

#ifdef XBOX	
  // BStringTemplate<BStringTemplate<WCHAR>::oppositeStringType,BStringFixedHeapAllocator> ansi2;
  // BStringTemplate<char> ansiString ;
	//mDebuggerAddress.sin_addr.s_addr = inet_addr( debuggerAddress.asANSI(ansiString) );
   //mDebuggerAddress.sin_addr.S_un.S_addr = inaddr;

   mDebuggerAddress.sin_family = AF_INET;
   mDebuggerAddress.sin_addr.s_addr = inet_addr( debuggerAddress.getPtr() );

#else	
	mDebuggerAddress = BSocksHelper::stringToAddress(debuggerAddress.asANSI());
#endif

	mDebuggerAddress.sin_port = htons( (u_short)debuggerPort );
	if(mpSocket->connect(&mDebuggerAddress, NULL) != S_OK)
	{
		mpSocket->dispose();
		mpSocket = NULL;
		mbConnected = false;
	}
	mbConnected = true;

	return true;
}


//==============================================================================
//==============================================================================
long BRemoteToolsHost::addObserver(BRemoteToolsHostObserver* pObserver)
{
	if(pObserver == NULL)
		return(-1);

	return(mTypedObservers.add(pObserver));
}

//==============================================================================
//==============================================================================
bool BRemoteToolsHost::removeObserver(long observerID)
{
	if(observerID<0 || observerID >= mTypedObservers.getNumber())
		return(false);

	mTypedObservers[observerID] = NULL;
	return(true);
}

#else
uint gRemoteToolsHostDummy;
#endif 
