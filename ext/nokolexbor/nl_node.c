#include "nokolexbor.h"

#define SORT_NAME css_result
#define SORT_TYPE lxb_dom_node_t *
#define SORT_CMP(x, y) (x->user >= y->user ? (x->user == y->user ? 0 : 1) : -1)
#include "timsort.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborDocument;
extern VALUE cNokolexborNodeSet;
extern VALUE eLexborError;
VALUE cNokolexborNode;

extern rb_data_type_t nl_document_type;

static const rb_data_type_t nl_node_type = {
    "Nokolexbor::Node",
    {
        0,
        0,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

VALUE
nl_rb_node_create(lxb_dom_node_t *node, VALUE rb_document)
{
  if (node == NULL)
  {
    rb_raise(rb_eArgError, "Cannot create Nokolexbor::Node with null pointer");
  }

  VALUE ret = TypedData_Wrap_Struct(cNokolexborNode, &nl_node_type, node);
  rb_iv_set(ret, "@document", rb_document);
  return ret;
}

inline lxb_dom_node_t *
nl_rb_node_unwrap(VALUE rb_node)
{
  lxb_dom_node_t *node;
  if (rb_obj_class(rb_node) == cNokolexborDocument)
  {
    TypedData_Get_Struct(rb_node, lxb_dom_document_t, &nl_document_type, node);
  }
  else
  {
    TypedData_Get_Struct(rb_node, lxb_dom_node_t, &nl_node_type, node);
  }
  return node;
}

static VALUE
nl_node_content(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  size_t str_len = 0;
  lxb_char_t *text = lxb_dom_node_text_content(node, &str_len);
  if (text == NULL)
  {
    return rb_str_new("", 0);
  }
  VALUE rb_str = rb_utf8_str_new(text, str_len);
  lxb_dom_document_destroy_text(node->owner_document, text);

  return rb_str;
}

static VALUE
nl_node_get_attr(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node);

  if (!lxb_dom_element_has_attribute(element, attr_c, attr_len))
  {
    return Qnil;
  }

  size_t attr_value_len;
  char *attr_value = lxb_dom_element_get_attribute(element, attr_c, attr_len, &attr_value_len);

  return rb_utf8_str_new(attr_value, attr_value_len);
}

static VALUE
nl_node_set_attr(VALUE self, VALUE rb_attr, VALUE rb_value)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  VALUE rb_value_s = rb_String(rb_value);

  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);
  const char *value_c = RSTRING_PTR(rb_value_s);
  int value_len = RSTRING_LEN(rb_value_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node);

  lxb_dom_element_set_attribute(element, attr_c, attr_len, value_c, value_len);

  return rb_value;
}

static VALUE
nl_node_remove_attr(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);

  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node);

  return lxb_dom_element_remove_attribute(element, attr_c, attr_len) == LXB_STATUS_OK ? Qtrue : Qfalse;
}

static lxb_status_t
nl_node_at_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  lexbor_array_t *array = (lexbor_array_t *)ctx;
  lxb_status_t status = lexbor_array_push_unique(array, node);
  if (status != LXB_STATUS_OK && status != LXB_STATUS_STOPPED)
  {
    nl_raise_lexbor_error(status);
  }
  // Stop at first result
  return LXB_STATUS_STOP;
}

static lxb_status_t
nl_node_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  lexbor_array_t *array = (lexbor_array_t *)ctx;
  lxb_status_t status = lexbor_array_push_unique(array, node);
  if (status != LXB_STATUS_OK && status != LXB_STATUS_STOPPED)
  {
    nl_raise_lexbor_error(status);
  }
  return LXB_STATUS_OK;
}

