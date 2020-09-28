// File: adler32.h
#pragma once

namespace ens
{
   const uint INIT_ADLER32 = 1;

   uint calcAdler32(const void* Pbuf, uint len, uint adler = INIT_ADLER32);
} // namespace ens   
