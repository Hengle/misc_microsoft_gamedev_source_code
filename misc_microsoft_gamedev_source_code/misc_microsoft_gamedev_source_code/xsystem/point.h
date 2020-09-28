#ifndef _POINT_
#define _POINT_

//==============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Point Stuff
//==============================================================================


//==============================================================================
// BPoint
//==============================================================================
//lint -e1927 

class BPoint;
extern BPoint tempPoint;

class BPoint
{
   public:

      // variables
      long x;
      long y;
      long z;

      // constructors
      BPoint() { x = 0; y = 0; z = 0; }
      BPoint(long xcoord, long ycoord, long zcoord) { x = xcoord; y = ycoord; z = zcoord; }
      BPoint(long xcoord, long ycoord) { x = xcoord; y = ycoord; z = 0; }
      BPoint(const BPoint &p) { *this = p; }

      // functions

      void set(long nX, long nY, long nZ) { x = nX; y = nY; z = nZ; }

      // operators
      BPoint      &operator+(const BPoint &pt) 
      { 
         tempPoint.x = x + pt.x;
         tempPoint.y = y + pt.y;
         tempPoint.z = z + pt.z;
         return tempPoint;
      }

      BPoint      &operator-(const BPoint &pt) 
      { 
         tempPoint.x = x - pt.x;
         tempPoint.y = y - pt.y;
         tempPoint.z = z - pt.z;
         return tempPoint;
      }

      BPoint      &operator*(const float scale) 
      { 
         tempPoint.x = long(x * scale);
         tempPoint.y = long(x * scale);
         tempPoint.z = long(x * scale);
         return tempPoint;
      }

      BPoint      &operator/(const float scale) 
      { 
         tempPoint.x = long(x / scale);
         tempPoint.y = long(x / scale);
         tempPoint.z = long(x / scale);
         return tempPoint;
      }

      bool        operator==(const BPoint &pt) 
      {
         if ((x == pt.x) && (y == pt.y) && (z == pt.z))
            return(true);
         else
            return(false);
      }

      bool        operator!=(const BPoint &pt) 
      {
         if ((x != pt.x) || (y != pt.y) || (z != pt.z))
            return(true);
         else
            return(false);
      }

      BPoint      &operator=(const BPoint &pt)
      {
#ifdef _DEBUG
         if (this == &pt)
         {
            BASSERT(0);
            return *this;
         }
#endif
         x = pt.x;
         y = pt.y;
         z = pt.z;
         return(*this);
      }

      float       distance(const BPoint &pt)
      {
         float dx = (float)(pt.x-x);
         float dy = (float)(pt.y-y);
         float dz = (float)(pt.z-z);
         return((float)sqrt(dx*dx + dy*dy + dz*dz));
      }

      float       distanceSquared(const BPoint &pt)
      {
         float dx = (float)(pt.x-x);
         float dy = (float)(pt.y-y);
         float dz = (float)(pt.z-z);
         return(dx*dx+dy*dy+dz*dz);
      }
};

class BRect
{
   public:

      // constructors
      BRect(const BPoint &ul, const BPoint &lr) { mP1 = ul; mP2 = lr ;}
      BRect(const BRect &r) { mP1 = r.mP1; mP2 = r.mP2; }
      BRect(long x1, long y1, long x2, long y2);
      BRect(long x1, long y1, long z1, long x2, long y2, long z2);
      BRect() { mP1.x = 0; mP1.y = 0; mP2.x = 0; mP2.y = 0; }

      BRect    &operator=(const BRect &r) 
      { 
#ifdef _DEBUG
         if (this == &r)
         {
            BASSERT(0);
            return *this;
         }
#endif
         mP1 = r.mP1; 
         mP2 = r.mP2; 
         return *this; 
      }

      // inclusive
      bool     contains(const BPoint &pt) 
      {
         if ((pt.x < mP1.x) || (pt.x > mP2.x) || (pt.y < mP1.y) || (pt.y > mP2.y))
            return(false);
         else
            return(true);
      }

      static bool getIntersection(const BRect &rectA, const BRect &rectB, BRect *rectOut);

      // actual data storage
      BPoint   mP1;
      BPoint   mP2;
};

//==============================================================================
//
//==============================================================================
//lint +e1927 

#endif

