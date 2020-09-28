//==============================================================================
// shape.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "shape.h"
#include "physics.h"
#include "xmlreader.h" 
//#include "boundingbox.h"
//#include "renderer.h"
//#include "debugprimitives.h"
#include "Physics/Internal/PreProcess/ConvexHull/hkpGeometryUtility.h"

#include "..\xgame\gamedirectories.h"

// xsystem
#include "reloadManager.h"


#pragma push_macro("new")
#undef new
//==============================================================================
// Defines
static const float cDefaultConvexRadius = 0.001f;

//==============================================================================
// BShape::BShape
//==============================================================================
BShape::BShape(void) :
   mphkShape(NULL),
   mType(cShapeTypeUnknown),
   mLoaded(false)
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif
}

//==============================================================================
// BShape::BShape
//==============================================================================
BShape::BShape( hkpShape* pShape ):
mphkShape(pShape)
{
   setShapeType(pShape);
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif
}


//==============================================================================
// BShape::~BShape
//==============================================================================
BShape::~BShape( void )
{
   if (mphkShape)
   {
      mphkShape->removeReference();
      mphkShape = NULL;
   }
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit(true);
#endif
} 

//==============================================================================
// BShape::addReference
//==============================================================================
void BShape::addReference( void )
{
   if (mphkShape)
      mphkShape->addReference();
}

//==============================================================================
// BShape::allocateSphere
//==============================================================================
bool BShape::allocateSphere(float radius)
{
   mphkShape = (hkpShape*) new hkpSphereShape(radius);
   if (!mphkShape)
      return (false);

   mType = cShapeTypeSphere;
   mphkShape->setUserData(mType);
   mLoaded = true;
   return (false);
}


//==============================================================================
// BShape::allocateCapsule
//==============================================================================
bool BShape::allocateCapsule(const BVector &v0, const BVector &v1, float radius)
{
   // Convert.
   hkVector4 hv0, hv1;
   BPhysics::convertPoint(v0, hv0);
   BPhysics::convertPoint(v1, hv1);
   
   mphkShape = (hkpShape*) new hkpCapsuleShape(hv0, hv1, radius);
   if (!mphkShape)
      return (false);

   mType = cShapeTypeCapsule;
   mphkShape->setUserData(mType);
   mLoaded = true;
   return (false);
}


//==============================================================================
// BShape::allocateBox
//==============================================================================
bool BShape::allocateBox(const BVector &halfExtents)
{
   hkVector4 hkHalfExtents;
  // BPhysics::convertPoint(halfExtents, hkHalfExtents);
   #ifdef HANDEDNESS_FLIP
      hkHalfExtents.set(halfExtents.z, halfExtents.y, halfExtents.x);
   #else
      hkHalfExtents.set(halfExtents.x, halfExtents.y, halfExtents.z);
   #endif
   mphkShape = (hkpShape*) new hkpBoxShape(hkHalfExtents);
   if (!mphkShape)
      return (false);

   mType = cShapeTypeBox;
   mphkShape->setUserData(mType);
   mLoaded = true;
   return (true);
}

//==============================================================================
// BShape::releaseHavokShape
//==============================================================================
void BShape::releaseHavokShape(void)
{
   if (mphkShape)
   {
      mphkShape->removeReference();
      mphkShape = NULL;
   }
}

//==============================================================================
// BShape::setShapeType
//==============================================================================
void BShape::setShapeType(const hkpShape* pShape)
{
   if (pShape)
   {
      long hkType = pShape->getType();
      switch (hkType)
      {
         case HK_SHAPE_BOX:
            mType = cShapeTypeBox;
            break;
            
         case HK_SHAPE_CONVEX_VERTICES:
            mType = cShapeTypeConvexVertices;
            break;

         case HK_SHAPE_SPHERE:
            mType = cShapeTypeSphere;
            break;
            
         case HK_SHAPE_CAPSULE:
            mType = cShapeTypeCapsule;
            break;
            
         case HK_SHAPE_LIST:
            mType = cShapeTypeList;
            break;

         default:
            mType = cShapeTypeUnknown;
      }
   }
}


//==============================================================================
// BShape::allocateConvexVertices
//==============================================================================
bool BShape::allocateConvexVertices(float *pVertices, long numVertices, int stride)
{

   return (false);
/*
  hkArray<hkVector4> planeEquations, usedVertices;
   hkGeometryUtil::computePlanesFromVertices(pVertices, numVertices, stride, planeEquations, usedVertices);
   mphkShape = (hkpShape*) new hkpConvexVerticesShape(pVertices, stride, numVertices, planeEquations);
   if (!mphkShape)
      return (false);
   
   hkpConvexVerticesShape* pConvexShape = (hkpConvexVerticesShape*)mphkShape;
   pConvexShape->setRadius(cDefaultConvexRadius);
   mType = cShapeTypeConvexVertices;
   mphkShape->setUserData(mType);
   return (true);
*/
}

