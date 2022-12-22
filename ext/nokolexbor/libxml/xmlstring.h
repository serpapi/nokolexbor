/*
 * Summary: set of routines to process strings
 * Description: type and interfaces needed for the internal string handling
 *              of the library, especially UTF8 processing.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_STRING_H__
#define __XML_STRING_H__

#include <stdarg.h>
#include "xmlversion.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlChar:
 *
 * This is a basic byte in an UTF-8 encoded string.
 * It's unsigned allowing to pinpoint case where char * are assigned
 * to xmlChar * (possibly making serialization back impossible).
 */
typedef unsigned char xmlChar;

/**
 * BAD_CAST:
 *
 * Macro to cast a string to an xmlChar * when one know its safe.
 */
#define BAD_CAST (xmlChar *)

/*
 * xmlChar handling
 */
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrdup                (const xmlChar *cur);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrndup               (const xmlChar *cur,
                                         int len);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlCharStrndup           (const char *cur,
                                         int len);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlCharStrdup            (const char *cur);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrsub                (const xmlChar *str,
                                         int start,
                                         int len);
XMLPUBFUN const xmlChar * XMLCALL
                nl_xmlStrchr                (const xmlChar *str,
                                         xmlChar val);
XMLPUBFUN const xmlChar * XMLCALL
                nl_xmlStrstr                (const xmlChar *str,
                                         const xmlChar *val);
XMLPUBFUN const xmlChar * XMLCALL
                nl_xmlStrcasestr            (const xmlChar *str,
                                         const xmlChar *val);
XMLPUBFUN int XMLCALL
                nl_xmlStrcmp                (const xmlChar *str1,
                                         const xmlChar *str2);
XMLPUBFUN int XMLCALL
                nl_xmlStrncmp               (const xmlChar *str1,
                                         const xmlChar *str2,
                                         int len);
XMLPUBFUN int XMLCALL
                nl_xmlStrcasecmp            (const xmlChar *str1,
                                         const xmlChar *str2);
XMLPUBFUN int XMLCALL
                nl_xmlStrncasecmp           (const xmlChar *str1,
                                         const xmlChar *str2,
                                         int len);
XMLPUBFUN int XMLCALL
                nl_xmlStrEqual              (const xmlChar *str1,
                                         const xmlChar *str2);
XMLPUBFUN int XMLCALL
                nl_xmlStrQEqual             (const xmlChar *pref,
                                         const xmlChar *name,
                                         const xmlChar *str);
XMLPUBFUN int XMLCALL
                nl_xmlStrlen                (const xmlChar *str);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrcat                (xmlChar *cur,
                                         const xmlChar *add);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrncat               (xmlChar *cur,
                                         const xmlChar *add,
                                         int len);
XMLPUBFUN xmlChar * XMLCALL
                nl_xmlStrncatNew            (const xmlChar *str1,
                                         const xmlChar *str2,
                                         int len);
XMLPUBFUN int XMLCALL
                nl_xmlStrPrintf             (xmlChar *buf,
                                         int len,
                                         const char *msg,
                                         ...) LIBXML_ATTR_FORMAT(3,4);
XMLPUBFUN int XMLCALL
                nl_xmlStrVPrintf                (xmlChar *buf,
                                         int len,
                                         const char *msg,
                                         va_list ap) LIBXML_ATTR_FORMAT(3,0);

XMLPUBFUN int XMLCALL
        nl_xmlGetUTF8Char                   (const unsigned char *utf,
                                         int *len);
XMLPUBFUN int XMLCALL
        nl_xmlCheckUTF8                     (const unsigned char *utf);
XMLPUBFUN int XMLCALL
        nl_xmlUTF8Strsize                   (const xmlChar *utf,
                                         int len);
XMLPUBFUN xmlChar * XMLCALL
        nl_xmlUTF8Strndup                   (const xmlChar *utf,
                                         int len);
XMLPUBFUN const xmlChar * XMLCALL
        nl_xmlUTF8Strpos                    (const xmlChar *utf,
                                         int pos);
XMLPUBFUN int XMLCALL
        nl_xmlUTF8Strloc                    (const xmlChar *utf,
                                         const xmlChar *utfchar);
XMLPUBFUN xmlChar * XMLCALL
        nl_xmlUTF8Strsub                    (const xmlChar *utf,
                                         int start,
                                         int len);
XMLPUBFUN int XMLCALL
        nl_xmlUTF8Strlen                    (const xmlChar *utf);
XMLPUBFUN int XMLCALL
        nl_xmlUTF8Size                      (const xmlChar *utf);
XMLPUBFUN int XMLCALL
        nl_xmlUTF8Charcmp                   (const xmlChar *utf1,
                                         const xmlChar *utf2);

#ifdef __cplusplus
}
#endif
#endif /* __XML_STRING_H__ */
