#if 0
template<class F>
struct Hilbert_Scan
{
   enum 
   {
      NONE,
      UP,
      LEFT,
      DOWN,
      RIGHT,
   };

   int mWidth, mHeight;
   int m_x, m_y;
   F& m_pixel_functor;

   void move(int direction)
   {
      if (((uint)m_x < (uint)mWidth) && ((uint)m_y < (uint)mHeight))
         m_pixel_functor.processPixel(m_x, m_y);

      switch (direction) 
      {
      case LEFT:
         m_x--;
         break;
      case RIGHT:
         m_x++;
         break;
      case UP:
         m_y--;
         break;
      case DOWN:
         m_y++;
         break;
      } 
   }

   void hilbert_level(int level, int direction)
   {
      if (level == 1) 
      {
         switch (direction) 
         {
         case LEFT:
            move(RIGHT);
            move(DOWN);
            move(LEFT);
            break;
         case RIGHT:
            move(LEFT);
            move(UP);
            move(RIGHT);
            break;
         case UP:
            move(DOWN);
            move(RIGHT);
            move(UP);
            break;
         case DOWN:
            move(UP);
            move(LEFT);
            move(DOWN);
            break;
         } 
      } 
      else 
      {
         switch (direction) 
         {
         case LEFT:
            hilbert_level(level-1, UP);
            move(RIGHT);
            hilbert_level(level-1, LEFT);
            move(DOWN);
            hilbert_level(level-1, LEFT);
            move(LEFT);
            hilbert_level(level-1, DOWN);
            break;
         case RIGHT:
            hilbert_level(level-1, DOWN);
            move(LEFT);
            hilbert_level(level-1, RIGHT);
            move(UP);
            hilbert_level(level-1, RIGHT);
            move(RIGHT);
            hilbert_level(level-1, UP);
            break;
         case UP:
            hilbert_level(level-1, LEFT);
            move(DOWN);
            hilbert_level(level-1, UP);
            move(RIGHT);
            hilbert_level(level-1, UP);
            move(UP);
            hilbert_level(level-1, RIGHT);
            break;
         case DOWN:
            hilbert_level(level-1, RIGHT);
            move(UP);
            hilbert_level(level-1, DOWN);
            move(LEFT);
            hilbert_level(level-1, DOWN);
            move(DOWN);
            hilbert_level(level-1, LEFT);
            break;
         } 
      }
   }

   Hilbert_Scan(int width, int height, F& pixel_functor) : 
   mWidth(width), mHeight(height), 
      m_pixel_functor(pixel_functor)
   {
   }

   void scan(void)
   {
      m_x = 0;
      m_y = 0;

      const int size = Math::Max(mWidth, mHeight);
      int level = Math::iLog2(size);
      if ((1L << level) < size)
         level++;

      if (level > 0)
         hilbert_level(level, UP);

      move(NONE);
   }
};

class Riemersma_Dither
{
   uint mErrorQueueSize;
   int mMaxRelativeWeight;

   const BRGBAImage& mSrc;

   uchar* mpDXTImage;
   uint mBytesPerBlock;
   uint mColorBlockOfs;

   BAlignedArray<float> mWeights;

   int mWidth, mHeight;

