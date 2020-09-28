/**********************************************************************

Filename    :   GImage.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GImage.h"
#include "GJPEGUtil.h"
#include "GFile.h"
#include "GStd.h"

struct GImageColorMap
{
    UInt StartIndex;
    UInt NumEntries;
    bool HasAlpha;
    struct Entry
    {
        UInt32 R: 8;
        UInt32 G: 8;
        UInt32 B: 8;
        UInt32 A: 8;
    } Entries[256];
};

// ***** GImageBase

// Set pixel, sets only the appropriate channels
void    GImageBase::SetPixelRGBA(SInt x, SInt y, UInt32 color)
{
    // Bounds check
    if ( (((UInt)x) >= Width) || (((UInt)y) >= Height))
        return;
    if (Format >= Image_DXT1)
    {
        GASSERT(0);
        return;
    }
    
    UByte *pline = GetScanline(y);

    switch(Format)
    {
        case Image_ARGB_8888: 
            *(((UInt32*)pline) + x) = GByteUtil::LEToSystem(color);
            break;
        case Image_RGB_888:             
        // Data order is packed 24-bit, RGBRGB..., regardless of the endian-ness of the CPU.    
            *(pline + x * 3)        = (UByte) color & 0xFF;
            *(pline + x * 3 + 1)    = (UByte) (color>>8) & 0xFF;
            *(pline + x * 3 + 2)    = (UByte) (color>>16) & 0xFF;
            break;
        case Image_A_8:
            *(pline + x) = (UByte) (color >> 24);
            break;
        case Image_L_8:
            *(pline + x) = (UByte) color & 0xFF;
            break;
        default:
            break;
    }
}

void    GImageBase::SetPixelAlpha(SInt x, SInt y, UByte alpha)
{
    if ( (((UInt)x) >= Width) || (((UInt)y) >= Height))
        return;
    if (Format >= Image_DXT1)
    {
        GASSERT(0);
        return;
    }
    
    UByte *pline = GetScanline(y);

    switch(Format)
    {
        case Image_ARGB_8888:
            // Target is always little-endian
            *(pline + x * 4  + 3)   = alpha;
            break;      
        case Image_A_8:
            *(pline + x) = alpha;
            break;      
        default:
            break;
    }
}

void    GImageBase::SetPixelLum(SInt x, SInt y, UByte lum)
{
    if ( (((UInt)x) >= Width) || (((UInt)y) >= Height))
        return;
    if (Format >= Image_DXT1)
    {
        GASSERT(0);
        return;
    }
    
    UByte *pline = GetScanline(y);

    switch(Format)
    {
        case Image_ARGB_8888:
            *(pline + x * 4)        = lum;
            *(pline + x * 4 + 1)    = lum;
            *(pline + x * 4 + 2)    = lum;
            break;
        case Image_RGB_888:
            *(pline + x * 3)        = lum;
            *(pline + x * 3 + 1)    = lum;
            *(pline + x * 3 + 2)    = lum;
            break;      
        case Image_L_8:
            *(pline + x) = lum;
            break;
        default:
            break;
    }
}

UInt    GImageBase::GetBytesPerPixel(GImageBase::ImageFormat fmt)
{   
    switch(fmt)
    {
        case Image_A_8:
        case Image_L_8:
        case Image_P_8:
            return 1;
        case Image_ARGB_8888:
            return 4;
        case Image_RGB_888:
            return 3;
        default:
            break;
    }
    return 0;
}

UInt    GImageBase::GetPitch(ImageFormat fmt, UInt width)
{
    switch(fmt)
    {
        case Image_A_8:
        case Image_L_8:
        case Image_P_8:
            return width;
        case Image_ARGB_8888:
            return width * 4;
        case Image_RGB_888:
            return (width * 3 + 3) & ~3;
        default:
            break;
    }
    return 0;
}




// Computes a hash of the given data buffer.
// Hash function suggested by http://www.cs.yorku.ca/~oz/hash.html
// Due to Dan Bernstein.  Allegedly very good on strings.
//
// One problem with this hash function is that e.g. if you take a
// bunch of 32-bit ints and hash them, their hash values will be
// concentrated toward zero, instead of randomly distributed in
// [0,2^32-1], because of shifting up only 5 bits per byte.
GINLINE size_t  GImageBase_BernsteinHash(const void* pdataIn, size_t size, size_t seed = 5381)
{
    const UByte*    pdata = (const UByte*) pdataIn;
    UPInt           h    = seed;

    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ (UInt) pdata[size];
    }

    return h;
}


// Compute a hash code based on image contents.  Can be useful
// for comparing images. Will return 0 if pData is null.
UPInt   GImageBase::ComputeHash() const
{
    if (!pData || DataSize == 0) return 0;

    UPInt   h = GImageBase_BernsteinHash(&Width, sizeof(Width));
    h = GImageBase_BernsteinHash(&Height, sizeof(Height), h);
    h = GImageBase_BernsteinHash(&MipMapCount, sizeof(MipMapCount), h);
    h = GImageBase_BernsteinHash(pData, DataSize, h);     
    return h;
}

UInt GImageBase::GetMipMapLevelSize(ImageFormat format, UInt w, UInt h)
{
    UInt levelSize;
    if (format == Image_DXT1)
        levelSize = GTL::gmax(1u, w / 4) * GTL::gmax(1u, h / 4) * 8;
    else if (format >= Image_DXT3 && format <= Image_DXT5)
        levelSize = GTL::gmax(1u, w / 4) * GTL::gmax(1u, h / 4) * 16;
    else
        levelSize = GetPitch(format, w) * h;
    return levelSize;
}

UByte* GImageBase::GetMipMapLevelData(UInt level, UInt* pwidth, UInt* pheight, UInt* ppitch)
{
    if (level > MipMapCount)
        return 0;
    if (level == 0)
    {
        if (pwidth)  *pwidth = UInt(Width);
        if (pheight) *pheight = UInt(Height);
        if (ppitch)  *ppitch = UInt(Pitch);
        return pData;
    }
    
    UInt32 w = Width;
    UInt32 h = Height;
    UByte* plevelData = pData;
    for(UInt i = 0; i < level; ++i)
    {
        plevelData += GetMipMapLevelSize(Format, w, h);
        w = GTL::gmax(UInt32(1), w/2);
        h = GTL::gmax(UInt32(1), h/2);
    }
    if (pwidth)  *pwidth = UInt(w);
    if (pheight) *pheight = UInt(h);
    if (ppitch)  *ppitch = UInt(GetPitch(Format, w));
    GASSERT(plevelData < (pData + DataSize));
    return plevelData;
}


// ***** GImage implementation


GImage::GImage()
{
    Format  = Image_None;
    Width   =
    Height  =
    Pitch   = 0;
    pData   = 0;
    DataSize= 0;
    MipMapCount = 1;
}

GImage::GImage(const GImageBase &src)
: GRefCountBase<GImage>(), GImageBase()
{
    // Copy data
    if (src.pData && (pData = (UByte*)GALLOC(src.DataSize)) != 0)
    {
        DataSize= src.DataSize;
        Format  = src.Format;
        Width   = src.Width;
        Height  = src.Height;
        Pitch   = src.Pitch;
        MipMapCount = src.MipMapCount;
        ColorMap = src.ColorMap;
        memcpy(pData, src.pData, src.DataSize);
    }
    else
    {
        Format  = Image_None;
        Width   =
        Height  =
        Pitch   = 0;
        pData   = 0;
        DataSize= 0;
        MipMapCount = 1;
        ColorMap.clear();
    }
}

GImage::GImage(ImageFormat format, UInt32 width, UInt32 height)
{
    GFC_DEBUG_WARNING((width <= 0) || (height <=0), "GImage::GImage - creating image with zero size");

    Pitch = GetPitch(format, width);
    
    // This size calculation accommodates DXT formats as well.
    DataSize = GetMipMapLevelSize(format, width, height);
    if ((pData = (UByte*)GALLOC(DataSize)) != 0)
    {
        Format  = format;
        Width   = width;
        Height  = height;
        memset(pData, 0, DataSize);
    }
    else
    {
        ClearImageBase();
    }
    MipMapCount = 1;
}

GImage::~GImage()
{
    if (pData)
        GFREE(pData);
}

void    GImage::Clear()
{
    if (pData)
        GFREE(pData);
   ClearImageBase();
}


// Create an image (return 0 if allocation failed)
GImage* GImage::CreateImage(ImageFormat format, UInt32 width, UInt32 height)
{
    GImage* pimage = new GImage(format, width, height);
    if (pimage->pData && (width !=0) && (height != 0))
        return pimage;
    pimage->Release();
    return 0;
}


// Raw comparison of data
bool    GImage::operator == (const GImage &src) const
{
    if ((Format != src.Format) ||
        (Width != src.Width) ||
        (Height != src.Height) ||
        (Pitch != src.Pitch) ||
        (MipMapCount != src.MipMapCount) ||
        (DataSize != src.DataSize) ||
        (ColorMap.size() != src.ColorMap.size()))
        return 0;

    if (!pData)
        return (bool)(src.pData == 0);

    if (ColorMap.size() && memcmp(&ColorMap[0], &src.ColorMap[0], ColorMap.size() * sizeof(GColor)))
        return 0;

    // Return 1 if two buffers are identical (i.e. memcmp == 0)
    return memcmp(pData, src.pData, DataSize) == 0;
}



// ***** Image I/O utility functions

// Write the given image to the given out stream, in jpeg format.
bool    GImage::WriteJpeg(GFile* pout, int quality)
{
    GJPEGOutput*    pjout = GJPEGOutput::Create(pout, Width, Height, quality);

    for (UInt y = 0; y < Height; y++)
        pjout->WriteScanline(GetScanline(y));
    
    delete pjout;
    
    return 1; // Error code
}

static bool ConvertScanline(const UByte* psrcScanline, UInt srcBitsPerPixel, GImage::ImageFormat srcFormat, UInt srcScanlineSize,
                            UByte* pdstScanline, UInt dstBitsPerPixel, GImage::ImageFormat dstFormat, UInt dstScanlineSize,
                            void* pextraInfo)
{
    GUNUSED(pextraInfo);

    UInt srcDelta = srcBitsPerPixel / 8;
    UInt dstDelta = dstBitsPerPixel / 8;
    if (srcFormat == GImage::Image_ARGB_8888)
    {
        if (dstFormat == GImage::Image_A_8)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                pdstScanline[j] = psrcScanline[i+3]; // take only alpha
            }
            return true;
        }
    }
    else if (srcFormat == GImage::Image_RGB_888)
    {
        if (dstFormat == GImage::Image_A_8)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                UByte alpha = UByte((UInt(psrcScanline[i]) + psrcScanline[i+1] + psrcScanline[i+2])/3);
                pdstScanline[j] = alpha; 
            }
            return true;
        }
    }
    else if (srcFormat == GImage::Image_P_8)
    {
        // pextraInfo is GImageColorMap
        const GImageColorMap* pcolorMap = reinterpret_cast<GImageColorMap*>(pextraInfo);
        if (dstFormat == GImage::Image_A_8)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                const GImageColorMap::Entry& cmEntry = pcolorMap->Entries[psrcScanline[i]];
                UByte alpha;
                if (pcolorMap->HasAlpha)
                    alpha = cmEntry.A;
                else
                    alpha = UByte((UInt(cmEntry.R) + cmEntry.G + cmEntry.B)/3);
                pdstScanline[j] = alpha; 
            }
            return true;
        }
        else if (dstFormat == GImage::Image_RGB_888 || dstFormat == GImage::Image_ARGB_8888)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                const GImageColorMap::Entry& cmEntry = pcolorMap->Entries[psrcScanline[i]];
                pdstScanline[j]     = cmEntry.R; 
                pdstScanline[j+1]   = cmEntry.G; 
                pdstScanline[j+2]   = cmEntry.B; 
                if (dstFormat == GImage::Image_ARGB_8888)
                    pdstScanline[j+3] = cmEntry.A; 
            }
            return true;
        }
    }
    else if (srcFormat == GImage::Image_A_8)
    {
        if (dstFormat == GImage::Image_RGB_888)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                pdstScanline[j]     = psrcScanline[i]; 
                pdstScanline[j+1]   = psrcScanline[i]; 
                pdstScanline[j+2]   = psrcScanline[i]; 
            }
            return true;
        }
        else if (dstFormat == GImage::Image_ARGB_8888)
        {
            for(UInt i = 0, j = 0; i < srcScanlineSize && j < dstScanlineSize; i += srcDelta, j += dstDelta)
            {
                pdstScanline[j]     = 0xFF; 
                pdstScanline[j+1]   = 0xFF; 
                pdstScanline[j+2]   = 0xFF; 
                pdstScanline[j+3]   = psrcScanline[i]; 
            }
            return true;
        }
    }
    return false;
}

GImage* GImage::ConvertImage(ImageFormat destFormat)
{
    UInt w = Width;
    UInt h = Height;

    if (Format == destFormat)
        return this; // no need conversion

    GPtr<GImage> pdstImage = *GImage::CreateImage(destFormat, w, h);
    GASSERT(pdstImage);

    for (UInt y = 0; y < h; y++)
    {
        const UByte* psrcScanline = GetScanline(y);
        UByte* pdstScanline = pdstImage->GetScanline(y);

        if (!ConvertScanline(psrcScanline, GetBytesPerPixel()*8, Format, GetPitch(),
            pdstScanline, pdstImage->GetBytesPerPixel()*8, destFormat, pdstImage->GetPitch(), NULL))
            return 0; // unable to convert!
    }

    pdstImage->AddRef();
    return pdstImage;
}

// Write a 32-bit Targa format bitmap.  Dead simple, no compression.
bool    GImage::WriteTga(GFile* pout)
{
    if (!pout->IsWritable())
        return false;
    pout->WriteUByte(0);    // ID length 
    
    // Color Map type
    //    0 - indicates that no color-map data is included with this image.
    //    1 - indicates that a color-map is included with this image.
    if (Format == Image_A_8)
        pout->WriteUByte(1);
    else
        pout->WriteUByte(0);
                            
    // Image Type
    //  0   No Image Data Included
    //  1   Uncompressed, Color-mapped Image
    //  2   Uncompressed, True-color Image
    //  3   Uncompressed, Black-and-white Image
    //  9   Run-length encoded, Color-mapped Image
    //  10  Run-length encoded, True-color Image
    //  11  Run-length encoded, Black-and-white Image
    if (Format == Image_A_8)
        pout->WriteUByte(1);
    else
        pout->WriteUByte(2);

    // Color Map Specification
    if (Format == Image_A_8)
    {
        pout->WriteUInt16(0);   // first entry index   
        pout->WriteUInt16(256); // color map length (in entries)
        pout->WriteUByte(24);   // color map entry size (in bits)
    }
    else
    {
        pout->WriteUInt16(0);   
        pout->WriteUInt16(0);
        pout->WriteUByte(0);
    }
    pout->WriteUInt16(0);   /* X origin */
    pout->WriteUInt16(0);   /* y origin */
    pout->WriteUInt16((UInt16)Width);
    pout->WriteUInt16((UInt16)Height);

    if (Format == Image_A_8)
        pout->WriteUByte(8);   // 8 bit bitmap
    else if (Format == Image_RGB_888)
        pout->WriteUByte(24);  // 24 bit bitmap
    else
        pout->WriteUByte(32);  // 32 bit bitmap
    UByte imageDescr = 0x20;
    if (Format == Image_ARGB_8888)
        imageDescr |= 8;
    pout->WriteUByte(imageDescr); 
    //  Image Descriptor: 
    //  Bits:    7   6   5   4   3   2   1   0
    //          |0   0| |r   r| |a   a   a   a|
    //  where bits 3-0: These bits specify the number of attribute bits per
    //                  pixel. In the case of the TrueVista, these bits indicate
    //                  the number of bits per pixel which are designated as
    //                  Alpha Channel bits.
    //          bits 5-4: These bits are used to indicate the order in which
    //                  pixel data is transferred from the file to the screen.
    //                  Bit 4 is for left-to-right ordering and bit 5 is for topto-
    //                  bottom ordering as shown below:
    //                  bits:   5   4
    //                          0   0   - bottom left
    //                          0   1   - bottom right
    //                          1   0   - top left
    //                          1   1   - top right
    if (Format == Image_A_8)
    {
        // color map
        for (UInt i = 0; i < 256; ++i)
        {
            pout->WriteUByte(UByte(i));
            pout->WriteUByte(UByte(i));
            pout->WriteUByte(UByte(i));
        }
    }

    for (UInt y = 0; y < Height; y++)
    {
        UByte*  p = GetScanline(y);
        if (Format == Image_RGB_888)
        {
            // write 24-bit scanline
            for (UInt x = 0, wx = Width*3; x < wx; x += 3)
            {
                pout->WriteUByte(p[x + 2]); // B
                pout->WriteUByte(p[x + 1]); // G
                pout->WriteUByte(p[x + 0]); // R
            }
        }
        else if (Format == Image_ARGB_8888)
        {
            // write 32-bit scanline
            for (UInt x = 0, wx = Width*4; x < wx; x += 4)
            {
                pout->WriteUByte(p[x + 2]); // B
                pout->WriteUByte(p[x + 1]); // G
                pout->WriteUByte(p[x + 0]); // R
                pout->WriteUByte(p[x + 3]); // A
            }
        }
        else if (Format == Image_A_8)
        {
            // write indices 
            for (UInt x = 0; x < Width; ++x)
            {
                pout->WriteUByte(p[x]);
            }
        }
        else
        {
            // unsupported format of scanline
            GASSERT(0);
        }
    }

    if (!pout->IsWritable())
        return false;

    // error code ?
    return true;
}

