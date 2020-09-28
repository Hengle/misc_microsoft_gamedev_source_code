// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Buffer_h
#define Buffer_h

class Buffer {
 public:
    Buffer(int fd, int maxsize=0); // default is infinite size
    virtual ~Buffer()=0;           // but it does exist!
    int eof() const;            // end of file
    int err() const;            // error in system call
 protected:
    int _fd;                    // file descriptor associated
    int _size;                  // current size of _ar[]
    char* _ar;                  // buffer
    int _beg;                   // index of first element in _ar[]
    int _n;                     // num of elements in buffer (_beg+_n<=_size)
    int _steof;
    int _sterr;
    //
    void shift();               // shift data to beginning of buffer
    void expand();              // increase buffer size
 private:
    int _maxsize;               // maximum size buffer can expand to (0=inf)
    DISABLE_COPY(Buffer);
};

class RBuffer : public Buffer {
 public:
    RBuffer(int fd, int maxsize=0);
    ~RBuffer();
    int fill();                 // ret: 0=no, 1=yes, -1=other
    void extract(int num);      // have read num bytes
    int num() const;
    char operator[](int bi) const;
    int hasline() const;
    // next dies if len not sufficient, includes '\n', ret success
    int extractline(char* s, int len);
    char  bgetchar(int bi) const; // same as operator[]
    int   bgetint(int bi) const;
    short bgetshort(int bi) const;
    float bgetfloat(int bi) const;
    void waitforinput();
};

class WBuffer : public Buffer {
 public:
    WBuffer(int fd, int maxsize=0);
    ~WBuffer();
    int flush(int nb=0);        // (nb==0 is all) ret: 0=part, 1=all, -1=other
    void bput(const void* buf, int nbytes);
    void bput(char c);
    void bput(short i);
    void bput(int i);
    void bput(float f);
};

//----------------------------------------------------------------------------

#include "NetworkOrder.h"

inline int Buffer::eof() const { return _steof; }

inline int Buffer::err() const { return _sterr; }

inline int RBuffer::num() const { return _n; }

inline char RBuffer::operator[](int bi) const
{
    // assertx(bi>=0 && bi<_n);
    return _ar[_beg+bi];
}

inline char RBuffer::bgetchar(int bi) const
{
    return (*this)[bi];
}

inline int RBuffer::bgetint(int bi) const
{
    // assertx(bi>=0 && bi+4<=_n);
    char* p=&_ar[_beg+bi];
    if ((pointer_to_unsigned(p)&0x3)==0) {
        int r=*(int*)p; StdToInt(&r); return r;
    } else {
        char s[4]; s[0]=p[0]; s[1]=p[1]; s[2]=p[2]; s[3]=p[3];
        int& r=*(int*)s; StdToInt(&r); return r;
    }
}

inline short RBuffer::bgetshort(int bi) const
{
    // assertx(bi>=0 && bi+2<=_n);
    char* p=&_ar[_beg+bi];
    if ((pointer_to_unsigned(p)&0x1)==0) {
        short r=*(short*)p; StdToShort(&r); return r;
    } else {
        char s[2]; s[0]=p[0]; s[1]=p[1];
        short& r=*(short*)s; StdToShort(&r); return r;
    }
}

inline float RBuffer::bgetfloat(int bi) const
{
    // assertx(bi>=0 && bi+4<=_n);
    char* p=&_ar[_beg+bi];
    if ((pointer_to_unsigned(p)&0x3)==0) {
        float r=*(float*)p; StdToFloat(&r); return r;
    } else {
        char s[4]; s[0]=p[0]; s[1]=p[1]; s[2]=p[2]; s[3]=p[3];
        float& r=*(float*)s; StdToFloat(&r); return r;
    }
}

#endif
