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
    __nl_xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}

/**
 * nl_xmlBuildQName:
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
nl_xmlBuildQName(const xmlChar *ncname, const xmlChar *prefix,
	      xmlChar *memory, int len) {
    int lenn, lenp;
    xmlChar *ret;

    if (ncname == NULL) return(NULL);
    if (prefix == NULL) return((xmlChar *) ncname);

    lenn = strlen((char *) ncname);
    lenp = strlen((char *) prefix);

    if ((memory == NULL) || (len < lenn + lenp + 2)) {
	ret = (xmlChar *) nl_xmlMallocAtomic(lenn + lenp + 2);
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
 * nl_xmlNodeGetContent:
 * @cur:  the node being read
 *
 * Read the value of a node, this can be either the text carried
 * directly by this node if it's a TEXT node or the aggregate string
 * of the values carried by this node child's (TEXT and ENTITY_REF).
 * Entity references are substituted.
 * Returns a new #xmlChar * or NULL if no content is available.
 *     It's up to the caller to free the memory with nl_xmlFree().
 */
xmlChar *
nl_xmlNodeGetContent(const lxb_dom_node_t *cur)
{
    size_t len;
    lxb_char_t *content = lxb_dom_node_text_content(cur, &len);
    if (content == NULL)
        return NULL;
    xmlChar *content_dup = nl_xmlStrndup(content, len);
    lxb_dom_document_destroy_text(cur->owner_document, content);
    return content_dup;
}

/**
 * nl_xmlDocGetRootElement:
 * @doc:  the document
 *
 * Get the root element of the document (doc->children is a list
 * containing possibly comments, PIs, etc ...).
 *
 * Returns the #lxb_dom_node_t_ptr for the root or NULL
 */