// Create and read a new image from the stream.
GImage* GImage::ReadTga(GFile* pin, ImageFormat destFormat)
{
    if (!pin || !pin->IsValid()) return 0;

    UByte idLen = pin->ReadUByte();
    UByte colorMapType = pin->ReadUByte();
    UByte imageType = pin->ReadUByte();

    // Color Map Specification
    GImageColorMap colorMap;
    colorMap.StartIndex = pin->ReadUInt16();   // first entry index   
    colorMap.NumEntries = pin->ReadUInt16();   // color map length (in entries)
    UInt cmEntrySize    = pin->ReadUByte();    // color map entry size (in bits)
    if (cmEntrySize > 0 && cmEntrySize != 24 && cmEntrySize != 32)
        return 0; // only 24- or 32-bits color maps are supported

    pin->ReadUInt16(); // X origin
    pin->ReadUInt16(); // Y origin

    UInt16 width  = pin->ReadUInt16(); // width
    UInt16 height = pin->ReadUInt16(); // height

    UByte bitsPerPixel = pin->ReadUByte();
    UByte imageDescr   = pin->ReadUByte();

    if (!((colorMapType == 0 && imageType == 2) || (colorMapType == 1 && imageType == 1)))
    {
        // only uncompressed RGB and color map formats are supported now
        return 0;
    }
    if (idLen > 0)
        pin->SkipBytes(idLen);

    ImageFormat format;
    SInt srcScanLineSize, dstScanLineSize;
    switch(bitsPerPixel)
    {
    case 8:  format = Image_P_8;       srcScanLineSize = width;   break;
    case 24: format = Image_RGB_888;   srcScanLineSize = width*3; break;
    case 32: format = Image_ARGB_8888; srcScanLineSize = width*4; break;
    default: return 0; // only 8/24/32-bits TrueType format supported
    }

    UByte destBitsPerPixel = bitsPerPixel;
    if (destFormat == Image_None)
    {
#if (!defined(GFC_OS_PSP))
        if (format == Image_P_8)
        {
            // by default, convert 256-colors TGA to RGB_888 if no alpha in palette
            if (cmEntrySize < 32)
            {
                destFormat      = Image_RGB_888;
                dstScanLineSize = width*3;
                destBitsPerPixel = 24;
            }
            else
            {   // or, to ARGB, if palette is 32 bit
                destFormat      = Image_ARGB_8888;
                dstScanLineSize = width*4;
                destBitsPerPixel = 32;
            }
        }
        else
#endif
        {
            destFormat      = format;
            dstScanLineSize = srcScanLineSize;
        }
    }
    else
    {
        switch(destFormat)
        {
        case Image_A_8:         destBitsPerPixel = 8; dstScanLineSize = width;   break;
        case Image_P_8:         destBitsPerPixel = 8; dstScanLineSize = width;   break;
        case Image_RGB_888:     destBitsPerPixel = 24; dstScanLineSize = width*3; break;
        case Image_ARGB_8888:   destBitsPerPixel = 32; dstScanLineSize = width*4; break;
        default: // unsupported destination format
            return 0;
        }
    }

    if (colorMapType == 1 && imageType == 1)
    {
        // load color map
        UInt entrySizeInBytes = ((cmEntrySize+7)/8);
        if (entrySizeInBytes*colorMap.NumEntries > sizeof(colorMap.Entries))
            return 0; // too big color map, only 256*4 is supported
        if (cmEntrySize == 32)
            colorMap.HasAlpha = true;
        else
            colorMap.HasAlpha = false;
        for (UInt i = 0; i < colorMap.NumEntries; ++i)
        {
            colorMap.Entries[i].R = pin->ReadUByte();
            colorMap.Entries[i].G = pin->ReadUByte();
            colorMap.Entries[i].B = pin->ReadUByte();
            if (cmEntrySize == 32)
                colorMap.Entries[i].A = pin->ReadUByte();
            else
                colorMap.Entries[i].A = 0xFF;
        }
    }

    GPtr<GImage> pimage = *CreateImage(destFormat, width, height);
    if (pimage)
    {
        UByte* pscanline = 0;
        UByte scanlineBuf[4096 * 4];
        if (UInt(srcScanLineSize) > sizeof(scanlineBuf))
            pscanline = (UByte*)GALLOC(srcScanLineSize);
        else
            pscanline = scanlineBuf;

        int ysl = (imageDescr & 0x20) ? 0 : height - 1;
        UInt y;
        for (y = 0; y < height; y++)
        {
            // read scan-line
            unsigned char* prgbData = pimage->GetScanline(GTL::gabs(ysl));
            unsigned char* pcurScanline;
            if (format == destFormat)
                pcurScanline = prgbData;
            else
                pcurScanline = pscanline;
            if (pin->Read(pcurScanline, srcScanLineSize) != srcScanLineSize)
                break; // read error!

            if (format == Image_ARGB_8888)
            {
                // convert BGRA->RGBA
                for (UInt x = 0, wx = width*4; x < wx; x += 4)
                {
                    UByte b = pcurScanline[x];
                    pcurScanline[x] = pcurScanline[x + 2];
                    pcurScanline[x + 2] = b;
                }
            }
            else if (format == Image_RGB_888)
            {
                // convert BGR->RGB
                for (UInt x = 0, wx = width*3; x < wx; x += 3)
                {
                    UByte b = pcurScanline[x];
                    pcurScanline[x] = pcurScanline[x + 2];
                    pcurScanline[x + 2] = b;
                }
            }
            else if (destFormat == Image_P_8)
            {
                pimage->ColorMap.resize(colorMap.NumEntries);
                for (UInt i = 0; i < colorMap.NumEntries; ++i)
                    pimage->ColorMap[i].SetRGBA(colorMap.Entries[i].R, colorMap.Entries[i].G, colorMap.Entries[i].B, colorMap.Entries[i].A);
            }
            if (format != destFormat)
            {
                if (!ConvertScanline(pscanline, bitsPerPixel, format, srcScanLineSize,
                                     prgbData, destBitsPerPixel, destFormat, dstScanLineSize, &colorMap))
                    break;
            }
            --ysl;
        }
        if (pscanline && pscanline != scanlineBuf) GFREE(pscanline);

        if (y < height) // error occured
            return 0;
    }

    pimage->AddRef();
    return pimage;
}


