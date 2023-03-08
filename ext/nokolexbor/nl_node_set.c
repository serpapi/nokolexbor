#include "nokolexbor.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;
VALUE cNokolexborNodeSet;
extern rb_data_type_t nl_document_type;

lxb_status_t nl_node_find(VALUE self, VALUE selector, lxb_selectors_cb_f cb, void *ctx);
void nl_sort_nodes_if_necessary(VALUE selector, lxb_dom_document_t *doc, lexbor_array_t *array);
lxb_status_t nl_node_at_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx);
lxb_status_t nl_node_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx);

lxb_status_t
lexbor_array_push_unique(lexbor_array_t *array, void *value)
{
  for (size_t i = 0; i < array->length; i++)
    if (array->list[i] == value)
      return LXB_STATUS_STOPPED;

  return lexbor_array_push(array, value);
}

static void
free_nl_node_set(lexbor_array_t *array)
{
  lexbor_array_destroy(array, true);
}

const rb_data_type_t nl_node_set_type = {
    "Nokolexbor::NodeSet",
    {
        0,
        free_nl_node_set,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

lexbor_array_t *
nl_rb_node_set_unwrap(VALUE rb_node_set)
{
  lexbor_array_t *array;
  TypedData_Get_Struct(rb_node_set, lexbor_array_t, &nl_node_set_type, array);
  return array;
}

static VALUE
nl_node_set_allocate(VALUE klass)
{
  lexbor_array_t *array = lexbor_array_create();
  return TypedData_Wrap_Struct(cNokolexborNodeSet, &nl_node_set_type, array);
}

VALUE
nl_rb_node_set_create_with_data(lexbor_array_t *array, VALUE rb_document)
{
  if (array == NULL) {
    array = lexbor_array_create();
  }
  VALUE ret = TypedData_Wrap_Struct(cNokolexborNodeSet, &nl_node_set_type, array);
  rb_iv_set(ret, "@document", rb_document);
  return ret;
}

/**
 * Get the length of this NodeSet.
 *
 * @return [Integer]
 */
static VALUE
nl_node_set_length(VALUE self)
{
  return INT2NUM(nl_rb_node_set_unwrap(self)->length);
}

/**
 * call-seq:
 *   push(node)
 *
 * Append +node+ to the NodeSet.
 *
 * @param node [Node]
 *
 * @return [NodeSet] +self+, to support chaining of calls.
 */
static VALUE
nl_node_set_push(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  lxb_status_t status = lexbor_array_push_unique(array, node);
  if (status != LXB_STATUS_OK && status != LXB_STATUS_STOPPED) {
    nl_raise_lexbor_error(status);
  }

  return self;
}

/**
 *  call-seq:
 *    delete(node)
 *
 * Delete +node+ from the NodeSet.
 *
 * @param node [Node]
 *
 * @return [Node,nil] The deleted node if found, otherwise returns nil.
 */
static VALUE
nl_node_set_delete(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  size_t i;
  for (i = 0; i < array->length; i++)
    if (array->list[i] == node) {
      break;
    }

  if (i >= array->length) {
    // not found
    return Qnil;
  }
  lexbor_array_delete(array, i, 1);
  return rb_node;
}

/**
 * call-seq:
 *   include?(node)
 *
 * @return true if any member of this NodeSet equals +node+.
 */
static VALUE
nl_node_set_is_include(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  for (size_t i = 0; i < array->length; i++)
    if (array->list[i] == node) {
      return Qtrue;
    }

  return Qfalse;
}

static VALUE
nl_node_set_index_at(VALUE self, long offset)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  if (offset >= (long)array->length || abs((int)offset) > (long)array->length) {
    return Qnil;
  }

  if (offset < 0) {
    offset += array->length;
  }

  lxb_dom_node_t *node = lexbor_array_get(array, offset);
  return nl_rb_node_create(node, nl_rb_document_get(self));
}

static VALUE
nl_node_set_subseq(VALUE self, long beg, long len)
{
  lexbor_array_t *old_array = nl_rb_node_set_unwrap(self);

  if (beg > (long)old_array->length) {
    return Qnil;
  }
  if (beg < 0 || len < 0) {
    return Qnil;
  }

  if ((beg + len) > (long)old_array->length) {
    len = old_array->length - beg;
  }

  lexbor_array_t *new_array = lexbor_array_create();
  if (len > 0) {
    lxb_status_t status = lexbor_array_init(new_array, len);
    if (status != LXB_STATUS_OK) {
      nl_raise_lexbor_error(status);
    }
  }

  for (long j = beg; j < beg + len; ++j) {
    lxb_status_t status = lexbor_array_push(new_array, old_array->list[j]);
    if (status != LXB_STATUS_OK) {
      nl_raise_lexbor_error(status);
    }
  }
  return nl_rb_node_set_create_with_data(new_array, nl_rb_document_get(self));
}

/**
 * call-seq:
 *   [](index) -> Node,nil
 *   [](start, length) -> NodeSet,nil
 *   [](range) -> NodeSet,nil
 *
 * @return [Node,NodeSet,nil] the {Node} at +index+, or returns a {NodeSet}
 *   containing nodes starting at +start+ and continuing for +length+ elements, or
 *   returns a {NodeSet} containing nodes specified by +range+. Negative +indices+
 *   count backward from the end of the +node_set+ (-1 is the last node). Returns
 *   nil if the +index+ (or +start+) are out of range.
 */
static VALUE
nl_node_set_slice(int argc, VALUE *argv, VALUE self)
{
  VALUE arg;
  long beg, len;

  lexbor_array_t *array = nl_rb_node_set_unwrap(self);

  if (argc == 2) {
    beg = NUM2LONG(argv[0]);
    len = NUM2LONG(argv[1]);
    if (beg < 0) {
      beg += array->length;
    }
    return nl_node_set_subseq(self, beg, len);
  }

  if (argc != 1) {
    rb_scan_args(argc, argv, "11", NULL, NULL);
  }
  arg = argv[0];

  if (FIXNUM_P(arg)) {
    return nl_node_set_index_at(self, FIX2LONG(arg));
  }

  /* if arg is Range */
  switch (rb_range_beg_len(arg, &beg, &len, array->length, 0)) {
  case Qfalse:
    break;
  case Qnil:
    return Qnil;
  default:
    return nl_node_set_subseq(self, beg, len);
  }

  return nl_node_set_index_at(self, NUM2LONG(arg));
}

/**
 * @return [Array<Node>] This list as an Array
 */
static VALUE
nl_node_set_to_array(VALUE self)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);

  VALUE list = rb_ary_new2(array->length);
  VALUE doc = nl_rb_document_get(self);
  for (size_t i = 0; i < array->length; i++) {
    lxb_dom_node_t *node = (lxb_dom_node_t *)array->list[i];
    VALUE rb_node = nl_rb_node_create(node, doc);
    rb_ary_push(list, rb_node);
  }

  return list;
}

