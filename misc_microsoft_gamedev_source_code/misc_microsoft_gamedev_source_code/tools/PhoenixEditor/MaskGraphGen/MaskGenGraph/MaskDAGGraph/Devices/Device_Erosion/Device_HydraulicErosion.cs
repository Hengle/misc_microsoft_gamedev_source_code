using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Xml;
using System.Xml.Serialization;

using System.Text;
using System.Windows.Forms;

/* TODO
 * 
 * Calculate the entire path of a drop SEPERATE from sediment transfer process.
 * Output flow map
 */ 
namespace graphapp
{
   public class Device_HydraulicErosion : MaskDevice
    {
        #region Connection points

       MaskParam mInputMask = new MaskParam();
       [XmlIgnore]
       [ConnectionType("Input", "Primary Input", true)]
       public DAGMask InputMask
       {
          get { return mInputMask.Value; }
          set { mInputMask.Value = value; }
       }

        MaskParam mOutputMask = new MaskParam();
       [XmlIgnore]
        [ConnectionType("Output", "Primary Output")]
       public DAGMask OutputMask
        {
            get { return mOutputMask.Value; }
            set { mOutputMask.Value = value; }
        }


        FloatParam mAmount = new FloatParam(1.0f, 0.1f, 2.0f);
        [ConnectionType("Param", "Amount")]
        public float Amount
        {
            get { return mAmount.Value; }
            set { mAmount.Value = value; }
        }

        FloatParam mSoilHardness = new FloatParam(0.5f,0,1);
        [ConnectionType("Param", "SoilHardness")]
        public float SoilHardness
        {
            get { return mSoilHardness.Value; }
            set { mSoilHardness.Value = value; }
        }

        FloatParam mSedimentCarryAmt = new FloatParam(0.5f, 0, 1);
        [ConnectionType("Param", "SedimentCarryAmt")]
        public float SedimentCarryAmt
        {
            get { return mSedimentCarryAmt.Value; }
            set { mSedimentCarryAmt.Value = value; }
        }

      

        //for 'geological time enhacnement'
        BoolParam mMultiscaleEnable = new BoolParam(true);
        [ConnectionType("Param", "MultiscaleEnable")]
        public bool MultiscaleEnable
        {
            get { return mMultiscaleEnable.Value; }
            set { mMultiscaleEnable.Value = value; }
        }

       

        
        #endregion

        #region init
       public Device_HydraulicErosion()
       {
       }
        public Device_HydraulicErosion(GraphCanvas owningCanvas)
            :
            base(owningCanvas)
        {
            base.Text = "Hydraulic Erosion";
            mColorTop = Color.White;
            mColorBottom = Color.Brown;
            mBorderSize = 1;

            mSize.Width = 60;
            mSize.Height = 20;

            generateConnectionPoints();
            resizeFromConnections();
        }
      public override bool load(MaskDAGGraphNode fromNode)
      {
         Device_HydraulicErosion dc = fromNode as Device_HydraulicErosion;
         mGUID = dc.mGUID;
         draggedByMouse(Location, dc.Location);

         MultiscaleEnable = dc.MultiscaleEnable;
         SedimentCarryAmt = dc.SedimentCarryAmt;
         SoilHardness = dc.SoilHardness;
         Amount = dc.Amount;

         return true;
      }
        #endregion

      [XmlIgnore]
       Random mRand = new Random();
      [XmlIgnore]
      Point[] neighOffset = new Point[]{new Point(-1,-1),new Point(-1,0),new Point(-1,1),
                                        new Point(1,-1),new Point(1,0),new Point(1,1),
                                        new Point(0,-1),new Point(0,1)};
      [XmlIgnore]
      float[] neighWeight = new float[]{(float)Math.Sqrt(2),1,(float)Math.Sqrt(2),
                                        (float)Math.Sqrt(2),1,(float)Math.Sqrt(2),
                                        1,1};

