/*
 * Summary: API to handle XML Pointers
 * Description: API to handle XML Pointers
 * Base implementation was made accordingly to
 * W3C Candidate Recommendation 7 June 2000
 * http://www.w3.org/TR/2000/CR-xptr-20000607
 *
 * Added support for the element() scheme described in:
 * W3C Proposed Recommendation 13 November 2002
 * http://www.w3.org/TR/2002/PR-xptr-element-20021113/
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XPTR_H__
#define __XML_XPTR_H__

#include "xmlversion.h"

#ifdef LIBXML_XPTR_ENABLED

#include "tree.h"
#include "xpath.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBXML_XPTR_LOCS_ENABLED
/*
 * A Location Set
 */
typedef struct _xmlLocationSet xmlLocationSet;
typedef xmlLocationSet *xmlLocationSetPtr;
struct _xmlLocationSet {
    int locNr;		      /* number of locations in the set */
    int locMax;		      /* size of the array as allocated */
    xmlXPathObjectPtr *locTab;/* array of locations */
};

/*
 * Handling of location sets.
 */

XML_DEPRECATED
XMLPUBFUN xmlLocationSetPtr XMLCALL
		    xmlXPtrLocationSetCreate	(xmlXPathObjectPtr val);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrFreeLocationSet	(xmlLocationSetPtr obj);
XML_DEPRECATED
XMLPUBFUN xmlLocationSetPtr XMLCALL
		    xmlXPtrLocationSetMerge	(xmlLocationSetPtr val1,
						 xmlLocationSetPtr val2);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRange		(lxb_dom_node_t_ptr start,
						 int startindex,
						 lxb_dom_node_t_ptr end,
						 int endindex);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRangePoints	(xmlXPathObjectPtr start,
						 xmlXPathObjectPtr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRangeNodePoint	(lxb_dom_node_t_ptr start,
						 xmlXPathObjectPtr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRangePointNode	(xmlXPathObjectPtr start,
						 lxb_dom_node_t_ptr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRangeNodes	(lxb_dom_node_t_ptr start,
						 lxb_dom_node_t_ptr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewLocationSetNodes	(lxb_dom_node_t_ptr start,
						 lxb_dom_node_t_ptr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewLocationSetNodeSet(xmlNodeSetPtr set);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewRangeNodeObject	(lxb_dom_node_t_ptr start,
						 xmlXPathObjectPtr end);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrNewCollapsedRange	(lxb_dom_node_t_ptr start);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrLocationSetAdd	(xmlLocationSetPtr cur,
						 xmlXPathObjectPtr val);
XML_DEPRECATED
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrWrapLocationSet	(xmlLocationSetPtr val);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrLocationSetDel	(xmlLocationSetPtr cur,
						 xmlXPathObjectPtr val);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrLocationSetRemove	(xmlLocationSetPtr cur,
						 int val);
#endif /* LIBXML_XPTR_LOCS_ENABLED */

/*
 * Functions.
 */
XMLPUBFUN xmlXPathContextPtr XMLCALL
		    xmlXPtrNewContext		(lxb_dom_document_t_ptr doc,
						 lxb_dom_node_t_ptr here,
						 lxb_dom_node_t_ptr origin);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		    xmlXPtrEval			(const xmlChar *str,
						 xmlXPathContextPtr ctx);
#ifdef LIBXML_XPTR_LOCS_ENABLED
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrRangeToFunction	(xmlXPathParserContextPtr ctxt,
						 int nargs);
XML_DEPRECATED
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		    xmlXPtrBuildNodeList	(xmlXPathObjectPtr obj);
XML_DEPRECATED
XMLPUBFUN void XMLCALL
		    xmlXPtrEvalRangePredicate	(xmlXPathParserContextPtr ctxt);
#endif /* LIBXML_XPTR_LOCS_ENABLED */
#ifdef __cplusplus
}
#endif

#endif /* LIBXML_XPTR_ENABLED */
#endif /* __XML_XPTR_H__ */
