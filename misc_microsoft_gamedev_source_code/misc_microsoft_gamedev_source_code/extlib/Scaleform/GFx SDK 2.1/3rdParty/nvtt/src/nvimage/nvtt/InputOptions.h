// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NV_TT_INPUTOPTIONS_H
#define NV_TT_INPUTOPTIONS_H

#include <nvmath/Vector.h>
#include <nvimage/Image.h>
#include "nvtt.h"

namespace nvtt
{

	struct InputOptions::Private
	{
		Private() : images(NULL) {}

		WrapMode wrapMode;
		TextureType textureType;
		InputFormat inputFormat;
		
		int faceCount;
		int mipmapCount;
		int imageCount;
		
		struct Image;
		Image * images;

		// Quantization.
		bool enableColorDithering;
		bool enableAlphaDithering;
		bool binaryAlpha;
		int alphaThreshold;			// reference value used for binary alpha quantization.

		bool alphaTransparency;	// set to true if alpha is used for transparency.
		
		// Gamma conversion.
		float inputGamma;
		float outputGamma;
		
		// Mipmap generation options.
		bool generateMipmaps;
		int maxLevel;
		MipmapFilter mipmapFilter;
		
		// Normal map options.
		bool normalMap;
		bool normalizeMipmaps;
		bool convertToNormalMap;
		nv::Vector4 heightFactors;
		nv::Vector4 bumpFrequencyScale;
	};

	// Internal image structure.
	struct InputOptions::Private::Image
	{
		Image() {}
		~Image() { delete data; }
		
		int mipLevel;
		int face;
		
		int width;
		int height;
		int depth;
		
		nv::Image * data;
	};

} // nvtt namespace

#endif // NV_TT_INPUTOPTIONS_H
