#include "nokolexbor.h"

VALUE cNokolexborComment;
extern VALUE cNokolexborCharacterData;
extern VALUE mNokolexbor;

static VALUE
nl_comment_new(int argc, VALUE *argv, VALUE klass)
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

  lxb_dom_comment_t *element = lxb_dom_document_create_comment(document, (const lxb_char_t *)StringValueCStr(rb_content), RSTRING_LEN(rb_content));
  if (element == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating comment");
  }

  VALUE rb_node = nl_rb_node_create(&element->char_data.node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

void Init_nl_comment(void)
{
  cNokolexborComment = rb_define_class_under(mNokolexbor, "Comment", cNokolexborCharacterData);

  rb_define_singleton_method(cNokolexborComment, "new", nl_comment_new, -1);
}
