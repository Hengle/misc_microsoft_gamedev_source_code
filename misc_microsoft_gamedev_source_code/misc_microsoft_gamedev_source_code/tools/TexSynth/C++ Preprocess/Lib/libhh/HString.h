// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HString_h
#define HString_h

class HString {
 public:
    HString();
    HString(const char* s);
    HString(const HString& s);
    ~HString();
    HString& operator=(const HString& s);
    operator const char*() const;
    const char& operator[](int i) const;
    char& operator[](int i);    // warning: (&hstring[0])[2]=0 insecure
    friend HString operator+(const HString& s1, const HString& s2);
    HString& operator+=(const HString& s);
 private:
    char* _s;
};

//----------------------------------------------------------------------

inline HString::HString() : _s(0) { }

inline HString::~HString() { delete[] _s; }

inline HString::operator const char*() const { return _s; }

inline const char& HString::operator[](int i) const { return _s[i]; }

inline char& HString::operator[](int i) { return _s[i]; }

#endif
