/*
 * buf.c: memory buffers for libxml2
 *
 * new buffer structures and entry points to simplify the maintenance
 * of libxml2 and ensure we keep good control over memory allocations
 * and stay 64 bits clean.
 * The new entry point use the xmlBufPtr opaque structure and
 * xmlBuf...() counterparts to the old xmlBuf...() functions
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"

#include <string.h> /* for memset() only ! */
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#include "libxml/tree.h"
#include "libxml/globals.h"
#include "libxml/parserInternals.h" /* for XML_MAX_TEXT_LENGTH */

#include "private/buf.h"
#include "private/error.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

#define WITH_BUFFER_COMPAT

/**
 * xmlBuf:
 *
 * A buffer structure. The base of the structure is somehow compatible
 * with struct _xmlBuffer to limit risks on application which accessed
 * directly the input->buf->buffer structures.
 */

struct _xmlBuf {
    xmlChar *content;		/* The buffer content UTF8 */
    unsigned int compat_use;    /* for binary compatibility */
    unsigned int compat_size;   /* for binary compatibility */
    xmlBufferAllocationScheme alloc; /* The realloc method */
    xmlChar *contentIO;		/* in IO mode we may have a different base */
    size_t use;		        /* The buffer size used */
    size_t size;		/* The buffer size */
    xmlBufferPtr buffer;        /* wrapper for an old buffer */
    int error;                  /* an error code if a failure occurred */
};

#ifdef WITH_BUFFER_COMPAT
/*
 * Macro for compatibility with xmlBuffer to be used after an xmlBuf
 * is updated. This makes sure the compat fields are updated too.
 */
#define UPDATE_COMPAT(buf)				    \
     if (buf->size < INT_MAX) buf->compat_size = buf->size; \
     else buf->compat_size = INT_MAX;			    \
     if (buf->use < INT_MAX) buf->compat_use = buf->use; \
     else buf->compat_use = INT_MAX;

/*
 * Macro for compatibility with xmlBuffer to be used in all the xmlBuf
 * entry points, it checks that the compat fields have not been modified
 * by direct call to xmlBuffer function from code compiled before 2.9.0 .
 */
#define CHECK_COMPAT(buf)				    \
     if (buf->size != (size_t) buf->compat_size)	    \
         if (buf->compat_size < INT_MAX)		    \
	     buf->size = buf->compat_size;		    \
     if (buf->use != (size_t) buf->compat_use)		    \
         if (buf->compat_use < INT_MAX)			    \
	     buf->use = buf->compat_use;

#else /* ! WITH_BUFFER_COMPAT */
#define UPDATE_COMPAT(buf)
#define CHECK_COMPAT(buf)
#endif /* WITH_BUFFER_COMPAT */

/**
 * xmlBufMemoryError:
 * @extra:  extra information
 *
 * Handle an out of memory condition
 * To be improved...
 */
static void
xmlBufMemoryError(xmlBufPtr buf, const char *extra)
{
    __nl_xmlSimpleError(XML_FROM_BUFFER, XML_ERR_NO_MEMORY, NULL, NULL, extra);
    if ((buf) && (buf->error == 0))
        buf->error = XML_ERR_NO_MEMORY;
}

/**
 * xmlBufCreate:
 *
 * routine to create an XML buffer.
 * returns the new structure.
 */
xmlBufPtr
xmlBufCreate(void) {
    xmlBufPtr ret;

    ret = (xmlBufPtr) nl_xmlMalloc(sizeof(xmlBuf));
    if (ret == NULL) {
	xmlBufMemoryError(NULL, "creating buffer");
        return(NULL);
    }
    ret->use = 0;
    ret->error = 0;
    ret->buffer = NULL;
    ret->size = nl_xmlDefaultBufferSize;
    UPDATE_COMPAT(ret);
    ret->alloc = nl_xmlBufferAllocScheme;
    ret->content = (xmlChar *) nl_xmlMallocAtomic(ret->size);
    if (ret->content == NULL) {
	xmlBufMemoryError(ret, "creating buffer");
	nl_xmlFree(ret);
        return(NULL);
    }
    ret->content[0] = 0;
    ret->contentIO = NULL;
    return(ret);
}

/**
 * xmlBufFree:
 * @buf:  the buffer to free
 *
 * Frees an XML buffer. It frees both the content and the structure which
 * encapsulate it.
 */
void
xmlBufFree(xmlBufPtr buf) {
    if (buf == NULL) {
#ifdef DEBUG_BUFFER
        nl_xmlGenericError(nl_xmlGenericErrorContext,
		"xmlBufFree: buf == NULL\n");
#endif
	return;
    }

    if ((buf->alloc == XML_BUFFER_ALLOC_IO) &&
        (buf->contentIO != NULL)) {
        nl_xmlFree(buf->contentIO);
    } else if (buf->content != NULL) {
        nl_xmlFree(buf->content);
    }
    nl_xmlFree(buf);
}

/**
 * nl_xmlBufContent:
 * @buf:  the buffer
 *
 * Function to extract the content of a buffer
 *
 * Returns the internal content
 */

xmlChar *
nl_xmlBufContent(const xmlBuf *buf)
{
    if ((!buf) || (buf->error))
        return NULL;

    return(buf->content);
}

/**
 * xmlBufResize:
 * @buf:  the buffer to resize
 * @size:  the desired size
 *
 * Resize a buffer to accommodate minimum size of @size.
 *
 * Returns  0 in case of problems, 1 otherwise
 */
