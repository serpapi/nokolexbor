#include "nokolexbor.h"

VALUE cNokolexborCData;
extern VALUE cNokolexborText;
extern VALUE mNokolexbor;

static VALUE
nl_cdata_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_content;
  VALUE rb_document;
  VALUE rest;

  rb_scan_args(argc, argv, "2*", &rb_content, &rb_document, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  const char* c_content = StringValuePtr(rb_content);
  size_t content_len = RSTRING_LEN(rb_content);
  lxb_dom_cdata_section_t *element = lxb_dom_document_create_cdata_section(document, (const lxb_char_t *)c_content, content_len);
  if (element == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating text node");
  }

  VALUE rb_node = nl_rb_node_create(&element->text.char_data.node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

void Init_nl_cdata(void)
{
  cNokolexborCData = rb_define_class_under(mNokolexbor, "CDATA", cNokolexborText);

  rb_define_singleton_method(cNokolexborCData, "new", nl_cdata_new, -1);
}
