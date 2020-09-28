
using System;
using System.Drawing;
using System.Collections.Generic;

using EditorCore;
using NoiseGeneration;

namespace RandomMapGenerator
{
    public class randomMapGen
    {
        int mWidth = 256;
        int mHeight = 256;

        public enum eMapType
        {
            e1v1 = 1,
            e2v2 = 2,
            e3v3 = 3
        };
        eMapType mMapType = eMapType.e1v1;

        Random mRand = new Random(128);

        public class generationOptions
        {
           eMapType mLayoutType = eMapType.e1v1;
           public eMapType LayoutType
           {
              get
              {
                 return mLayoutType;
              }
              set
              {
                 mLayoutType = value;
              }
           }

           int mRandomSeed = 0;
           public int RandomSeed
           {
              get
              {
                 return mRandomSeed;
              }
              set
              {
                 mRandomSeed = value;
              }
           }

           bool mAllowHorizontalFlip = false;
           public bool AllowHorizontalFlip
           {
              get
              {
                 return mAllowHorizontalFlip;
              }
              set
              {
                 mAllowHorizontalFlip = value;
              }
           }

            float mHorizontalFlipFrequency = 0.5f;
           public float HorizontalFlipFrequency
           {
              get
              {
                 return mHorizontalFlipFrequency;
              }
              set
              {
                 mHorizontalFlipFrequency = value;
              }
           }

            bool mAllowVerticalFlip = false;
            public bool AllowVerticalFlip
           {
              get
              {
                 return mAllowVerticalFlip;
              }
              set
              {
                 mAllowVerticalFlip = value;
              }
           }

            float mVerticalFlipFrequency = 0.5f;
           public float VerticalFlipFrequency
           {
              get
              {
                 return mVerticalFlipFrequency;
              }
              set
              {
                 mVerticalFlipFrequency = value;
              }
           }

           int mBaseAreaWidth = 30;
           public int BaseAreaWidth
           {
              get
              {
                 return mBaseAreaWidth;
              }
              set
              {
                 mBaseAreaWidth = value;
              }
           }


           int mInterestAreaWidth = 20;
           public int InterestAreaWidth
           {
              get
              {
                 return mInterestAreaWidth;
              }
              set
              {
                 mInterestAreaWidth = value;
              }
           }

           int mMainPathWidth = 20;
           public int MainPathWidth
           {
              get
              {
                 return mMainPathWidth;
              }
              set
              {
                 mMainPathWidth = value;
              }
           }

           int mSubPathWidth = 10;
           public int SubPathWidth
           {
              get
              {
                 return mSubPathWidth;
              }
              set
              {
                 mSubPathWidth = value;
              }
           }

           int mClampStep = 1;
           public int ClampStep
           {
              get
              {
                 return mClampStep;
              }
              set
              {
                 mClampStep = value;
              }
           }
        }

        #region dataChannels
        Sparse2DBlockArray<bool>    mObstructionsMap = null;    //TRUE if an obstruction should be here
        Sparse2DBlockArray<float>   mTerrainHeightMap = null;   //height of terrains
        Sparse2DBlockArray<bool>    mPathsMap = null;           //TRUE if path should be here.
        #endregion

