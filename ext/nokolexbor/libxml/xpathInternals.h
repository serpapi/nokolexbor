/*
 * Summary: internal interfaces for XML Path Language implementation
 * Description: internal interfaces for XML Path Language implementation
 *              used to build new modules on top of XPath like XPointer and
 *              XSLT
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XPATH_INTERNALS_H__
#define __XML_XPATH_INTERNALS_H__

#include "xmlversion.h"
#include "xpath.h"

#ifdef LIBXML_XPATH_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 *									*
 *			Helpers						*
 *									*
 ************************************************************************/

/*
 * Many of these macros may later turn into functions. They
 * shouldn't be used in #ifdef's preprocessor instructions.
 */
/**
 * xmlXPathSetError:
 * @ctxt:  an XPath parser context
 * @err:  an xmlXPathError code
 *
 * Raises an error.
 */
#define xmlXPathSetError(ctxt, err)					\
    { nl_xmlXPatherror((ctxt), __FILE__, __LINE__, (err));			\
      if ((ctxt) != NULL) (ctxt)->error = (err); }

/**
 * xmlXPathSetArityError:
 * @ctxt:  an XPath parser context
 *
 * Raises an XPATH_INVALID_ARITY error.
 */
#define xmlXPathSetArityError(ctxt)					\
    xmlXPathSetError((ctxt), XPATH_INVALID_ARITY)

/**
 * xmlXPathSetTypeError:
 * @ctxt:  an XPath parser context
 *
 * Raises an XPATH_INVALID_TYPE error.
 */
#define xmlXPathSetTypeError(ctxt)					\
    xmlXPathSetError((ctxt), XPATH_INVALID_TYPE)

/**
 * xmlXPathGetError:
 * @ctxt:  an XPath parser context
 *
 * Get the error code of an XPath context.
 *
 * Returns the context error.
 */
#define xmlXPathGetError(ctxt)	  ((ctxt)->error)

/**
 * xmlXPathCheckError:
 * @ctxt:  an XPath parser context
 *
 * Check if an XPath error was raised.
 *
 * Returns true if an error has been raised, false otherwise.
 */
#define xmlXPathCheckError(ctxt)  ((ctxt)->error != XPATH_EXPRESSION_OK)

/**
 * xmlXPathGetDocument:
 * @ctxt:  an XPath parser context
 *
 * Get the document of an XPath context.
 *
 * Returns the context document.
 */
#define xmlXPathGetDocument(ctxt)	((ctxt)->context->doc)

/**
 * xmlXPathGetContextNode:
 * @ctxt: an XPath parser context
 *
 * Get the context node of an XPath context.
 *
 * Returns the context node.
 */
#define xmlXPathGetContextNode(ctxt)	((ctxt)->context->node)

XMLPUBFUN int XMLCALL
		nl_xmlXPathPopBoolean	(xmlXPathParserContextPtr ctxt);
XMLPUBFUN double XMLCALL
		nl_xmlXPathPopNumber	(xmlXPathParserContextPtr ctxt);