// Create and read a new image from the given filename, if possible.
GImage* GImage::ReadJpeg(const char* filename)
{
#ifdef GFC_USE_SYSFILE
    GSysFile in(filename);
    if (in.IsValid())
        return ReadJpeg(&in);
#else
    GUNUSED(filename);
    GFC_DEBUG_WARNING(1, "GImage::ReadJpeg with filename failed - GFC_USE_SYSFILE not defined");
#endif
    return 0;
}

// Create and read a new image from the stream.
GImage* GImage::ReadJpeg(GFile* pin)
{
    GJPEGInput* pjin = GJPEGInput::Create(pin);
    if (!pjin) return 0;

    GImage* pimage;
    if (!pjin->IsErrorOccurred())
    {
        pimage = CreateImage(Image_RGB_888, pjin->GetWidth(), pjin->GetHeight());
        if (pimage)
        {
            for (UInt y = 0; y < pimage->Height; y++)
            {
                if (!pjin->ReadScanline(pimage->GetScanline(y)))
                {
                    pimage->Release();
                    pimage = NULL;
                    break;
                }
            }
        }
    }
    else
        pimage = NULL;
    delete pjin;
    return pimage;
}



// Create and read a new image from the stream.  Image is pin
// SWF JPEG2-style Format (the encoding tables come first pin a
// separate "stream" -- otherwise it's just normal JPEG).  The
// IJG documentation describes this as "abbreviated" format.
GImage* GImage::ReadSwfJpeg2(GFile* pin)
{
    GJPEGInput* pjin = GJPEGInput::CreateSwfJpeg2HeaderOnly(pin);
    if (!pjin) return 0;
    
    GImage* pimage;
    if (!pjin->IsErrorOccurred())
        pimage = ReadSwfJpeg2WithTables(pjin);
    else
        pimage = NULL;
    delete pjin;
    return pimage;
}

