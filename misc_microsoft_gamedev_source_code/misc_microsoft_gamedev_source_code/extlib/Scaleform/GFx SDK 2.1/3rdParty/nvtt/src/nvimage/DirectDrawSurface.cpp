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
#include <nvcore/Containers.h> // max
#include <nvcore/StdStream.h>

#include <nvimage/DirectDrawSurface.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/Image.h>
#include <nvimage/BlockDXT.h>

#include <string.h> // memset


using namespace nv;

#if !defined(MAKEFOURCC)
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		(uint(uint8(ch0)) | (uint(uint8(ch1)) << 8) | \
		(uint(uint8(ch2)) << 16) | (uint(uint8(ch3)) << 24 ))
#endif

namespace
{
	static const uint FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	static const uint FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	static const uint FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
	static const uint FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	static const uint FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
	static const uint FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
	static const uint FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
	static const uint FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
	static const uint FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

	// RGB formats.
	static const uint D3DFMT_R8G8B8 = 20;
	static const uint D3DFMT_A8R8G8B8 = 21;
	static const uint D3DFMT_X8R8G8B8 = 22;
	static const uint D3DFMT_R5G6B5 = 23;
	static const uint D3DFMT_X1R5G5B5 = 24;
	static const uint D3DFMT_A1R5G5B5 = 25;
	static const uint D3DFMT_A4R4G4B4 = 26;
	static const uint D3DFMT_R3G3B2 = 27;
	static const uint D3DFMT_A8 = 28;
	static const uint D3DFMT_A8R3G3B2 = 29;
	static const uint D3DFMT_X4R4G4B4 = 30;
	static const uint D3DFMT_A2B10G10R10 = 31;
	static const uint D3DFMT_A8B8G8R8 = 32;
	static const uint D3DFMT_X8B8G8R8 = 33;
	static const uint D3DFMT_G16R16 = 34;
	static const uint D3DFMT_A2R10G10B10 = 35;
	static const uint D3DFMT_A16B16G16R16 = 36;

	// Palette formats.
	static const uint D3DFMT_A8P8 = 40;
	static const uint D3DFMT_P8 = 41;
	
	// Luminance formats.
	static const uint D3DFMT_L8 = 50;
	static const uint D3DFMT_A8L8 = 51;
	static const uint D3DFMT_A4L4 = 52;

	// Floating point formats
	static const uint D3DFMT_R16F = 111;
	static const uint D3DFMT_G16R16F = 112;
	static const uint D3DFMT_A16B16G16R16F = 113;
	static const uint D3DFMT_R32F = 114;
	static const uint D3DFMT_G32R32F = 115;
	static const uint D3DFMT_A32B32G32R32F = 116;
	
	static const uint DDSD_CAPS = 0x00000001U;
	static const uint DDSD_PIXELFORMAT = 0x00001000U;
	static const uint DDSD_WIDTH = 0x00000004U;
	static const uint DDSD_HEIGHT = 0x00000002U;
	static const uint DDSD_PITCH = 0x00000008U;
	static const uint DDSD_MIPMAPCOUNT = 0x00020000U;
	static const uint DDSD_LINEARSIZE = 0x00080000U;
	static const uint DDSD_DEPTH = 0x00800000U;
		
	static const uint DDSCAPS_COMPLEX = 0x00000008U;
	static const uint DDSCAPS_TEXTURE = 0x00001000U;
	static const uint DDSCAPS_MIPMAP = 0x00400000U;
	static const uint DDSCAPS2_VOLUME = 0x00200000U;
	static const uint DDSCAPS2_CUBEMAP = 0x00000200U;

	static const uint DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
	static const uint DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

