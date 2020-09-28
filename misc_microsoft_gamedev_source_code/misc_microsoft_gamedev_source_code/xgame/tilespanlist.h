//==============================================================================
// tilespanlist.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once


// Includes
#include "configsgame.h"
#include "tilespan.h"

//==============================================================================
// BTileSpanList
//==============================================================================
class BTileSpanList : public BDynamicSimArray< BTileSpan >
{
   public:
      // ctor/dtor
      BTileSpanList();

      // extend or create a new span as needed, merging if possible
      // force specifies whether or not pre-existing data is overwritten
      void     addToSpan(long x, long z);//, const Type &data = Type(), bool force = false);

      // adds an entire new span to the list, merging as appropriate.
      void     addSpan(const BTileSpan &span, bool merge=true);

      // go through and find any adjacent spans and, well, merge them
      void     mergeSpans();

      // finds the Nth tile among the spans, counting from the start
      bool     findTileByIndex(long n, long *pX, long *pZ) const;

      // fills with the tiles within a given radius
      void     buildFromRadius(long radius, long xOffset=0, long zOffset=0);
};

//==============================================================================
// BTileSpanList::BTileSpanList
//==============================================================================
BTileSpanList::BTileSpanList() : BDynamicSimArray< BTileSpan >()
{
};

//==============================================================================
// BTileSpanList::addToSpan
//==============================================================================
void BTileSpanList::addToSpan(long x, long z)
{
   BTileSpan newSpan(x,z,1);//,true);
   addSpan(newSpan,true);
};

//==============================================================================
// BTileSpanList::addSpan
//==============================================================================
void BTileSpanList::addSpan(const BTileSpan &span, bool merge)
{
   if (span.getNumberTiles()<=0)
      return;
   
   long count=getNumber();
   long insertAt=-1;
   long sx=span.getStartX();
   long curSX;
   bool dupeX=false;

   // find appropriate X location
   for (long idx=0; idx<count; idx++)
   {
      curSX = (*this)[idx].getStartX();

      // if dupe starting X found, set idx
      if (curSX>=sx)
      {
         insertAt=idx;
         dupeX=(curSX==sx);
         break;
      }
   }

   // if no spot found, add to end
   if (insertAt<0)
   {
      insertAt = count;
   }
   // found an item with the same X value
   else if (dupeX)
   {      
      if (merge)
      {
         sx      = span.getStartX();
         long sz = span.getStartZ();
         long n  = span.getNumberTiles();

         while (sx==(*this)[insertAt].getStartX() &&
                (sz+n) >= (*this)[insertAt].getStartZ())
         {
            long sx2 = (*this)[insertAt].getStartX(); sx2;
            long sz2 = (*this)[insertAt].getStartZ();
            long n2  = (*this)[insertAt].getNumberTiles();

            // look for identical match, and skip
            if (span == (*this)[insertAt])
            {
#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
               if (gConfig.isDefined(cConfigSpanListSpew))
                  blogtrace("duplicate span X: %d Z: %d N: %d removed", sx, sz, n);
#endif                  
#endif
               return;
            }

            // look for overlaps, and merge where appropriate

            // if the start of the new span is somewhere in the current span
            if ( (sz>sz2) && (sz<=(sz2+n2)) )
            {
               // totally contained
               if ((sz+n)<=(sz2+n2))
               {
#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
                  if (gConfig.isDefined(cConfigSpanListSpew))
                     blogtrace("span X: %d Z: %d N: %d inside span X: %d Z: %d, N: %d", sx, sz, n, sx2, sz2, n2);
#endif                     
#endif
                  return;
               }
               // first part of new span overlaps end of current span
               else
               {
#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
                  if (gConfig.isDefined(cConfigSpanListSpew))
                     blogtrace("span X: %d Z: %d N: %d overlaps span X: %d Z: %d, N: %d", sx, sz, n, sx2, sz2, n2);
#endif                     
#endif
                  long tempCount = ((sz+n)-(sz2+n2))+n2;
                  (*this)[insertAt].setNumberTiles(tempCount);

                  return;
               }
            }
            // else if some part of the end of the new span overlaps the start of the current span
            else if ( (sz<sz2) && ((sz+n)>sz2) )
            {
#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
               if (gConfig.isDefined(cConfigSpanListSpew))
                  blogtrace("span X: %d Z: %d N: %d overlaps span X: %d Z: %d, N: %d", sx, sz, n, sx2, sz2, n2);
#endif                  
#endif
               (*this)[insertAt].setStartZ(sz);
               long tempCount = (n+n2)-((sz+n)-sz2);
               (*this)[insertAt].setNumberTiles(tempCount);

               return;
            }

            insertAt++;
         }
      }
      else
      {
         // don't add exact duplicate spans
         if (span == (*this)[insertAt])
         {
#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
            if (gConfig.isDefined(cConfigSpanListSpew))
               blogtrace("duplicate span X: %d Z: %d N: %d removed", span.getStartX(), span.getStartZ(), span.getNumberTiles()); 
#endif               
#endif
            return;
         }
      }
   }

   // add at insertion point
   setNumber(count+1);
   for (long idx=(getNumber()-1); idx>insertAt; idx--)
      (*this)[idx]=(*this)[idx-1];
   (*this)[insertAt]=span;

#ifndef BUILD_FINAL
#ifdef SPAN_LIST_SPEW
   if (gConfig.isDefined(cConfigSpanListSpew))
      blogtrace("span X: %d Z: %d N: %d added at %d", span.getStartX(), span.getStartZ(), span.getNumberTiles(), insertAt);
#endif      
#endif
};