        #region Starting Locations
        void generateLocations(eMapType type, int width, int height, ref List<Point> startingLocations, ref List<Point> battleLocations, ref List<Point> interestLocations)
        {
            if (type == eMapType.e1v1)
               generate1v1Positions(type, width, height, ref startingLocations, ref battleLocations, ref interestLocations);
            else if (type == eMapType.e2v2)
               generate2v2Positions(type, width, height, ref startingLocations, ref battleLocations, ref interestLocations);
            else if (type == eMapType.e3v3)
               generate3v3Positions(type, width, height, ref startingLocations, ref battleLocations, ref interestLocations);
        }
       void generate1v1Positions(eMapType type, int width, int height, ref List<Point> startingLocations, ref List<Point> battleLocations, ref List<Point> interestLocations)
        {
            int midX = width >> 1;
            int qtrX = width >> 2;
            int qtr3X = width - qtrX;
            int ethX = width >> 3;
            int eth7X = ethX * 7;

            int midY = height >> 1;
            int qtrY = height >> 2;
            int qtr3Y = height - qtrY;
            int ethY = height >> 3;
            int eth7Y = ethY * 7;

            Point[] positions = new Point[]{
                //p1                        //p2
                new Point(midX,ethY),       new Point(midX,eth7Y),      //center 
                new Point(ethX,ethY),       new Point(ethX,eth7Y),      //left 
                new Point(eth7X,ethY),      new Point(eth7X,eth7Y),     //right 
                new Point(ethX,eth7Y),      new Point(eth7X,ethY),      //diag lb->rt
                new Point(ethX,ethY),       new Point(eth7X,eth7Y),     //diag lt->rb 
            };


            int layoutType = mRand.Next(positions.Length / 2);

            startingLocations.Add(positions[layoutType * 2]);
            startingLocations.Add(positions[layoutType * 2 + 1]);

            //transpose points?
            if (mRand.Next(2) != 0)
            {
               for (int i = 0; i < startingLocations.Count; i++)
                  startingLocations[i] = new Point(startingLocations[i].Y, startingLocations[i].X);
            }

            battleLocations.Add(new Point(midX, midY));

        }
       void generate2v2Positions(eMapType type, int width, int height, ref List<Point> startingLocations, ref List<Point> battleLocations, ref List<Point> interestLocations)
        {
            int midX = width >> 1;
            int qtrX = width >> 2;
            int qtr3X = width - qtrX;
            int ethX = width >> 3;
            int eth7X = ethX * 7;

            int midY = height >> 1;
            int qtrY = height >> 2;
            int qtr3Y = height - qtrY;
            int ethY = height >> 3;
            int eth7Y = ethY * 7;

            Point[] positions = new Point[]{
                //p1                        //p2                        //p3                        //p4
                new Point(ethX,ethY),       new Point(eth7X,ethY),      new Point(ethX,eth7Y),      new Point(eth7X,eth7Y),  //square wide 
                
                new Point(midX,ethY),       new Point(ethX,midY),      new Point(eth7X,midY),      new Point(midX,eth7Y),    //diagonal wide
                new Point(qtrX,ethY),       new Point(ethX,qtrY),      new Point(qtr3X,eth7Y),      new Point(eth7X,qtr3Y),  //diagonal tight lt->rb
                new Point(qtr3X,ethY),       new Point(eth7X,qtrY),      new Point(qtrX,eth7Y),      new Point(ethX,qtr3Y),  //diagonal tight lt->rb
                
            };

            int layoutType = mRand.Next(positions.Length / 4);

            startingLocations.Add(positions[layoutType * 4]);
            startingLocations.Add(positions[layoutType * 4 + 1]);
            startingLocations.Add(positions[layoutType * 4 + 2]);
            startingLocations.Add(positions[layoutType * 4 + 3]);

            //transpose points?
            if (mRand.Next(2) != 0)
            {
               for (int i = 0; i < startingLocations.Count; i++)
                  startingLocations[i] = new Point(startingLocations[i].Y, startingLocations[i].X);
            }

            battleLocations.Add(new Point(midX, midY));

        }
       void generate3v3Positions(eMapType type, int width, int height, ref List<Point> startingLocations, ref List<Point> battleLocations, ref List<Point> interestLocations)
        {
            int midX = width >> 1;
            int qtrX = width >> 2;
            int qtr3X = width - qtrX;

            int midY = height >> 1;
            int qtrY = height >> 2;
            int qtr3Y = height - qtrY;

            Point[] positions = new Point[]{
                //p1                        //p2                        //p3                        //p4                        //p5                        //p6
                new Point(qtrX,qtrY),       new Point(midX,qtrY),      new Point(qtr3X,qtrY),      new Point(qtrX,qtr3Y),       new Point(midX,qtr3Y),      new Point(qtr3X,qtr3Y),
            };

            int layoutType = mRand.Next(positions.Length / 6);

            startingLocations.Add(positions[layoutType * 6]);
            startingLocations.Add(positions[layoutType * 6 + 1]);
            startingLocations.Add(positions[layoutType * 6 + 2]);
            startingLocations.Add(positions[layoutType * 6 + 3]);
            startingLocations.Add(positions[layoutType * 6 + 4]);
            startingLocations.Add(positions[layoutType * 6 + 5]);

            //transpose points?
            if (mRand.Next(2) != 0)
            {
               for (int i = 0; i < startingLocations.Count; i++)
                  startingLocations[i] = new Point(startingLocations[i].Y, startingLocations[i].X);
            }

            battleLocations.Add(new Point(midX, midY));
        }
       void generatePathMap(List<Point> startingLocations, List<Point> battleLocations, List<Point> interestLocations)
       {

       }
       void generateConnectivityGraph(List<Point> startingLocations, List<Point> battleLocations, List<Point> interestLocations)
       {
       }
        #endregion


