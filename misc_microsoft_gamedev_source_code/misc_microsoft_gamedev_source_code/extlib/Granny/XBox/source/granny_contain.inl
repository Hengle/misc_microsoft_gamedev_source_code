#if !defined(CONTAINER_LEFT_NAME)
#define CONTAINER_LEFT_NAME Left
#endif

#if !defined(CONTAINER_RIGHT_NAME)
#define CONTAINER_RIGHT_NAME Right
#endif

#if !defined(CONTAINER_NEXT_NAME)
#define CONTAINER_NEXT_NAME Next
#endif

#if !defined(CONTAINER_PREV_NAME)
#define CONTAINER_PREV_NAME Previous
#endif

#define CONTAINER_FUNCTION_DECORATE(return_type) return_type
#define CONTAINER_ASSERT Assert
#define CONTAINER_USE_OVERLOADING 1
#define CONTAINER_EMIT_CODE 1
#define CONTAINER_MALLOC(Size) AllocateSize(Size)
#define CONTAINER_FREE(Pointer) Deallocate(Pointer)
#include "contain.inl"