	static const uint DDPF_ALPHAPIXELS = 0x00000001U;
	static const uint DDPF_ALPHA = 0x00000002U;
	static const uint DDPF_FOURCC = 0x00000004U;
	static const uint DDPF_RGB = 0x00000040U;
	static const uint DDPF_PALETTEINDEXED1 = 0x00000800U;
	static const uint DDPF_PALETTEINDEXED2 = 0x00001000U;
	static const uint DDPF_PALETTEINDEXED4 = 0x00000008U;
	static const uint DDPF_PALETTEINDEXED8 = 0x00000020U;
	static const uint DDPF_LUMINANCE = 0x00020000U;
	static const uint DDPF_ALPHAPREMULT = 0x00008000U;
	static const uint DDPF_NORMAL = 0x80000000U;	// @@ Custom nv flag.

} // namespace

namespace nv
{
	static Stream & operator<< (Stream & s, DDSPixelFormat & pf)
	{
		s << pf.size;
		s << pf.flags;
		s << pf.fourcc;
		s << pf.bitcount;
		s << pf.rmask;
		s << pf.gmask;
		s << pf.bmask;
		s << pf.amask;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSCaps & caps)
	{
		s << caps.caps1;
		s << caps.caps2;
		s << caps.caps3;
		s << caps.caps4;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSHeader & header)
	{
		nvStaticCheck(sizeof(DDSHeader) == 128);
		s << header.fourcc;
		s << header.size;
		s << header.flags;
		s << header.height;
		s << header.width;
		s << header.pitch;
		s << header.depth;
		s << header.mipmapcount;
		s.serialize(header.reserved, 11 * sizeof(uint));
		s << header.pf;
		s << header.caps;
		s << header.notused;
		return s;
	}
}

DDSHeader::DDSHeader()
{
	this->fourcc = FOURCC_DDS;
	this->size = 124;
	this->flags  = (DDSD_CAPS|DDSD_PIXELFORMAT);
	this->height = 0;
	this->width = 0;
	this->pitch = 0;
	this->depth = 0;
	this->mipmapcount = 0;
	memset(this->reserved, 0, sizeof(this->reserved));

	// Store version information on the reserved header attributes.
	this->reserved[9] = MAKEFOURCC('N', 'V', 'T', 'T');
	this->reserved[10] = (0 << 16) | (9 << 8) | (3);	// major.minor.revision

	this->pf.size = 32;
	this->pf.flags = 0;
	this->pf.fourcc = 0;
	this->pf.bitcount = 0;
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
	this->caps.caps1 = DDSCAPS_TEXTURE;
	this->caps.caps2 = 0;
	this->caps.caps3 = 0;
	this->caps.caps4 = 0;
	this->notused = 0;
}

void DDSHeader::setWidth(uint w)
{
	this->flags |= DDSD_WIDTH;
	this->width = w;
}

void DDSHeader::setHeight(uint h)
{
	this->flags |= DDSD_HEIGHT;
	this->height = h;
}

void DDSHeader::setDepth(uint d)
{
	this->flags |= DDSD_DEPTH;
	this->height = d;
}

void DDSHeader::setMipmapCount(uint count)
{
	if (count == 0)
	{
		this->flags &= ~DDSD_MIPMAPCOUNT;
		this->mipmapcount = 0;

		if (this->caps.caps2 == 0) {
			this->caps.caps1 = DDSCAPS_TEXTURE;
		}
		else {
			this->caps.caps1 = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
		}
	}
	else
	{
		this->flags |= DDSD_MIPMAPCOUNT;
		this->mipmapcount = count;

		this->caps.caps1 |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	}
}

void DDSHeader::setTexture2D()
{
	// nothing to do here.
}

void DDSHeader::setTexture3D()
{
	this->caps.caps2 = DDSCAPS2_VOLUME;
}

void DDSHeader::setTextureCube()
{
	this->caps.caps1 |= DDSCAPS_COMPLEX;
	this->caps.caps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
}

void DDSHeader::setLinearSize(uint size)
{
	this->flags &= ~DDSD_PITCH;
	this->flags |= DDSD_LINEARSIZE;
	this->pitch = size;
}

void DDSHeader::setPitch(uint pitch)
{
	this->flags &= ~DDSD_LINEARSIZE;
	this->flags |= DDSD_PITCH;
	this->pitch = pitch;
}

