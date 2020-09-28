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

#ifndef NV_TT_COMPRESSDXT_H
#define NV_TT_COMPRESSDXT_H

#include <nvimage/nvimage.h>
#include "nvtt.h"

namespace nv
{
	class Image;
	class FloatImage;

	void doPrecomputation();
	
	// Fast compressors.
	void fastCompressDXT1(const Image * image, const nvtt::OutputOptions & outputOptions);
	void fastCompressDXT3(const Image * image, const nvtt::OutputOptions & outputOptions);
	void fastCompressDXT5(const Image * image, const nvtt::OutputOptions & outputOptions);
	void fastCompressDXT5n(const Image * image, const nvtt::OutputOptions & outputOptions);
	void fastCompressBC4(const Image * image, const nvtt::OutputOptions & outputOptions);
	void fastCompressBC5(const Image * image, const nvtt::OutputOptions & outputOptions);

	// Normal compressors.
	void compressDXT1(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	void compressDXT3(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	void compressDXT5(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	void compressDXT5n(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	void compressBC4(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	void compressBC5(const Image * image, const nvtt::OutputOptions & outputOptions, const nvtt::CompressionOptions::Private & compressionOptions);
	
	// External compressors.
#if defined(HAVE_S3QUANT)
	void s3CompressDXT1(const Image * image, const nvtt::OutputOptions & outputOptions);
#endif
	
#if defined(HAVE_ATITC)
	void atiCompressDXT1(const Image * image, const nvtt::OutputOptions & outputOptions);
#endif

} // nv namespace


#endif // NV_TT_COMPRESSDXT_H
