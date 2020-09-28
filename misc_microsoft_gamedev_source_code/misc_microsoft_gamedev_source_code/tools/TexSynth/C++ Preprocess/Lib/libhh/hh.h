// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.
//3456789012345678901234567890123456789012345678901234567890123456789012345678#

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Hh_h
#define Hh_h


// *** Identify system

#if defined(__sgi)
#define __SGICC
#endif

#if defined(WIN32)
#define __WIN32
#endif

// *** System stuff

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include <string.h>

#if defined(__WIN32) && defined(_MSC_VER)
#if _MSC_VER < 1310 // VC7.1 .NET 2003
#define OLD_IOSTREAM
#endif
#endif

#if defined(OLD_IOSTREAM)

#if defined(__WIN32)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4995)   // deprecated(_OLD_IOSTREAMS_ARE_DEPRECATED)
#endif
#include <iostream.h>
#if defined(__WIN32)
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
#endif
#include <iomanip.h>
// Maybe something here is getting VC intellisense confused?
#if defined(WIN32)
#include <strstrea.h>
#else
#include <strstream.h>
#endif

#else

#include <iostream>
#include <iomanip>
#include <strstream>
using namespace std;
#define NAMESPACE_STD

#endif


#if defined(__WIN32)
// disable some nitpicky level4 warnings (for -W4)
#pragma warning(disable:4127)   // conditional expression is constant "if (0)"
#pragma warning(disable:4511)   // copy constructor could not be generated
#pragma warning(disable:4512)   // assignment operator could not be generated
#pragma warning(disable:4702)   // unreachable code (clever "for(;;) break")
#pragma warning(disable:4291)   // no matching operator delete (SACABLE macro)
#endif

#if defined(__WIN32)
// If were to use windef.h, disable stupid macros min and max.
#define NOMINMAX
// If already defined, undo it.
#undef min
#undef max
#include <io.h>                 // _read(),...
#include <memory.h>             // memcpy()
#else
#include <unistd.h>
#endif

#if defined(__SGICC) && !defined(_BOOL)
// if using "OCC" or "NCC -LANG:bool=OFF", then _BOOL is undefined
typedef unsigned char bool;
static const bool false=0;
static const bool true=1;
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;

#if defined(__DECCXX) && defined(__alpha)
typedef void (*HHSIG_PF)(int);
#elif defined(__GNUG__)
typedef void (*HHSIG_PF)(...);
// #define HHSIG_PF SignalHandler
#elif defined(__DECCXX)
#define HHSIG_PF sigvec_t
#elif defined(ATTCC)
#define HHSIG_PF SIG_PF
#elif defined(__SGICC)
// was this:
//  typedef void (*HHSIG_PF)(...);
// but no longer works on IRIS 6.2, so go with this one (even though it gives
//  off warnings under IRIS 5.3).
typedef void (*HHSIG_PF)(int);
#elif defined(__WIN32)
typedef void (__cdecl *HHSIG_PF)(int sig);
#else
#error HH: unexpected environment.
#endif

#if defined(__WIN32)
// sleep() available as Sleep()
extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long millsecs);
inline void sleep(int sec) { Sleep(sec*1000); }
// I have not thought this through.  Look at WSAEWOULDBLOCK...
#define EWOULDBLOCK EAGAIN
// neither hypotf nor _hypotf available it seems.
inline float _hypotf(float v1, float v2) { return (float)_hypot(v1,v2); }
#endif

// *** Syntactic sugar

// Useful for "NEST { int localvar; ... }" so that indentation is correct
#define NEST
#define bcase break; case
#define ocase case
#define bdefault break; default
#define ForIndex(i,ub) { int zzub=ub; for (int i=0;i<zzub;i++) {
#define DummyEndFor }}
#define ForIndexLU(i,lb,ub) { int zzub=ub; for (int i=lb;i<=zzub;i++) {
#define DummyEndFor }}
#define For {{ for
#define EndFor }}
#define DISABLE_COPY(class) class& operator=(class&); class(class&)
#define DISABLE_ASSIGN_INT(class) void operator=(int)
#define dummy_use(v) ((void)v)
#define dummy_init(v) v=0
#define NumElements(array) (sizeof(array)/sizeof(array[0]))


