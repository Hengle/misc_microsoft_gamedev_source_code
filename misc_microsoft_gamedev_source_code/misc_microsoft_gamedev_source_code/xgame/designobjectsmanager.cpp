//==============================================================================
// DesignObjectmanager.h
//
// DesignObjectmanager manages all DesignObjects
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "ai.h"
#include "designobjectsmanager.h"
#include "player.h"
#include "world.h"
#include "game.h"
#include "SegIntersect.h"
#include "usermanager.h"
#include "user.h"

IMPLEMENT_FREELIST(BDesignSphere, 5, &gSimHeap);
IMPLEMENT_FREELIST(BDesignLine, 5, &gSimHeap);



//==============================================================================
// Constants
//==============================================================================


////==============================================================================
//// BDesignObjectData::BDesignObjectData
////==============================================================================
BDesignObjectData::BDesignObjectData( void ) :
   mAttract(0),
   mRepel(0),
   mChokePoint(0),
   mValue(0)
{

}

////==============================================================================
//// BDesignObjectData::load
////==============================================================================
void BDesignObjectData::load(BXMLNode  xmlNode)
{
   long childCount = xmlNode.getNumberChildren();
   for(long j=0; j<childCount; j++)
   {
      BXMLNode childNode(xmlNode.getChild(j));

      if(childNode.getName() == "Type")
      {
         childNode.getText(mType);
      }
      else if(childNode.getName() == "Attract")
      {
         childNode.getTextAsFloat(mAttract);
      }
      else if(childNode.getName() == "Repel")
      {
         childNode.getTextAsFloat(mRepel);
      }
      else if(childNode.getName() == "ChokePoint")
      {
         childNode.getTextAsFloat(mChokePoint);
      }
      else if(childNode.getName() == "Value")
      {
         childNode.getTextAsFloat(mValue);
      }
      //Quick and fugly.
      else if (childNode.getName() == "Value1")
         childNode.getTextAsFloat(mValues[0]);
      else if (childNode.getName() == "Value2")
         childNode.getTextAsFloat(mValues[1]);
      else if (childNode.getName() == "Value3")
         childNode.getTextAsFloat(mValues[2]);
      else if (childNode.getName() == "Value4")
         childNode.getTextAsFloat(mValues[3]);
      else if (childNode.getName() == "Value5")
         childNode.getTextAsFloat(mValues[4]);
      else if (childNode.getName() == "Value6")
         childNode.getTextAsFloat(mValues[5]);
      else if (childNode.getName() == "Value7")
         childNode.getTextAsFloat(mValues[6]);
      else if (childNode.getName() == "Value8")
         childNode.getTextAsFloat(mValues[7]);
      else if (childNode.getName() == "Value9")
         childNode.getTextAsFloat(mValues[8]);
      else if (childNode.getName() == "Value10")
         childNode.getTextAsFloat(mValues[9]);
      else if (childNode.getName() == "Value11")
         childNode.getTextAsFloat(mValues[10]);
      else if (childNode.getName() == "Value12")
         childNode.getTextAsFloat(mValues[11]);

   }
}

////==============================================================================
////==============================================================================
bool BDesignObjectData::getValuesAsVector(uint index, BVector& result)
{
   if (index > 3)
      return (false);
   result.set(mValues[index*3], mValues[index*3+1], mValues[index*3+2]);
   return (true);
}

////==============================================================================
////==============================================================================
bool BDesignObjectData::isPhysicsLine() const
{
   float diff=mValue-(float)cPhysicsLine;
   if ((float)fabs(diff) < cFloatCompareEpsilon)
      return (true);
   return (false);
}

////==============================================================================
////==============================================================================
bool BDesignObjectData::isOneWayBarrierLine() const
{
   float diff=mValue-(float)cOneWayBarrierLine;
   if ((float)fabs(diff) < cFloatCompareEpsilon)
      return (true);
   return (false);
}

////==============================================================================
////==============================================================================
bool BDesignObjectData::isTerrainCollisionToggleLine() const
{
   float diff=mValue-(float)cTerrainCollisionsToggleLine;
   if ((float)fabs(diff) < cFloatCompareEpsilon)
      return (true);
   return (false);
}


////==============================================================================
//// BDesignSphere::BDesignSphere
////==============================================================================
BDesignSphere::BDesignSphere( void )
{

}


////==============================================================================
//// BDesignSphere::load
////==============================================================================
void BDesignSphere::load(BXMLNode  xmlNode)
{
   if(!xmlNode.getAttribValueAsLong("ID", mID))
   {

   }
   if(!xmlNode.getAttribValueAsVector("Position", mPosition))
   {

   }
   if(!xmlNode.getAttribValueAsFloat("Radius", mRadius))
   {

   }

   long childCount = xmlNode.getNumberChildren();
   for(long j=0; j<childCount; j++)
   {
      BXMLNode childNode(xmlNode.getChild(j));
      if(childNode.getName() == "Data")
      {
         mDesignData.load(childNode);
      }

   }
}


