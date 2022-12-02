#define IN_LIBXML
#include "libxml.h"
#include <stdarg.h>
#include "libxml/xmlerror.h"

void *_xmlGenericErrorContext = NULL;

// void * *
// __xmlGenericErrorContext(void) {
// 	return (&_xmlGenericErrorContext);
// }

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
    va_list args;

    if (_xmlGenericErrorContext == NULL)
	_xmlGenericErrorContext = (void *) stderr;

    va_start(args, msg);
    vfprintf((FILE *)_xmlGenericErrorContext, msg, args);
    va_end(args);
}

// xmlGenericErrorFunc _xmlGenericError = xmlGenericErrorDefaultFunc;

// xmlGenericErrorFunc *
// __xmlGenericError(void) {
// 	return (&_xmlGenericError);
// }

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
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}