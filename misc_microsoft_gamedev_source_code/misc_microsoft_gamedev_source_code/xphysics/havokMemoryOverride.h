//==============================================================================
// havokMemoryOverride.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _HAVOKMEMORYOVERRIDE_H_
#define _HAVOKMEMORYOVERRIDE_H_

// include hkMemory.h then override the allocation macros
//@Wrench - If we are tracking memory, declare a new and delete with 3 parameters, so that creation of havok elements
//will still work with our #defined NEW.  These macros are taken from hkMemory.h and redefined here.  mwc
#if MEMORY_MANAGER_ENABLE_TRACKING

#include <Common/Base/Types/hkBaseTypes.h>
#include <Common/Base/Memory/Memory/hkMemory.h>

#undef HK_DECLARE_CLASS_ALLOCATOR
#undef HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR_BY_SIZE
#undef HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR

#define HK_DECLARE_CLASS_ALLOCATOR(TYPE) \
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t nbytes)		{ HK_ASSERT_OBJECT_SIZE_OK(nbytes); hkReferencedObject* b = static_cast<hkReferencedObject*>(hkMemory::getInstance().allocateChunk(nbytes,TYPE)); b->m_memSizeAndFlags = static_cast<hkUint16>(nbytes); return b; }	\
   HK_FORCE_INLINE void  HK_CALL operator delete(void* p)			{ hkReferencedObject* b = static_cast<hkReferencedObject*>(p); hkMemory::getInstance().deallocateChunk(p, (b->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE),TYPE); }	\
   HK_FORCE_INLINE void* HK_CALL operator new[](hk_size_t nbytes)	{ hkReferencedObject* b = static_cast<hkReferencedObject*>(hkMemory::getInstance().allocateChunk(nbytes,TYPE)); b->m_memSizeAndFlags = static_cast<hkUint16>(nbytes); return b; }	\
   HK_FORCE_INLINE void  HK_CALL operator delete[](void* p)			{ hkReferencedObject* b = static_cast<hkReferencedObject*>(p); hkMemory::getInstance().deallocateChunk(p, (b->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE),TYPE); }	\
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t, void* p)	{ return p; }	\
   HK_FORCE_INLINE void  HK_CALL operator delete(void*, void*)		{ }	\
   HK_FORCE_INLINE void* HK_CALL operator new[](hk_size_t, void* p)	{ HK_BREAKPOINT(); return p; }	\
   HK_FORCE_INLINE void  HK_CALL operator delete[](void*, void*)	{ HK_BREAKPOINT();	}  \
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t nbytes, const char* pFileName, long lineNumber)		{ HK_ASSERT_OBJECT_SIZE_OK(nbytes); hkReferencedObject* b = static_cast<hkReferencedObject*>(hkMemory::getInstance().allocateChunk(nbytes,TYPE)); b->m_memSizeAndFlags = static_cast<hkUint16>(nbytes); return b; }	\
   HK_FORCE_INLINE void  HK_CALL operator delete(void* p, const char* pFileName, long lineNumber)			{ hkReferencedObject* b = static_cast<hkReferencedObject*>(p); hkMemory::getInstance().deallocateChunk(p, b->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE,TYPE); }	


#define HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR_BY_SIZE(TYPE,CLASS_SIZE) \
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t nbytes)		{ HK_ASSERT(0x6c787b7f, nbytes == CLASS_SIZE ); return hkMemory::getInstance().allocateChunk(nbytes,TYPE); }	\
   HK_FORCE_INLINE void  HK_CALL operator delete(void* p)			{ hkMemory::getInstance().deallocateChunk(p, CLASS_SIZE, TYPE); }								\
   HK_FORCE_INLINE void* HK_CALL operator new[](hk_size_t nbytes)	{ return hkMemory::getInstance().allocate(nbytes,TYPE); }												\
   HK_FORCE_INLINE void  HK_CALL operator delete[](void* p)			{ hkMemory::getInstance().deallocate(p); }															\
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t n, void* p)	{ HK_ASSERT(0x77bb90a1, n == CLASS_SIZE); return p; }										\
   HK_FORCE_INLINE void  HK_CALL operator delete(void*, void*)		{ }																						\
   HK_FORCE_INLINE void* HK_CALL operator new[](hk_size_t, void* p)	{ return p;	}																			\
   HK_FORCE_INLINE void  HK_CALL operator delete[](void*, void*)	{ }							\
   HK_FORCE_INLINE void* HK_CALL operator new(hk_size_t nbytes, const char* pFileName, long lineNumber)		{ HK_ASSERT(0x6c787b7f, nbytes == CLASS_SIZE ); return hkMemory::getInstance().allocateChunk(nbytes,TYPE); }	\
   HK_FORCE_INLINE void  HK_CALL operator delete(void* p, const char* pFileName, long lineNumber)			{ hkMemory::getInstance().deallocateChunk(p, CLASS_SIZE, TYPE); }	


#define HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(TYPE,THIS_CLASS) \
   HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR_BY_SIZE(TYPE, sizeof(THIS_CLASS))

#endif

#endif

