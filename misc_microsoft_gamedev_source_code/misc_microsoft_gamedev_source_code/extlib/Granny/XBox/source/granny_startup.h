#if !defined(GRANNY_STARTUP_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_startup.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

bool Startup(void);
void Shutdown(void);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STARTUP_H
#endif
