#include "nokolexbor.h"

VALUE cNokolexborDocumentFragment;
extern VALUE cNokolexborNode;
extern VALUE mNokolexbor;

/**
 * call-seq:
 *   new(document, tags = nil, ctx = nil) -> DocumentFragment
 *
 * Create a {DocumentFragment} from +tags+.
 *
 * If +ctx+ is present, it is used as a context node for the
 * subtree created.
 */
static VALUE
nl_document_fragment_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_document;
  VALUE rest;

  rb_scan_args(argc, argv, "1*", &rb_document, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  lxb_dom_document_fragment_t *node = lxb_dom_document_create_document_fragment(document);
  if (node == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating document fragment");
  }

  VALUE rb_node = nl_rb_node_create(&node->node, rb_document);

  rb_obj_call_init(rb_node, argc, argv);

  return rb_node;
}

void Init_nl_document_fragment(void)
{
  cNokolexborDocumentFragment = rb_define_class_under(mNokolexbor, "DocumentFragment", cNokolexborNode);

  rb_define_singleton_method(cNokolexborDocumentFragment, "new", nl_document_fragment_new, -1);
}