static void
nl_node_find(VALUE self, VALUE selector, lxb_selectors_cb_f cb, void *ctx)
{
  const char *selector_c = StringValuePtr(selector);
  int selector_len = RSTRING_LEN(selector);

  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  /* Create CSS parser. */
  lxb_css_parser_t *parser = lxb_css_parser_create();
  lxb_status_t status = lxb_css_parser_init(parser, NULL, NULL);
  if (status != LXB_STATUS_OK)
  {
    nl_raise_lexbor_error(status);
  }

  /* Selectors. */
  lxb_selectors_t *selectors = lxb_selectors_create();
  status = lxb_selectors_init(selectors);
  if (status != LXB_STATUS_OK)
  {
    nl_raise_lexbor_error(status);
  }

  /* Parse and get the log. */
  // TODO: Cache the list for reuse, improves performance
  lxb_css_selector_list_t *list = lxb_css_selectors_parse_relative_list(parser, selector_c, selector_len);
  if (parser->status != LXB_STATUS_OK)
  {
    nl_raise_lexbor_error(parser->status);
  }

  /* Find HTML nodes by CSS Selectors. */
  status = lxb_selectors_find(selectors, node, list, cb, ctx);
  if (status != LXB_STATUS_OK)
  {
    nl_raise_lexbor_error(status);
  }

  /* Destroy Selectors object. */
  (void)lxb_selectors_destroy(selectors, true);

  /* Destroy resources for CSS Parser. */
  (void)lxb_css_parser_destroy(parser, true);

  /* Destroy all object for all CSS Selector List. */
  lxb_css_selector_list_destroy_memory(list);
}

static void
mark_node_orders(lxb_dom_node_t *root)
{
  int count = 1;
  root->user = count;
  lxb_dom_node_t *node = root;
  do
  {
    if (node->first_child != NULL)
    {
      node = node->first_child;
      node->user = ++count;
    }
    else
    {
      while (node != root && node->next == NULL)
      {
        node = node->parent;
      }

      if (node == root)
      {
        break;
      }

      node = node->next;
      node->user = ++count;
    }

  } while (true);
}

// Sort nodes in document traversal order (the same as Nokorigi)
void sort_nodes_if_necessary(VALUE selector, lxb_dom_document_t *doc, lexbor_array_t *array)
{
  // No need to sort if there's only one selector, the results are natually in document traversal order
  if (strnstr(RSTRING_PTR(selector), ",", RSTRING_LEN(selector)) != NULL)
  {
    int need_order = 0;
    // Check if we have already markded orders, note that
    // we need to order again if new nodes are added to the document
    for (int i = 0; i < array->length; i++)
    {
      if (((lxb_dom_node_t *)array->list[i])->user == 0)
      {
        need_order = 1;
        break;
      }
    }
    if (need_order)
    {
      mark_node_orders(doc);
    }
    css_result_tim_sort(&array->list[0], array->length);
  }
}

VALUE
nl_node_at_css(VALUE self, VALUE selector)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_array_t *array = lexbor_array_create();

  nl_node_find(self, selector, nl_node_at_css_callback, array);

  if (array->length == 0)
  {
    return Qnil;
  }

  sort_nodes_if_necessary(selector, node->owner_document, array);

  return nl_rb_node_create(array->list[0], nl_rb_document_get(self));
}

VALUE
nl_node_css(VALUE self, VALUE selector)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_array_t *array = lexbor_array_create();

  nl_node_find(self, selector, nl_node_css_callback, array);

  sort_nodes_if_necessary(selector, node->owner_document, array);

  return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));
}

static VALUE
nl_node_inner_html(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  lxb_status_t status = lxb_html_serialize_deep_str(node, &str);
  if (status != LXB_STATUS_OK)
  {
    if (str.data != NULL)
    {
      lexbor_str_destroy(&str, node->owner_document->text, false);
    }
    nl_raise_lexbor_error(status);
  }

  if (str.data != NULL)
  {
    VALUE ret = rb_utf8_str_new(str.data, str.length);
    lexbor_str_destroy(&str, node->owner_document->text, false);
    return ret;
  }

  return Qnil;
}

static VALUE
nl_node_outer_html(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  lxb_status_t status = lxb_html_serialize_tree_str(node, &str);
  if (status != LXB_STATUS_OK)
  {
    if (str.data != NULL)
    {
      lexbor_str_destroy(&str, node->owner_document->text, false);
    }
    nl_raise_lexbor_error(status);
  }

  if (str.data != NULL)
  {
    VALUE ret = rb_utf8_str_new(str.data, str.length);
    lexbor_str_destroy(&str, node->owner_document->text, false);
    return ret;
  }

  return Qnil;
}

