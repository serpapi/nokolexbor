/*
 * Summary: pattern expression handling
 * Description: allows to compile and test pattern expressions for nodes
 *              either in a tree or based on a parser state.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_PATTERN_H__
#define __XML_PATTERN_H__

#include "xmlversion.h"
#include "tree.h"
#include "dict.h"

#ifdef LIBXML_PATTERN_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlPattern:
 *
 * A compiled (XPath based) pattern to select nodes
 */
typedef struct _xmlPattern xmlPattern;
typedef xmlPattern *xmlPatternPtr;

/**
 * xmlPatternFlags:
 *
 * This is the set of options affecting the behaviour of pattern
 * matching with this module
 *
 */
typedef enum {
    XML_PATTERN_DEFAULT		= 0,	/* simple pattern match */
    XML_PATTERN_XPATH		= 1<<0,	/* standard XPath pattern */
    XML_PATTERN_XSSEL		= 1<<1,	/* XPath subset for schema selector */
    XML_PATTERN_XSFIELD		= 1<<2	/* XPath subset for schema field */
} xmlPatternFlags;

XMLPUBFUN void XMLCALL
			nl_xmlFreePattern		(xmlPatternPtr comp);

XMLPUBFUN void XMLCALL
			nl_xmlFreePatternList	(xmlPatternPtr comp);

XMLPUBFUN xmlPatternPtr XMLCALL
			nl_xmlPatterncompile	(const xmlChar *pattern,
						 xmlDict *dict,
						 int flags,
						 const xmlChar **namespaces);
XMLPUBFUN int XMLCALL
			xmlPatternMatch		(xmlPatternPtr comp,
						 lxb_dom_node_t_ptr node);

/* streaming interfaces */
typedef struct _xmlStreamCtxt xmlStreamCtxt;
typedef xmlStreamCtxt *xmlStreamCtxtPtr;

XMLPUBFUN int XMLCALL
			nl_xmlPatternStreamable	(xmlPatternPtr comp);
XMLPUBFUN int XMLCALL
			nl_xmlPatternMaxDepth	(xmlPatternPtr comp);
XMLPUBFUN int XMLCALL
			nl_xmlPatternMinDepth	(xmlPatternPtr comp);
XMLPUBFUN int XMLCALL
			nl_xmlPatternFromRoot	(xmlPatternPtr comp);
XMLPUBFUN xmlStreamCtxtPtr XMLCALL
			nl_xmlPatternGetStreamCtxt	(xmlPatternPtr comp);
XMLPUBFUN void XMLCALL
			nl_xmlFreeStreamCtxt	(xmlStreamCtxtPtr stream);
XMLPUBFUN int XMLCALL
			nl_xmlStreamPushNode	(xmlStreamCtxtPtr stream,
						 const xmlChar *name,
						 const xmlChar *ns,
						 int nodeType);
XMLPUBFUN int XMLCALL
			nl_xmlStreamPush		(xmlStreamCtxtPtr stream,
						 const xmlChar *name,
						 const xmlChar *ns);
XMLPUBFUN int XMLCALL
			nl_xmlStreamPushAttr	(xmlStreamCtxtPtr stream,
						 const xmlChar *name,
						 const xmlChar *ns);
XMLPUBFUN int XMLCALL
			nl_xmlStreamPop		(xmlStreamCtxtPtr stream);
XMLPUBFUN int XMLCALL
			nl_xmlStreamWantsAnyNode	(xmlStreamCtxtPtr stream);
#ifdef __cplusplus
}
#endif

#endif /* LIBXML_PATTERN_ENABLED */

#endif /* __XML_PATTERN_H__ */