lxb_dom_node_t_ptr
nl_xmlDocGetRootElement(const lxb_dom_document_t *doc) {
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

/**
 * nl_xmlFreeNodeList:
 * @cur:  the first node in the list
 *
 * Free a node and all its siblings, this is a recursive behaviour, all
 * the children are freed too.
 */
void
nl_xmlFreeNodeList(lxb_dom_node_t_ptr cur) {
    // Should never be called
}

/**
 * xmlGetNodePath:
 * @node: a node
 *
 * Build a structure based Path for the given node
 *
 * Returns the new path or NULL in case of error. The caller must free
 *     the returned string
 */
xmlChar *
nl_xmlGetNodePath(const lxb_dom_node_t *node)
{
    const lxb_dom_node_t *cur, *tmp, *next;
    xmlChar *buffer = NULL, *temp;
    size_t buf_len;
    xmlChar *buf;
    const char *sep;
    const char *name;
    char nametemp[100];
    int occur = 0, generic;

    if ((node == NULL) || (node->type == XML_NAMESPACE_DECL))
        return (NULL);

    buf_len = 500;
    buffer = (xmlChar *) nl_xmlMallocAtomic(buf_len);
    if (buffer == NULL) {
	xmlTreeErrMemory("getting node path");
        return (NULL);
    }
    buf = (xmlChar *) nl_xmlMallocAtomic(buf_len);
    if (buf == NULL) {
	xmlTreeErrMemory("getting node path");
        nl_xmlFree(buffer);
        return (NULL);
    }

    buffer[0] = 0;
    cur = node;
    do {
        name = "";
        sep = "?";
        occur = 0;
        const lxb_char_t* cur_name = NODE_NAME(cur);
        const lxb_char_t* cur_ns_prefix = NODE_NS_PREFIX(cur);
        if ((cur->type == LXB_DOM_NODE_TYPE_DOCUMENT) ||
            (cur->type == XML_HTML_DOCUMENT_NODE)) {
            if (buffer[0] == '/')
                break;
            sep = "/";
            next = NULL;
        } else if (cur->type == LXB_DOM_NODE_TYPE_ELEMENT) {
	    generic = 0;
            sep = "/";
            name = (const char *) cur_name;
            next = cur->parent;

            /*
             * Thumbler index computation
	     * TODO: the occurrence test seems bogus for namespaced names
             */
            tmp = cur->prev;
            while (tmp != NULL) {
                if ((tmp->type == LXB_DOM_NODE_TYPE_ELEMENT) &&
		    (generic ||
		     (nl_xmlStrEqual(cur_name, NODE_NAME(tmp)) &&
		     ((tmp->ns == cur->ns) ||
		      ((tmp->ns != NULL) && (cur->ns != NULL) &&
		       (nl_xmlStrEqual(cur_ns_prefix, NODE_NS_PREFIX(tmp))))))))
                    occur++;
                tmp = tmp->prev;
            }
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL && occur == 0) {
                    if ((tmp->type == LXB_DOM_NODE_TYPE_ELEMENT) &&
			(generic ||
			 (nl_xmlStrEqual(cur_name, NODE_NAME(tmp)) &&
			 ((tmp->ns == cur->ns) ||
			  ((tmp->ns != NULL) && (cur->ns != NULL) &&
			   (nl_xmlStrEqual(cur_ns_prefix, NODE_NS_PREFIX(tmp))))))))
                        occur++;
                    tmp = tmp->next;
                }
                if (occur != 0)
                    occur = 1;
            } else
                occur++;
        } else if (cur->type == LXB_DOM_NODE_TYPE_COMMENT) {
            sep = "/";
	    name = "comment()";
            next = cur->parent;

            /*
             * Thumbler index computation
             */
            tmp = cur->prev;
            while (tmp != NULL) {
                if (tmp->type == LXB_DOM_NODE_TYPE_COMMENT)
		    occur++;
                tmp = tmp->prev;
            }
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL && occur == 0) {
		    if (tmp->type == LXB_DOM_NODE_TYPE_COMMENT)
		        occur++;
                    tmp = tmp->next;
                }
                if (occur != 0)
                    occur = 1;
            } else
                occur++;
        } else if ((cur->type == LXB_DOM_NODE_TYPE_TEXT) ||
                   (cur->type == LXB_DOM_NODE_TYPE_CDATA_SECTION)) {
            sep = "/";
	    name = "text()";
            next = cur->parent;

            /*
             * Thumbler index computation
             */
            tmp = cur->prev;
            while (tmp != NULL) {
                if ((tmp->type == LXB_DOM_NODE_TYPE_TEXT) ||
		    (tmp->type == LXB_DOM_NODE_TYPE_CDATA_SECTION))
		    occur++;
                tmp = tmp->prev;
            }
	    /*
	    * Evaluate if this is the only text- or CDATA-section-node;
	    * if yes, then we'll get "text()", otherwise "text()[1]".
	    */
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL) {
		    if ((tmp->type == LXB_DOM_NODE_TYPE_TEXT) ||
			(tmp->type == LXB_DOM_NODE_TYPE_CDATA_SECTION))
		    {
			occur = 1;
			break;
		    }
		    tmp = tmp->next;
		}
            } else
                occur++;
        } else if (cur->type == LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION) {
            sep = "/";
	    snprintf(nametemp, sizeof(nametemp) - 1,
		     "processing-instruction('%s')", (char *)cur_name);
            nametemp[sizeof(nametemp) - 1] = 0;
            name = nametemp;

	    next = cur->parent;

            /*
             * Thumbler index computation
             */
            tmp = cur->prev;
            while (tmp != NULL) {
                if ((tmp->type == LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION) &&
		    (nl_xmlStrEqual(cur_name, NODE_NAME(tmp))))
                    occur++;
                tmp = tmp->prev;
            }
            if (occur == 0) {
                tmp = cur->next;
                while (tmp != NULL && occur == 0) {
                    if ((tmp->type == LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION) &&
			(nl_xmlStrEqual(cur_name, NODE_NAME(tmp))))
                        occur++;
                    tmp = tmp->next;
                }
                if (occur != 0)
                    occur = 1;
            } else
                occur++;

        } else if (cur->type == LXB_DOM_NODE_TYPE_ATTRIBUTE) {
            sep = "/@";
            name = (const char *) lxb_dom_attr_qualified_name(cur, &tmp_len);
            next = ((lxb_dom_attr_t_ptr)cur)->owner;
        } else {
            nl_xmlFree(buf);
            nl_xmlFree(buffer);
            return (NULL);
        }

        /*
         * Make sure there is enough room
         */
        if (nl_xmlStrlen(buffer) + sizeof(nametemp) + 20 > buf_len) {
            buf_len =
                2 * buf_len + nl_xmlStrlen(buffer) + sizeof(nametemp) + 20;
            temp = (xmlChar *) nl_xmlRealloc(buffer, buf_len);
            if (temp == NULL) {
		xmlTreeErrMemory("getting node path");
                nl_xmlFree(buf);
                nl_xmlFree(buffer);
                return (NULL);
            }
            buffer = temp;
            temp = (xmlChar *) nl_xmlRealloc(buf, buf_len);
            if (temp == NULL) {
		xmlTreeErrMemory("getting node path");
                nl_xmlFree(buf);
                nl_xmlFree(buffer);
                return (NULL);
            }
            buf = temp;
        }
        if (occur == 0)
            snprintf((char *) buf, buf_len, "%s%s%s",
                     sep, name, (char *) buffer);
        else
            snprintf((char *) buf, buf_len, "%s%s[%d]%s",
                     sep, name, occur, (char *) buffer);
        snprintf((char *) buffer, buf_len, "%s", (char *)buf);
        cur = next;
    } while (cur != NULL);
    nl_xmlFree(buf);
    return (buffer);
}