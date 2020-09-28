// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef FileIO_h
#define FileIO_h

class WFile {
 public:
    // supports "-", ".Z", "|..."
    WFile(const char* filename, int assert_success=1);
    ~WFile();
    int success() const;        // use after !assert_success
    ostream& operator()() const;
    FILE* cfile();
 private:
    bool _file_ispipe;
    FILE* _file;
    ostream* _os;
    DISABLE_COPY(WFile);
};

class RFile {
 public:
    // supports "-", ".Z", "...|"
    RFile(const char *filename, int assert_success=1);
    ~RFile();
    int success() const;        // use after !assert_success
    istream& operator()() const;
    FILE* cfile();
 private:
    bool _file_ispipe;
    FILE* _file;
    istream* _is;
    DISABLE_COPY(RFile);
};

extern bool FileExists(const char* s);

#endif
