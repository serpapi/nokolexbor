/*
 * encoding.c : implements the encoding conversion functions needed for XML
 *
 * Related specs:
 * rfc2044        (UTF-8 and UTF-16) F. Yergeau Alis Technologies
 * rfc2781        UTF-16, an encoding of ISO 10646, P. Hoffman, F. Yergeau
 * [ISO-10646]    UTF-8 and UTF-16 in Annexes
 * [ISO-8859-1]   ISO Latin-1 characters codes.
 * [UNICODE]      The Unicode Consortium, "The Unicode Standard --
 *                Worldwide Character Encoding -- Version 1.0", Addison-
 *                Wesley, Volume 1, 1991, Volume 2, 1992.  UTF-8 is
 *                described in Unicode Technical Report #4.
 * [US-ASCII]     Coded Character Set--7-bit American Standard Code for
 *                Information Interchange, ANSI X3.4-1986.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 *
 * Original code for IsoLatin1 and UTF-16 by "Martin J. Duerst" <duerst@w3.org>
 */

#define IN_LIBXML
#include "libxml.h"

#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef LIBXML_ICONV_ENABLED
#include <errno.h>
#endif

#include "libxml/encoding.h"
#include "libxml/xmlmemory.h"
#ifdef LIBXML_HTML_ENABLED
#include "libxml/HTMLparser.h"
#endif
#include "libxml/globals.h"
#include "libxml/xmlerror.h"

#include "private/buf.h"
#include "private/enc.h"
#include "private/error.h"

#ifdef LIBXML_ICU_ENABLED
#include <unicode/ucnv.h>
/* Size of pivot buffer, same as icu/source/common/ucnv.cpp CHUNK_SIZE */
#define ICU_PIVOT_BUF_SIZE 1024
typedef struct _uconv_t uconv_t;
struct _uconv_t {
  UConverter *uconv; /* for conversion between an encoding and UTF-16 */
  UConverter *utf8; /* for conversion between UTF-8 and UTF-16 */
  UChar      pivot_buf[ICU_PIVOT_BUF_SIZE];
  UChar      *pivot_source;
  UChar      *pivot_target;
};
#endif

typedef struct _xmlCharEncodingAlias xmlCharEncodingAlias;
typedef xmlCharEncodingAlias *xmlCharEncodingAliasPtr;
struct _xmlCharEncodingAlias {
    const char *name;
    const char *alias;
};

static xmlCharEncodingAliasPtr xmlCharEncodingAliases = NULL;
static int xmlCharEncodingAliasesNb = 0;
static int xmlCharEncodingAliasesMax = 0;

#if defined(LIBXML_ICONV_ENABLED) || defined(LIBXML_ICU_ENABLED)
#if 0
#define DEBUG_ENCODING  /* Define this to get encoding traces */
#endif
#else
#endif

static int xmlLittleEndian = 1;

/**
 * xmlEncodingErrMemory:
 * @extra:  extra information
 *
 * Handle an out of memory condition
 */
static void
xmlEncodingErrMemory(const char *extra)
{
    __nl_xmlSimpleError(XML_FROM_I18N, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}

/**
 * xmlErrEncoding:
 * @error:  the error number
 * @msg:  the error message
 *
 * n encoding error
 */
static void LIBXML_ATTR_FORMAT(2,0)
xmlEncodingErr(xmlParserErrors error, const char *msg, const char *val)
{
    __nl_xmlRaiseError(NULL, NULL, NULL, NULL, NULL,
                    XML_FROM_I18N, error, XML_ERR_FATAL,
                    NULL, 0, val, NULL, NULL, 0, 0, msg, val);
}

/**
 * xmlInitEncodingInternal:
 *
 * Initialize the char encoding support.
 */
void
xmlInitEncodingInternal(void) {
    unsigned short int tst = 0x1234;
    unsigned char *ptr = (unsigned char *) &tst;

    if (*ptr == 0x12) xmlLittleEndian = 0;
    else if (*ptr == 0x34) xmlLittleEndian = 1;
    else {
        xmlEncodingErr(XML_ERR_INTERNAL_ERROR,
	               "Odd problem at endianness detection\n", NULL);
    }
}
