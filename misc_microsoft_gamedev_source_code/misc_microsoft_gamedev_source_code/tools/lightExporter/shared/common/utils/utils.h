//-----------------------------------------------------------------------------
// File: utils.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
// Helper functions.
#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "common/core/core.h"
#include <malloc.h>

namespace gr
{
	namespace Utils
	{
		template<class T> void ClearObj(T& obj);
	  
		template<class T> bool IsAligned(T p, size_t alignment);
		template<class T> T AlignUp(T p, size_t alignment);
		template<class T> int BytesToAlignUp(T p, size_t alignment);
		
		template<class T> void* WriteValue(void* pDst, const T& v);
					 			  
		template<class T>
		struct RelativeOperators
		{   
  		friend bool operator >  (const T& a, const T& b) 
			{ 
					return b < a;		 
			}
  		friend bool operator <= (const T& a, const T& b) 
			{ 
				return !(b < a);  
			}
			friend bool operator >= (const T& a, const T& b) 
			{ 
				return !(a < b);  
			}
			friend bool operator != (const T& a, const T& b) 
			{ 
				return !(a == b); 
			}
		};
		
		template<class Type>
		void DeletePointerVector(Type& vec)
		{
			for (int i = 0; i < vec.size(); i++)
				delete vec[i];
		}
		
		inline int ConvertHexChar(int c);
		
	} // namespace Utils

} // namespace gr

#include "utils.inl"

#endif // UTILS_H

