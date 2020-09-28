//==============================================================================
// mpplayermap.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPPLAYERMAP_H_
#define _MPPLAYERMAP_H_

//==============================================================================
// Includes

#include "mptypes.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BMPPlayerEntry
{
   public:
      BMPPlayerEntry(ClientID clientID, const BSimString& name) : mClientID(clientID), mName(name) {}
      virtual ~BMPPlayerEntry() {}

      virtual PlayerID getPlayerID(void) const = 0;
      ClientID getClientID(void) const { return(mClientID); }

      void           setName(const BSimString& name) { mName=name; }
      const BSimString& getName(void) const   { return(mName); }
      
   protected:
      ClientID       mClientID;
      BSimString        mName;
};

//==============================================================================
class BMPPlayerMap
{
   public:
      // Constructors
      BMPPlayerMap() {}

      // Destructor
      virtual ~BMPPlayerMap( void );

      // Functions      
      bool     addPlayer(BMPPlayerEntry *entry);
      bool     removePlayer(ClientID clientID);
      bool     removePlayer(PlayerID playerID);
      bool     removePlayer(const BSimString& name);

      ClientID getClientID(PlayerID playerID) const;
      ClientID getClientID(const BSimString& name) const;

      PlayerID getPlayerID(ClientID clientID) const;
      PlayerID getPlayerID(const BSimString& name) const;

      const BSimString* getName(PlayerID playerID) const;
      const BSimString* getName(ClientID clientID) const;

   protected:
      long     findEntry(PlayerID playerID) const;
      long     findEntry(ClientID clientID) const;
      long     findEntry(const BSimString& name) const;

      BDynamicSimArray<BMPPlayerEntry*> mPlayers;

}; // BMPPlayerMap

//==============================================================================
#endif // _MPPLAYERMAP_H_

//==============================================================================
// eof: mpplayermap.h
//==============================================================================
