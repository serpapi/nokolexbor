/*
 * Summary: interfaces for tree manipulation
 * Description: this module describes the structures found in an tree resulting
 *              from an XML or HTML parsing, as well as the API provided for
 *              various processing on that tree
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_TREE_H__
#define __XML_TREE_H__

#include <stdio.h>
#include <limits.h>
#include "xmlversion.h"
#include "xmlstring.h"
#include "../nokolexbor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Some of the basic types pointer to structures:
 */
/* xmlIO.h */
typedef struct _xmlParserInputBuffer xmlParserInputBuffer;
typedef xmlParserInputBuffer *xmlParserInputBufferPtr;

typedef struct _xmlOutputBuffer xmlOutputBuffer;
typedef xmlOutputBuffer *xmlOutputBufferPtr;

/* parser.h */
typedef struct _xmlParserInput xmlParserInput;
typedef xmlParserInput *xmlParserInputPtr;

typedef struct _xmlParserCtxt xmlParserCtxt;
typedef xmlParserCtxt *xmlParserCtxtPtr;

typedef struct _xmlSAXLocator xmlSAXLocator;
typedef xmlSAXLocator *xmlSAXLocatorPtr;

typedef struct _xmlSAXHandler xmlSAXHandler;
typedef xmlSAXHandler *xmlSAXHandlerPtr;

/* entities.h */
typedef struct _xmlEntity xmlEntity;
typedef xmlEntity *xmlEntityPtr;

/**
 * BASE_BUFFER_SIZE:
 *
 * default buffer size 4000.
 */
#define BASE_BUFFER_SIZE 4096

/**
 * LIBXML_NAMESPACE_DICT:
 *
 * Defines experimental behaviour:
 * 1) xmlNs gets an additional field @context (a xmlDoc)
 * 2) when creating a tree, xmlNs->href is stored in the dict of xmlDoc.
 */
/* #define LIBXML_NAMESPACE_DICT */

/**
 * xmlBufferAllocationScheme:
 *
 * A buffer allocation scheme can be defined to either match exactly the
 * need or double it's allocated size each time it is found too small.
 */

typedef enum {
    XML_BUFFER_ALLOC_DOUBLEIT,	/* double each time one need to grow */
    XML_BUFFER_ALLOC_EXACT,	/* grow only to the minimal size */
    XML_BUFFER_ALLOC_IMMUTABLE, /* immutable buffer, deprecated */
    XML_BUFFER_ALLOC_IO,	/* special allocation scheme used for I/O */
    XML_BUFFER_ALLOC_HYBRID,	/* exact up to a threshold, and doubleit thereafter */
    XML_BUFFER_ALLOC_BOUNDED	/* limit the upper size of the buffer */
} xmlBufferAllocationScheme;

/**
 * xmlBuffer:
 *
 * A buffer structure, this old construct is limited to 2GB and
 * is being deprecated, use API with xmlBuf instead
 */
typedef struct _xmlBuffer xmlBuffer;
typedef xmlBuffer *xmlBufferPtr;
struct _xmlBuffer {
    xmlChar *content;		/* The buffer content UTF8 */
    unsigned int use;		/* The buffer size used */
    unsigned int size;		/* The buffer size */
    xmlBufferAllocationScheme alloc; /* The realloc method */
    xmlChar *contentIO;		/* in IO mode we may have a different base */
};

/**
 * xmlBuf:
 *
 * A buffer structure, new one, the actual structure internals are not public
 */

typedef struct _xmlBuf xmlBuf;

/**
 * xmlBufPtr:
 *
 * A pointer to a buffer structure, the actual structure internals are not
 * public
 */

typedef xmlBuf *xmlBufPtr;

/*
 * A few public routines for xmlBuf. As those are expected to be used
 * mostly internally the bulk of the routines are internal in buf.h
 */
XMLPUBFUN xmlChar* XMLCALL       xmlBufContent	(const xmlBuf* buf);
XMLPUBFUN xmlChar* XMLCALL       xmlBufEnd      (xmlBufPtr buf);
XMLPUBFUN size_t XMLCALL         xmlBufUse      (const xmlBufPtr buf);
XMLPUBFUN size_t XMLCALL         xmlBufShrink	(xmlBufPtr buf, size_t len);

/*
 * LIBXML2_NEW_BUFFER:
 *
 * Macro used to express that the API use the new buffers for
 * xmlParserInputBuffer and xmlOutputBuffer. The change was
 * introduced in 2.9.0.
 */
#define LIBXML2_NEW_BUFFER

/**
 * XML_XML_NAMESPACE:
 *
 * This is the namespace for the special xml: prefix predefined in the
 * XML Namespace specification.
 */
#define XML_XML_NAMESPACE \
    (const xmlChar *) "http://www.w3.org/XML/1998/namespace"

/**
 * XML_XML_ID:
 *
 * This is the name for the special xml:id attribute
 */
#define XML_XML_ID (const xmlChar *) "xml:id"

/*
 * The different element types carried by an XML tree.
 *
 * NOTE: This is synchronized with DOM Level1 values
 *       See http://www.w3.org/TR/REC-DOM-Level-1/
 *
 * Actually this had diverged a bit, and now XML_DOCUMENT_TYPE_NODE should
 * be deprecated to use an XML_DTD_NODE.
 */
