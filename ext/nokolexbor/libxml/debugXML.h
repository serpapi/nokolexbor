/*
 * Summary: Tree debugging APIs
 * Description: Interfaces to a set of routines used for debugging the tree
 *              produced by the XML parser.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __DEBUG_XML__
#define __DEBUG_XML__
#include <stdio.h>
#include "xmlversion.h"
#include "tree.h"

#ifdef LIBXML_DEBUG_ENABLED

#include "xpath.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The standard Dump routines.
 */
XMLPUBFUN void XMLCALL
	xmlDebugDumpString	(FILE *output,
				 const xmlChar *str);
XMLPUBFUN void XMLCALL
	xmlDebugDumpAttr	(FILE *output,
				 lxb_dom_attr_t_ptr attr,
				 int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpAttrList	(FILE *output,
				 lxb_dom_attr_t_ptr attr,
				 int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpOneNode	(FILE *output,
				 lxb_dom_node_t_ptr node,
				 int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpNode	(FILE *output,
				 lxb_dom_node_t_ptr node,
				 int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpNodeList	(FILE *output,
				 lxb_dom_node_t_ptr node,
				 int depth);
XMLPUBFUN void XMLCALL
	xmlDebugDumpDocumentHead(FILE *output,
				 lxb_dom_document_t_ptr doc);
XMLPUBFUN void XMLCALL
	xmlDebugDumpDocument	(FILE *output,
				 lxb_dom_document_t_ptr doc);
XMLPUBFUN void XMLCALL
	xmlDebugDumpDTD		(FILE *output,
				 xmlDtdPtr dtd);
XMLPUBFUN void XMLCALL
	xmlDebugDumpEntities	(FILE *output,
				 lxb_dom_document_t_ptr doc);

/****************************************************************
 *								*
 *			Checking routines			*
 *								*
 ****************************************************************/

XMLPUBFUN int XMLCALL
	xmlDebugCheckDocument	(FILE * output,
				 lxb_dom_document_t_ptr doc);

/****************************************************************
 *								*
 *			XML shell helpers			*
 *								*
 ****************************************************************/

XMLPUBFUN void XMLCALL
	xmlLsOneNode		(FILE *output, lxb_dom_node_t_ptr node);
XMLPUBFUN int XMLCALL
	xmlLsCountNode		(lxb_dom_node_t_ptr node);

XMLPUBFUN const char * XMLCALL
	xmlBoolToText		(int boolval);

/****************************************************************
 *								*
 *	 The XML shell related structures and functions		*
 *								*
 ****************************************************************/

#ifdef LIBXML_XPATH_ENABLED
/**
 * xmlShellReadlineFunc:
 * @prompt:  a string prompt
 *
 * This is a generic signature for the XML shell input function.
 *
 * Returns a string which will be freed by the Shell.
 */
typedef char * (* xmlShellReadlineFunc)(char *prompt);

/**
 * xmlShellCtxt:
 *
 * A debugging shell context.
 * TODO: add the defined function tables.
 */
typedef struct _xmlShellCtxt xmlShellCtxt;
typedef xmlShellCtxt *xmlShellCtxtPtr;
struct _xmlShellCtxt {
    char *filename;
    lxb_dom_document_t_ptr doc;
    lxb_dom_node_t_ptr node;
    xmlXPathContextPtr pctxt;
    int loaded;
    FILE *output;
    xmlShellReadlineFunc input;
};

/**
 * xmlShellCmd:
 * @ctxt:  a shell context
 * @arg:  a string argument
 * @node:  a first node
 * @node2:  a second node
 *
 * This is a generic signature for the XML shell functions.
 *
 * Returns an int, negative returns indicating errors.
 */
typedef int (* xmlShellCmd) (xmlShellCtxtPtr ctxt,
                             char *arg,
			     lxb_dom_node_t_ptr node,
			     lxb_dom_node_t_ptr node2);

XMLPUBFUN void XMLCALL
	xmlShellPrintXPathError	(int errorType,
				 const char *arg);
XMLPUBFUN void XMLCALL
	xmlShellPrintXPathResult(xmlXPathObjectPtr list);
XMLPUBFUN int XMLCALL
	xmlShellList		(xmlShellCtxtPtr ctxt,
				 char *arg,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellBase		(xmlShellCtxtPtr ctxt,
				 char *arg,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellDir		(xmlShellCtxtPtr ctxt,
				 char *arg,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellLoad		(xmlShellCtxtPtr ctxt,
				 char *filename,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void XMLCALL
	xmlShellPrintNode	(lxb_dom_node_t_ptr node);
XMLPUBFUN int XMLCALL
	xmlShellCat		(xmlShellCtxtPtr ctxt,
				 char *arg,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellWrite		(xmlShellCtxtPtr ctxt,
				 char *filename,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellSave		(xmlShellCtxtPtr ctxt,
				 char *filename,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
#endif /* LIBXML_OUTPUT_ENABLED */
#ifdef LIBXML_VALID_ENABLED
XMLPUBFUN int XMLCALL
	xmlShellValidate	(xmlShellCtxtPtr ctxt,
				 char *dtd,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);
#endif /* LIBXML_VALID_ENABLED */
XMLPUBFUN int XMLCALL
	xmlShellDu		(xmlShellCtxtPtr ctxt,
				 char *arg,
				 lxb_dom_node_t_ptr tree,
				 lxb_dom_node_t_ptr node2);
XMLPUBFUN int XMLCALL
	xmlShellPwd		(xmlShellCtxtPtr ctxt,
				 char *buffer,
				 lxb_dom_node_t_ptr node,
				 lxb_dom_node_t_ptr node2);

/*
 * The Shell interface.
 */
XMLPUBFUN void XMLCALL
	xmlShell		(lxb_dom_document_t_ptr doc,
				 char *filename,
				 xmlShellReadlineFunc input,
				 FILE *output);

#endif /* LIBXML_XPATH_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_DEBUG_ENABLED */
#endif /* __DEBUG_XML__ */
