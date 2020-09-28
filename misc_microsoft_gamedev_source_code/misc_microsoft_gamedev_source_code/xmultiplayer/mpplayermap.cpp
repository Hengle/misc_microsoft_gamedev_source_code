//==============================================================================
// mpplayermap.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "mpplayermap.h"

//==============================================================================
// Defines

//==============================================================================
// BMPPlayerMap::~BMPPlayerMap
//==============================================================================
BMPPlayerMap::~BMPPlayerMap( void )
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
      delete mPlayers[idx];
   mPlayers.setNumber(0);
}

//==============================================================================
// BMPPlayerMap::addPlayer
//==============================================================================
bool BMPPlayerMap::addPlayer(BMPPlayerEntry *entry)
{
   long index = mPlayers.find(entry);
   if (index >= 0)
      return(false);

   mPlayers.add(entry);
   return(true);
}

//==============================================================================
// BMPPlayerMap::removePlayer
//==============================================================================
bool BMPPlayerMap::removePlayer(ClientID clientID)
{
   long index = findEntry(clientID);
   if (index < 0)
      return(false);

   BMPPlayerEntry *entry = mPlayers[index];
   mPlayers.removeIndex(index);
   delete entry;

   return(true);
}

//==============================================================================
// BMPPlayerMap::removePlayer
//==============================================================================
bool BMPPlayerMap::removePlayer(PlayerID playerID)
{
   long index = findEntry(playerID);
   if (index < 0)
      return(false);

   BMPPlayerEntry *entry = mPlayers[index];
   mPlayers.removeIndex(index);
   delete entry;

   return(true);
}

//==============================================================================
// BMPPlayerMap::removePlayer
//==============================================================================
bool BMPPlayerMap::removePlayer(const BSimString& name)
{
   long index = findEntry(name);
   if (index < 0)
      return(false);

   BMPPlayerEntry *entry = mPlayers[index];
   mPlayers.removeIndex(index);
   delete entry;

   return(true);
}

//==============================================================================
// BMPPlayerMap::getClientID
//==============================================================================
ClientID BMPPlayerMap::getClientID(PlayerID playerID) const
{
   long index = findEntry(playerID);
   if (index < 0)
      return(cMPInvalidClientID);

   return(mPlayers[index]->getClientID());
}

//==============================================================================
// BMPPlayerMap::getClientID
//==============================================================================
ClientID BMPPlayerMap::getClientID(const BSimString& name) const
{
   long index = findEntry(name);
   if (index < 0)
      return(cMPInvalidClientID);

   return(mPlayers[index]->getClientID());
}

//==============================================================================
// BMPPlayerMap::getPlayerID
//==============================================================================
PlayerID BMPPlayerMap::getPlayerID(ClientID clientID) const
{
   long index = findEntry(clientID);
   if (index < 0)
      return(cMPInvalidPlayerID);

   return(mPlayers[index]->getPlayerID());
}

//==============================================================================
// BMPPlayerMap::getPlayerID
//==============================================================================
PlayerID BMPPlayerMap::getPlayerID(const BSimString& name) const
{
   long index = findEntry(name);
   if (index < 0)
      return(cMPInvalidPlayerID);

   return(mPlayers[index]->getPlayerID());
}

//==============================================================================
// BMPPlayerMap::getName
//==============================================================================
const BSimString* BMPPlayerMap::getName(PlayerID playerID) const
{
   long index = findEntry(playerID);
   if (index < 0)
      return(NULL);
   return(&mPlayers[index]->getName());
}

//==============================================================================
// BMPPlayerMap::getName
//==============================================================================
const BSimString* BMPPlayerMap::getName(ClientID clientID) const
{
   long index = findEntry(clientID);
   if (index < 0)
      return(NULL);

   return(&mPlayers[index]->getName());
}

//==============================================================================
// BMPPlayerMap::findEntry
//==============================================================================
long BMPPlayerMap::findEntry(long playerID) const
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (mPlayers[idx]->getPlayerID() == playerID)
         return(idx);
   }
   return(-1);
}

//==============================================================================
// BMPPlayerMap::findEntry
//==============================================================================
long BMPPlayerMap::findEntry(ClientID clientID) const
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (mPlayers[idx]->getClientID() == clientID)
         return(idx);
   }
   return(-1);
}

//==============================================================================
// BMPPlayerMap::findEntry
//==============================================================================
long BMPPlayerMap::findEntry(const BSimString& name) const
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (name.compare(mPlayers[idx]->getName()) == 0)
         return(idx);
   }
   return(-1);
}

//==============================================================================
// BMPPlayerMap::
//==============================================================================

//==============================================================================
// eof: mpplayermap.cpp
//==============================================================================