typedef enum {
    XML_ELEMENT_NODE=		1,
    XML_ATTRIBUTE_NODE=		2,
    XML_TEXT_NODE=		3,
    XML_CDATA_SECTION_NODE=	4,
    XML_ENTITY_REF_NODE=	5,
    XML_ENTITY_NODE=		6,
    XML_PI_NODE=		7,
    XML_COMMENT_NODE=		8,
    XML_DOCUMENT_NODE=		9,
    XML_DOCUMENT_TYPE_NODE=	10,
    XML_DOCUMENT_FRAG_NODE=	11,
    XML_NOTATION_NODE=		12,
    XML_HTML_DOCUMENT_NODE=	13,
    XML_DTD_NODE=		14,
    XML_ELEMENT_DECL=		15,
    XML_ATTRIBUTE_DECL=		16,
    XML_ENTITY_DECL=		17,
    XML_NAMESPACE_DECL=		18,
    XML_XINCLUDE_START=		19,
    XML_XINCLUDE_END=		20
    /* XML_DOCB_DOCUMENT_NODE=	21 */ /* removed */
} xmlElementType;

/** DOC_DISABLE */
/* For backward compatibility */
#define XML_DOCB_DOCUMENT_NODE 21
/** DOC_ENABLE */

/**
 * xmlNotation:
 *
 * A DTD Notation definition.
 */

typedef struct _xmlNotation xmlNotation;
typedef xmlNotation *xmlNotationPtr;
struct _xmlNotation {
    const xmlChar               *name;	        /* Notation name */
    const xmlChar               *PublicID;	/* Public identifier, if any */
    const xmlChar               *SystemID;	/* System identifier, if any */
};

/**
 * xmlAttributeType:
 *
 * A DTD Attribute type definition.
 */

typedef enum {
    XML_ATTRIBUTE_CDATA = 1,
    XML_ATTRIBUTE_ID,
    XML_ATTRIBUTE_IDREF	,
    XML_ATTRIBUTE_IDREFS,
    XML_ATTRIBUTE_ENTITY,
    XML_ATTRIBUTE_ENTITIES,
    XML_ATTRIBUTE_NMTOKEN,
    XML_ATTRIBUTE_NMTOKENS,
    XML_ATTRIBUTE_ENUMERATION,
    XML_ATTRIBUTE_NOTATION
} xmlAttributeType;

/**
 * xmlAttributeDefault:
 *
 * A DTD Attribute default definition.
 */

typedef enum {
    XML_ATTRIBUTE_NONE = 1,
    XML_ATTRIBUTE_REQUIRED,
    XML_ATTRIBUTE_IMPLIED,
    XML_ATTRIBUTE_FIXED
} xmlAttributeDefault;

/**
 * xmlEnumeration:
 *
 * List structure used when there is an enumeration in DTDs.
 */

typedef struct _xmlEnumeration xmlEnumeration;
typedef xmlEnumeration *xmlEnumerationPtr;
struct _xmlEnumeration {
    struct _xmlEnumeration    *next;	/* next one */
    const xmlChar            *name;	/* Enumeration name */
};

/**
 * xmlAttribute:
 *
 * An Attribute declaration in a DTD.
 */

typedef struct _xmlAttribute xmlAttribute;
typedef xmlAttribute *xmlAttributePtr;
struct _xmlAttribute {
    void           *_private;	        /* application data */
    xmlElementType          type;       /* XML_ATTRIBUTE_DECL, must be second ! */
    const xmlChar          *name;	/* Attribute name */
    struct _xmlNode    *children;	/* NULL */
    struct _xmlNode        *last;	/* NULL */
    struct _xmlDtd       *parent;	/* -> DTD */
    struct _xmlNode        *next;	/* next sibling link  */
    struct _xmlNode        *prev;	/* previous sibling link  */
    struct _xmlDoc          *doc;       /* the containing document */

    struct _xmlAttribute  *nexth;	/* next in hash table */
    xmlAttributeType       atype;	/* The attribute type */
    xmlAttributeDefault      def;	/* the default */
    const xmlChar  *defaultValue;	/* or the default value */
    xmlEnumerationPtr       tree;       /* or the enumeration tree if any */
    const xmlChar        *prefix;	/* the namespace prefix if any */
    const xmlChar          *elem;	/* Element holding the attribute */
};

/**
 * xmlElementContentType:
 *
 * Possible definitions of element content types.
 */
typedef enum {
    XML_ELEMENT_CONTENT_PCDATA = 1,
    XML_ELEMENT_CONTENT_ELEMENT,
    XML_ELEMENT_CONTENT_SEQ,
    XML_ELEMENT_CONTENT_OR
} xmlElementContentType;

/**
 * xmlElementContentOccur:
 *
 * Possible definitions of element content occurrences.
 */
typedef enum {
    XML_ELEMENT_CONTENT_ONCE = 1,
    XML_ELEMENT_CONTENT_OPT,
    XML_ELEMENT_CONTENT_MULT,
    XML_ELEMENT_CONTENT_PLUS
} xmlElementContentOccur;

/**
 * xmlElementContent:
 *
 * An XML Element content as stored after parsing an element definition
 * in a DTD.
 */

typedef struct _xmlElementContent xmlElementContent;
typedef xmlElementContent *xmlElementContentPtr;
struct _xmlElementContent {
    xmlElementContentType     type;	/* PCDATA, ELEMENT, SEQ or OR */
    xmlElementContentOccur    ocur;	/* ONCE, OPT, MULT or PLUS */
    const xmlChar             *name;	/* Element name */
    struct _xmlElementContent *c1;	/* first child */
    struct _xmlElementContent *c2;	/* second child */
    struct _xmlElementContent *parent;	/* parent */
    const xmlChar             *prefix;	/* Namespace prefix */
};

/**
 * xmlElementTypeVal:
 *
 * The different possibilities for an element content type.
 */

typedef enum {
    XML_ELEMENT_TYPE_UNDEFINED = 0,
    XML_ELEMENT_TYPE_EMPTY = 1,
    XML_ELEMENT_TYPE_ANY,
    XML_ELEMENT_TYPE_MIXED,
    XML_ELEMENT_TYPE_ELEMENT
} xmlElementTypeVal;

