//==============================================================================
// SocksMapper.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _SocksMapper_H_
#define _SocksMapper_H_

//==============================================================================
// Includes

#include "etl/singleton.hpp"
#include "winsockinc.h"
#include "SocksUnreliableSocket.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BSocksMapper : public BSingleton<BSocksMapper>,
                     public BSocket::BObserver
{
   public:
      
      static void destroyInstance();
      virtual ~BSocksMapper();
      
      // helper funcs for BSocksGameSocket
      HRESULT                    connectSocket(BSocksUnreliableSocketUser *user, IN CONST SOCKADDR_IN * RemoteAddress, 
                                                IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);
      HRESULT                    bindListenerSocket(BSocksUnreliableSocketUser *user, IN CONST SOCKADDR_IN * localAddress);

      void                       disconnectSocket(BSocksUnreliableSocketUser *user);


      virtual void recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress);

      virtual void connected(IN BSocket *Socket );
      virtual void disconnected(IN BSocket * Socket,IN DWORD Status);

      // Variables      

   private:   
      void  cleanup();
      // Functions
      // Variables
      BDynamicSimArray<BSocksUnreliableSocket*>     mUnreliableSocketVector;
      BDynamicSimArray<BSocksUnreliableSocketUser*> mSocketUserVector;

}; // BSocksMapper

//==============================================================================
#endif // _SocksMapper_H_

//==============================================================================
// eof: SocksMapper.h
//==============================================================================