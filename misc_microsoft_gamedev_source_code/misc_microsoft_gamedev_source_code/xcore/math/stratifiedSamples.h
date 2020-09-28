// File: stratifiedSamples.cpp
#pragma once

// n = -1 : random (sphere)
// n = 0 : proportional to solid angle (hemisphere)
// n = 1 : proportional to cosine-weighted solid angle (hemisphere)
// n > 1 : proportional to cosine lobe around normal, n = power (hemisphere)
void generateStratifiedSamples(BDynamicArray<BVec3>& samples, float n, int sqrtNumSamples);
