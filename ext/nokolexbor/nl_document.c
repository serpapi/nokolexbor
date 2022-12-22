#include "nokolexbor.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;
VALUE cNokolexborDocument;

static void
free_nl_document(lxb_html_document_t *document)
{
  lxb_html_document_destroy(document);
}

const rb_data_type_t nl_document_type = {
    "Nokolexbor::Document",
    {
        0,
        free_nl_document,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

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

  lxb_html_document_t *document;

  document = lxb_html_document_create();
  if (document == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating document");
  }

  lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t *)html_c, html_len);
  if (status != LXB_STATUS_OK) {
    nl_raise_lexbor_error(status);
  }

  return TypedData_Wrap_Struct(cNokolexborDocument, &nl_document_type, document);
}

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

void Init_nl_document(void)
{
  cNokolexborDocument = rb_define_class_under(mNokolexbor, "Document", cNokolexborNode);
  rb_define_singleton_method(cNokolexborDocument, "new", nl_document_new, 0);
  rb_define_singleton_method(cNokolexborDocument, "parse", nl_document_parse, 1);
}