//==============================================================================
// BShape::allocateConvexVertices
//==============================================================================
bool BShape::allocateConvexWrappedShape(const BVector4 *pVertices, long numVertices, float tolerance)
{
   return (false);
/*
   hkGeometry *pGeom = hkGeometryUtil::createConvexGeometry((const hkVector4*) pVertices, numVertices, false, tolerance);
   if (!pGeom)
      return (false);

   mType = cShapeTypeAutoWrapped;
  
   bool result =  allocateConvexVertices((float *) pGeom->getVertices(), pGeom->getNumVertices(), 16);
   if (!result)
      return (false);

   mphkShape->setUserData(mType);
   return (true);
*/
}


//==============================================================================
// BShape::allocateComplexShape
//==============================================================================
bool BShape::allocateComplexShape(const BDynamicSimArray<BShape*> &shapes, const BDynamicSimArray<BPhysicsMatrix> &transforms)
{
   //-- first create the transform shapes
   hkArray<hkpShape*> shapeArray;

   long count = shapes.getNumber();

   if (count == 0)
      return (false);

   for (long i=0; i < count; i++)
   {
      /*if (i==0)
      {
         shapeArray.pushBack(shapes[i]->getHavokShape());
         continue;
      }
      else
      {*/


     

      hkTransform t;
      hkVector4 translation(0,0,0,0);
      //hkQuaternion r;
      //r.setIdentity();
      BPhysicsMatrix tempMatrix;
      tempMatrix = transforms[i];
      tempMatrix.setTranslation(0,0,0);

      hkRotation rot;
      BPhysics::convertRotation(tempMatrix, rot);

      BPhysics::convertModelSpacePoint(transforms[i].getTranslation(), translation);
      t.setRotation(rot);
      t.setTranslation( translation );

      hkpTransformShape* transformShape = new hkpTransformShape( shapes[i]->getHavokShape(), t );
      shapeArray.pushBack(transformShape);

      //}
   
   }

   hkpListShape* listShape = new hkpListShape(&shapeArray[0], shapeArray.getSize());
   mphkShape = listShape;
   mType = cShapeTypeList;
   mphkShape->setUserData(mType);
   return (true);

}

//==============================================================================
// BShape::serializeToSimpleFile
//==============================================================================
bool BShape::serializeToSimpleFile(long dirID, const BCHAR_T *szFileName, bool bBinaryFormat /*=false*/)
{

   /*hkHavokSession session;
   hkRegisterHavokClasses::registerAll(session);


   hkStreambuf* sb = customStreambufFactory::getInstance().open(dirID, szFileName, hkStreambuf::OUTPUT);

   if (!sb->isOk())
   {
      return (false);
   }

   // And an serializer around it; we keep track if it's an XML serializer
   hkSerializer* serializer;
   hkXmlSerializer* xmlSerializer = HK_NULL;
   {
      if (bBinaryFormat)
      {
         serializer = new hkBinarySerializer(sb, session.getStringPool());
      }
      else
      {
         serializer = xmlSerializer = new hkXmlSerializer(sb, session.getStringPool());
      }
   }

   if (!serializer)
      return (false);

   sb->removeReference(); // serializer now owns this

   hkSerializeMissingObjectsCallback serializeAll(session, *serializer);
 
   if (xmlSerializer)
   {
   
      xmlSerializer->writeDocumentHeader();
      xmlSerializer->writeHkeStartTag();
   }

   serialize(session, *serializer, serializeAll);

   if (xmlSerializer)
   {
      xmlSerializer->writeHkeEndTag();
   }

   serializer->removeReference();*/

   if (bBinaryFormat)
      return (false);

   if (!mphkShape)
      return (false);

   hkStreamWriter *sw = customStreambufFactory::getInstance().openWriter(dirID, szFileName);
   if (!sw)
      return (false);

   hkOstream ostream(sw);
   hkXmlPackfileWriter writer;
   writer.setContents(&mphkShape, hkpShapeClass);
   hkPackfileWriter::Options options; // use default options
   writer.save(ostream.getStreamWriter(), options);

   return (true);
}


