//-----------------------------------------------------------------------------
// File: vector_font.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef VECTOR_FONT_H
#define VECTOR_FONT_H

#include "common/math/vector.h"

namespace gr
{
	struct VectorFont
	{
		static void textOutInit(void);
		static void textOutDeinit(void);

		static void textOutModelSpace(
			const char* pStr, 
			const Vec3& modelPos, 
			const Vec3& modelUp,
			const Vec3& modelRight, 
			const uint color = 0xffffffff);

		static void textOutScreenSpace(
			const char* pStr, 
			const Vec2& ScreenPos, 
			const Vec2& ScreenUp,
			const Vec2& ScreenRight, 
			const uint color = 0xffffffff);

		static void renderLineInit(void);
		static void renderLineDeinit(void);

		static void renderLine(
			const Vec3& s, 
			const Vec3& e,
			const uint color = 0xffffffff);
	};
  
} // namespace gr

#endif // VECTOR_FONT_H