// Create and read a new image, using a input object that
// already has tables loaded.
GImage* GImage::ReadSwfJpeg2WithTables(GJPEGInput* pjin)
{
    if (!pjin || pjin->IsErrorOccurred())
        return NULL;

    pjin->StartImage();

    GImage* pimage = CreateImage(Image_RGB_888, pjin->GetWidth(), pjin->GetHeight());
    if (pimage)
    {
        for (UInt y = 0; y < pjin->GetHeight(); y++)
        {
            if (!pjin->ReadScanline(pimage->GetScanline(y)))
            {
                pimage->Release();
                pimage = NULL;
                break;
            }
        }
    }

    pjin->FinishImage();
    return pimage;
}

// For reading SWF JPEG3-style image data, like ordinary JPEG, 
// but stores the data pin GImage format.
GImage* GImage::ReadSwfJpeg3(GFile* pin)
{
    GJPEGInput* pjin = GJPEGInput::CreateSwfJpeg2HeaderOnly(pin);
    if (pjin == NULL) return NULL;
    
    GImage* pimage = NULL;
    if (!pjin->IsErrorOccurred())
    {
        if (pjin->StartImage())
        {
            pimage = CreateImage(Image_ARGB_8888, pjin->GetWidth(), pjin->GetHeight());
            if (pimage)
            {
                UInt    width = pimage->Width; // Cache width
                UByte*  line = (UByte*)GALLOC(3*width);

                for (UInt y = 0; y < pjin->GetHeight(); y++) 
                {
                    if (!pjin->ReadScanline(line))
                    {
                        pimage->Release();
                        pimage = NULL;
                        break;
                    }

                    UByte*  data = pimage->GetScanline(y);
                    for (UInt x = 0; x < width; x++) 
                    {
                        data[4*x+0] = line[3*x+0];
                        data[4*x+1] = line[3*x+1];
                        data[4*x+2] = line[3*x+2];
                        data[4*x+3] = 255;
                    }
                }

                GFREE(line);
            }
            
            pjin->FinishImage();
        }
    }
    delete pjin;

    return pimage;
}





