//==============================================================================
// vectorline.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _VECTORLINE_H_
#define _VECTORLINE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
class BVectorLine
{
   public:
      //Constructors and Destructor.
      BVectorLine( void );
      ~BVectorLine( void );

      //Point methods.
      long                       getNumberPoints( void ) const { return(mPoints.getNumber()); }
      const BVector              *getPoints( void ) const { return(mPoints.getPtr()); }
      const BVector              &getPoint( long index ) const;
      long                       getPointIndex( const BVector &p ) const;
      bool                       addPointAtStart( const BVector &p );
      bool                       addPointBefore( long index, const BVector &p );
      bool                       addPointAfter( long index, const BVector &p );
      bool                       addPointAtEnd( const BVector &p );
      bool                       addPointAtEnd( float x, float y, float z );
      bool                       splitSegment( long segmentNumber );
      bool                       setPoint( long index, const BVector &p );
      bool                       setPoints( const BVector *points, long numPoints );
      bool                       removePoint( long index );
      bool                       removePoint( const BVector &p );
      void                       offsetPoints( const BVector &o, long startIndex );

      //Length lookup.  Length is recalculated everytime the line is changed.
      float                      getLength( void ) const { return(mLength); }

      //Misc.
      bool                       getXZOnly( void ) const { return(mXZOnly); }
      void                       setXZOnly( bool v ) { mXZOnly=v; }
      bool                       getJoinEnds( void ) const { return(mJoinEnds); }
      void                       setJoinEnds( bool v ) { mJoinEnds=v; }
      long                       getSegment(const BVector &p) const {return getPointIndex(p);}
      void                       getSegmentDir(long segmentIndex, BVector& direction, bool normalize = true) const;
      void                       getDirection(long segmentStart, long segmentEnd, BVector& direction, bool normalize = true) const;

      //"Follow" methods.
      const BVector&             getCurrentPoint( void ) const { return(mCurrentPoint); }
      const BVector&             getCurrentDir(void) const { return(mCurrentDir); }
      float                      getCurrentLength( void ) const { return(mCurrentLength); }
      float                      getCurrentPercentage( void ) const { if (mLength < cFloatCompareEpsilon) return(0.0f); return(mCurrentLength/mLength); }
      float                      getRemainingLength( void ) const { return(mLength-mCurrentLength); }
      bool                       startFollow( float length );
      bool                       moveForward( float length );

      //Cleanup.
      void                       cleanUp( void );

   protected:
      bool                       setCurrentPoint( float totalLength );
      float                      calculateLength( void );

      BDynamicSimArray<BVector>      mPoints;
      float                      mLength;
      bool                       mXZOnly;
      bool                       mJoinEnds;
      BVector                    mCurrentPoint;
      BVector                    mCurrentDir;
      float                      mCurrentLength;
      float                      mInternalLength;
};


//==============================================================================
#endif // _VECTORLINE_H_

//==============================================================================
// eof: vectorline.h
//==============================================================================
