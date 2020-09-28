//==============================================================================
//
// File: tigerCrypt.h
//
// Copyright (c) 2007, Ensemble Studios
//
// This isn't used anymore - see teaCrypt.cpp/.h instead.
//
//==============================================================================
#pragma once

const uint cTigerCryptBlockSize = 48;
const uint64 cDefaultTigerCryptIV = 0x15EF0AF334248FE2UL;

// iv is the initialization vector. 
// k1, k2, and k3 are the keys.
// pSrcData and pDstData must point to 48-byte buffers. They can be the same buffers.
// counter is only used in Counter Mode encryption.
void tigerCryptInitKeys(const BString& keyPhrase, uint64& k1, uint64& k2, uint64& k3);
void tigerEncryptBlock48(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter = 0);
void tigerDecryptBlock48(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter = 0);