int
xmlBufResize(xmlBufPtr buf, size_t size)
{
    size_t newSize;
    xmlChar* rebuf = NULL;
    size_t start_buf;

    if ((buf == NULL) || (buf->error))
        return(0);
    CHECK_COMPAT(buf)

    if (buf->alloc == XML_BUFFER_ALLOC_BOUNDED) {
        /*
	 * Used to provide parsing limits
	 */
        if (size >= XML_MAX_TEXT_LENGTH) {
	    xmlBufMemoryError(buf, "buffer error: text too long\n");
	    return(0);
	}
    }

    /* Don't resize if we don't have to */
    if (size < buf->size)
        return 1;

    /* figure out new size */
    switch (buf->alloc){
	case XML_BUFFER_ALLOC_IO:
	case XML_BUFFER_ALLOC_DOUBLEIT:
	    /*take care of empty case*/
            if (buf->size == 0) {
                newSize = (size > SIZE_MAX - 10 ? SIZE_MAX : size + 10);
            } else {
                newSize = buf->size;
            }
	    while (size > newSize) {
	        if (newSize > SIZE_MAX / 2) {
	            xmlBufMemoryError(buf, "growing buffer");
	            return 0;
	        }
	        newSize *= 2;
	    }
	    break;
	case XML_BUFFER_ALLOC_EXACT:
            newSize = (size > SIZE_MAX - 10 ? SIZE_MAX : size + 10);
	    break;
        case XML_BUFFER_ALLOC_HYBRID:
            if (buf->use < BASE_BUFFER_SIZE)
                newSize = size;
            else {
                newSize = buf->size;
                while (size > newSize) {
                    if (newSize > SIZE_MAX / 2) {
                        xmlBufMemoryError(buf, "growing buffer");
                        return 0;
                    }
                    newSize *= 2;
                }
            }
            break;

	default:
            newSize = (size > SIZE_MAX - 10 ? SIZE_MAX : size + 10);
	    break;
    }

    if ((buf->alloc == XML_BUFFER_ALLOC_IO) && (buf->contentIO != NULL)) {
        start_buf = buf->content - buf->contentIO;

        if (start_buf > newSize) {
	    /* move data back to start */
	    memmove(buf->contentIO, buf->content, buf->use);
	    buf->content = buf->contentIO;
	    buf->content[buf->use] = 0;
	    buf->size += start_buf;
	} else {
	    rebuf = (xmlChar *) nl_xmlRealloc(buf->contentIO, start_buf + newSize);
	    if (rebuf == NULL) {
		xmlBufMemoryError(buf, "growing buffer");
		return 0;
	    }
	    buf->contentIO = rebuf;
	    buf->content = rebuf + start_buf;
	}
    } else {
	if (buf->content == NULL) {
	    rebuf = (xmlChar *) nl_xmlMallocAtomic(newSize);
	    buf->use = 0;
	    rebuf[buf->use] = 0;
	} else if (buf->size - buf->use < 100) {
	    rebuf = (xmlChar *) nl_xmlRealloc(buf->content, newSize);
        } else {
	    /*
	     * if we are reallocating a buffer far from being full, it's
	     * better to make a new allocation and copy only the used range
	     * and free the old one.
	     */
	    rebuf = (xmlChar *) nl_xmlMallocAtomic(newSize);
	    if (rebuf != NULL) {
		memcpy(rebuf, buf->content, buf->use);
		nl_xmlFree(buf->content);
		rebuf[buf->use] = 0;
	    }
	}
	if (rebuf == NULL) {
	    xmlBufMemoryError(buf, "growing buffer");
	    return 0;
	}
	buf->content = rebuf;
    }
    buf->size = newSize;
    UPDATE_COMPAT(buf)

    return 1;
}

/**
 * xmlBufAdd:
 * @buf:  the buffer to dump
 * @str:  the #xmlChar string
 * @len:  the number of #xmlChar to add
 *
 * Add a string range to an XML buffer. if len == -1, the length of
 * str is recomputed.
 *
 * Returns 0 successful, a positive error code number otherwise
 *         and -1 in case of internal or API error.
 */
int
xmlBufAdd(xmlBufPtr buf, const xmlChar *str, int len) {
    size_t needSize;

    if ((str == NULL) || (buf == NULL) || (buf->error))
	return -1;
    CHECK_COMPAT(buf)

    if (len < -1) {
#ifdef DEBUG_BUFFER
        nl_xmlGenericError(nl_xmlGenericErrorContext,
		"xmlBufAdd: len < 0\n");
#endif
	return -1;
    }
    if (len == 0) return 0;

    if (len < 0)
        len = nl_xmlStrlen(str);

    if (len < 0) return -1;
    if (len == 0) return 0;

    /* Note that both buf->size and buf->use can be zero here. */
    if ((size_t) len >= buf->size - buf->use) {
        if ((size_t) len >= SIZE_MAX - buf->use) {
            xmlBufMemoryError(buf, "growing buffer past SIZE_MAX");
            return(-1);
        }
        needSize = buf->use + len + 1;
	if (buf->alloc == XML_BUFFER_ALLOC_BOUNDED) {
	    /*
	     * Used to provide parsing limits
	     */
	    if (needSize >= XML_MAX_TEXT_LENGTH) {
		xmlBufMemoryError(buf, "buffer error: text too long\n");
		return(-1);
	    }
	}
        if (!xmlBufResize(buf, needSize)){
	    xmlBufMemoryError(buf, "growing buffer");
            return XML_ERR_NO_MEMORY;
        }
    }

    memmove(&buf->content[buf->use], str, len);
    buf->use += len;
    buf->content[buf->use] = 0;
    UPDATE_COMPAT(buf)
    return 0;
}
