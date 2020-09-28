// File: RGBAImage.inl
template<class ColorType>
inline BRGBAImageTemplate<ColorType>::BRGBAImageTemplate(const BRGBAImageTemplate& b) :
   mWidth(b.mWidth),
   mHeight(b.mHeight),
   mPitch(b.mPitch),
   mpImage(NULL)
{
   if (!mPitch) mPitch = mWidth;
   BDEBUG_ASSERT(mPitch >= mWidth);
   if ((mWidth) && (mHeight))
   {
      mImageBuf.resize(mPitch * mHeight);
      mpImage = &mImageBuf[0];
      memcpy(mpImage, b.mpImage, mPitch * mHeight * sizeof(colorType));
   }
}

template<class ColorType>
BRGBAImageTemplate<ColorType>::BRGBAImageTemplate(uint width, uint height, uint pitch) : 
   mWidth(width), 
   mHeight(height),
   mPitch(pitch),
   mpImage(NULL)
{
   if (!mPitch) mPitch = mWidth;
   BDEBUG_ASSERT(mPitch >= mWidth);
   if ((mWidth) && (mHeight))
   {
      mImageBuf.resize(mPitch * mHeight);
      std::fill(mImageBuf.begin(), mImageBuf.end(), colorType(0, 0, 0, 0));
      mpImage = &mImageBuf[0];
   }
}

template<class ColorType>
BRGBAImageTemplate<ColorType>::BRGBAImageTemplate(colorType* pImage, uint width, uint height, uint pitch) :
   mpImage(pImage),
   mWidth(width),
   mHeight(height),
   mPitch(pitch)
{
   if (!mPitch) mPitch = mWidth;
   BDEBUG_ASSERT(mPitch >= mWidth);
}

template<class ColorType>
BRGBAImageTemplate<ColorType>& BRGBAImageTemplate<ColorType>::operator= (const BRGBAImageTemplate& b)
{
   if (this == &b)
      return *this;

   setSize(b.getWidth(), b.getHeight(), b.getPitch());

   memcpy(mpImage, b.mpImage, mPitch * mHeight * sizeof(colorType));

   return *this;
}

template<class ColorType>
bool BRGBAImageTemplate<ColorType>::setSize(uint width, uint height, uint pitch, bool preserveContents)
{
   if (!pitch) pitch = width;
   BDEBUG_ASSERT(pitch >= width);

   if ((mWidth == width) && (mHeight == height) && (mPitch == pitch))
      return true;

   BRGBAImageTemplate temp;
   if (preserveContents)      
      temp.swap(*this);

   mWidth = width;
   mHeight = height;
   mPitch = pitch;

   mImageBuf.resize(pitch * height);
   if ((width) && (height))
   {
      std::fill(mImageBuf.begin(), mImageBuf.end(), colorType(0, 0, 0, 0));
      mpImage = &mImageBuf[0];

      if (preserveContents)      
      {
         for (uint y = 0; y < Math::Min<uint>(height, temp.getHeight()); y++)
            for (uint x = 0; x < Math::Min<uint>(width, temp.getWidth()); x++)
               (*this)(x, y) = temp(x, y);
      }
   }
   else
   {
      mpImage = NULL;
   }

   return true;
}

template<class ColorType>
bool BRGBAImageTemplate<ColorType>::aliasToImage(colorType* pImage, uint width, uint height, uint pitch)
{
   if (!pitch) pitch = width;
   BDEBUG_ASSERT(pitch >= width);   

   mImageBuf.clear();

   mpImage = pImage;
   mWidth = width;
   mHeight = height;
   mPitch = pitch;

   return true;
}

template<class ColorType>
void BRGBAImageTemplate<ColorType>::clear(const colorType& color)
{
   for (uint y = 0; y < mHeight; y++)
      for (uint x = 0; x < mWidth; x++)
         (*this)(x, y) = color;
}

// Sets pixel to color, with clipping.
template<class ColorType>
void BRGBAImageTemplate<ColorType>::setPixel(int x, int y, const colorType& color)
{
   if ((static_cast<uint>(x) < mWidth) && (static_cast<uint>(y) < mHeight))
      (*this)(x, y) = color;
}

template<class ColorType>
const typename BRGBAImageTemplate<ColorType>::colorType* BRGBAImageTemplate<ColorType>::getScanlinePtr(uint y) const
{
   return &getPixel(0, y);
}

template<class ColorType>
typename BRGBAImageTemplate<ColorType>::colorType* BRGBAImageTemplate<ColorType>::getScanlinePtr(uint y)
{
   return &getPixel(0, y);
}

