#define IN_LIBXML
#include "libxml.h"
#include <stdarg.h>
#include "libxml/xmlerror.h"

#define XML_GET_VAR_STR(msg, str) {				\
    int       size, prev_size = -1;				\
    int       chars;						\
    char      *larger;						\
    va_list   ap;						\
								\
    str = (char *) xmlMalloc(150);				\
    if (str != NULL) {						\
								\
    size = 150;							\
								\
    while (size < 64000) {					\
	va_start(ap, msg);					\
	chars = vsnprintf(str, size, msg, ap);			\
	va_end(ap);						\
	if ((chars > -1) && (chars < size)) {			\
	    if (prev_size == chars) {				\
		break;						\
	    } else {						\
		prev_size = chars;				\
	    }							\
	}							\
	if (chars > -1)						\
	    size += chars + 1;					\
	else							\
	    size += 100;					\
	if ((larger = (char *) xmlRealloc(str, size)) == NULL) {\
	    break;						\
	}							\
	str = larger;						\
    }}								\
}

/**
 * xmlGenericErrorDefaultFunc:
 * @ctx:  an error context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Default handler for out of context error messages.
 */
void XMLCDECL
xmlGenericErrorDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...) {
}

/**
 * xmlCopyError:
 * @from:  a source error
 * @to:  a target error
 *
 * Save the original error to the new place.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int
xmlCopyError(xmlErrorPtr from, xmlErrorPtr to) {
    char *message, *file, *str1, *str2, *str3;

    if ((from == NULL) || (to == NULL))
        return(-1);

    message = (char *) xmlStrdup((xmlChar *) from->message);
    file = (char *) xmlStrdup ((xmlChar *) from->file);
    str1 = (char *) xmlStrdup ((xmlChar *) from->str1);
    str2 = (char *) xmlStrdup ((xmlChar *) from->str2);
    str3 = (char *) xmlStrdup ((xmlChar *) from->str3);

    if (to->message != NULL)
        xmlFree(to->message);
    if (to->file != NULL)
        xmlFree(to->file);
    if (to->str1 != NULL)
        xmlFree(to->str1);
    if (to->str2 != NULL)
        xmlFree(to->str2);
    if (to->str3 != NULL)
        xmlFree(to->str3);
    to->domain = from->domain;
    to->code = from->code;
    to->level = from->level;
    to->line = from->line;
    to->node = from->node;
    to->int1 = from->int1;
    to->int2 = from->int2;
    to->node = from->node;
    to->ctxt = from->ctxt;
    to->message = message;
    to->file = file;
    to->str1 = str1;
    to->str2 = str2;
    to->str3 = str3;

    return 0;
}

/**
 * __xmlRaiseError:
 * @schannel: the structured callback channel
 * @channel: the old callback channel
 * @data: the callback data
 * @ctx: the parser context or NULL
 * @ctx: the parser context or NULL
 * @domain: the domain for the error
 * @code: the code for the error
 * @level: the xmlErrorLevel for the error
 * @file: the file source of the error (or NULL)
 * @line: the line of the error or 0 if N/A
 * @str1: extra string info
 * @str2: extra string info
 * @str3: extra string info
 * @int1: extra int info
 * @col: column number of the error or 0 if N/A
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Update the appropriate global or contextual error structure,
 * then forward the error message down the parser or generic
 * error callback handler
 */
