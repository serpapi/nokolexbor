#include "nokolexbor.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;
VALUE cNokolexborDocument;
extern rb_data_type_t nl_node_type;

extern VALUE nl_node_css(VALUE self, VALUE selector);
extern VALUE nl_node_at_css(VALUE self, VALUE selector);

static void
free_nl_document(lxb_html_document_t *document)
{
  lxb_html_document_destroy(document);
}

static const rb_data_type_t nl_document_type = {
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

  return TypedData_Wrap_Struct(cNokolexborDocument, &nl_document_type, document);
}

static VALUE
nl_document_at_css(VALUE self, VALUE selector)
{
  lxb_html_document_t *document;
  TypedData_Get_Struct(self, struct lxb_html_document_t, &nl_document_type, document);

  lxb_dom_node_t *body = lxb_dom_interface_node(lxb_html_document_body_element(document));

  VALUE rb_node = nl_rb_node_create(body, self);
  return nl_node_at_css(rb_node, selector);
}

static VALUE
nl_document_css(VALUE self, VALUE selector)
{
  lxb_html_document_t *document;
  TypedData_Get_Struct(self, struct lxb_html_document_t, &nl_document_type, document);

  lxb_dom_node_t *body = lxb_dom_interface_node(lxb_html_document_body_element(document));

  VALUE rb_node = nl_rb_node_create(body, self);
  return nl_node_css(rb_node, selector);
}

void Init_nl_document(void)
{
  cNokolexborDocument = rb_define_class_under(mNokolexbor, "Document", rb_cObject);
  rb_define_singleton_method(cNokolexborDocument, "parse", nl_document_parse, 1);
  rb_define_method(cNokolexborDocument, "at_css", nl_document_at_css, 1);
  rb_define_method(cNokolexborDocument, "css", nl_document_css, 1);
}