       void multiscaleDisplace(ref DAGMask baseMask, ref DAGMask subMask,ref DAGMask addMask)
        {

           eFilterType type = eFilterType.cFilter_Linear;//eFilterType.cFilter_Nearest;//
            int level = 4;
           // int l = level;
            for (int l = level; l > 0; l--)
            {
                //resize to smaller texture
               DAGMask smallMask = new DAGMask(baseMask.Width >> l, baseMask.Height >> l);
               resizeF32Img(baseMask, smallMask, baseMask.Width, baseMask.Height, smallMask.Width, smallMask.Height, type);

                DAGMask addMaskSmall = new DAGMask(smallMask.Width, smallMask.Height);
                DAGMask subMaskSmall = new DAGMask(smallMask.Width, smallMask.Height);
                resizeF32Img(addMask, addMaskSmall, addMask.Width, addMask.Height, addMaskSmall.Width, addMaskSmall.Height, type);
                resizeF32Img(subMask, subMaskSmall, subMask.Width, subMask.Height, subMaskSmall.Width, subMaskSmall.Height, type);

                //calculate erosion 
                calculateErosion(smallMask, smallMask.Width, smallMask.Height, ref subMaskSmall, ref addMaskSmall, l );
            
                //move back
                DAGMask addMaskBig = new DAGMask(baseMask.Width, baseMask.Height);
                DAGMask subMaskBig = new DAGMask(baseMask.Width, baseMask.Height);
                resizeF32Img(addMaskSmall, addMaskBig, addMaskSmall.Width, addMaskSmall.Height, addMask.Width, addMask.Height, type);
                resizeF32Img(subMaskSmall, subMaskBig, subMaskSmall.Width, subMaskSmall.Height, subMask.Width, subMask.Height, type);

                DAGMask smallMaskBig = new DAGMask(baseMask.Width, baseMask.Height);
                resizeF32Img(smallMask, smallMaskBig, smallMask.Width, smallMask.Height, smallMaskBig.Width, smallMaskBig.Height, type);

                for (int x = 0; x < baseMask.Width; x++)
                {
                   for (int y = 0; y < baseMask.Height; y++)
                    {
                      // baseMask[x, y] = smallMaskBig[x, y];
                        addMask[x, y] = addMaskBig[x, y];
                        subMask[x, y] = subMaskBig[x, y];
                    }
                }
            }
           
        }

      

      List<Point> calculatePath(DAGMask inMask, int width, int height, ref DAGMask subMask, ref DAGMask addMask, Point currPt)
      {
         List<Point> points = new List<Point>();
         points.Add(currPt);
         Point prevPt = currPt;

         int distanceMax = inMask.Width;
         while (distanceMax > 0)
         {
            float currHeight = inMask[currPt.X, currPt.Y] - subMask[currPt.X, currPt.Y] + addMask[currPt.X, currPt.Y];

            bool[] lowerPts = new bool[8];
            float[] diffPts = new float[8];


            //calculate neighbor info
            
            float diffSum = 0;
            bool foundLower = false;
            for (int i = 0; i < neighOffset.Length; i++)
            {
               Point off = neighOffset[i];
               if (currPt.X + off.X < 0 || currPt.X + off.X >= width ||
                  currPt.Y + off.Y < 0 || currPt.Y + off.Y >= height ||
                  (currPt.X + off.X == prevPt.X && currPt.Y + off.Y == prevPt.Y))
               {
                  lowerPts[i] = false;
                  diffPts[i] = 0;
                  continue;
               }

               float nHeight = inMask[currPt.X + off.X, currPt.Y + off.Y] - subMask[currPt.X + off.X, currPt.Y + off.Y] + addMask[currPt.X + off.X, currPt.Y + off.Y];

               if (currHeight > nHeight)
               {
                  lowerPts[i] = true;
                  diffPts[i] = (currHeight - nHeight) * neighWeight[i];
                  diffSum += diffPts[i];
                  foundLower = true;
               }
               else
               {
                  lowerPts[i] = false;
                  diffPts[i] = 0;
               }
            }

            //we fell in a local minima;
            if (!foundLower)
            {
               break;
            }


            //randomly pick one of our next stebs based upon propbability of weight
            int minIndex = 0;
            float randomNum = (float)mRand.NextDouble();
            float runningTotal = 0;
            for (int i = 0; i < neighOffset.Length; i++)
            {
               if (lowerPts[i])
               {
                  float weight = diffPts[i] / diffSum;
                  if (runningTotal + weight > randomNum)
                  {
                     minIndex = i;
                     break;
                  }
                  runningTotal += weight;
               }
            }

            prevPt.X = currPt.X;
            prevPt.Y = currPt.Y;

            currPt.X += neighOffset[minIndex].X;
            currPt.Y += neighOffset[minIndex].Y;

            points.Add(currPt);

            distanceMax--;
            if (distanceMax < 0)
               break;
         };

         return points;
      }

