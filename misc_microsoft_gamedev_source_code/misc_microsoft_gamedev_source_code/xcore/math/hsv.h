// File: hsv.h
// RGB to HSV, HSV to RGB transformation.
#pragma once

inline BVec3 HSVToRGB(const BVec3& hsv)
{
   BVec3 c1(hsv), c2, sat;

   while (c1[0] < 0.0f)
      c1[0] += 360.0f;
      
   while (c1[0] > 360.0f)
      c1[0] -= 360.0f;

   if (c1[0] < 120.0f) 
   {
      sat[0] = (120.0f - c1[0]) / 60.0f;
      sat[1] = c1[0] / 60.0f;
      sat[2] = 0.0f;
   } 
   else if (c1[0] < 240.0f) 
   {
      sat[0] = 0.0f;
      sat[1] = (240.0f - c1[0]) / 60.0f;
      sat[2] = (c1[0] - 120.0f) / 60.0f;
   }
   else 
   {
      sat[0] = (c1[0] - 240.0f) / 60.0f;
      sat[1] = 0.0f;
      sat[2] = (360.0f - c1[0]) / 60.0f;
   }
   
   sat[0] = Math::Min(sat[0], 1.0f);
   sat[1] = Math::Min(sat[1], 1.0f);
   sat[2] = Math::Min(sat[2], 1.0f);

   c2[0] = (1.0f - c1[1] + c1[1] * sat[0]) * c1[2];
   c2[1] = (1.0f - c1[1] + c1[1] * sat[1]) * c1[2];
   c2[2] = (1.0f - c1[1] + c1[1] * sat[2]) * c1[2];

   return c2;
}

// Returns:
// x = Hue - degrees
// y = Saturation - 0 to 1
// z = Lightness - 0 to 1
inline BVec3 RGBToHSV(const BVec3& rgb)
{
   const float minComp = Math::Min3(rgb[0], rgb[1], rgb[2]);
   const float maxComp = Math::Max3(rgb[0], rgb[1], rgb[2]);
   
   const float delta = maxComp - minComp;
   
   BVec3 c2;
   c2[2] = maxComp;
   c2[1] = 0.0f;
   if (maxComp > 0.0f)
      c2[1] = delta / maxComp;
      
   c2[0] = 0.0f;
   
   if (delta > 0.0f) 
   {
      if (maxComp == rgb[0] && maxComp != rgb[1])
         c2[0] += (rgb[1] - rgb[2]) / delta;
         
      if (maxComp == rgb[1] && maxComp != rgb[2])
         c2[0] += (2 + (rgb[2] - rgb[0]) / delta);
         
      if (maxComp == rgb[2] && maxComp != rgb[0])
         c2[0] += (4 + (rgb[0] - rgb[1]) / delta);
         
      c2[0] *= 60.0f;
   }
   
   return c2;
}
