// --------------------------------------------------------------------------
/*

  Some memory management tools

  Sylvain Lefebvre - (c) Microsoft Corp. - 2005-09-29

*/
// --------------------------------------------------------------------------
#ifndef __MEM_TOOLS__
#define __MEM_TOOLS__
// --------------------------------------------------------------------------

/*
  Memory management exception
*/

#pragma warning(disable : 4995)

#include <cstdarg>
#include <cstdio>

#ifdef WIN32
#include <windows.h>
#endif

class MemoryException
{
protected:
  char m_szMsg[512];
public:

  MemoryException(){m_szMsg[0]='\0';}

  MemoryException(char *msg,...)
  {
    va_list args;
    va_start(args, msg);

    sprintf(m_szMsg,msg,args);

#ifdef WIN32
    OutputDebugStringA(m_szMsg);
    OutputDebugStringA("\n");
#endif

    va_end(args);
  }

  const char *getMsg() const {return (m_szMsg);}
};


// --------------------------------------------------------------------------

/*
  TemporaryObject holds a pointer until the end of the scope of the variable.
  Then the objects gets destroyed.
*/

template <class T_Type> class TemporaryObject
{
private:

  T_Type *m_Object;

public:

  TemporaryObject(T_Type *o)       { m_Object=o; }

  ~TemporaryObject()               { delete (m_Object); }

  // cast to pointer
  operator T_Type* ()              {return (m_Object);}
  // dereference
  T_Type& operator*()  const       {return (*m_Object);}
  // member access
  T_Type* operator->() const       {return (m_Object);}

};

// --------------------------------------------------------------------------

/*
  TableAllocator allocates a buffer and free it when object gets destroyed. 
  Data can also be cloned.
*/

template <class T_Type,bool T_bZeroed=true,bool T_bBoundCheck=true> class TableAllocator
{
private:

  T_Type *m_Data;
  int     m_iSize;

public:

  TableAllocator()
  {
    m_Data  = NULL;
    m_iSize = 0;
  }

  TableAllocator(const TableAllocator& tbl)
  {
    if (tbl.m_Data != NULL) {
      m_Data  = tbl.clone();
      m_iSize = tbl.m_iSize;
    } else {
      m_Data  = NULL;
      m_iSize = 0;
    }
  }

  const TableAllocator& operator=(const TableAllocator& tbl)
  {
    if (tbl.m_Data != NULL) {
      if (m_Data != NULL && m_Data != tbl.m_Data) delete [](m_Data);
      m_Data  = tbl.clone();
      m_iSize = tbl.m_iSize;
    } else {
      if (m_Data != NULL) delete [](m_Data);
      m_Data  = NULL;
      m_iSize = 0;
    }
    return (*this);
  }

  TableAllocator(int size)
  {
    if (size <= 0)
      throw MemoryException("TableAllocator  --  allocation size is <= 0");
    m_Data  = NULL;
    allocate(size);
  }

  ~TableAllocator()
  {
    if (m_Data != NULL)
      delete [](m_Data);
  }

  void allocate(unsigned int size)
  {
    if (m_Data != NULL)
      throw MemoryException("TableAllocator  --  reallocation not permitted");
    m_Data   = new T_Type[size];
    m_iSize  = size;
    if (T_bZeroed)
      zero();
  }

  void reallocate(unsigned int size)
  {
    if (m_Data != NULL) {
      if (size != m_iSize) {
        delete [](m_Data);
        m_Data   = new T_Type[size];
      }
    } else
      m_Data   = new T_Type[size];
    m_iSize  = size;
    if (T_bZeroed)
      zero();
  }

  void zero()
  {
    memset(m_Data,0,sizeof(T_Type)*m_iSize);
  }

  void fill(T_Type v)
  {
    for (int i=0;i<m_iSize;i++) {
      m_Data[i]=v;
    }
  }

  T_Type *clone() const
  {
    if (m_Data == NULL)
      throw MemoryException("TableAllocator  --  cannot clone uninitialized table");
    T_Type *cloned = new T_Type[m_iSize];
    memcpy(cloned,m_Data,sizeof(T_Type)*m_iSize);
    return (cloned);
  }

  void checkAccess(int n) const
  {
    if (m_Data == NULL)
      throw MemoryException("TableAllocator  --  Access violation  --  null pointer");
    else if (n < 0 || n >= m_iSize)
      throw MemoryException("TableAllocator  --  Access violation  --  out of bounds");
  }

  T_Type&       operator [](int n)       {if (T_bBoundCheck) checkAccess(n); return (m_Data[n]);}
  const T_Type& operator [](int n) const {if (T_bBoundCheck) checkAccess(n); return (m_Data[n]);}
  
  // cast to pointer
  operator T_Type* ()      {if (T_bBoundCheck) checkAccess(0); return (m_Data);}

  int size() const {return (m_iSize);}

};

// --------------------------------------------------------------------------

#define ForTable(T,I) for (int I=0;I<int(T.size());I++)

// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
