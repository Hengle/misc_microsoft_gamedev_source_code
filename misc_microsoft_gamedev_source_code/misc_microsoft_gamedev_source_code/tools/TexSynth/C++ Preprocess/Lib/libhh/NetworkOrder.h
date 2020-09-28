// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef NetworkOrder_h
#define NetworkOrder_h

// Notes:
// - Network order is Big Endian (MSB first)
//
// - Intel_x86 and VAX are Little Endian
// - RISC is mostly Big Endian

inline unsigned int swap4bytes(unsigned int v) {
    return (((v&0xFF000000)>>24)|
            ((v&0x00FF0000)>> 8)|
            ((v&0x0000FF00)<< 8)|
            ((v&0x000000FF)<<24));
//     unsigned int t=v;
//     char* p=(char*)&t; char tc;
//     tc=p[0]; p[0]=p[3]; p[3]=tc;
//     tc=p[1]; p[1]=p[2]; p[2]=tc;
//     return t;
}

inline unsigned short swap2bytes(unsigned short v) {
    return (((v&0xFF00)>>8)|
            ((v&0x00FF)<<8));
//     unsigned short t=v;
//     char* p=(char*)&t; char tc;
//     tc=p[0]; p[0]=p[1]; p[1]=tc;
//     return t;
}

#if defined(__WIN32)

#if defined(_M_IX86)
// For Win32, ignore definitions in other header files.
#define htonl private_htonl
#define ntohl private_ntohl
#define htons private_htons
#define ntohs private_ntohs
inline unsigned int htonl(unsigned int v) { return swap4bytes(v); }
inline unsigned int ntohl(unsigned int v) { return swap4bytes(v); }
inline unsigned short htons(unsigned short v) { return swap2bytes(v); }
inline unsigned short ntohs(unsigned short v) { return swap2bytes(v); }
#else
#error HH: unexpected environment.
#endif

#else

// Note: normally, these are defined to act on unsigned long (32bit),
//  but I assume that 'int' is 32 bit, so that works on alphas too
//  (where long is 64 bit)
extern "C" {                    // <netinet/in.h>
    unsigned int htonl(unsigned int),ntohl(unsigned int);
    unsigned short htons(unsigned short),ntohs(unsigned short);
}

#endif

// *** conversions to/from standard byte order

inline void IntToStd(unsigned int* p) { *p=htonl(*p); }
inline void StdToInt(unsigned int* p) { *p=ntohl(*p); }
inline void ShortToStd(unsigned short* p) { *p=htons(*p); }
inline void StdToShort(unsigned short* p) { *p=ntohs(*p); }

inline void FloatToStd(float* p) { IntToStd((unsigned int*)p); }
inline void StdToFloat(float* p) { StdToInt((unsigned int*)p); }
inline void IntToStd(int* p) { IntToStd((unsigned int*)p); }
inline void StdToInt(int* p) { StdToInt((unsigned int*)p); }
inline void ShortToStd(short* p) { ShortToStd((unsigned short*)p); }
inline void StdToShort(short* p) { StdToShort((unsigned short*)p); }

// *** conversions to/from DOS (non-Std) byte order

inline void IntToDos(unsigned int* p) { IntToStd(p); *p=swap4bytes(*p); }
inline void DosToInt(unsigned int* p) { *p=swap4bytes(*p); StdToInt(p); }
inline void ShortToDos(unsigned short* p) { ShortToStd(p); *p=swap2bytes(*p); }
inline void DosToShort(unsigned short* p) { *p=swap2bytes(*p); StdToShort(p); }

inline void IntToDos(int* p) { IntToDos((unsigned int*)p); }
inline void DosToInt(int* p) { DosToInt((unsigned int*)p); }
inline void ShortToDos(short* p) { ShortToDos((unsigned short*)p); }
inline void DosToShort(short* p) { DosToShort((unsigned short*)p); }

#if defined(__WIN32) && defined(_M_IX86)
#undef htonl
#undef ntohl
#undef htons
#undef ntohs
#endif

#if !defined(HH_NO_AVOIDS)
#define htonl HH_AVOID_USING_HTONL
#define ntohl HH_AVOID_USING_NTOHL
#define htons HH_AVOID_USING_HTONS
#define ntohs HH_AVOID_USING_NTOHS
#endif

#endif
