/*
 * parser.c : an XML 1.0 parser, namespaces and validity support are mostly
 *            implemented on top of the SAX interfaces
 *
 * References:
 *   The XML specification:
 *     http://www.w3.org/TR/REC-xml
 *   Original 1.0 version:
 *     http://www.w3.org/TR/1998/REC-xml-19980210
 *   XML second edition working draft
 *     http://www.w3.org/TR/2000/WD-xml-2e-20000814
 *
 * Okay this is a big file, the parser core is around 7000 lines, then it
 * is followed by the progressive parser top routines, then the various
 * high level APIs to call the parser and a few miscellaneous functions.
 * A number of helper functions and deprecated ones have been moved to
 * parserInternals.c to reduce this file size.
 * As much as possible the functions are associated with their relative
 * production in the XML specification. A few productions defining the
 * different ranges of character are actually implanted either in
 * parserInternals.h or parserInternals.c
 * The DOM tree build is realized from the default SAX callbacks in
 * the module SAX.c.
 * The routines doing the validation checks are in valid.c and called either
 * from the SAX callbacks or as standalone functions using a preparsed
 * document.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

/* To avoid EBCDIC trouble when parsing on zOS */
#if defined(__MVS__)
#pragma convert("ISO8859-1")
#endif

#define IN_LIBXML
#include "libxml.h"

#if defined(_WIN32)
#define XML_DIR_SEP '\\'
#else
#define XML_DIR_SEP '/'
#endif

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include "libxml/xmlmemory.h"
#include "libxml/threads.h"
#include "libxml/globals.h"
#include "libxml/tree.h"
#include "libxml/parser.h"
#include "libxml/parserInternals.h"
#include "libxml/HTMLparser.h"
#include "libxml/valid.h"
#include "libxml/entities.h"
#include "libxml/xmlerror.h"
#include "libxml/encoding.h"
#include "libxml/xmlIO.h"
#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
#include "libxml/xpath.h"
#endif

#include "private/threads.h"
#include "private/enc.h"
#include "private/xpath.h"
#include "private/dict.h"
#include "private/memory.h"
#include "private/globals.h"

/************************************************************************
 *									*
 *				Miscellaneous				*
 *									*
 ************************************************************************/

static int xmlParserInitialized = 0;

/**
 * nl_xmlInitParser:
 *
 * Initialization function for the XML parser.
 * This is not reentrant. Call once before processing in case of
 * use in multithreaded programs.
 */

void
nl_xmlInitParser(void) {
    /*
     * Note that the initialization code must not make memory allocations.
     */
    if (xmlParserInitialized != 0)
	return;

#ifdef LIBXML_THREAD_ENABLED
    __xmlGlobalInitMutexLock();
    if (xmlParserInitialized == 0) {
#endif
#if defined(_WIN32) && (!defined(LIBXML_STATIC) || defined(LIBXML_STATIC_FOR_DLL))
        if (nl_xmlFree == free)
            atexit(xmlCleanupParser);
#endif

	xmlInitThreadsInternal();
	xmlInitGlobalsInternal();
	xmlInitMemoryInternal();
        __xmlInitializeDict();
	xmlInitEncodingInternal();
// 	xmlRegisterDefaultInputCallbacks();
// #ifdef LIBXML_OUTPUT_ENABLED
// 	xmlRegisterDefaultOutputCallbacks();
// #endif /* LIBXML_OUTPUT_ENABLED */
#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
	xmlInitXPathInternal();
#endif
	xmlParserInitialized = 1;
#ifdef LIBXML_THREAD_ENABLED
    }
    __xmlGlobalInitMutexUnlock();
#endif
}