/*
 * xmlmemory.c:  libxml memory allocator wrapper.
 *
 * daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

/* #define DEBUG_MEMORY */

/**
 * MEM_LIST:
 *
 * keep track of all allocated blocks for error reporting
 * Always build the memory list !
 */
#ifdef DEBUG_MEMORY_LOCATION
#ifndef MEM_LIST
#define MEM_LIST /* keep a list of all the allocated memory blocks */
#endif
#endif

#include "libxml/globals.h"	/* must come before xmlmemory.h */
#include "libxml/xmlmemory.h"
#include "libxml/xmlerror.h"
#include "libxml/threads.h"

#include "private/memory.h"
#include "private/threads.h"

static unsigned long  debugMemSize = 0;
static unsigned long  debugMemBlocks = 0;
static unsigned long  debugMaxMemSize = 0;
static xmlMutex xmlMemMutex;

void xmlMallocBreakpoint(void);

/************************************************************************
 *									*
 *		Macros, variables and associated types			*
 *									*
 ************************************************************************/

#if !defined(LIBXML_THREAD_ENABLED) && !defined(LIBXML_THREAD_ALLOC_ENABLED)
#ifdef xmlMalloc
#undef xmlMalloc
#endif
#ifdef xmlRealloc
#undef xmlRealloc
#endif
#ifdef xmlMemStrdup
#undef xmlMemStrdup
#endif
#endif

/*
 * Each of the blocks allocated begin with a header containing information
 */

#define MEMTAG 0x5aa5U

#define MALLOC_TYPE 1
#define REALLOC_TYPE 2
#define STRDUP_TYPE 3
#define MALLOC_ATOMIC_TYPE 4
#define REALLOC_ATOMIC_TYPE 5

typedef struct memnod {
    unsigned int   mh_tag;
    unsigned int   mh_type;
    unsigned long  mh_number;
    size_t         mh_size;
#ifdef MEM_LIST
   struct memnod *mh_next;
   struct memnod *mh_prev;
#endif
   const char    *mh_file;
   unsigned int   mh_line;
}  MEMHDR;


#ifdef SUN4
#define ALIGN_SIZE  16
#else
#define ALIGN_SIZE  sizeof(double)
#endif
#define HDR_SIZE    sizeof(MEMHDR)
#define RESERVE_SIZE (((HDR_SIZE + (ALIGN_SIZE-1)) \
		      / ALIGN_SIZE ) * ALIGN_SIZE)

#define MAX_SIZE_T ((size_t)-1)

#define CLIENT_2_HDR(a) ((void *) (((char *) (a)) - RESERVE_SIZE))
#define HDR_2_CLIENT(a)    ((void *) (((char *) (a)) + RESERVE_SIZE))


static unsigned int block=0;
static unsigned int xmlMemStopAtBlock = 0;
static void *xmlMemTraceBlockAt = NULL;
#ifdef MEM_LIST
static MEMHDR *memlist = NULL;
#endif

static void debugmem_tag_error(void *addr);
#ifdef MEM_LIST
static void  debugmem_list_add(MEMHDR *);
static void debugmem_list_delete(MEMHDR *);
#endif
#define Mem_Tag_Err(a) debugmem_tag_error(a);

#ifndef TEST_POINT
#define TEST_POINT
#endif

/**
 * xmlInitMemory:
 *
 * DEPRECATED: Alias for xmlInitParser.
 */
int
xmlInitMemory(void) {
    xmlInitParser();
    return(0);
}

/**
 * xmlInitMemoryInternal:
 *
 * Initialize the memory layer.
 *
 * Returns 0 on success
 */
void
xmlInitMemoryInternal(void) {
     char *breakpoint;
#ifdef DEBUG_MEMORY
     xmlGenericError(xmlGenericErrorContext,
	     "xmlInitMemory()\n");
#endif
     xmlInitMutex(&xmlMemMutex);

     breakpoint = getenv("XML_MEM_BREAKPOINT");
     if (breakpoint != NULL) {
         sscanf(breakpoint, "%ud", &xmlMemStopAtBlock);
     }
     breakpoint = getenv("XML_MEM_TRACE");
     if (breakpoint != NULL) {
         sscanf(breakpoint, "%p", &xmlMemTraceBlockAt);
     }

#ifdef DEBUG_MEMORY
     xmlGenericError(xmlGenericErrorContext,
	     "xmlInitMemory() Ok\n");
#endif
}
