// Generalized functor implementation. Concept inspired by Andrei Alexandrescu. 
// Copyright Aleksei Trunov 2005 
// Use, copy, modify, distribute and sell it for free.
// "Yet Another Generalized Functors Implementation in C++"
// http://www.codeproject.com/cpp/genfunctors.asp
// rg [5/25/06] - Dunno if we can use this. We can always use Boost's functors once we port it to Xbox (but Boost's aren't as efficient).

#ifndef _FUNCTOR_H_
#define _FUNCTOR_H_

#include "Generic.h"
#include "FunTraits.h"
#include "FunCall.h"

namespace GF {

// Generalized functor class template 

const uint cTypelessStorageSize = 4;
template <typename R, class TList, unsigned int size = cTypelessStorageSize * sizeof(void*)>
class Functor 
{
public:
	typedef R ResultType;
	typedef TList TypeListType;
	typedef typename CallParms<TList>::ParmsListType ParmsListType;
	// default construction, assignment and destruction
	Functor() : vptr_(0) {}
	~Functor()
	{
		if (vptr_) vptr_->destroy_(*this); 
	}
	Functor(Functor const& src) 
	{
		vptr_ = src.vptr_ ? src.vptr_->clone_(src, *this) : 0;
	}
	Functor& operator=(Functor const& src)
	{
		if (this != &src) {
			if (vptr_) vptr_->destroy_(*this); 
			vptr_ = src.vptr_ ? src.vptr_->clone_(src, *this) : 0;
		}
		return *this;
	}
	// is-empty selector
	bool operator!() const { return vptr_ == 0; }
	// ctor for static fns and arbitrary functors 
	template <typename F> explicit Functor(F const& fun) 
	{ 
		typedef FunctorImpl<F> StoredType;
		vptr_ = _init<StoredType>(fun);
	}
	// ctor for member fns (note: raw ptrs and smart ptrs are equally welcome in pobj)
	template <class P, typename MF> explicit Functor(P const& pobj, MF memfun) 
	{
		typedef MemberFnImpl<P, MF> StoredType;
		vptr_ = _init<StoredType>(std::pair<P, MF>(pobj, memfun));
	}
	// calls 
	typedef typename GU::TypeAtNonStrict<TList, 0, GU::NullType>::Result Parm1;
	typedef typename GU::TypeAtNonStrict<TList, 1, GU::NullType>::Result Parm2;
	typedef typename GU::TypeAtNonStrict<TList, 2, GU::NullType>::Result Parm3;
	typedef typename GU::TypeAtNonStrict<TList, 3, GU::NullType>::Result Parm4;
	typedef typename GU::TypeAtNonStrict<TList, 4, GU::NullType>::Result Parm5;
	typedef typename GU::TypeAtNonStrict<TList, 5, GU::NullType>::Result Parm6;
	typedef typename GU::TypeAtNonStrict<TList, 6, GU::NullType>::Result Parm7;
#define DoCall(parms) return vptr_->call_(*this, parms);
	inline R operator()(ParmsListType const& parms) const { DoCall(parms) }
	inline R operator()() const { DoCall(CallParms<TList>::Make()) }
	inline R operator()(Parm1 p1) const { DoCall(CallParms<TList>::Make(p1)) }
	inline R operator()(Parm1 p1, Parm2 p2) const { DoCall(CallParms<TList>::Make(p1, p2)) }
	inline R operator()(Parm1 p1, Parm2 p2, Parm3 p3) const { DoCall(CallParms<TList>::Make(p1, p2, p3)) }
	inline R operator()(Parm1 p1, Parm2 p2, Parm3 p3, Parm4 p4) const { DoCall(CallParms<TList>::Make(p1, p2, p3, p4)) }
	inline R operator()(Parm1 p1, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5) const { DoCall(CallParms<TList>::Make(p1, p2, p3, p4, p5)) }
	inline R operator()(Parm1 p1, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6) const { DoCall(CallParms<TList>::Make(p1, p2, p3, p4, p5, p6)) }
	inline R operator()(Parm1 p1, Parm2 p2, Parm3 p3, Parm4 p4, Parm5 p5, Parm6 p6, Parm7 p7) const { DoCall(CallParms<TList>::Make(p1, p2, p3, p4, p5, p6, p7)) }
private:
	// copying/destruction and calls implementation
	struct FunImplBase
	{
		struct VTable;
		struct VTable
		{
			void (*destroy_)(Functor const&);
			VTable* (*clone_)(Functor const&, Functor&);
			R (*call_)(Functor const&, ParmsListType);
		};
		// VTable vtbl_;	// not needed here and actually wastes space!
	};
	template <typename V, class Derived>
	struct FunStorageImpl : public FunImplBase
	{
		V val_;
		FunStorageImpl(V const& val) : val_(val) {}
		static void Destroy(Functor const& src) { src.val_.template destroy<Derived>(); }
		static typename FunImplBase::VTable* Clone(Functor const& src, Functor& dest) 
		{ 
			Derived const& this_ = src.val_.template get<Derived const>();
			return dest._init<Derived>(this_.val_); 
		}
	};
	template <typename T>
	struct FunctorImpl : public FunStorageImpl<T, FunctorImpl<T> >
	{
		FunctorImpl(T const& val) : FunStorageImpl<T, FunctorImpl>(val) {}
		static R Call(Functor const& src, ParmsListType parms) 
		{ 
			FunctorImpl const& this_ = src.val_.template get<FunctorImpl const>();
			return FunctorCall<T, R, TList>::Call(this_.val_, parms); 
		}
	};
	template <class P, typename T>
	struct MemberFnImpl : public FunStorageImpl<std::pair<P, T>, MemberFnImpl<P, T> >
	{
		MemberFnImpl(std::pair<P, T> const& val) : FunStorageImpl<std::pair<P, T>, MemberFnImpl>(val) {}
		static R Call(Functor const& src, ParmsListType parms) 
		{ 
			MemberFnImpl const& this_ = src.val_.template get<MemberFnImpl const>();
			return FunctorCall<T, R, TList>::Call(this_.val_.first, this_.val_.second, parms); 
		}
	};
	// initialization helper
	template <class T, class V>
	typename FunImplBase::VTable* _init(V const& v)
	{
		FunImplBase* pimpl = val_.template init<T>(v);
		pimpl; // throw away pimpl, we don't need it in this implementation
		static typename FunImplBase::VTable vtbl =
		{
			&T::Destroy,
			&T::Clone,
			&T::Call,
		};
		return &vtbl;
	}
	// typeless storage support
	struct Typeless
	{
		template <typename T> inline T* init1(T* v) { return new(getbuf()) T(v); }
		template <typename T, typename V> inline T* init(V const& v) { return new(getbuf()) T(v); }
    	template <typename T> inline void destroy() const { (*reinterpret_cast<T const*>(getbuf())).~T(); }
		template <typename T> inline T const& get() const { return *reinterpret_cast<T const*>(getbuf()); }
		template <typename T> inline T& get() { return *reinterpret_cast<T*>(getbuf()); }
		void* getbuf() { return &buffer_[0]; }
		void const* getbuf() const { return &buffer_[0]; }
		unsigned char buffer_[size];
	};
	template <typename T>
	struct ByValue
	{
		template <typename V> inline static T* init(Typeless& val, V const& v) { return val.template init<T>(v); }
		inline static void destroy(Typeless const& val) { val.template destroy<T>(); }
		inline static T const& get(Typeless const& val) { return val.get<T>(); }
		inline static T& get(Typeless& val) { return val.get<T>(); }
	};
	template <typename T>
	struct NewAlloc
	{
		template <typename V> inline static T* init(Typeless& val, V const& v) { return *val.template init<T*>(new T(v)); }
		inline static void destroy(Typeless const& val) { delete val.get<T*>(); }
		inline static T const& get(Typeless const& val) { return *val.get<T const*>(); }
		inline static T& get(Typeless& val) { return *val.get<T*>(); }
	};
	template <typename T>
	struct SelectStored 
	{ 
		// TODO: it seems this is a good place to add alignment calculations
		typedef typename GU::Select<
			sizeof(T)<=sizeof(Typeless), 
			ByValue<T>, 
			NewAlloc<T> 
		>::Result Type; 
	};
	struct Stored 
	{ 
		template <typename T, typename V> inline T* init(V const& v) { return SelectStored<T>::Type::init(val_, v); }
		template <typename T> inline void destroy() const { SelectStored<T>::Type::destroy(val_); }
		template <typename T> inline T const& get() const { return SelectStored<T>::Type::get(val_); }
		template <typename T> inline T& get() { return SelectStored<T>::Type::get(val_); }
		Typeless val_;
	};
	Stored val_;
	typename FunImplBase::VTable* vptr_;
};

// Helper functor creation functions

template <typename CallType> inline 
Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType> 
MakeFunctor(CallType fun)
{
	return Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType>(fun);
}
template <typename CallType, class PObj> inline 
Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType> 
MakeFunctor(CallType memfun, PObj const& pobj)
{
	return Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType>(pobj, memfun);
}
template <typename CallType, class Fun> inline 
Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType> 
MakeFunctor(Fun const& fun)
{
	return Functor<typename GU::FunTraits<CallType>::ResultType, typename GU::FunTraits<CallType>::TypeListType>(fun);
}

}

#endif // _FUNCTOR_H_
