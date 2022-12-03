/*
 * tree.c : implementation of access function for an XML tree.
 *
 * References:
 *   XHTML 1.0 W3C REC: http://www.w3.org/TR/2002/REC-xhtml1-20020801/
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 *
 */

/* To avoid EBCDIC trouble when parsing on zOS */
#if defined(__MVS__)
#pragma convert("ISO8859-1")
#endif

#define IN_LIBXML
#include "libxml.h"

#include <string.h> /* for memset() only ! */
#include <stddef.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef LIBXML_ZLIB_ENABLED
#include <zlib.h>
#endif

#include "libxml/xmlmemory.h"
#include "libxml/tree.h"
#include "libxml/parser.h"
#include "libxml/uri.h"
#include "libxml/entities.h"
#include "libxml/valid.h"
#include "libxml/xmlerror.h"
#include "libxml/parserInternals.h"
#include "libxml/globals.h"
#ifdef LIBXML_DEBUG_ENABLED
#include "libxml/debugXML.h"
#endif

#include "private/buf.h"
#include "private/error.h"
#include "private/tree.h"

int __xmlRegisterCallbacks = 0;

/**
 * xmlTreeErrMemory:
 * @extra:  extra information
 *
 * Handle an out of memory condition
 */
static void
xmlTreeErrMemory(const char *extra)
{
    __xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}

/**
 * xmlBuildQName:
 * @ncname:  the Name
 * @prefix:  the prefix
 * @memory:  preallocated memory
 * @len:  preallocated memory length
 *
 * Builds the QName @prefix:@ncname in @memory if there is enough space
 * and prefix is not NULL nor empty, otherwise allocate a new string.
 * If prefix is NULL or empty it returns ncname.
 *
 * Returns the new string which must be freed by the caller if different from
 *         @memory and @ncname or NULL in case of error
 */
xmlChar *
xmlBuildQName(const xmlChar *ncname, const xmlChar *prefix,
	      xmlChar *memory, int len) {
    int lenn, lenp;
    xmlChar *ret;

    if (ncname == NULL) return(NULL);
    if (prefix == NULL) return((xmlChar *) ncname);

    lenn = strlen((char *) ncname);
    lenp = strlen((char *) prefix);

    if ((memory == NULL) || (len < lenn + lenp + 2)) {
	ret = (xmlChar *) xmlMallocAtomic(lenn + lenp + 2);
	if (ret == NULL) {
	    xmlTreeErrMemory("building QName");
	    return(NULL);
	}
    } else {
	ret = memory;
    }
    memcpy(&ret[0], prefix, lenp);
    ret[lenp] = ':';
    memcpy(&ret[lenp + 1], ncname, lenn);
    ret[lenn + lenp + 1] = 0;
    return(ret);
}

/**
 * xmlNodeGetContent:
 * @cur:  the node being read
 *
 * Read the value of a node, this can be either the text carried
 * directly by this node if it's a TEXT node or the aggregate string
 * of the values carried by this node child's (TEXT and ENTITY_REF).
 * Entity references are substituted.
 * Returns a new #xmlChar * or NULL if no content is available.
 *     It's up to the caller to free the memory with xmlFree().
 */
xmlChar *
xmlNodeGetContent(const lxb_dom_node_t *cur)
{
    size_t tmp_len;
    return lxb_dom_node_text_content(cur, &tmp_len);
}

/**
 * xmlDocGetRootElement:
 * @doc:  the document
 *
 * Get the root element of the document (doc->children is a list
 * containing possibly comments, PIs, etc ...).
 *
 * Returns the #xmlNodePtr for the root or NULL
 */
lxb_dom_node_t_ptr
xmlDocGetRootElement(const lxb_dom_document_t *doc) {
    lxb_dom_node_t_ptr ret;

    if (doc == NULL) return(NULL);
    ret = doc->node.first_child;
    while (ret != NULL) {
	if (ret->type == LXB_DOM_NODE_TYPE_ELEMENT)
	    return(ret);
        ret = ret->next;
    }
    return(ret);
}