        #region filter Functions
        static public float Clamp(float val, float min, float max)
        {
            if (val < min)
                return min;
            if (val > max)
                return max;
            return val;
        }

        void peterbMap(ref byte[] map, int randomSeed)
        {
            float xInc = 1.0f / mWidth;
            float yInc = 1.0f / mHeight;
            byte[] tmap = (byte[])map.Clone();
            Perlin pl = new Perlin();
            pl.mFrequency.Value = 0.03f;

            float u=0;
            float v=0;
            for (int y = 0; y < mHeight; y++, v += yInc)
            {
                for (int x = 0; x < mWidth; x++, u+=xInc)
                {
                    int srcIndex = x + mWidth * y;

                    float ss = (float)(u + (xInc * 8) * pl.getValue(x, y, randomSeed));
                    float tt = (float)(v + (yInc * 8) * pl.getValue(x + 3.5f, y + 6.7f, randomSeed + 3.4f));

                    int kx = (int)(Clamp(ss,0,1) * mWidth); if (kx >= mWidth) kx = mWidth - 1; if (kx < 0)  kx = 0;
                    int ky = (int)(Clamp(tt,0,1) * mHeight); if (ky >= mHeight) ky = mHeight - 1; if (ky < 0) ky = 0;

                    
                    int dstIndex = kx + mWidth * ky;
                    map[dstIndex] = tmap[srcIndex];
                }
                u = 0;
            }
        }

        void flipMapHorizontal(ref byte[] map)
        {
            int halfWidth = mWidth >> 1;
            for (int y = 0; y < mHeight; y++)
            {
                for (int x = 0; x < halfWidth; x++)
                {
                    int srcIndex = (mWidth-1-x) + mWidth * y;
                    int dstIndex = x + mWidth * y;
                    map[dstIndex] = map[srcIndex];
                }
            }
        }
        void flipMapVertical(ref byte[] map)
        {
            int halfHeight = mHeight >> 1;
            for (int y = 0; y < halfHeight; y++)
            {
                for (int x = 0; x < mWidth; x++)
                {
                    int srcIndex = x + mWidth * (mHeight - 1 - y);
                    int dstIndex = x + mWidth * y;
                    map[dstIndex] = map[srcIndex];
                }
            }
        }