// *** DDS Format loading

static const UByte* ParseUInt32(const UByte* buf, UInt32* pval)
{
    *pval = GByteUtil::LEToSystem(*(UInt32*)buf);    
    return buf + 4;
}

struct GImage_DDSFormatDescr
{
    UInt32    RGBBitCount;
    UInt32    RBitMask;
    UInt32    GBitMask;
    UInt32    BBitMask;
    UInt32    ABitMask;
    bool      HasAlpha;
    
    inline GImage_DDSFormatDescr()
    {
        RGBBitCount = RBitMask = GBitMask = BBitMask = ABitMask = 0;
        HasAlpha = false;
    }
};

static bool GImage_ParseDDSHeader(GImageBase* pimage, const UByte* buf, const UByte** pdata, GImage_DDSFormatDescr* pDDSFmt)
{
    enum {
        GFx_DDSD_CAPS               =0x00000001l,
        GFx_DDSD_HEIGHT             =0x00000002l,
        GFx_DDSD_WIDTH              =0x00000004l,
        GFx_DDSD_PITCH              =0x00000008l,
        GFx_DDSD_BACKBUFFERCOUNT    =0x00000020l,
        GFx_DDSD_ZBUFFERBITDEPTH    =0x00000040l,
        GFx_DDSD_ALPHABITDEPTH      =0x00000080l,
        GFx_DDSD_LPSURFACE          =0x00000800l,
        GFx_DDSD_PIXELFORMAT        =0x00001000l,
        GFx_DDSD_CKDESTOVERLAY      =0x00002000l,
        GFx_DDSD_CKDESTBLT          =0x00004000l,
        GFx_DDSD_CKSRCOVERLAY       =0x00008000l,
        GFx_DDSD_CKSRCBLT           =0x00010000l,
        GFx_DDSD_MIPMAPCOUNT        =0x00020000l,
        GFx_DDSD_REFRESHRATE        =0x00040000l,
        GFx_DDSD_LINEARSIZE         =0x00080000l,
        GFx_DDSD_TEXTURESTAGE       =0x00100000l,
        GFx_DDSD_FVF                =0x00200000l,
        GFx_DDSD_SRCVBHANDLE        =0x00400000l,
        GFx_DDSD_DEPTH              =0x00800000l
    };
    UInt32 flags;
    UInt32 v;
    buf = ParseUInt32(buf, &flags);

    buf = ParseUInt32(buf, &v);
    if (flags & GFx_DDSD_HEIGHT)
        pimage->Height = v;

    buf = ParseUInt32(buf, &v);
    if (flags & GFx_DDSD_WIDTH)
        pimage->Width = v;
    buf = ParseUInt32(buf, &v);
    if (flags & GFx_DDSD_PITCH)
        pimage->Pitch = v;
    //else if (flags & GFx_DDSD_LINEARSIZE)
    //    pimage->LinearSize = v;

    buf = ParseUInt32(buf, &v);
    //if (flags & GFx_DDSD_DEPTH)
    //    pimage->Depth = v;

    buf = ParseUInt32(buf, &v);
    if (flags & GFx_DDSD_MIPMAPCOUNT)
        pimage->MipMapCount = v;

    //buf = ParseUInt32(buf, &v); // alpha bit count
    //if (flags & GFx_DDSD_ALPHABITDEPTH)
    //    pimage->AlphaBitDepth = v;

    buf += 11 * 4;

    if (flags & GFx_DDSD_PIXELFORMAT)
    {
        // pixel format (DDPIXELFORMAT)
        buf = ParseUInt32(buf, &v); // dwSize
        if (v != 32) // dwSize should be == 32 
        {
            GASSERT(0);
            return false;
        }
        enum GFx_DDPIXELFORMAT
        {
            GFx_DDPF_ALPHAPIXELS                =0x00000001l,
            GFx_DDPF_ALPHA                      =0x00000002l,
            GFx_DDPF_FOURCC                     =0x00000004l,
            GFx_DDPF_PALETTEINDEXED4            =0x00000008l,
            GFx_DDPF_PALETTEINDEXEDTO8          =0x00000010l,
            GFx_DDPF_PALETTEINDEXED8            =0x00000020l,
            GFx_DDPF_RGB                        =0x00000040l,
            GFx_DDPF_COMPRESSED                 =0x00000080l
        };
        UInt32 pfflags;
        buf = ParseUInt32(buf, &pfflags);   // dwFlags
        buf = ParseUInt32(buf, &v);         // dwFourCC
        if (pfflags & GFx_DDPF_FOURCC)
        {
            if (v == 0x35545844)        // DXT5
                pimage->Format = GImage::Image_DXT5;
            else if (v == 0x33545844)   // DXT3
                pimage->Format = GImage::Image_DXT3;
            else if (v == 0x31545844)   // DXT1
                pimage->Format = GImage::Image_DXT1;
            buf += 20;  // skip remaining part of PixelFormat
        }
        else if ((pfflags & GFx_DDPF_RGB) || (pfflags & GFx_DDPF_ALPHA)) 
        {
            // uncompressed DDS. Only 32-bit/24-bit RGB formats and alpha only (A8) are supported
            UInt32 bitCount;
            buf = ParseUInt32(buf, &bitCount); // dwRGBBitCount
            if (pDDSFmt) pDDSFmt->RGBBitCount = bitCount;
            switch(bitCount)
            {
                case 32: pimage->Format = GImage::Image_ARGB_8888; break;
                case 24: pimage->Format = GImage::Image_RGB_888; break;
                case 8:
                    if (pfflags & GFx_DDPF_ALPHA)
                    {
                        pimage->Format = GImage::Image_A_8;
                        break;
                    }
                default: GASSERT(0); // unsupported
            }
            if (!(flags & GFx_DDSD_PITCH))
                pimage->Pitch = pimage->Width*(bitCount/8);
                //AB: what is the Pitch in DDS for 24-bit RGB?
            
            buf = ParseUInt32(buf, &v); // dwRBitMask
            if (pDDSFmt) pDDSFmt->RBitMask = v;
            buf = ParseUInt32(buf, &v); // dwGBitMask
            if (pDDSFmt) pDDSFmt->GBitMask = v;
            buf = ParseUInt32(buf, &v); // dwBBitMask
            if (pDDSFmt) pDDSFmt->BBitMask = v;
            buf = ParseUInt32(buf, &v); // dwRGBAlphaBitMask
            if (pDDSFmt && (pfflags & GFx_DDPF_ALPHAPIXELS)) 
            {
                pDDSFmt->ABitMask = v;
                pDDSFmt->HasAlpha = true;
            }

            // check for X8R8G8B8 - need to set alpha to 255
            if (v == 0 && bitCount == 32)
            {
                GASSERT(0); // not supported for now.
                //@TODO - need to have one more Image_<> format for X8R8G8B8
            }
        }
        GASSERT(pimage->Format != GImage::Image_None); // Unsupported format
        if (pimage->Format == GImage::Image_None)
            return false;
    }
    else
        buf += 32;
    buf += 16; // skip ddsCaps
    buf += 4; // skip reserved
    if (pdata) *pdata = buf;
    return true;
}