XMLPUBFUN xmlChar * XMLCALL
		nl_xmlXPathPopString	(xmlXPathParserContextPtr ctxt);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathPopNodeSet	(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void * XMLCALL
		nl_xmlXPathPopExternal	(xmlXPathParserContextPtr ctxt);

/**
 * xmlXPathReturnBoolean:
 * @ctxt:  an XPath parser context
 * @val:  a boolean
 *
 * Pushes the boolean @val on the context stack.
 */
#define xmlXPathReturnBoolean(ctxt, val)				\
    valuePush((ctxt), nl_xmlXPathNewBoolean(val))

/**
 * xmlXPathReturnTrue:
 * @ctxt:  an XPath parser context
 *
 * Pushes true on the context stack.
 */
#define xmlXPathReturnTrue(ctxt)   xmlXPathReturnBoolean((ctxt), 1)

/**
 * xmlXPathReturnFalse:
 * @ctxt:  an XPath parser context
 *
 * Pushes false on the context stack.
 */
#define xmlXPathReturnFalse(ctxt)  xmlXPathReturnBoolean((ctxt), 0)

/**
 * xmlXPathReturnNumber:
 * @ctxt:  an XPath parser context
 * @val:  a double
 *
 * Pushes the double @val on the context stack.
 */
#define xmlXPathReturnNumber(ctxt, val)					\
    valuePush((ctxt), nl_xmlXPathNewFloat(val))

/**
 * xmlXPathReturnString:
 * @ctxt:  an XPath parser context
 * @str:  a string
 *
 * Pushes the string @str on the context stack.
 */
#define xmlXPathReturnString(ctxt, str)					\
    valuePush((ctxt), nl_xmlXPathWrapString(str))

/**
 * xmlXPathReturnEmptyString:
 * @ctxt:  an XPath parser context
 *
 * Pushes an empty string on the stack.
 */
#define xmlXPathReturnEmptyString(ctxt)					\
    valuePush((ctxt), nl_xmlXPathNewCString(""))

/**
 * xmlXPathReturnNodeSet:
 * @ctxt:  an XPath parser context
 * @ns:  a node-set
 *
 * Pushes the node-set @ns on the context stack.
 */
#define xmlXPathReturnNodeSet(ctxt, ns)					\
    valuePush((ctxt), nl_xmlXPathWrapNodeSet(ns))

/**
 * xmlXPathReturnEmptyNodeSet:
 * @ctxt:  an XPath parser context
 *
 * Pushes an empty node-set on the context stack.
 */
#define xmlXPathReturnEmptyNodeSet(ctxt)				\
    valuePush((ctxt), nl_xmlXPathNewNodeSet(NULL))

/**
 * xmlXPathReturnExternal:
 * @ctxt:  an XPath parser context
 * @val:  user data
 *
 * Pushes user data on the context stack.
 */
#define xmlXPathReturnExternal(ctxt, val)				\
    valuePush((ctxt), nl_xmlXPathWrapExternal(val))

/**
 * xmlXPathStackIsNodeSet:
 * @ctxt: an XPath parser context
 *
 * Check if the current value on the XPath stack is a node set or
 * an XSLT value tree.
 *
 * Returns true if the current object on the stack is a node-set.
 */
#define xmlXPathStackIsNodeSet(ctxt)					\
    (((ctxt)->value != NULL)						\
     && (((ctxt)->value->type == XPATH_NODESET)				\
         || ((ctxt)->value->type == XPATH_XSLT_TREE)))

/**
 * xmlXPathStackIsExternal:
 * @ctxt: an XPath parser context
 *
 * Checks if the current value on the XPath stack is an external
 * object.
 *
 * Returns true if the current object on the stack is an external
 * object.
 */
#define xmlXPathStackIsExternal(ctxt)					\
	((ctxt->value != NULL) && (ctxt->value->type == XPATH_USERS))

/**
 * xmlXPathEmptyNodeSet:
 * @ns:  a node-set
 *
 * Empties a node-set.
 */
#define xmlXPathEmptyNodeSet(ns)					\
    { while ((ns)->nodeNr > 0) (ns)->nodeTab[--(ns)->nodeNr] = NULL; }

/**
 * CHECK_ERROR:
 *
 * Macro to return from the function if an XPath error was detected.
 */
#define CHECK_ERROR							\
    if (ctxt->error != XPATH_EXPRESSION_OK) return

/**
 * CHECK_ERROR0:
 *
 * Macro to return 0 from the function if an XPath error was detected.
 */
#define CHECK_ERROR0							\
    if (ctxt->error != XPATH_EXPRESSION_OK) return(0)

/**
 * XP_ERROR:
 * @X:  the error code
 *
 * Macro to raise an XPath error and return.
 */
#define XP_ERROR(X)							\
    { nl_xmlXPathErr(ctxt, X); return; }

/**
 * XP_ERROR0:
 * @X:  the error code
 *
 * Macro to raise an XPath error and return 0.
 */
#define XP_ERROR0(X)							\
    { nl_xmlXPathErr(ctxt, X); return(0); }

/**
 * CHECK_TYPE:
 * @typeval:  the XPath type
 *
 * Macro to check that the value on top of the XPath stack is of a given
 * type.
 */
#define CHECK_TYPE(typeval)						\
    if ((ctxt->value == NULL) || (ctxt->value->type != typeval))	\
        XP_ERROR(XPATH_INVALID_TYPE)

/**
 * CHECK_TYPE0:
 * @typeval:  the XPath type
 *
 * Macro to check that the value on top of the XPath stack is of a given
 * type. Return(0) in case of failure
 */
#define CHECK_TYPE0(typeval)						\
    if ((ctxt->value == NULL) || (ctxt->value->type != typeval))	\
        XP_ERROR0(XPATH_INVALID_TYPE)

/**
 * CHECK_ARITY:
 * @x:  the number of expected args
 *
 * Macro to check that the number of args passed to an XPath function matches.
 */
#define CHECK_ARITY(x)							\
    if (ctxt == NULL) return;						\
    if (nargs != (x))							\
        XP_ERROR(XPATH_INVALID_ARITY);					\
    if (ctxt->valueNr < ctxt->valueFrame + (x))				\
        XP_ERROR(XPATH_STACK_ERROR);

/**
 * CAST_TO_STRING:
 *
 * Macro to try to cast the value on the top of the XPath stack to a string.
 */
#define CAST_TO_STRING							\
    if ((ctxt->value != NULL) && (ctxt->value->type != XPATH_STRING))	\
        nl_xmlXPathStringFunction(ctxt, 1);

/**
 * CAST_TO_NUMBER:
 *
 * Macro to try to cast the value on the top of the XPath stack to a number.
 */
#define CAST_TO_NUMBER							\
    if ((ctxt->value != NULL) && (ctxt->value->type != XPATH_NUMBER))	\
        nl_xmlXPathNumberFunction(ctxt, 1);

/**
 * CAST_TO_BOOLEAN:
 *
 * Macro to try to cast the value on the top of the XPath stack to a boolean.
 */
#define CAST_TO_BOOLEAN							\
    if ((ctxt->value != NULL) && (ctxt->value->type != XPATH_BOOLEAN))	\
        nl_xmlXPathBooleanFunction(ctxt, 1);

/*
 * Variable Lookup forwarding.
 */

XMLPUBFUN void XMLCALL
	nl_xmlXPathRegisterVariableLookup	(xmlXPathContextPtr ctxt,
					 xmlXPathVariableLookupFunc f,
					 void *data);

/*
 * Function Lookup forwarding.
 */

XMLPUBFUN void XMLCALL
	    nl_xmlXPathRegisterFuncLookup	(xmlXPathContextPtr ctxt,
					 xmlXPathFuncLookupFunc f,
					 void *funcCtxt);

/*
 * Error reporting.
 */
XMLPUBFUN void XMLCALL
		nl_xmlXPatherror	(xmlXPathParserContextPtr ctxt,
				 const char *file,
				 int line,
				 int no);

XMLPUBFUN void XMLCALL
		nl_xmlXPathErr	(xmlXPathParserContextPtr ctxt,
				 int error);

#ifdef LIBXML_DEBUG_ENABLED
XMLPUBFUN void XMLCALL
		xmlXPathDebugDumpObject	(FILE *output,
					 xmlXPathObjectPtr cur,
					 int depth);
XMLPUBFUN void XMLCALL
	    xmlXPathDebugDumpCompExpr(FILE *output,
					 xmlXPathCompExprPtr comp,
					 int depth);
#endif
/**
 * NodeSet handling.
 */
XMLPUBFUN int XMLCALL
		nl_xmlXPathNodeSetContains		(xmlNodeSetPtr cur,
						 lxb_dom_node_t_ptr val);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathDifference		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathIntersection		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);

XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathDistinctSorted		(xmlNodeSetPtr nodes);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathDistinct		(xmlNodeSetPtr nodes);

XMLPUBFUN int XMLCALL
		nl_xmlXPathHasSameNodes		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);

XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathNodeLeadingSorted	(xmlNodeSetPtr nodes,
						 lxb_dom_node_t_ptr node);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathLeadingSorted		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathNodeLeading		(xmlNodeSetPtr nodes,
						 lxb_dom_node_t_ptr node);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathLeading			(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);

XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathNodeTrailingSorted	(xmlNodeSetPtr nodes,
						 lxb_dom_node_t_ptr node);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathTrailingSorted		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathNodeTrailing		(xmlNodeSetPtr nodes,
						 lxb_dom_node_t_ptr node);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathTrailing		(xmlNodeSetPtr nodes1,
						 xmlNodeSetPtr nodes2);


/**
 * Extending a context.
 */

XMLPUBFUN int XMLCALL
		nl_xmlXPathRegisterNs		(xmlXPathContextPtr ctxt,
						 const xmlChar *prefix,
						 const xmlChar *ns_uri);
XMLPUBFUN const xmlChar * XMLCALL
		nl_xmlXPathNsLookup		(xmlXPathContextPtr ctxt,
						 const xmlChar *prefix);