        #region convolution matrix
        public class ConvMatrix
        {
            public enum eCoeffSpot
            {
                cTopLeft = 0,
                cTopCenter,
                cTopRight,
                cMidLeft,
                cMidCenter,
                cMidRight,
                cBotLeft,
                cBotCenter,
                cBotRight

            };
            public ConvMatrix()
            {
            }
            public ConvMatrix(float topLeft, float topCenter, float topRight, float midLeft, float midCenter, float midRight, float botLeft, float botCenter, float botRight)
            {
                mFilterCoeffs = new float[9];
                mFilterCoeffs[0] = topLeft;
                mFilterCoeffs[1] = topCenter;
                mFilterCoeffs[2] = topRight;
                mFilterCoeffs[3] = midLeft;
                mFilterCoeffs[4] = midCenter;
                mFilterCoeffs[5] = midRight;
                mFilterCoeffs[6] = botLeft;
                mFilterCoeffs[7] = botCenter;
                mFilterCoeffs[8] = botRight;

            }
            public float[] mFilterCoeffs = null;
            public float mFactor = 1;
            public float mOffset = 0;
            public void SetAll(float nVal)
            {
                if (mFilterCoeffs == null)
                    mFilterCoeffs = new float[9];

                for (int i = 0; i < 9; i++)
                    mFilterCoeffs[i] = nVal;
            }
        }
        void tapNeightbor(int k, ref int x, ref int z, int mWidth, int mHeight)
        {
            int[] neightbors = new int[16] { +1, +0, 
                                          -1, +0,
                                          +1, +1,
                                          -1, +1,
                                          +1, -1,
                                          -1, -1,
                                          +0, +1,
                                          +0, -1 };

            x = x + neightbors[k * 2];
            z = z + neightbors[k * 2 + 1];
            Clamp(x, 0, mWidth-1);
            Clamp(z, 0, mHeight -1);
        }
        void applyConvMatrix(ConvMatrix m, ref byte[] map, int mWidth, int mHeight)
        {
            int[] neightbors = new int[] {   -1, 1,          //top left
                                           0, 1,          //top center
                                           1, 1,          //top right

                                          -1, 0,          //mid left
                                           0, 0,          //mid center
                                           1, 0,          //mid right

                                          -1, -1,          //bot left
                                           0, -1,          //bot center
                                           1, -1,          //bot right
                                          };

            int imgWidth = mWidth;
            float[] tempImgArray = new float[mWidth * mHeight];

            for (int x = 0; x < imgWidth; x++)
            {
                for (int y = 0; y < imgWidth; y++)
                {
                    float total = 0;
                    for (int k = 0; k < 9; k++)
                    {
                        int xIndex = x + neightbors[k * 2];
                        int zIndex = y + neightbors[k * 2 + 1];
                        if (xIndex < 0 || xIndex > imgWidth - 1 || zIndex < 0 || zIndex > imgWidth - 1)
                            continue;



                        int index = xIndex * imgWidth + zIndex;
                        float amt = map[index] / 255.0f;
                        total += m.mFilterCoeffs[k] * amt;

                    }
                    total = total / m.mFactor + m.mOffset;

                    if (total > 1.0f)
                        total = 1.0f;
                    if (total < 0)
                        total = 0;

                    tempImgArray[x * imgWidth + y] = total;
                }
            }

            //send our mask back
            for (int x = 0; x < mWidth; x++)
            {
                for (int y = 0; y < mHeight; y++)
                {
                    int indx = x * imgWidth + y;
                    if (tempImgArray[indx] != 0)
                        map[indx] = (byte)(randomMapGen.Clamp(tempImgArray[indx], 0, 1) * 255);
                }
            }


            tempImgArray = null;

        }
        void smoothFilter(ref byte[] map, int width, int height)
        {

            //use a gaussin matrix
            ConvMatrix filter = new ConvMatrix(1, 2, 1, 2, 4, 2, 1, 2, 1);
            filter.mFactor = 16;
            filter.mOffset = 0;

            applyConvMatrix(filter, ref map, width, height);

        }
        void detectEdges(ref byte[] map, int width, int height)
        {
            ConvMatrix simpleEdgeFilter = new ConvMatrix(0, -1, 0, -1, 4, -1, 0, -1, 0);
            ConvMatrix fancyEdgeFilter = new ConvMatrix(1.0f / 6.0f, 4.0f / 6.0f, 1.0f / 6.0f, 4.0f / 6.0f, -20.0f / 6.0f, 4.0f / 6.0f, 1.0f / 6.0f, 4.0f / 6.0f, 1.0f / 6.0f);

            applyConvMatrix(fancyEdgeFilter,ref  map, width, height);
        }
        void sharpenFilter(ref byte[] map, int width, int height)
        {
            ConvMatrix filter = new ConvMatrix(0, -2, 0, -2, 11, -2, 0, -2, 0);
            filter.mFactor = 3;
            filter.mOffset = 0;


            applyConvMatrix(filter,ref  map, width, height);
        }
        void expandSelection(int numVerts, ref byte[] map, int width, int height)
        {
            ConvMatrix filter = new ConvMatrix(1, 1, 1, 1, 0, 1, 1, 1, 1);
            filter.mFactor = 1;
            filter.mOffset = 0;

            for (int i = 0; i < numVerts; i++)
                applyConvMatrix(filter, ref map, width, height);

        }
        #endregion

