#if 0  // ------------------------------------------------------------------------------------------
{
   BImageUtils::writeTGA24("before.tga", image, 1);

   BRGBAImage htransImage(image.getWidth()/2, image.getHeight()/2);

   typedef BStaticArray2D<BVec4, 2, 2> BBlockVec;
   BArray2D<BBlockVec> blockArray(cellsX, cellsY);

   uint hHist[3][256];
   Utils::ClearObj(hHist);

#if 0      
   BRGBAImage temp(8192, 8192);
#endif      

   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         BBlockVec h;

         float hSum[3];
         Utils::ClearObj(hSum);

#if 0                        
         for (uint q = 0; q < 64; q++)
         {
            temp.setPixel(cx*64+q,cy*64,BRGBAColor(64));
            temp.setPixel(cx*64,cy*64+q,BRGBAColor(64));
         }
#endif            

         for (uint sy = 0; sy < 2; sy++)
         {
            for (uint sx = 0; sx < 2; sx++)
            {
               uint x = cx*4+sx*2;
               uint y = cy*4+sy*2;

               int a00 = image(x+0,y+0).g;   
               int a10 = image(x+1,y+0).g;   
               int a01 = image(x+0,y+1).g;   
               int a11 = image(x+1,y+1).g;

               int h0 = (a11 + a10 + a01 + a00) / 4;
               int h1 = (a11 + a10 - a01 - a00); // / 4;
               int h2 = (a11 - a10 + a01 - a00); // / 4;
               int h3 = (a11 - a10 - a01 + a00); // / 4;

               float fh1 = h1 / 4.0f;
               float fh2 = h2 / 4.0f;
               float fh3 = h3 / 4.0f;

               //fh1 = -127.5f;
               //fh2 = -127.5f;
               //fh3 = -127.5f;

               fh1 /= 127.5f;
               fh2 /= 127.5f;
               fh3 /= 127.5f;

               //float mag = sqrt(fh1*fh1+fh2*fh2);
               //float angle = Math::Clamp((atan2(fh1, fh2) + Math::fPi) / Math::fTwoPi, 0.0f, 1.0f);

               //temp(cx*2+sx, cy*2+sy).set(mag * 255.0f, angle * 255.0f, 0, 0);
#if 0                  
               {
                  int xx = cx*64+Math::Clamp<int>(32.5f+fh1*32.0f, 0, 63);
                  int yy = cy*64+Math::Clamp<int>(32.5f+fh2*32.0f, 0, 63);
                  temp.setPixel(xx,yy,BRGBAColor(255));
               }
#endif                  

               fh1 = Math::Sign(fh1)*sqrt(fabs(fh1));
               fh2 = Math::Sign(fh2)*sqrt(fabs(fh2));
               //fh3 = Math::Sign(fh3)*sqrt(fabs(fh3));

               fh1 *= 127.0f;
               fh2 *= 127.0f;
               fh3 *= 127.0f;

               // -127 to 127
               h(sx, sy)[0] = h0;
               h(sx, sy)[1] = fh1;
               h(sx, sy)[2] = fh2;
               h(sx, sy)[3] = fh3;

               hSum[0] += fh1*fh1;
               hSum[1] += fh2*fh2;
               hSum[2] += fh3*fh3;

               h1 = (int)((fh1 >= 0.0f) ? floor(fh1) : ceil(fh1));
               h2 = (int)((fh2 >= 0.0f) ? floor(fh2) : ceil(fh2));
               h3 = (int)((fh3 >= 0.0f) ? floor(fh3) : ceil(fh3));

               h1 = Math::Clamp(h1+128, 1, 255);
               h2 = Math::Clamp(h2+128, 1, 255);
               h3 = Math::Clamp(h3+128, 1, 255);

               hHist[0][h1]++;
               hHist[1][h2]++;
               hHist[2][h3]++;
            }
         }

#if 0            
         int minC;
         if ((hsum[0] < hsum[1]) && (hsum[0] < hsum[2]))                                            
         {
            minC = 1;
         }
         else if ((hsum[1] < hsum[0]) && (hsum[1] < hsum[2]))                                            
         {
            minC = 2;
         }
         else
         {
            minC = 3;
         }

         for (uint sy = 0; sy < 2; sy++)
            for (uint sx = 0; sx < 2; sx++)
               h[sx][sy][minC] = 0;
#endif                  
         blockArray(cx, cy) = h;
      }
   }

#if 0      
   BImageUtils::writeTGA24("anglemag.tga", temp);
#endif      

   BVec3 range(127.0f);

#if 1      
   for (uint channel = 0; channel < 3; channel++)
   {
      uint total = 0;
      for (uint i = 1; i < 256; i++)
         total += hHist[channel][i];

      const uint thresh = 5;//Math::Max<uint>(1, (total * 5) / 1000);

      uint l;
      uint c = 0;
      for (l = 1; l <= 127; l++)
      {
         c += hHist[channel][l];
         if (c >= thresh)
            break;
      }

      uint h;
      c = 0;
      for (h = 255; h > l; h--)
      {
         c += hHist[channel][h];
         if (c >= thresh)
            break;
      }

      if ((l < h) && (l <= 127))
      {
         range[channel] = Math::Max(labs(l - 128), labs(h - 128));
      }
   }