#ifdef __cplusplus
}
#endif
#include "xmlregexp.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlElement:
 *
 * An XML Element declaration from a DTD.
 */

typedef struct _xmlElement xmlElement;
typedef xmlElement *xmlElementPtr;
struct _xmlElement {
    void           *_private;	        /* application data */
    xmlElementType          type;       /* XML_ELEMENT_DECL, must be second ! */
    const xmlChar          *name;	/* Element name */
    struct _xmlNode    *children;	/* NULL */
    struct _xmlNode        *last;	/* NULL */
    struct _xmlDtd       *parent;	/* -> DTD */
    struct _xmlNode        *next;	/* next sibling link  */
    struct _xmlNode        *prev;	/* previous sibling link  */
    struct _xmlDoc          *doc;       /* the containing document */

    xmlElementTypeVal      etype;	/* The type */
    xmlElementContentPtr content;	/* the allowed element content */
    xmlAttributePtr   attributes;	/* List of the declared attributes */
    const xmlChar        *prefix;	/* the namespace prefix if any */
#ifdef LIBXML_REGEXP_ENABLED
    xmlRegexpPtr       contModel;	/* the validating regexp */
#else
    void	      *contModel;
#endif
};


/**
 * XML_LOCAL_NAMESPACE:
 *
 * A namespace declaration node.
 */
#define XML_LOCAL_NAMESPACE XML_NAMESPACE_DECL
typedef xmlElementType xmlNsType;

/**
 * xmlNs:
 *
 * An XML namespace.
 * Note that prefix == NULL is valid, it defines the default namespace
 * within the subtree (until overridden).
 *
 * xmlNsType is unified with xmlElementType.
 */

typedef struct _xmlNs xmlNs;
typedef xmlNs *xmlNsPtr;
struct _xmlNs {
    struct _xmlNs  *next;	/* next Ns link for this node  */
    xmlNsType      type;	/* global or local */
    const xmlChar *href;	/* URL for the namespace */
    const xmlChar *prefix;	/* prefix for the namespace */
    void           *_private;   /* application data */
    struct _xmlDoc *context;		/* normally an xmlDoc */
};

/**
 * xmlDtd:
 *
 * An XML DTD, as defined by <!DOCTYPE ... There is actually one for
 * the internal subset and for the external subset.
 */
typedef struct _xmlDtd xmlDtd;
typedef xmlDtd *xmlDtdPtr;
struct _xmlDtd {
    void           *_private;	/* application data */
    xmlElementType  type;       /* XML_DTD_NODE, must be second ! */
    const xmlChar *name;	/* Name of the DTD */
    struct _xmlNode *children;	/* the value of the property link */
    struct _xmlNode *last;	/* last child link */
    struct _xmlDoc  *parent;	/* child->parent link */
    struct _xmlNode *next;	/* next sibling link  */
    struct _xmlNode *prev;	/* previous sibling link  */
    struct _xmlDoc  *doc;	/* the containing document */

    /* End of common part */
    void          *notations;   /* Hash table for notations if any */
    void          *elements;    /* Hash table for elements if any */
    void          *attributes;  /* Hash table for attributes if any */
    void          *entities;    /* Hash table for entities if any */
    const xmlChar *ExternalID;	/* External identifier for PUBLIC DTD */
    const xmlChar *SystemID;	/* URI for a SYSTEM or PUBLIC DTD */
    void          *pentities;   /* Hash table for param entities if any */
};

/**
 * xmlAttr:
 *
 * An attribute on an XML node.
 */
typedef struct _xmlAttr xmlAttr;
typedef lxb_dom_attr_t *lxb_dom_attr_t_ptr;
struct _xmlAttr {
    void           *_private;	/* application data */
    xmlElementType   type;      /* XML_ATTRIBUTE_NODE, must be second ! */
    const xmlChar   *name;      /* the name of the property */
    struct _xmlNode *children;	/* the value of the property */
    struct _xmlNode *last;	/* NULL */
    struct _xmlNode *parent;	/* child->parent link */
    struct _xmlAttr *next;	/* next sibling link  */
    struct _xmlAttr *prev;	/* previous sibling link  */
    struct _xmlDoc  *doc;	/* the containing document */
    xmlNs           *ns;        /* pointer to the associated namespace */
    xmlAttributeType atype;     /* the attribute type if validating */
    void            *psvi;	/* for type/PSVI information */
};

/**
 * xmlID:
 *
 * An XML ID instance.
 */

typedef struct _xmlID xmlID;
typedef xmlID *xmlIDPtr;
struct _xmlID {
    struct _xmlID    *next;	/* next ID */
    const xmlChar    *value;	/* The ID name */
    lxb_dom_attr_t_ptr        attr;	/* The attribute holding it */
    const xmlChar    *name;	/* The attribute if attr is not available */
    int               lineno;	/* The line number if attr is not available */
    struct _xmlDoc   *doc;	/* The document holding the ID */
};

/**
 * xmlRef:
 *
 * An XML IDREF instance.
 */

typedef struct _xmlRef xmlRef;
typedef xmlRef *xmlRefPtr;
struct _xmlRef {
    struct _xmlRef    *next;	/* next Ref */
    const xmlChar     *value;	/* The Ref name */
    lxb_dom_attr_t_ptr        attr;	/* The attribute holding it */
    const xmlChar    *name;	/* The attribute if attr is not available */
    int               lineno;	/* The line number if attr is not available */
};

/**
 * xmlNode:
 *
 * A node in an XML tree.
 */
