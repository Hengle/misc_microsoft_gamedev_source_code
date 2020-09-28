// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Stat_h
#define Stat_h

#if 0
{
    NEST { STAT(Svdeg); ForIndex(i,10) { Svdeg+=vdeg[i]; } EndFor; }
    SSTAT(Svanum,va.num());
}
#endif

// GetenvValue("STATFILES") -> store data values in files.

class WFile;

class Stat {
 public:
    Stat(const char* pname=0, int pprint=0, int isstatic=0);
    ~Stat();
    void setname(const char* pname);
    void setprint(int pprint);
    void setrms();              // show rms instead of sdv
    void zero();
    void terminate();
    void enter(float f);
    void enter_multiple(float f, int fac); // fac could be negative
    Stat& operator+=(int f);
    Stat& operator+=(float f);
    Stat& operator+=(const Stat& st);
    const char* name() const;
    __int64 num() const;
    float min() const;
    float max() const;
    float avg() const;
    float var() const;
    float sdv() const;
    float sum() const;
    float rms() const;
    float maxabs() const;
    const char* string() const; // no leading name, no trailing '\n'
    const char* namestring() const; // operator<< uses namestring format
    friend ostream& operator<<(ostream& s, const Stat& st);
 private:
    const char* _name;
    int _print;                 // print statistics in destructor
    int _setrms;
    __int64 _n;
    double _sum;
    double _sum2;
    float _min;
    float _max;
    WFile* _pfi;                // if getenv("STATFILES")
    //
friend class Statclass;
    void output(float f) const;
    DISABLE_COPY(Stat);
};

#define STAT(Svar) Stat Svar(#Svar,1)
#define STATNP(Svar) Stat Svar(#Svar,0)
#define SSTAT(Svar,v) { static Stat Svar(#Svar,1,1); Svar+=v; }

//----------------------------------------------------------------------------

inline const char* Stat::name() const { return _name; }
inline __int64 Stat::num() const { return _n; }
inline float Stat::min() const { return _min; }
inline float Stat::max() const { return _max; }
inline float Stat::sdv() const { return sqrt(var()); }
inline float Stat::avg() const { return assertw1(_n)?0.f:float(_sum/_n); }
inline float Stat::sum() const { return (float)_sum; }
inline float Stat::maxabs() const { return ::max(abs(min()),abs(max())); }
inline void Stat::setprint(int pprint) { _print=pprint; }
inline void Stat::setrms() { _setrms=1; }

inline Stat& Stat::operator+=(float f)
{
    _n++; _sum+=f; _sum2+=f*f;
    if (f<_min) _min=f;
    if (f>_max) _max=f;
    if (_pfi) output(f);
    return *this;
}

inline void Stat::enter_multiple(float f, int fac)
{
    _n+=fac; _sum+=f*fac; _sum2+=f*f*fac;
    if (f<_min) _min=f;
    if (f>_max) _max=f;
    if (_pfi) ForIndex(i,fac) { output(f); } EndFor;
}

inline Stat& Stat::operator+=(int f)
{
    return *this+=(float)f;
}

inline void Stat::enter(float f)
{
    *this+=f;
}

#endif
