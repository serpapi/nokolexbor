/*
 * Summary: string dictionary
 * Description: dictionary of reusable strings, just used to avoid allocation
 *         and freeing operations.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_DICT_H__
#define __XML_DICT_H__

#include <stddef.h>
#include "xmlversion.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The dictionary.
 */
typedef struct _xmlDict xmlDict;
typedef xmlDict *xmlDictPtr;

/*
 * Initializer
 */
XML_DEPRECATED
XMLPUBFUN int XMLCALL  nl_xmlInitializeDict(void);

/*
 * Constructor and destructor.
 */
XMLPUBFUN xmlDictPtr XMLCALL
			nl_xmlDictCreate	(void);
XMLPUBFUN size_t XMLCALL
			nl_xmlDictSetLimit	(xmlDictPtr dict,
                                         size_t limit);
XMLPUBFUN size_t XMLCALL
			nl_xmlDictGetUsage (xmlDictPtr dict);
XMLPUBFUN xmlDictPtr XMLCALL
			nl_xmlDictCreateSub(xmlDictPtr sub);
XMLPUBFUN int XMLCALL
			nl_xmlDictReference(xmlDictPtr dict);
XMLPUBFUN void XMLCALL
			nl_xmlDictFree	(xmlDictPtr dict);

/*
 * Lookup of entry in the dictionary.
 */
XMLPUBFUN const xmlChar * XMLCALL
			nl_xmlDictLookup	(xmlDictPtr dict,
		                         const xmlChar *name,
		                         int len);
XMLPUBFUN const xmlChar * XMLCALL
			nl_xmlDictExists	(xmlDictPtr dict,
		                         const xmlChar *name,
		                         int len);
XMLPUBFUN const xmlChar * XMLCALL
			nl_xmlDictQLookup	(xmlDictPtr dict,
		                         const xmlChar *prefix,
		                         const xmlChar *name);
XMLPUBFUN int XMLCALL
			nl_xmlDictOwns	(xmlDictPtr dict,
					 const xmlChar *str);
XMLPUBFUN int XMLCALL
			nl_xmlDictSize	(xmlDictPtr dict);

/*
 * Cleanup function
 */
XML_DEPRECATED
XMLPUBFUN void XMLCALL
                        nl_xmlDictCleanup  (void);

#ifdef __cplusplus
}
#endif
#endif /* ! __XML_DICT_H__ */