template<class ColorType>
void BRGBAImageTemplate<ColorType>::setScanline(uint x, uint y, const colorType* pColors, uint numColors, bool flip)
{
   if ((x >= mWidth) || (y >= mHeight))
      return;
         
   numColors = Math::Min(numColors, (mWidth - x));
      
   if (flip)
   {  
      for (int i = numColors - 1; i >= 0; i--)
         setPixel(x + i, y, pColors[i]);
   }
   else
      memcpy( &getPixel(x, y), pColors, numColors * sizeof(colorType));
}

template<class ColorType>
typename const BRGBAImageTemplate<ColorType>::colorType& BRGBAImageTemplate<ColorType>::getPixel(int x, int y) const
{
   if ((static_cast<uint>(x) < mWidth) && (static_cast<uint>(y) < mHeight))
      return (*this)(x, y);

   static colorType blackColor(0,0,0,0);
   return blackColor;
}

template<class ColorType>
typename BRGBAImageTemplate<ColorType>::colorType& BRGBAImageTemplate<ColorType>::getPixel(int x, int y) 
{
   if ((static_cast<uint>(x) < mWidth) && (static_cast<uint>(y) < mHeight))
      return (*this)(x, y);

   static colorType blackColor(0,0,0,0);
   return blackColor;
}

template<class ColorType>
void BRGBAImageTemplate<ColorType>::fillRect(int x, int y, uint width, uint height, const colorType& color)
{
   for (uint sy = 0; sy < height; sy++)
      for (uint sx = 0; sx < width; sx++)
         setPixel(x + sx, y + sy, color);
}

enum { eLINEX = 0, eLINEY = 1 };

template<class ColorType>
void BRGBAImageTemplate<ColorType>::drawLine(int xs, int ys, int xe, int ye, const colorType& color)
{ 
   if (xs > xe) 
   { 
      std::swap(xs, xe);
      std::swap(ys, ye);
   } 

   int dx = xe - xs;
   int dy = ye - ys; 

   int e;
   int eInc;
   int eNoInc;

   if (0 == dx)
   { 
      if (ys > ye) 
         std::swap(ys, ye);

      for (int i = ys ; i <= ye ; i++) 
         setPixel(xs, i, color); 

      return; 
   } 

   if (0 == dy)  
   { 
      for (int i = xs ; i < xe ; i++) 
         setPixel(i, ys, color); 

      return; 
   } 

   if (dy > 0)
   {
      if (dy <= dx) 
      { 
         e = 2 * dy - dx; 

         eNoInc = 2 * dy; 
         eInc = 2 * (dy - dx); 

         drawLineInternal(xs, ys, xe, ye, eLINEX, +1, e, eInc, eNoInc, color); 
      } 
      else 
      { 
         e = 2 * dx - dy;

         eNoInc = 2 * dx;
         eInc = 2 * (dx - dy); 

         drawLineInternal(xs, ys, xe, ye, eLINEY, +1, e, eInc, eNoInc, color); 
      } 
   }      
   else
   {
      dy = -dy; 

      if (dy <= dx) 
      { 
         e = 2 * dy - dx; 

         eNoInc = 2 * dy; 
         eInc = 2 * (dy - dx); 

         drawLineInternal(xs, ys, xe, ye, eLINEX, -1, e, eInc, eNoInc, color); 
      } 
      else 
      { 
         e = 2 * dx - dy; 
         eNoInc = (2 * dx);
         eInc = 2 * (dx - dy); 

         drawLineInternal(xe, ye, xs, ys, eLINEY, -1, e, eInc, eNoInc, color); 
      } 
   }      
} 

template<class ColorType>
void BRGBAImageTemplate<ColorType>::drawLineInternal(int xs, int ys, int xe, int ye, int pred, int incDec, int e, int eInc, int eNoInc, const colorType& color) 
{ 
   int start, end, var; 

   if (pred == eLINEY) 
   {
      start = ys;
      end = ye;
      var = xs; 

      for (int i = start; i <= end; i++) 
      { 
         setPixel(var, i, color); 

         if (e < 0) 
            e += eNoInc; 
         else 
         { 
            var += incDec; 
            e += eInc;
         } 
      } 
   }
   else
   {
      start = xs;
      end = xe;
      var = ys; 

      for (int i = start; i <= end; i++) 
      { 
         setPixel(i, var, color); 

         if (e < 0) 
            e += eNoInc; 
         else 
         { 
            var += incDec; 
            e += eInc;
         } 
      } 
   }
}

template<class ColorType>
void BRGBAImageTemplate<ColorType>::swap(BRGBAImageTemplate& image)
{
   std::swap(mWidth, image.mWidth);
   std::swap(mHeight, image.mHeight);
   std::swap(mPitch, image.mPitch);
   std::swap(mpImage, image.mpImage);
   mImageBuf.swap(image.mImageBuf);
}