        void addNoiseToMap(ref byte[] map, int randomSeed, int clampStep)
        {
           Voronoi vi = new Voronoi();
            vi.mFrequency.Value = 0.01f;
            Perlin pl = new Perlin();
            pl.mFrequency.Value = 0.01f;
            //cuttoff value is used to determing if this is an obstruction, or pathable
            float fBmCuttoff = 0.2f;
            for (int x = 0; x < mWidth; x++)
            {
                for (int y = 0; y < mHeight; y++)
                {
                    double v = Clamp((float)((vi.getValue(x, y, randomSeed) + 1) * 0.5f), 0, 1);
                    double p =  Clamp((float)((pl.getValue(x, y, randomSeed) + 1) * 0.5f), 0, 1);
                    double d = Clamp((float)((v*(1.0f / 3.0f)) + (p*(2.0f/3.0f))),0,1);

                    //map[x + mWidth * y] = (byte)(d * 255);
                    //continue;


                    byte k = (byte)(d > fBmCuttoff ? 0 : 255);
                    k |= map[x + mWidth * y];

                    if (map[x + mWidth * y]!=0x00)   //pathable area
                    {
                        map[x + mWidth * y] = 127;
                    }
                    else
                    {
                        //nonpathable, use the noise value as height data.
                        float ds = ((Clamp((float)d, 0, 1) - fBmCuttoff) / (1.0f-fBmCuttoff));
                        int dc = (int)(ds * 255);

                        //adjust for 'stepping'
                        int dcrm = dc % clampStep;
                        if (dcrm != 0)
                           dc -= dcrm;

                        map[x + mWidth * y] = (byte)(Clamp(dc, 0, 255));
                    }
                   

                    
                }
            }
        }
        void flipMap(generationOptions options, ref byte[] map)
        {
            if (options.AllowHorizontalFlip)
            {
                if (mRand.NextDouble() > options.HorizontalFlipFrequency)
                    flipMapHorizontal(ref map);
            }

            if (options.AllowVerticalFlip)
            {
                if (mRand.NextDouble() > options.VerticalFlipFrequency)
                    flipMapVertical(ref map);
            }
        }
        #endregion

       

        public byte[] generateMap(generationOptions options, int width, int height)
        {
            mRand = new Random(options.RandomSeed);

            mMapType = options.LayoutType;
            mWidth = width;
            mHeight = height;

            byte[] b = new byte[width * height];
 
           List<Point> startingLocations = new List<Point>();
           List<Point> battleLocations = new List<Point>();
           List<Point> interestLocations = new List<Point>();
           generateLocations(mMapType, width, height, ref startingLocations, ref battleLocations, ref interestLocations);
           generateConnectivityGraph(startingLocations, battleLocations, interestLocations);


           for (int i = 0; i < startingLocations.Count; i++)
           {
              drawCircle(startingLocations[i], options.BaseAreaWidth, options.BaseAreaWidth, ref b);
               for (int k = 0; k < battleLocations.Count; k++)
                  drawLine(startingLocations[i], battleLocations[k], options.MainPathWidth, ref b);
           }
           for (int i = 0; i < interestLocations.Count; i++)
              drawCircle(interestLocations[i], options.InterestAreaWidth, options.InterestAreaWidth, ref b);
            for (int i = 0; i < battleLocations.Count; i++)
                drawCircle(battleLocations[i], options.InterestAreaWidth, options.InterestAreaWidth, ref b);





             addNoiseToMap(ref b, options.RandomSeed, options.ClampStep);
             peterbMap(ref b, options.RandomSeed);


             flipMap(options, ref b);

             int smoothAmt = 4;
             for (int i = 0; i < smoothAmt; i++)
                smoothFilter(ref b, mWidth, mHeight);

            mRand = null;

            return b;
        }

