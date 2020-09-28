//==============================================================================
// RemoteToolsHost.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

#ifndef RemoteToolsHost_h
#define RemoteToolsHost_h

#if defined(ENABLE_TIMELINE_PROFILER)

//==============================================================================
// Includes
//
#include "Socket.h"
#include "Packet.h"
#include "socksreliablesocket.h"

//==============================================================================
//
//class BKBTimedStat;
//class BKB;
class BRemoteToolsHost;
//class BXSRuntime;
//class BRemoteAIDebugger;
//class BRemoteXSDebugger;
class BNetDebugGameLister;


extern BRemoteToolsHost gRemoteToolsHost;

//==============================================================================
//
class BNetDebugGameLister : public BSocket::BObserver
{
   public:
      
		BNetDebugGameLister(const char* gameListserver, long port, BRemoteToolsHost* debugHook);
		virtual ~BNetDebugGameLister();

		enum { cErrorNoBuffers=1 };

		HRESULT send( const void* pData, const long size );

		HRESULT sendTo (
			IN CONST VOID * Buffer,
			IN ULONG Length,
			IN CONST SOCKADDR_IN * RemoteAddress);

		virtual HRESULT sendPacket(BPacket &packet);    
		virtual HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);    

		BSocket * getSocket (void) { return mpSocket; }

		//-- Helpers
		bool advertiseGame	(bool advert);

      // From BSocket::BObserver
      //
      virtual void recvd (
         IN BSocket * Socket,
         IN const void * Buffer,
         IN DWORD Length,
         IN DWORD voiceLength,
         IN CONST SOCKADDR_IN * RemoteAddress);

	  virtual void disconnected (IN BSocket * Socket, IN DWORD Status);
      

      virtual void connected ( IN BSocket *Socket ) 
      {  
         UNREFERENCED_PARAMETER(Socket);
      }

   private:

		BSocksReliableSocket*	mpSocket;
      bool							mbConnected;
		SOCKADDR_IN					mAdvertiseAddress;
		BRemoteToolsHost*	   mpDebugHook;

      // Hidden to deny access
      //
      BNetDebugGameLister( const BNetDebugGameLister& other );
      BNetDebugGameLister& operator=( const BNetDebugGameLister& rhs );
};



//==============================================================================
//
//class BNetServiceHost : public BSocket::BObserver
//{
//public:
//
//   BNetServiceHost(long port);
//   virtual ~BNetServiceHost();
//
//   enum { cErrorNoBuffers=1 };
//
//   HRESULT send( const void* pData, const long size );
//
//   HRESULT sendTo (
//      IN CONST VOID * Buffer,
//      IN ULONG Length,
//      IN CONST SOCKADDR_IN * RemoteAddress);
//
//   virtual HRESULT sendPacket(BPacket &packet);    
//   virtual HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);    
//
//   BSocket * getSocket (void) { return mpSocket; }
//
//   //-- Helpers
//   bool advertiseGame	(bool advert);
//
//   // From BSocket::BObserver
//   //
//   virtual void recvd (
//      IN BSocket * Socket,
//      IN const void * Buffer,
//      IN DWORD Length,
//      IN CONST SOCKADDR_IN * RemoteAddress);
//
//   virtual void disconnected (IN BSocket * Socket, IN DWORD Status);
//
//
//   virtual void connected ( IN BSocket *Socket ) 
//   {  
//      UNREFERENCED_PARAMETER(Socket);
//   }
//
//private:
//
//   BSocksReliableSocket*	mpSocket;
//   bool							mbConnected;
//   //SOCKADDR_IN					mAdvertiseAddress;
//   //BRemoteToolsHost*	   mpDebugHook;
//
//   // Hidden to deny access
//   //
//   //BNetDebugGameLister( const BNetDebugGameLister& other );
//   //BNetDebugGameLister& operator=( const BNetDebugGameLister& rhs );
//   BNetServiceHost( const BNetServiceHost& other );
//   BNetServiceHost& operator=( const BNetServiceHost& rhs );
//};


// Internal classes
//==============================================================================
//
class BRemoteToolsHostObserver
{
public:            
	BRemoteToolsHostObserver(){}
	virtual ~BRemoteToolsHostObserver(){}

	virtual bool read (IN BSerialBuffer& sb, IN long packetType) { sb;packetType;return(false);}
};

