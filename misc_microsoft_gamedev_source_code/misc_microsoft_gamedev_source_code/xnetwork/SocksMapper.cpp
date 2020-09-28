//==============================================================================
// SocksMapper.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "SocksMapper.h"
#include "Socket.h"
#include "winsockinc.h"
#include "SocksUnreliableSocket.h"
#include "SocksHelper.h"

//==============================================================================
// Defines


//==============================================================================
//
//==============================================================================
void BSocksMapper::destroyInstance()
{
   mInstance.cleanup();
}

BSocksMapper::~BSocksMapper()
{
   cleanup();   
}

void BSocksMapper::cleanup()
{
   long count = mUnreliableSocketVector.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      mUnreliableSocketVector[idx]->dispose();
   }
   mUnreliableSocketVector.setNumber(0);
}

//==============================================================================
//
HRESULT BSocksMapper::connectSocket(BSocksUnreliableSocketUser *user, IN CONST SOCKADDR_IN * RemoteAddress,
                                                    IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
{
   nlog(cTransportNL, "connectSocket - SocksUnreliableSocketUser [%p]", user);
   if (RemoteAddress)
      nlog(cTransportNL, "  RemoteAddress %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));

   if (LocalAddress)
   {
      // See if we already have a socket on this port
      for (long i=0;i<mUnreliableSocketVector.getNumber();i++)
      {
         SOCKADDR_IN addr;
         HRESULT hr = mUnreliableSocketVector[i]->getLocalAddress(&addr);
         if (FAILED(hr))
            return hr;

#ifdef XBOX
         // rg [7/23/05] - Don't compare the local addresses on xbox because the local addr's will always be the same (or INADDR_ANY)
         const bool equal = (addr.sin_port == LocalAddress->sin_port);
#else
         const bool equal = (addr.sin_addr.S_un.S_addr == LocalAddress->sin_addr.S_un.S_addr) &&
                            (addr.sin_port == LocalAddress->sin_port);
#endif                            
         if (equal)
         {
            // if so, just point to this socket
            user->setSocket(mUnreliableSocketVector[i]);
            mSocketUserVector.add(user);
            nlog(cTransportNL, "  already have an UnreliableSocket [%p]", mUnreliableSocketVector[i]);
            return S_OK;
         }            
      }
   }

   // otherwise create a new socket and point to that one
   BSocksUnreliableSocket *socket = new BSocksUnreliableSocket(this);
   user->setSocket(socket);
   nlog(cTransportNL, "  new UnreliableSocket [%p]", socket);
   mUnreliableSocketVector.add(socket);
   mSocketUserVector.add(user);

   return (socket)->connect(RemoteAddress, LocalAddress); // keep in mind, the connected callback isn't going 
                                                          // to get to the calling class because 
                                                          // we haven't returned from this method yet   
}

HRESULT BSocksMapper::bindListenerSocket(BSocksUnreliableSocketUser *user, IN CONST SOCKADDR_IN * localAddress)
{
   nlog(cTransportNL, "bindListenerSocket - SocksUnreliableSocketUser [%p]", user);
   if (localAddress)
      nlog(cTransportNL, "  localAddress %s:%ld", inet_ntoa(localAddress->sin_addr), ntohs(localAddress->sin_port));

   if (localAddress)
   {
      // See if we already have a socket on this port
      for (long i=0;i<mUnreliableSocketVector.getNumber();i++)
      {
         SOCKADDR_IN addr;
         HRESULT hr = mUnreliableSocketVector[i]->getLocalAddress(&addr);
         if (FAILED(hr))
            return hr;

         const bool equal = (addr.sin_port == localAddress->sin_port);
         if (equal)
         {
            // if so, just point to this socket
            user->setSocket(mUnreliableSocketVector[i]);
            mSocketUserVector.add(user);
            nlog(cTransportNL, "  already have an UnreliableSocket [%p]", mUnreliableSocketVector[i]);
            return S_OK;
         }            
      }
   }

   // otherwise create a new socket and point to that one
   BSocksUnreliableSocket *socket = new BSocksUnreliableSocket(this);
   user->setSocket(socket);
   nlog(cTransportNL, "  new UnreliableSocket [%p]", socket);
   mUnreliableSocketVector.add(socket);
   mSocketUserVector.add(user);
   return (socket->createAndBind( localAddress ));
}

//==============================================================================
//
void BSocksMapper::disconnectSocket(BSocksUnreliableSocketUser *user)
{
   nlog(cTransportNL, "disconnectSocket - SocksUnreliableSocketUser [%p]", user);
   
   for (long i=0; i<mSocketUserVector.getNumber(); i++)
   {
      if (mSocketUserVector[i] == user)
      {         
         BSocksUnreliableSocket *usocket = user->getSocket();
         mSocketUserVector.removeIndex(i);

         // if no-one is connected to this unreliable socket anymore, delete it
         bool found = false;
         for (long j=0;j<mSocketUserVector.getNumber();j++)
         {
            if (mSocketUserVector[j]->getSocket() == usocket)
            {
               found = true;
               break;
            }
         }

         if (!found)
         {
            for (long j=0; j<mUnreliableSocketVector.getNumber(); j++)
            {               
               if (mUnreliableSocketVector[j] == usocket)
               {
                  mUnreliableSocketVector.removeIndex(j);
                  usocket->dispose();
                  break;
               }      
            }
         }

         break;
      }
   }
}

//==============================================================================
//
void BSocksMapper::recvd (
         IN BSocket * Socket,
         IN const void * Buffer,
         IN DWORD Length,
         IN DWORD voiceLength,
         IN CONST SOCKADDR_IN * RemoteAddress)
{
   if (RemoteAddress)
      nlog(cTransportNL, "  BSocksMapper::recvd UnreliableSocket [%p], Length %ld, RemoteAddress %s:%ld", Socket, Length, inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));

   for (long i=0;i<mSocketUserVector.getNumber();i++)
   {
      if ( mSocketUserVector[i] && (mSocketUserVector[i]->getSocket() == Socket))
      {
         if (mSocketUserVector[i]->recvd(Socket, Buffer, Length, voiceLength, RemoteAddress))
            break;
      }
   }
}

//==============================================================================
//
void BSocksMapper::connected ( IN BSocket *Socket )
{
   Socket;
}

//==============================================================================
//
void BSocksMapper::disconnected (
   IN BSocket * Socket,
   IN DWORD Status)
{
   Socket;
   Status;
}

//==============================================================================
// BSocksMapper::
//==============================================================================



//==============================================================================
// eof: SocksMapper.cpp
//==============================================================================