static UByte GFx_CalcShiftByMask(UInt32 mask)
{
    UInt shifts = 0;

    if (mask == 0) return 0;

    if ((mask & 0xFFFFFFu) == 0)
    {
        mask >>= 24;
        shifts += 24;
    }
    else if ((mask & 0xFFFFu) == 0)
    {
        mask >>= 16;
        shifts += 16;
    }
    else if ((mask & 0xFFu) == 0)
    {
        mask >>= 8;
        shifts += 8;
    }
    while((mask & 1) == 0)
    {
        mask >>= 1;
        ++shifts;
    }
    return UByte(shifts);
}

static void PostProcessUDDSData(GImage* pimage, const GImage_DDSFormatDescr& ddsFmt)
{
    if (!pimage->IsDataCompressed() && 
        (pimage->Format == GImage::Image_ARGB_8888 || pimage->Format == GImage::Image_RGB_888))
    {
        UByte shiftR = UByte(GFx_CalcShiftByMask(ddsFmt.RBitMask));
        UByte shiftG = UByte(GFx_CalcShiftByMask(ddsFmt.GBitMask));
        UByte shiftB = UByte(GFx_CalcShiftByMask(ddsFmt.BBitMask));
        UByte shiftA = UByte(GFx_CalcShiftByMask(ddsFmt.ABitMask));
        // uncompressed DDS - reorganize RGBA in all mipmap levels
        for (UInt curlevel = 0; curlevel < pimage->MipMapCount; ++curlevel)
        {
            UInt w, h;
            UInt pitch;
            UByte* pimgData = pimage->GetMipMapLevelData(curlevel, &w, &h, &pitch);
            for (UInt y = 0; y < h; y++)
            {
                UByte* p = pimgData + pitch*y;
                if (pimage->Format == GImage::Image_RGB_888)
                {
                    for (UInt x = 0, wx = w*3; x < wx; x += 3)
                    {
                        UInt32 val = p[x + 0] | (UInt32(p[x + 1]) << 8) | (UInt32(p[x + 2]) << 16);
                        p[x + 2] = UByte((val >> shiftB) & 0xFF); // B
                        p[x + 1] = UByte((val >> shiftG) & 0xFF); // G
                        p[x + 0] = UByte((val >> shiftR) & 0xFF); // R
                    }
                }
                else if (pimage->Format == GImage::Image_ARGB_8888)
                {
                    for (UInt x = 0, wx = w*4; x < wx; x += 4)
                    {
                        UInt32 val = p[x + 0] | (UInt32(p[x + 1]) << 8) | (UInt32(p[x + 2]) << 16) | (UInt32(p[x + 3]) << 24);
                        p[x + 2] = UByte((val >> shiftB) & 0xFF); // B
                        p[x + 1] = UByte((val >> shiftG) & 0xFF); // G
                        p[x + 0] = UByte((val >> shiftR) & 0xFF); // R
                        if (ddsFmt.HasAlpha)
                            p[x + 3] = UByte((val >> shiftA) & 0xFF); // A
                        else
                            p[x + 3] = 0xFF;
                    }
                }
            }
        }
    }
}

