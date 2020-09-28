// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Timer_h
#define Timer_h

#if 0
{
    NEST { TIMER(__atimer2); stat; }
    NEST { TIMER(atimer2); stats; ETIMER(atimer2); ...; }
}
#endif

// GetenvValue("SHOWTIMES")==-1 -> all->Noprint
// GetenvValue("SHOWTIMES")==1 -> all but Noprint -> Default

class Timer {
 public:
    enum mode { Default, Diagnostic, Abbrev, Summary, Possibly, Noprint };
    // Default: SHOWDF every time
    // Diagnostic:      SHOWF every time
    // Abbrev:  SHOWF first time
    // Summary:  only print in summary
    // Possibly: never print, do not keep stats (except if SHOWTIMES)
    // Noprint: never print, do not keep stats
    Timer(const char* pname=0, mode=Noprint); // zero but not started
    ~Timer();
    void setname(const char* pname);
    void setmode(mode pmode);
    void zero();
    void start();
    void stop();
    void terminate();
    const char* name() const;
    float real() const;
    float user() const;
    friend ostream& operator<<(ostream& s, const Timer& t);
    static int showtimes();
    static void showtimes(int val);
 private:
    const char* _name;
    mode _mode;
    int _started;
    int _tmspu, _tmsps;         // process{user,system}
    double _real;
    static int _s_show;
    DISABLE_COPY(Timer);
};

#define TIMERN(id) Timer_##id
#define TIMER(id) Timer TIMERN(id)(#id,Timer::Default); TIMERN(id).start();
#define DTIMER(id) TIMER(id); TIMERN(id).setmode(Timer::Diagnostic);
#define ATIMER(id) TIMER(id); TIMERN(id).setmode(Timer::Abbrev);
#define STIMER(id) TIMER(id); TIMERN(id).setmode(Timer::Summary);
#define PTIMER(id) TIMER(id); TIMERN(id).setmode(Timer::Possibly);
#define ETIMER(id) TIMERN(id).terminate()

#endif