      void addSoil(float soilAmt, int x, int y,ref DAGMask inMask, ref DAGMask subMask, ref DAGMask addMask)
      {

         addMask[x, y] += soilAmt;
         return;
         //if the addition of the soil makes me taller than any neighbor
         //then distribute soil between them and myself.

         //float currHeight = inMask[x, y] - subMask[x, y] + addMask[x, y] + soilAmt;

         //bool[] lowerPts = new bool[8];
         //float[] diffPts = new float[8];


         ////calculate neighbor info
         //float slope = 0;
         //float diffSum = 0;
         //int lowestDiffIdx = 0;
         //float lowestDiff = float.MaxValue;
         //bool foundLower = false;
         //for (int i = 0; i < neighOffset.Length; i++)
         //{
         //   Point off = neighOffset[i];
         //   if (x + off.X < 0 || x + off.X >= inMask.Width ||
         //      y + off.Y < 0 || y + off.Y >= inMask.Height )
         //   {
         //      lowerPts[i] = false;
         //      diffPts[i] = 0;
         //      continue;
         //   }

         //   float nHeight = inMask[x + off.X, y + off.Y] - subMask[x + off.X, y + off.Y] + addMask[x + off.X, y + off.Y];

         //   if (currHeight > nHeight)
         //   {
         //      lowerPts[i] = true;
         //      diffPts[i] = currHeight - nHeight;
         //      diffSum += diffPts[i];
         //      foundLower = true;
         //      if(lowestDiff > diffPts[i])
         //      {
         //         lowestDiff = diffPts[i];
         //         lowestDiffIdx = i;
         //      }
         //   }
         //   else
         //   {
         //      lowerPts[i] = false;
         //      diffPts[i] = 0;
         //   }
         //}

         //if(!foundLower)
         //{
         //   addMask[x, y] += soilAmt;
         //   return;
         //}


         ////distribute soil
         //float amtToDistribute = lowestDiff;
         //addMask[x, y] += soilAmt - lowestDiff;

         //for (int i = 0; i < neighOffset.Length; i++)
         //{
         //   if (lowerPts[i])
         //   {
         //      float weight = diffPts[i] / diffSum;

         //      addMask[x, y] += amtToDistribute * weight;
         //   }
         //}


      }
      void removeSoil(float soilAmt, int x, int y, ref DAGMask inMask, ref DAGMask subMask, ref DAGMask addMask)
      {
         //for visual clairty, make this a 2x2
         subMask[x, y] += soilAmt;

         float hlfSol = soilAmt * 0.5f;
         if (y - 1 > 0) subMask[x, y - 1] += hlfSol;
         if (x - 1 > 0) subMask[x - 1, y] += hlfSol;
         if (y + 1 < inMask.Height) subMask[x, y + 1] += hlfSol;
         if (x + 1 < inMask.Width) subMask[x + 1, y] += hlfSol;
      }
      void transferSoil(DAGMask inMask, List<Point> travelPath, int width, int height, ref DAGMask subMask, ref DAGMask addMask, float slopeBiasScalar)
       {
          float amtOfSoilInStream = 0;
          Point currPt = new Point();
          Point nxtPt = new Point();
          for (int i = 0; i < travelPath.Count-1; i++)
          {
             currPt.X = travelPath[i].X;
             currPt.Y = travelPath[i].Y;
             nxtPt.X = travelPath[i+1].X;
             nxtPt.Y = travelPath[i+1].Y;
            
             //determine sediment saturation based upon slope^2
             float Height = inMask[currPt.X, currPt.Y] - subMask[currPt.X, currPt.Y] + addMask[currPt.X, currPt.Y];
             float nHeight = inMask[nxtPt.X, nxtPt.Y] - subMask[nxtPt.X, nxtPt.Y] + addMask[nxtPt.X, nxtPt.Y];

             float slope = Height - nHeight;

             float cDepositThreshold = 0.00390625f;// 1/256 //CLM MAGIC NUMBER
             if (slope < cDepositThreshold || amtOfSoilInStream > SedimentCarryAmt)
             {
                //deposit soil

                //how much soil should we deposit?
                float soilAmt = (slope * (1.0f - SoilHardness));
                if (amtOfSoilInStream > 0)
                {
                   amtOfSoilInStream -= soilAmt;
                   addSoil(soilAmt, currPt.X, currPt.Y, ref inMask, ref subMask, ref addMask);
                }
             }
             else
             {
                //pickup soil
                float soilAmt = (slope * (1.0f - SoilHardness)); // CLM MAGIC NUMBER
                amtOfSoilInStream += soilAmt;
                removeSoil(soilAmt, currPt.X, currPt.Y, ref inMask, ref subMask, ref addMask);
             }




             if (amtOfSoilInStream < 0)
                break;
          }

          //do we have soil suspended in the stream?
        //  if (amtOfSoilInStream > 0)
         //    addSoil(amtOfSoilInStream, currPt.X, currPt.Y, ref inMask, ref subMask, ref addMask);
       }
      void calculateErosion(DAGMask inMask, int width, int height, ref DAGMask subMask, ref DAGMask addMask, int amtScalar)
        {
            Random rnd = new Random();
            Point prevPt = new Point(rnd.Next(width), rnd.Next(width));
            for (int passCount = 0; passCount < width * Amount; passCount++)
            {
                Point currPt = new Point(rnd.Next(width), rnd.Next(width));

                List<Point> travelPath = calculatePath(inMask, width, height, ref subMask, ref addMask, currPt);
                transferSoil(inMask, travelPath, width, height, ref subMask, ref addMask, 1.0f); 
            }
        }
      
