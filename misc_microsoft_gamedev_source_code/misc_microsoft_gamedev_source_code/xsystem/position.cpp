//==============================================================================
// position.cpp
//
// Copyright (c) 1998-2000 Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "position.h"
#include "chunker.h"


//==============================================================================
// BPosition::msSaveVersion
//==============================================================================
const DWORD BPosition::msSaveVersion=0;


//==============================================================================
// BPosition::msLoadVersion
//==============================================================================
DWORD BPosition::msLoadVersion=0xFFFF;


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BPosition, 8, &gSimHeap);


//==============================================================================
//==============================================================================
void BPosition::onAcquire()
{
   mPosition.zero();
   mForward.zero();
   mUp.zero();
   mRight.zero();
}


//==============================================================================
//==============================================================================
void BPosition::onRelease()
{
   mPosition.zero();
   mForward.zero();
   mUp.zero();
   mRight.zero();
}


//==============================================================================
// BPosition::writeVersion
//==============================================================================
bool BPosition::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4071); blogerror("BPosition::writeVersion -- bad chunkWriter");}
      return(false);
   }

   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("PV"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4072); blogerror("BPosition::writeVersion -- failed to write version");}
      return(false);
   }

   return(true);
}


//==============================================================================
// BPosition::readVersion
//==============================================================================
bool BPosition::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4073); blogerror("BPosition::readVersion -- bad chunkReader");}
      return(false);
   }

   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("PV"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4074); blogerror("BPosition::readVersion -- failed to read version");}
      return(false);
   }

   return(true);
}


//==============================================================================
// BPosition::save
//==============================================================================
bool BPosition::save(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4075); blogerror("BPosition::save -- bad chunkWriter");}
      return(false);
   }

   /*bool ok=mPosition.save(chunkWriter);
   if(!ok)
   {
      {setBlogError(4076); blogerror("BPosition::save -- failed to write mPosition");}
      return(false);
   }

   ok=mForward.save(chunkWriter);
   if(!ok)
   {
      {setBlogError(4077); blogerror("BPosition::save -- failed to write mForward");}
      return(false);
   }

   ok=mUp.save(chunkWriter);
   if(!ok)
   {
      {setBlogError(4078); blogerror("BPosition::save -- failed to write mUp");}
      return(false);
   }

   ok=mRight.save(chunkWriter);
   if(!ok)
   {
      {setBlogError(4079); blogerror("BPosition::save -- failed to write mRight");}
      return(false);
   }*/
   long result;
   CHUNKWRITESAFE(chunkWriter, Vector, mPosition);
   CHUNKWRITESAFE(chunkWriter, Vector, mForward);
   CHUNKWRITESAFE(chunkWriter, Vector, mUp);
   CHUNKWRITESAFE(chunkWriter, Vector, mRight);

   return(true);
}


//==============================================================================
// BPosition::load
//==============================================================================
bool BPosition::load(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4080); blogerror("BPosition::load -- bad chunkReader");}
      return(false);
   }

   /*bool ok=mPosition.load(chunkReader);
   if(!ok)
   {
      {setBlogError(4081); blogerror("BPosition::load -- failed to read mPosition");}
      return(false);
   }

   ok=mForward.load(chunkReader);
   if(!ok)
   {
      {setBlogError(4082); blogerror("BPosition::load -- failed to read mForward");}
      return(false);
   }

   ok=mUp.load(chunkReader);
   if(!ok)
   {
      {setBlogError(4083); blogerror("BPosition::load -- failed to read mUp");}
      return(false);
   }

   ok=mRight.load(chunkReader);
   if(!ok)
   {
      {setBlogError(4084); blogerror("BPosition::load -- failed to read mRight");}
      return(false);
   }*/
   long result;
   CHUNKREADSAFE(chunkReader, Vector, mPosition);
   CHUNKREADSAFE(chunkReader, Vector, mForward);
   CHUNKREADSAFE(chunkReader, Vector, mUp);
   CHUNKREADSAFE(chunkReader, Vector, mRight);

   return(true);
}

//==============================================================================
// BPosition::yaw
//==============================================================================
void BPosition::yaw(const float amount)
{
   BMatrix rot;
   rot.makeRotateArbitrary(amount, mUp);
   mForward=rot*mForward;
   mRight=rot*mRight;
   mForward.normalize();
   mRight.normalize();
}


//==============================================================================
// BPosition::pitch
//==============================================================================
void BPosition::pitch(const float amount)
{
   BMatrix rot;
   rot.makeRotateArbitrary(amount, mRight);
   mForward=rot*mForward;
   mUp=rot*mUp;
   mForward.normalize();
   mUp.normalize();
}


//==============================================================================
// BPosition::roll
//==============================================================================
void BPosition::roll(const float amount)
{
   BMatrix rot;
   rot.makeRotateArbitrary(amount, mForward);
   mUp=rot*mUp;
   mRight=rot*mRight;
   mUp.normalize();
   mRight.normalize();
}


//==============================================================================
// eof: position.cpp
//==============================================================================
