/* ========================================================================
   $RCSfile: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

// You must define: 
// CONTAINER_NAME
// CONTAINER_ITEM_TYPE 
// CONTAINER_COMPARE_ITEMS(Item1, Item2)
// CONTAINER_FIND_FIELDS 
// CONTAINER_COMPARE_FIND_FIELDS(Item)

#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_FUNCTION_DECORATE(return_type) return_type
#define CONTAINER_LEFT_NAME  Left
#define CONTAINER_RIGHT_NAME Right
#define CONTAINER_ASSERT Assert
#define CONTAINER_DO_ALLOCATION 0
#define CONTAINER_USE_OVERLOADING 1
#define CONTAINER_EMIT_CODE 1
#include "contain.inl"
