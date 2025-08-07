/* cimem.c - stub functions for memory allocation
 *
 * Copyright (c) 2018-2025 Cosmin Truta
 * Copyright (c) 1998-2002,2004,2006-2014,2016 Glenn Randers-Pehrson
 * Copyright (c) 1996-1997 Andreas Dilger
 * Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * This code is released under the libci license.
 * For conditions of distribution and use, see the disclaimer
 * and license in ci.h
 *
 * This file provides a location for all memory allocation.  Users who
 * need special memory handling are expected to supply replacement
 * functions for ci_malloc() and ci_free(), and to use
 * ci_create_read_struct_2() and ci_create_write_struct_2() to
 * identify the replacement functions.
 */

#include "cipriv.h"

#if defined(CI_READ_SUPPORTED) || defined(CI_WRITE_SUPPORTED)
/* Free a ci_struct */
void /* PRIVATE */
ci_destroy_ci_struct(ci_structrp ci_ptr)
{
   if (ci_ptr != NULL)
   {
      /* ci_free might call ci_error and may certainly call
       * ci_get_mem_ptr, so fake a temporary ci_struct to support this.
       */
      ci_struct dummy_struct = *ci_ptr;
      memset(ci_ptr, 0, (sizeof *ci_ptr));
      ci_free(&dummy_struct, ci_ptr);

#     ifdef CI_SETJMP_SUPPORTED
         /* We may have a jmp_buf left to deallocate. */
         ci_free_jmpbuf(&dummy_struct);
#     endif
   }
}

/* Allocate memory.  For reasonable files, size should never exceed
 * 64K.  However, zlib may allocate more than 64K if you don't tell
 * it not to.  See zconf.h and ci.h for more information.  zlib does
 * need to allocate exactly 64K, so whatever you call here must
 * have the ability to do that.
 */
CI_FUNCTION(ci_voidp,CIAPI
ci_calloc,(ci_const_structrp ci_ptr, ci_alloc_size_t size),CI_ALLOCATED)
{
   ci_voidp ret;

   ret = ci_malloc(ci_ptr, size);

   if (ret != NULL)
      memset(ret, 0, size);

   return ret;
}

/* ci_malloc_base, an internal function added at libci 1.6.0, does the work of
 * allocating memory, taking into account limits and CI_USER_MEM_SUPPORTED.
 * Checking and error handling must happen outside this routine; it returns NULL
 * if the allocation cannot be done (for any reason.)
 */
CI_FUNCTION(ci_voidp /* PRIVATE */,
ci_malloc_base,(ci_const_structrp ci_ptr, ci_alloc_size_t size),
    CI_ALLOCATED)
{
   /* Moved to ci_malloc_base from ci_malloc_default in 1.6.0; the DOS
    * allocators have also been removed in 1.6.0, so any 16-bit system now has
    * to implement a user memory handler.  This checks to be sure it isn't
    * called with big numbers.
    */
#  ifdef CI_MAX_MALLOC_64K
      /* This is support for legacy systems which had segmented addressing
       * limiting the maximum allocation size to 65536.  It takes precedence
       * over CI_SIZE_MAX which is set to 65535 on true 16-bit systems.
       *
       * TODO: libci-1.8: finally remove both cases.
       */
      if (size > 65536U) return NULL;
#  endif

   /* This is checked too because the system malloc call below takes a (size_t).
    */
   if (size > CI_SIZE_MAX) return NULL;

#  ifdef CI_USER_MEM_SUPPORTED
      if (ci_ptr != NULL && ci_ptr->malloc_fn != NULL)
         return ci_ptr->malloc_fn(ci_constcast(ci_structrp,ci_ptr), size);
#  else
      CI_UNUSED(ci_ptr)
#  endif

   /* Use the system malloc */
   return malloc((size_t)/*SAFE*/size); /* checked for truncation above */
}

#if defined(CI_TEXT_SUPPORTED) || defined(CI_sPLT_SUPPORTED) ||\
   defined(CI_STORE_UNKNOWN_CHUNKS_SUPPORTED)
/* This is really here only to work round a spurious warning in GCC 4.6 and 4.7
 * that arises because of the checks in ci_realloc_array that are repeated in
 * ci_malloc_array.
 */
static ci_voidp
ci_malloc_array_checked(ci_const_structrp ci_ptr, int nelements,
    size_t element_size)
{
   ci_alloc_size_t req = (ci_alloc_size_t)nelements; /* known to be > 0 */

   if (req <= CI_SIZE_MAX/element_size)
      return ci_malloc_base(ci_ptr, req * element_size);

   /* The failure case when the request is too large */
   return NULL;
}

CI_FUNCTION(ci_voidp /* PRIVATE */,
ci_malloc_array,(ci_const_structrp ci_ptr, int nelements,
    size_t element_size),CI_ALLOCATED)
{
   if (nelements <= 0 || element_size == 0)
      ci_error(ci_ptr, "internal error: array alloc");

   return ci_malloc_array_checked(ci_ptr, nelements, element_size);
}

