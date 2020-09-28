//============================================================================
// File: renderThread.inl
//============================================================================

//============================================================================
// BRenderThread::submitCommand
//============================================================================
inline void BRenderThread::submitCommand(BRenderCommandListener& listener, DWORD commandType, DWORD dataLen, const void* pData)
{
   submitCommand(listener.getCommandHandle(), commandType, dataLen, pData);
}

//============================================================================
// BRenderThread::submitCommand
//============================================================================
inline void BRenderThread::submitCommand(BRenderCommandListener& listener, DWORD commandType)
{
   submitCommand(listener.getCommandHandle(), commandType);
}

//============================================================================
// BRenderThread::submitCommandBegin
//============================================================================
inline void* BRenderThread::submitCommandBegin(BRenderCommandListener& listener, DWORD commandType, DWORD dataLen)
{
   return submitCommandBegin(listener.getCommandHandle(), commandType, dataLen);
}

//============================================================================
// BRenderThread::submitCopyOfCommandObject
//============================================================================
template<typename T>
inline void BRenderThread::submitCopyOfCommandObject(const T& obj, DWORD data)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(sizeof(T) >= sizeof(BRenderCommandObjectInterface));

   void* pDst = submitCommandBegin(cRCCCommandObjCopy, data, sizeof(T));

   Utils::ConstructInPlace<T>(reinterpret_cast<T*>(pDst), obj);

   submitCommandEnd(sizeof(T));
}      

//============================================================================
// BRenderThread::submitPtrToCommandObject
//============================================================================
template<typename T>
inline void BRenderThread::submitPtrToCommandObject(const T& obj, DWORD data)
{
   ASSERT_MAIN_THREAD

   const T* pObj = &obj;
   submitCommand(cRCCCommandObjPtr, data, sizeof(const T*), &pObj);
}      

//============================================================================
// BRenderThread::allocateFrameStorageObj
//============================================================================
template<class T, bool construct> 
inline T* BRenderThread::allocateFrameStorageObj(uint num, const T& obj, uint alignment)
{
   T* pObj = (T*)allocateFrameStorage(sizeof(T) * num, alignment);
   if (!pObj)
      return NULL;

   if (construct)
      for (uint i = 0; i < num; i++)
         Utils::ConstructInPlace(&pObj[i], obj);

   return pObj;
}

//============================================================================
// BRenderThread::workerAllocateFrameStorageObj
//============================================================================
template<class T, bool construct> 
inline T* BRenderThread::workerAllocateFrameStorageObj(uint num, const T& obj, uint alignment)
{
   T* pObj = (T*)workerAllocateFrameStorage(sizeof(T) * num, alignment);
   if (!pObj)
      return NULL;

   if (construct)
      for (uint i = 0; i < num; i++)
         Utils::ConstructInPlace(&pObj[i], obj);

   return pObj;
}

//============================================================================
// BRenderThread::deferredAlignedDelete
//============================================================================
template<class T> 
inline void BRenderThread::deferredAlignedDelete(T* p)
{
   if (p)
   {
      Utils::DestructInPlace(p);
      deferredAlignedFree(p, true);
   }
}

//============================================================================
// BRenderThread::submitFunctorWithObject
//============================================================================
template<typename T>
inline void BRenderThread::submitFunctorWithObject(const BFunctor& functor, const T& obj, uint alignment)
{
   submitFunctor(functor, allocateFrameStorageObj<T, true>(1, obj, alignment));
}