void XMLCDECL
__xmlRaiseError(xmlStructuredErrorFunc schannel,
                xmlGenericErrorFunc channel, void *data, void *ctx,
                void *nod, int domain, int code, xmlErrorLevel level,
                const char *file, int line, const char *str1,
                const char *str2, const char *str3, int int1, int col,
                const char *msg, ...)
{
    xmlParserCtxtPtr ctxt = NULL;
    lxb_dom_node_t_ptr node = (lxb_dom_node_t_ptr)nod;
    char *str = NULL;
    xmlParserInputPtr input = NULL;
    xmlErrorPtr to = &xmlLastError;
    lxb_dom_node_t_ptr baseptr = NULL;

    if (code == XML_ERR_OK)
        return;
    if ((xmlGetWarningsDefaultValue == 0) && (level == XML_ERR_WARNING))
        return;
    if ((domain == XML_FROM_PARSER) || (domain == XML_FROM_HTML) ||
        (domain == XML_FROM_DTD) || (domain == XML_FROM_NAMESPACE) ||
        (domain == XML_FROM_IO) || (domain == XML_FROM_VALID))
    {
        ctxt = (xmlParserCtxtPtr)ctx;
        if ((schannel == NULL) && (ctxt != NULL) && (ctxt->sax != NULL) &&
            (ctxt->sax->initialized == XML_SAX2_MAGIC) &&
            (ctxt->sax->serror != NULL))
        {
            schannel = ctxt->sax->serror;
            data = ctxt->userData;
        }
    }
    /*
     * Check if structured error handler set
     */
    if (schannel == NULL)
    {
        schannel = xmlStructuredError;
        /*
         * if user has defined handler, change data ptr to user's choice
         */
        if (schannel != NULL)
            data = xmlStructuredErrorContext;
    }
    /*
     * Formatting the message
     */
    if (msg == NULL)
    {
        str = (char *)xmlStrdup(BAD_CAST "No error message provided");
    }
    else
    {
        XML_GET_VAR_STR(msg, str);
    }

    /*
     * specific processing if a parser context is provided
     */
    if (ctxt != NULL)
    {
        if (file == NULL)
        {
            input = ctxt->input;
            if ((input != NULL) && (input->filename == NULL) &&
                (ctxt->inputNr > 1))
            {
                input = ctxt->inputTab[ctxt->inputNr - 2];
            }
            if (input != NULL)
            {
                file = input->filename;
                line = input->line;
                col = input->col;
            }
        }
        to = &ctxt->lastError;
    }

    /*
     * Save the information about the error
     */
    xmlResetError(to);
    to->domain = domain;
    to->code = code;
    to->message = str;
    to->level = level;
    if (file != NULL)
        to->file = (char *)xmlStrdup((const xmlChar *)file);
    to->line = line;
    if (str1 != NULL)
        to->str1 = (char *)xmlStrdup((const xmlChar *)str1);
    if (str2 != NULL)
        to->str2 = (char *)xmlStrdup((const xmlChar *)str2);
    if (str3 != NULL)
        to->str3 = (char *)xmlStrdup((const xmlChar *)str3);
    to->int1 = int1;
    to->int2 = col;
    to->node = node;
    to->ctxt = ctx;

    if (to != &xmlLastError)
        xmlCopyError(to, &xmlLastError);

    if (schannel != NULL)
    {
        schannel(data, to);
    }
}

/**
 * xmlResetError:
 * @err: pointer to the error.
 *
 * Cleanup the error.
 */
void
xmlResetError(xmlErrorPtr err)
{
    if (err == NULL)
        return;
    if (err->code == XML_ERR_OK)
        return;
    if (err->message != NULL)
        xmlFree(err->message);
    if (err->file != NULL)
        xmlFree(err->file);
    if (err->str1 != NULL)
        xmlFree(err->str1);
    if (err->str2 != NULL)
        xmlFree(err->str2);
    if (err->str3 != NULL)
        xmlFree(err->str3);
    memset(err, 0, sizeof(xmlError));
    err->code = XML_ERR_OK;
}

/**
 * __xmlSimpleError:
 * @domain: where the error comes from
 * @code: the error code
 * @node: the context node
 * @extra:  extra information
 *
 * Handle an out of memory condition
 */
void
__xmlSimpleError(int domain, int code, lxb_dom_node_t_ptr node,
                 const char *msg, const char *extra)
{

    if (code == XML_ERR_NO_MEMORY) {
	if (extra)
	    __xmlRaiseError(NULL, NULL, NULL, NULL, node, domain,
			    XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra,
			    NULL, NULL, 0, 0,
			    "Memory allocation failed : %s\n", extra);
	else
	    __xmlRaiseError(NULL, NULL, NULL, NULL, node, domain,
			    XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, NULL,
			    NULL, NULL, 0, 0, "Memory allocation failed\n");
    } else {
	__xmlRaiseError(NULL, NULL, NULL, NULL, node, domain,
			code, XML_ERR_ERROR, NULL, 0, extra,
			NULL, NULL, 0, 0, msg, extra);
    }
}

/**
 * xmlSetGenericErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * One can simply force messages to be emitted to another FILE * than
 * stderr by setting @ctx to this file handle and @handler to NULL.
 * For multi-threaded applications, this must be set separately for each thread.
 */
void
xmlSetGenericErrorFunc(void *ctx, xmlGenericErrorFunc handler) {
    xmlGenericErrorContext = ctx;
    if (handler != NULL)
	xmlGenericError = handler;
    else
	xmlGenericError = xmlGenericErrorDefaultFunc;
}

/**
 * xmlSetStructuredErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context structured error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * For multi-threaded applications, this must be set separately for each thread.
 */
void
xmlSetStructuredErrorFunc(void *ctx, xmlStructuredErrorFunc handler) {
    xmlStructuredErrorContext = ctx;
    xmlStructuredError = handler;
}