// Loads DDS from file creating GImage.
GImage* GImage::ReadDDS(GFile* pin)
{
    if (!pin->IsValid())
        return false;

    // First, read and verify the header.    
    GImageBase  imageBase;
    GImage *    pimage = 0;    

    SInt    fileSize = pin->GetLength();
    UInt32  fourcc   = pin->ReadUInt32();
    if (fourcc != 0x20534444) // 'D','D','S',' '
        return 0;

    UInt32 sz = pin->ReadUInt32();
    if (sz != 124)
        return 0;
    UByte buf[256];
    if (pin->Read(buf, 120) != 120)
        return 0;

    imageBase.ClearImageBase();
    GImage_DDSFormatDescr ddsFmt;
    if (!GImage_ParseDDSHeader(&imageBase, buf, 0, &ddsFmt))
        return 0;

    // Allocate image and read-in data.
    if ((pimage = new GImage())==0)
        return 0;
    pimage->Format = imageBase.Format;
    pimage->Height = imageBase.Height;
    pimage->Width  = imageBase.Width;
    pimage->Pitch  = imageBase.Pitch;
    pimage->MipMapCount = imageBase.MipMapCount;

    SInt    dataSize = fileSize - pin->Tell();
    UByte*  pdata    = (UByte*)GALLOC(dataSize);
    if (!pdata)
    {
        pimage->Release();
        return 0;
    }

    if (pin->Read(pdata, dataSize) != dataSize)
    {
        pimage->Release();
        GFREE(pdata);
        return 0;
    }
    
    // AB: do we need to do same for uncompressed DDS?
#ifdef GFC_OS_XBOX360
    if (pimage->IsDataCompressed())
    {
        // We need to convert byte order for XBox360. This does not apply
        // to other big-endian systems such as PS3.
        UInt16 *pidata = (UInt16*)pdata;
        SInt    i;
        for (i=0; i<dataSize/2; i++)
        {
            *pidata = GByteUtil::LEToSystem(*pidata);
            pidata++;
        }   
    }
#endif

    pimage->pData = pdata;
    pimage->DataSize = dataSize;

    PostProcessUDDSData(pimage, ddsFmt);

    return pimage;
}


// Loads DDS from a chunk of memory, data is copied.
GImage* GImage::ReadDDSFromMemory(const UByte* ddsData, UInt dataSize)
{
    GImage *     pimage = 0;
    const UByte* pdata;

    if ((pimage = new GImage()) == 0)
        return 0;

    UInt32  fourcc;
    ddsData = ParseUInt32(ddsData, &fourcc);
    if (fourcc != 0x20534444) // 'D','D','S',' '
        return 0;

    UInt32 sz;
    ddsData = ParseUInt32(ddsData, &sz);
    if (sz != 124)
        return 0;

    GImage_DDSFormatDescr ddsFmt;
    if (!GImage_ParseDDSHeader(pimage, ddsData, &pdata, &ddsFmt))
    {
        pimage->Release();
        return 0;
    }

    // Alloc data and copy it.
    pimage->DataSize = dataSize - UInt(pdata - ddsData);
    pimage->pData    = (UByte*)GALLOC(pimage->DataSize);
    if (!pimage->pData)
    {
        pimage->Release();
        return 0;
    }

    memcpy(pimage->pData, pdata, pimage->DataSize);

    // AB: do we need to do same for uncompressed DDS? I guess - not.
#ifdef GFC_OS_XBOX360
    if (pimage->IsDataCompressed())
    {
        // We need to convert byte order for XBox360. This does not apply
        // to other big-endian systems such as PS3.
        UInt16 *pidata = (UInt16*)pdata;
        UInt    i;
        for (i = 0; i < pimage->DataSize/2; i++)
        {
            *pidata = GByteUtil::LEToSystem(*pidata);
            pidata++;
        }   
    }
#endif

    PostProcessUDDSData(pimage, ddsFmt);

    return pimage;   
}

// Create and read a new image from the given filename, if possible.
GImage* GImage::ReadPng(const char* filename)
{
#ifdef GFC_USE_SYSFILE
    GSysFile in(filename);
    if (in.IsValid())
        return ReadPng(&in);
#else
    GUNUSED(filename);
    GFC_DEBUG_WARNING(1, "GImage::ReadPng with filename failed - GFC_USE_SYSFILE not defined");
#endif
    return 0;
}

#ifdef GFC_USE_LIBPNG
#include "png.h"

#ifdef GFC_CC_MSVC
// disable warning "warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable"
#pragma warning(disable:4611)
#endif

struct GFxPngContext
{
    png_structp         png_ptr;
    png_infop           info_ptr;
    png_uint_32         width, height; 
    int                 bitDepth;
    int                 colorType;
    png_uint_32         ulRowBytes;
    char                errorMessage[100];
    char                filePath[256];
};

static void
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    GFile* pfile = reinterpret_cast<GFile*>(png_ptr->io_ptr);
    SInt check = pfile->Read(data, length);

    if (check < 0 || ((png_size_t)check) != length)
    {
        png_error(png_ptr, "Read Error.");
    }
}

static void 
png_error_handler(png_structp png_ptr, png_const_charp msg)
{
    size_t mlen = strlen(msg);

    GFxPngContext *pcontext = (GFxPngContext*)png_get_error_ptr(png_ptr);
    if (mlen < sizeof(pcontext->errorMessage))
        strcpy(pcontext->errorMessage, msg);
    else
    {
        strncpy(pcontext->errorMessage, msg, sizeof(pcontext->errorMessage) - 1);
        pcontext->errorMessage[sizeof(pcontext->errorMessage) - 1] = 0;
    }
    longjmp(png_ptr->jmpbuf, 1);
}

