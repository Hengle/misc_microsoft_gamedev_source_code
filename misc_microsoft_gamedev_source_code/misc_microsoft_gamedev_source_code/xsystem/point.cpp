//==============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Point Stuff
//==============================================================================

#include "xsystem.h"
#include "point.h"

BPoint tempPoint;


//==============================================================================
// BRect::BRect
//==============================================================================
BRect::BRect(long x1, long y1, long x2, long y2)
{
   mP1.x = x1;
   mP1.y = y1;
   mP1.z = 0;

   mP2.x = x2;
   mP2.y = y2;
   mP2.z = 0;
}

BRect::BRect(long x1, long y1, long z1, long x2, long y2, long z2)
{
   mP1.x = x1;
   mP1.y = y1;
   mP1.z = z1;

   mP2.x = x2;
   mP2.y = y2;
   mP2.z = z2;
}
//==============================================================================
// BRect::getIntersection
//==============================================================================
/*Static*/
bool BRect::getIntersection(const BRect &rectA, const BRect &rectB, BRect *rectOut) //RectB is the clipping rect
{
   bool emptyIntersect = false;

   if(!rectOut)
      return false;

   long rAx1 = rectA.mP1.x;
   long rAy1 = rectA.mP1.y;
   long rAx2 = rectA.mP2.x;
   long rAy2 = rectA.mP2.y;

   long rBx1 = rectB.mP1.x;
   long rBy1 = rectB.mP1.y;
   long rBx2 = rectB.mP2.x;
   long rBy2 = rectB.mP2.y;

   //Determine if rectA is inside rectB
   
   //Check Horizontal Overlap
   if(rAx1 <= rBx1) 
   {
      rectOut->mP1.x = rBx1;
      if(rAx2 < rBx1)
      {
         emptyIntersect = true;
      }
      else
      {
         //Get min(rax2, rbx2)
         long minVal = -1; 
         if(rAx2 < rBx2)
            minVal = rAx2;
         else
            minVal = rBx2;

         rectOut->mP2.x = minVal;
      }
   }
   else
   {
      rectOut->mP1.x  = rAx1;
      if(rBx2 < rAx1)
      {
         emptyIntersect = true;
      }
      else
      {
         long minVal = -1;
         if(rBx2 < rAx2)
            minVal = rBx2;
         else
            minVal = rAx2;

         rectOut->mP2.x = minVal;
      }
   }
   //Check Vertical Overlap
   if(rAy2 >= rBy2)
   {
      rectOut->mP2.y = rBy2;
      if(rAy1 > rBy2)
      {
         emptyIntersect = true;
      }
      else
      {
         long maxVal = -1;
         if(rAy1 > rBy1)
            maxVal = rAy1;
         else
            maxVal = rBy1;

         rectOut->mP1.y = maxVal;
      }
   }
   else
   {
      rectOut->mP2.y = rAy2;
      if(rBy1 > rAy2)
      {
         emptyIntersect = true;
      }
      else
      {
         long maxVal = -1;
         if(rBy1 > rAy1)
            maxVal = rBy1;
         else
            maxVal = rAy1;

         rectOut->mP1.y = maxVal;
      }
   }
   if(emptyIntersect)
   {
      rectOut->mP1.x = 0;
      rectOut->mP1.y = 0;
      rectOut->mP2.x = 0;
      rectOut->mP2.y = 0;
      return false;
   }
   else
      return true;   
}
