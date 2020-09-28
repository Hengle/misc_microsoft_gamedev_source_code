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

#include <nvcore/Debug.h>
#include <nvcore/Containers.h>
#include <nvmath/Color.h>
#include <nvmath/Fitting.h>
#include <nvimage/Image.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/nvtt/CompressionOptions.h>
#include <nvimage/nvtt/FastCompressDXT.h>

#include "CudaCompressDXT.h"
#include "CudaUtils.h"


#if defined HAVE_CUDA
#include <cuda_runtime.h>
#endif

#include <time.h>
#include <stdio.h>

using namespace nv;
using namespace nvtt;

#if defined HAVE_CUDA

extern "C" void setupCompressKernel(const float weights[3]);
extern "C" void compressKernel(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);
extern "C" void compressWeightedKernel(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps);

static uint * d_bitmaps = NULL;

static void doPrecomputation()
{
	if (d_bitmaps != NULL) {
		return;
	}

	uint bitmaps[1024];

	int indices[16];
	int num = 0;

	// Compute bitmaps with 3 clusters:

	// first cluster [0,i) is at the start
	for( int m = 0; m < 16; ++m )
	{
		indices[m] = 0;
	}
	const int imax = 15;
	for( int i = imax; i >= 0; --i )
	{
		// second cluster [i,j) is half along
		for( int m = i; m < 16; ++m )
		{
			indices[m] = 2;
		}
		const int jmax = ( i == 0 ) ? 15 : 16;
		for( int j = jmax; j >= i; --j )
		{
			// last cluster [j,k) is at the end
			if( j < 16 )
			{
				indices[j] = 1;
			}

			uint bitmap = 0;
			
			for(int p = 0; p < 16; p++) {
				bitmap |= indices[p] << (p * 2);
			}
				
			bitmaps[num] = bitmap;
			
			num++;
		}
	}
	nvDebugCheck(num == 151);

	// Align to 160.
	for(int i = 0; i < 9; i++)
	{
		bitmaps[num] = 0x555AA000;
		num++;
	}
	nvDebugCheck(num == 160);

	// Append bitmaps with 4 clusters:

	// first cluster [0,i) is at the start
	for( int m = 0; m < 16; ++m )
	{
		indices[m] = 0;
	}
	for( int i = imax; i >= 0; --i )
	{
		// second cluster [i,j) is one third along
		for( int m = i; m < 16; ++m )
		{
			indices[m] = 2;
		}
		const int jmax = ( i == 0 ) ? 15 : 16;
		for( int j = jmax; j >= i; --j )
		{
			// third cluster [j,k) is two thirds along
			for( int m = j; m < 16; ++m )
			{
				indices[m] = 3;
			}

			int kmax = ( j == 0 ) ? 15 : 16;
			for( int k = kmax; k >= j; --k )
			{
				// last cluster [k,n) is at the end
				if( k < 16 )
				{
					indices[k] = 1;
				}
				
				uint bitmap = 0;

				bool hasThree = false;
				for(int p = 0; p < 16; p++) {
					bitmap |= indices[p] << (p * 2);

					if (indices[p] == 3) hasThree = true;
				}
				
				if (hasThree) {
					bitmaps[num] = bitmap;
					num++;
				}
			}
		}
	}
	nvDebugCheck(num == 975);
	
	// Align to 1024.
	for(int i = 0; i < 49; i++)
	{
		bitmaps[num] = 0x555AA000;
		num++;
	}

	nvDebugCheck(num == 1024);

	/*
	printf("uint bitmaps[1024] = {\n");
	for (int i = 0; i < 1024; i++)
	{
		printf("\t0x%.8X,\n", bitmaps[i]);
	}
	printf("};\n");
	*/

    // Upload bitmaps.
    cudaMalloc((void**) &d_bitmaps, 1024 * sizeof(uint));
    cudaMemcpy(d_bitmaps, bitmaps, 1024 * sizeof(uint), cudaMemcpyHostToDevice);

	// @@ Check for errors.

	// @@ Free allocated memory.
}

#endif

// Convert linear image to block linear.
static void convertToBlockLinear(const Image * image, uint * blockLinearImage)
{
	const uint w = (image->width() + 3) / 4;
	const uint h = (image->height() + 3) / 4;

	for(uint by = 0; by < h; by++) {
		for(uint bx = 0; bx < w; bx++) {
			const uint bw = min(image->width() - bx * 4, 4U);
			const uint bh = min(image->height() - by * 4, 4U);

			for (uint i = 0; i < 16; i++) {
				const int x = (i % 4) % bw;
				const int y = (i / 4) % bh;
				blockLinearImage[(by * w + bx) * 16 + i] = image->pixel(bx * 4 + x, by * 4 + y).u;
			}
		}
	}
}

// @@ This code is very repetitive and needs to be cleaned up.


