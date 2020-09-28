//==============================================================================
// mpplayer.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPPLAYER_H_
#define _MPPLAYER_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

class BClient;

//==============================================================================
// Const declarations

//==============================================================================
class BMPPlayer
{
   public:

      // Constructors
      BMPPlayer( BClient *client );

      // Destructors
      virtual ~BMPPlayer( void );

      // Functions
      DWORD                getID(void) const;
      const BSimString&       getName(void) const;
      void                 reset(void);
      
      void                 setLoaded(bool r);
      bool                 getLoaded(void) const;
      void                 setReadyToStart(bool r);
      bool                 getReadyToStart(void) const;

   private:

      DWORD             mID;
      BSimString           mName;
      bool              mLoaded;
      bool              mReadyToStart;
      BClient           *mClient;

}; // BMPPlayer

typedef BDynamicSimArray<BMPPlayer*> BMPPlayerList;

//==============================================================================
#endif // _MPPLAYER_H_

//==============================================================================
// eof: mpplayer.h
//==============================================================================