//==============================================================================
// BShape::serialize
//==============================================================================
bool BShape::serialize(hkHavokSession &session, hkSerializer &serializer, hkSerializeMissingObjectsCallback &callback)
{
   return (false);
/*
   if (!mphkShape)
      return (false);

  
   session.serializeObject(serializer, mphkShape, &callback, HK_NULL, true);
   return (true);
*/
}

//==============================================================================
// BShape::renderWireframe
//==============================================================================
bool BShape::renderWireframe( const BPhysicsMatrix &worldMatrix, const hkpShape *pBaseShape, const hkVector4 &centerOfMass, bool drawCenterOfMass, DWORD color) 
{
   if (!pBaseShape)
      return (false);

   //gRenderer.setWorldTransform(worldMatrix);
   if (!gPhysics->getRenderinterface())
      return (false);

   //gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);

   BVector start, next, end;
   
   switch(pBaseShape->getType())
   {
      case HK_SHAPE_SPHERE:
         {
            hkpSphereShape *pShape = (hkpSphereShape*) pBaseShape;
            float radius = pShape->getRadius();
            //-- draw the sphere
            //-- get the old matrix
            BPhysicsMatrix oldMatrix;
            gPhysics->getRenderinterface()->getWorldMatrix(oldMatrix);

            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);

            gPhysics->getRenderinterface()->drawDebugSphere(radius, color);

            // orientations
            BVector pos = worldMatrix.getTranslation();
            BVector forward = worldMatrix.getForward();
            BVector up = worldMatrix.getUp();
            BVector right = worldMatrix.getRight();
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*forward, cDWORDGreen, cDWORDGreen);
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*up,      cDWORDBlue, cDWORDBlue);
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*right,   cDWORDRed, cDWORDRed);

            //-- restore
            gPhysics->getRenderinterface()->setWorldMatrix(oldMatrix);
         }
         break;

      case HK_SHAPE_BOX:
         {
            hkpBoxShape *pShape = (hkpBoxShape*) pBaseShape;
            BVector halfExtents;
            BPhysics::convertModelSpacePoint(pShape->getHalfExtents(), halfExtents);
            BPhysicsMatrix oldMatrix;
            gPhysics->getRenderinterface()->getWorldMatrix(oldMatrix);

            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);

            gPhysics->getRenderinterface()->drawDebugBox(halfExtents, color);

            // orientations
            BVector pos = worldMatrix.getTranslation();
            BVector forward = worldMatrix.getForward();
            BVector up = worldMatrix.getUp();
            BVector right = worldMatrix.getRight();
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*forward, cDWORDGreen, cDWORDGreen);
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*up,      cDWORDBlue, cDWORDBlue);
            gPhysics->getRenderinterface()->drawDebugLine(pos, pos+0.5f*right,   cDWORDRed, cDWORDRed);

            //-- restore
            gPhysics->getRenderinterface()->setWorldMatrix(oldMatrix);
         }   
         break;

      case HK_SHAPE_CYLINDER:
         {
            hkpCylinderShape *pShape = (hkpCylinderShape*) pBaseShape;
            BVector point1, point2, COM;
            BPhysics::convertModelSpacePoint(pShape->getVertex(0), point1);
            BPhysics::convertModelSpacePoint(pShape->getVertex(1), point2);
            BPhysics::convertModelSpacePoint(centerOfMass, COM);
            point1 -= COM;
            point2 -= COM;
            worldMatrix.transformPoint(point1, point1);
            worldMatrix.transformPoint(point2, point2);
            float radius = pShape->getCylinderRadius();
            
          
            gPhysics->getRenderinterface()->drawDebugLine(point1, point2, color, color);

            //-- draw two spheres
            BPhysicsMatrix worldMatrix;
            worldMatrix.makeIdentity();

            //-- get the old matrix
            BPhysicsMatrix oldMatrix;
            gPhysics->getRenderinterface()->getWorldMatrix(oldMatrix);


            worldMatrix.setTranslation(point1);
            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);
            gPhysics->getRenderinterface()->drawDebugSphere(radius, color);

            worldMatrix.setTranslation(point2);
            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);
            gPhysics->getRenderinterface()->drawDebugSphere(radius, color);

            //-- restore
            gPhysics->getRenderinterface()->setWorldMatrix(oldMatrix);
         }
         break;

      case HK_SHAPE_CAPSULE:
         {
            hkpCapsuleShape *pShape = (hkpCapsuleShape*) pBaseShape;
            BVector point1, point2, COM;
            BPhysics::convertModelSpacePoint(pShape->getVertex(0), point1);
            BPhysics::convertModelSpacePoint(pShape->getVertex(1), point2);
            BPhysics::convertModelSpacePoint(centerOfMass, COM);
            point1 -= COM;
            point2 -= COM;
            worldMatrix.transformPoint(point1, point1);
            worldMatrix.transformPoint(point2, point2);
            float radius = pShape->getRadius();
            
          
            gPhysics->getRenderinterface()->drawDebugLine(point1, point2, color, color);

            //-- draw two spheres
            BPhysicsMatrix worldMatrix;
            worldMatrix.makeIdentity();

            //-- get the old matrix
            BPhysicsMatrix oldMatrix;
            gPhysics->getRenderinterface()->getWorldMatrix(oldMatrix);


            worldMatrix.setTranslation(point1);
            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);
            gPhysics->getRenderinterface()->drawDebugSphere(radius, color);

            worldMatrix.setTranslation(point2);
            gPhysics->getRenderinterface()->setWorldMatrix(worldMatrix);
            gPhysics->getRenderinterface()->drawDebugSphere(radius, color);

            //-- restore
            gPhysics->getRenderinterface()->setWorldMatrix(oldMatrix);
         }
         break;

      case HK_SHAPE_LIST:
         {
            hkpListShape *pShape = (hkpListShape*) pBaseShape;
            long num = pShape->getNumChildShapes();
            for (long i=0; i < num; i++)
            {
               if(pShape->isChildEnabled(i))
               {
                  const hkpShape *pChildShape = pShape->getChildShapeInl(i);
                  renderWireframe(worldMatrix, pChildShape, centerOfMass);
               }
            }
         }
         break;

      case HK_SHAPE_MOPP:
         {
            hkpMoppBvTreeShape *pMoppShape = (hkpMoppBvTreeShape*) pBaseShape;
            const hkpShapeCollection* pShapeCollection = pMoppShape->getShapeCollection();

            if(pShapeCollection)
            {
               renderWireframe(worldMatrix, pShapeCollection, centerOfMass);
            }
         }
         break;

      case HK_SHAPE_TRANSFORM:
         {
            hkpTransformShape *pShape = (hkpTransformShape*) pBaseShape;
            hkTransform transform = pShape->getTransform();
            const hkVector4 &vec = transform.getTranslation();
            BVector offset;
            BPhysics::convertModelSpacePoint(vec, offset);
            const hkpShape *pChildShape = pShape->getChildShape();

            BPhysicsMatrix matrix(cIdentityMatrix);
            BPhysics::convertModelSpaceRotation(transform, matrix);
            matrix.setTranslation(offset);
            BPhysicsMatrix tempMatrix = matrix * worldMatrix;
            renderWireframe(tempMatrix, pChildShape, centerOfMass, false);
         }
         break;

      case HK_SHAPE_CONVEX_TRANSLATE:
         {
            hkpConvexTranslateShape *pShape = (hkpConvexTranslateShape*) pBaseShape;
            const hkVector4 &vec = pShape->getTranslation();
            BVector offset;
            BPhysics::convertModelSpacePoint(vec, offset);
            const hkpShape *pChildShape = pShape->getChildShape();

            BPhysicsMatrix matrix(cIdentityMatrix);
            matrix.setTranslation(offset);
            BPhysicsMatrix tempMatrix = matrix * worldMatrix;
            renderWireframe(tempMatrix, pChildShape, centerOfMass, false);
         }
         break;

      case HK_SHAPE_CONVEX_TRANSFORM:
         {
            hkpConvexTransformShape *pShape = (hkpConvexTransformShape*) pBaseShape;
            hkTransform transform = pShape->getTransform();
            const hkVector4 &vec = transform.getTranslation();
            BVector offset;
            BPhysics::convertModelSpacePoint(vec, offset);
            const hkpShape *pChildShape = pShape->getChildShape();

            BPhysicsMatrix matrix(cIdentityMatrix);
            BPhysics::convertModelSpaceRotation(transform, matrix);
            matrix.setTranslation(offset);
            BPhysicsMatrix tempMatrix = matrix * worldMatrix;
            renderWireframe(tempMatrix, pChildShape, centerOfMass, false);
         }
         break;

      case HK_SHAPE_CONVEX_VERTICES:
         {
            hkpConvexVerticesShape *pShape = (hkpConvexVerticesShape*) pBaseShape;

            // Convert these vertices to a geom.
            hkArray<hkVector4> originalVertices;
            pShape->getOriginalVertices(originalVertices); 

            long numVertices = originalVertices.getSize();
            float *vertices = new float[numVertices * 4];

            long i;
            for (i=0; i < numVertices; i++)
            {
               hkVector4 vertex = originalVertices[i];
               vertices[i*4] = vertex(0);
               vertices[i*4+1] = vertex(1);
               vertices[i*4+2] = vertex(2);
               vertices[i*4+3] = vertex(3);
            }

            hkStridedVertices stridedVerts;
            stridedVerts.m_numVertices = numVertices;
            stridedVerts.m_striding = sizeof(float) * 4;
            stridedVerts.m_vertices = vertices;

            hkGeometry geom;
            hkArray<hkVector4> planeEquations;

            hkpGeometryUtility::createConvexGeometry( stridedVerts, geom, planeEquations );

            delete[] vertices;

            // Render the geom
            long numTriangles = geom.m_triangles.getSize();
            for (i=0; i < numTriangles; i++)
            {
               const hkGeometry::Triangle &triangle = geom.m_triangles[i];
               hkVector4 hkStart = originalVertices[triangle.m_a];
               hkVector4 hkNext =originalVertices[triangle.m_b];
               hkVector4 hkEnd =originalVertices[triangle.m_c];

               hkStart.sub4(centerOfMass);
               hkNext.sub4(centerOfMass);
               hkEnd.sub4(centerOfMass);

               BPhysics::convertModelSpacePoint(hkStart, start);
               BPhysics::convertModelSpacePoint(hkEnd, end);
               BPhysics::convertModelSpacePoint(hkNext, next);

               //-- draw the triangle         
               worldMatrix.transformPoint(start, start);
               worldMatrix.transformPoint(end, end);
               worldMatrix.transformPoint(next, next);

               gPhysics->getRenderinterface()->drawDebugLine(start, next, color, color);
               gPhysics->getRenderinterface()->drawDebugLine(next, end, color, color);
               gPhysics->getRenderinterface()->drawDebugLine(end, start, color, color);
            }
         }
         break;
   }

   /*
   if (drawCenterOfMass)
   {
      //-- draw the center of mass
      BPhysicsMatrix orientation;
      BVector vCenter;
      orientation.makeIdentity();
      BPhysics::convertModelSpacePoint(centerOfMass, vCenter);
      orientation.setTranslation(vCenter);
      orientation *= worldMatrix;
      
      //gpDebugPrimitives->addDebugBox(orientation, 0.1f, RGB_GREEN, BDebugPrimitives::cCategoryPhysicsShape, -1.0f);
   }
   */
   
   return (true);

}


