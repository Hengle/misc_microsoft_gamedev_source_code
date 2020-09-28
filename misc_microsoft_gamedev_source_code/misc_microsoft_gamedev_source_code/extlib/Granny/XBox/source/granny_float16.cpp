// ========================================================================
// $File: //jeffr/granny/rt/granny_float16.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #5 $
//
// See the header for some comments on this format conversion.
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_FLOAT16_H)
#include "granny_float16.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode DataTypeLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

const uint16 UnsignedHalfInfValue = 0x7C00;
const uint16 UnsignedHalfNaNBase  = 0x7C00;  // needs non-zero mantissa bits...

END_GRANNY_NAMESPACE;


real16 GRANNY
Real32ToReal16(real32 Value)
{
    // Convert the input float to an integer in a way that won't confuse compilers that
    // assume no type aliasing.
    union { real32 f; uint32 i; } Converter;
    Converter.f = Value;
    uint32 IntValue = Converter.i;

    // Extract sign, exponent and mantissa f32 = 1:8:23
    //  We can just put the sign in the right place, since we'll be preserving it, of
    //  course.
    uint32 Sign = (IntValue >> 16) & 0x8000;
    int32x Exp  = ((IntValue >> 23) & 0xFF) - 127;  // account for bias
    int32x Mant = IntValue & ((1 << 23) - 1);

    // Rebias the exponent for storage in the half value.  We'll use this to determine
    // which case we're handling.
    //
    // BiasExp:  > 30 = Inf or NaN
    //          < -10 = too small to represent as denorm half, clamp to 0.0
    //          <=  0 = represent as denorm half
    //      otherwise = normalized half value

    int32x BiasExp = Exp + 15;
    if (BiasExp > 30)
    {
        // Check the original exponent to see if this needs to turn into a NaN value
        if (Exp == (0xFF - 127) && Mant != 0)
        {
            // NaN = Exp of 0xFF, non-zero mantissa.  Create a NaN half value with the
            // correct sign.  We need to preserve the mantissa bits to keep any flag
            // values people may care about in place.  Note that there is a special case,
            // if the top 10 bits of the 23-bit mantissa are 0, we need to stuff a made-up
            // non-zero value in there to keep the value from becoming an Inf.
            int32x NaNMant = Mant >> 13;
            if (NaNMant == 0x0)
                NaNMant = 0x1;

            return real16(Sign | UnsignedHalfNaNBase | NaNMant);
        }
        else
        {
            // Inf, with matching sign
            return real16(Sign | UnsignedHalfInfValue);
        }
    }
    else if (BiasExp < -10)
    {
        // Value is too small to represent as half, flush to zero, maintain sign bit
        return real16(Sign | 0x0);
    }
    else if (BiasExp <= 0)
    {
        // We'll be returning this as a denorm half. We need to paste the implied "1" onto
        // the mantissa, and shift it down to account for the exponent.  In addition, we
        // need to perform the rounding in the least significant digit.  More on that
        // later.
        int32x DenormMant = Mant | (1 << 23);
        DenormMant >>= (1 - BiasExp);

        // Ok, do the rounding.  Note that overflow here is ok, what will happen is that
        // if the value is all ones in the stored part of the mantissa, overflow will
        // convert that to all zeros in the mantissa, with an exponent value of 1, which
        // is the correct normalized float that is the first value beyond the denorm
        // range.
        //
        // We're throwing away 13 bits (23 - 10), so we can just add (1 << 12) to
        // accomplish the correct rounding.
        DenormMant += (1 << 12);

        // Return with the correct sign bit, shifting the mantissa into the correct
        // position.
        return real16(Sign | (DenormMant >> 13));
    }
    else
    {
        // BiasExp is in the range [1, 30], which are all normal half values.  We need to
        // perform rounding, before shearing off the unused mantissa bits.  Note that
        // overflow in the mantissa in the case Exp = 30, Mant = 1111... will take the
        // output value to an Inf, so that's correctly handled.

        // We're throwing away 13 bits (23 - 10), so we can just add (1 << 12) to
        // accomplish the correct rounding.
        int NewMant = Mant + (1 << 12);
        if (NewMant & (1 << 23))
        {
            // Overflow
            NewMant = 0;
            ++BiasExp;
        }

        return real16(Sign | (BiasExp << 10) | (NewMant >> 13));
    }
}

void GRANNY
Real16ToReal32(real16 Value, real32* Output)
{
    CheckPointerNotNull(Output, return);

    // real16's are a typedef of uint16 already, so we can just pull bits out of them
    // without conversion hassle
    uint32 Sign = (Value & 0x8000) << 16;
    int32x Exp  = ((Value >> 10) & 0x1f); // leave the bias alone for now
    int32x Mant = Value & ((1 << 10) - 1);

    // Return value converter
    uint32 Result;

    if (Exp == 0)
    {
        // Zero?
        if (Mant == 0)
        {
            Result = (Sign | 0x0);
        }
        else
        {
            // Denorm half representation.  Convert to a normalized float, any denorm half can
            // be represented as a valid, normalized real32.
            int32x ExtraBias = 0;
            int32x NormMant  = Mant;
            while ((NormMant & (1 << 10)) == 0)
            {
                ++ExtraBias;
                NormMant <<= 1;
            }

            // Mask off the implied "1"
            NormMant &= ~(1 << 10);

            // Ok, create the new exponent
            int32x BiasExp = (-15 - ExtraBias + 1) + 127;
            Assert(BiasExp > 0 && BiasExp < 0xFF);

            // And the new mantissa.  No replication necessary, all 0 bits in non-signified
            // locations.
            Result = (Sign | (BiasExp << 23) | (NormMant << 13));
        }

    }
    else if (Exp == 31)
    {
        // Either an Inf or a NaN value, depending on the Mantissa.  Just or in the
        // shifted mantissa, it will behave correctly...
        Result = (Sign | (0xFF << 23) | (Mant << 13));
    }
    else
    {
        // Normalized half value.  We need to rebias the exponent to the real32 range.
        // Zero the lower bits of the new mantissa.
        int32x BiasExp = (Exp - 15) + 127;
        Assert(BiasExp > 0 && BiasExp < 0xFF);

        Result = (Sign | (BiasExp << 23) | (Mant << 13));
    }

    // We have to memcpy this, rather than returning by value because we want to preserve
    // the signalling bits of the NaNs...
    Copy32(1, &Result, Output);
}
