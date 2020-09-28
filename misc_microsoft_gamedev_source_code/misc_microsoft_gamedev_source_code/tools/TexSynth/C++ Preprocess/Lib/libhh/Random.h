// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Random_h
#define Random_h

// Environment variable "SEED_RANDOM" overrides default seed 0 for Random::G

class Random {
 public:
    static Random G;            // can be uninitialized before main()!
    Random(unsigned seed=0);
    void setseed(unsigned seed);
    ~Random();
    unsigned getuint();
    float unif();
    double dunif();
    float gauss();
    double dgauss();
 private:
    int _sequencer;
    unsigned _sequence_table[64];

    DISABLE_COPY(Random);
};

#endif