static VALUE
nl_node_has_key(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return Qfalse;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node);

  return lxb_dom_element_has_attribute(element, attr_c, attr_len) ? Qtrue : Qfalse;
}

static VALUE
nl_node_keys(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE ary_keys = rb_ary_new();

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return ary_keys;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node));

  while (attr != NULL)
  {
    size_t tmp_len;
    lxb_char_t *tmp = lxb_dom_attr_qualified_name(attr, &tmp_len);
    rb_ary_push(ary_keys, rb_utf8_str_new(tmp, tmp_len));

    attr = lxb_dom_element_next_attribute(attr);
  }

  return ary_keys;
}

static VALUE
nl_node_values(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE ary_values = rb_ary_new();

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return ary_values;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node));

  while (attr != NULL)
  {
    size_t tmp_len;
    lxb_char_t *tmp = lxb_dom_attr_value(attr, &tmp_len);
    if (tmp != NULL)
    {
      rb_ary_push(ary_values, rb_utf8_str_new(tmp, tmp_len));
    }

    attr = lxb_dom_element_next_attribute(attr);
  }

  return ary_values;
}

static VALUE
nl_node_attrs(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE rb_hash = rb_hash_new();

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return rb_hash;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node));

  while (attr != NULL)
  {
    size_t tmp_len;
    lxb_char_t *tmp = lxb_dom_attr_qualified_name(attr, &tmp_len);
    VALUE rb_key = rb_utf8_str_new(tmp, tmp_len);

    tmp = lxb_dom_attr_value(attr, &tmp_len);
    VALUE rb_value = tmp != NULL ? rb_utf8_str_new(tmp, tmp_len) : Qnil;

    rb_hash_aset(rb_hash, rb_key, rb_value);

    attr = lxb_dom_element_next_attribute(attr);
  }

  return rb_hash;
}

static VALUE
nl_node_parent(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  return node->parent ? nl_rb_node_create(node->parent, nl_rb_document_get(self)) : Qnil;
}

static VALUE
nl_node_previous(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  return node->prev ? nl_rb_node_create(node->prev, nl_rb_document_get(self)) : Qnil;
}

static VALUE
nl_node_previous_element(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  while (node->prev != NULL)
  {
    node = node->prev;
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT)
    {
      return nl_rb_node_create(node, nl_rb_document_get(self));
    }
  }
  return Qnil;
}

static VALUE
nl_node_next(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  return node->next ? nl_rb_node_create(node->next, nl_rb_document_get(self)) : Qnil;
}

static VALUE
nl_node_next_element(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  while (node->next != NULL)
  {
    node = node->next;
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT)
    {
      return nl_rb_node_create(node, nl_rb_document_get(self));
    }
  }
  return Qnil;
}

static VALUE
nl_node_children(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_node_t *child = node->first_child;
  lexbor_array_t *array = lexbor_array_create();

  while (child != NULL)
  {
    lexbor_array_push(array, child);
    child = child->next;
  }

  return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));
}

static VALUE
nl_node_child(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_node_t *child = node->first_child;
  return child ? nl_rb_node_create(child, nl_rb_document_get(self)) : Qnil;
}

static VALUE
nl_node_remove(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_node_remove(node);
  return Qnil;
}

static VALUE
nl_node_destroy(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_node_destroy(node);
  return Qnil;
}

static VALUE
nl_node_equals(VALUE self, VALUE other)
{
  lxb_dom_node_t *node1 = nl_rb_node_unwrap(self);
  lxb_dom_node_t *node2 = nl_rb_node_unwrap(other);
  return node1 == node2 ? Qtrue : Qfalse;
}

const lxb_char_t *
lxb_dom_node_name_qualified(lxb_dom_node_t *node, size_t *len)
{
  if (node->type == LXB_DOM_NODE_TYPE_ELEMENT)
  {
    return lxb_dom_element_qualified_name(lxb_dom_interface_element(node),
                                          len);
  }
  return lxb_dom_node_name(node, len);
}

static VALUE
nl_node_name(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  size_t len;
  lxb_char_t *name = lxb_dom_node_name_qualified(node, &len);
  return rb_utf8_str_new(name, len);
}

