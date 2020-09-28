//=========================================================================
// RefObj.hpp
//=========================================================================

#ifndef __RefObj__
#define __RefObj__

namespace Etl
{

class RefObj
{
public:

   RefObj() : m_lRefCount(1) {}

   inline long ref(void)   
   { 
      return ++m_lRefCount; 
   }

   inline long unref(void) 
   { 
      BASSERT(m_lRefCount >= 1);
      return --m_lRefCount; 
   }

   inline long refCount(void) const 
   { 
      return m_lRefCount; 
   }

private:

   long m_lRefCount;

}; 

#define ETL_COUNTED_CLASS         \
   Etl::RefObj  refobj;           \
   long ref(void)                 \
   {                              \
      return refobj.ref();        \
   }                              \
   long unref(void)               \
   {                              \
      return refobj.unref();      \
   }
   

}

#endif
