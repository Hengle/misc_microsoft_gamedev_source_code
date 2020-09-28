// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Options_h
#define Options_h

#include "Queue.h"

class Options {
 public:
    // sample names (when multiple Options):  "Main", "Hx", "Starbase"
    Options(const char* pname="");
    ~Options();
    // Flag:  (if integer, increments count)
    void f(const char* st, int& arg,                    const char* doc="");
    void f(const char* st, const char*& arg,            const char* doc="");
    // Parameter:
    void p(const char* st, int& arg,                    const char* doc="");
    void p(const char* st, int* argp, int narg,         const char* doc="");
    void p(const char* st, float& arg,                  const char* doc="");
    void p(const char* st, float* argp, int narg,       const char* doc="");
    void p(const char* st, const char*& arg,            const char* doc="");
    void p(const char* st, const char** argp, int narg, const char* doc="");
    // Comment:
    void c(const char* st="", const char* doc="");
    // Do not skip over unrecognized args
    void allmustparse();
    // Function f is given (argc,argv) starting with the matched option.
    // It must return the number of arguments removed (incl. option).
    // A return value less than 0 indicates a parsing error.
    typedef int (*PARSEF)(int argc, char** argv);
    void p(const char* st, PARSEF f, const char* doc="");
    // Function for handling a flag
    typedef void (*HANDLEF)();
    void p(const char* st, HANDLEF f, const char* doc="");
    // Only one parse() may be active at any one time.
    // Returns 0 if either not enough arguments after last flag.
    //        or if user-defined flag handler returns value<0 .
    // Returns 1 upon success.
    // Note: argv[0] is unchanged
    // Note: changes argv[] in place!
    int parse(int& argc, char** argv);
    int noflags(int& argc, char** argv); // ret 0 if any flags remain (--)
    int noargs(int argc, char** argv);   // ret 0 if any args remain
    void problem(int argc, char** argv); // call if cannot parse
    void print_help(ostream& s) const;
    void disallow_prefixes();   // all options end with '[' if none there.
    // Parse arguments:
    static int parse_int(const char* s); // or die
    static float parse_float(const char* s); // or die
    static char parse_char(const char* s); // or die
 private:
    struct option {
        const char* str;
        PARSEF f;
        void* argp;
        int narg;               // number of arguments if f is built-in
        const char* doc;
    };
    static Options* _s_curopt;
    const char* _name;          // name of options (hx,starbase)
    char _ename[210];           // execution name: argv[0]+_name
    Queue<option*> _qoptions;
    void* _curarg;
    int _curnarg;
    int _allmustparse;
    int _disallow_prefixes;
    //
    option* match(const char* s);
    friend int fint(int argc, char** argv);
    friend int ffloat(int argc, char** argv);
    friend int fstring(int argc, char** argv);
    friend int parsequestion(int argc, char** argv);
    void iadd(const char* st, PARSEF f, void* argp, int narg,
              const char* doc);
    DISABLE_COPY(Options);
};

#define OPTSFC(opts,var,comment) opts.f("-" #var, var, comment)
#define OPTSPC(opts,var,comment) opts.p("-" #var, var, comment)
#define OPTSAC(opts,var,n,comment) opts.p("-" #var, var, n, comment)
#define OPTSDC(opts,var,comment) opts.p("-" #var, do_##var, comment)

#define OPTSF(var,comment)   opts.f("-" #var, var, comment)
#define OPTSP(var,comment)   opts.p("-" #var, var, comment)
#define OPTSA(var,n,comment) opts.p("-" #var, var, n, comment)
#define OPTSD(var,comment)   opts.p("-" #var, do_##var, comment)
#define OPTSC(comment1,com2) opts.c(comment1,com2)

#endif