        #region draw functions
        void drawPoint(int X, int Y, ref byte[] map)
        {
            map[X + mWidth * Y] = 255;
        }
        void drawLine(Point a, Point b, int Width, ref byte[] map)
        {
            //Bresenham
            int x0 = a.X;
            int y0 = a.Y;
            int x1 = b.X;
            int y1 = b.Y;
            {
                int dy = y1 - y0;
                int dx = x1 - x0;
                int stepx, stepy;

                if (dy < 0) { dy = -dy; stepy = -1; } else { stepy = 1; }
                if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }
                dy <<= 1;                                                  // dy is now 2*dy
                dx <<= 1;                                                  // dx is now 2*dx

                if (dx > dy)
                {
                    int fraction = dy - (dx >> 1);                         // same as 2*dy - dx
                    while (x0 != x1)
                    {
                        if (fraction >= 0)
                        {
                            y0 += stepy;
                            fraction -= dx;                                // same as fraction -= 2*dx
                        }
                        x0 += stepx;
                        fraction += dy;                                    // same as fraction -= 2*dy

                        if (Width <= 1)
                            drawPoint(x0, y0, ref map);
                        else
                            drawCircle(new Point(x0, y0), mRand.Next(Width >> 2), mRand.Next(Width ), ref map);
                        
                    }
                }
                else
                {
                    int fraction = dx - (dy >> 1);
                    while (y0 != y1)
                    {
                        if (fraction >= 0)
                        {
                            x0 += stepx;
                            fraction -= dy;
                        }
                        y0 += stepy;
                        fraction += dx;

                        if (Width <= 1)
                            drawPoint(x0, y0, ref map);
                        else
                            drawCircle(new Point(x0, y0), mRand.Next(Width), mRand.Next(Width>>2), ref map);
                    }
                }
            }
        }
        void drawCircle(Point a, int Width, int Height, ref byte[] map)
        {
           
            int radius = Math.Max(Width >> 1, Height >> 1);
            int centerx = a.X;
            int centery = a.Y;

            for (int y = centery - radius; y < centery + radius; y++)
            {
               for (int x = centerx - radius; x < centerx + radius; x++)
                {
                    float dist = (float)Math.Sqrt((centerx - x) * (centerx - x) + (centery - y) * (centery - y));
                    if (dist < radius)
                    {
                        drawPoint(x, y, ref map);
                    }
                }
            }
        }
        void drawSquare(Point a, int Width, int Height, ref byte[] map)
        {
            int radius = Math.Max(Width >> 1, Height >> 1);
            int radiusX = radius;
            int radiusY = radius;
            int centerx = a.X;
            int centery = a.Y;

            
            for (int y = 0; y < mHeight; y++)
            {
                for (int x = 0; x < mWidth; x++)
                {
                    
                    float distX = (float)Math.Sqrt((centerx - x) * (centerx - x));
                    float distY = (float)Math.Sqrt((centery - y) * (centery - y));

                    if (distX < radiusX && distY < radiusY)
                    {
                        drawPoint(x, y, ref map);
                    }

                }
            }
        }
        #endregion
    };

    class TerrainTextureEvent
    {
        public float mSlopeVal = 0.0f;
        public float mMinHeight = 0.0f;
        public float mMaxHeight = 0.0f;
        public string mTextureName = "";
    };

    
}