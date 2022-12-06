#include "nokolexbor.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;
VALUE cNokolexborDocument;

static void
free_nl_document(nl_document_t *nl_document)
{
  lxb_html_document_destroy(nl_document->document);
  lexbor_free(nl_document);
}

const rb_data_type_t nl_document_type = {
    "Document",
    {
        0,
        free_nl_document,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE
nl_document_parse(VALUE self, VALUE rb_html)
{
  const char *html_c = StringValuePtr(rb_html);
  int html_len = RSTRING_LEN(rb_html);

  lxb_html_document_t *document;

  document = lxb_html_document_create();
  if (document == NULL)
  {
    return Qnil;
  }

  lxb_status_t status = lxb_html_document_parse(document, html_c, html_len);
  if (status != LXB_STATUS_OK)
  {
    return Qnil;
  }

  nl_document_t *nl_document = lexbor_malloc(sizeof(nl_document_t));
  VALUE rb_document = TypedData_Wrap_Struct(cNokolexborDocument, &nl_document_type, nl_document);

  nl_document->nl_node.node = &document->dom_document.node;
  nl_document->nl_node.rb_document = rb_document;
  nl_document->document = document;
  nl_document->rb_document = rb_document;

  return rb_document;
}

void Init_nl_document(void)
{
  cNokolexborDocument = rb_define_class_under(mNokolexbor, "Document", cNokolexborNode);
  rb_define_singleton_method(cNokolexborDocument, "parse", nl_document_parse, 1);
}