typedef struct _xmlNode xmlNode;
typedef lxb_dom_node_t *lxb_dom_node_t_ptr;
struct _xmlNode {
    void           *_private;	/* application data */
    xmlElementType   type;	/* type number, must be second ! */
    const xmlChar   *name;      /* the name of the node, or the entity */
    struct _xmlNode *children;	/* parent->childs link */
    struct _xmlNode *last;	/* last child link */
    struct _xmlNode *parent;	/* child->parent link */
    struct _xmlNode *next;	/* next sibling link  */
    struct _xmlNode *prev;	/* previous sibling link  */
    struct _xmlDoc  *doc;	/* the containing document */

    /* End of common part */
    xmlNs           *ns;        /* pointer to the associated namespace */
    xmlChar         *content;   /* the content */
    struct _xmlAttr *properties;/* properties list */
    xmlNs           *nsDef;     /* namespace definitions on this node */
    void            *psvi;	/* for type/PSVI information */
    unsigned short   line;	/* line number */
    unsigned short   extra;	/* extra data for XPath/XSLT */
};

/**
 * XML_GET_CONTENT:
 *
 * Macro to extract the content pointer of a node.
 */
#define XML_GET_CONTENT(n)					\
    ((n)->type == XML_ELEMENT_NODE ? NULL : (n)->content)

/**
 * XML_GET_LINE:
 *
 * Macro to extract the line number of an element node.
 */
#define XML_GET_LINE(n)						\
    (xmlGetLineNo(n))

/**
 * xmlDocProperty
 *
 * Set of properties of the document as found by the parser
 * Some of them are linked to similarly named xmlParserOption
 */
typedef enum {
    XML_DOC_WELLFORMED		= 1<<0, /* document is XML well formed */
    XML_DOC_NSVALID		= 1<<1, /* document is Namespace valid */
    XML_DOC_OLD10		= 1<<2, /* parsed with old XML-1.0 parser */
    XML_DOC_DTDVALID		= 1<<3, /* DTD validation was successful */
    XML_DOC_XINCLUDE		= 1<<4, /* XInclude substitution was done */
    XML_DOC_USERBUILT		= 1<<5, /* Document was built using the API
                                           and not by parsing an instance */
    XML_DOC_INTERNAL		= 1<<6, /* built for internal processing */
    XML_DOC_HTML		= 1<<7  /* parsed or built HTML document */
} xmlDocProperties;

/**
 * xmlDoc:
 *
 * An XML document.
 */
typedef struct _xmlDoc xmlDoc;
typedef lxb_dom_document_t *lxb_dom_document_t_ptr;
struct _xmlDoc {
    void           *_private;	/* application data */
    xmlElementType  type;       /* XML_DOCUMENT_NODE, must be second ! */
    char           *name;	/* name/filename/URI of the document */
    struct _xmlNode *children;	/* the document tree */
    struct _xmlNode *last;	/* last child link */
    struct _xmlNode *parent;	/* child->parent link */
    struct _xmlNode *next;	/* next sibling link  */
    struct _xmlNode *prev;	/* previous sibling link  */
    struct _xmlDoc  *doc;	/* autoreference to itself */

    /* End of common part */
    int             compression;/* level of zlib compression */
    int             standalone; /* standalone document (no external refs)
				     1 if standalone="yes"
				     0 if standalone="no"
				    -1 if there is no XML declaration
				    -2 if there is an XML declaration, but no
					standalone attribute was specified */
    struct _xmlDtd  *intSubset;	/* the document internal subset */
    struct _xmlDtd  *extSubset;	/* the document external subset */
    struct _xmlNs   *oldNs;	/* Global namespace, the old way */
    const xmlChar  *version;	/* the XML version string */
    const xmlChar  *encoding;   /* external initial encoding, if any */
    void           *ids;        /* Hash table for ID attributes if any */
    void           *refs;       /* Hash table for IDREFs attributes if any */
    const xmlChar  *URL;	/* The URI for that document */
    int             charset;    /* Internal flag for charset handling,
				   actually an xmlCharEncoding */
    struct _xmlDict *dict;      /* dict used to allocate names or NULL */
    void           *psvi;	/* for type/PSVI information */
    int             parseFlags;	/* set of xmlParserOption used to parse the
				   document */
    int             properties;	/* set of xmlDocProperties for this document
				   set at the end of parsing */
};


typedef struct _xmlDOMWrapCtxt xmlDOMWrapCtxt;
typedef xmlDOMWrapCtxt *xmlDOMWrapCtxtPtr;

/**
 * xmlDOMWrapAcquireNsFunction:
 * @ctxt:  a DOM wrapper context
 * @node:  the context node (element or attribute)
 * @nsName:  the requested namespace name
 * @nsPrefix:  the requested namespace prefix
 *
 * A function called to acquire namespaces (xmlNs) from the wrapper.
 *
 * Returns an xmlNsPtr or NULL in case of an error.
 */
typedef xmlNsPtr (*xmlDOMWrapAcquireNsFunction) (xmlDOMWrapCtxtPtr ctxt,
						 lxb_dom_node_t_ptr node,
						 const xmlChar *nsName,
						 const xmlChar *nsPrefix);

/**
 * xmlDOMWrapCtxt:
 *
 * Context for DOM wrapper-operations.
 */
struct _xmlDOMWrapCtxt {
    void * _private;
    /*
    * The type of this context, just in case we need specialized
    * contexts in the future.
    */
    int type;
    /*
    * Internal namespace map used for various operations.
    */
    void * namespaceMap;
    /*
    * Use this one to acquire an xmlNsPtr intended for node->ns.
    * (Note that this is not intended for elem->nsDef).
    */
    xmlDOMWrapAcquireNsFunction getNsForNodeFunc;
};

/**
 * xmlChildrenNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children."
 */
#ifndef xmlChildrenNode
#define xmlChildrenNode children
#endif

