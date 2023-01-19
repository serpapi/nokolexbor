#include "nokolexbor.h"

VALUE cNokolexborText;
extern VALUE cNokolexborCharacterData;
extern VALUE mNokolexbor;

/**
 * call-seq:
 *   new(text, document) { |Text| ... } -> Text
 *
 * Create a new Text from +text+.
 */
static VALUE
nl_text_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_text;
  VALUE rb_document;
  VALUE rest;

  rb_scan_args(argc, argv, "2*", &rb_text, &rb_document, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  const char *c_text = StringValuePtr(rb_text);
  size_t text_len = RSTRING_LEN(rb_text);
  lxb_dom_text_t *element = lxb_dom_document_create_text_node(document, (const lxb_char_t *)c_text, text_len);
  if (element == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating text node");
  }

  VALUE rb_node = nl_rb_node_create(&element->char_data.node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

void Init_nl_text(void)
{
  cNokolexborText = rb_define_class_under(mNokolexbor, "Text", cNokolexborCharacterData);

  rb_define_singleton_method(cNokolexborText, "new", nl_text_new, -1);
}