/**
 * @return [NodeSet] A new set built by merging the +other+ set, excluding duplicates.
 */
static VALUE
nl_node_set_union(VALUE self, VALUE other)
{
  if (!rb_obj_is_kind_of(other, cNokolexborNodeSet)) {
    rb_raise(rb_eArgError, "Parameter must be a Nokolexbor::NodeSet");
  }

  lexbor_array_t *self_array = nl_rb_node_set_unwrap(self);
  lexbor_array_t *other_array = nl_rb_node_set_unwrap(other);

  if (self_array->length + other_array->length == 0) {
    return nl_rb_node_set_create_with_data(NULL, nl_rb_document_get(self));
  }

  lexbor_array_t *new_array = lexbor_array_create();
  lxb_status_t status = lexbor_array_init(new_array, self_array->length + other_array->length);
  if (status != LXB_STATUS_OK) {
    nl_raise_lexbor_error(status);
  }

  memcpy(new_array->list, self_array->list, sizeof(lxb_dom_node_t *) * self_array->length);
  new_array->length = self_array->length;

  for (size_t i = 0; i < other_array->length; i++) {
    lexbor_array_push_unique(new_array, other_array->list[i]);
  }

  return nl_rb_node_set_create_with_data(new_array, nl_rb_document_get(self));
}

/**
 * @return [NodeSet] A new NodeSet with the common nodes only.
 */
static VALUE
nl_node_set_intersection(VALUE self, VALUE other)
{
  if (!rb_obj_is_kind_of(other, cNokolexborNodeSet)) {
    rb_raise(rb_eArgError, "Parameter must be a Nokolexbor::NodeSet");
  }

  lexbor_array_t *self_array = nl_rb_node_set_unwrap(self);
  lexbor_array_t *other_array = nl_rb_node_set_unwrap(other);

  lexbor_array_t *new_array = lexbor_array_create();

  for (size_t i = 0; i < self_array->length; i++) {
    for (size_t j = 0; j < other_array->length; j++) {
      if (self_array->list[i] == other_array->list[j]) {
        lexbor_array_push(new_array, self_array->list[i]);
        break;
      }
    }
  }

  return nl_rb_node_set_create_with_data(new_array, nl_rb_document_get(self));
}

/**
 * @return [NodeSet] A new NodeSet with the nodes in this NodeSet that aren't in +other+
 */
static VALUE
nl_node_set_difference(VALUE self, VALUE other)
{
  if (!rb_obj_is_kind_of(other, cNokolexborNodeSet)) {
    rb_raise(rb_eArgError, "Parameter must be a Nokolexbor::NodeSet");
  }

  lexbor_array_t *self_array = nl_rb_node_set_unwrap(self);
  lexbor_array_t *other_array = nl_rb_node_set_unwrap(other);

  lexbor_array_t *new_array = lexbor_array_create();

  for (size_t i = 0; i < self_array->length; i++) {
    bool found = false;
    for (size_t j = 0; j < other_array->length; j++) {
      if (self_array->list[i] == other_array->list[j]) {
        found = true;
        break;
      }
    }
    if (!found) {
      lexbor_array_push(new_array, self_array->list[i]);
    }
  }

  return nl_rb_node_set_create_with_data(new_array, nl_rb_document_get(self));
}