void DDSHeader::setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3)
{
	// set fourcc pixel format.
	this->pf.flags = DDPF_FOURCC;
	this->pf.fourcc = MAKEFOURCC(c0, c1, c2, c3);
	this->pf.bitcount = 0;
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
}

void DDSHeader::setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
{
	// Make sure the masks are correct.
	nvCheck((rmask & gmask) == 0);
	nvCheck((rmask & bmask) == 0);
	nvCheck((rmask & amask) == 0);
	nvCheck((gmask & bmask) == 0);
	nvCheck((gmask & amask) == 0);
	nvCheck((bmask & amask) == 0);

	this->pf.flags = DDPF_RGB;

	if (amask != 0) {
		this->pf.flags |= DDPF_ALPHAPIXELS;
	}

	if (bitcount == 0)
	{
		// Compute bit count from the masks.
		uint total = rmask | gmask | bmask | amask;
		while(total != 0) {
			bitcount++;
			total >>= 1;
		}
		// @@ Align to 8?
	}

	this->pf.fourcc = 0;
	this->pf.bitcount = bitcount;
	this->pf.rmask = rmask;
	this->pf.gmask = gmask;
	this->pf.bmask = bmask;
	this->pf.amask = amask;
}

void DDSHeader::setNormalFlag(bool b)
{
	if (b) this->pf.flags |= DDPF_NORMAL;
	else this->pf.flags &= ~DDPF_NORMAL;
}

void DDSHeader::swapBytes()
{
	this->fourcc = POSH_LittleU32(this->fourcc);
	this->size = POSH_LittleU32(this->size);
	this->flags = POSH_LittleU32(this->flags);
	this->height = POSH_LittleU32(this->height);
	this->width = POSH_LittleU32(this->width);
	this->pitch = POSH_LittleU32(this->pitch);
	this->depth = POSH_LittleU32(this->depth);
	this->mipmapcount = POSH_LittleU32(this->mipmapcount);
	
	for(int i = 0; i < 11; i++) {
		this->reserved[i] = POSH_LittleU32(this->reserved[i]);
	}

	this->pf.size = POSH_LittleU32(this->pf.size);
	this->pf.flags = POSH_LittleU32(this->pf.flags);
	this->pf.fourcc = POSH_LittleU32(this->pf.fourcc);
	this->pf.bitcount = POSH_LittleU32(this->pf.bitcount);
	this->pf.rmask = POSH_LittleU32(this->pf.rmask);
	this->pf.gmask = POSH_LittleU32(this->pf.gmask);
	this->pf.bmask = POSH_LittleU32(this->pf.bmask);
	this->pf.amask = POSH_LittleU32(this->pf.amask);
	this->caps.caps1 = POSH_LittleU32(this->caps.caps1);
	this->caps.caps2 = POSH_LittleU32(this->caps.caps2);
	this->caps.caps3 = POSH_LittleU32(this->caps.caps3);
	this->caps.caps4 = POSH_LittleU32(this->caps.caps4);
	this->notused = POSH_LittleU32(this->notused);
}



DirectDrawSurface::DirectDrawSurface(const char * name) : stream(new StdInputStream(name))
{
	if (!stream->isError())
	{
		(*stream) << header;
	}
}

DirectDrawSurface::~DirectDrawSurface()
{
	delete stream;
}

bool DirectDrawSurface::isValid() const
{
	if (stream->isError())
	{
		return false;
	}
	
	if (header.fourcc != FOURCC_DDS || header.size != 124)
	{
		return false;
	}
	
	const uint required = (DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT);
	if( (header.flags & required) != required ) {
		return false;
	}
	
	if (header.pf.size != 32) {
		return false;
	}
	
	if( !(header.caps.caps1 & DDSCAPS_TEXTURE) ) {
		return false;
	}
	
	return true;
}

