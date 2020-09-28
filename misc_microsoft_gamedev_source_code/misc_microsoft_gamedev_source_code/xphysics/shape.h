//==============================================================================
// shape.h
//
// Copyright (c) 2003, 2004 Ensemble Studios
//==============================================================================
#pragma once
#ifndef _SHAPE_H_
#define _SHAPE_H_

//==============================================================================
// Includes
#include "xmlreader.h"
#include "string\stringtable.h"
#include "physics.h"
#include "physicsmatrix.h"


//==============================================================================
// Forward declarations
class hkpShape;
class hkHavokSession;
class hkSerializer;
class hkSerializeMissingObjectsCallback;

//==============================================================================
// Const declarations


//==============================================================================

//==============================================================================
class BShape
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:

      enum
      {
         cShapeTypeFailedToLoad = -2,
         cShapeTypeUnknown = -1,
         cShapeTypeOldExporter = 0,
         cShapeTypeBox,
         cShapeTypeSphere,
         cShapeTypeCapsule,
         cShapeTypeConvexVertices,
         cShapeTypeAutoWrapped,
         cShapeTypeList,
         cShapeTypeCount,
      };

      BShape( void );
      BShape( hkpShape *pShape );
      virtual ~BShape( void );
      
      // Filename.
      void                    setFilename(const BCHAR_T *filename) {mFilename=filename;}
      const BSimString           &getFilename(void) const {return(mFilename);}

      // Load/unload.
      bool                    load(long dirID);
      void                    unload(void);
      bool                    reload(void);

      // Helper Funcs
      static hkpShape*         createHavokShapeFromNode(BXMLNode node, BStringTable<hkpShape*> &shapeMap);

      //-- accessors
      hkpShape *      getHavokShape( void )  { return mphkShape; }

      //-- memory management
      void           addReference(void);

      //-- shape creation & deletion
      bool           allocateSphere(float radius);
      bool           allocateCapsule(const BVector &v0, const BVector &v1, float radius);
      bool           allocateBox(const BVector &halfExtents);
      bool           allocateConvexVertices(float *pVertices, long numVertices, int stride);
      bool           allocateConvexWrappedShape(const BVector4 *pVertices, long numVertices,  float tolerance);
      bool           allocateComplexShape(const BDynamicSimArray<BShape*> &shapes, const BDynamicSimArray<BPhysicsMatrix> &transforms);
      void           releaseHavokShape(void);


      void           setShapeType(const hkpShape *pShape);

      //-- helper function to save this object to single file
      bool           serializeToSimpleFile(long dirID, const BCHAR_T *szFileName, bool bBinaryFormat =false);

      //-- save and load to havok serialized format
      bool           serialize(hkHavokSession &session, hkSerializer &serializer, hkSerializeMissingObjectsCallback &callback);
      bool           loadFromSHP(hkHavokSession &session, hkSerializer &serializer);

      //-- visualization
      static bool    renderWireframe(const BPhysicsMatrix &worldMatrix, const hkpShape *pShape, const hkVector4 &centerOfMass, bool drawCenterOfMass=true, DWORD color=cDWORDRed);

      
   protected:
      BSimString           mFilename;
      bool              mLoaded;

      hkpShape*          mphkShape;
      long              mType;

#ifdef ENABLE_RELOAD_MANAGER
      virtual bool            receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
};





//==============================================================================
#endif // _SHAPE_H_

//==============================================================================
// eof: shape.h
//==============================================================================
