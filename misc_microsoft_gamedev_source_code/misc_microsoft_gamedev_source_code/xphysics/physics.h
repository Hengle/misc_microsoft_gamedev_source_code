//==============================================================================
// physics.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _PHYSICS
#define _PHYSICS

//#define HANDEDNESS_FLIP

//==============================================================================
// Includes
#pragma warning( disable : 4244 )

//Must undefine new before including any havok headers. mwc
#pragma push_macro("new")
#undef new
   #include "havokMemoryOverride.h"
#pragma pop_macro("new")

//#include "common.h"
#pragma warning(disable: 4995)
//#include "physicsobject.h"
#pragma warning(default: 4995)
//#include "physicscharacter.h"
//#include "physicsvehicle.h"
//#include "vehicleblueprintmanager.h"
//#include "havokarchive.h"
//#include "shape.h"
//#include "constraint.h"
#include "shapemanager.h"
#include "physicsobjectblueprintmanager.h"
#include "physicsevent.h"
//#include "physicsworld.h"
#include <Physics/Collide/hkpCollide.h>
#include <Common/Base/System/Io/Reader/hkStreamReader.h>
#include <Common/Base/System/Io/Writer/hkStreamWriter.h>
#include <Common/Serialize/Packfile/Xml/hkXmlPackfileWriter.h>
#include <Common/Base/System/Io/StreambufFactory/hkStreambufFactory.h>
#include <Physics/Dynamics/Entity/hkpEntity.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>
#include <Physics/Dynamics/Entity/hkpRigidBodyCinfo.h>
#include <Physics/Dynamics/World/hkpWorld.h>
#include <Physics/Utilities/Destruction/BrakeOffParts/hkpBreakOffPartsUtil.h>


typedef void (*BShapeIDRemapFunc)(long& id);
typedef void (*BPhysicsInfoIDRemapFunc)(long& id);


//==============================================================================
// conversions
float convertKilogramsToPounds(float kg);
float convertPoundsToKilograms(float lbs);
float convertInchesToMillimeters(float inches);
float convertMillimetersToInches(float mm);

//----------------------------------------------------------------------------
// CLAMP -- clamps the passed in value between the min / max to ensure 
//          that is within a certain range
//----------------------------------------------------------------------------
#define CLAMP( a, min, max )	( ((a) < (min)) ? (a) = (min) : ( ((a) > (max)) ? (a) = (max) : (a) ) )

#define CLAMP_SIGNED(val, maxVal)   if(val>0.0f && val>maxVal) val=maxVal; else if(val<0.0f && val<-maxVal) val=-maxVal;

//----------------------------------------------------------------------------
// Wrap -- similar to clamp, but effects a wrap around effect
//----------------------------------------------------------------------------
#define WRAP( a, min, max )  { while ( (a) < (min)) { (a)+=((max)-(min)+1);};  while ((a) > (max)) { (a)-=((max)-(min)+1);};  }

//----------------------------------------------------------------------------
// ABS -- returns the absolute value of a number 
//----------------------------------------------------------------------------
#define ABS( a ) ( ((a) < 0) ? -(a) : (a) )

//----------------------------------------------------------------------------
// FABS -- returns the absolute value of a number
//----------------------------------------------------------------------------
#define FABS( a ) ( ((a) < 0.0f) ? -(a) : (a) )

//----------------------------------------------------------------------------
// SIGN -- returns the sign of a number, either 1 or -1
//----------------------------------------------------------------------------
#define SIGN( a ) ( ((a) < 0) ? (-1) : (1) )

//----------------------------------------------------------------------------
// MIN -- get the minimum of two values
//----------------------------------------------------------------------------
#define MIN( val1, val2) (((val1) < (val2)) ? (val1) : (val2))

//----------------------------------------------------------------------------
// MAX -- get the maximum of two values
//----------------------------------------------------------------------------
#define MAX( val1, val2) (((val1) > (val2)) ? (val1) : (val2))

//----------------------------------------------------------------------------
// LERP -- Lineraly interpolate from value a to value b to a given alpha
//----------------------------------------------------------------------------
#define LERP( a, b, alpha) ( (a) + (( (b) - (a) ) * (alpha)) )