/// Compress image using CUDA.
void nv::cudaCompressDXT1(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	doPrecomputation();

	// Image size in blocks.
	const uint w = (image->width() + 3) / 4;
	const uint h = (image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(image, blockLinearImage);

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;
	const uint blockMax = 32768; // 49152, 65535

    // Allocate image in device memory.
    uint * d_data = NULL;
    cudaMalloc((void**) &d_data, min(imageSize, blockMax * 64U));

	// Allocate result.
    uint * d_result = NULL;
    cudaMalloc((void**) &d_result, min(compressedSize, blockMax * 8U));

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	clock_t start = clock();

	// TODO: Add support for multiple GPUs.
	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, blockMax);

	    cudaMemcpy(d_data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		compressKernel(count, d_data, d_result, d_bitmaps);

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));

			if (outputOptions.errorHandler != NULL)
			{
				outputOptions.errorHandler->error(Error_CudaError);
			}
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, d_result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(blockLinearImage, count * 8);
		}

		bn += count;
	}

	clock_t end = clock();
	printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(blockLinearImage);
	cudaFree(d_data);
	cudaFree(d_result);

#else
	if (outputOptions.errorHandler != NULL)
	{
		outputOptions.errorHandler->error(Error_CudaError);
	}
#endif
}


/// Compress image using CUDA.
void nv::cudaCompressDXT3(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	doPrecomputation();

	// Image size in blocks.
	const uint w = (image->width() + 3) / 4;
	const uint h = (image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(image, blockLinearImage);

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;
	const uint blockMax = 32768; // 49152, 65535

    // Allocate image in device memory.
    uint * d_data = NULL;
    cudaMalloc((void**) &d_data, min(imageSize, blockMax * 64U));

	// Allocate result.
    uint * d_result = NULL;
    cudaMalloc((void**) &d_result, min(compressedSize, blockMax * 8U));

	AlphaBlockDXT3 * alphaBlocks = NULL;
	alphaBlocks = (AlphaBlockDXT3 *)malloc(min(compressedSize, blockMax * 8U));

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	clock_t start = clock();

	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, blockMax);

	    cudaMemcpy(d_data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		compressWeightedKernel(count, d_data, d_result, d_bitmaps);

		// Compress alpha in parallel with the GPU.
		for (uint i = 0; i < count; i++)
		{
			ColorBlock rgba(blockLinearImage + (bn + i) * 16);
			compressBlock(rgba, alphaBlocks + i);
		}

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));

			if (outputOptions.errorHandler != NULL)
			{
				outputOptions.errorHandler->error(Error_CudaError);
			}
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, d_result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		if (outputOptions.outputHandler != NULL)
		{
			for (uint i = 0; i < count; i++)
			{
				outputOptions.outputHandler->writeData(alphaBlocks + i, 8);
				outputOptions.outputHandler->writeData(blockLinearImage + i * 2, 8);
			}
		}

		bn += count;
	}

	clock_t end = clock();
	printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(alphaBlocks);
	free(blockLinearImage);
	cudaFree(d_data);
	cudaFree(d_result);

#else
	if (outputOptions.errorHandler != NULL)
	{
		outputOptions.errorHandler->error(Error_CudaError);
	}
#endif
}


/// Compress image using CUDA.
void nv::cudaCompressDXT5(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvDebugCheck(cuda::isHardwarePresent());
#if defined HAVE_CUDA

	doPrecomputation();

	// Image size in blocks.
	const uint w = (image->width() + 3) / 4;
	const uint h = (image->height() + 3) / 4;

	uint imageSize = w * h * 16 * sizeof(Color32);
    uint * blockLinearImage = (uint *) malloc(imageSize);
	convertToBlockLinear(image, blockLinearImage);

	const uint blockNum = w * h;
	const uint compressedSize = blockNum * 8;
	const uint blockMax = 32768; // 49152, 65535

    // Allocate image in device memory.
    uint * d_data = NULL;
    cudaMalloc((void**) &d_data, min(imageSize, blockMax * 64U));

	// Allocate result.
    uint * d_result = NULL;
    cudaMalloc((void**) &d_result, min(compressedSize, blockMax * 8U));

	AlphaBlockDXT5 * alphaBlocks = NULL;
	alphaBlocks = (AlphaBlockDXT5 *)malloc(min(compressedSize, blockMax * 8U));

	setupCompressKernel(compressionOptions.colorWeight.ptr());
	
	clock_t start = clock();

	uint bn = 0;
	while(bn != blockNum)
	{
		uint count = min(blockNum - bn, blockMax);

	    cudaMemcpy(d_data, blockLinearImage + bn * 16, count * 64, cudaMemcpyHostToDevice);

		// Launch kernel.
		compressWeightedKernel(count, d_data, d_result, d_bitmaps);

		// Compress alpha in parallel with the GPU.
		for (uint i = 0; i < count; i++)
		{
			ColorBlock rgba(blockLinearImage + (bn + i) * 16);
			compressBlock_Iterative(rgba, alphaBlocks + i);
		}

		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));

			if (outputOptions.errorHandler != NULL)
			{
				outputOptions.errorHandler->error(Error_CudaError);
			}
		}

		// Copy result to host, overwrite swizzled image.
		cudaMemcpy(blockLinearImage, d_result, count * 8, cudaMemcpyDeviceToHost);

		// Output result.
		if (outputOptions.outputHandler != NULL)
		{
			for (uint i = 0; i < count; i++)
			{
				outputOptions.outputHandler->writeData(alphaBlocks + i, 8);
				outputOptions.outputHandler->writeData(blockLinearImage + i * 2, 8);
			}
		}

		bn += count;
	}

	clock_t end = clock();
	printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

	free(alphaBlocks);
	free(blockLinearImage);
	cudaFree(d_data);
	cudaFree(d_result);