CI_FUNCTION(ci_voidp /* PRIVATE */,
ci_realloc_array,(ci_const_structrp ci_ptr, ci_const_voidp old_array,
    int old_elements, int add_elements, size_t element_size),CI_ALLOCATED)
{
   /* These are internal errors: */
   if (add_elements <= 0 || element_size == 0 || old_elements < 0 ||
      (old_array == NULL && old_elements > 0))
      ci_error(ci_ptr, "internal error: array realloc");

   /* Check for overflow on the elements count (so the caller does not have to
    * check.)
    */
   if (add_elements <= INT_MAX - old_elements)
   {
      ci_voidp new_array = ci_malloc_array_checked(ci_ptr,
          old_elements+add_elements, element_size);

      if (new_array != NULL)
      {
         /* Because ci_malloc_array worked the size calculations below cannot
          * overflow.
          */
         if (old_elements > 0)
            memcpy(new_array, old_array, element_size*(unsigned)old_elements);

         memset((char*)new_array + element_size*(unsigned)old_elements, 0,
             element_size*(unsigned)add_elements);

         return new_array;
      }
   }

   return NULL; /* error */
}
#endif /* TEXT || sPLT || STORE_UNKNOWN_CHUNKS */

/* Various functions that have different error handling are derived from this.
 * ci_malloc always exists, but if CI_USER_MEM_SUPPORTED is defined a separate
 * function ci_malloc_default is also provided.
 */
CI_FUNCTION(ci_voidp,CIAPI
ci_malloc,(ci_const_structrp ci_ptr, ci_alloc_size_t size),CI_ALLOCATED)
{
   ci_voidp ret;

   if (ci_ptr == NULL)
      return NULL;

   ret = ci_malloc_base(ci_ptr, size);

   if (ret == NULL)
       ci_error(ci_ptr, "Out of memory"); /* 'm' means ci_malloc */

   return ret;
}

#ifdef CI_USER_MEM_SUPPORTED
CI_FUNCTION(ci_voidp,CIAPI
ci_malloc_default,(ci_const_structrp ci_ptr, ci_alloc_size_t size),
    CI_ALLOCATED CI_DEPRECATED)
{
   ci_voidp ret;

   if (ci_ptr == NULL)
      return NULL;

   /* Passing 'NULL' here bypasses the application provided memory handler. */
   ret = ci_malloc_base(NULL/*use malloc*/, size);

   if (ret == NULL)
      ci_error(ci_ptr, "Out of Memory"); /* 'M' means ci_malloc_default */

   return ret;
}
#endif /* USER_MEM */

/* This function was added at libci version 1.2.3.  The ci_malloc_warn()
 * function will issue a ci_warning and return NULL instead of issuing a
 * ci_error, if it fails to allocate the requested memory.
 */
CI_FUNCTION(ci_voidp,CIAPI
ci_malloc_warn,(ci_const_structrp ci_ptr, ci_alloc_size_t size),
    CI_ALLOCATED)
{
   if (ci_ptr != NULL)
   {
      ci_voidp ret = ci_malloc_base(ci_ptr, size);

      if (ret != NULL)
         return ret;

      ci_warning(ci_ptr, "Out of memory");
   }

   return NULL;
}

/* Free a pointer allocated by ci_malloc().  If ptr is NULL, return
 * without taking any action.
 */
void CIAPI
ci_free(ci_const_structrp ci_ptr, ci_voidp ptr)
{
   if (ci_ptr == NULL || ptr == NULL)
      return;

#ifdef CI_USER_MEM_SUPPORTED
   if (ci_ptr->free_fn != NULL)
      ci_ptr->free_fn(ci_constcast(ci_structrp,ci_ptr), ptr);

   else
      ci_free_default(ci_ptr, ptr);
}

CI_FUNCTION(void,CIAPI
ci_free_default,(ci_const_structrp ci_ptr, ci_voidp ptr),CI_DEPRECATED)
{
   if (ci_ptr == NULL || ptr == NULL)
      return;
#endif /* USER_MEM */

   free(ptr);
}

#ifdef CI_USER_MEM_SUPPORTED
/* This function is called when the application wants to use another method
 * of allocating and freeing memory.
 */
void CIAPI
ci_set_mem_fn(ci_structrp ci_ptr, ci_voidp mem_ptr, ci_malloc_ptr
  malloc_fn, ci_free_ptr free_fn)
{
   if (ci_ptr != NULL)
   {
      ci_ptr->mem_ptr = mem_ptr;
      ci_ptr->malloc_fn = malloc_fn;
      ci_ptr->free_fn = free_fn;
   }
}

/* This function returns a pointer to the mem_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before ci_write_destroy and ci_read_destroy are called.
 */
ci_voidp CIAPI
ci_get_mem_ptr(ci_const_structrp ci_ptr)
{
   if (ci_ptr == NULL)
      return NULL;

   return ci_ptr->mem_ptr;
}
#endif /* USER_MEM */
#endif /* READ || WRITE */