static
int GFxPngReadInfo(GFxPngContext* context)
{
    double              dGamma;
    png_color_16       *pBackground;

    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "GImage::ReadPng failed - Can't read info from file %s, error - %s\n", 
            context->filePath, context->errorMessage);
        png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
        return 0;
    }

    // initialize the png structure

    png_set_sig_bytes(context->png_ptr, 8);

    // read all PNG info up to image data

    png_read_info(context->png_ptr, context->info_ptr);

    // get width, height, bit-depth and color-type

    png_get_IHDR(context->png_ptr, context->info_ptr, &context->width, &context->height, &context->bitDepth,
        &context->colorType, NULL, NULL, NULL);

    // expand images of all color-type and bit-depth to 3x8 bit RGB images
    // let the library process things like alpha, transparency, background

    // convert to 8-bit per color component
    if (context->bitDepth == 16)
        png_set_strip_16(context->png_ptr);

    // convert palette  (8-bit per pixel) to RGB
    if (context->colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(context->png_ptr);

    if (context->bitDepth < 8)
        png_set_gray_1_2_4_to_8(context->png_ptr);

    if (png_get_valid(context->png_ptr, context->info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(context->png_ptr);

    if (context->colorType == PNG_COLOR_TYPE_GRAY ||
        context->colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(context->png_ptr);


    // set the background color to draw transparent and alpha images over.
    if (png_get_bKGD(context->png_ptr, context->info_ptr, &pBackground))
    {
        png_set_background(context->png_ptr, pBackground, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    }

    // if required set gamma conversion
    if (png_get_gAMA(context->png_ptr, context->info_ptr, &dGamma))
        png_set_gamma(context->png_ptr, (double) 2.2, dGamma);

    // after the transformations have been registered update info_ptr data

    png_read_update_info(context->png_ptr, context->info_ptr);

    // get again width, height and the new bit-depth and color-type

    png_get_IHDR(context->png_ptr, context->info_ptr, &context->width, &context->height, &context->bitDepth,
        &context->colorType, NULL, NULL, NULL);

    // row_bytes is the width x number of channels

    context->ulRowBytes = png_get_rowbytes(context->png_ptr, context->info_ptr);

    return 1;
}

static
int GFxPngReadData(GFxPngContext* context, png_byte **ppbRowPointers)
{
    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        GFC_DEBUG_WARNING2(1, "GImage::ReadPng failed - Can't read data from file %s, error - %s\n", 
            context->filePath, context->errorMessage);
        png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
        return 0;
    }

    // now we can go ahead and just read the whole image

    png_read_image(context->png_ptr, ppbRowPointers);

    // read the additional chunks in the PNG file (not really needed)

    png_read_end(context->png_ptr, NULL);

    return 1;
}
#endif // GFC_USE_LIBPNG

// Create and read a new image from the stream.
GImage* GImage::ReadPng(GFile* pin)
{
#ifdef GFC_USE_LIBPNG
    png_byte            pbSig[8];
    png_byte           *pbImageData; // = *ppbImageData;
    png_byte            **ppbRowPointers = NULL;
    ImageFormat         destFormat;
    UInt                srcScanLineSize;

    if (!pin)
        return NULL;

    GFxPngContext context;
    memset(&context, 0, sizeof(context));

    const char* ppath = pin->GetFilePath();
    gfc_strcpy(context.filePath, sizeof(context.filePath), ppath);

    // first check the eight byte PNG signature

    if (pin->Read(pbSig, 8) != 8 || !png_check_sig(pbSig, 8))
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't read signature from file %s\n", 
            context.filePath);
        return NULL;
    }

    // create the two png(-info) structures

    context.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_error_ptr)&context,
        (png_error_ptr)png_error_handler, NULL);
    if (!context.png_ptr)
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't create read struct for file %s\n", 
            context.filePath);
        return NULL;
    }

    context.info_ptr = png_create_info_struct(context.png_ptr);
    if (!context.info_ptr)
    {
        GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Can't create info struct for file %s\n", 
            context.filePath);
        png_destroy_read_struct(&context.png_ptr, NULL, NULL);
        return NULL;
    }

    png_set_read_fn(context.png_ptr, (png_voidp)pin, png_read_data);

    if (!GFxPngReadInfo(&context))
    {
        // context.errorMessage contains an error message
        png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
        return NULL;
    }

    switch(context.colorType)
    {
        case PNG_COLOR_TYPE_RGB:        destFormat = Image_RGB_888;   srcScanLineSize = context.width*3; break;
        case PNG_COLOR_TYPE_RGB_ALPHA:  destFormat = Image_ARGB_8888; srcScanLineSize = context.width*4; break;
        default: destFormat = Image_None; break;
    }

    if (destFormat != Image_None)
    {
        // now we can allocate memory to store the image
        GPtr<GImage> pimage = *CreateImage(destFormat, context.width, context.height);
        if (pimage)
        {
            pbImageData = (png_byte*)pimage->GetScanline(0);

            // and allocate memory for an array of row-pointers

            if ((ppbRowPointers = (png_bytepp) GALLOC((context.height)
                * sizeof(png_bytep))) == NULL)
            {
                GFC_DEBUG_WARNING1(1, "GImage::ReadPng failed - Out of memory, file %s\n", 
                    context.filePath);
                png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
                return NULL;
            }

            // set the individual row-pointers to point at the correct offsets

            for (UInt i = 0; i < context.height; i++)
                ppbRowPointers[i] = pbImageData + i * context.ulRowBytes;

            if (!GFxPngReadData(&context, ppbRowPointers))
            {
                GFREE(ppbRowPointers);
                return NULL;
            }
            png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);

            GFREE(ppbRowPointers);

            pimage->AddRef();
            return pimage;
        }
    }
    png_destroy_read_struct(&context.png_ptr, &context.info_ptr, NULL);
    return NULL;
    
#else
    GUNUSED(pin);
    GFC_DEBUG_WARNING(1, "GImage::ReadPng failed - GFC_USE_LIBPNG not defined");
    return NULL;
#endif
}

