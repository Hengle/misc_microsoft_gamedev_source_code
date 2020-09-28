//-----------------------------------------------------------------------------
// File: half_float.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef HALF_FLOAT_H
#define HALF_FLOAT_H

#include "math.h"

namespace gr
{
	namespace Half
	{
		inline float IntToFloat(int i)
		{
			return *reinterpret_cast<const float*>(&i);
		}
		
		inline uint FloatToUInt(float f)
		{
			return *reinterpret_cast<const uint*>(&f);
		}
		
		inline ushort FloatToHalf(float f, bool noAbnormalValues = false)
		{
			const uint i = FloatToUInt(f);
			
			const uint sign = 0x8000 & (i >> 16);
			int exp = (255 & (i >> 23)) - 112;
			uint man = 0x7FFFFF & i;
			
			// max. possible exponent value indicates Inf/NaN
			if (143 == exp)	
			{
				if (noAbnormalValues)
					return 0;
					
				if (0 == man)
				{
					// output infinity
					return sign | 0x7C00;
				}
				else 
				{
					// output NaN
					return sign | (man >> 13) | 0x7C00;
				}
			}
			else if (exp <= 0)
			{
				if (noAbnormalValues)
					return 0;
					
				// too small
				if (exp < -10)
					return sign;
									
				// output denormal
				man = (man | 0x800000) >> (1 - exp);
				
				// round
				if (man & 0x1000)
					man += 0x2000;
					
				return sign | (man >> 13);
			}
						
			// round
			if (man & 0x1000)
			{
				man += 0x2000;
				if (man > 0x7FFFFF)
				{
					exp++;
					man = 0;
				}
			}
			
			if (exp > 30)	// was 29, oops!
				return sign | 0x7C00;
				
			return sign | (exp << 10) | (man >> 13);
		}

		inline float HalfToFloat (ushort h, bool noAbnormalValues = false)
		{
			const uint sign = 0x80000000 & ((uint)h << 16);
			uint exp = 31 & (h >> 10);
			uint man = 1023 & h;

			if (exp == 0)
			{
				if (man == 0)	
				{
					// +-0
					return IntToFloat(sign);
				}
				else	
				{
					// normalize denormal
					while (0 == (man & 0x400)) 
					{
						exp--;
						man <<= 1;
					}
					
					exp++;
					man &= ~0x400;
				}
			}
			else if (exp == 31)
			{
			  if (noAbnormalValues)
					return 0;
					
				if (man)
				{
					// NaN
					return IntToFloat(sign | (man << 13) | 0x7F800000); 
				}
				else
				{
					// +-inf
					return IntToFloat(sign | 0x7F800000);
				}
			}
	    							    
			return IntToFloat(sign | ((exp + 112) << 23) | (man << 13));
		}

	} // namespace Half

} // namespace gr

#endif // HALF_FLOAT_H	
