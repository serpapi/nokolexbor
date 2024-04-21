#include "nokolexbor.h"
#include "config.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;
VALUE cNokolexborDocument;

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
pthread_key_t p_key_parser;
#endif

static void
free_nl_document(lxb_html_document_t *document)
{
  lxb_html_document_destroy(document);
}

const rb_data_type_t nl_document_type = {
    "Nokolexbor::Document",
    {
        0,
        (RUBY_DATA_FUNC)free_nl_document,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

/**
 * call-seq:
 *   parse(string_or_io) -> Document
 *
 * Parse HTML into a {Document}.
 *
 * @param string_or_io [String, #read]
 *   The HTML to be parsed. It may be a String, or any object that
 *   responds to #read such as an IO, or StringIO.
 */
static VALUE
nl_document_parse(VALUE self, VALUE rb_string_or_io)
{
  VALUE id_read = rb_intern("read");
  VALUE rb_html;
  if (rb_respond_to(rb_string_or_io, id_read)) {
    rb_html = rb_funcall(rb_string_or_io, id_read, 0);
  } else {
    rb_html = rb_string_or_io;
  }
  const char *html_c = StringValuePtr(rb_html);
  size_t html_len = RSTRING_LEN(rb_html);

#ifdef HAVE_PTHREAD_H
  lxb_html_parser_t *g_parser = (lxb_html_parser_t *)pthread_getspecific(p_key_parser);
#else
  lxb_html_parser_t *g_parser = NULL;
#endif
  if (g_parser == NULL) {
    g_parser = lxb_html_parser_create();
    lxb_status_t status = lxb_html_parser_init(g_parser);
    if (status != LXB_STATUS_OK) {
      nl_raise_lexbor_error(status);
    }
    g_parser->tree->scripting = true;
#ifdef HAVE_PTHREAD_H
    pthread_setspecific(p_key_parser, g_parser);
#endif
  }

  lxb_html_document_t *document = lxb_html_parse(g_parser, (const lxb_char_t *)html_c, html_len);

  if (document == NULL) {
    rb_raise(rb_eRuntimeError, "Error parsing document");
  }

  return TypedData_Wrap_Struct(cNokolexborDocument, &nl_document_type, document);
}

/**
 * Create a new document.
 *
 * @return [Document]
 */
static VALUE
nl_document_new(VALUE self)
{
  return nl_document_parse(self, rb_str_new("", 0));
}

lxb_dom_document_t *
nl_rb_document_unwrap(VALUE rb_doc)
{
  lxb_dom_document_t *doc;
  TypedData_Get_Struct(rb_doc, lxb_dom_document_t, &nl_document_type, doc);
  return doc;
}

/**
 * Get the title of this document.
 *
 * @return [String]
 */
static VALUE
nl_document_get_title(VALUE self)
{
  size_t len;
  lxb_char_t *str = lxb_html_document_title(nl_rb_document_unwrap(self), &len);
  return str == NULL ? rb_str_new("", 0) : rb_utf8_str_new(str, len);
}

/**
 * call-seq:
 *   title=(text) -> String
 *
 * Set the title of this document.
 *
 * If a title element is already present, its content is replaced
 * with the given text.
 *
 * Otherwise, this method tries to create one inside <head>.
 *
 * @return [String]
 */
static VALUE
nl_document_set_title(VALUE self, VALUE rb_title)
{
  const char *c_title = StringValuePtr(rb_title);
  size_t len = RSTRING_LEN(rb_title);
  lxb_html_document_title_set(nl_rb_document_unwrap(self), (const lxb_char_t *)c_title, len);
  return rb_title;
}

/**
 * Get the root node for this document.
 *
 * @return [Node]
 */
static VALUE
nl_document_root(VALUE self)
{
  lxb_dom_document_t *doc = nl_rb_document_unwrap(self);
  return nl_rb_node_create(lxb_dom_document_root(doc), self);
}

static void
free_parser(void *data)
{
  lxb_html_parser_t *g_parser = (lxb_html_parser_t *)data;
  if (g_parser != NULL) {
    g_parser = lxb_html_parser_destroy(g_parser);
  }
}

void Init_nl_document(void)
{
#ifdef HAVE_PTHREAD_H
  pthread_key_create(&p_key_parser, free_parser);
#endif

  cNokolexborDocument = rb_define_class_under(mNokolexbor, "Document", cNokolexborNode);
  rb_define_singleton_method(cNokolexborDocument, "new", nl_document_new, 0);
  rb_define_singleton_method(cNokolexborDocument, "parse", nl_document_parse, 1);
  rb_define_method(cNokolexborDocument, "title", nl_document_get_title, 0);
  rb_define_method(cNokolexborDocument, "title=", nl_document_set_title, 1);
  rb_define_method(cNokolexborDocument, "root", nl_document_root, 0);
}