//==============================================================================
// BShape::load
//==============================================================================
bool BShape::load(long dirID)
{
   // Bail if we're already loaded (or if we tried and failed).
   if(mLoaded || mType == cShapeTypeFailedToLoad)
      return(true);
  
   // Nuke existing havok shape.
   if (mphkShape)
   {
      mphkShape->removeReference();
      mphkShape = NULL;
   }
   
   // Assume the worst.
   mLoaded = false;
   mType = cShapeTypeFailedToLoad;
   
   //-- no need to havokize the loading process.  Use a file.
   BSimString fullname(mFilename);
   fullname += B(".shp");


#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);

   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(dirID, fullname, fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cEventClassReloadNotify, 0);
#endif

   /*BFile file;
   bool ok = file.openReadOnly(dirID, fullname);
   if (!ok)
      return (false);*/

   BXMLReader reader;
   bool ok=reader.load(dirID, fullname);
   if (ok == false)
   {
      {setBlogError(0000); blogerror("Error loading %s during BShape::Load.", mFilename.getPtr());}
      return(false);
   }
   
   //Get root node.
   BXMLNode root(reader.getRootNode());
   
   if (root.getName().compare(B("hke")) != 0)
   {
      {setBlogError(0002); blogerror("Error loading %s during BShape::Load.  Not an HKE file.", mFilename.getPtr());}
      return(false);
   }

   long numObjects = root.getNumberChildren();
   if (numObjects == 0)
   {
      {setBlogError(0003); blogerror("Error loading %s during BShape::Load.  No objects in file.", mFilename.getPtr());}
      return(false);
   }

   BStringTable<hkpShape*> shapeMap;

   for (long i =0; i< numObjects; i++)
   {
      BXMLNode child(root.getChild(i));
      
      //-- last shape will be the root shape
      hkpShape *phkShape = BShape::createHavokShapeFromNode(child, shapeMap);
      if (phkShape)
         mphkShape = phkShape;
      
   }


   if(!mphkShape)
      return(false);

   setShapeType(mphkShape);
   mLoaded=true;


   return (true);
/*
   hkDefaultPointerRegistry nameMapper;
   hkHavokSession session(&nameMapper);
   hkRegisterHavokClasses::registerAll(session);
   
   // Assemble fullname (add extension)
   BSimString fullname(mFilename);
   fullname += B(".shp");
   
   hkStreambuf* sb = customStreambufFactory::getInstance().open(dirID, fullname, hkStreambuf::INPUT);
   if (!sb->isOk())
   {
      blogtrace("Could not open shape file '%s'.", BStrConv::toA(fullname));
      return (false);
   }

   // And an serializer around it; we keep track if it's an XML serializer
   hkXmlSerializer* serializer = new hkXmlSerializer(sb, session.getStringPool());
   if (!serializer)
      return (false);

   sb->removeReference(); // serializer now owns this


   // The following loop reads and interprets all packets, keeping track of the world


   while( 1 )
   {
      // We read a packet 
      hkPacket* packet = session.loadSinglePacket(*serializer);
      if( packet != HK_NULL )
      {
         // We only interpret Object packets
         if( packet->getType() == hkPacket::OBJECT)
         {
            hkObjectPacket* opacket = static_cast<hkObjectPacket*>(packet);
            // We create an object from the object packet
            hkSerializable* s = session.createObject( *opacket );

            if( s )
            {
               if (hkpShapeClass.isSuperClass((*s->getClass())))
               {
                  mphkShape = static_cast<hkpShape*>(s);
                  mphkShape->addReference();
                  if (mphkShape->getType() == HK_SHAPE_CONVEX_VERTICES)
                  {
                     hkpConvexVerticesShape *pConvexShape = (hkpConvexVerticesShape*)mphkShape;
                     pConvexShape->setRadius(cDefaultConvexRadius);
                  }
               }
            }
         }

          packet->removeReference();
          
        
      }
      else
         break;
   }

  
   // jce [7/14/2004] -- Since we store this outside of the hkpWorld, we don't want to support stuff
   // with backpointers.  Supposedly ConvexSweepShape is the only thing that does this,  
   if(session.hasBackPointers())
   {
      BFAIL("Backpointers needed loading shape.  Can't handle this.");
      
      if(mphkShape)
      {
         mphkShape->removeReference();
         mphkShape=NULL;
      }
      return(false);
   }

   if(!mphkShape)
      return(false);

   setShapeType(mphkShape);
   mLoaded=true;

   serializer->removeReference();
   return (true);
*/
}