////==============================================================================
//// BDesignSphere::BDesignSphere
////==============================================================================
BDesignLine::BDesignLine( void )
{

}


////==============================================================================
//// BDesignSphere::load
////==============================================================================
void BDesignLine::load(BXMLNode  xmlNode)
{
   BVector position;
   mPoints.clear();
   if(!xmlNode.getAttribValueAsLong("ID", mID))
   {

   }
   if(xmlNode.getAttribValueAsVector("Position", position))
   {
      mPoints.add(position);
   }


   long childCount = xmlNode.getNumberChildren();
   for(long j=0; j<childCount; j++)
   {
      BXMLNode childNode(xmlNode.getChild(j));
      if(childNode.getName() == "Data")
      {
         mDesignData.load(childNode);
      }

      if(childNode.getName() == "Points")
      {
         BSimString tempStr;
         const BSimString& varNodeText = childNode.getText(tempStr); 
         if (!varNodeText.isEmpty())
         {
            BSimString token;
            BSimString locationTok;
            BVector location;
            long strLen = varNodeText.length();
            long loc = token.copyTok(varNodeText, strLen, -1, B("|"));
            while (loc != -1)
            {
               long locationLen = token.length();
               long tokenLoc = locationTok.copyTok(token, locationLen, -1, B(","));
               float x = locationTok.asFloat();
               tokenLoc = locationTok.copyTok(token, locationLen, tokenLoc+1, B(","));
               float y = locationTok.asFloat();
               tokenLoc = locationTok.copyTok(token, locationLen, tokenLoc+1, B(","));
               float z = locationTok.asFloat();
               location.set(x, y, z);
               mPoints.add(location);

               loc = token.copyTok(varNodeText, strLen, loc+1, B("|"));
            }
         }

      }
   }
}

//==============================================================================
//==============================================================================
bool BDesignLine::intersects(BVector point1, BVector point2, BVector& intersectionPoint)
{
   //Returns true if there's an intersection, false if not.  If there's an
   //intersection, intersectionPoint will be filled out.
   for (uint i=0; i < (mPoints.getSize()-1); i++)
   {
      //Just do X/Z collision.
      //long iVal=segmentIntersect(point1, point2, mPoints[i], mPoints[i+1], intersectionPoint);
      float r;
      float s;
      long iVal=segIntersect(point1.x, point1.z, point2.x, point2.z, mPoints[i].x, mPoints[i].z, mPoints[i+1].x, mPoints[i+1].z, r, s);
      if (iVal == cIntersection)
      {
         intersectionPoint.x=mPoints[i].x + (mPoints[i+1].x - mPoints[i].x)*s;
         intersectionPoint.y=mPoints[i].y + (mPoints[i+1].y - mPoints[i].y)*s;
         intersectionPoint.z=mPoints[i].z + (mPoints[i+1].z - mPoints[i].z)*s;
         return (true);
      }
      
   }
   return (false);
}

//==============================================================================
//==============================================================================
bool BDesignLine::imbeddedIncidenceIntersects(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint)
{
   //Returns true if there's an intersection, false if not.  If there's an
   //intersection, intersectionPoint will be filled out.
   //Uses the direction of the line to test the incidence of entry angle

   BVector orientDirection;
   //Angle (in Degrees) is Value 10.
   float angleLimit=88.0f;//mDesignData.mValues[9];
   forward.y=0.0f;
   forward.safeNormalize();

   for (uint i=0; i < (mPoints.getSize()-1); i++)
   {
      //Test angle
      orientDirection = mPoints[i+1] - mPoints[i];
      orientDirection.y = 0.0f;
      orientDirection.safeNormalize();
      orientDirection.assignCrossProduct(orientDirection, cYAxisVector);
         
      //Decide if we're "close enough" to the right direction, skip if not.
      float angleDifference=orientDirection.angleBetweenVector(forward);
      if (angleDifference > angleLimit*cRadiansPerDegree)
         continue;

      //Just do X/Z collision.
      float r;
      float s;
      long iVal=segIntersect(point1.x, point1.z, point2.x, point2.z, mPoints[i].x, mPoints[i].z, mPoints[i+1].x, mPoints[i+1].z, r, s);
      if (iVal == cIntersection)
      {
         intersectionPoint.x=mPoints[i].x + (mPoints[i+1].x - mPoints[i].x)*s;
         intersectionPoint.y=mPoints[i].y + (mPoints[i+1].y - mPoints[i].y)*s;
         intersectionPoint.z=mPoints[i].z + (mPoints[i+1].z - mPoints[i].z)*s;
         return (true);
      }
      
   }
   return (false);
}

