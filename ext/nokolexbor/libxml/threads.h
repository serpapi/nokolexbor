/**
 * Summary: interfaces for thread handling
 * Description: set of generic threading related routines
 *              should work with pthreads, Windows native or TLS threads
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_THREADS_H__
#define __XML_THREADS_H__

#include "xmlversion.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * xmlMutex are a simple mutual exception locks.
 */
typedef struct _xmlMutex xmlMutex;
typedef xmlMutex *xmlMutexPtr;

/*
 * xmlRMutex are reentrant mutual exception locks.
 */
typedef struct _xmlRMutex xmlRMutex;
typedef xmlRMutex *xmlRMutexPtr;

#ifdef __cplusplus
}
#endif
#include "globals.h"
#ifdef __cplusplus
extern "C" {
#endif
XMLPUBFUN xmlMutexPtr XMLCALL
			nl_xmlNewMutex	(void);
XMLPUBFUN void XMLCALL
			nl_xmlMutexLock	(xmlMutexPtr tok);
XMLPUBFUN void XMLCALL
			nl_xmlMutexUnlock	(xmlMutexPtr tok);
XMLPUBFUN void XMLCALL
			nl_xmlFreeMutex	(xmlMutexPtr tok);

XMLPUBFUN xmlRMutexPtr XMLCALL
			nl_xmlNewRMutex	(void);
XMLPUBFUN void XMLCALL
			nl_xmlRMutexLock	(xmlRMutexPtr tok);
XMLPUBFUN void XMLCALL
			nl_xmlRMutexUnlock	(xmlRMutexPtr tok);
XMLPUBFUN void XMLCALL
			nl_xmlFreeRMutex	(xmlRMutexPtr tok);

/*
 * Library wide APIs.
 */
XML_DEPRECATED
XMLPUBFUN void XMLCALL
			nl_xmlInitThreads	(void);
XMLPUBFUN void XMLCALL
			nl_xmlLockLibrary	(void);
XMLPUBFUN void XMLCALL
			nl_xmlUnlockLibrary(void);
XML_DEPRECATED
XMLPUBFUN int XMLCALL
			nl_xmlGetThreadId	(void);
XML_DEPRECATED
XMLPUBFUN int XMLCALL
			nl_xmlIsMainThread	(void);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
			nl_xmlCleanupThreads(void);
XML_DEPRECATED
XMLPUBFUN xmlGlobalStatePtr XMLCALL
			nl_xmlGetGlobalState(void);

/** DOC_DISABLE */
#if defined(LIBXML_THREAD_ENABLED) && defined(_WIN32) && \
    !defined(HAVE_COMPILER_TLS) && defined(LIBXML_STATIC_FOR_DLL)
int XMLCALL
xmlDllMain(void *hinstDLL, unsigned long fdwReason,
           void *lpvReserved);
#endif
/** DOC_ENABLE */

#ifdef __cplusplus
}
#endif


#endif /* __XML_THREADS_H__ */