//----------------------------------------------------------------------------
// ROUND -- Round to nearest whole number
//----------------------------------------------------------------------------
#define ROUND( val ) ((long)(((float)(val))+(0.5f)) )

//----------------------------------------------------------------------------
// RADTODEG -- converts radians to degrees
//----------------------------------------------------------------------------
#define RADTODEG( radian ) (cDegreesPerRadian*radian)

//----------------------------------------------------------------------------
// DEGTORAD -- converts degrees to radians
//----------------------------------------------------------------------------
#define DEGTORAD( degrees ) (cRadiansPerDegree*degrees)

#include "physicsmatrix.h"
#include "physicsworld.h"
#include "physicsobject.h"




//==============================================================================
// Forward declarations


class hkpWorld;
class hkVisualDebugger;
class hkpAction;
class hkpRigidBody;
class hkTransform;
class hkDriverInputComponent;
class BRaycastVehicle;
class hkVehicleFramework;
class hkRotation;
class hkVector4;
class hkQuaternion;
class hkpPhantom;
class BHavokArchive;
class BTerrainNode;
class BRayHit;

class hkDefaultWheels;
class hkDefaultChassis;
class hkDefaultSteering;
class BEngine;
class BTransmission;
class BBrakes;
class hkDefaultSuspension;
class hkDefaultAerodynamics;
class hkAngularVelocityDamper;
class BPhysicsMatrix;






//==============================================================================
// Const declarations
enum
{
   cPhysicsEventConstraintBroken=cPhysicsEventBaseSim,
   cPhysicsEventCount
};


//==============================================================================
// Defines

class BPhysicsRenderInterface
{
public:
   virtual void                           addDebugLine(const char *szName, long group, const BVector &start, const BVector &end, DWORD color1, DWORD color2)=0;
   virtual void                           drawDebugLine(const BVector &start, const BVector &end, DWORD color1, DWORD color2)=0;
   virtual void                           drawDebugSphere(float radius, DWORD color)=0;
   virtual void                           drawDebugBox(const BVector& halfExtents,DWORD color)=0;
   virtual void                           removeDebugLines(const char* szName)=0;
   virtual void                           setWorldMatrix(const BPhysicsMatrix &matrix)=0;
   virtual void                           getWorldMatrix( BPhysicsMatrix &matrix )=0;
 
};
class BPhysicsWorld;
//==============================================================================
class BPhysics 
{
   public:
      BPhysics( void );
      virtual ~BPhysics( void );

      //-- Havok conversion utilities
      static void                convertModelSpacePoint(const hkVector4& in, BVector &out); 
      static void                convertModelSpacePoint(const BVector& in, hkVector4 &out);
      static void                convertModelSpaceRotation(const hkTransform &in,  BPhysicsMatrix &out);

      static void                convertPoint(const hkVector4& in, BVector &out); 
      static void                convertPoint(const BVector& in, hkVector4 &out);

      static void                convertDimension(const hkVector4& in, BVector &out); 
      static void                convertDimension(const BVector& in, hkVector4 &out);

      static void                convertRotation(const hkRotation &in, BPhysicsMatrix &out);
      static void                convertRotation(const hkQuaternion &in, BPhysicsMatrix &out);
      static void                convertRotation(const hkQuaternion &in, BPhysicsQuat &out);
      static void                convertRotation(const BPhysicsQuat &in, hkQuaternion &out);
      static void                convertRotation(const BPhysicsMatrix &in, hkRotation &out);

      static bool                convertStringToHKVector(const char* szString, hkVector4 &vec);
      static bool                convertStringToHKTransform(const char* szString, hkTransform &t);


      //-- Initialization\Shutdown
      bool                       setup(void);
      bool                       shutdown(void);

      static void                setShapeIDRemapFunc(BShapeIDRemapFunc func) { sShapeIDRemapFunc = func; }
      void                       remapShapeID(long& id) { if (sShapeIDRemapFunc) (sShapeIDRemapFunc)(id); }

      static void                setPhysicsInfoIDRemapFunc(BPhysicsInfoIDRemapFunc func) { sPhysicsInfoIDRemapFunc = func; }
      void                       remapPhysicsInfoID(long& id) { if (sPhysicsInfoIDRemapFunc) (sPhysicsInfoIDRemapFunc)(id); }