bool DirectDrawSurface::isSupported() const
{
	nvDebugCheck(isValid());
	
	if (header.pf.flags & DDPF_FOURCC)
	{
		if (header.pf.fourcc != FOURCC_DXT1 &&
		    header.pf.fourcc != FOURCC_DXT2 &&
		    header.pf.fourcc != FOURCC_DXT3 &&
		    header.pf.fourcc != FOURCC_DXT4 &&
		    header.pf.fourcc != FOURCC_DXT5 &&
		    header.pf.fourcc != FOURCC_RXGB &&
		    header.pf.fourcc != FOURCC_ATI1 &&
		    header.pf.fourcc != FOURCC_ATI2)
		{
			// Unknown fourcc code.
			return false;
		}
	}
	else if (header.pf.flags & DDPF_RGB)
	{
		if (header.pf.bitcount == 24)
		{
			return false;
		}
		else if (header.pf.bitcount == 32)
		{
			return false;
		}
		else
		{
			// Unsupported pixel format.
			return false;
		}
	}
	else
	{
		return false;
	}
	
	if (isTextureCube() && (header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) != DDSCAPS2_CUBEMAP_ALL_FACES)
	{
		// Cubemaps must contain all faces.
		return false;
	}
	
	if (isTexture3D())
	{
		// @@ 3D textures not supported yet.
		return false;
	}
	
	return true;
}


uint DirectDrawSurface::mipmapCount() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_MIPMAPCOUNT) return header.mipmapcount;
	else return 0;
}


uint DirectDrawSurface::width() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_WIDTH) return header.width;
	else return 1;
}

uint DirectDrawSurface::height() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_HEIGHT) return header.height;
	else return 1;
}

uint DirectDrawSurface::depth() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_DEPTH) return header.depth;
	else return 1;
}

bool DirectDrawSurface::isTexture2D() const
{
	nvDebugCheck(isValid());
	return !isTexture3D() && !isTextureCube();
}

bool DirectDrawSurface::isTexture3D() const
{
	nvDebugCheck(isValid());
	return (header.caps.caps2 & DDSCAPS2_VOLUME) != 0;
}

bool DirectDrawSurface::isTextureCube() const
{
	nvDebugCheck(isValid());
	return (header.caps.caps2 & DDSCAPS2_CUBEMAP) != 0;
}

void DirectDrawSurface::mipmap(Image * img, uint face, uint mipmap)
{
	nvDebugCheck(isValid());
	
	stream->seek(offset(face, mipmap));
	
	uint w = width();
	uint h = height();
	
	// Compute width and height.
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
	}
	
	img->allocate(w, h);
	
	if (header.pf.flags & DDPF_RGB) 
	{
		readLinearImage(img);
	}
	else if (header.pf.flags & DDPF_FOURCC)
	{
		readBlockImage(img);
	}
}

static uint8 bitExpand(uint8 c, uint bits)
{
	int shifts = 0;
	uint8 output = c;
	// @@ TODO!!!
	
}

void DirectDrawSurface::readLinearImage(Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);
	
	// @@ Read linear RGB images.
}

void DirectDrawSurface::readBlockImage(Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);
	
	const uint w = img->width();
	const uint h = img->height();
	
	const uint bw = (w + 3) / 4;
	const uint bh = (h + 3) / 4;
	
	for (uint by = 0; by < bh; by++)
	{
		for (uint bx = 0; bx < bw; bx++)
		{
			ColorBlock block;
			
			// Read color block.
			readBlock(&block);
			
			// Write color block.
			for (uint y = 0; y < min(4U, h-4*by); y++)
			{
				for (uint x = 0; x < min(4U, w-4*bx); x++)
				{
					img->pixel(4*bx+x, 4*by+y) = block.color(x, y);
				}
			}
		}
	}
}

static Color32 buildNormal(uint8 x, uint8 y)
{
	float nx = 2 * (x / 255) - 1;
	float ny = 2 * (x / 255) - 1;
	float nz = sqrtf(1 - nx*nx - ny*ny);
	uint8 z = clamp(int(255 * (nz + 1) / 2), 0, 255);
	
	return Color32(x, y, z);
}


