/*
 * SAX2.c : Default SAX2 handler to build a tree.
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 */


#define IN_LIBXML
#include "libxml.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include "libxml/globals.h"

/**
 * nl_xmlSAX2GetPublicId:
 * @ctx: the user data (XML parser context)
 *
 * Provides the public ID e.g. "-//SGMLSOURCE//DTD DEMO//EN"
 *
 * Returns a xmlChar *
 */
const xmlChar *
nl_xmlSAX2GetPublicId(void *ctx ATTRIBUTE_UNUSED)
{
    /* xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx; */
    return(NULL);
}

/**
 * nl_xmlSAX2GetSystemId:
 * @ctx: the user data (XML parser context)
 *
 * Provides the system ID, basically URL or filename e.g.
 * http://www.sgmlsource.com/dtds/memo.dtd
 *
 * Returns a xmlChar *
 */
const xmlChar *
nl_xmlSAX2GetSystemId(void *ctx)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
    if ((ctx == NULL) || (ctxt->input == NULL)) return(NULL);
    return((const xmlChar *) ctxt->input->filename);
}

/**
 * nl_xmlSAX2GetLineNumber:
 * @ctx: the user data (XML parser context)
 *
 * Provide the line number of the current parsing point.
 *
 * Returns an int
 */
int
nl_xmlSAX2GetLineNumber(void *ctx)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
    if ((ctx == NULL) || (ctxt->input == NULL)) return(0);
    return(ctxt->input->line);
}

/**
 * nl_xmlSAX2GetColumnNumber:
 * @ctx: the user data (XML parser context)
 *
 * Provide the column number of the current parsing point.
 *
 * Returns an int
 */
int
nl_xmlSAX2GetColumnNumber(void *ctx)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
    if ((ctx == NULL) || (ctxt->input == NULL)) return(0);
    return(ctxt->input->col);
}