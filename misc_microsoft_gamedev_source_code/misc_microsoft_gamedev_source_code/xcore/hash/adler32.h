// File: adler32.h
#pragma once

const uint INIT_ADLER32 = 1;

// reference: zlib standard
uint calcAdler32(const void* Pbuf, uint len, uint adler = INIT_ADLER32);