//==============================================================================
//
class BRemoteToolsHost : private BSocket::BObserver
{
   public:
      
		enum 
		{ 
			cAdvertiseGame = 1,
			cDebuggerAttach,
			cDebuggerDetach,
			cConsoleCommand,
			cNumberOfPackets 
		};

		BRemoteToolsHost();
		virtual ~BRemoteToolsHost();

		enum { cErrorNoBuffers=1 };
		
		void					startup(const char* gameListserver, long port);
		void					shutdown(void);
		void					disconnect(bool detachDebugger);
		void					update(long currentTime, bool force=false);
		HRESULT				send( const void* pData, const long size );
		HRESULT				sendTo (IN CONST VOID * Buffer, IN ULONG Length, IN CONST SOCKADDR_IN * RemoteAddress);
		virtual HRESULT	sendPacket(BPacket &packet);    
		virtual HRESULT	sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);    
		BSocket *			getSocket (void) { return mpSocket; }
		bool					isConnected() const {return(mbConnected);}
		bool					isStarted() const {return(mbStarted);}

		//-- Helpers
		bool attachDebugger	(const BSimString& debuggerAddress, long debuggerPort);
		long addObserver		(BRemoteToolsHostObserver* pObserver);
		bool removeObserver  (long observerID);

   private:
			
		// From BSocket::BObserver
      //
      virtual void recvd (IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN * RemoteAddress);
      virtual void disconnected (IN BSocket * Socket, IN DWORD Status)
      {
         UNREFERENCED_PARAMETER (Socket);
         UNREFERENCED_PARAMETER (Status);

			disconnect(false);
      }

      virtual void connected ( IN BSocket *Socket ) 
      {  
         UNREFERENCED_PARAMETER(Socket);
      }

      BSocksReliableSocket*	mpSocket;
		bool							mbStarted;
      bool							mbConnected;
		SOCKADDR_IN					mDebuggerAddress;
		BNetDebugGameLister*		mpGameAdvertiser;
		DWORD							mAdvertiseTimeOut;

		BDynamicArray<BRemoteToolsHostObserver*> mTypedObservers;

      // Hidden to deny access
      //
      BRemoteToolsHost( const BRemoteToolsHost& other );
      BRemoteToolsHost& operator=( const BRemoteToolsHost& rhs );
};

//==============================================================================
//
class BAdvertisePacket : public BTypedPacket
{
   public:
		BAdvertisePacket(long packetType=BRemoteToolsHost::cAdvertiseGame) : BTypedPacket(packetType), mAdvertise(true) {}
   
      virtual void            serialize(BSerialBuffer &sb)
										{ 
											BTypedPacket::serialize(sb); 
											sb.add(mAdvertise);
											sb.add(mGameName);
										}

      virtual void            deserialize(BSerialBuffer &sb) 
										{ 
											BTypedPacket::deserialize(sb); 
											sb.get(&mAdvertise); 
											sb.get(&mGameName);
										}

   public:
      bool		mAdvertise;
		BSimString	mGameName;
};



//==============================================================================
class BDebuggerAttachPacket : public BTypedPacket
{
   public:
      BDebuggerAttachPacket(long packetType=BRemoteToolsHost::cDebuggerAttach) : BTypedPacket(packetType) {}
   
      virtual void            deserialize(BSerialBuffer &sb) 
										{ 
											BTypedPacket::deserialize(sb); 
											sb.get(&mMyAdvertisedName); 
											sb.get(&mDebuggerAddress);
											sb.get(&mDebuggerPort);
										}

   public:
      BSimString	mMyAdvertisedName;
		BSimString	mDebuggerAddress;
		long		mDebuggerPort;
};



//==============================================================================
class BConsoleCommandPacket: public BTypedPacket
{
public:
	BConsoleCommandPacket(long packetType=BRemoteToolsHost::cConsoleCommand) : BTypedPacket(packetType) {}

	virtual void            deserialize(BSerialBuffer &sb) 
	{ 
		BTypedPacket::deserialize(sb); 
		sb.get(&mCommand);
	}

public:
	BSimString mCommand;
};



#endif // BUILD_FINAL
#endif // RemoteToolsHost_h

//==============================================================================
// EOF: RemoteToolsHost.h