//==============================================================================
//==============================================================================
bool BDesignLine::incidenceIntersects(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint)
{
   //See if we pass the incidence angle test.
   //OrientDirection is Vector0.
   BVector orientDirection;
   mDesignData.getValuesAsVector(0, orientDirection);
   orientDirection.y=0.0f;
   orientDirection.safeNormalize();

   //Angle (in Degrees) is Value 10.
   float angleLimit=mDesignData.mValues[9];
      
   //Decide if we're "close enough" to the right direction, fail if not.
   forward.y=0.0f;
   forward.safeNormalize();
   float angleDifference=orientDirection.angleBetweenVector(forward);
   if (angleDifference > angleLimit*cRadiansPerDegree)
      return (false);
      
   //If that passes, return true if there's an intersection, false if not.  If there's an
   //intersection, intersectionPoint will be filled out.
   if (intersects(point1, point2, intersectionPoint))
      return (true);

   return (false);
}

//==============================================================================
//==============================================================================
void BDesignLine::closestPointToLine(BVector point1, BVector direction, BVector& closestPoint)
{
   BASSERT(mPoints.getSize() >= 1);

   float mindist = mPoints[0].distanceToLine(point1, direction);
   closestPoint = mPoints[0];
   //Goes through the points in the design line and returns the one that is closest to the Ray
   for (uint i=1; i < mPoints.getSize(); i++)
   {
      float dist = mPoints[i].distanceToLine(point1, direction);
      if( dist < mindist )
      {
         mindist = dist;
         closestPoint = mPoints[i];
      }
   }
}

//==============================================================================
// BDesignObjectManager::BDesignObjectManager()
//==============================================================================
BDesignObjectManager::BDesignObjectManager() 
{
}

//==============================================================================
// BDesignObjectManager::~BDesignObjectManager()
//==============================================================================
BDesignObjectManager::~BDesignObjectManager( void )
{
}

//==============================================================================
// BDesignObjectManager::init
//==============================================================================
bool BDesignObjectManager::init( void )
{
   return( true );
}


//==============================================================================
// BDesignObjectManager::reset
//==============================================================================
void BDesignObjectManager::reset( void )
{
   uint sphereCount = mDesignSpheres.size();
   for (uint i=0; i<sphereCount; i++)
   {
      BDesignSphere::releaseInstance(mDesignSpheres[i]);      
   }
   mDesignSpheres.clear();   

   uint lineCount = mDesignLines.size();
   for (uint i=0; i<lineCount; i++)
   {
      BDesignLine::releaseInstance(mDesignLines[i]);      
   }
   mDesignLines.clear(); 
}

//==============================================================================
// BDesignObjectManager::load
//==============================================================================
void BDesignObjectManager::load( BXMLNode  xmlNode )
{
   long childCount = xmlNode.getNumberChildren();
   for(long j=0; j<childCount; j++)
   {
      BXMLNode childNode(xmlNode.getChild(j));
      if(childNode.getName() == "Spheres")
      {
         long sphereCount = childNode.getNumberChildren();
         for(long h=0; h<sphereCount; h++)
         {
            BDesignSphere* pDesignSphere=BDesignSphere::getInstance();
            if(!pDesignSphere)
               return;
            pDesignSphere->load(childNode.getChild(h));

            // Add opportunities for design sphere scouting chokepoints.
            if (pDesignSphere->mDesignData.mChokePoint > 0.0f)
            {
               long numPlayers = gWorld->getNumberPlayers();
               for (long i=0; i<numPlayers; i++)
               {
                  BAI* pAI = gWorld->getAI(i);
                  BASSERT(pAI);
                  if (pAI)
                     pAI->generateDesignSphereMissionTarget(pDesignSphere);
               }
            }

            mDesignSpheres.add(pDesignSphere);
         }
      }
      else if(childNode.getName() == "Lines")
      {
         long lineCount = childNode.getNumberChildren();
         for(long h=0; h<lineCount; h++)
         {
            BDesignLine* pObject=BDesignLine::getInstance();
            if(!pObject)
               return;
            pObject->load(childNode.getChild(h));
            mDesignLines.add(pObject);
         }
      }
   }
}

//==============================================================================
// Get the design line by the id
//==============================================================================
BDesignLine& BDesignObjectManager::getDesignLine(BDesignLineID id)
{
   BDesignLine empty;
   BDesignLine& foundLine = empty;
   uint lineCount = getDesignLineCount();   
   for(uint i = 0; i < lineCount; i++)
   {
//-- FIXING PREFIX BUG ID 2582
      const BDesignLine& line = getDesignLine(i);
//--
      if (line.mID == id)
      {
         foundLine = line;
      }
   }

   return (foundLine);
}

//==============================================================================
//==============================================================================
bool BDesignObjectManager::intersectsPhysicsLine(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint) const
{
   for (uint i=0; i < mDesignLines.getSize(); i++)
   {
      if (!mDesignLines[i]->mDesignData.isPhysicsLine())
         continue;
      if (mDesignLines[i]->incidenceIntersects(forward, point1, point2, intersectionPoint))
         return (true);
   }
   return (false);
}