static lxb_status_t
nl_node_set_find(VALUE self, VALUE selector, lxb_selectors_cb_f cb, void *ctx)
{
  lxb_dom_document_t *doc = nl_rb_document_unwrap(nl_rb_document_get(self));
  if (doc == NULL) {
    rb_raise(rb_eRuntimeError, "Error getting document");
  }
  // Wrap direct children with a temporary fragment so that they can be searched
  lxb_dom_document_fragment_t *frag = lxb_dom_document_fragment_interface_create(doc);
  if (frag == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating document fragment");
  }
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);

  lexbor_array_t *backup_array = lexbor_array_create();
  if (array->length > 0) {
    lxb_status_t status = lexbor_array_init(backup_array, array->length);
    if (status != LXB_STATUS_OK) {
      nl_raise_lexbor_error(status);
    }
  }
  // Backup original node data and re-group them into a fragment
  for (size_t i = 0; i < array->length; i++) {
    lxb_dom_node_t *node = (lxb_dom_node_t *)array->list[i];
    lxb_dom_node_t *backup_node = malloc(sizeof(lxb_dom_node_t));
    if (backup_node == NULL) {
      nl_raise_lexbor_error(LXB_STATUS_ERROR_MEMORY_ALLOCATION);
    }
    memcpy(backup_node, node, sizeof(lxb_dom_node_t));
    lxb_status_t status = lexbor_array_push(backup_array, backup_node);
    if (status != LXB_STATUS_OK) {
      nl_raise_lexbor_error(LXB_STATUS_ERROR_MEMORY_ALLOCATION);
    }
    lxb_dom_node_insert_child(&frag->node, node);
  }
  VALUE rb_frag = nl_rb_node_create(&frag->node, nl_rb_document_get(self));

  lxb_status_t status = nl_node_find(rb_frag, selector, cb, ctx);

  lxb_dom_document_fragment_interface_destroy(frag);
  // Restore original node data
  for (size_t i = 0; i < array->length; i++) {
    memcpy(array->list[i], backup_array->list[i], sizeof(lxb_dom_node_t));
    free(backup_array->list[i]);
  }
  lexbor_array_destroy(backup_array, true);

  return status;
}

/**
 * (see Node#at_css)
 */
static VALUE
nl_node_set_at_css(VALUE self, VALUE selector)
{
  lexbor_array_t *array = lexbor_array_create();
  lxb_dom_document_t *doc = nl_rb_document_unwrap(nl_rb_document_get(self));

  lxb_status_t status = nl_node_set_find(self, selector, nl_node_at_css_callback, array);

  if (status != LXB_STATUS_OK) {
    lexbor_array_destroy(array, true);
    nl_raise_lexbor_error(status);
  }

  if (array->length == 0) {
    lexbor_array_destroy(array, true);
    return Qnil;
  }

  nl_sort_nodes_if_necessary(selector, doc, array);

  VALUE ret = nl_rb_node_create(array->list[0], nl_rb_document_get(self));

  lexbor_array_destroy(array, true);

  return ret;
}

/**
 * (see Node#css)
 */
static VALUE
nl_node_set_css(VALUE self, VALUE selector)
{
  lexbor_array_t *array = lexbor_array_create();
  lxb_dom_document_t *doc = nl_rb_document_unwrap(nl_rb_document_get(self));

  lxb_status_t status = nl_node_set_find(self, selector, nl_node_css_callback, array);
  if (status != LXB_STATUS_OK) {
    lexbor_array_destroy(array, true);
    nl_raise_lexbor_error(status);
  }

  nl_sort_nodes_if_necessary(selector, doc, array);

  return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));
}

void Init_nl_node_set(void)
{
  cNokolexborNodeSet = rb_define_class_under(mNokolexbor, "NodeSet", cNokolexborNode);

  rb_define_alloc_func(cNokolexborNodeSet, nl_node_set_allocate);

  rb_define_method(cNokolexborNodeSet, "length", nl_node_set_length, 0);
  rb_define_method(cNokolexborNodeSet, "[]", nl_node_set_slice, -1);
  rb_define_method(cNokolexborNodeSet, "push", nl_node_set_push, 1);
  rb_define_method(cNokolexborNodeSet, "|", nl_node_set_union, 1);
  rb_define_method(cNokolexborNodeSet, "&", nl_node_set_intersection, 1);
  rb_define_method(cNokolexborNodeSet, "-", nl_node_set_difference, 1);
  rb_define_method(cNokolexborNodeSet, "to_a", nl_node_set_to_array, 0);
  rb_define_method(cNokolexborNodeSet, "delete", nl_node_set_delete, 1);
  rb_define_method(cNokolexborNodeSet, "include?", nl_node_set_is_include, 1);
  rb_define_method(cNokolexborNodeSet, "at_css", nl_node_set_at_css, 1);
  rb_define_method(cNokolexborNodeSet, "css", nl_node_set_css, 1);

  rb_define_alias(cNokolexborNodeSet, "slice", "[]");
  rb_define_alias(cNokolexborNodeSet, "<<", "push");
  rb_define_alias(cNokolexborNodeSet, "size", "length");
  rb_define_alias(cNokolexborNodeSet, "+", "|");
}