void DirectDrawSurface::readBlock(ColorBlock * rgba)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(rgba != NULL);
	
	if (header.pf.fourcc == FOURCC_DXT1)
	{
		BlockDXT1 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_DXT2 ||
	    header.pf.fourcc == FOURCC_DXT3)
	{
		BlockDXT3 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_DXT4 ||
	    header.pf.fourcc == FOURCC_DXT5 ||
	    header.pf.fourcc == FOURCC_RXGB)
	{
		BlockDXT5 block;
		*stream << block;
		block.decodeBlock(rgba);
		
		if (header.pf.fourcc == FOURCC_RXGB)
		{
			// Swap R & A.
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				uint tmp = c.r;
				c.r = c.a;
				c.a = tmp;
			}
		}
	}
	else if (header.pf.fourcc == FOURCC_ATI1)
	{
		BlockATI1 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	else if (header.pf.fourcc == FOURCC_ATI2)
	{
		BlockATI2 block;
		*stream << block;
		block.decodeBlock(rgba);
	}
	
	// If normal flag set, convert to normal.
	if (header.pf.flags & DDPF_NORMAL)
	{
		if (header.pf.fourcc == FOURCC_ATI2)
		{
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				c = buildNormal(c.r, c.g);
			}
		}
		else if (header.pf.fourcc == FOURCC_DXT5)
		{
			for (int i = 0; i < 16; i++)
			{
				Color32 & c = rgba->color(i);
				c = buildNormal(c.g, c.a);
			}
		}
	}
}


uint DirectDrawSurface::blockSize() const
{
	switch(header.pf.fourcc)
	{
		case FOURCC_DXT1:
		case FOURCC_ATI1:
			return 8;
		case FOURCC_DXT2:
		case FOURCC_DXT3:
		case FOURCC_DXT4:
		case FOURCC_DXT5:
		case FOURCC_RXGB:
		case FOURCC_ATI2:
			return 16;
	};

	// Not a block image.
	return 0;
}

uint DirectDrawSurface::mipmapSize(uint mipmap) const
{
	uint w = width();
	uint h = height();
	uint d = depth();
	
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
		d = max(1U, d / 2);
	}

	if (header.pf.flags & DDPF_FOURCC)
	{
		// @@ How are 3D textures aligned?
		w = (w + 3) / 4;
		h = (h + 3) / 4;
		return blockSize() * w * h;
	}
	else
	{
		nvDebugCheck(header.pf.flags & DDPF_RGB);
		
		// Align pixels to bytes.
		uint byteCount = (header.pf.bitcount + 7) / 8;
		
		// Align pitch to 4 bytes.
		uint pitch = 4 * ((w * byteCount + 3) / 4);
		
		return pitch * h * d;
	}
}

