// ========================================================================
// $File: //jeffr/granny/rt/granny_find_knot.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #1 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_FIND_KNOT_H)
#include "granny_find_knot.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;
/*
  Note: the FindKnot routines return the knot that's _ahead of_ where you
  are at time t. However, it clamps to KnotCount-1, so it will never reference
  off the end of the array.
 */

int32x GRANNY
FindKnot(int32x KnotCount, real32 const *Knots, real32 t)
{
#if 1
#if DEBUG
    real32 const *OriginalKnots = Knots;
#endif
    int32x KnotWindow = KnotCount;
    int32x KnotIndex = 0;

    while(KnotWindow > 1)
    {
        if(KnotWindow & 1)
        {
            // It's odd
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                KnotIndex += KnotWindow;
                Knots += KnotWindow;
            }

            ++KnotWindow;
        }
        else
        {
            // It's even
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                if((KnotIndex+KnotWindow+1 >= KnotCount) || (t < Knots[KnotWindow + 1]))
                {
                    KnotIndex += KnotWindow;
                    Knots += KnotWindow;
                    break;
                }
                else
                {
                    KnotIndex += KnotWindow + 1;
                    Knots += KnotWindow + 1;
                    --KnotWindow;
                }
            }
        }
    }

    // Slight adjustment for duplicate knots.
    while ( ( KnotIndex < KnotCount ) && ( Knots[0] <= t ) )
    {
        ++KnotIndex;
        ++Knots;
    }

#if DEBUG
    Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
    if ( KnotIndex < KnotCount )
    {
        Assert ( OriginalKnots[KnotIndex] > t );
    }
    if ( KnotIndex > 0 )
    {
        Assert ( OriginalKnots[KnotIndex-1] <= t );
    }
#endif

    if ( ( KnotIndex == KnotCount ) && ( KnotIndex > 0 ) )
    {
        // Stop it falling off the end of the knot array.
        // This is slightly artificial, but calling code gets confused otherwise.
        KnotIndex--;
    }

    return(KnotIndex);



#else
    uint32 i = 1 << (Log2KnotCount - 1);
    switch(Log2KnotCount - 1)
    {
#define Level(N)                                \
        case N:                                 \
        if((i >= KnotCount) || (Knots[i] < t))  \
        {                                       \
            i -= 1 << (N - 1);                  \
        }                                       \
        else                                    \
        {                                       \
            i += 1 << (N - 1);                  \
        }

#define Level0
        case 0:

        break;

#if (MaximumBSplineKnotPower >= 31)
        Level(31);
#endif

#if (MaximumBSplineKnotPower >= 30)
        Level(30);
#endif

#if (MaximumBSplineKnotPower >= 29)
        Level(29);
#endif

#if (MaximumBSplineKnotPower >= 28)
        Level(28);
#endif

#if (MaximumBSplineKnotPower >= 27)
        Level(27);
#endif

#if (MaximumBSplineKnotPower >= 26)
        Level(26);
#endif

#if (MaximumBSplineKnotPower >= 25)
        Level(25);
#endif

#if (MaximumBSplineKnotPower >= 24)
        Level(24);
#endif

#if (MaximumBSplineKnotPower >= 23)
        Level(23);
#endif

#if (MaximumBSplineKnotPower >= 22)
        Level(22);
#endif

#if (MaximumBSplineKnotPower >= 21)
        Level(21);
#endif

#if (MaximumBSplineKnotPower >= 20)
        Level(20);
#endif

#if (MaximumBSplineKnotPower >= 19)
        Level(19);
#endif

#if (MaximumBSplineKnotPower >= 18)
        Level(18);
#endif

#if (MaximumBSplineKnotPower >= 17)
        Level(17);
#endif

#if (MaximumBSplineKnotPower >= 16)
        Level(16);
#endif

#if (MaximumBSplineKnotPower >= 15)
        Level(15);
#endif

#if (MaximumBSplineKnotPower >= 14)
        Level(14);
#endif

#if (MaximumBSplineKnotPower >= 13)
        Level(13);
#endif

#if (MaximumBSplineKnotPower >= 12)
        Level(12);
#endif

#if (MaximumBSplineKnotPower >= 11)
        Level(11);
#endif

#if (MaximumBSplineKnotPower >= 10)
        Level(10);
#endif

#if (MaximumBSplineKnotPower >= 9)
        Level(9);
#endif

#if (MaximumBSplineKnotPower >= 8)
        Level(8);
#endif

#if (MaximumBSplineKnotPower >= 7)
        Level(7);
#endif

#if (MaximumBSplineKnotPower >= 6)
        Level(6);
#endif

#if (MaximumBSplineKnotPower >= 5)
        Level(5);
#endif

#if (MaximumBSplineKnotPower >= 4)
        Level(4);
#endif

#if (MaximumBSplineKnotPower >= 3)
        Level(3);
#endif

#if (MaximumBSplineKnotPower >= 2)
        Level(2);
#endif

#if (MaximumBSplineKnotPower >= 1)
        Level(1);
#endif

        Level0;

        default:
        {
            InvalidCodePath();
        } break;
    }
#endif
}


