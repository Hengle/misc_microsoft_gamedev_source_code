// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BQem_h
#define BQem_h

#include "Qem.h"

template<class T>
class BQem {
 public:
    BQem() { }
    virtual ~BQem() { }
    virtual void set_zero()=0;
    virtual void copy(const BQem<T>& qem)=0;
    virtual void add(const BQem<T>& qem)=0;
    virtual void scale(float f)=0;
    virtual void set_d2_from_plane(const float* dir, float d)=0;
    virtual void set_d2_from_point(const float* p0)=0;
    virtual void set_distance_gh98(const float* p0, const float* p1,
                           const float* p2)=0;
    virtual void set_distance_hh99(const float* p0, const float* p1,
                           const float* p2)=0;
    virtual float evaluate(const float* p) const=0;
    virtual bool compute_minp(float* minp) const=0;
    virtual bool compute_minp_constr_first(float* minp, int nfixed) const=0;
    virtual bool compute_minp_constr_lf(float* minp, const float* lf) const=0;
    virtual bool fast_minp_constr_lf(float* minp, const float* lf) const=0;
    virtual bool ar_compute_minp(const Array<BQem<T>*>& ar_q,
                                 Matrix<float>& minp) const=0;
    virtual bool ar_compute_minp_constr_lf(const Array<BQem<T>*>& ar_q,
                                           Matrix<float>& minp,
                                           const float* lf) const=0;
    virtual void print_cerr() const=0;
    virtual bool check_type(int ptsize, int pn) const=0;
 private:
    DISABLE_COPY(BQem);
};

template<class T, int n>
class DQem : public BQem<T> {
 public:
    DQem()
    { }
    virtual ~DQem()
    { }
    virtual void set_zero()
    { _q.set_zero(); }
    virtual void copy(const BQem<T>& qem) {
        ASSERTX(qem.check_type(sizeof(T),n));
        _q=((const DQem<T,n>&)qem)._q;
    }
    virtual void add(const BQem<T>& qem) {
        ASSERTX(qem.check_type(sizeof(T),n));
        _q.add(((const DQem<T,n>&)qem)._q);
    }
    virtual void scale(float f)
    { _q.scale(f); }
    virtual void set_d2_from_plane(const float* dir, float d)
    { _q.set_d2_from_plane(dir,d); }
    virtual void set_d2_from_point(const float* p0)
    { _q.set_d2_from_point(p0); }
    virtual void set_distance_gh98(const float* p0, const float* p1,
                                   const float* p2)
    { _q.set_distance_gh98(p0,p1,p2); }
    virtual void set_distance_hh99(const float* p0, const float* p1,
                                   const float* p2)
    { _q.set_distance_hh99(p0,p1,p2); }
    virtual float evaluate(const float* p) const
    { return _q.evaluate(p); }
    virtual bool compute_minp(float* minp) const
    { return _q.compute_minp(minp); }
    virtual bool compute_minp_constr_first(float* minp, int nfixed) const
    { return _q.compute_minp_constr_first(minp,nfixed); }
    virtual bool compute_minp_constr_lf(float* minp, const float* lf) const
    { return _q.compute_minp_constr_lf(minp,lf); }
    virtual bool fast_minp_constr_lf(float* minp, const float* lf) const
    { return _q.fast_minp_constr_lf(minp, lf); }
    virtual bool ar_compute_minp(const Array<BQem<T>*>& ar_q, Matrix<float>& minp) const {
        assertx(((Array<DQem<T,n>*>&)ar_q)[0]==this);
        static Array<Qem<T,n>*> ar_q2; ar_q2.init(ar_q.num());
        ForIndex(i,ar_q.num()) {
            ar_q2[i]=&((Array<DQem<T,n>*>&)ar_q)[i]->_q;
        } EndFor;
        assertx(ar_q2[0]==&_q);
        return _q.ar_compute_minp(ar_q2,minp);
    }
    virtual bool ar_compute_minp_constr_lf(const Array<BQem<T>*>& ar_q, Matrix<float>& minp, const float* lf) const {
        assertx(((Array<DQem<T,n>*>&)ar_q)[0]==this);
        static Array<Qem<T,n>*> ar_q2; ar_q2.init(ar_q.num());
        ForIndex(i,ar_q.num()) {
            ar_q2[i]=&((Array<DQem<T,n>*>&)ar_q)[i]->_q;
        } EndFor;
        assertx(ar_q2[0]==&_q);
        return _q.ar_compute_minp_constr_lf(ar_q2,minp,lf);
    }
    virtual void print_cerr() const
    { _q.print_cerr(); }
    virtual bool check_type(int ptsize, int pn) const
    { return ptsize==sizeof(T) && pn==n; }
 private:
    Qem<T,n> _q;
};

#endif
