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

  if (rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    document = nl_rb_document_unwrap(rb_document);
  } else if (rb_obj_is_kind_of(rb_document, cNokolexborNode)) {
    lxb_dom_node_t *node = nl_rb_node_unwrap(rb_document);
    document = node->owner_document;
    rb_document = nl_rb_document_get(rb_document);
  } else {
    rb_raise(rb_eArgError, "Expected a Document or Node, got %s", rb_class2name(CLASS_OF(rb_document)));
  }

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