// *** Assertions, warnings, errors, debug

extern
#if defined(__WIN32)
__declspec(noreturn)
#endif
void hhassertx(const char* s, const char* file, int line);
extern int hhassert(int warncode, const char* s, const char* file, int line);

template<class T> inline const T&
assertvaux(const T& expr, const char* s, const char* file, int line)
{ if (!expr) hhassertx(s,file,line); return expr; }

// Warning: ret: message_printed
#define Warning(s) hhassert(1, s, __FILE__, __LINE__)
// assertnever: ret: void
#define assertnever(str) \
(void)hhassertx("assertnever(" str ")", __FILE__, __LINE__)
// assertx: ret: void
#define assertx(expr) \
((void)((expr)?0:hhassertx("assertx(" #expr ")", __FILE__, __LINE__)))
// assertw: ret: expr_failed
#define assertw(expr) \
((expr)?0:(hhassert(+0, "assertw(" #expr ")", __FILE__, __LINE__),1))
// assertw1: ret: expr_failed
#define assertw1(expr) \
((expr)?0:(hhassert(+1, "assertw1(" #expr ")", __FILE__, __LINE__),1))
// assertv: ret: val
#define assertv(e) \
assertvaux(e, (const char*)"assertv(" #e ")", (const char*)__FILE__, __LINE__)

#if !defined(NDEBUG)
#define ASSERTX(expr) assertx(expr)
#else
#if defined(__WIN32)
#define ASSERTX(expr) ( __assume(expr) )
#else
#define ASSERTX(expr) do {} while (0)
#endif
#endif

#if defined(DEBUG) || defined(__WIN32) && !defined(NDEBUG)
#define HHDEBUG
#endif

#if !defined(NDEBUG)
const bool g_DEBUG=true;
#else
const bool g_DEBUG=false;
#endif


// *** Conversions

class SUniv; typedef SUniv* Univ; // bogus class

#if defined(__WIN32)
#define ok_conversion(x) ((_w64 unsigned long)x)
#else
#define ok_conversion(x) (x)
#endif

inline int safe_trunc_int(size_t s) {
    int i=(int)ok_conversion(s);
    assertx(unsigned(i)==s);
    return i;
}

inline unsigned pointer_to_unsigned(const void* e) { return (unsigned)ok_conversion(e); }
inline const void* unsigned_to_pointer(unsigned e) { return (const void*)ok_conversion(e); }

inline int pointer_to_int(const void* e) { return (int)ok_conversion(e); }
inline const void* int_to_pointer(int e) { return (const void*)ok_conversion(e); }

inline float pointer_to_float(const void* e)
{ unsigned u=pointer_to_unsigned(e); return *(float*)&u; }
inline const void* float_to_pointer(const float& f)
{ return unsigned_to_pointer(*((unsigned*)(&f))); }

template<class T>
class Conv {
 public:
    inline static Univ e(T e) { return (Univ)(const SUniv*)e; }
    inline static T d(Univ e) { return (T)(void*)e; }
};

template<>
class Conv<int> {
 public:
    inline static Univ e(int e) { return (Univ)int_to_pointer(e); }
    inline static int d(Univ e) { return pointer_to_int(e); }
};

template<>
class Conv<unsigned> {
 public:
    inline static Univ e(int e) { return (Univ)unsigned_to_pointer(e); }
    inline static int d(Univ e) { return pointer_to_unsigned(e); }
};

template<>
class Conv<float> {
 public:
    inline static Univ e(float f) { return (Univ)float_to_pointer(f); }
    inline static float d(Univ e) { return pointer_to_float(e); }
};


// *** Debug output

// SHOWN(x*y);
// SHOW(point+vector);
// SHOWS("Could not open display");
// SHOWF("%s: Argument '%s' ambiguous, '%s' assumed\n",argv[0],arg,assumed);
// SHOWL;
// os << hform(" Endprogram: %dgons %dlines\n",ngons,nlines);

// Formatted show, uses small buffer
extern void SHOWF(const char* format, ...);
// Output a string prefixed with "# " on cerr, and on cout if _is_a_file
extern void SHOWDF(const char* format, ...);
// Output a string prefixed with "# " on cout only if _is_a_file
extern void SHOWFF(const char* format, ...);
// Default is "# ".
extern const char* g_CommentPrefixString;

// Show an expression
#define SHOW(x) (cerr << #x << " = " << (x))
// Show an expression, and terminate with a newline
#define SHOWN(x) (cerr << #x << " = " << (x) << "\n")
// Show a string
#define SHOWS(s) SHOWF("%s\n",s)
// Show current line number and file
#define SHOWL SHOWF("Now in %s at line %d\n",__FILE__,__LINE__)

#if defined(NDEBUG) || defined(__WIN32)
#define OPTIMIZE_THIS_FILE
#define OPTIMIZE_HERE
#else
#define OPTIMIZE_THIS_FILE \
static int f_optimize_this_file() \
{ Warning("File not optimized"); return 1; } \
static int dummy_optimize_this_file=f_optimize_this_file()
// #define OPTIMIZE_THIS_FILE \
// static struct c_optimize_this_file \
// { c_optimize_this_file() { Warning("File not optimized"); } } \
// v_optimize_this_file;
#define OPTIMIZE_HERE Warning("File wasn't optimized here")
#endif

// *** Constants

#undef PI
const float PI=3.14159265358979323846f; // ==float(M_PI)
const float BIGFLOAT=1e30f;
const int BIGINT=2147483647;    // 2**31-1

// *** Hh.cxx

extern void FlushTimers();
extern void FlushStats();
extern void FlushWarnings();
inline void CleanUp() { FlushTimers(); FlushStats(); FlushWarnings(); }

extern ostream& DebugStream();

extern void* hmalloc(void* oldp, int size);
extern void hfree(void* e);

const int hform_bufsize=512;
// Format a string.  Uses a few rotating small buffers
// use { char s[bigv]; sprintf() or ostrstream() } if you need a big buffer
extern const char* hform(const char* format, ...);
// Free up the last hform() call.  Only use if you know what you are doing.
extern void hform_reuse();

// Allocate a duplicate of string, using new char[], use delete[]!
extern char* newString(const char* s);

extern const char* SingleQuote(const char* s);

extern int GetenvValue(const char* varname);

// val not modified if varname not found
extern bool GetenvValueF(const char* varname, float* val);

// Returns fcntl return value
extern int Setfdnodelay(int fd, int nodelay);

// Return absolute time, in secs (accuracy ~.01, better: PreciseTime.h)
extern double GetSemiPreciseTime();
// Return string of time, without a trailing '\n'
extern const char* CTime();
// Delay for some number of seconds
extern void Delay(double sec);
// Return user login name
extern const char* HhGetUserName();

extern void unsetenv(const char* name);
extern void setenv(const char* name, const char* value);

// Multi-line header, each line begins with "# " and ends with "\n"
extern const char* CreateHeader(int argc, char** argv);

// use SHOWFF, print date and header.
extern const char* CreateHeader2(int argc, char** argv);


// *** Inlines

template<class T>
inline void hqsort(T* ar, int n, int (*cmp)(const T*, const T*))
{ qsort(ar,n,sizeof(T),(int(*)(const void*,const void*))cmp); }

template<class T> inline void swap(T* e1, T* e2) { T e=*e1; *e1=*e2; *e2=e; }
template<class T> inline T sign(T e) { return e>T(0)?T(1):e<T(0)?T(-1):0; }
template<class T> inline T abs(T e) { return e<T(0)?-e:e; }
template<class T> inline T square(T e) { return e*e; }

// avoid size_t return
inline unsigned my_strlen(const char* s) { return safe_trunc_int(strlen(s)); }
#define strlen(s) my_strlen(s)

#if !defined(NAMESPACE_STD)
template<class T> inline T min(T a, T b) { return a<b?a:b; }
template<class T> inline T max(T a, T b) { return a>b?a:b; }
inline float abs(float e) { return fabsf(e); }
inline double abs(double e) { return fabs(e); }
#endif

template<class T> inline T clamp(T v, T vmin, T vmax)
{ return min(max(v,vmin),vmax); }

#if !defined(HH_NO_AVOIDS)
#define fabsf HH_AVOID_USING_FABSF
#define fabs HH_AVOID_USING_FABS
#endif

inline float macro_sqrt(float e) { ASSERTX(e>=0.f); return sqrtf(e); }
inline double macro_sqrt(double e) { ASSERTX(e>=0.); return sqrt(e); }
#define sqrt macro_sqrt
#if !defined(HH_NO_AVOIDS)
#define sqrtf HH_AVOID_USING_SQRTF
#endif

#if !defined(HH_NO_AVOIDS)
// These are brain-dead and dangerous.  Use something else like Random.h
#define rand HH_AVOID_USING_RAND
#define srand HH_AVOID_USING_SRAND
#endif

inline float torad(float deg) { return deg/180.f*PI; }
inline float todeg(float rad) { return rad/PI*180.f; }

// Prevent NaN's from appearing.
inline float myacos(float a)
{
    if (a<-1.f) {
        assertw(a>-1.001f);
        a=-1.f;
    } else if (a>1.f) {
        assertw(a<1.001f);
        a=1.f;
    }
    return acosf(a);
}

inline float mysqrt(float a)
{
    if (a<0.f) {
        assertx(a>-1e-5f);
        return 0.f;
    } else {
        return sqrt(a);
    }
}

inline double mysqrt(double a)
{
    if (a<0.) {
        assertx(a>-1e-10);
        return 0.;
    } else {
        return sqrt(a);
    }
}

static const int ar_mod3[6]={0,1,2,0,1,2};
inline int MOD3(int j)
{
    ASSERTX(j>=0 && j<6);
    return ar_mod3[j];
}

inline int MOD4(int j) { return j&0x3; }


#if 0 && defined(__WIN32)
inline bool isnanf(float f) {
    return _isnan(f)?true:false;
}
#else
// (float)0., -0.      0x00000000
// (float)1            0x3f800000
// (float)3            0x40400000
// (float)-zero        0x80000000  (-1/1e20/1e20)
// (float)-1           0xbf800000
// (float)-3           0xc0400000
// (float)1.#INF       0x7f800000  (+1/0)
// (float)-1.#INF      0xff800000  (-1/0)
// (float)-1.#IND      0xffc00000  (acos(2), 0/0)  (isnan())\
//                     0x00400000 always forced on by HW for any nanf (x86)
inline bool isfinitef(float fp) {
    float f=fp; return ((*(unsigned *)&f)&0x7f800000)!=0x7f800000;
}
inline bool isinfinitef(float fp) {
    float f=fp; return ((*(unsigned *)&f)&0x7fffffff)==0x7f800000;
}
inline bool isnanf(float fp) {
    float f=fp; return !isfinitef(f) && !isinfinitef(f);
//     return (((*(unsigned *)&(f)&0x7f800000)==0x7f800000)&& \
//             ((*(unsigned *)&(f)&0x007fffff)!=0x00000000) );
}
inline float createinfinityf() {
    float f; *(unsigned*)&f=0x7f800000; return f;
}
inline float createnanf(unsigned i=0) {
    ASSERTX((i&0xffc00000)==0);
    // Could in principle set and retrieve 0x80000000 bit, but forget it.
    float f; *(unsigned*)&f=(0xffc00000|(i&0x003fffff)); return f;
}
inline unsigned nanfvalue(float fp) {
    float f=fp; ASSERTX(isnanf(f)); return ((*(unsigned*)&f)&0x003fffff);
}
#endif

inline bool isPow2(unsigned int i)
{
    return i>0 && (i&(i-1))==0;
}

// (int)floor(log2(x))
inline int intFloorLog2(unsigned int x)
{
    int a=0;
    while (x>>=1) a++;
    return a;
}


// *** Safe CRT

#if defined(__WIN32) && defined(_MSC_VER)
#if _MSC_VER <= 1310 // VC7.1 .NET 2003 (i.e. before VC 2005)
// char *strncpy(char *strDest, const char *strSource, size_t count);
inline int strncpy_s(char *strDest, size_t sizeInBytes, const char *strSource, size_t count) {
    if (!strDest || !strSource) return EINVAL;
    // if template size_t min(size_t,size_t) is instantiated, then subsequent min(int,int) will return size_t.
    size_t len=min(unsigned(count),strlen(strSource));
    if (sizeInBytes<len+1) { *strDest=0; return EINVAL; }
    (void)strncpy(strDest,strSource,len);
    strDest[len]=0;
    return 0;
}
inline int strcpy_s(char *strDest, size_t sizeInBytes, const char *strSource) {
    if (!strDest || !strSource) return EINVAL;
    size_t len=(size_t)strlen(strSource);
    if (sizeInBytes<len+1) { *strDest=0; return EINVAL; }
    (void)strcpy(strDest,strSource);
    return 0;
}
//
// char *strncat(char *strDest, const char *strSource, size_t count);
inline int strncat_s(char *strDest, size_t sizeInBytes, const char *strSource, size_t count) {
    if (!strDest || !strSource) return EINVAL;
    // if template size_t min(size_t,size_t) is instantiated, then subsequent min(int,int) will return size_t.
    size_t lend=(size_t)strlen(strDest), lens=min(unsigned(count),strlen(strSource));
    if (sizeInBytes<lend+lens+1) return EINVAL;
    (void)strncat(strDest,strSource,lens);
    strDest[lend+lens]=0;
    return 0;
}
inline int strcat_s(char *strDest, size_t sizeInBytes, const char *strSource) {
    if (!strDest || !strSource) return EINVAL;
    size_t lend=(size_t)strlen(strDest), lens=(size_t)strlen(strSource);
    if (sizeInBytes<lend+lens+1) return EINVAL;
    (void)strcat(strDest,strSource);
    return 0;
}
#endif
#endif
#if defined(HH_STRICT_STR)
#define strncpy HH_AVOID_USING_STRNCPY
#define strcpy  HH_AVOID_USING_STRCPY
#define strncat HH_AVOID_USING_STRNCAT
#define strcat  HH_AVOID_USING_STRCAT
#endif

// Do not force me to use other *_s secure function calls.
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// *** Basic classes

class Timer;

class ConsoleProgress {
 public:
    ConsoleProgress(const char* taskname=0);
    ~ConsoleProgress();
    void set_silent();
    void show_timer();
    void update(float f);
    void clear();
 private:
    int _last_val;              // -1 if never printed
    const char* _taskname;
    bool _silent;
    Timer* _timer;
};


template<class T>
class DynamicAlloc {
 public:
    inline DynamicAlloc(T* o) : _o(o) { }
    inline ~DynamicAlloc() { delete _o; }
    inline T& operator()() { return *_o; }
 private:
    T* _o;
    inline DynamicAlloc(DynamicAlloc&) { } // DISABLE_COPY
    inline DynamicAlloc& operator=(DynamicAlloc&) { } // DISABLE_COPY
};

#define DYNAMIC_ALLOC(T,variable,value) \
DynamicAlloc< T > dynamic_##variable(value); \
T& variable=dynamic_##variable();

extern int Hh_init();
static int dummy_init_Hh=Hh_init();

#endif