      // Worlds.
      BPhysicsWorld              *createWorld(void);
      void                       releaseWorld(BPhysicsWorld *world);

      // Shape manager.
      BShapeManager              &getShapeManager(void) {return(mShapeManager);}

      // Blueprint manager.
      BPhysicsObjectBlueprintManager &getPhysicsObjectBlueprintManager(void) {return(mPhysicsObjectBlueprintManager);}

      void                             setRenderInterface(BPhysicsRenderInterface *pInterface)  { mpRenderInterface = pInterface; }
      BPhysicsRenderInterface *        getRenderinterface( void )                               { return mpRenderInterface; }

   protected:
  
      static BPhysicsInfoIDRemapFunc sPhysicsInfoIDRemapFunc;
      static BShapeIDRemapFunc sShapeIDRemapFunc;

      BShapeManager                       mShapeManager;
      BPhysicsObjectBlueprintManager      mPhysicsObjectBlueprintManager;
      BPhysicsRenderInterface             *mpRenderInterface;
}; 



/*//==============================================================================
// class customStreambuf 
// 
// Havok stream IO stuff.
//==============================================================================
class customStreambuf : public hkStreambuf
{
   public:
      customStreambuf(const char* fname, OpenMode mode);
      customStreambuf(long dirID, const BCHAR_T *fname, OpenMode mode);
      virtual ~customStreambuf();
      virtual int sysWrite(const void* buf, int nbytes);
      virtual int sysRead(void* buf, int nbytes);
      virtual void sysFlush();
      virtual hkResult sysSeek( long offset, SeekWhence whence);
      virtual int sysTell();
      virtual int sysGetFileSize();

   protected:
      BFile                   mFile;
		char                    m_buffer[256];    // secretly required but undocumented
};*/

/*class customStreamReader : public hkStreamReader
{
public:
   
};*/

class customStreamReader : public hkStreamReader
{
public:
   customStreamReader(const char* fname);
   customStreamReader(long dirID, const BCHAR_T* fname);
   virtual ~customStreamReader();

   virtual hkBool  isOk () const;
   virtual int  read(void *buf, int nbytes);
   virtual int  skip (int nbytes);
   
   virtual hkBool  seekTellSupported () const; 
   virtual hkResult  seek (int offset, SeekWhence whence); 
   virtual int  tell () const;

protected:
   BFile                   mFile;
   bool                    mbOK;

};

class customStreamWriter : public hkStreamWriter
{
public:
      customStreamWriter(const char* fname);
      customStreamWriter(long dirID, const BCHAR_T* fname);
      virtual ~customStreamWriter();

      virtual hkBool  isOk () const;
      virtual int  write (const void *buf, int nbytes);
      virtual void  flush (); 
      virtual hkBool  seekTellSupported () const; 
      virtual hkResult  seek (int offset, SeekWhence whence); 
      virtual int  tell () const;

protected:
      BFile                   mFile;
      bool                    mbOK;

};

//==============================================================================
// class customStreambufFactory
// 
// Havok stream IO stuff.
//==============================================================================
class customStreambufFactory : public hkStreambufFactory
{
   public:
#pragma push_macro("new")
#undef new
    
      virtual hkStreamWriter* openConsole(StdStream s)
      {
         return NULL;
      }

      virtual hkStreamReader* openReader(const char* name)
      {
         //return new customStreamReader(name);
         return NULL;
      }

      virtual hkStreamReader* openReader(long dirID, const BCHAR_T* name)
      {
         //return new customStreamReader(name);
         return NULL;
      }
       
      virtual hkStreamWriter* openWriter(const char* name)
      {
         return new customStreamWriter(name);
      }

      virtual hkStreamWriter* openWriter(long dirID, const BCHAR_T* name)
      {
         return new customStreamWriter(dirID, name);
      }

      static hkReferencedObject* create() 
      { 
         return new customStreambufFactory();
      }

      static customStreambufFactory &getInstance(void) {return(*(customStreambufFactory*)(&hkStreambufFactory::getInstance()));}

#pragma pop_macro("new")
};


extern BPhysics* gPhysics;

#endif
//==============================================================================
// eof: physics.h
//==============================================================================