/**
 * xmlRootNode:
 *
 * Macro for compatibility naming layer with libxml1. Maps
 * to "children".
 */
#ifndef xmlRootNode
#define xmlRootNode children
#endif

/*
 * Variables.
 */

/*
 * Some helper functions
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || \
    defined (LIBXML_HTML_ENABLED) || defined(LIBXML_SAX1_ENABLED) || \
    defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || \
    defined(LIBXML_LEGACY_ENABLED)
XMLPUBFUN int XMLCALL
		xmlValidateNCName	(const xmlChar *value,
					 int space);
#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL
		xmlValidateQName	(const xmlChar *value,
					 int space);
XMLPUBFUN int XMLCALL
		xmlValidateName		(const xmlChar *value,
					 int space);
XMLPUBFUN int XMLCALL
		xmlValidateNMToken	(const xmlChar *value,
					 int space);
#endif

XMLPUBFUN xmlChar * XMLCALL
		xmlBuildQName		(const xmlChar *ncname,
					 const xmlChar *prefix,
					 xmlChar *memory,
					 int len);
XMLPUBFUN xmlChar * XMLCALL
		xmlSplitQName2		(const xmlChar *name,
					 xmlChar **prefix);
XMLPUBFUN const xmlChar * XMLCALL
		xmlSplitQName3		(const xmlChar *name,
					 int *len);

/*
 * Handling Buffers, the old ones see @xmlBuf for the new ones.
 */

XMLPUBFUN void XMLCALL
		xmlSetBufferAllocationScheme(xmlBufferAllocationScheme scheme);
XMLPUBFUN xmlBufferAllocationScheme XMLCALL
		xmlGetBufferAllocationScheme(void);

XMLPUBFUN xmlBufferPtr XMLCALL
		xmlBufferCreate		(void);
XMLPUBFUN xmlBufferPtr XMLCALL
		xmlBufferCreateSize	(size_t size);
XMLPUBFUN xmlBufferPtr XMLCALL
		xmlBufferCreateStatic	(void *mem,
					 size_t size);
XMLPUBFUN int XMLCALL
		xmlBufferResize		(xmlBufferPtr buf,
					 unsigned int size);
XMLPUBFUN void XMLCALL
		xmlBufferFree		(xmlBufferPtr buf);
XMLPUBFUN int XMLCALL
		xmlBufferDump		(FILE *file,
					 xmlBufferPtr buf);
XMLPUBFUN int XMLCALL
		xmlBufferAdd		(xmlBufferPtr buf,
					 const xmlChar *str,
					 int len);
XMLPUBFUN int XMLCALL
		xmlBufferAddHead	(xmlBufferPtr buf,
					 const xmlChar *str,
					 int len);
XMLPUBFUN int XMLCALL
		xmlBufferCat		(xmlBufferPtr buf,
					 const xmlChar *str);
XMLPUBFUN int XMLCALL
		xmlBufferCCat		(xmlBufferPtr buf,
					 const char *str);
XMLPUBFUN int XMLCALL
		xmlBufferShrink		(xmlBufferPtr buf,
					 unsigned int len);
XMLPUBFUN int XMLCALL
		xmlBufferGrow		(xmlBufferPtr buf,
					 unsigned int len);
XMLPUBFUN void XMLCALL
		xmlBufferEmpty		(xmlBufferPtr buf);
XMLPUBFUN const xmlChar* XMLCALL
		xmlBufferContent	(const xmlBuffer *buf);
XMLPUBFUN xmlChar* XMLCALL
		xmlBufferDetach         (xmlBufferPtr buf);
XMLPUBFUN void XMLCALL
		xmlBufferSetAllocationScheme(xmlBufferPtr buf,
					 xmlBufferAllocationScheme scheme);
XMLPUBFUN int XMLCALL
		xmlBufferLength		(const xmlBuffer *buf);

/*
 * Creating/freeing new structures.
 */
XMLPUBFUN xmlDtdPtr XMLCALL
		xmlCreateIntSubset	(lxb_dom_document_t_ptr doc,
					 const xmlChar *name,
					 const xmlChar *ExternalID,
					 const xmlChar *SystemID);
XMLPUBFUN xmlDtdPtr XMLCALL
		xmlNewDtd		(lxb_dom_document_t_ptr doc,
					 const xmlChar *name,
					 const xmlChar *ExternalID,
					 const xmlChar *SystemID);
XMLPUBFUN xmlDtdPtr XMLCALL
		xmlGetIntSubset		(const xmlDoc *doc);
XMLPUBFUN void XMLCALL
		xmlFreeDtd		(xmlDtdPtr cur);
#ifdef LIBXML_LEGACY_ENABLED
XML_DEPRECATED
XMLPUBFUN xmlNsPtr XMLCALL
		xmlNewGlobalNs		(lxb_dom_document_t_ptr doc,
					 const xmlChar *href,
					 const xmlChar *prefix);
#endif /* LIBXML_LEGACY_ENABLED */
XMLPUBFUN xmlNsPtr XMLCALL
		xmlNewNs		(lxb_dom_node_t_ptr node,
					 const xmlChar *href,
					 const xmlChar *prefix);
XMLPUBFUN void XMLCALL
		xmlFreeNs		(xmlNsPtr cur);
XMLPUBFUN void XMLCALL
		xmlFreeNsList		(xmlNsPtr cur);
XMLPUBFUN lxb_dom_document_t_ptr XMLCALL
		xmlNewDoc		(const xmlChar *version);
XMLPUBFUN void XMLCALL
		xmlFreeDoc		(lxb_dom_document_t_ptr cur);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlNewDocProp		(lxb_dom_document_t_ptr doc,
					 const xmlChar *name,
					 const xmlChar *value);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlNewProp		(lxb_dom_node_t_ptr node,
					 const xmlChar *name,
					 const xmlChar *value);
