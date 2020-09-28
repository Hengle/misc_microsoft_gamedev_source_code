//==============================================================================
// Listener.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _Listener_H_
#define _Listener_H_

//==============================================================================
// Includes

#include "Socket.h"

//==============================================================================
// Forward declarations

class BReliableSocket;
class BSocket;

//==============================================================================
// Const declarations

//==============================================================================
class BReliableListener
{
   public:
      // Internal classes
      class BAcceptor
      {
         public:
            virtual bool canAccept (
               IN CONST SOCKADDR_IN * RemoteAddress)
            {
               UNREFERENCED_PARAMETER (RemoteAddress);
               return true;
            }

            virtual void accepted (
               IN BSocket * Socket) = 0;
      };

      // Constructors
      BReliableListener (
         IN BAcceptor * acceptor) : mAcceptor(acceptor)
      {
      }

      // Destructors
      virtual ~BReliableListener(void) {}

      // Functions
      // listen for incoming connections and generate sockets
      enum { cErrorAccepting=1, cErrorListening };
      enum { cDefaultBacklog=16 };

      virtual long listen (
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL,
         IN LONG Backlog = cDefaultBacklog) = 0;

      virtual long stopListening (void) = 0;
     
      // Variables
      
   protected:
      // Functions
      BAcceptor                  *getAcceptor(void) { return mAcceptor; }
      
      // Variables

   private:

      // Functions

      // Variables
      BAcceptor                  *mAcceptor;
}; // BListener


//==============================================================================
#endif // _Listener_H_

//==============================================================================
// eof: Listener.h
//==============================================================================