static VALUE
nl_node_add_sibling(VALUE self, VALUE next_or_previous, VALUE new)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_document_t *doc = node->owner_document;

  int insert_after;
  if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("next")))
  {
    insert_after = 1;
  }
  else if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("previous")))
  {
    insert_after = 0;
  }
  else
  {
    rb_raise(rb_eArgError, "Unsupported inserting position");
  }

  if (TYPE(new) == T_STRING)
  {
    size_t tag_name_len;
    lxb_char_t *tag_name = lxb_tag_name_by_id(lxb_html_document_tags(doc), LXB_TAG__UNDEF, &tag_name_len);
    if (tag_name == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error getting tag name");
    }
    lxb_dom_element_t *element = lxb_dom_document_create_element(doc, tag_name, tag_name_len, NULL);
    if (element == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error creating element");
    }
    lxb_dom_node_t *frag_root = lxb_html_document_parse_fragment(doc, element, RSTRING_PTR(new), RSTRING_LEN(new));
    if (frag_root == NULL)
    {
      rb_raise(rb_eArgError, "Error parsing HTML");
    }

    while (frag_root->first_child != NULL)
    {
      lxb_dom_node_t *child = frag_root->first_child;
      lxb_dom_node_remove(child);
      insert_after ? lxb_dom_node_insert_after(node, child) : lxb_dom_node_insert_before(node, child);
    }
    lxb_dom_node_destroy(frag_root);
  }
  else if (rb_obj_class(new) == cNokolexborNode)
  {
    lxb_dom_node_t *node_new = nl_rb_node_unwrap(new);
    insert_after ? lxb_dom_node_insert_after(node, node_new) : lxb_dom_node_insert_before(node, node_new);
  }
  else
  {
    rb_raise(rb_eArgError, "Unsupported node type");
  }
  return Qnil;
}

static VALUE
nl_node_add_child(VALUE self, VALUE new)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_document_t *doc = node->owner_document;

  if (TYPE(new) == T_STRING)
  {
    size_t tag_name_len;
    lxb_char_t *tag_name = lxb_tag_name_by_id(lxb_html_document_tags(doc), LXB_TAG__UNDEF, &tag_name_len);
    if (tag_name == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error getting tag name");
    }
    lxb_dom_element_t *element = lxb_dom_document_create_element(doc, tag_name, tag_name_len, NULL);
    if (element == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error creating element");
    }
    lxb_dom_node_t *frag_root = lxb_html_document_parse_fragment(doc, element, RSTRING_PTR(new), RSTRING_LEN(new));
    if (frag_root == NULL)
    {
      rb_raise(rb_eArgError, "Error parsing HTML");
    }

    while (frag_root->first_child != NULL)
    {
      lxb_dom_node_t *child = frag_root->first_child;
      lxb_dom_node_remove(child);
      lxb_dom_node_insert_child(node, child);
    }
    lxb_dom_node_destroy(frag_root);
  }
  else if (rb_obj_class(new) == cNokolexborNode)
  {
    lxb_dom_node_t *node_new = nl_rb_node_unwrap(new);
    lxb_dom_node_remove(node_new);
    lxb_dom_node_insert_child(node, node_new);
  }
  else
  {
    rb_raise(rb_eArgError, "Unsupported node type");
  }
  return Qnil;
}

static VALUE
nl_node_get_type(VALUE self)
{
  return INT2NUM(nl_rb_node_unwrap(self)->type);
}

static VALUE
nl_node_first_element_child(VALUE self)
{
  lxb_dom_node_t *parent = nl_rb_node_unwrap(self);
  lxb_dom_node_t *cur;

  if (parent == NULL)
  {
    return Qnil;
  }
  switch (parent->type)
  {
  case LXB_DOM_NODE_TYPE_ELEMENT:
  case LXB_DOM_NODE_TYPE_ENTITY:
  case LXB_DOM_NODE_TYPE_DOCUMENT:
    cur = parent->first_child;
    break;
  default:
    return Qnil;
  }
  while (cur != NULL)
  {
    if (cur->type == LXB_DOM_NODE_TYPE_ELEMENT)
    {
      return nl_rb_node_create(cur, nl_rb_document_get(self));
    }
    cur = cur->next;
  }
  return Qnil;
}

