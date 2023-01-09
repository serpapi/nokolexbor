#include "nokolexbor.h"

VALUE cNokolexborProcessingInstruction;
extern VALUE cNokolexborNode;
extern VALUE mNokolexbor;

static VALUE
nl_processing_instruction_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_name;
  VALUE rb_content;
  VALUE rb_document;
  VALUE rest;

  rb_scan_args(argc, argv, "3*", &rb_name, &rb_content, &rb_document, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  const char* c_name = StringValuePtr(rb_name);
  size_t name_len = RSTRING_LEN(rb_name);
  const char* c_content = StringValuePtr(rb_content);
  size_t content_len = RSTRING_LEN(rb_content);
  lxb_dom_processing_instruction_t * node = lxb_dom_document_create_processing_instruction(document, (const lxb_char_t *)c_name, name_len, (const lxb_char_t *)c_content, content_len);
  if (node == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating processing instruction");
  }

  VALUE rb_node = nl_rb_node_create(&node->char_data.node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

void Init_nl_processing_instruction(void)
{
  cNokolexborProcessingInstruction = rb_define_class_under(mNokolexbor, "ProcessingInstruction", cNokolexborNode);

  rb_define_singleton_method(cNokolexborProcessingInstruction, "new", nl_processing_instruction_new, -1);
}