   typedef std::deque<BVec3> ErrorCont;
   ErrorCont mErrorList;

public:
   // quite inefficient: this doesn't use the significant component map
   void processPixel(int x, int y)
   {
      const BRGBAColor& srcColor = mSrc(x, y);
      BVec3 src_unmapped(srcColor.r, srcColor.g, srcColor.b);

      BVec3 error(0.0f);

      int weight_index = 0;
      for (ErrorCont::const_iterator it = mErrorList.begin(); it != mErrorList.end(); it++, weight_index++)
         error += *it * mWeights[weight_index];

      BVec3 perturbed(src_unmapped);
      perturbed += error;
      perturbed.clampComponents(0.0f, 255.0f);

      BRGBAColor perturbedColor;
      perturbedColor.set(fastRound(perturbed[0]), fastRound(perturbed[1]), fastRound(perturbed[2]), 0);

      const uint cellX = x >> 2;
      const uint cellY = y >> 2;
      BDXTUtils::BDXT1Cell& cell = *(BDXTUtils::BDXT1Cell*)(mpDXTImage + (cellX + cellY * (mWidth >> 2)) * mBytesPerBlock + mColorBlockOfs);
      BRGBAColor colors[4];
      BColorUtils::unpackColor(cell.getColor0(), colors[0], true);
      BColorUtils::unpackColor(cell.getColor1(), colors[1], true);
      uint numColors;
      if (cell.getColor0() <= cell.getColor1())
      {
         colors[2].set((colors[0].r+colors[1].r)/2, (colors[0].g+colors[1].g)/2, (colors[0].b+colors[1].b)/2, 0);
         numColors = 3;
      }
      else
      {
         colors[2].set((colors[0].r*2+colors[1].r)/3, (colors[0].g*2+colors[1].g)/3, (colors[0].b*2+colors[1].b)/3, 0);
         colors[3].set((colors[1].r*2+colors[0].r)/3, (colors[1].g*2+colors[0].g)/3, (colors[1].b*2+colors[0].b)/3, 0);
         numColors = 4;
      }

      BVec3 axis(colors[0].r-colors[1].r, colors[0].g-colors[1].g, colors[0].b-colors[1].b);
      axis.tryNormalize();
      BVec3 mean(colors[0].r+colors[1].r, colors[0].g+colors[1].g, colors[0].b+colors[1].b);
      mean *= .5f;

      float c[4];
      for (uint i = 0; i < 4; i++)
         c[i] = BVec3((colors[i].r-mean[0]), (colors[i].g-mean[1]), (colors[i].b-mean[2])) * axis;

      float r = fabs((c[1] - c[0]) * .25f);

      float p = BVec3((perturbedColor.r-mean[0]), (perturbedColor.g-mean[1]), (perturbedColor.b-mean[2])) * axis;

      float l = Math::Min(c[0], c[1]);
      float h = Math::Max(c[0], c[1]);

      int bestDist = INT_MAX;
      uint bestIndex = 0;
      for (uint i = 0; i < numColors; i++)
      {
         int dist = BColorUtils::colorDistance(perturbedColor, colors[i]);
         if (dist < bestDist)
         {
            bestDist = dist;
            bestIndex = i;
         }
      }

      const uint origSelector = cell.getSelector(x & 3, y & 3);

      cell.setSelector(x & 3, y & 3, bestIndex);

      BVec3 a(perturbed);
      BVec3 b(colors[bestIndex].r, colors[bestIndex].g, colors[bestIndex].b);

      error = a - b;

      //float mult = 2.00f - Math::Clamp(fabs( (p - (l + h) * .5f) ) / ((h - l) * .5f), 1.0f, 2.0f);
      //mult = Math::Clamp(mult, 0.0f, 1.0f);
      //error *= mult;

#if 0           
      if (p < (l - r))
      {
         error *= .5f;
      }
      else if (p > (h + r))
      {
         error *- .5f;
      }
#endif      

      if (mErrorList.size() == mErrorQueueSize)
         mErrorList.pop_back();

      mErrorList.push_front(error);
   }

   void create_weights(void)
   {
      double v = 1.0f, m = exp(logf(float(mMaxRelativeWeight)) / float(mErrorQueueSize - 1));
      double sum = 0.0f;
      for (int i = mErrorQueueSize - 1; i >= 0; i--)
      {
         mWeights[i] = float(v * (1.0f / mMaxRelativeWeight)); 
         sum += mWeights[i];
         v *= m;    
      }

      for (uint i = 0; i < mErrorQueueSize; i++)
         mWeights[i] *= float(1.0f / sum);			
   }

   Riemersma_Dither(
      const BRGBAImage& src, 
      uchar* pDXTImage, uint bytesPerBlock, uint colorBlockOfs,
      int errorQueueSize = 8, int maxRelativeWeight = 12) :
   mSrc(src),  
      mpDXTImage(pDXTImage),
      mBytesPerBlock(bytesPerBlock),
      mColorBlockOfs(colorBlockOfs),
      mWidth(src.getWidth()),
      mHeight(src.getHeight()),
      mErrorQueueSize(errorQueueSize),
      mMaxRelativeWeight(maxRelativeWeight),
      mWeights(errorQueueSize)
   {
      create_weights();
   }

   void operator()()
   {
      mErrorList.clear();

      Hilbert_Scan<Riemersma_Dither> scanner(mWidth, mHeight, *this);

      scanner.scan();
   }
};
#endif