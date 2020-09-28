//==============================================================================
// typedid.h
//
// Copyright (c) 1999-2000, Ensemble Studios
//==============================================================================

#ifndef _TYPEDID_H_
#define _TYPEDID_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations.
class BChunkWriter;
class BChunkReader;
class BTypedID;

typedef BDynamicSimArray<BTypedID> BTypedIDArray;

//==============================================================================
class BTypedID
{
   public:
      BTypedID(long type, long id) :
         mType(type),
         mID(id) { }
      BTypedID(const BTypedID& t) :
         mType(t.mType),
         mID(t.mID) { }
      BTypedID(void)
      {
         BTypedID::reset();
      }

      long                    operator==(const BTypedID &tID) const { return(mType==tID.getType() && mID==tID.getID()); }
      long                    operator!=(const BTypedID &tID) const { return(!(*this==tID)); }
      BTypedID&               operator=(const BTypedID& t) { mType=t.mType; mID=t.mID; return(*this); }
      
      long                    getType(void) const {return(mType);}
      void                    setType(long type) {mType=type;}
      long                    getID(void) const {return(mID);}
      void                    setID(long id) {mID=id;}
      void                    setData( long type, long id ) { mType=type; mID=id; }

      // This is for packed XZ tile coordinates.  Both x & z are stored as 16 bits.
      void                    setXZPackedID(long x, long z) {mID = (x<<16) | z;}
      void                    unpackXZPackedID(long &x, long &z) const {x=(0xFFFF0000&mID)>>16; z=0x0000FFFF&mID;}

      // This is for packed dopple IDs.  PlayerID gets 5 bits (allowing for 32 players) and doppleIndex gets 27 bits (allowing for a lot).
      void                    setDopplePackedID(long playerID, long doppleIndex) {mID=(playerID<<27)|doppleIndex;}
      void                    unpackDopplePackedID(long &playerID, long &doppleIndex) const {playerID=(0xF8000000&mID)>>27; doppleIndex=0x07FFFFFF&mID;}

      virtual void            reset( void ) { mType=mID=-1; }

      // Convenience function that copies only unit ids into the temp array.
      static void             loadUnitIDsIntoArray(long filterType, const BTypedID *ids, long count, BDynamicSimLongArray &array);
      // convenience funtion that copies only those typed id of cUnit type into the temp array
      static void             loadUnitIDsIntoArray(long filterType, const BTypedID *ids, long count, BTypedIDArray &array);


      //Save and load.
      static bool             writeVersion( BChunkWriter* chunkWriter );
      static bool             readVersion( BChunkReader* chunkReader );
      bool                    save( BChunkWriter* chunkWriter, bool scenario );
      bool                    load( BChunkReader* chunkReader, bool scenario );

   protected:
      long                    mType;
      long                    mID;

      //Static savegame stuff.
      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
};


extern const BTypedID cTypedIDNone;


//==============================================================================
#endif // _typedid_H_

//==============================================================================
// eof: typedid.h
//==============================================================================
