//==============================================================================
//
// File: teaCrypt.h
//
// Copyright (c) 2007, Ensemble Studios
//
// See: http://www.simonshepherd.supanet.com/source.htm#ansi
//==============================================================================
#pragma once

// v and w point to 64 bits
// k points to 128 bits
// "n is the number of iterations. 32 is ample, 16 is sufficient, as few as eight should be OK for most applications, 
// especially ones where the data age quickly (real-time video, for example). The algorithm achieves good dispersion 
// after six iterations. The iteration count can be made variable if required."
void teaEncipher(const uint* v, uint* w, const uint* k, uint n = 32);
void teaDecipher(const uint* v, uint* w, const uint* k, uint n = 32);

// Hardcoded to 8 iterations.
uint64 teaEncipher(uint64 v0, uint64 k0, uint64 k1);
uint64 teaDecipher(uint64 v0, uint64 k0, uint64 k1);

// Encrypts 4 64-bit values in parallel using the same keys using 16 iterations.
void teaEncipher(uint64 v0, uint64 v1, uint64 v2, uint64 v3,
                 uint64& w0, uint64& w1, uint64& w2, uint64& w3, 
                 uint64 k0, uint64 k1);

// Decrypts 4 64-bit values in parallel using the same keys using 16 iterations.                 
void teaDecipher(uint64 v0, uint64 v1, uint64 v2, uint64 v3,
                 uint64& w0, uint64& w1, uint64& w2, uint64& k3,
                 uint64 k0, uint64 k1);
                 
const uint cTeaCryptBlockSize = 64;
const uint cTeaCryptBlockSizeLog2 = 6;
const uint64 cDefaultTeaCryptIV = 0x15EF0AF334248FE2UL;

// iv is the initialization vector. 
// k1, k2, and k3 are the keys.
// pSrcData and pDstData must point to 64-byte buffers. They can be the same buffers.
// counter is the index of the block being encrypted.
void teaCryptInitKeys(const char* pKeyPhrase, uint64& k1, uint64& k2, uint64& k3);
void teaEncryptBlock64(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter = 0);
void teaDecryptBlock64(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter = 0);