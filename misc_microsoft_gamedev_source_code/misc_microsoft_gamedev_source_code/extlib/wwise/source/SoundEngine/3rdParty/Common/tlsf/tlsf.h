/*
** Two Level Segregated Fit memory allocator, version 1.7.
** Written by Matthew Conte, and placed in the Public Domain.
**	http://tlsf.baisoku.org
**
** Based on the original documentation by Miguel Masmano:
**	http://rtportal.upv.es/rtmalloc/allocators/tlsf/index.shtml
**
** Please see the accompanying Readme.txt for implementation
** notes and caveats.
**
** This implementation was written to the specification
** of the document, therefore no GPL restrictions apply.
*/

#pragma once

/* Create/destroy a memory pool. */
typedef void* tlsf_pool;
tlsf_pool tlsf_create(void* mem, size_t bytes);
void tlsf_destroy(tlsf_pool pool);

/* malloc/memalign/realloc/free replacements. */
void* tlsf_malloc(tlsf_pool pool, size_t bytes);
void* tlsf_memalign(tlsf_pool pool, size_t align, size_t bytes);
void* tlsf_realloc(tlsf_pool pool, void* ptr, size_t size);

void  tlsf_free(tlsf_pool pool, void* ptr);

/* Debugging. */
typedef void (*tlsf_walker)(void* ptr, size_t size, int used, void* user, char *fileName, int lineNumber);
void tlsf_walk_heap(tlsf_pool pool, tlsf_walker walker, void* user);

/* Returns internal block size, not original request size */
size_t tlsf_block_size(void* ptr);

/* Overhead of per-pool internal structures. */
size_t tlsf_overhead();