#else
	if (outputOptions.errorHandler != NULL)
	{
		outputOptions.errorHandler->error(Error_CudaError);
	}
#endif
}



#if defined HAVE_CUDA

class Task
{
public:
	explicit Task(uint numBlocks) : blockMaxCount(numBlocks), blockCount(0)
	{
		// System memory allocations.
		blockLinearImage = new uint[blockMaxCount * 16];
		xrefs = new uint[blockMaxCount * 16];
		
		// Device memory allocations.
		cudaMalloc((void**) &d_blockLinearImage, blockMaxCount * 16 * sizeof(uint));
		cudaMalloc((void**) &d_compressedImage, blockMaxCount * 8U);
		
		// @@ Check for allocation errors.
	}
	
	~Task()
	{
		delete [] blockLinearImage;
		delete [] xrefs;
		
		cudaFree(d_blockLinearImage);
		cudaFree(d_compressedImage);
	}
	
	
	
	void addColorBlock(const ColorBlock & rgba)
	{
		nvDebugCheck(!isFull());
		
		// @@ Count unique colors?
		/*
		// Convert colors to vectors.
		Array<Vector3> pointArray(16);
		
		for(int i = 0; i < 16; i++) {
			const Color32 color = rgba.color(i);
			pointArray.append(Vector3(color.r, color.g, color.b));
		}
		
		// Find best fit line.
		const Vector3 axis = Fit::bestLine(pointArray).direction();
		
		// Project points to axis.
		float dps[16];
		uint * order = &xrefs[blockCount * 16];
		
		for (uint i = 0; i < 16; ++i)
		{
			dps[i] = dot(pointArray[i], axis);
			order[i] = i;
		}
		
		// Sort them.
		for (uint i = 0; i < 16; ++i)
		{
			for (uint j = i; j > 0 && dps[j] < dps[j - 1]; --j)
			{
				swap(dps[j], dps[j - 1]);
				swap(order[j], order[j - 1]);
			}
		}
		*/
		// Write sorted colors to blockLinearImage.
		for(uint i = 0; i < 16; ++i)
		{
		//	blockLinearImage[blockCount * 16 + i] = rgba.color(order[i]);
			blockLinearImage[blockCount * 16 + i] = rgba.color(i);
		}
		
		++blockCount;
	}
	
	bool isFull()
	{
		nvDebugCheck(blockCount <= blockMaxCount);
		return blockCount == blockMaxCount;
	}
	
	void flush(const OutputOptions & outputOptions)
	{
		if (blockCount == 0)
		{
			// Nothing to do.
			return;
		}
		
		// Copy input color blocks.
		cudaMemcpy(d_blockLinearImage, blockLinearImage, blockCount * 64, cudaMemcpyHostToDevice);
		
		// Launch kernel.
		compressKernel(blockCount, d_blockLinearImage, d_compressedImage, d_bitmaps);
		
		// Check for errors.
		cudaError_t err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			nvDebug("CUDA Error: %s\n", cudaGetErrorString(err));
			
			if (outputOptions.errorHandler != NULL)
			{
				outputOptions.errorHandler->error(Error_CudaError);
			}
		}
		
		// Copy result to host, overwrite swizzled image.
		uint * compressedImage = blockLinearImage;
		cudaMemcpy(compressedImage, d_compressedImage, blockCount * 8, cudaMemcpyDeviceToHost);
		
		// @@ Sort block indices.
		
		// Output result.
		if (outputOptions.outputHandler != NULL)
		{
		//	outputOptions.outputHandler->writeData(compressedImage, blockCount * 8);
		}

		blockCount = 0;
	}
	
private:
	
	const uint blockMaxCount;
	uint blockCount;
	
	uint * blockLinearImage;
	uint * xrefs;
	
	uint * d_blockLinearImage;
	uint * d_compressedImage;
	
};

#endif // defined HAVE_CUDA


void nv::cudaCompressDXT1_2(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
#if defined HAVE_CUDA	
	const uint w = image->width();
	const uint h = image->height();
	
	const uint blockNum = ((w + 3) / 4) * ((h + 3) / 4);
	const uint blockMax = 32768; // 49152, 65535
	
	doPrecomputation();
	
	setupCompressKernel(compressionOptions.colorWeight.ptr());

	ColorBlock rgba;
	Task task(min(blockNum, blockMax));

	clock_t start = clock();

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			task.addColorBlock(rgba);
			
			if (task.isFull())
			{
				task.flush(outputOptions);
			}
		}
	}
	
	task.flush(outputOptions);

	clock_t end = clock();
	printf("\rCUDA time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);

#else
	if (outputOptions.errorHandler != NULL)
	{
		outputOptions.errorHandler->error(Error_CudaError);
	}
#endif
}


