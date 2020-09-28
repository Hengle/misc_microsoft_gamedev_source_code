//==============================================================================
// savegame.inl
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "containers\staticArray.h"

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoObjectID(T& id) const
{ 
   if (id<0)
   { 
      id=-1;
      return false;
   } 
   if (id>=mDB.mProtoObjectMap.getNumber())
   { 
      int val = getUniqueProtoObjectID(id);
      if (val == -1)
      {
         id = -1;
         return false;
      }
      id = (T)val;
      return true;
   } 
   id=(T)(mDB.mProtoObjectMap[id]);
   if (id == -1)
      return false;
   return true;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoSquadID(T& id) const
{ 
   if (id<0)
   { 
      id=-1; 
      return false;
   } 
   if (id>=mDB.mProtoSquadMap.getNumber())
   { 
      int val = getUniqueProtoSquadID(id);
      if (val == -1)
      {
         id = -1;
         return false;
      }
      id = (T)val;
      return true;
   } 
   id=(T)(mDB.mProtoSquadMap[id]);
   if (id == -1)
      return false;
   return true;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoObjectIDs(T& list) const
{
   bool retval = true;
   uint size = list.size();
   for (uint i=0; i<size; i++)
   {
      list[i] = getProtoObjectID(list[i]);
      if (list[i] == -1)
         retval = false;
   }
   return retval;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoSquadIDs(T& list) const
{
   bool retval = true;
   uint size = list.size();
   for (uint i=0; i<size; i++)
   {
      list[i] = getProtoSquadID(list[i]);
      if (list[i] == -1)
         retval = false;
   }
   return retval;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoTechIDs(T& list) const
{
   bool retval = true;
   uint size = list.size();
   for (uint i=0; i<size; i++)
   {
      list[i] = getProtoTechID(list[i]);
      if (list[i] == -1)
         retval = false;
   }
   return retval;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapProtoPowerIDs(T& list) const
{
   bool retval = true;
   uint size = list.size();
   for (uint i=0; i<size; i++)
   {
      list[i] = getProtoPowerID(list[i]);
      if (list[i] == -1)
         retval = false;
   }
   return retval;
}

//==============================================================================
//==============================================================================
template<class T> bool BSaveGame::remapObjectTypes(T& list) const
{
   bool retval = true;
   uint size = list.size();
   for (uint i=0; i<size; i++)
   {
      list[i] = getObjectType(list[i]);
      if (list[i] == -1)
         retval = false;
   }
   return retval;
}