uint DirectDrawSurface::faceSize() const
{
	const uint count = mipmapCount();
	uint size = 0;
	
	for (uint m = 0; m < count; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}

uint DirectDrawSurface::offset(const uint face, const uint mipmap)
{
	uint size = sizeof(DDSHeader);
	
	if (face != 0)
	{
		size += face * faceSize();
	}
	
	for (uint m = 0; m < mipmap; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}


void DirectDrawSurface::printInfo() const
{
	printf("Flags: 0x%.8X\n", header.flags);
	if (header.flags & DDSD_CAPS) printf("\tDDSD_CAPS\n");
	if (header.flags & DDSD_PIXELFORMAT) printf("\tDDSD_PIXELFORMAT\n");
	if (header.flags & DDSD_WIDTH) printf("\tDDSD_WIDTH\n");
	if (header.flags & DDSD_HEIGHT) printf("\tDDSD_HEIGHT\n");
	if (header.flags & DDSD_DEPTH) printf("\tDDSD_DEPTH\n");
	if (header.flags & DDSD_PITCH) printf("\tDDSD_PITCH\n");
	if (header.flags & DDSD_LINEARSIZE) printf("\tDDSD_LINEARSIZE\n");
	if (header.flags & DDSD_MIPMAPCOUNT) printf("\tDDSD_MIPMAPCOUNT\n");

	printf("Height: %d\n", header.height);
	printf("Width: %d\n", header.width);
	printf("Depth: %d\n", header.depth);
	if (header.flags & DDSD_PITCH) printf("Pitch: %d\n", header.pitch);
	else if (header.flags & DDSD_LINEARSIZE) printf("Linear size: %d\n", header.pitch);
	printf("Mipmap count: %d\n", header.mipmapcount);
	
	printf("Pixel Format:\n");
	printf("\tFlags: 0x%.8X\n", header.pf.flags);
	if (header.pf.flags & DDPF_RGB) printf("\t\tDDPF_RGB\n");
	if (header.pf.flags & DDPF_FOURCC) printf("\t\tDDPF_FOURCC\n");
	if (header.pf.flags & DDPF_ALPHAPIXELS) printf("\t\tDDPF_ALPHAPIXELS\n");
	if (header.pf.flags & DDPF_ALPHA) printf("\t\tDDPF_ALPHA\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED1) printf("\t\tDDPF_PALETTEINDEXED1\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED2) printf("\t\tDDPF_PALETTEINDEXED2\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED4) printf("\t\tDDPF_PALETTEINDEXED4\n");
	if (header.pf.flags & DDPF_PALETTEINDEXED8) printf("\t\tDDPF_PALETTEINDEXED8\n");
	if (header.pf.flags & DDPF_ALPHAPREMULT) printf("\t\tDDPF_ALPHAPREMULT\n");
	if (header.pf.flags & DDPF_NORMAL) printf("\t\tDDPF_NORMAL\n");
	
	printf("\tFourCC: '%c%c%c%c'\n", ((header.pf.fourcc >> 0) & 0xFF), ((header.pf.fourcc >> 8) & 0xFF), ((header.pf.fourcc >> 16) & 0xFF), ((header.pf.fourcc >> 24) & 0xFF));
	printf("\tBit count: %d\n", header.pf.bitcount);
	printf("\tRed mask: 0x%.8X\n", header.pf.rmask);
	printf("\tGreen mask: 0x%.8X\n", header.pf.gmask);
	printf("\tBlue mask: 0x%.8X\n", header.pf.bmask);
	printf("\tAlpha mask: 0x%.8X\n", header.pf.amask);

	printf("Caps:\n");
	printf("\tCaps 1: 0x%.8X\n", header.caps.caps1);
	if (header.caps.caps1 & DDSCAPS_COMPLEX) printf("\t\tDDSCAPS_COMPLEX\n");
	if (header.caps.caps1 & DDSCAPS_TEXTURE) printf("\t\tDDSCAPS_TEXTURE\n");
	if (header.caps.caps1 & DDSCAPS_MIPMAP) printf("\t\tDDSCAPS_MIPMAP\n");

	printf("\tCaps 2: 0x%.8X\n", header.caps.caps2);
	if (header.caps.caps2 & DDSCAPS2_VOLUME) printf("\t\tDDSCAPS2_VOLUME\n");
	else if (header.caps.caps2 & DDSCAPS2_CUBEMAP)
	{
		printf("\t\tDDSCAPS2_CUBEMAP\n");
		if ((header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) == DDSCAPS2_CUBEMAP_ALL_FACES) printf("\t\tDDSCAPS2_CUBEMAP_ALL_FACES\n");
		else {
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEX\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEX\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEY\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEY\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) printf("\t\tDDSCAPS2_CUBEMAP_POSITIVEZ\n");
			if (header.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) printf("\t\tDDSCAPS2_CUBEMAP_NEGATIVEZ\n");
		}
	}

	printf("\tCaps 3: 0x%.8X\n", header.caps.caps3);
	printf("\tCaps 4: 0x%.8X\n", header.caps.caps4);

	if (header.reserved[9] == MAKEFOURCC('N', 'V', 'T', 'T'))
	{
		int major = (header.reserved[10] >> 16) & 0xFF;
		int minor = (header.reserved[10] >> 8) & 0xFF;
		int revision= header.reserved[10] & 0xFF;
		
		printf("Version:\n");
		printf("\tNVIDIA Texture Tools %d.%d.%d\n", major, minor, revision);
	}
}