#if 0   // Ugh.
int32x GRANNY
FindCloseKnot(int32x KnotCount, real32 const *Knots,
              real32 t, int32x StartingIndex)
{
    if(KnotCount > 0)
    {
        int32x KnotIndex = StartingIndex;
        if(KnotIndex == 0)
        {
            ++KnotIndex;
        }

        real32 PrevT = Knots[KnotIndex - 1];
        while(KnotIndex < KnotCount)
        {
            real32 ThisT = Knots[KnotIndex];
            if((PrevT <= t) && (ThisT > t))
            {
#if DEBUG
                Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
                if ( KnotIndex < KnotCount )
                {
                    Assert ( Knots[KnotIndex] > t );
                }
                if ( KnotIndex > 0 )
                {
                    Assert ( Knots[KnotIndex-1] <= t );
                }
#endif
                return(KnotIndex);
            }

            PrevT = ThisT;
            ++KnotIndex;
        }

        KnotIndex = 1;
        while(KnotIndex < StartingIndex)
        {
            if(Knots[KnotIndex] >= t)
            {
#if DEBUG
                Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
                if ( KnotIndex < KnotCount )
                {
                    Assert ( Knots[KnotIndex] > t );
                }
                if ( KnotIndex > 0 )
                {
                    Assert ( Knots[KnotIndex-1] <= t );
                }
#endif
                return(KnotIndex);
            }

            ++KnotIndex;
        }
    }

    return(0);
}
#else
int32x GRANNY
FindCloseKnot(int32x KnotCount, real32 const *Knots,
              real32 t, int32x StartingIndex)
{
    return FindKnot ( KnotCount, Knots, t );
}
#endif




// Identical to FindKnot, except with the function declaration changed!
int32x GRANNY
FindKnotUint16(int32x KnotCount, uint16 const *Knots, uint16 t)
{
#if DEBUG
    uint16 const *OriginalKnots = Knots;
#endif
    int32x KnotWindow = KnotCount;
    int32x KnotIndex = 0;

    while(KnotWindow > 1)
    {
        if(KnotWindow & 1)
        {
            // It's odd
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                KnotIndex += KnotWindow;
                Knots += KnotWindow;
            }

            ++KnotWindow;
        }
        else
        {
            // It's even
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                if((KnotIndex+KnotWindow+1 >= KnotCount) || (t < Knots[KnotWindow + 1]))
                {
                    KnotIndex += KnotWindow;
                    Knots += KnotWindow;
                    break;
                }
                else
                {
                    KnotIndex += KnotWindow + 1;
                    Knots += KnotWindow + 1;
                    --KnotWindow;
                }
            }
        }
    }

    // Slight adjustment for duplicate knots.
    while ( ( KnotIndex < KnotCount ) && ( Knots[0] <= t ) )
    {
        ++KnotIndex;
        ++Knots;
    }

#if DEBUG
    Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
    if ( KnotIndex < KnotCount )
    {
        Assert ( OriginalKnots[KnotIndex] > t );
    }
    if ( KnotIndex > 0 )
    {
        Assert ( OriginalKnots[KnotIndex-1] <= t );
    }
#endif

    if ( ( KnotIndex == KnotCount ) && ( KnotIndex > 0 ) )
    {
        // Stop it falling off the end of the knot array.
        // This is slightly artificial, but calling code gets confused otherwise.
        KnotIndex--;
    }

    return(KnotIndex);
}


// Identical to FindKnot, except with the function declaration changed!
int32x GRANNY
FindKnotUint8(int32x KnotCount, uint8 const *Knots, uint8 t)
{
#if DEBUG
    uint8 const *OriginalKnots = Knots;
#endif
    int32x KnotWindow = KnotCount;
    int32x KnotIndex = 0;

    while(KnotWindow > 1)
    {
        if(KnotWindow & 1)
        {
            // It's odd
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t > Knots[KnotWindow])
            {
                KnotIndex += KnotWindow;
                Knots += KnotWindow;
            }

            ++KnotWindow;
        }
        else
        {
            // It's even
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t > Knots[KnotWindow])
            {
                if((KnotIndex+KnotWindow+1 >= KnotCount) || (t <= Knots[KnotWindow + 1]))
                {
                    KnotIndex += KnotWindow;
                    Knots += KnotWindow;
                    break;
                }
                else
                {
                    KnotIndex += KnotWindow + 1;
                    Knots += KnotWindow + 1;
                    --KnotWindow;
                }
            }
        }
    }

    // Slight adjustment for duplicate knots.
    while ( ( KnotIndex < KnotCount ) && ( Knots[0] <= t ) )
    {
        ++KnotIndex;
        ++Knots;
    }

#if DEBUG
    Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
    if ( KnotIndex < KnotCount )
    {
        Assert ( OriginalKnots[KnotIndex] > t );
    }
    if ( KnotIndex > 0 )
    {
        Assert ( OriginalKnots[KnotIndex-1] <= t );
    }
#endif

    if ( ( KnotIndex == KnotCount ) && ( KnotIndex > 0 ) )
    {
        // Stop it falling off the end of the knot array.
        // This is slightly artificial, but calling code gets confused otherwise.
        KnotIndex--;
    }

    return(KnotIndex);
}



int32x GRANNY
FindCloseKnotUint16(int32x KnotCount, uint16 const *Knots,
                    uint16 t, int32x StartingIndex)
{
    return FindKnotUint16 ( KnotCount, Knots, t );
}

int32x GRANNY
FindCloseKnotUint8(int32x KnotCount, uint8 const *Knots,
                    uint8 t, int32x StartingIndex)
{
    return FindKnotUint8 ( KnotCount, Knots, t );
}