//==============================================================================
// BTileSpanList::mergeSpans
// NOTE: This only connects spans that meet at a location, not overlap.
//==============================================================================
void BTileSpanList::mergeSpans()
{
   long count = getNumber()-1;

   for (long idx=0; idx<count; idx++)
   {
      long sx = (*this)[idx].getStartX();
      long sx2 = (*this)[idx+1].getStartX();
      
      // spans are in same X
      if (sx==sx2)
      {
         long sz = (*this)[idx].getStartZ();
         long n = (*this)[idx].getNumberTiles();
         long sz2 = (*this)[idx+1].getStartZ();
         long n2 = (*this)[idx+1].getNumberTiles();

         // span 2 starts at end of span 1
         // Since the spans are sorted on Z as well, this is the only
         // case we have to check
         if ((sz+n+1) == sz2)
         {
            (*this)[idx].setNumberTiles(n+n2);

            // remove the next node
            removeIndex(idx+1);

            // back up the counter so we stay on the same node
            // If someone can't stand this, change it to a while loop instead
            idx--;
         }
      }
   }
};

//==============================================================================
// BTileSpanList::findTileByIndex
//==============================================================================
bool BTileSpanList::findTileByIndex(long n, long *pX, long *pZ) const
{
   if ((pX == NULL) || (pZ == NULL))
      return(false);

   long count = 0;
   long i;
   long num = getNumber();
   for (i=0; i < num; i++)
   {
      long numTiles = (*this)[i].getNumberTiles();
      // if we will pass our target by adding in this span, the hit is somewhere inside
      if (count + numTiles > n)
      {
         *pZ = (*this)[i].getStartZ() + (n - count);
         *pX = (*this)[i].getStartX();
         return(true);
      }
      // otherwise, advance to next span
      else
      {
         count = count + numTiles;
      }
   }

   // not found!
   return(false);
}

void BTileSpanList::buildFromRadius(long radius, long xOffset, long zOffset)
{
   setNumber(0);
   
   long x = 0;
   long z = radius;
   long p = ((5 - radius*4)/4);
   long start, end, t;

   start = -z;
   end = +z;
   t = 0;
   addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));

   while (x < z) 
   {
      x++;
      if (p < 0) 
      {
         start = -z;
         end = z;
         t = -x;
         addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
         t = x;
         addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));

         p += 2*x+1;
      } 
      else 
      {
         start = -x+1;
         end = x-1;
         t = z;
         addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
         t = -z;
         addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));

         z--;

         if (x < z)
         {
            start = -z;
            end = z;
            t = x;
            addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
            t = -x;
            addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
         }

         p += 2*(x-z)+1;
      }
   }

   start = -x;
   end = x;
   t = z;
   addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
   t = -z;
   addSpan(BTileSpan(t+xOffset, start+zOffset, end-start));
}