static VALUE
nl_node_last_element_child(VALUE self)
{
  lxb_dom_node_t *parent = nl_rb_node_unwrap(self);
  lxb_dom_node_t *cur;

  if (parent == NULL)
  {
    return Qnil;
  }
  switch (parent->type)
  {
  case LXB_DOM_NODE_TYPE_ELEMENT:
  case LXB_DOM_NODE_TYPE_ENTITY:
  case LXB_DOM_NODE_TYPE_DOCUMENT:
    cur = parent->last_child;
    break;
  default:
    return Qnil;
  }
  while (cur != NULL)
  {
    if (cur->type == LXB_DOM_NODE_TYPE_ELEMENT)
    {
      return nl_rb_node_create(cur, nl_rb_document_get(self));
    }
    cur = cur->prev;
  }
  return Qnil;
}

static VALUE
nl_node_clone(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_node_t *clone = lxb_dom_node_clone(node, 1);
  return nl_rb_node_create(clone, nl_rb_document_get(self));
}

void Init_nl_node(void)
{
  cNokolexborNode = rb_define_class_under(mNokolexbor, "Node", rb_cObject);
  rb_define_method(cNokolexborNode, "content", nl_node_content, 0);
  rb_define_method(cNokolexborNode, "[]", nl_node_get_attr, 1);
  rb_define_method(cNokolexborNode, "[]=", nl_node_set_attr, 2);
  rb_define_method(cNokolexborNode, "remove_attr", nl_node_remove_attr, 1);
  rb_define_method(cNokolexborNode, "==", nl_node_equals, 1);
  rb_define_method(cNokolexborNode, "css_impl", nl_node_css, 1);
  rb_define_method(cNokolexborNode, "at_css_impl", nl_node_at_css, 1);
  rb_define_method(cNokolexborNode, "inner_html", nl_node_inner_html, 0);
  rb_define_method(cNokolexborNode, "outer_html", nl_node_outer_html, 0);
  rb_define_method(cNokolexborNode, "key?", nl_node_has_key, 1);
  rb_define_method(cNokolexborNode, "keys", nl_node_keys, 0);
  rb_define_method(cNokolexborNode, "values", nl_node_values, 0);
  rb_define_method(cNokolexborNode, "parent", nl_node_parent, 0);
  rb_define_method(cNokolexborNode, "previous", nl_node_previous, 0);
  rb_define_method(cNokolexborNode, "previous_element", nl_node_previous_element, 0);
  rb_define_method(cNokolexborNode, "next", nl_node_next, 0);
  rb_define_method(cNokolexborNode, "next_element", nl_node_next_element, 0);
  rb_define_method(cNokolexborNode, "children", nl_node_children, 0);
  rb_define_method(cNokolexborNode, "child", nl_node_child, 0);
  rb_define_method(cNokolexborNode, "remove", nl_node_remove, 0);
  rb_define_method(cNokolexborNode, "destroy", nl_node_destroy, 0);
  rb_define_method(cNokolexborNode, "attrs", nl_node_attrs, 0);
  rb_define_method(cNokolexborNode, "name", nl_node_name, 0);
  rb_define_method(cNokolexborNode, "add_sibling", nl_node_add_sibling, 2);
  rb_define_method(cNokolexborNode, "add_child", nl_node_add_child, 1);
  rb_define_method(cNokolexborNode, "node_type", nl_node_get_type, 0);
  rb_define_method(cNokolexborNode, "first_element_child", nl_node_first_element_child, 0);
  rb_define_method(cNokolexborNode, "last_element_child", nl_node_last_element_child, 0);
  rb_define_method(cNokolexborNode, "clone", nl_node_clone, 0);

  rb_define_alias(cNokolexborNode, "attr", "[]");
  rb_define_alias(cNokolexborNode, "set_attr", "[]=");
  rb_define_alias(cNokolexborNode, "text", "content");
  rb_define_alias(cNokolexborNode, "inner_text", "content");
  rb_define_alias(cNokolexborNode, "to_str", "content");
  rb_define_alias(cNokolexborNode, "to_html", "outer_html");
  rb_define_alias(cNokolexborNode, "to_s", "outer_html");
  rb_define_alias(cNokolexborNode, "type", "node_type");
  rb_define_alias(cNokolexborNode, "dup", "clone");
}