#endif
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlNewNsProp		(lxb_dom_node_t_ptr node,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *value);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlNewNsPropEatName	(lxb_dom_node_t_ptr node,
					 xmlNsPtr ns,
					 xmlChar *name,
					 const xmlChar *value);
XMLPUBFUN void XMLCALL
		xmlFreePropList		(lxb_dom_attr_t_ptr cur);
XMLPUBFUN void XMLCALL
		xmlFreeProp		(lxb_dom_attr_t_ptr cur);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlCopyProp		(lxb_dom_node_t_ptr target,
					 lxb_dom_attr_t_ptr cur);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlCopyPropList		(lxb_dom_node_t_ptr target,
					 lxb_dom_attr_t_ptr cur);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlDtdPtr XMLCALL
		xmlCopyDtd		(xmlDtdPtr dtd);
#endif /* LIBXML_TREE_ENABLED */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN lxb_dom_document_t_ptr XMLCALL
		xmlCopyDoc		(lxb_dom_document_t_ptr doc,
					 int recursive);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
/*
 * Creating new nodes.
 */
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocNode		(lxb_dom_document_t_ptr doc,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocNodeEatName	(lxb_dom_document_t_ptr doc,
					 xmlNsPtr ns,
					 xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewNode		(xmlNsPtr ns,
					 const xmlChar *name);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewNodeEatName	(xmlNsPtr ns,
					 xmlChar *name);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewChild		(lxb_dom_node_t_ptr parent,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *content);
#endif
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocText		(const xmlDoc *doc,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewText		(const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocPI		(lxb_dom_document_t_ptr doc,
					 const xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewPI		(const xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocTextLen	(lxb_dom_document_t_ptr doc,
					 const xmlChar *content,
					 int len);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewTextLen		(const xmlChar *content,
					 int len);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocComment	(lxb_dom_document_t_ptr doc,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewComment		(const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewCDataBlock	(lxb_dom_document_t_ptr doc,
					 const xmlChar *content,
					 int len);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewCharRef		(lxb_dom_document_t_ptr doc,
					 const xmlChar *name);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewReference		(const xmlDoc *doc,
					 const xmlChar *name);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlCopyNode		(lxb_dom_node_t_ptr node,
					 int recursive);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlDocCopyNode		(lxb_dom_node_t_ptr node,
					 lxb_dom_document_t_ptr doc,
					 int recursive);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlDocCopyNodeList	(lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr node);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlCopyNodeList		(lxb_dom_node_t_ptr node);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewTextChild		(lxb_dom_node_t_ptr parent,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocRawNode	(lxb_dom_document_t_ptr doc,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *content);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlNewDocFragment	(lxb_dom_document_t_ptr doc);
#endif /* LIBXML_TREE_ENABLED */

/*
 * Navigating.
 */
XMLPUBFUN long XMLCALL
		xmlGetLineNo		(const xmlNode *node);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
XMLPUBFUN xmlChar * XMLCALL
		xmlGetNodePath		(const xmlNode *node);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED) */
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlDocGetRootElement	(const lxb_dom_document_t *doc);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlGetLastChild		(const xmlNode *parent);
XMLPUBFUN int XMLCALL
		xmlNodeIsText		(const xmlNode *node);
XMLPUBFUN int XMLCALL
		xmlIsBlankNode		(const xmlNode *node);

/*
 * Changing the structure.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlDocSetRootElement	(lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr root);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetName		(lxb_dom_node_t_ptr cur,
					 const xmlChar *name);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlAddChild		(lxb_dom_node_t_ptr parent,
					 lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlAddChildList		(lxb_dom_node_t_ptr parent,
					 lxb_dom_node_t_ptr cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlReplaceNode		(lxb_dom_node_t_ptr old,
					 lxb_dom_node_t_ptr cur);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlAddPrevSibling	(lxb_dom_node_t_ptr cur,
					 lxb_dom_node_t_ptr elem);
#endif /* LIBXML_TREE_ENABLED || LIBXML_HTML_ENABLED || LIBXML_SCHEMAS_ENABLED */
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlAddSibling		(lxb_dom_node_t_ptr cur,
					 lxb_dom_node_t_ptr elem);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlAddNextSibling	(lxb_dom_node_t_ptr cur,
					 lxb_dom_node_t_ptr elem);
XMLPUBFUN void XMLCALL
		xmlUnlinkNode		(lxb_dom_node_t_ptr cur);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlTextMerge		(lxb_dom_node_t_ptr first,
					 lxb_dom_node_t_ptr second);
XMLPUBFUN int XMLCALL
		xmlTextConcat		(lxb_dom_node_t_ptr node,
					 const xmlChar *content,
					 int len);
XMLPUBFUN void XMLCALL
		xmlFreeNodeList		(lxb_dom_node_t_ptr cur);
XMLPUBFUN void XMLCALL
		xmlFreeNode		(lxb_dom_node_t_ptr cur);
XMLPUBFUN void XMLCALL
		xmlSetTreeDoc		(lxb_dom_node_t_ptr tree,
					 lxb_dom_document_t_ptr doc);
XMLPUBFUN void XMLCALL
		xmlSetListDoc		(lxb_dom_node_t_ptr list,
					 lxb_dom_document_t_ptr doc);
/*
 * Namespaces.
 */
XMLPUBFUN xmlNsPtr XMLCALL
		xmlSearchNs		(lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr node,
					 const xmlChar *nameSpace);
XMLPUBFUN xmlNsPtr XMLCALL
		xmlSearchNsByHref	(lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr node,
					 const xmlChar *href);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN xmlNsPtr * XMLCALL
		xmlGetNsList		(const xmlDoc *doc,
					 const xmlNode *node);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) */

XMLPUBFUN void XMLCALL
		xmlSetNs		(lxb_dom_node_t_ptr node,
					 xmlNsPtr ns);
XMLPUBFUN xmlNsPtr XMLCALL
		xmlCopyNamespace	(xmlNsPtr cur);
XMLPUBFUN xmlNsPtr XMLCALL
		xmlCopyNamespaceList	(xmlNsPtr cur);

/*
 * Changing the content.
 */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || \
    defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlSetProp		(lxb_dom_node_t_ptr node,
					 const xmlChar *name,
					 const xmlChar *value);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlSetNsProp		(lxb_dom_node_t_ptr node,
					 xmlNsPtr ns,
					 const xmlChar *name,
					 const xmlChar *value);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || \
	  defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED) */
XMLPUBFUN xmlChar * XMLCALL
		xmlGetNoNsProp		(const xmlNode *node,
					 const xmlChar *name);
XMLPUBFUN xmlChar * XMLCALL
		xmlGetProp		(const xmlNode *node,
					 const xmlChar *name);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlHasProp		(const xmlNode *node,
					 const xmlChar *name);
XMLPUBFUN lxb_dom_attr_t_ptr XMLCALL
		xmlHasNsProp		(const xmlNode *node,
					 const xmlChar *name,
					 const xmlChar *nameSpace);
XMLPUBFUN xmlChar * XMLCALL
		xmlGetNsProp		(const xmlNode *node,
					 const xmlChar *name,
					 const xmlChar *nameSpace);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlStringGetNodeList	(const xmlDoc *doc,
					 const xmlChar *value);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
		xmlStringLenGetNodeList	(const xmlDoc *doc,
					 const xmlChar *value,
					 int len);
XMLPUBFUN xmlChar * XMLCALL
		xmlNodeListGetString	(lxb_dom_document_t_ptr doc,
					 const xmlNode *list,
					 int inLine);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN xmlChar * XMLCALL
		xmlNodeListGetRawString	(const xmlDoc *doc,
					 const xmlNode *list,
					 int inLine);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlNodeSetContent	(lxb_dom_node_t_ptr cur,
					 const xmlChar *content);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetContentLen	(lxb_dom_node_t_ptr cur,
					 const xmlChar *content,
					 int len);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN void XMLCALL
		xmlNodeAddContent	(lxb_dom_node_t_ptr cur,
					 const xmlChar *content);
XMLPUBFUN void XMLCALL
		xmlNodeAddContentLen	(lxb_dom_node_t_ptr cur,
					 const xmlChar *content,
					 int len);
XMLPUBFUN xmlChar * XMLCALL
		xmlNodeGetContent	(const lxb_dom_node_t *cur);

XMLPUBFUN int XMLCALL
		xmlNodeBufGetContent	(xmlBufferPtr buffer,
					 const lxb_dom_node_t *cur);
XMLPUBFUN int XMLCALL
		xmlBufGetNodeContent	(xmlBufPtr buf,
					 const lxb_dom_node_t *cur);

XMLPUBFUN xmlChar * XMLCALL
		xmlNodeGetLang		(const lxb_dom_node_t *cur);
XMLPUBFUN int XMLCALL
		xmlNodeGetSpacePreserve	(const lxb_dom_node_t *cur);
#ifdef LIBXML_TREE_ENABLED
XMLPUBFUN void XMLCALL
		xmlNodeSetLang		(lxb_dom_node_t_ptr cur,
					 const xmlChar *lang);
XMLPUBFUN void XMLCALL
		xmlNodeSetSpacePreserve (lxb_dom_node_t_ptr cur,
					 int val);
#endif /* LIBXML_TREE_ENABLED */
XMLPUBFUN xmlChar * XMLCALL
		xmlNodeGetBase		(const xmlDoc *doc,
					 const lxb_dom_node_t *cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
XMLPUBFUN void XMLCALL
		xmlNodeSetBase		(lxb_dom_node_t_ptr cur,
					 const xmlChar *uri);
#endif

/*
 * Removing content.
 */
XMLPUBFUN int XMLCALL
		xmlRemoveProp		(lxb_dom_attr_t_ptr cur);
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
XMLPUBFUN int XMLCALL
		xmlUnsetNsProp		(lxb_dom_node_t_ptr node,
					 xmlNsPtr ns,
					 const xmlChar *name);
XMLPUBFUN int XMLCALL
		xmlUnsetProp		(lxb_dom_node_t_ptr node,
					 const xmlChar *name);
#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */

/*
 * Internal, don't use.
 */
XMLPUBFUN void XMLCALL
		xmlBufferWriteCHAR	(xmlBufferPtr buf,
					 const xmlChar *string);
XMLPUBFUN void XMLCALL
		xmlBufferWriteChar	(xmlBufferPtr buf,
					 const char *string);
XMLPUBFUN void XMLCALL
		xmlBufferWriteQuotedString(xmlBufferPtr buf,
					 const xmlChar *string);

#ifdef LIBXML_OUTPUT_ENABLED
XMLPUBFUN void xmlAttrSerializeTxtContent(xmlBufferPtr buf,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_attr_t_ptr attr,
					 const xmlChar *string);
#endif /* LIBXML_OUTPUT_ENABLED */

#ifdef LIBXML_TREE_ENABLED
/*
 * Namespace handling.
 */
XMLPUBFUN int XMLCALL
		xmlReconciliateNs	(lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr tree);
#endif

#ifdef LIBXML_OUTPUT_ENABLED
/*
 * Saving.
 */
XMLPUBFUN void XMLCALL
		xmlDocDumpFormatMemory	(lxb_dom_document_t_ptr cur,
					 xmlChar **mem,
					 int *size,
					 int format);
XMLPUBFUN void XMLCALL
		xmlDocDumpMemory	(lxb_dom_document_t_ptr cur,
					 xmlChar **mem,
					 int *size);
XMLPUBFUN void XMLCALL
		xmlDocDumpMemoryEnc	(lxb_dom_document_t_ptr out_doc,
					 xmlChar **doc_txt_ptr,
					 int * doc_txt_len,
					 const char *txt_encoding);
XMLPUBFUN void XMLCALL
		xmlDocDumpFormatMemoryEnc(lxb_dom_document_t_ptr out_doc,
					 xmlChar **doc_txt_ptr,
					 int * doc_txt_len,
					 const char *txt_encoding,
					 int format);
XMLPUBFUN int XMLCALL
		xmlDocFormatDump	(FILE *f,
					 lxb_dom_document_t_ptr cur,
					 int format);
XMLPUBFUN int XMLCALL
		xmlDocDump		(FILE *f,
					 lxb_dom_document_t_ptr cur);
XMLPUBFUN void XMLCALL
		xmlElemDump		(FILE *f,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr cur);
XMLPUBFUN int XMLCALL
		xmlSaveFile		(const char *filename,
					 lxb_dom_document_t_ptr cur);
XMLPUBFUN int XMLCALL
		xmlSaveFormatFile	(const char *filename,
					 lxb_dom_document_t_ptr cur,
					 int format);
XMLPUBFUN size_t XMLCALL
		xmlBufNodeDump		(xmlBufPtr buf,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr cur,
					 int level,
					 int format);
XMLPUBFUN int XMLCALL
		xmlNodeDump		(xmlBufferPtr buf,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr cur,
					 int level,
					 int format);

XMLPUBFUN int XMLCALL
		xmlSaveFileTo		(xmlOutputBufferPtr buf,
					 lxb_dom_document_t_ptr cur,
					 const char *encoding);
XMLPUBFUN int XMLCALL
		xmlSaveFormatFileTo     (xmlOutputBufferPtr buf,
					 lxb_dom_document_t_ptr cur,
				         const char *encoding,
				         int format);
XMLPUBFUN void XMLCALL
		xmlNodeDumpOutput	(xmlOutputBufferPtr buf,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr cur,
					 int level,
					 int format,
					 const char *encoding);

XMLPUBFUN int XMLCALL
		xmlSaveFormatFileEnc    (const char *filename,
					 lxb_dom_document_t_ptr cur,
					 const char *encoding,
					 int format);

XMLPUBFUN int XMLCALL
		xmlSaveFileEnc		(const char *filename,
					 lxb_dom_document_t_ptr cur,
					 const char *encoding);

#endif /* LIBXML_OUTPUT_ENABLED */
/*
 * XHTML
 */
XMLPUBFUN int XMLCALL
		xmlIsXHTML		(const xmlChar *systemID,
					 const xmlChar *publicID);

/*
 * Compression.
 */
XMLPUBFUN int XMLCALL
		xmlGetDocCompressMode	(const xmlDoc *doc);
XMLPUBFUN void XMLCALL
		xmlSetDocCompressMode	(lxb_dom_document_t_ptr doc,
					 int mode);
XMLPUBFUN int XMLCALL
		xmlGetCompressMode	(void);
XMLPUBFUN void XMLCALL
		xmlSetCompressMode	(int mode);

/*
* DOM-wrapper helper functions.
*/
XMLPUBFUN xmlDOMWrapCtxtPtr XMLCALL
		xmlDOMWrapNewCtxt	(void);
XMLPUBFUN void XMLCALL
		xmlDOMWrapFreeCtxt	(xmlDOMWrapCtxtPtr ctxt);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapReconcileNamespaces(xmlDOMWrapCtxtPtr ctxt,
					 lxb_dom_node_t_ptr elem,
					 int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapAdoptNode		(xmlDOMWrapCtxtPtr ctxt,
					 lxb_dom_document_t_ptr sourceDoc,
					 lxb_dom_node_t_ptr node,
					 lxb_dom_document_t_ptr destDoc,
					 lxb_dom_node_t_ptr destParent,
					 int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapRemoveNode	(xmlDOMWrapCtxtPtr ctxt,
					 lxb_dom_document_t_ptr doc,
					 lxb_dom_node_t_ptr node,
					 int options);
XMLPUBFUN int XMLCALL
	    xmlDOMWrapCloneNode		(xmlDOMWrapCtxtPtr ctxt,
					 lxb_dom_document_t_ptr sourceDoc,
					 lxb_dom_node_t_ptr node,
					 lxb_dom_node_t_ptr *clonedNode,
					 lxb_dom_document_t_ptr destDoc,
					 lxb_dom_node_t_ptr destParent,
					 int deep,
					 int options);

#ifdef LIBXML_TREE_ENABLED
/*
 * 5 interfaces from DOM ElementTraversal, but different in entities
 * traversal.
 */
XMLPUBFUN unsigned long XMLCALL
            xmlChildElementCount        (lxb_dom_node_t_ptr parent);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
            xmlNextElementSibling       (lxb_dom_node_t_ptr node);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
            xmlFirstElementChild        (lxb_dom_node_t_ptr parent);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
            xmlLastElementChild         (lxb_dom_node_t_ptr parent);
XMLPUBFUN lxb_dom_node_t_ptr XMLCALL
            xmlPreviousElementSibling   (lxb_dom_node_t_ptr node);
#endif
#ifdef __cplusplus
}
#endif
#ifndef __XML_PARSER_H__
#include "xmlmemory.h"
#endif

#endif /* __XML_TREE_H__ */

