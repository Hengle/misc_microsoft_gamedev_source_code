// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Gim_h
#define Gim_h

class Gim {
 public:
    Gim();
    ~Gim();
// Initialization
    void clear();
    // init: channels=="" ok; pysize>0, pxsize>0, pzsize>=0; must be clear
    //  e.g. channels=="G3B1N3C3D25"
    void init(const char* channels, int pysize, int pxsize, int pzsize);
    void copy(const Gim& pgim); // must be clear
// Data access
    int ysize() const { return _ysize; }
    int xsize() const { return _xsize; }
    int zsize() const { return _zsize; } // == sum of channel widths
    inline float operator()(int y, int x, int z) const;
    inline float& operator()(int y, int x, int z);
// Channel management
    bool channel_info(char ch, int& ich, int& width) const; // ret: exists
    int retrieve_channel(char ch, int chw) const; // ret: ich or <0
    int add_channel(char ch, int width); // ret: ich; 0-filled, die if exists
    void remove_channel(char ch);        // no-op if !exists
    void rename_channel(char ch1, char ch2); // die if !exists
    const char* channels() const { return _channels; }
// Input-output
    void read(istream& is);
    void read_from_fmp(istream& is, char ch,
                       int pysize, int pxsize, int pzsize); // append a channel
    void write(ostream& os) const;
    void write_to_fmp(ostream& os, char ch) const;
    void write_to_bmp(const char* filename, char ch,
                      float scale=BIGFLOAT, float offset=BIGFLOAT) const;
 private:
    int _ysize;
    int _xsize;
    int _zsize;
    float* _im;
    inline float& elem(int y, int x, int z) const;
    void channels_ok() const;   // internal sanity check
    const char* _channels;      // e.g. "G3B1N3C3D25"
    DISABLE_COPY(Gim);
};

#endif
