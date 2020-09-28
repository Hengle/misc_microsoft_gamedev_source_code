/**********************************************************************

Filename    :   GFxAction.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXACTION_H
#define INC_GFXACTION_H

#include "GFxActionTypes.h"
//#include "GFxStringBuiltins.h"
#include "GFxValue.h"
#include "GFxObject.h"
#include "GFxFunction.h"

#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS)
#include <wchar.h>
#endif

// ***** Declared Classes
class GASLocalFrame;
class GASEnvironment;
class GASGlobalContext; 

// ***** External Classes
class GFxStream;
class GFxASCharacter;
class GFxMovieRoot;



// ***** GASEnvironment
class GASLocalFrame : public GRefCountBase<GASLocalFrame>
{
public:
    GASStringHash<GASValue>     Variables;
    GPtr<GASLocalFrame>         PrevFrame;
    GASObjectInterface*         SuperThis; //required for "super" in SWF6
    GTL::ghash<GASFunctionObject*,int> LocallyDeclaredFuncs; // registry for locally declared funcs.

    GASLocalFrame() : SuperThis(0) {}
    ~GASLocalFrame() 
    { }

    void ReleaseFramesForLocalFuncs ();
};

#ifdef GFC_BUILD_DEBUG
#define GFX_DEF_STACK_PAGE_SIZE 8 // to debug page allocations
#else
#define GFX_DEF_STACK_PAGE_SIZE 32
#endif


// ***** GASPagedStack - Stack used for ActionScript values.

// Performance of access is incredibly important here, as most of the values 
// are passed through the stack. The class is particularly optimized to ensure
// that the following methods are inlined and as efficient as possible:
// 
//  - Top0() - accesses the element on top of stack.
//  - Top1() - accesses the element one slot below stack top.
//  - Push() - push value; parameterized to allow copy constructors.
//  - Pop1() - pop one value from stack.
//
// Additionally the stack includes the following interesting features:
//  - Address of values on stack doesn't change.
//  - A default-constructed element always lives at the bottom of the stack.
//  - Top0()/Top1() are always valid, even if nothing was pushed.
//  - Pop() is allowed even if nothing was pushed.


template <class T, int DefPageSize = GFX_DEF_STACK_PAGE_SIZE>
class GASPagedStack : public GNewOverrideBase 
{    
    // State stack value pointers for fast access.
    T*                  pCurrent;
    T*                  pPageStart;
    T*                  pPageEnd;
    T*                  pPrevPageTop;

    // Page structure. This structure is GALLOC'ed and NOT new'd because 
    // we need to manage construction/cleanup of T values manually.    
    struct Page
    {        
        T       Values[DefPageSize];
        Page*   pNext;
    };

    // Page array.
    GTL::garray<Page*>  Pages;
    // Already allocated pages in a linked list.
    Page*               pReserved;
    

    // *** Page allocator.

    Page*   NewPage()
    {   
        Page* p;
        if (pReserved) 
        {
            p = pReserved;
            pReserved = p->pNext;
        }
        else
        {
            p = (Page*) GALLOC(sizeof(Page));
        }
        return p;
    }
       
    void    ReleasePage(Page* ppage)
    {
        ppage->pNext = pReserved;
        pReserved = ppage;
    }

    // *** PushPage and PopPage implementations.

    // This is called after pCurrent has been incremented to make space
    // available for new item, i.e. when pCurrent reached pPageEnd;
    // Adds a new page to stack and adjusts pointer values to refer to it.
    void    PushPage()
    {
        GASSERT(pCurrent == pPageEnd);

        Page* pnewPage = NewPage();
        if (pnewPage)
        {
            Pages.push_back(pnewPage);
            // Previous page top == last page.
            pPrevPageTop= pPageEnd - 1;
            // Init new page pointers.
            pPageStart  = pnewPage->Values;
            pPageEnd    = pPageStart + DefPageSize;
            pCurrent    = pPageStart;            
        }
        else
        {
            // If for some reason allocation failed, we will overwrite last value.
            // (i.e. PushPage) will adjust pCurrent down by 1.
            pCurrent--;
        }
    }

    // Called from Pop() when we need to adjust pointer to the last page.
    // At his point, pCurrent should have already been decremented.
    void    PopPage()
    {
        GASSERT(pCurrent < pPageStart);

        if (Pages.size() > 1)
        {            
            ReleasePage(Pages.back());
            Pages.pop_back();

            Page* ppage = Pages.back();

            pPageStart  = ppage->Values;
            pPageEnd    = pPageStart + DefPageSize;
            pCurrent    = pPageEnd - 1;

            if (Pages.size() > 1)
            {
                pPrevPageTop = Pages[Pages.size() - 2]->Values + DefPageSize - 1;
            }
            else
            {
                // If there are no more values, point pPrevPageTop to
                // the empty value in the bottom of the stack.
                pPrevPageTop = pPageStart;
            }            
        }
        else
        {
            // Nothing to pop. Adjust pCurrent and push default
            // value back on stack.
            pCurrent++;
            GTL::gconstruct<T>(pCurrent);
        }
    }

public:

    GASPagedStack()
        : Pages(), pReserved(0)
    {
        // Always allocate one page.       
        Page *ppage = NewPage();
        GASSERT(ppage);
        
        // Initialize the first element.
        // There is always an empty 'undefined' value living at the bottom of the stack.
        Pages.reserve(15);
        Pages.push_back(ppage);
        pPageStart  = ppage->Values;        
        pPageEnd    = pPageStart + DefPageSize;
        pCurrent    = pPageStart;
        pPrevPageTop= pCurrent;
        GTL::gconstruct<T>(pCurrent);
    }

    ~GASPagedStack()        
    {
        // Call destructor for all but the last page item.
        // (Stack will refuse to destroy last bottom item).
        Pop(TopIndex());
        // Call last item destructor and release its page.
        pCurrent->~T();
        ReleasePage(Pages.back());

        // And release all reserved pages.        
        while(pReserved)
        {
            Page* pnext = pReserved->pNext;
            GFREE(pReserved);
            pReserved = pnext;
        }
    }

    void Reset()
    {
        // Call destructor for all but the last page item.
        // (Stack will refuse to destroy last bottom item).
        Pop(TopIndex());
        pCurrent->~T();
        Page *ppage = Pages.back();

        pPageStart  = ppage->Values;        
        pPageEnd    = pPageStart + DefPageSize;
        pCurrent    = pPageStart;
        pPrevPageTop= pCurrent;
        GTL::gconstruct<T>(pCurrent);
    }

    // ***** High-performance Stack Access

    // Top0() and Top1() are used most commonly to access the stack.
    // Inline and optimize them as much as possible.

    // There is *always* one valid value on stack.
    // For an empty stack this value is at index 0: (undefined).
    inline T*   Top0() const
    {        
        return pCurrent;
    }

    inline T*   Top1() const
    {     
        return (pCurrent > pPageStart) ? (pCurrent - 1) : pPrevPageTop;     
    }

    template<class V>
    inline void Push(const V& val)
    {
        pCurrent++;
        if (pCurrent >= pPageEnd)
        {
            // Adds a new page to stack and adjusts pointer values to refer to it.
            // If for some reason allocation failed, we will overwrite last value.
            // (i.e. PushPage) will adjust pCurrent down by 1.
            PushPage();
        }
        // Use copy constructor to initialize stack top.
        GTL::gconstruct_alt<T, V>(pCurrent, val);
    }

    // Pop one element from stack. 
    inline void Pop1()
    {
        pCurrent->~T();
        pCurrent--;
        if (pCurrent < pPageStart)
        {
            // We can always pop an element from stack. If these isn't one left after 
            // pop, PopPage will create a default undefined object and make it available.
            PopPage();
        }
    }

    inline void Pop2()
    {
        if ((pCurrent-2) < pPageStart)
            Pop(2);
        else
        {
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
        }
    }

    void    Pop3()
    {
        if ((pCurrent-3) < pPageStart)
            Pop(3);
        else
        {
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
            pCurrent->~T();
            pCurrent--;
        }
    }

    // General Pop(number) implementation.
    void    Pop(UInt n)
    {
        while(n > 0)
        {
            Pop1();
            n--;
        }
    }

    // Returns absolute index of the item on top (*pCurrent).
    inline UInt TopIndex () const
    {        
        return (((UInt)Pages.size() - 1) * DefPageSize) + (UInt)(pCurrent - pPageStart);
    }

    
    // *** General Top(offset) / Bottom(offset) implementation.

    // These are used less frequently to can be less efficient then above.

    T*  Top(UInt offset) const
    {
        // Compute where in stack we are...
        T*   p        = 0; 
        UInt topIndex = TopIndex();

        if (offset <= topIndex)
        {
            const UInt absidx   = topIndex - offset;
            const UInt pageidx  = absidx / DefPageSize;
            const UInt idx      = absidx % DefPageSize;
            p = &Pages[pageidx]->Values[idx];
        }
        return p;
    }

    T*      Bottom(UInt offset) const
    {
        T*   p = 0;

        if (offset <= TopIndex())
        {        
            const UInt pageidx  = offset / DefPageSize;
            const UInt idx      = offset % DefPageSize;
            p = &Pages[pageidx]->Values[idx];
        }
        return p;
    }
};



// ActionScript "environment", essentially VM state?
class GASEnvironment : public GFxLogBase<GASEnvironment>, public GNewOverrideBase
{
public:
    enum RecursionType
    {
        RG_ToString = 0,
        RG_ValueOf  = 1,

        RG_Count,
        MAX_RECURSION_LEVEL = 256
    };

protected:
    GASPagedStack<GASValue>     Stack;
    GASValue                    GlobalRegister[4];
    GTL::garray<GASValue>       LocalRegister;  // function2 uses this
    GFxASCharacter*             Target;
    // Cached global contest and SWF version fast access.
    mutable GASStringContext    StringContext;
    bool                        IsInvalidTarget; // set to true, if tellTarget can't find the target
    
    GASStringHash<GASValue>     Variables;
    GASPagedStack<GPtr<GASFunctionObject> > CallStack; // stack of calls



    struct RecursionDescr
    {
        int         CurLevel;
        int         MaxRecursionLevel;

        RecursionDescr(int maxRecursionLevel):CurLevel(1),MaxRecursionLevel(maxRecursionLevel) {}
    };

    typedef GTL::ghash<const void*, RecursionDescr, GTL::gidentity_hash<const void*> > RecursionGuardHash;

    GTL::garray<RecursionGuardHash >    RecursionGuards; // recursion guards
public:

    // For local vars.  Use empty names to separate frames.
    GTL::garray<GPtr<GASLocalFrame> >   LocalFrames;
    
    //AB: do we need to store whole stack here?
    //const GTL::garray<GASWithStackEntry>* pWithStack; 


    GASEnvironment()
        :
        Target(0),//,pWithStack(0) //, Stack (200) // <- temp
        IsInvalidTarget(false)        
    {
    
    }

    ~GASEnvironment() 
    { }

    

    void            SetTarget(GFxASCharacter* ptarget);     
    void            SetInvalidTarget(GFxASCharacter* ptarget);
    // Used to set target right after construction
    void            SetTargetOnConstruct(GFxASCharacter* ptarget);
    inline bool     IsTargetValid() const { return !IsInvalidTarget; }

    inline GFxASCharacter*  GetTarget() const       {  GASSERT(Target); return Target;  }
    
    // LogBase implementation.
    GFxLog*         GetLog() const;         
    bool            IsVerboseAction() const;
    bool            IsVerboseActionErrors() const;

    // Determines if gfxExtensions are enabled; returns 1 if enabled, 0 otherwise.
    bool            CheckExtensions() const;

    // Helper similar to GFxCharacter.  
    GFxMovieRoot*   GetMovieRoot() const;
    inline UInt     GetVersion() const                  { return StringContext.GetVersion(); }
    inline bool     IsCaseSensitive() const      { return StringContext.IsCaseSensitive(); }    
    
    // These are used *A LOT* - short names for convenience.
    // GetGC: Always returns a global object, never null.
    inline GASGlobalContext*   GetGC() const            { return StringContext.pContext; }
    // GetSC: Always returns string context, never null.
    inline GASStringContext*   GetSC() const            { return &StringContext; }
    // Convenient prototype access (delegates to GC).
    inline GPtr<GASObject>     GetPrototype(GASBuiltinType type) const;

    
    // Inline to built-in access through the context
    inline const GASString&  GetBuiltin(GASBuiltinType btype) const;
    inline GASString  CreateConstString(const char *pstr) const;
    inline GASString  CreateString(const char *pstr) const;
    inline GASString  CreateString(const wchar_t *pwstr) const;
    inline GASString  CreateString(const char *pstr, size_t length) const;
    inline GASString  CreateString(const GFxString& str) const;


    // Stack access/manipulation
    // @@ TODO do more checking on these
    template<class V>
    inline void        Push(const V& val)          { Stack.Push(val); }
    inline void        Drop1()                     { Stack.Pop1(); }
    inline void        Drop2()                     { Stack.Pop2(); }
    inline void        Drop3()                     { Stack.Pop3(); }
    inline void        Drop(UInt count)            { Stack.Pop(count); }
    inline GASValue&   Top()                       { return *Stack.Top0(); }   
    inline GASValue&   Top1()                      { return *Stack.Top1(); }
    inline GASValue&   Top(UInt dist)              { return *Stack.Top(dist); }
    inline GASValue&   Bottom(UInt index)          { return *Stack.Bottom (index); }
    inline int         GetTopIndex() const         { return int(Stack.TopIndex()); }

    enum ExcludeFlags {
        IgnoreLocals = 0x1,     // ignore local vars (incl this)
        IgnoreContainers = 0x2  // ignore _root,_levelN,_global
    };

    bool    GetVariable(const GASString& varname, GASValue* presult, const GTL::garray<GASWithStackEntry>* pwithStack = NULL, 
                        GFxASCharacter **ppnewTarget = 0, GASValue* powner = 0, UInt excludeFlag = 0) const;
    // Similar to GetVariable, determines if a var is available based on varName path.
    bool    IsAvailable(const GASString& varname, const GTL::garray<GASWithStackEntry>* pwithStack = NULL) const;
    // no path stuff:
    bool    GetVariableRaw(const GASString& varname, GASValue* presult, const GTL::garray<GASWithStackEntry>* pwithStack = NULL,
                           GASValue* powner = 0, UInt excludeFlag = 0) const;
    // find owner of varname. Return UNDEFINED for "this","_root","_levelN","_global".
    bool    FindOwnerOfMember (const GASString& varname, GASValue* presult,
                               const GTL::garray<GASWithStackEntry>* pwithStack = NULL) const;

    // find variable by path. if onlyTarget == true then only characters can appear in the path
    bool    FindVariable(const GASString& path, GASValue* presult, const GTL::garray<GASWithStackEntry>* pwithStack = NULL, 
                         GFxASCharacter **pplastTarget = 0, GASValue* powner = 0, bool onlyTargets = false, GASString* varName = 0) const;

    bool    SetVariable(const GASString& path, const GASValue& val, const GTL::garray<GASWithStackEntry>* pwithStack = NULL, bool doDisplayErrors = true);
    // no path stuff:
    void    SetVariableRaw(const GASString& path, const GASValue& val, const GTL::garray<GASWithStackEntry>* pwithStack = NULL);

    void    SetLocal(const GASString& varname, const GASValue& val);
    void    AddLocal(const GASString& varname, const GASValue& val);    // when you know it doesn't exist.
    void    DeclareLocal(const GASString& varname); // Declare varname; undefined unless it already exists.

    bool    GetMember(const GASString& varname, GASValue* val) const;
    void    SetMember(const GASString& varname, const GASValue& val);
    bool    DeleteMember(const GASString& name);
    void    VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags = 0) const;

    // Parameter/local stack frame management.
    int     GetLocalFrameTop() const { return (UInt)LocalFrames.size(); }
    void    SetLocalFrameTop(UInt t) { GASSERT(t <= LocalFrames.size()); LocalFrames.resize(t); }
    void    AddFrameBarrier() { LocalFrames.push_back(0); }
    
    GASLocalFrame* GetTopLocalFrame (int off = 0) const;
    GASLocalFrame* CreateNewLocalFrame ();

    // Local registers.
    void    AddLocalRegisters(UInt RegisterCount)
    {
        LocalRegister.resize(LocalRegister.size() + RegisterCount);
    }
    void    DropLocalRegisters(UInt RegisterCount)
    {
        LocalRegister.resize(LocalRegister.size() - RegisterCount);
    }
    GASValue*   LocalRegisterPtr(UInt reg);

    // Internal.
    //?int  FindLocal(const GFxString& varname) const;
    bool            FindLocal(const GASString& varname, GASValue* pvalue) const;
    GASValue*       FindLocal(const GASString& varname);
    const GASValue* FindLocal(const GASString& varname) const;  
    GFxASCharacter* FindTarget(const GASString& path) const;
    GFxASCharacter* FindTargetByValue(const GASValue& val);

    GPtr<GASObject> OperatorNew(const GASFunctionRef& constructor, int nargs = 0, int argsTopOff = -1);
    GPtr<GASObject> OperatorNew(const GASString &className, int nargs = 0, int argsTopOff = -1);

    GASFunctionRef  GetConstructor(GASBuiltinType className);

    static bool     ParsePath(GASStringContext* psc, const GASString& varPath, GASString* path, GASString* var);
    static bool     IsPath(const GASString& varPath);

    // used from Shutdown to force release of local frames to prevent mem leak.
    void            ForceReleaseLocalFrames ();

    GASValue        PrimitiveToTempObject(int index);

    inline void     CallPush(const GASFunctionObject* pfuncObj)
    {
        CallStack.Push(GPtr<GASFunctionObject>(const_cast<GASFunctionObject*>(pfuncObj)));
    }
    inline void     CallPop()
    {
        GASSERT(CallStack.TopIndex() >= 0);
        CallStack.Pop1();
    }
    inline GASValue CallTop(int off) const
    {
        if (CallStack.TopIndex() < (UInt)off)
            return GASValue(GASValue::NULLTYPE);
        else
            return GASValue(CallStack.Top(off)->GetPtr());
    }

    inline GASValue& GetGlobalRegister(int r)
    {
        GASSERT(r >= 0 && UInt(r) < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]));
        return GlobalRegister[r];
    }
    inline const GASValue& GetGlobalRegister(int r) const
    {
        GASSERT(r >= 0 && UInt(r) < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]));
        return GlobalRegister[r];
    }

    // recursion guard for toString, valueOf, etc methods
    bool RecursionGuardStart(RecursionType type, const void* ptr, int maxRecursionAllowed = MAX_RECURSION_LEVEL);
    void RecursionGuardEnd(RecursionType type, const void* ptr);

    void Reset();
};

//
// Some handy helpers
//

// Global Context, created once for a root movie
class GASGlobalContext : public GRefCountBase<GASGlobalContext>, public GASStringBuiltinManager
{
    GTL::ghash_uncached<GASBuiltinType, GPtr<GASObject> >    Prototypes;
    GASStringHash<GASFunctionRef>                            RegisteredClasses;    
public:
    
    // Constructor, initializes the context variables
    GASGlobalContext(GFxMovieRoot*);
    ~GASGlobalContext();

    // Publicly accessible global members
    // ActionScript global object, corresponding to '_global'
    GPtr<GASObject>             pGlobal;

    Bool3W                      GFxExtensions;
 
    // A map of standard member names for efficient access. This hash
    // table is used by GASCharacter implementation. The values in it
    // are of GFxASCharacter::StandardMember type.
    GASStringHash<SByte>        StandardMemberMap;


    void GetPrototype(GPtr<GASObject>* pobjPtr, GASBuiltinType type) const
    {        
        Prototypes.get(type, pobjPtr);
    }

    GPtr<GASObject> GetPrototype(GASBuiltinType type) const
    {
        GPtr<GASObject> result;
        GetPrototype(&result, type);        
        return result;
    }
    

    // registerClass related methods
    bool RegisterClass(GASStringContext* psc, const GASString& className, const GASFunctionRef& ctorFunction);
    bool UnregisterClass(GASStringContext* psc, const GASString& className);
    bool FindRegisteredClass(GASStringContext* psc, const GASString& className, GASFunctionRef* pctorFunction);
    void UnregisterAllClasses();

    // Escape functions - src length required.
    static void EscapeWithMask(const char* psrc, UPInt length, GFxString* pescapedStr, const UInt* escapeMask);
    static void EscapePath(const char* psrc, UPInt length, GFxString* pescapedStr);
    static void Escape(const char* psrc, UPInt length, GFxString *pescapedStr);
    static void Unescape(const char* psrc, UPInt length, GFxString *punescapedStr);

    static void EscapeSpecialHTML(const char* psrc, UPInt length, GFxString* pescapedStr);
    static void UnescapeSpecialHTML(const char* psrc, UPInt length, GFxString* punescapedStr);

    static GASObject* AddPackage(GASStringContext *psc, GASObject* pparent, GASObject* objProto, const char* const packageName);
    
    GASString FindClassName(GASStringContext *psc, GASObjectInterface* iobj);

    inline bool CheckExtensions() const { return GFxExtensions.IsTrue(); }
protected:   
    void    InitStandardMembers();
};



// Inline in environment for built-in access.
inline const GASString&     GASEnvironment::GetBuiltin(GASBuiltinType btype) const
{    
    return (const GASString&)GetGC()->GetBuiltinNodeHolder(btype);
}
inline GASString  GASEnvironment::CreateString(const char *pstr) const
{
    return GetGC()->CreateString(pstr);
}
inline GASString  GASEnvironment::CreateString(const wchar_t *pwstr) const
{
    return GetGC()->CreateString(pwstr);
}
inline GASString  GASEnvironment::CreateConstString(const char *pstr) const
{
    return GetGC()->CreateConstString(pstr);
}
inline GASString  GASEnvironment::CreateString(const char *pstr, size_t length) const
{
    return GetGC()->CreateString(pstr, length);
}
inline GASString  GASEnvironment::CreateString(const GFxString& str) const
{
    return GetGC()->CreateString(str);
}
inline GPtr<GASObject> GASEnvironment::GetPrototype(GASBuiltinType type) const
{
    GPtr<GASObject> result;
    GetGC()->GetPrototype(&result, type);
    return result;
}
inline bool GASEnvironment::CheckExtensions() const
{
    return GetGC()->CheckExtensions();
}


// Context-dependent inlines in GASStringContext.
inline const GASString& GASStringContext::GetBuiltin(GASBuiltinType btype) const
{
    return (const GASString&)pContext->GetBuiltinNodeHolder(btype);
}
inline GASString    GASStringContext::CreateConstString(const char *pstr) const
{
    return pContext->CreateConstString(pstr);
}
inline GASString    GASStringContext::CreateString(const char *pstr) const
{
    return pContext->CreateString(pstr);
}
inline GASString    GASStringContext::CreateString(const wchar_t *pwstr) const
{
    return pContext->CreateString(pwstr);
}
inline GASString    GASStringContext::CreateString(const char *pstr, size_t length) const
{
    return pContext->CreateString(pstr, length);
}
inline GASString    GASStringContext::CreateString(const GFxString& str) const
{
    return pContext->CreateString(str);
}
inline bool         GASStringContext::CompareConstString_CaseCheck(const GASString& pstr1, const char* pstr2)
{
    return CreateConstString(pstr2).Compare_CaseCheck(pstr1, IsCaseSensitive());
}
inline bool         GASStringContext::CompareConstString_CaseInsensitive(const GASString& pstr1, const char* pstr2)
{
    return CreateConstString(pstr2).Compare_CaseCheck(pstr1, false);
}


// Dispatching methods from C++.
bool        GAS_InvokeParsed(
                const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthisPtr,
                GASEnvironment* penv,
                const char* pmethodArgFmt, 
                va_list args);

bool        GAS_Invoke(
                const GASValue& method,
                GASValue* presult,
                GASObjectInterface* pthisPtr,
                GASEnvironment* penv, 
                int nargs, int firstArgBottomIndex);

bool        GAS_Invoke(
                const GASValue& method,
                GASValue* presult,
                const GASValue& thisPtr,
                GASEnvironment* penv, 
                int nargs, int firstArgBottomIndex);

bool        GAS_Invoke(
                const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthis,
                GASEnvironment* penv,
                int numArgs, int firstArgBottomIndex);

inline bool GAS_Invoke0(const GASValue& method,
                        GASValue* presult,
                        GASObjectInterface* pthis,
                        GASEnvironment* penv)
{
    return GAS_Invoke(method, presult, pthis, penv, 0, penv->GetTopIndex() + 1);
}

#ifdef GFC_OS_WIN32
#define snprintf _snprintf
#endif // GFC_OS_WIN32

#ifdef GFC_OS_SYMBIAN
#include <stdarg.h>

static inline int snprintf(char *out, int n, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = gfc_vsprintf(out,fmt,args);
    va_end(args);
    return ret;
}
#endif

#endif // INC_GFXACTION_H
