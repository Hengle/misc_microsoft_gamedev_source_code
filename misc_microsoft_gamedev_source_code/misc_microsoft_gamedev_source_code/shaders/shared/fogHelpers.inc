//-----------------------------------------------------------------------------
// File: fogHelpers.inc
//-----------------------------------------------------------------------------
static float computeSquaredExponentialFogDensity(float fogDistance2)
{
   float  finalFogDensity = saturate(exp2(-fogDistance2));
   return finalFogDensity;   
}
//-----------------------------------------------------------------------------
static float computeRadialFogDensity(float3 worldPos)
{
   float3 worldCameraVec = gWorldCameraPos - worldPos;
   float dist2 = dot(worldCameraVec, worldCameraVec);
   return computeSquaredExponentialFogDensity(gFogDensity2 * max(0.0, dist2 - gFogStart2));
}
//-----------------------------------------------------------------------------
static float computePlanarFogDensityInternal(float3 worldPos)
{
   float3 rayDir = gWorldCameraPos - worldPos;
   float maxDist = length(rayDir);
   rayDir *= 1.0 / maxDist;
   
   float dirAlong = -rayDir.y;
      
   float dist = worldPos.y - gPlanarFogStart;
   dist = dist * recipClamp(dirAlong);
            
   if (rayDir.y < 0.0)
      dist = maxDist - dist;      
   
   // if dist < 0.0, this means the ray doesn't traverse any fog
                             
   dist = clamp(dist, 0.0, maxDist);
         
   return computeSquaredExponentialFogDensity(gPlanarFogDensity2 * dist * dist);
}
//-----------------------------------------------------------------------------
static float computePlanarFogDensity(float3 worldPos)
{
   float density = 1.0f;
   [branch]
   if (gPlanarFogEnabled)
      density = computePlanarFogDensityInternal(worldPos);
   return density;      
}
//-----------------------------------------------------------------------------
static float3 computeFog(float3 result, float zFogDensity, float planarFogDensity)
{
   result = lerp(gPlanarFogColor, result, planarFogDensity);
   result = lerp(gFogColor, result, zFogDensity);
   return result;
} 