XMLPUBFUN void XMLCALL
		nl_xmlXPathRegisteredNsCleanup	(xmlXPathContextPtr ctxt);

XMLPUBFUN int XMLCALL
		nl_xmlXPathRegisterFunc		(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 xmlXPathFunction f);
XMLPUBFUN int XMLCALL
		nl_xmlXPathRegisterFuncNS		(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri,
						 xmlXPathFunction f);
XMLPUBFUN int XMLCALL
		nl_xmlXPathRegisterVariable	(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 xmlXPathObjectPtr value);
XMLPUBFUN int XMLCALL
		nl_xmlXPathRegisterVariableNS	(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri,
						 xmlXPathObjectPtr value);
XMLPUBFUN xmlXPathFunction XMLCALL
		nl_xmlXPathFunctionLookup		(xmlXPathContextPtr ctxt,
						 const xmlChar *name);
XMLPUBFUN xmlXPathFunction XMLCALL
		nl_xmlXPathFunctionLookupNS	(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
XMLPUBFUN void XMLCALL
		nl_xmlXPathRegisteredFuncsCleanup	(xmlXPathContextPtr ctxt);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathVariableLookup		(xmlXPathContextPtr ctxt,
						 const xmlChar *name);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathVariableLookupNS	(xmlXPathContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
XMLPUBFUN void XMLCALL
		nl_xmlXPathRegisteredVariablesCleanup(xmlXPathContextPtr ctxt);

/**
 * Utilities to extend XPath.
 */
XMLPUBFUN xmlXPathParserContextPtr XMLCALL
		  nl_xmlXPathNewParserContext	(const xmlChar *str,
						 xmlXPathContextPtr ctxt);
XMLPUBFUN void XMLCALL
		nl_xmlXPathFreeParserContext	(xmlXPathParserContextPtr ctxt);

XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathValuePop		(xmlXPathParserContextPtr ctxt);
XMLPUBFUN int XMLCALL
		nl_xmlXPathValuePush		(xmlXPathParserContextPtr ctxt,
						 xmlXPathObjectPtr value);

XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewString		(const xmlChar *val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewCString		(const char *val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathWrapString		(xmlChar *val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathWrapCString		(char * val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewFloat		(double val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewBoolean		(int val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewNodeSet		(lxb_dom_node_t_ptr val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewValueTree		(lxb_dom_node_t_ptr val);
XMLPUBFUN int XMLCALL
		nl_xmlXPathNodeSetAdd		(xmlNodeSetPtr cur,
						 lxb_dom_node_t_ptr val);
XMLPUBFUN int XMLCALL
		nl_xmlXPathNodeSetAddUnique	(xmlNodeSetPtr cur,
						 lxb_dom_node_t_ptr val);
XMLPUBFUN int XMLCALL
		nl_xmlXPathNodeSetAddNs		(xmlNodeSetPtr cur,
						 lxb_dom_node_t_ptr node,
						 xmlNsPtr ns);
XMLPUBFUN void XMLCALL
		nl_xmlXPathNodeSetSort		(xmlNodeSetPtr set);

XMLPUBFUN void XMLCALL
		nl_xmlXPathRoot			(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL
		nl_xmlXPathEvalExpr		(xmlXPathParserContextPtr ctxt);
XMLPUBFUN xmlChar * XMLCALL
		nl_xmlXPathParseName		(xmlXPathParserContextPtr ctxt);
XMLPUBFUN xmlChar * XMLCALL
		nl_xmlXPathParseNCName		(xmlXPathParserContextPtr ctxt);

/*
 * Existing functions.
 */
XMLPUBFUN double XMLCALL
		nl_xmlXPathStringEvalNumber	(const xmlChar *str);
XMLPUBFUN int XMLCALL
		nl_xmlXPathEvaluatePredicateResult (xmlXPathParserContextPtr ctxt,
						 xmlXPathObjectPtr res);
XMLPUBFUN void XMLCALL
		nl_xmlXPathRegisterAllFunctions	(xmlXPathContextPtr ctxt);
XMLPUBFUN xmlNodeSetPtr XMLCALL
		nl_xmlXPathNodeSetMerge		(xmlNodeSetPtr val1,
						 xmlNodeSetPtr val2);
XMLPUBFUN void XMLCALL
		nl_xmlXPathNodeSetDel		(xmlNodeSetPtr cur,
						 lxb_dom_node_t_ptr val);
XMLPUBFUN void XMLCALL
		nl_xmlXPathNodeSetRemove		(xmlNodeSetPtr cur,
						 int val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathNewNodeSetList		(xmlNodeSetPtr val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathWrapNodeSet		(xmlNodeSetPtr val);
XMLPUBFUN xmlXPathObjectPtr XMLCALL
		nl_xmlXPathWrapExternal		(void *val);

XMLPUBFUN int XMLCALL nl_xmlXPathEqualValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN int XMLCALL nl_xmlXPathNotEqualValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN int XMLCALL nl_xmlXPathCompareValues(xmlXPathParserContextPtr ctxt, int inf, int strict);
XMLPUBFUN void XMLCALL nl_xmlXPathValueFlipSign(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL nl_xmlXPathAddValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL nl_xmlXPathSubValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL nl_xmlXPathMultValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL nl_xmlXPathDivValues(xmlXPathParserContextPtr ctxt);
XMLPUBFUN void XMLCALL nl_xmlXPathModValues(xmlXPathParserContextPtr ctxt);

XMLPUBFUN int XMLCALL nl_xmlXPathIsNodeType(const xmlChar *name);

/*
 * Some of the axis navigation routines.
 */
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextSelf(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextChild(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextDescendant(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextDescendantOrSelf(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextParent(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextAncestorOrSelf(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextFollowingSibling(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextFollowing(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextNamespace(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextAttribute(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextPreceding(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextAncestor(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL nl_xmlXPathNextPrecedingSibling(xmlXPathParserContextPtr ctxt,
			lxb_dom_node_t_ptr cur);
/*
 * The official core of XPath functions.
 */
XMLPUBFUN void XMLCALL nl_xmlXPathLastFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathPositionFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathCountFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathIdFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathLocalNameFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathNamespaceURIFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathStringFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathStringLengthFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathConcatFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathContainsFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathStartsWithFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathSubstringFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathSubstringBeforeFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathSubstringAfterFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathNormalizeFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathTranslateFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathNotFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathTrueFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathFalseFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathLangFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathNumberFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathSumFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathFloorFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathCeilingFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathRoundFunction(xmlXPathParserContextPtr ctxt, int nargs);
XMLPUBFUN void XMLCALL nl_xmlXPathBooleanFunction(xmlXPathParserContextPtr ctxt, int nargs);

/**
 * Really internal functions
 */
XMLPUBFUN void XMLCALL nl_xmlXPathNodeSetFreeNs(xmlNsPtr ns);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_XPATH_ENABLED */
#endif /* ! __XML_XPATH_INTERNALS_H__ */