#endif      

   range[0] = Math::Clamp(range[0], 1.0f, 127.0f);
   range[1] = Math::Clamp(range[1], 1.0f, 127.0f);
   range[2] = Math::Clamp(range[2], 1.0f, 127.0f);
   range[0] = range[1] = range[2] = Math::Max3(range[0], range[1], range[2]);
   //range[2] = Math::Clamp(range[2]*2.0f, 1.0f, 127.0f);

   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         const BBlockVec& block = blockArray(cx, cy);

         for (uint sy = 0; sy < 2; sy++)
         {
            for (uint sx = 0; sx < 2; sx++)
            {  
               float fh0 = block(sx, sy)[0];
               float fh1 = block(sx, sy)[1];
               float fh2 = block(sx, sy)[2];
               float fh3 = block(sx, sy)[3];

               fh1 = fh1 / range[0];
               fh2 = fh2 / range[1];
               fh3 = fh3 / range[2];

               fh1 = Math::Clamp(fh1, -1.0f, 1.0f) * 127.0f;
               fh2 = Math::Clamp(fh2, -1.0f, 1.0f) * 127.0f;
               fh3 = Math::Clamp(fh3, -1.0f, 1.0f) * 127.0f;

               int h0 = (int)fh0;
               int h1 = (int)(Math::Sign(fh1)*floor(.5f+fabs(fh1)));
               int h2 = (int)(Math::Sign(fh2)*floor(.5f+fabs(fh2)));
               int h3 = (int)(Math::Sign(fh3)*floor(.5f+fabs(fh3)));

               h1 += 125;
               h2 += 123;
               h3 += 123;

               h1 = Math::Clamp(h1, 0, 255);
               h2 = Math::Clamp(h2, 0, 255);
               h3 = Math::Clamp(h3, 0, 255);

               htransImage(cx*2+sx,cy*2+sy).a = h0;
               htransImage(cx*2+sx,cy*2+sy).r = h2;
               htransImage(cx*2+sx,cy*2+sy).g = h1;
               htransImage(cx*2+sx,cy*2+sy).b = h3;
            }
         }                        
      } // cx
   } // cy            

   //a11 = (h0 + h1 + h2 + h3);
   //a10 = (h0 + h1 - h2 - h3);
   //a01 = (h0 - h1 + h2 - h3);
   //a00 = (h0 - h1 - h2 + h3);

   //image(x+0,y+0).set(a00);   //ave
   //image(x+1,y+0).set(a10);   //bottom-top
   //image(x+0,y+1).set(a01);   //left-right
   //image(x+1,y+1).set(a11);   //diagnol

   BDXTPacker packer;
   BAlignedArray<uchar> htransDXT5;
   packer.pack(htransImage, cDXT5, false, false, false, htransDXT5);

   BRGBAImage unpackedHTrans;
   BDXTUnpacker unpacker;
   unpacker.unpack(unpackedHTrans, &htransDXT5[0], cDXT5, htransImage.getWidth(), htransImage.getHeight());

   BImageUtils::writeTGA24("hunpacked.tga", unpackedHTrans);
   BImageUtils::writeTGA24("hunpackedA.tga", unpackedHTrans, 3);

   BRGBAImage newImage(image.getWidth(), image.getHeight());

   for (uint y = 0; y < newImage.getHeight(); y+=2)
   {
      for (uint x = 0; x < newImage.getWidth(); x+=2)
      {
         int h0 = unpackedHTrans(x/2,y/2).a;
         int h2 = unpackedHTrans(x/2,y/2).r-123;
         int h1 = unpackedHTrans(x/2,y/2).g-125;
         int h3 = unpackedHTrans(x/2,y/2).b-123;

         h1 = Math::Clamp(h1, -127, 127);
         h2 = Math::Clamp(h2, -127, 127);
         h3 = Math::Clamp(h3, -127, 127);

         float fh1 = (h1 / 127.0f); fh1 = fh1*fh1*Math::Sign(h1) * range[0];
         float fh2 = (h2 / 127.0f); fh2 = fh2*fh2*Math::Sign(h2) * range[1];
         //float fh3 = (h3 / 127.0f); fh3 = fh3*fh3*Math::Sign(h3) * range[2];

         //float fh1 = (h1 / 127.0f); fh1 = fh1 * range[0];
         //float fh2 = (h2 / 127.0f); fh2 = fh2 * range[1];
         float fh3 = (h3 / 127.0f); fh3 = fh3 * range[2];

         int a11 = Math::Clamp((int)(.5f + h0 + fh1 + fh2 + fh3), 0, 255);
         int a10 = Math::Clamp((int)(.5f + h0 + fh1 - fh2 - fh3), 0, 255);
         int a01 = Math::Clamp((int)(.5f + h0 - fh1 + fh2 - fh3), 0, 255);
         int a00 = Math::Clamp((int)(.5f + h0 - fh1 - fh2 + fh3), 0, 255);

         //int a11 = Math::Clamp((h0 * 4 + h1 + h2 + h3)/4,0,255);
         //int a10 = Math::Clamp((h0 * 4 + h1 - h2 - h3)/4,0,255);
         //int a01 = Math::Clamp((h0 * 4 - h1 + h2 - h3)/4,0,255);
         //int a00 = Math::Clamp((h0 * 4 - h1 - h2 + h3)/4,0,255);

         newImage(x+0,y+0).set(a00);   
         newImage(x+1,y+0).set(a10);   
         newImage(x+0,y+1).set(a01);   
         newImage(x+1,y+1).set(a11);

         image(x+0,y+0).g = a00;   
         image(x+1,y+0).g = a10;   
         image(x+0,y+1).g = a01;   
         image(x+1,y+1).g = a11;
      }
   }

   BImageUtils::writeTGA24("after.tga", newImage);
}  
#endif  // ------------------------------------------------------------------------------------------