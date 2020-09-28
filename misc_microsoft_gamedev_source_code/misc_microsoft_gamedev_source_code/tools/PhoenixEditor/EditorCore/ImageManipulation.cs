using System;
using System.Collections.Generic;
using System.Text;

using Microsoft.Ink;
using Microsoft.StylusInput;
using Microsoft.StylusInput.PluginData;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using System.Windows.Forms;
using System.Drawing;

namespace EditorCore
{
   public class ImageManipulation
   {
      //morphology operations
      public enum eFilterType
      {
         cFilter_Nearest = 0,
         cFilter_Linear,
         cFilter_Bicubic
      }
      private static float BiCubicKernel(float x)
      {
         if (x > 2.0f)
            return 0.0f;

         float a, b, c, d;
         float xm1 = x - 1.0f;
         float xp1 = x + 1.0f;
         float xp2 = x + 2.0f;

         a = (xp2 <= 0.0f) ? 0.0f : xp2 * xp2 * xp2;
         b = (xp1 <= 0.0f) ? 0.0f : xp1 * xp1 * xp1;
         c = (x <= 0.0f) ? 0.0f : x * x * x;
         d = (xm1 <= 0.0f) ? 0.0f : xm1 * xm1 * xm1;

         return (0.16666666666666666667f * (a - (4.0f * b) + (6.0f * c) - (4.0f * d)));
      }
      unsafe static public byte[] resizeGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, int newWidth, int newHeight, eFilterType method)
      {
         byte[] newImg = null;
         if (inputWidth == newWidth && inputHeight == newHeight)
         {
            //just clone the image
            newImg = new byte[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            return newImg;
         }
         newImg = new byte[newWidth * newHeight];

         resizeGreyScaleImg(inputTexture, newImg, inputWidth, inputHeight, newWidth, newHeight, method);
         return newImg;
      }
      unsafe static public void resizeGreyScaleImg(byte[] inputTexture, byte []outputTexture,int inputWidth, int inputHeight, int newWidth, int newHeight, eFilterType method)
      {
         if(outputTexture==null)
            outputTexture = new byte[newWidth * newHeight];
         if (inputWidth == newWidth && inputHeight == newHeight)
         {
            return;
         }

         byte[] newImg = outputTexture;

         float xFactor = (float)inputWidth / newWidth;
         float yFactor = (float)inputHeight / newHeight;

         int dstOffset = inputWidth - newWidth;

         //create a new texture of new size
         
         switch (method)
         {
            case eFilterType.cFilter_Nearest:
               {
                  int ox, oy;

                  int dstIndex = 0;
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

                        newImg[dstIndex++] = inputTexture[srcIndex++];

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
                  byte v1, v2;
                  int tp1, tp2;

                  int dstIndex = 0;
                  int srcIndex1 = 0;
                  int srcIndex2 = 0;
                  int srcIndex3 = 0;
                  int srcIndex4 = 0;

                  // for each line
                  for (int y = 0; y < newHeight; y++)
                  {
                     // Y coordinates
                     oy = (float)y * yFactor;
                     oy1 = (int)oy;
                     oy2 = (oy1 == ymax) ? oy1 : oy1 + 1;
                     dy1 = oy - (float)oy1;
                     dy2 = 1.0f - dy1;

                     // get temp pointers
                     tp1 = oy1 * inputWidth;
                     tp2 = oy2 * inputWidth;

                     // for each pixel
                     for (int x = 0; x < newWidth; x++)
                     {
                        // X coordinates
                        ox = (float)x * xFactor;
                        ox1 = (int)ox;
                        ox2 = (ox1 == xmax) ? ox1 : ox1 + 1;
                        dx1 = ox - (float)ox1;
                        dx2 = 1.0f - dx1;

                        // get four points
                        srcIndex1 = (int)(tp1 + ox1);
                        srcIndex2 = (int)(tp1 + ox2);
                        srcIndex3 = (int)(tp2 + ox1);
                        srcIndex4 = (int)(tp2 + ox2);

                        // interpolate using 4 points
                        {
                           v1 = (byte)(dx2 * (inputTexture[srcIndex1]) + dx1 * (inputTexture[srcIndex2]));
                           v2 = (byte)(dx2 * (inputTexture[srcIndex3]) + dx1 * (inputTexture[srcIndex4]));
                           newImg[dstIndex++] = (byte)(dy2 * v1 + dy1 * v2);
                        }
                     }
                     //  dstIndex += dstOffset;
                  }
                  break;
               }
            case eFilterType.cFilter_Bicubic:
               {

                  float ox, oy, dx, dy, k1, k2;
                  float r, g, b;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  int dstIndex = 0;
                  int srcIndex = 0;


                  for (int y = 0; y < newHeight; y++)
                  {
                     // Y coordinates
                     oy = (float)y * yFactor - 0.5f;
                     oy1 = (int)oy;
                     dy = oy - (float)oy1;

                     for (int x = 0; x < newWidth; x++, dstIndex++)
                     {
                        // X coordinates
                        ox = (float)x * xFactor - 0.5f;
                        ox1 = (int)ox;
                        dx = ox - (float)ox1;

                        g = 0;

                        for (int n = -1; n < 3; n++)
                        {
                           k1 = BiCubicKernel(dy - (float)n);

                           oy2 = oy1 + n;
                           if (oy2 < 0)
                              oy2 = 0;
                           if (oy2 > ymax)
                              oy2 = ymax;

                           for (int m = -1; m < 3; m++)
                           {
                              k2 = k1 * BiCubicKernel((float)m - dx);

                              ox2 = ox1 + m;
                              if (ox2 < 0)
                                 ox2 = 0;
                              if (ox2 > xmax)
                                 ox2 = xmax;

                              g += k2 * inputTexture[oy2 * inputWidth + ox2];
                           }
                        }
                        newImg[dstIndex] = (byte)g;
                     }
                     //   dstIndex += dstOffset;
                  }

                  break;
               }
         };

         newImg = null ;
      }
      unsafe static public byte[] rotateGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, float rotAngle, bool keepOrigSize, out int outputWidth, out int outputHeight, eFilterType filterMethod)
      {
         byte[] newImg = null;
         if (rotAngle == 0)
         {
            //just clone the image
            newImg = new byte[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            outputWidth = inputWidth;
            outputHeight = inputHeight;
            return newImg;
         }

         double angleRad = -rotAngle * Math.PI / 180;
         double angleCos = Math.Cos(angleRad);
         double angleSin = Math.Sin(angleRad);

         double halfWidth = (double)inputWidth / 2;
         double halfHeight = (double)inputHeight / 2;

         double halfNewWidth, halfNewHeight;
         int newWidth, newHeight;

         if (keepOrigSize)
         {
            halfNewWidth = halfWidth;
            halfNewHeight = halfHeight;

            newWidth = inputWidth;
            newHeight = inputHeight;
         }
         else
         {
            // rotate corners
            double cx1 = halfWidth * angleCos;
            double cy1 = halfWidth * angleSin;

            double cx2 = halfWidth * angleCos - halfHeight * angleSin;
            double cy2 = halfWidth * angleSin + halfHeight * angleCos;

            double cx3 = -halfHeight * angleSin;
            double cy3 = halfHeight * angleCos;

            double cx4 = 0;
            double cy4 = 0;

            halfNewWidth = Math.Max(Math.Max(cx1, cx2), Math.Max(cx3, cx4)) - Math.Min(Math.Min(cx1, cx2), Math.Min(cx3, cx4));
            halfNewHeight = Math.Max(Math.Max(cy1, cy2), Math.Max(cy3, cy4)) - Math.Min(Math.Min(cy1, cy2), Math.Min(cy3, cy4));

            newWidth = (int)(halfNewWidth * 2 + 0.5);
            newHeight = (int)(halfNewHeight * 2 + 0.5);
         }


         outputWidth = newWidth;
         outputHeight = newHeight;

         newImg = new byte[newWidth * newHeight];
         int dstOffset = inputWidth - newWidth;

         byte fill = 0;
         switch (filterMethod)
         {
            case eFilterType.cFilter_Nearest:
               {
                  double cx, cy;
                  int ox, oy;
                  int dstIndex = 0;

                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {
                           // coordinate of the nearest point
                           ox = (int)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (int)(-angleSin * cx + angleCos * cy + halfHeight);

                           if ((ox < 0) || (oy < 0) || (ox >= inputWidth) || (oy >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              newImg[dstIndex] = inputTexture[oy * inputWidth + ox];
                           }
                           cx++;
                        }
                        cy++;
                        //  dstIndex += dstOffset;
                     }
                  }
                  break;
               }
            case eFilterType.cFilter_Linear:
               {
                  double cx, cy;
                  float ox, oy, dx1, dy1, dx2, dy2;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  byte v1, v2;
                  int p1, p2, p3, p4;

                  int dstIndex = 0;
                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {
                           ox = (float)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (float)(-angleSin * cx + angleCos * cy + halfHeight);

                           // top-left coordinate
                           ox1 = (int)ox;
                           oy1 = (int)oy;

                           if ((ox1 < 0) || (oy1 < 0) || (ox1 >= inputWidth) || (oy1 >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              // bottom-right coordinate
                              ox2 = (ox1 == xmax) ? ox1 : ox1 + 1;
                              oy2 = (oy1 == ymax) ? oy1 : oy1 + 1;

                              if ((dx1 = ox - (float)ox1) < 0)
                                 dx1 = 0;
                              dx2 = 1.0f - dx1;

                              if ((dy1 = oy - (float)oy1) < 0)
                                 dy1 = 0;
                              dy2 = 1.0f - dy1;

                              p1 = oy1 * inputWidth;
                              p2 = oy2 * inputWidth;

                              // interpolate using 4 points
                              v1 = (byte)(dx2 * inputTexture[p1 + ox1] + dx1 * inputTexture[p1 + ox2]);
                              v2 = (byte)(dx2 * inputTexture[p2 + ox1] + dx1 * inputTexture[p2 + ox2]);
                              newImg[dstIndex] = (byte)(dy2 * v1 + dy1 * v2);
                           }
                           cx++;
                        }
                        cy++;
                        //     dstIndex += dstOffset;
                     }
                  }
                  break;
               }
            case eFilterType.cFilter_Bicubic:
               {
                  double cx, cy;
                  float ox, oy, dx, dy, k1, k2;
                  float r, g, b;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  byte* p;

                  int dstIndex = 0;
                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {

                           ox = (float)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (float)(-angleSin * cx + angleCos * cy + halfHeight);

                           ox1 = (int)ox;
                           oy1 = (int)oy;

                           if ((ox1 < 0) || (oy1 < 0) || (ox1 >= inputWidth) || (oy1 >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              dx = ox - (float)ox1;
                              dy = oy - (float)oy1;

                              g = 0;

                              for (int n = -1; n < 3; n++)
                              {
                                 k1 = BiCubicKernel(dy - (float)n);

                                 oy2 = oy1 + n;
                                 if (oy2 < 0)
                                    oy2 = 0;
                                 if (oy2 > ymax)
                                    oy2 = ymax;

                                 for (int m = -1; m < 3; m++)
                                 {
                                    k2 = k1 * BiCubicKernel((float)m - dx);

                                    ox2 = ox1 + m;
                                    if (ox2 < 0)
                                       ox2 = 0;
                                    if (ox2 > xmax)
                                       ox2 = xmax;

                                    g += k2 * inputTexture[oy2 * inputWidth + ox2];
                                 }
                              }
                              newImg[dstIndex] = (byte)g;
                           }
                           cx++;
                        }
                        cy++;
                        //  dstIndex += dstOffset;
                     }
                  }
                  break;
               }

         };

         return newImg;
      }
      unsafe static public byte[] translateGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, int xPixelsToTranslate, int yPixelsToTranslate)
      {
         byte[] newImg = null;
         if (xPixelsToTranslate == 0 && yPixelsToTranslate == 0)
         {
            //just clone the image
            newImg = new byte[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            return newImg;
         }

         newImg = new byte[inputWidth * inputHeight];
         for (int i = 0; i < inputHeight * inputWidth; i++)
            newImg[i] = 0;


         //walk each pixel in the src img, and move it to it's location in the dst img
         for (int x = 0; x < inputWidth; x++)
         {
            for (int y = 0; y < inputHeight; y++)
            {
               if (x + xPixelsToTranslate >= inputWidth || y + yPixelsToTranslate >= inputHeight || x + xPixelsToTranslate < 0 || y + yPixelsToTranslate < 0)
                  continue;

               int srcPixelIndex = x + inputWidth * y;
               int dstPixelIndex = (x + xPixelsToTranslate) + inputWidth * (y + yPixelsToTranslate);

               newImg[dstPixelIndex] = inputTexture[srcPixelIndex];

            }
         }

         return newImg;
      }
      unsafe static public byte[] flipGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, bool flipX, bool flipY)
      {
         byte[] newImg = null;
         if (flipX == false && flipY == false)
         {
            //just clone the image
            newImg = new byte[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            return newImg;
         }

         newImg = new byte[inputWidth * inputHeight];
         for (int i = 0; i < inputHeight * inputWidth; i++)
            newImg[i] = 0;


         //walk each pixel in the src img, and move it to it's location in the dst img
         if (flipX)
         {
            for (int x = 0; x < inputWidth; x++)
            {
               for (int y = 0; y < inputHeight; y++)
               {
                  int srcPixelIndex = x + inputWidth * y;
                  int dstPixelIndex = (inputWidth - x) + inputWidth * y;

                  newImg[dstPixelIndex] = inputTexture[srcPixelIndex];
               }
            }
         }

         if (flipY)
         {
            for (int x = 0; x < inputWidth; x++)
            {
               for (int y = 0; y < inputHeight; y++)
               {
                  int srcPixelIndex = x + inputWidth * y;
                  int dstPixelIndex = x + inputWidth * (inputHeight - y);

                  newImg[dstPixelIndex] = inputTexture[srcPixelIndex];
               }
            }
         }


         return newImg;
      }
      unsafe static public byte[] cropGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, int rectX0, int rectX1, int rectY0, int rectY1)
      {
         byte[] newImg = null;

         //swap inverted values
         if (rectX0 > rectX1)
         {
            int rc = rectX0;
            rectX0 = rectX1;
            rectX1 = rc;
         }
         if (rectY0 > rectY1)
         {
            int rc = rectY0;
            rectY0 = rectY1;
            rectY1 = rc;
         }


         if (rectX0 == 0 && rectY0 == 0 && rectX1 == inputWidth && rectY1 == inputHeight)
         {
            newImg = new byte[inputTexture.Length];
            inputTexture.CopyTo(newImg, 0);
            return newImg;
         }

         int newWidth = rectX1 - rectX0;
         int newHeight = rectY1 - rectY0;

         newImg = new byte[newWidth * newHeight];
         for (int x = 0; x < newWidth; x++)
         {
            for (int y = 0; y < newHeight; y++)
            {
               int srcIndex = (x + rectX0) + inputWidth * (y + rectY0);
               int dstIndex = x + newWidth * y;

               newImg[dstIndex] = inputTexture[srcIndex];
            }
         }

         return newImg;
      }
      unsafe static public byte[] pasteGreyScaleImg(byte[] texA, int texAWidth, int texAHeight,
                                                     byte[] texB, int texBWidth, int texBHeight,
                                                     int texBXPixelsToTranslate, int texBYPixelsToTranslate)
      {
         byte[] newImg = new byte[texAWidth * texBWidth];
         texA.CopyTo(newImg, 0);

         //B will not influence A
         if (texBXPixelsToTranslate + texBWidth < 0 || texBYPixelsToTranslate + texBHeight < 0 ||
            texBXPixelsToTranslate > texAWidth || texBYPixelsToTranslate > texAHeight)
            return newImg;

         for (int i = 0; i < texBWidth; i++)
         {
            for (int j = 0; j < texBHeight; j++)
            {
               int aXIndex = texBXPixelsToTranslate + i;
               int aYIndex = texBYPixelsToTranslate + j;
               int aIndex = aXIndex + texAWidth * aYIndex;

               int bXIndex = i;
               int bYIndex = j;
               int bIndex = i + texBWidth * j;

               if (aXIndex < 0 || aYIndex < 0 ||
                  aXIndex >= texAWidth || aYIndex >= texBWidth)
                  continue;

               texA[aIndex] = texB[bIndex];
            }
         }

         return newImg;
      }

      //binary operations
      public enum eBinaryOperation
      {
         cBinary_Add = 0,
         cBinary_Subtract,
         cBinary_Multiply,
         cBinary_Difference,
         cBinary_Max,
         cBinary_Min,
      }
      unsafe static public byte[] binaryGreyScaleImgs(byte[] texA, int texAWidth, int texAHeight,
                                                      byte[] texB, int texBWidth, int texBHeight,
                                                      out int outputWidth, out int outputHeight,
                                                      eBinaryOperation operation)
      {
         int newWidth = Math.Max(texAWidth, texBWidth);
         int newHeight = Math.Max(texAHeight, texBHeight);

         outputWidth = newWidth;
         outputHeight = newHeight;
         byte[] newImg = new byte[newWidth * newHeight];

         for (int x = 0; x < newWidth; x++)
         {
            for (int y = 0; y < newWidth; y++)
            {
               byte imgAContrib = 0;
               byte imgBContrib = 0;

               if (x < texAWidth && y < texAHeight)
                  imgAContrib = texA[x + texAWidth * y];

               if (x < texBWidth && y < texBHeight)
                  imgBContrib = texB[x + texBWidth * y];

               int dstIndex = x + newWidth * y;

               switch (operation)
               {
                  case eBinaryOperation.cBinary_Add:
                     newImg[dstIndex] = (byte)BMathLib.Clamp(imgAContrib + imgBContrib, 0, byte.MaxValue);
                     break;
                  case eBinaryOperation.cBinary_Subtract:
                     newImg[dstIndex] = (byte)BMathLib.Clamp(imgAContrib - imgBContrib, 0, byte.MaxValue);
                     break;
                  case eBinaryOperation.cBinary_Multiply:
                     float newVal = (imgAContrib / 255.0f) * (imgBContrib / 255.0f) * 255;
                     newImg[dstIndex] = (byte)BMathLib.Clamp(newVal, 0, byte.MaxValue);
                     break;
                  case eBinaryOperation.cBinary_Difference:
                     newImg[dstIndex] = (byte)BMathLib.Clamp(Math.Abs(imgAContrib - imgBContrib), 0, byte.MaxValue);
                     break;
                  case eBinaryOperation.cBinary_Max:
                     newImg[dstIndex] = (byte)Math.Max(imgAContrib, imgBContrib);
                     break;
                  case eBinaryOperation.cBinary_Min:
                     newImg[dstIndex] = (byte)Math.Min(imgAContrib, imgBContrib);
                     break;
               };

            }
         }


         return newImg;
      }

      //value operations - will take input value and apply it to every pixel
      public enum eValueOperation
      {
         cValue_Add = 0,
         cValue_Subtract_ValueFrom,
         cValue_Subtract_FromValue,
         cValue_Multiply,
         cValue_Difference,
         cValue_Max,
         cValue_Min,
         cValue_Threshold,
         cValue_Set,
      }
      unsafe static public byte[] valueGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, eValueOperation operation, byte value)
      {
         byte[] newImg = new byte[inputWidth * inputHeight];

         for (int x = 0; x < inputWidth * inputHeight; x++)
         {
            byte imgAContrib = inputTexture[x];
            switch (operation)
            {
               case eValueOperation.cValue_Set:
                  newImg[x] = (byte)BMathLib.Clamp(value, 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Add:
                  newImg[x] = (byte)BMathLib.Clamp(imgAContrib + value, 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Subtract_ValueFrom:
                  newImg[x] = (byte)BMathLib.Clamp(imgAContrib - value, 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Subtract_FromValue:
                  newImg[x] = (byte)BMathLib.Clamp(value - imgAContrib, 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Multiply:
                  float newVal = (imgAContrib / 255.0f) * (value / 255.0f) * 255;
                  newImg[x] = (byte)BMathLib.Clamp(newVal, 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Difference:
                  newImg[x] = (byte)BMathLib.Clamp(Math.Abs(imgAContrib - value), 0, byte.MaxValue);
                  break;
               case eValueOperation.cValue_Max:
                  newImg[x] = (byte)Math.Max(imgAContrib, value);
                  break;
               case eValueOperation.cValue_Min:
                  newImg[x] = (byte)Math.Min(imgAContrib, value);
                  break;
               case eValueOperation.cValue_Threshold:
                  newImg[x] = imgAContrib > value ? imgAContrib : (byte)0;
                  break;
            };


         }


         return newImg;
      }
      unsafe static public byte[] invertGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight)
      {
         return valueGreyScaleImg(inputTexture, inputWidth, inputHeight, eValueOperation.cValue_Subtract_FromValue, 255);
      }

      //will alpha blend between two greyscale imgs
      unsafe static public byte[] blendGreyScaleImgs(byte[] texA, int texAWidth, int texAHeight, byte[] texB, int texBWidth, int texBHeight, float blendAmt, out int outputWidth, out int outputHeight)
      {
         int newWidth = Math.Max(texAWidth, texBWidth);
         int newHeight = Math.Max(texAHeight, texBHeight);

         outputWidth = newWidth;
         outputHeight = newHeight;

         blendAmt = BMathLib.Clamp(blendAmt, 0, 1);

         byte[] newImg = new byte[newWidth * newHeight];

         for (int x = 0; x < newWidth; x++)
         {
            for (int y = 0; y < newWidth; y++)
            {
               byte imgAContrib = 0;
               byte imgBContrib = 0;

               if (x < texAWidth && y < texAHeight)
                  imgAContrib = texA[x + texAWidth * y];

               if (x < texBWidth && y < texBHeight)
                  imgBContrib = texB[x + texBWidth * y];

               int dstIndex = x + newWidth * y;

               newImg[dstIndex] = (byte)BMathLib.Clamp((blendAmt * imgAContrib) + ((1 - blendAmt) * imgBContrib), 0, byte.MaxValue);

            }
         }


         return newImg;
      }

      //will set the border of the image to a value
      unsafe static public byte[] borderGreyScaleImg(byte[] inputTexture, int inputWidth, int inputHeight, int borderWidth, byte borderValue)
      {
         byte[] newImg = new byte[inputWidth * inputHeight];
         for (int i = 0; i < inputHeight * inputWidth; i++)
            newImg[i] = inputTexture[i];

         if (borderWidth == 0)
            return newImg;

         //top & bottom border
         for (int x = 0; x < inputWidth; x++)
         {
            for (int y = 0; y < borderWidth; y++)
            {
               //top
               int dstIndex = x + inputWidth * y;
               newImg[dstIndex] = borderValue;

               //bottom
               dstIndex = x + inputWidth * ((inputHeight - 1) - y);
               newImg[dstIndex] = borderValue;
            }
         }

         //left & right border
         for (int y = 0; y < inputHeight; y++)
         {
            for (int x = 0; x < borderWidth; x++)
            {
               //left
               int dstIndex = x + inputWidth * y;
               newImg[dstIndex] = borderValue;

               //right
               dstIndex = ((inputWidth - 1) - x) + inputWidth * y;
               newImg[dstIndex] = borderValue;
            }
         }

         return newImg;
      }

      public enum eImageType
      {
         eFormat_A8 = 0,
         eFormat_R8G8B8,
         eFormat_R8G8B8A8
      }
      public enum eDesiredChannel
      {
         eChannel_R = 0,
         eChannel_G,
         eChannel_B,
         eChannel_A
      }
      unsafe static public byte[] extractChannelFromImg(byte[] inputTexture, int inputWidth, int inputHeight, eImageType inputImgType, eDesiredChannel desiredChannel)
      {
         byte[] newImg = new byte[inputWidth * inputHeight];


         switch (inputImgType)
         {
            case eImageType.eFormat_A8:
               for (int i = 0; i < inputWidth * inputHeight; i++)
                  newImg[i] = inputTexture[i];
               break;
            case eImageType.eFormat_R8G8B8:
               for (int i = 0; i < inputWidth * inputHeight; i++)
                  newImg[i] = inputTexture[(i * 3) + ((int)desiredChannel)];
               break;
            case eImageType.eFormat_R8G8B8A8:
               for (int i = 0; i < inputWidth * inputHeight; i++)
                  newImg[i] = inputTexture[(i * 4) + ((int)desiredChannel)];
               break;
         }


         return newImg;
      }



      //CLM I DON'T LIKE DOING THIS
      //i'd like to use generics, but not sure how in C#?
      unsafe static public float[] resizeF32Img(float[] inputTexture, int inputWidth, int inputHeight, int newWidth, int newHeight, eFilterType method)
      {
         float[] newImg = null;
         if (inputWidth == newWidth && inputHeight == newHeight)
         {
            //just clone the image
            newImg = new float[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            return newImg;
         }
         newImg = new float[newWidth * newHeight];

         resizeF32Img(inputTexture, newImg, inputWidth, inputHeight, newWidth, newHeight, method);
         return newImg;
      }
      unsafe static public void resizeF32Img(float[] inputTexture, float[] outputTexture, int inputWidth, int inputHeight, int newWidth, int newHeight, eFilterType method)
      {
         if (outputTexture == null)
            outputTexture = new float[newWidth * newHeight];
         if (inputWidth == newWidth && inputHeight == newHeight)
         {
            for (int i = 0; i < inputHeight * inputWidth; i++)
               outputTexture[i] = inputTexture[i];
            return;
         }

         float[] newImg = outputTexture;


         float xFactor = (float)inputWidth / newWidth;
         float yFactor = (float)inputHeight / newHeight;

         int dstOffset = inputWidth - newWidth;

         //create a new texture of new size
         
         switch (method)
         {
            case eFilterType.cFilter_Nearest:
               {
                  int ox, oy;

                  int dstIndex = 0;
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

                        newImg[dstIndex++] = inputTexture[srcIndex++];

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
                  int tp1, tp2;

                  int dstIndex = 0;
                  int srcIndex1 = 0;
                  int srcIndex2 = 0;
                  int srcIndex3 = 0;
                  int srcIndex4 = 0;

                  // for each line
                  for (int y = 0; y < newHeight; y++)
                  {
                     // Y coordinates
                     oy = (float)y * yFactor;
                     oy1 = (int)oy;
                     oy2 = (oy1 == ymax) ? oy1 : oy1 + 1;
                     dy1 = oy - (float)oy1;
                     dy2 = 1.0f - dy1;

                     // get temp pointers
                     tp1 = oy1 * inputWidth;
                     tp2 = oy2 * inputWidth;

                     // for each pixel
                     for (int x = 0; x < newWidth; x++)
                     {
                        // X coordinates
                        ox = (float)x * xFactor;
                        ox1 = (int)ox;
                        ox2 = (ox1 == xmax) ? ox1 : ox1 + 1;
                        dx1 = ox - (float)ox1;
                        dx2 = 1.0f - dx1;

                        // get four points
                        srcIndex1 = (int)(tp1 + ox1);
                        srcIndex2 = (int)(tp1 + ox2);
                        srcIndex3 = (int)(tp2 + ox1);
                        srcIndex4 = (int)(tp2 + ox2);

                        // interpolate using 4 points
                        {
                           v1 = (float)(dx2 * (inputTexture[srcIndex1]) + dx1 * (inputTexture[srcIndex2]));
                           v2 = (float)(dx2 * (inputTexture[srcIndex3]) + dx1 * (inputTexture[srcIndex4]));
                           newImg[dstIndex++] = (float)(dy2 * v1 + dy1 * v2);
                        }
                     }
                     //  dstIndex += dstOffset;
                  }
                  break;
               }
            case eFilterType.cFilter_Bicubic:
               {

                  float ox, oy, dx, dy, k1, k2;
                  float r, g, b;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  int dstIndex = 0;
                  int srcIndex = 0;


                  for (int y = 0; y < newHeight; y++)
                  {
                     // Y coordinates
                     oy = (float)y * yFactor - 0.5f;
                     oy1 = (int)oy;
                     dy = oy - (float)oy1;

                     for (int x = 0; x < newWidth; x++, dstIndex++)
                     {
                        // X coordinates
                        ox = (float)x * xFactor - 0.5f;
                        ox1 = (int)ox;
                        dx = ox - (float)ox1;

                        g = 0;

                        for (int n = -1; n < 3; n++)
                        {
                           k1 = BiCubicKernel(dy - (float)n);

                           oy2 = oy1 + n;
                           if (oy2 < 0)
                              oy2 = 0;
                           if (oy2 > ymax)
                              oy2 = ymax;

                           for (int m = -1; m < 3; m++)
                           {
                              k2 = k1 * BiCubicKernel((float)m - dx);

                              ox2 = ox1 + m;
                              if (ox2 < 0)
                                 ox2 = 0;
                              if (ox2 > xmax)
                                 ox2 = xmax;

                              g += k2 * inputTexture[oy2 * inputWidth + ox2];
                           }
                        }
                        newImg[dstIndex] = (float)g;
                     }
                     //   dstIndex += dstOffset;
                  }

                  break;
               }
         };

      }
      unsafe static public float[] rotateF32Img(float[] inputTexture, int inputWidth, int inputHeight, float rotAngle, bool keepOrigSize, out int outputWidth, out int outputHeight, eFilterType filterMethod)
      {
         float[] newImg = null;
         if (rotAngle == 0)
         {
            //just clone the image
            newImg = new float[inputWidth * inputHeight];
            for (int i = 0; i < inputHeight * inputWidth; i++)
               newImg[i] = inputTexture[i];

            outputWidth = inputWidth;
            outputHeight = inputHeight;
            return newImg;
         }

         double angleRad = -rotAngle * Math.PI / 180;
         double angleCos = Math.Cos(angleRad);
         double angleSin = Math.Sin(angleRad);

         double halfWidth = (double)inputWidth / 2;
         double halfHeight = (double)inputHeight / 2;

         double halfNewWidth, halfNewHeight;
         int newWidth, newHeight;

         if (keepOrigSize)
         {
            halfNewWidth = halfWidth;
            halfNewHeight = halfHeight;

            newWidth = inputWidth;
            newHeight = inputHeight;
         }
         else
         {
            // rotate corners
            double cx1 = halfWidth * angleCos;
            double cy1 = halfWidth * angleSin;

            double cx2 = halfWidth * angleCos - halfHeight * angleSin;
            double cy2 = halfWidth * angleSin + halfHeight * angleCos;

            double cx3 = -halfHeight * angleSin;
            double cy3 = halfHeight * angleCos;

            double cx4 = 0;
            double cy4 = 0;

            halfNewWidth = Math.Max(Math.Max(cx1, cx2), Math.Max(cx3, cx4)) - Math.Min(Math.Min(cx1, cx2), Math.Min(cx3, cx4));
            halfNewHeight = Math.Max(Math.Max(cy1, cy2), Math.Max(cy3, cy4)) - Math.Min(Math.Min(cy1, cy2), Math.Min(cy3, cy4));

            newWidth = (int)(halfNewWidth * 2 + 0.5);
            newHeight = (int)(halfNewHeight * 2 + 0.5);
         }


         outputWidth = newWidth;
         outputHeight = newHeight;

         newImg = new float[newWidth * newHeight];
         int dstOffset = inputWidth - newWidth;

         float fill = 0;
         switch (filterMethod)
         {
            case eFilterType.cFilter_Nearest:
               {
                  double cx, cy;
                  int ox, oy;
                  int dstIndex = 0;

                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {
                           // coordinate of the nearest point
                           ox = (int)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (int)(-angleSin * cx + angleCos * cy + halfHeight);

                           if ((ox < 0) || (oy < 0) || (ox >= inputWidth) || (oy >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              newImg[dstIndex] = inputTexture[oy * inputWidth + ox];
                           }
                           cx++;
                        }
                        cy++;
                        //  dstIndex += dstOffset;
                     }
                  }
                  break;
               }
            case eFilterType.cFilter_Linear:
               {
                  double cx, cy;
                  float ox, oy, dx1, dy1, dx2, dy2;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  float v1, v2;
                  int p1, p2, p3, p4;

                  int dstIndex = 0;
                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {
                           ox = (float)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (float)(-angleSin * cx + angleCos * cy + halfHeight);

                           // top-left coordinate
                           ox1 = (int)ox;
                           oy1 = (int)oy;

                           if ((ox1 < 0) || (oy1 < 0) || (ox1 >= inputWidth) || (oy1 >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              // bottom-right coordinate
                              ox2 = (ox1 == xmax) ? ox1 : ox1 + 1;
                              oy2 = (oy1 == ymax) ? oy1 : oy1 + 1;

                              if ((dx1 = ox - (float)ox1) < 0)
                                 dx1 = 0;
                              dx2 = 1.0f - dx1;

                              if ((dy1 = oy - (float)oy1) < 0)
                                 dy1 = 0;
                              dy2 = 1.0f - dy1;

                              p1 = oy1 * inputWidth;
                              p2 = oy2 * inputWidth;

                              // interpolate using 4 points
                              v1 = (float)(dx2 * inputTexture[p1 + ox1] + dx1 * inputTexture[p1 + ox2]);
                              v2 = (float)(dx2 * inputTexture[p2 + ox1] + dx1 * inputTexture[p2 + ox2]);
                              newImg[dstIndex] = (float)(dy2 * v1 + dy1 * v2);
                           }
                           cx++;
                        }
                        cy++;
                        //     dstIndex += dstOffset;
                     }
                  }
                  break;
               }
            case eFilterType.cFilter_Bicubic:
               {
                  double cx, cy;
                  float ox, oy, dx, dy, k1, k2;
                  float r, g, b;
                  int ox1, oy1, ox2, oy2;
                  int ymax = inputHeight - 1;
                  int xmax = inputWidth - 1;
                  byte* p;

                  int dstIndex = 0;
                  {
                     cy = -halfNewHeight;
                     for (int y = 0; y < newHeight; y++)
                     {
                        cx = -halfNewWidth;
                        for (int x = 0; x < newWidth; x++, dstIndex++)
                        {

                           ox = (float)(angleCos * cx + angleSin * cy + halfWidth);
                           oy = (float)(-angleSin * cx + angleCos * cy + halfHeight);

                           ox1 = (int)ox;
                           oy1 = (int)oy;

                           if ((ox1 < 0) || (oy1 < 0) || (ox1 >= inputWidth) || (oy1 >= inputHeight))
                           {
                              newImg[dstIndex] = fill;
                           }
                           else
                           {
                              dx = ox - (float)ox1;
                              dy = oy - (float)oy1;

                              g = 0;

                              for (int n = -1; n < 3; n++)
                              {
                                 k1 = BiCubicKernel(dy - (float)n);

                                 oy2 = oy1 + n;
                                 if (oy2 < 0)
                                    oy2 = 0;
                                 if (oy2 > ymax)
                                    oy2 = ymax;

                                 for (int m = -1; m < 3; m++)
                                 {
                                    k2 = k1 * BiCubicKernel((float)m - dx);

                                    ox2 = ox1 + m;
                                    if (ox2 < 0)
                                       ox2 = 0;
                                    if (ox2 > xmax)
                                       ox2 = xmax;

                                    g += k2 * inputTexture[oy2 * inputWidth + ox2];
                                 }
                              }
                              newImg[dstIndex] = (float)g;
                           }
                           cx++;
                        }
                        cy++;
                        //  dstIndex += dstOffset;
                     }
                  }
                  break;
               }

         };

         return newImg;
      }

      unsafe static public void resizeD3DTexture(Texture srcTex, int inputWidth, int inputHeight, ref Texture dstTex, int newWidth, int newHeight, eFilterType method)
      {

         byte[] imgDatFull = new byte[inputHeight * inputWidth * 4];
         GraphicsStream texstream = srcTex.LockRectangle(0, LockFlags.None);
         texstream.Read(imgDatFull, 0, inputWidth * inputHeight * 4);
         srcTex.UnlockRectangle(0);

         byte[] chnR = new byte[inputHeight * inputWidth];
         byte[] chnG = new byte[inputHeight * inputWidth];
         byte[] chnB = new byte[inputHeight * inputWidth];
         chnR = ImageManipulation.extractChannelFromImg(imgDatFull, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_R);
         chnG = ImageManipulation.extractChannelFromImg(imgDatFull, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_G);
         chnB = ImageManipulation.extractChannelFromImg(imgDatFull, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_B);

         byte[] RchnR = new byte[newHeight * newWidth];
         byte[] RchnG = new byte[newHeight * newWidth];
         byte[] RchnB = new byte[newHeight * newWidth];
         RchnR = ImageManipulation.resizeGreyScaleImg(chnR, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);
         RchnG = ImageManipulation.resizeGreyScaleImg(chnG, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);
         RchnB = ImageManipulation.resizeGreyScaleImg(chnB, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);


         texstream = dstTex.LockRectangle(0, LockFlags.None);
         byte* dImg = (byte*)texstream.InternalDataPointer;
         for (int i = 0; i < newWidth; i++)
         {
            for (int j = 0; j < newHeight; j++)
            {
               int idx = i + j * newHeight;
               dImg[idx + 0] = 255;
               dImg[idx + 1] = RchnR[idx];
               dImg[idx + 2] = RchnG[idx];
               dImg[idx + 3] = RchnB[idx];
            }
         }
         texstream.Read(imgDatFull, 0, inputWidth * inputHeight * 4);

         dstTex.UnlockRectangle(0);


         RchnB = null;
         RchnG = null;
         RchnR = null;
         chnB = null;
         chnG = null;
         chnR = null;
         imgDatFull = null;
      }
      unsafe static public void resizeRGBATexture(byte[] srcTex, int inputWidth, int inputHeight, ref byte[] dstTex, int newWidth, int newHeight, eFilterType method)
      {

         byte[] chnR = new byte[inputHeight * inputWidth];
         byte[] chnG = new byte[inputHeight * inputWidth];
         byte[] chnB = new byte[inputHeight * inputWidth];
         chnR = ImageManipulation.extractChannelFromImg(srcTex, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_R);
         chnG = ImageManipulation.extractChannelFromImg(srcTex, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_G);
         chnB = ImageManipulation.extractChannelFromImg(srcTex, inputWidth, inputHeight, ImageManipulation.eImageType.eFormat_R8G8B8A8, ImageManipulation.eDesiredChannel.eChannel_B);

         byte[] RchnR = new byte[newHeight * newWidth];
         byte[] RchnG = new byte[newHeight * newWidth];
         byte[] RchnB = new byte[newHeight * newWidth];
         RchnR = ImageManipulation.resizeGreyScaleImg(chnR, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);
         RchnG = ImageManipulation.resizeGreyScaleImg(chnG, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);
         RchnB = ImageManipulation.resizeGreyScaleImg(chnB, inputWidth, inputHeight, newWidth, newHeight, ImageManipulation.eFilterType.cFilter_Linear);

         dstTex = new byte[newWidth * newHeight * 4];
         for (int j = 0; j < newHeight; j++)
         {
            for (int i = 0; i < newWidth; i++)   
            {
               int idx = i + j * newHeight;
               dstTex[(idx*4) + 3] = 255;
               dstTex[(idx*4) + 0] = RchnR[idx];
               dstTex[(idx*4) + 1] = RchnG[idx];
               dstTex[(idx*4) + 2] = RchnB[idx];
            }
         }

         RchnB = null;
         RchnG = null;
         RchnR = null;
         chnB = null;
         chnG = null;
         chnR = null;
      }


      #region Advanced resampling filter
      public enum eResamplerMethod
      {
         cRS_Box = 0,           //AKA Nearest Neighbor (4bit compression OK)
         cRS_Triangle,        //AKA Bilnear           (4bit compression OK)
         cRS_Hermite,         //Unknown what it does exactly but is as fast is the Triangle Filter.   
         cRS_Bell,            //This filter blurs the image at the same time it resizes. If you want to smooth the video and your source video is very noisy use this.
         cRS_CubicBSpline,    //One step further from 'Bell Filter'. A bit slower and more blurred image but less noisy and less sharp though.
         cRS_Lanczos3,
         cRS_Lanczos8,
         cRS_Mitchell,        //Similar to bicubic filtering but it's a bit slower. If you want similar performance to bicubic filtering but your source video is noisy, try this one.
         cRS_Cosine,
         cRS_CatmullRom,
         cRS_Quadratic,
         cRS_QuadraticBSpline,
         cRS_CubicConvolution,
      };
      abstract class ResamplingFilter
      {

         public double defaultFilterRadius;
         public abstract double GetValue(double x);
      }
      class HermiteFilter : ResamplingFilter
      {

         public HermiteFilter()
         {

            defaultFilterRadius = 1;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 1) return ((2 * x - 3) * x * x + 1);
            return 0;
         }
      }
      class BoxFilter : ResamplingFilter
      {

         public BoxFilter()
         {

            defaultFilterRadius = 0.5;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x <= 0.5) return 1;
            return 0;
         }
      }
      class TriangleFilter : ResamplingFilter
      {

         public TriangleFilter()
         {

            defaultFilterRadius = 1;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 1) return (1 - x);
            return 0;
         }
      }
      class BellFilter : ResamplingFilter
      {

         public BellFilter()
         {

            defaultFilterRadius = 1.5;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 0.5) return (0.75 - x * x);
            if (x < 1.5) return (0.5 * Math.Pow(x - 1.5, 2));
            return 0;
         }
      }
      class CubicBSplineFilter : ResamplingFilter
      {

         double temp;

         public CubicBSplineFilter()
         {

            defaultFilterRadius = 2;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 1)
            {

               temp = x * x;
               return (0.5 * temp * x - temp + 2f / 3f);
            }
            if (x < 2)
            {

               x = 2f - x;
               return (Math.Pow(x, 3) / 6f);
            }
            return 0;
         }
      }
      class Lanczos3Filter : ResamplingFilter
      {

         public Lanczos3Filter()
         {

            defaultFilterRadius = 3;
         }

         double SinC(double x)
         {

            if (x != 0)
            {

               x *= Math.PI;
               return (Math.Sin(x) / x);
            }
            return 1;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 3) return (SinC(x) * SinC(x / 3f));
            return 0;
         }
      }
      class MitchellFilter : ResamplingFilter
      {

         const double C = 1 / 3;
         double temp;

         public MitchellFilter()
         {

            defaultFilterRadius = 2;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            temp = x * x;
            if (x < 1)
            {

               x = (((12 - 9 * C - 6 * C) * (x * temp)) + ((-18 + 12 * C + 6 * C) * temp) + (6 - 2 * C));
               return (x / 6);
            }
            if (x < 2)
            {

               x = (((-C - 6 * C) * (x * temp)) + ((6 * C + 30 * C) * temp) + ((-12 * C - 48 * C) * x) + (8 * C + 24 * C));
               return (x / 6);
            }
            return 0;
         }
      }
      class CosineFilter : ResamplingFilter
      {

         public CosineFilter()
         {

            defaultFilterRadius = 1;
         }

         public override double GetValue(double x)
         {

            if ((x >= -1) && (x <= 1)) return ((Math.Cos(x * Math.PI) + 1) / 2f);
            return 0;
         }
      }
      class CatmullRomFilter : ResamplingFilter
      {

         const double C = 1 / 2;
         double temp;

         public CatmullRomFilter()
         {

            defaultFilterRadius = 2;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            temp = x * x;
            if (x <= 1) return (1.5 * temp * x - 2.5 * temp + 1);
            if (x <= 2) return (-0.5 * temp * x + 2.5 * temp - 4 * x + 2);
            return 0;
         }
      }
      class QuadraticFilter : ResamplingFilter
      {

         public QuadraticFilter()
         {

            defaultFilterRadius = 1.5;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x <= 0.5) return (-2 * x * x + 1);
            if (x <= 1.5) return (x * x - 2.5 * x + 1.5);
            return 0;
         }
      }
      class QuadraticBSplineFilter : ResamplingFilter
      {

         public QuadraticBSplineFilter()
         {

            defaultFilterRadius = 1.5;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x <= 0.5) return (-x * x + 0.75);
            if (x <= 1.5) return (0.5 * x * x - 1.5 * x + 1.125);
            return 0;
         }
      }
      class CubicConvolutionFilter : ResamplingFilter
      {

         double temp;

         public CubicConvolutionFilter()
         {

            defaultFilterRadius = 3;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            temp = x * x;
            if (x <= 1) return ((4f / 3f) * temp * x - (7f / 3f) * temp + 1);
            if (x <= 2) return (-(7f / 12f) * temp * x + 3 * temp - (59f / 12f) * x + 2.5);
            if (x <= 3) return ((1f / 12f) * temp * x - (2f / 3f) * temp + 1.75 * x - 1.5);
            return 0;
         }
      }
      class Lanczos8Filter : ResamplingFilter
      {

         public Lanczos8Filter()
         {

            defaultFilterRadius = 8;
         }

         double SinC(double x)
         {

            if (x != 0)
            {

               x *= Math.PI;
               return (Math.Sin(x) / x);
            }
            return 1;
         }

         public override double GetValue(double x)
         {

            if (x < 0) x = -x;
            if (x < 8) return (SinC(x) * SinC(x / 8f));
            return 0;
         }
      }
      static private ResamplingFilter giveFilter(eResamplerMethod method)
      {
         switch (method)
         {
            case eResamplerMethod.cRS_Hermite: return new HermiteFilter(); break;
            case eResamplerMethod.cRS_Box: return new BoxFilter(); break;
            case eResamplerMethod.cRS_Triangle: return new TriangleFilter(); break;
            case eResamplerMethod.cRS_Bell: return new BellFilter(); break;
            case eResamplerMethod.cRS_CubicBSpline: return new CubicBSplineFilter(); break;
            case eResamplerMethod.cRS_Lanczos3: return new Lanczos3Filter(); break;
            case eResamplerMethod.cRS_Lanczos8: return new Lanczos8Filter(); break;
            case eResamplerMethod.cRS_Mitchell: return new MitchellFilter(); break;
            case eResamplerMethod.cRS_Cosine: return new CosineFilter(); break;
            case eResamplerMethod.cRS_CatmullRom: return new CatmullRomFilter(); break;
            case eResamplerMethod.cRS_Quadratic: return new QuadraticFilter(); break;
            case eResamplerMethod.cRS_QuadraticBSpline: return new QuadraticBSplineFilter(); break;
            case eResamplerMethod.cRS_CubicConvolution: return new CubicConvolutionFilter(); break;
         };
         return new BoxFilter();
      }

      struct Contributor
      {
         public int pixel;
         public double weight;
      }

      struct ContributorEntry
      {
         public int n;
         public Contributor[] p;
         public double wsum;
      }

      static public byte[] ResampleGreyScaleImg(byte[] inputImg, int inputWidth, int inputHeight, int newWidth, int newHeight, eResamplerMethod method)
      {
         ResamplingFilter filter = giveFilter(method);

         if ((inputImg == null) || (inputWidth == 0) || (inputHeight == 0) || (newWidth == 0) || (newHeight == 0))
            return null;

         int current = 0;
         int total = newWidth + inputHeight;

         int i, j, k, left, right;
         double xScale, yScale, center, wdth, weight;
         double alpha;
         int rowIn, rowOut;
         int offset;

         // set operating pixel formating
         int shift = 1;

         // create intermediate image to hold horizontal zoom
         byte[] work = new byte[newWidth * inputHeight];

         // lock bitmaps
         byte[] bdIn = inputImg;
         byte[] bdOut = work;

         xScale = (double)newWidth / (double)inputWidth;
         yScale = (double)newHeight / (double)inputHeight;


         ContributorEntry[] contrib = new ContributorEntry[newWidth];
         // horizontal downsampling
         if (xScale < 1)
         {

            // scales from bigger to smaller width
            wdth = filter.defaultFilterRadius / xScale;
            for (i = 0; i < newWidth; i++)
            {
               contrib[i].n = 0;
               contrib[i].p = new Contributor[(int)Math.Floor(2 * wdth + 1)];
               contrib[i].wsum = 0;
               center = (i + 0.5) / xScale;
               left = (int)(center - wdth);
               right = (int)(center + wdth);
               for (j = left; j <= right; j++)
               {
                  weight = filter.GetValue((center - j - 0.5) * xScale);
                  if ((weight == 0) || (j < 0) || (j >= inputWidth)) continue;
                  contrib[i].p[contrib[i].n].pixel = j;
                  contrib[i].p[contrib[i].n].weight = weight;
                  contrib[i].wsum += weight;
                  contrib[i].n++;
               }
            }
         }
         else
         {

            // horizontal upsampling
            // scales from smaller to bigger width
            for (i = 0; i < newWidth; i++)
            {
               contrib[i].n = 0;
               contrib[i].p = new Contributor[(int)Math.Floor(2 * filter.defaultFilterRadius + 1)];
               contrib[i].wsum = 0;
               center = (i + 0.5) / xScale;
               left = (int)Math.Floor(center - filter.defaultFilterRadius);
               right = (int)Math.Ceiling(center + filter.defaultFilterRadius);
               for (j = left; j <= right; j++)
               {
                  weight = filter.GetValue(center - j - 0.5);
                  if ((weight == 0) || (j < 0) || (j >= inputWidth)) continue;
                  contrib[i].p[contrib[i].n].pixel = j;
                  contrib[i].p[contrib[i].n].weight = weight;
                  contrib[i].wsum += weight;
                  contrib[i].n++;

               }
            }
         }

         // filter horizontally from input to work
         for (k = 0; k < inputHeight; k++)
         {
            rowIn = k * inputWidth;
            rowOut = k * newWidth;
            for (i = 0; i < newWidth; i++)
            {
               alpha = 0;
               for (j = 0; j < contrib[i].n; j++)
               {
                  offset = contrib[i].p[j].pixel * shift;
                  weight = contrib[i].p[j].weight;
                  if (weight == 0) continue;

                  alpha += bdIn[rowIn + offset] * weight;
               }
               alpha /= contrib[i].wsum;
               if (alpha > 255) bdOut[rowOut + 0] = 255; else if (bdOut[rowOut + 0] < 0) bdOut[rowOut + 0] = 0; else bdOut[rowOut + 0] = (byte)alpha;
               rowOut += shift;
            }
         }


         // create final output image
         byte[] output = new byte[newWidth * newHeight];

         // swap for 2nd pass
         bdIn = bdOut;
         bdOut = output;

         // pre-calculate filter contributions for a column
         contrib = new ContributorEntry[newHeight];
         // vertical downsampling
         if (yScale < 1)
         {
            // scales from bigger to smaller height
            wdth = filter.defaultFilterRadius / yScale;
            for (i = 0; i < newHeight; i++)
            {

               contrib[i].n = 0;
               contrib[i].p = new Contributor[(int)Math.Floor(2 * wdth + 1)];
               contrib[i].wsum = 0;
               center = (i + 0.5) / yScale;
               left = (int)(center - wdth);
               right = (int)(center + wdth);
               for (j = left; j <= right; j++)
               {

                  weight = filter.GetValue((center - j - 0.5) * yScale);
                  if ((weight == 0) || (j < 0) || (j >= inputHeight)) continue;
                  contrib[i].p[contrib[i].n].pixel = j;
                  contrib[i].p[contrib[i].n].weight = weight;
                  contrib[i].wsum += weight;
                  contrib[i].n++;
               }


            }
         }
         else
         {

            // vertical upsampling
            // scales from smaller to bigger height
            for (i = 0; i < newHeight; i++)
            {

               contrib[i].n = 0;
               contrib[i].p = new Contributor[(int)Math.Floor(2 * filter.defaultFilterRadius + 1)];
               contrib[i].wsum = 0;
               center = (i + 0.5) / yScale;
               left = (int)(center - filter.defaultFilterRadius);
               right = (int)(center + filter.defaultFilterRadius);
               for (j = left; j <= right; j++)
               {

                  weight = filter.GetValue(center - j - 0.5);
                  if ((weight == 0) || (j < 0) || (j >= inputHeight)) continue;
                  contrib[i].p[contrib[i].n].pixel = j;
                  contrib[i].p[contrib[i].n].weight = weight;
                  contrib[i].wsum += weight;
                  contrib[i].n++;
               }


            }
         }

         // filter vertically from work to output
         for (k = 0; k < newWidth; k++)
         {
            for (i = 0; i < newHeight; i++)
            {

               alpha = 0;
               for (j = 0; j < contrib[i].n; j++)
               {

                  rowIn = contrib[i].p[j].pixel * newWidth + k * shift;
                  weight = contrib[i].p[j].weight;
                  if (weight == 0) continue;
                  alpha += bdIn[rowIn + 0] * weight;
               }
               rowOut = i * newWidth + k * shift;

               alpha /= contrib[i].wsum;
               if (alpha > 255) bdOut[rowOut + 0] = 255; else if (alpha < 0) bdOut[rowOut + 0] = 0; else bdOut[rowOut + 0] = (byte)alpha;
            }
         }

         filter = null;
         bdIn = null;
         bdOut = null;
         work = null;
         contrib = null;
         return output;
      }
      #endregion

   }
}