//==============================================================================
// BShape::unload
//==============================================================================
void BShape::unload(void)
{
   if (mphkShape)
   {
      mphkShape->removeReference();
      mphkShape = NULL;
   }
   mType = cShapeTypeUnknown;
   mLoaded = false;
}


//============================================================================
// BShape::reload
//============================================================================
bool BShape::reload(void)
{
   if (mFilename.isEmpty())
      return false;

   unload();
   
   return load(gPhysics->getShapeManager().getBaseDirectoryID());  
}


//==============================================================================
// BShape::createHavokShapeFromNode
//==============================================================================
hkpShape* BShape::createHavokShapeFromNode(BXMLNode node, BStringTable<hkpShape*> &shapeMap)
{
   if (!node)
      return NULL;

   BSimString szName;
   if (!node.getAttribValueAsString("name", szName))
      return NULL;

   BSimString szValue;
   if (!node.getAttribValueAsString("type", szValue))
      return NULL;

   hkpShape *phkShape = NULL;
   long numberChildren = node.getNumberChildren();
   if (numberChildren <= 0)
      return (NULL);

   BXMLNode childNode;

   // switch on the shape type
   if (szValue.compare(B("hkBoxShape")) == 0)
   {
      for (long i =0; i < numberChildren; i++)
      {
          childNode = node.getChild(i);
          if (childNode)
          {
             if (childNode.getAttribValueAsString("name", szValue))
             {
                  //-- if this is not here, we cannot create this shape
                  //-- and it is the only thing we have to have to create the shape
                  if (szValue.compare(B("halfExtents"))==0)
                  {
                     if (childNode.getTextAsString(szValue))
                     {
                        //-- convert this to an hkVector4
                        hkVector4 vec;
                        if (BPhysics::convertStringToHKVector(szValue, vec))
                        {
                           phkShape = new hkpBoxShape(vec);
                        }
                     }
                  }
             }
          }

      }
   }
   else if (szValue.compare(B("hkCylinderShape")) == 0)
   {
      hkVector4 vertA, vertB;
      float radius = 0.0f;
      for (uint i = 0; i < (uint) numberChildren; i++)
      {
         childNode = node.getChild(i);
         if (childNode)
         {
            childNode.getAttribValueAsString("name", szValue);
            if (szValue.compare(B("radius")) == 0)
            {
               if (!childNode.getTextAsFloat(radius))
                  return NULL;
            }
            else if (szValue.compare(B("vertexA")) == 0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  bool success = BPhysics::convertStringToHKVector(szValue, vertA);
                  if (!success)
                     return NULL;
               }
            }
            else if (szValue.compare(B("vertexB")) == 0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  bool success = BPhysics::convertStringToHKVector(szValue, vertB);
                  if (!success)
                     return NULL;
               }
            }
         }
      }
      phkShape = new hkpCylinderShape(vertA, vertB, radius);
   }
   else if (szValue.compare(B("hkTransformShape")) == 0)
   {
      hkpShape *phkChildShape = NULL;
      hkTransform transform;

      for (long i =0; i < numberChildren; i++)
      {
         childNode = node.getChild(i);
         if (childNode)
         {
            if (childNode.getAttribValueAsString("name", szValue))
            {
               if (szValue.compare(B("childShape"))==0)
               {
                  //-- lookup the child shape and get a pointer 
                  //-- if this doesn't work, we cannot create this shape
                  BSimString tempStr;
                  if (!shapeMap.find(childNode.getTextPtr(tempStr), &phkChildShape))
                  {
                     return NULL;
                  }

               }
               else if (szValue.compare(B("transform"))==0)
               {
                  if (childNode.getTextAsString(szValue))
                  {
                     BPhysics::convertStringToHKTransform(szValue, transform);
                  }
               }
            }
         }
      }

      //-- now create the transform shape
      phkShape = new hkpTransformShape(phkChildShape, transform);

      //  WMJ , yes I am evil.  But Havok is more evil.  Why return a const shape?? [9/15/2005]
      if (phkShape)
      {
          const hkpShape *pNewShape = hkpCollapseTransformsDeprecated::collapseTransformShape((hkpTransformShape*)phkShape);
          if (pNewShape == phkShape)
          {
             // Horrible hack to remove the increased ref count [9/15/2005]
             phkShape->removeReference();
          }
          else
          {
            //-- we don't need the only one
            phkShape->removeReference();
            //- Horrible hack to cast away the constness so I can assign this
            phkShape = const_cast<hkpShape*>(pNewShape);
          }
      }
     

   }
   else if (szValue.compare(B("hkCapsuleShape")) == 0)
   {
      hkVector4 vertA, vertB;
      float radius = 0.0f;
      for (uint i = 0; i < (uint) numberChildren; i++)
      {
         childNode = node.getChild(i);
         if (childNode)
         {
            childNode.getAttribValueAsString("name", szValue);
            if (szValue.compare(B("radius")) == 0)
            {
               if (!childNode.getTextAsFloat(radius))
                  return NULL;
            }
            else if (szValue.compare(B("vertexA")) == 0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  bool success = BPhysics::convertStringToHKVector(szValue, vertA);
                  if (!success)
                     return NULL;
               }
            }
            else if (szValue.compare(B("vertexB")) == 0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  bool success = BPhysics::convertStringToHKVector(szValue, vertB);
                  if (!success)
                     return NULL;
               }
            }
         }
      }
      phkShape = new hkpCapsuleShape(vertA, vertB, radius);
   }
   else if (szValue.compare(B("hkListShape")) == 0)
   {
      hkArray<hkpShape*> shapeList;

      for (long i =0; i < numberChildren; i++)
      {
         childNode = node.getChild(i);
         if (childNode)
         {
            if (childNode.getAttribValueAsString("name", szValue))
            {
               if (szValue.compare(B("childShapes"))==0)
               {
                  
                  // WMJ HACK - we only use single list shapes right now.  so, no need to do much here but 
                  //-- get a single name.  This will not work with more complex shapes.  But we should
                  //-- have a diff loading mechanism for 3.0 object serialization before that matters
                  //BSimString text = childNode.getText();
                  BSimString text;
                  childNode.getText(text);
                  text.trimLeft(B(" ("));
                  text.trimRight(B(" )"));
                  

                  //-- now look it up
                  BSimString shapeName;

                  while(!text.isEmpty())
                  {
                     int spacePos = text.findLeft(' ');

                     if(spacePos != -1)
                     {
                        shapeName.copy(text, spacePos, 0);
                        text.substring(spacePos + 1, text.length());
                     }
                     else
                     {
                        shapeName.copy(text);
                        text.empty();
                     }

                     hkpShape *phkChildShape = NULL;
                     if (shapeMap.find(shapeName, &phkChildShape))
                     {
                        if (phkChildShape)
                           shapeList.pushBack(phkChildShape);
                     }
                  }
               }
            }
         }
      }

      //-- create the list shape
      if (shapeList.getSize() > 0)
      {
         phkShape = new hkpListShape(&shapeList[0], shapeList.getSize());
      }
   }
   else if (szValue.compare(B("hkSphereShape")) == 0)
   {
      float radius = 0.0f;
      for (uint i = 0; i < (uint) numberChildren; i++)
      {
         childNode = node.getChild(i);
         if (childNode)
         {
            childNode.getAttribValueAsString("name", szValue);
            if (szValue.compare(B("radius")) == 0)
            {
               if (!childNode.getTextAsFloat(radius))
                  return NULL;
            }
         }
      }
      phkShape = new hkpSphereShape(radius);
   }
   else if (szValue.compare(B("hkConvexVerticesShape")) == 0)
   {
      float radius = 0.0f;
      hkVector4 aabbHalfExtents;
      hkVector4 aabbCenter;

      long numVertices = 0;
      long numPlanes = 0;

      hkVector4 *planes = NULL;
      hkpConvexVerticesShape::FourVectors* rotatedVertices = NULL;

      for (uint i = 0; i < (uint) numberChildren; i++)
      {         
         childNode = node.getChild(i);
         if (childNode)
         {
            childNode.getAttribValueAsString("name", szValue);

            if (szValue.compare(B("radius")) == 0)
            {
               if (!childNode.getTextAsFloat(radius))
                  return NULL;
            }      
            else if (szValue.compare(B("aabbHalfExtents"))==0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  BPhysics::convertStringToHKVector(szValue, aabbHalfExtents);
               }
            }
            else if (szValue.compare(B("aabbCenter"))==0)
            {
               if (childNode.getTextAsString(szValue))
               {
                  BPhysics::convertStringToHKVector(szValue, aabbCenter);
               }
            }
            else if (szValue.compare(B("rotatedVertices")) == 0)
            {
               long numElements = 0;
               childNode.getAttribValueAsLong("numelements", numElements);

               rotatedVertices = new hkpConvexVerticesShape::FourVectors[numElements];
               
               for (uint j = 0; j < (uint) numElements; j++)
               {         
                  BXMLNode grandChildNode = childNode.getChild(j);
                  BXMLNode tempChildNode;

                  tempChildNode = grandChildNode.getChild((long)0);
                  if (tempChildNode.getTextAsString(szValue))
                  {
                     BPhysics::convertStringToHKVector(szValue, rotatedVertices[j].m_x);
                  }

                  tempChildNode = grandChildNode.getChild((long)1);
                  if (tempChildNode.getTextAsString(szValue))
                  {
                     BPhysics::convertStringToHKVector(szValue, rotatedVertices[j].m_y);
                  }

                  tempChildNode = grandChildNode.getChild((long)2);
                  if (tempChildNode.getTextAsString(szValue))
                  {
                     BPhysics::convertStringToHKVector(szValue, rotatedVertices[j].m_z);
                  }
               }
            }            
            else if (szValue.compare(B("numVertices")) == 0)
            {
               if (!childNode.getTextAsLong(numVertices))
                  return NULL;
            }            
            else if (szValue.compare(B("planeEquations")) == 0)
            {
               childNode.getAttribValueAsLong("numelements", numPlanes);

               planes = new hkVector4[numPlanes];
               long count = 0;

               if (childNode.getTextAsString(szValue))
               {
                  const char *string = szValue.getPtr();
                  do
                  {
                     string = strchr(string, '(');
                     BPhysics::convertStringToHKVector(string, planes[count++]);
                     string = strchr(string, '\n');
                  }
                  while(string != NULL);
               }
            }
         }
      }

      hkAabb aabb(aabbCenter - aabbHalfExtents, aabbCenter + aabbHalfExtents);
      phkShape = new hkpConvexVerticesShape(rotatedVertices, numVertices, planes, numPlanes, aabb, radius);
   }


   if (phkShape)
      shapeMap.add(szName, phkShape);
   return phkShape;
}


//============================================================================
// BShape::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BShape::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
      gConsoleOutput.status("Reloading shape: %s", mFilename.getPtr());
      
      // Reload animation
      reload();

      // Fix bindings if the animation is playing
      int shapeIndex = gPhysics->getShapeManager().find(mFilename);
      gPhysics->getShapeManager().resetShape(shapeIndex);
   }

   return false;
}
#endif

#pragma pop_macro("new")

//==============================================================================
// eof: shape.cpp
//==============================================================================
