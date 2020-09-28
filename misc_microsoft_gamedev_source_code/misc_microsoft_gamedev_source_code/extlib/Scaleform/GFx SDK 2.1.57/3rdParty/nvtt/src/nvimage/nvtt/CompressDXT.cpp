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

#include <nvcore/Memory.h>

#include <nvimage/Image.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>

#include "nvtt.h"
#include "CompressDXT.h"
#include "FastCompressDXT.h"
#include "CompressionOptions.h"

// squish
#include "squish/colourset.h"
//#include "squish/clusterfit.h"
#include "squish/fastclusterfit.h"
#include "squish/weightedclusterfit.h"

// s3_quant
#if defined(HAVE_S3QUANT)
#include "s3tc/s3_quant.h"
#endif

// ati tc
#if defined(HAVE_ATITC)
#include "atitc/ATI_Compress.h"
#endif

//#include <time.h>

using namespace nv;
using namespace nvtt;


void nv::fastCompressDXT1(const Image * image, const OutputOptions & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			
			compressBlock_BoundsRange(rgba, &block);

			// @@ Use iterative optimization.
			optimizeEndPoints(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT3(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT3 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			compressBlock_BoundsRange(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT5(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			compressBlock_BoundsRange(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT5n(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			
			// copy X coordinate to alpha channel and Y coordinate to green channel.
			rgba.swizzleDXT5n();
			
			compressBlock_BoundsRange(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressBC4(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	// @@ TODO
	// compress red channel (X)
}


void nv::fastCompressBC5(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	// @@ TODO
	// compress red, green channels (X,Y)
}


void nv::doPrecomputation()
{
	static bool done = false;
	
	if (!done)
	{
		done = true;
		squish::FastClusterFit::doPrecomputation();
	}
}


void nv::compressDXT1(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	doPrecomputation();

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), 0);
			squish::FastClusterFit fit(&colours, squish::kDxt1);
			//squish::WeightedClusterFit fit(&colours, squish::kDxt1);
			//squish::ClusterFit fit(&colours, squish::kDxt1);
			fit.setMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());
			fit.Compress(&block);
			
			// @@ Use iterative cluster fit algorithm to improve error in highest quality mode.
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressDXT3(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT3 block;
	
	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// Compress explicit alpha.
			compressBlock(rgba, &block.alpha);
			
			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), squish::kWeightColourByAlpha);
			squish::WeightedClusterFit fit(&colours, 0);
			fit.setMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());
			fit.Compress(&block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}

void nv::compressDXT5(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;
	
	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);

			// Compress alpha.
			uint error;
			if (compressionOptions.quality == Quality_Highest)
			{
				error = compressBlock_BruteForce(rgba, &block.alpha);
			}
			else
			{
				error = compressBlock_Iterative(rgba, &block.alpha);
			}

			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), squish::kWeightColourByAlpha);
			squish::WeightedClusterFit fit(&colours, 0);
			fit.setMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());
			fit.Compress(&block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressDXT5n(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;
	
	doPrecomputation();

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// copy X coordinate to green channel and Y coordinate to alpha channel.
			rgba.swizzleDXT5n();			
			
			// Compress X.
			uint error = compressBlock_Iterative(rgba, &block.alpha);
			if (compressionOptions.quality == Quality_Highest)
			{
				error = compressBlock_BruteForce(rgba, &block.alpha);
			}
			
			// Compress Y.
 			compressGreenBlock_BruteForce(rgba, &block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressBC4(const Image * image, const nvtt::OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	// threshold should be from [0 - 1] but may also be higher...
	const uint threshold = uint(compressionOptions.errorThreshold * 256);
	
	ColorBlock rgba;
	AlphaBlockDXT5 block;
	
	uint totalError = 0;
	
	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);

			//error = compressBlock_BoundsRange(rgba, &block);
			uint error = compressBlock_Iterative(rgba, &block);

			if (compressionOptions.quality == Quality_Highest ||
				(compressionOptions.quality == Quality_Production && error > threshold))
			{
				// Try brute force algorithm.
				error = compressBlock_BruteForce(rgba, &block);
			}

			totalError += error;

			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}

	// @@ All the compressors should work like this.
	// Effect of adjusting threshold: 
	// (threshold: error - time)
	// 0: 4.29 - 1.83
	// 32: 4.32 - 1.77
	// 48: 4.37 - 1.72
	// 64: 4.43 - 1.45
	// 74: 4.45 - 1.35
	// 92: 4.54 - 1.15
	// 128: 4.67 - 0.79
	// 256: 4.92 - 0.20
	// inf: 4.98 - 0.09
	//printf("Alpha error: %f\n", float(totalError) / (w*h));
}


void nv::compressBC5(const Image * image, const nvtt::OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();

	ColorBlock xcolor;
	ColorBlock ycolor;

	BlockATI2 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			xcolor.init(image, x, y);
			xcolor.splatX();
			
			ycolor.init(image, x, y);
			ycolor.splatY();

			// @@ Compute normal error, instead of separate xy errors.
			uint xerror, yerror;
			
			if (compressionOptions.quality == Quality_Highest)
			{
				xerror = compressBlock_BruteForce(xcolor, &block.x);
				yerror = compressBlock_BruteForce(ycolor, &block.y);
			}
			else
			{
				xerror = compressBlock_Iterative(xcolor, &block.x);
				yerror = compressBlock_Iterative(ycolor, &block.y);
			}

			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


#if defined(HAVE_S3QUANT)

void nv::s3CompressDXT1(const Image * image, const nvtt::OutputOptions & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	float error = 0.0f;

	BlockDXT1 dxtBlock3;
	BlockDXT1 dxtBlock4;
	ColorBlock block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			block.init(image, x, y);

			// Init rgb block.
			RGBBlock rgbBlock;
			rgbBlock.n = 16;
			for (uint i = 0; i < 16; i++) {
				rgbBlock.colorChannel[i][0] = clamp(float(block.color(i).r) / 255.0f, 0.0f, 1.0f);
				rgbBlock.colorChannel[i][1] = clamp(float(block.color(i).g) / 255.0f, 0.0f, 1.0f);
				rgbBlock.colorChannel[i][2] = clamp(float(block.color(i).b) / 255.0f, 0.0f, 1.0f);
			}
			rgbBlock.weight[0] = 1.0f;
			rgbBlock.weight[1] = 1.0f;
			rgbBlock.weight[2] = 1.0f;

			rgbBlock.inLevel = 4;
			CodeRGBBlock(&rgbBlock);

			// Copy results to DXT block.
			dxtBlock4.col0.r = rgbBlock.endPoint[0][0];
			dxtBlock4.col0.g = rgbBlock.endPoint[0][1];
			dxtBlock4.col0.b = rgbBlock.endPoint[0][2];

			dxtBlock4.col1.r = rgbBlock.endPoint[1][0];
			dxtBlock4.col1.g = rgbBlock.endPoint[1][1];
			dxtBlock4.col1.b = rgbBlock.endPoint[1][2];

			dxtBlock4.setIndices(rgbBlock.index);

			if (dxtBlock4.col0.u < dxtBlock4.col1.u) {
				swap(dxtBlock4.col0.u, dxtBlock4.col1.u);
				dxtBlock4.indices ^= 0x55555555;
			}

			uint error4 = blockError(block, dxtBlock4);

			rgbBlock.inLevel = 3;

			CodeRGBBlock(&rgbBlock);

			// Copy results to DXT block.
			dxtBlock3.col0.r = rgbBlock.endPoint[0][0];
			dxtBlock3.col0.g = rgbBlock.endPoint[0][1];
			dxtBlock3.col0.b = rgbBlock.endPoint[0][2];

			dxtBlock3.col1.r = rgbBlock.endPoint[1][0];
			dxtBlock3.col1.g = rgbBlock.endPoint[1][1];
			dxtBlock3.col1.b = rgbBlock.endPoint[1][2];

			dxtBlock3.setIndices(rgbBlock.index);

			if (dxtBlock3.col0.u > dxtBlock3.col1.u) {
				swap(dxtBlock3.col0.u, dxtBlock3.col1.u);
				dxtBlock3.indices ^= (~dxtBlock3.indices  >> 1) & 0x55555555;
			}

			uint error3 = blockError(block, dxtBlock3);

			if (error3 < error4) {
				error += error3;

				if (outputOptions.outputHandler != NULL) {
					outputOptions.outputHandler->writeData(&dxtBlock3, sizeof(dxtBlock3));
				}
			}
			else {
				error += error4;

				if (outputOptions.outputHandler != NULL) {
					outputOptions.outputHandler->writeData(&dxtBlock4, sizeof(dxtBlock4));
				}
			}
		}
	}

	printf("error = %f\n", error/((w+3)/4 * (h+3)/4));
}

#endif // defined(HAVE_S3QUANT)


#if defined(HAVE_ATITC)

void nv::atiCompressDXT1(const Image * image, const OutputOptions & outputOptions)
{
	// Init source texture
	ATI_TC_Texture srcTexture;
	srcTexture.dwSize = sizeof(srcTexture);
	srcTexture.dwWidth = image->width();
	srcTexture.dwHeight = image->height();
	srcTexture.dwPitch = image->width() * 4;
	srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
	srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
	srcTexture.pData = (ATI_TC_BYTE*) image->pixels();

	// Init dest texture
	ATI_TC_Texture destTexture;
	destTexture.dwSize = sizeof(destTexture);
	destTexture.dwWidth = image->width();
	destTexture.dwHeight = image->height();
	destTexture.dwPitch = 0;
	destTexture.format = ATI_TC_FORMAT_DXT1;
	destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
	destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

	// Compress
	ATI_TC_ConvertTexture(&srcTexture, &destTexture, NULL, NULL, NULL, NULL);

	if (outputOptions.outputHandler != NULL) {
		outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
	}
}

#endif // defined(HAVE_ATITC)