        override public bool computeOutput(ConnectionPoint connPoint, OutputGenerationParams parms)
        {
            if (!verifyInputConnections())
                return false;

            gatherInputAndParameters(parms);


            MaskParam mp = ((MaskParam)(connPoint.ParamType));
           // mp.Value = InputMask.Clone();

            DAGMask baseMask = InputMask.Clone();

            DAGMask addMask = new DAGMask(parms.Width, parms.Height);
            DAGMask subMask = new DAGMask(parms.Width, parms.Height);

            //if we're doing multiscale erosion
            if (MultiscaleEnable)
            {
               //subtract multiscale mask from our base mask before doing erosion
               multiscaleDisplace(ref baseMask, ref subMask, ref addMask);
            }


           //CLM it actually looks better (less noisy) w/o this high fidelity step!
            calculateErosion(baseMask, baseMask.Width, baseMask.Height, ref subMask, ref addMask, 0);


            //calculate our mask 'channels'

            for (int x = 0; x < parms.Width; x++)
            {
                for (int y = 0; y < parms.Height; y++)
                {
                   baseMask[x, y] = baseMask[x, y] + addMask[x, y] - subMask[x, y];
                }
            }

            //need to find which output this connection point is connected to...
            mp.Value = baseMask.Clone();


            return true;
        }

        public enum eFilterType
        {
            cFilter_Nearest = 0,
            cFilter_Linear
        }
       static public void resizeF32Img(DAGMask inputTexture, DAGMask outputTexture, int inputWidth, int inputHeight, int newWidth, int newHeight, eFilterType method)
        {
            if (outputTexture == null)
               outputTexture = new DAGMask(newWidth, newHeight);
            if (inputWidth == newWidth && inputHeight == newHeight)
            {
                    outputTexture = inputTexture.Clone();
                return;
            }
            float xFactor = (float)inputWidth / newWidth;
            float yFactor = (float)inputHeight / newHeight;

            int dstOffset = inputWidth - newWidth;

            //create a new texture of new size

            switch (method)
            {
                case eFilterType.cFilter_Nearest:
                    {
                        int ox, oy;

                      
                        // for each line
                        for (int y = 0; y < newHeight; y++)
                        {
                            // Y coordinate of the nearest point
                            oy = (int)(y * yFactor);

                            // for each pixel
                            for (int x = 0; x < newWidth; x++)
                            {
                                // X coordinate of the nearest point
                                ox = (int)(x * xFactor);

                                int srcIndex = oy * inputWidth + ox;

                                outputTexture[x, y] = inputTexture[ox, oy];

                            }
                            //     dstIndex += dstOffset;
                        }
                        break;
                    }
                case eFilterType.cFilter_Linear:
                    {
                        float ox, oy, dx1, dy1, dx2, dy2;
                        int ox1, oy1, ox2, oy2;
                        int ymax = inputHeight - 1;
                        int xmax = inputWidth - 1;
                        float v1, v2;


                        // for each line
                        for (int y = 0; y < newHeight; y++)
                        {
                            // Y coordinates
                            oy = (float)y * yFactor;
                            oy1 = (int)oy;
                            oy2 = (oy1 == ymax) ? oy1 : oy1 + 1;
                            dy1 = oy - (float)oy1;
                            dy2 = 1.0f - dy1;

                            // for each pixel
                            for (int x = 0; x < newWidth; x++)
                            {
                                // X coordinates
                                ox = (float)x * xFactor;
                                ox1 = (int)ox;
                                ox2 = (ox1 == xmax) ? ox1 : ox1 + 1;
                                dx1 = ox - (float)ox1;
                                dx2 = 1.0f - dx1;


                                // interpolate using 4 points
                                {
                                    v1 = (float)(dx2 * (inputTexture[ox1, oy1]) + dx1 * (inputTexture[ox2, oy1]));
                                    v2 = (float)(dx2 * (inputTexture[ox1, oy2]) + dx1 * (inputTexture[ox2, oy2]));
                                    outputTexture[x, y] = (float)(dy2 * v1 + dy1 * v2);
                                }
                            }
                            //  dstIndex += dstOffset;
                        }
                        break;
                    }
            };
 
        }
     
    }
}
