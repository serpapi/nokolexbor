#include "nokolexbor.h"

#define SORT_NAME nl_css_result
#define SORT_TYPE lxb_dom_node_t *
#define SORT_CMP(x, y) (x->user >= y->user ? (x->user == y->user ? 0 : 1) : -1)
#include "timsort.h"

extern VALUE mNokolexbor;
extern VALUE cNokolexborDocument;
extern VALUE cNokolexborText;
extern VALUE cNokolexborComment;
extern VALUE cNokolexborProcessingInstruction;
extern VALUE cNokolexborNodeSet;
extern VALUE cNokolexborDocumentFragment;
extern VALUE cNokolexborAttribute;
extern VALUE eLexborError;
VALUE cNokolexborNode;
VALUE cNokolexborElement;
VALUE cNokolexborCharacterData;

extern rb_data_type_t nl_document_type;

VALUE
nl_rb_node_create(lxb_dom_node_t *node, VALUE rb_document)
{
  if (node == NULL) {
    rb_raise(rb_eArgError, "Cannot create Nokolexbor::Node with null pointer");
  }

  VALUE rb_class;
  switch (node->type) {
  case LXB_DOM_NODE_TYPE_ELEMENT:
    rb_class = cNokolexborElement;
    break;
  case LXB_DOM_NODE_TYPE_ATTRIBUTE:
    rb_class = cNokolexborAttribute;
    break;
  case LXB_DOM_NODE_TYPE_TEXT:
    rb_class = cNokolexborText;
    break;
  case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    rb_class = cNokolexborCharacterData;
    break;
  // case LXB_DOM_NODE_TYPE_ENTITY_REFERENCE:
  //   break;
  // case LXB_DOM_NODE_TYPE_ENTITY:
  //   break;
  case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
    rb_class = cNokolexborProcessingInstruction;
    break;
  case LXB_DOM_NODE_TYPE_COMMENT:
    rb_class = cNokolexborComment;
    break;
  // case LXB_DOM_NODE_TYPE_DOCUMENT:
  //   break;
  // case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE:
  //   break;
  case LXB_DOM_NODE_TYPE_DOCUMENT_FRAGMENT:
    rb_class = cNokolexborDocumentFragment;
    break;
  // case LXB_DOM_NODE_TYPE_NOTATION:
  //   break;
  default:
    rb_class = cNokolexborNode;
  }

  VALUE ret = Data_Wrap_Struct(rb_class, NULL, NULL, node);
  rb_iv_set(ret, "@document", rb_document);
  return ret;
}

inline lxb_dom_node_t *
nl_rb_node_unwrap(VALUE rb_node)
{
  lxb_dom_node_t *node;
  if (rb_obj_is_kind_of(rb_node, cNokolexborDocument)) {
    TypedData_Get_Struct(rb_node, lxb_dom_node_t, &nl_document_type, node);
  } else {
    Data_Get_Struct(rb_node, lxb_dom_node_t, node);
  }
  return node;
}

static VALUE
nl_node_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_name;
  VALUE rb_document;
  VALUE rest;

  rb_scan_args(argc, argv, "2*", &rb_name, &rb_document, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  const char *c_name = StringValuePtr(rb_name);
  size_t name_len = RSTRING_LEN(rb_name);
  lxb_dom_element_t *element = lxb_dom_document_create_element(document, (const lxb_char_t *)c_name, name_len, NULL);
  if (element == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating element");
  }

  VALUE rb_node = nl_rb_node_create(&element->node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

static VALUE
nl_node_attribute(VALUE self, VALUE rb_name)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  const char *c_name = StringValuePtr(rb_name);
  size_t name_len = RSTRING_LEN(rb_name);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return Qnil;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_attr_by_name(lxb_dom_interface_element(node), (const lxb_char_t *)c_name, name_len);
  if (attr == NULL) {
    return Qnil;
  }
  if (attr->owner == NULL) {
    attr->owner = node;
  }
  return nl_rb_node_create(attr, nl_rb_document_get(self));
}

static VALUE
nl_node_attribute_nodes(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE ary = rb_ary_new();
  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return ary;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));

  if (attr == NULL) {
    return ary;
  }

  VALUE rb_doc = nl_rb_document_get(self);
  while (attr != NULL) {
    if (attr->owner == NULL) {
      attr->owner = node;
    }
    rb_ary_push(ary, nl_rb_node_create(attr, rb_doc));
    attr = attr->next;
  }

  return ary;
}

static VALUE
nl_node_content(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  size_t str_len = 0;
  lxb_char_t *text = lxb_dom_node_text_content(node, &str_len);
  if (text == NULL) {
    return rb_str_new("", 0);
  }
  VALUE rb_str = rb_utf8_str_new((char *)text, str_len);
  lxb_dom_document_destroy_text(node->owner_document, text);

  return rb_str;
}

static VALUE
nl_node_content_set(VALUE self, VALUE content)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  const char *c_content = StringValuePtr(content);
  size_t content_len = RSTRING_LEN(content);
  lxb_status_t status = lxb_dom_node_text_content_set(node, (const lxb_char_t *)c_content, content_len);
  if (status != LXB_STATUS_OK) {
    nl_raise_lexbor_error(status);
  }
  return content;
}

static VALUE
nl_node_get_attr(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  size_t attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_dom_interface_element(node);

  if (!lxb_dom_element_has_attribute(element, (const lxb_char_t *)attr_c, attr_len)) {
    return Qnil;
  }

  size_t attr_value_len;
  const lxb_char_t *attr_value = lxb_dom_element_get_attribute(element, (const lxb_char_t *)attr_c, attr_len, &attr_value_len);

  return rb_utf8_str_new((const char *)attr_value, attr_value_len);
}

static VALUE
nl_node_set_attr(VALUE self, VALUE rb_attr, VALUE rb_value)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  VALUE rb_value_s = rb_String(rb_value);

  const char *attr_c = RSTRING_PTR(rb_attr_s);
  size_t attr_len = RSTRING_LEN(rb_attr_s);
  const char *value_c = RSTRING_PTR(rb_value_s);
  size_t value_len = RSTRING_LEN(rb_value_s);

  lxb_dom_element_t *element = lxb_dom_interface_element(node);

  lxb_dom_element_set_attribute(element, (const lxb_char_t *)attr_c, attr_len, (const lxb_char_t *)value_c, value_len);

  return rb_value;
}

static VALUE
nl_node_remove_attr(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return Qnil;
  }

  VALUE rb_attr_s = rb_String(rb_attr);

  const char *attr_c = RSTRING_PTR(rb_attr_s);
  size_t attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_dom_interface_element(node);

  return lxb_dom_element_remove_attribute(element, (const lxb_char_t *)attr_c, attr_len) == LXB_STATUS_OK ? Qtrue : Qfalse;
}

lxb_status_t
nl_node_at_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  lexbor_array_t *array = (lexbor_array_t *)ctx;
  lxb_status_t status = lexbor_array_push_unique(array, node);
  if (status != LXB_STATUS_OK && status != LXB_STATUS_STOPPED) {
    nl_raise_lexbor_error(status);
  }
  // Stop at first result
  return LXB_STATUS_STOP;
}

lxb_status_t
nl_node_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  lexbor_array_t *array = (lexbor_array_t *)ctx;
  lxb_status_t status = lexbor_array_push_unique(array, node);
  if (status != LXB_STATUS_OK && status != LXB_STATUS_STOPPED) {
    nl_raise_lexbor_error(status);
  }
  return LXB_STATUS_OK;
}

lxb_status_t
nl_node_find(VALUE self, VALUE selector, lxb_selectors_cb_f cb, void *ctx)
{
  const char *selector_c = StringValuePtr(selector);
  size_t selector_len = RSTRING_LEN(selector);

  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  lxb_status_t status;
  lxb_css_parser_t *parser = NULL;
  lxb_selectors_t *selectors = NULL;
  lxb_css_selector_list_t *list = NULL;

  /* Create CSS parser. */
  parser = lxb_css_parser_create();
  status = lxb_css_parser_init(parser, NULL, NULL);
  if (status != LXB_STATUS_OK) {
    goto cleanup;
  }

  /* Selectors. */
  selectors = lxb_selectors_create();
  status = lxb_selectors_init(selectors);
  if (status != LXB_STATUS_OK) {
    goto cleanup;
  }

  /* Parse and get the log. */
  // TODO: Cache the list for reuse, improves performance
  list = lxb_css_selectors_parse_relative_list(parser, (const lxb_char_t *)selector_c, selector_len);
  if (parser->status != LXB_STATUS_OK) {
    status = parser->status;
    goto cleanup;
  }

  /* Find HTML nodes by CSS Selectors. */
  status = lxb_selectors_find(selectors, node, list, cb, ctx);
  if (status != LXB_STATUS_OK) {
    goto cleanup;
  }

cleanup:
  /* Destroy Selectors object. */
  (void)lxb_selectors_destroy(selectors, true);

  /* Destroy resources for CSS Parser. */
  (void)lxb_css_parser_destroy(parser, true);

  /* Destroy all object for all CSS Selector List. */
  lxb_css_selector_list_destroy_memory(list);

  return status;
}

static void
mark_node_orders(lxb_dom_node_t *root)
{
  size_t count = 1;
  root->user = (void *)count;
  lxb_dom_node_t *node = root;
  do {
    if (node->first_child != NULL) {
      node = node->first_child;
      node->user = (void *)++count;
    } else {
      while (node != root && node->next == NULL) {
        node = node->parent;
      }

      if (node == root) {
        break;
      }

      node = node->next;
      node->user = (void *)++count;
    }

  } while (true);
}

// Sort nodes in document traversal order (the same as Nokorigi)
void nl_sort_nodes_if_necessary(VALUE selector, lxb_dom_document_t *doc, lexbor_array_t *array)
{
  // No need to sort if there's only one selector, the results are natually in document traversal order
  if (strchr(RSTRING_PTR(selector), ',') != NULL) {
    int need_order = 0;
    // Check if we have already markded orders, note that
    // we need to order again if new nodes are added to the document
    for (size_t i = 0; i < array->length; i++) {
      if (((lxb_dom_node_t *)array->list[i])->user == 0) {
        need_order = 1;
        break;
      }
    }
    if (need_order) {
      mark_node_orders(&doc->node);
    }
    nl_css_result_tim_sort((lxb_dom_node_t **)&array->list[0], array->length);
  }
}

static VALUE
nl_node_at_css(VALUE self, VALUE selector)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_array_t *array = lexbor_array_create();

  lxb_status_t status = nl_node_find(self, selector, nl_node_at_css_callback, array);

  if (status != LXB_STATUS_OK) {
    lexbor_array_destroy(array, true);
    nl_raise_lexbor_error(status);
  }

  if (array->length == 0) {
    lexbor_array_destroy(array, true);
    return Qnil;
  }

  nl_sort_nodes_if_necessary(selector, node->owner_document, array);

  VALUE ret = nl_rb_node_create(array->list[0], nl_rb_document_get(self));

  lexbor_array_destroy(array, true);

  return ret;
}

static VALUE
nl_node_css(VALUE self, VALUE selector)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_array_t *array = lexbor_array_create();

  lxb_status_t status = nl_node_find(self, selector, nl_node_css_callback, array);
  if (status != LXB_STATUS_OK) {
    lexbor_array_destroy(array, true);
    nl_raise_lexbor_error(status);
  }

  nl_sort_nodes_if_necessary(selector, node->owner_document, array);

  return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));
}

static VALUE
nl_node_inner_html(int argc, VALUE *argv, VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  VALUE options;
  lxb_status_t status;
  size_t indent = 0;
  rb_scan_args(argc, argv, "01", &options);

  if (TYPE(options) == T_HASH) {
    VALUE rb_indent = rb_hash_aref(options, ID2SYM(rb_intern("indent")));
    if (!NIL_P(rb_indent)) {
      indent = NUM2INT(rb_indent);
    }
  }
  if (indent > 0) {
    status = lxb_html_serialize_pretty_deep_str(node, 0, 0, &str);
  } else {
    status = lxb_html_serialize_deep_str(node, &str);
  }
  if (status != LXB_STATUS_OK) {
    if (str.data != NULL) {
      lexbor_str_destroy(&str, node->owner_document->text, false);
    }
    nl_raise_lexbor_error(status);
  }

  if (str.data != NULL) {
    VALUE ret = rb_utf8_str_new((const char *)str.data, str.length);
    lexbor_str_destroy(&str, node->owner_document->text, false);
    return ret;
  }

  return Qnil;
}

static VALUE
nl_node_outer_html(int argc, VALUE *argv, VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  VALUE options;
  lxb_status_t status;
  size_t indent = 0;
  rb_scan_args(argc, argv, "01", &options);

  if (TYPE(options) == T_HASH) {
    VALUE rb_indent = rb_hash_aref(options, ID2SYM(rb_intern("indent")));
    if (!NIL_P(rb_indent)) {
      indent = NUM2INT(rb_indent);
    }
  }
  if (indent > 0) {
    status = lxb_html_serialize_pretty_tree_str(node, 0, 0, &str);
  } else {
    status = lxb_html_serialize_tree_str(node, &str);
  }
  if (status != LXB_STATUS_OK) {
    if (str.data != NULL) {
      lexbor_str_destroy(&str, node->owner_document->text, false);
    }
    nl_raise_lexbor_error(status);
  }

  if (str.data != NULL) {
    VALUE ret = rb_utf8_str_new((const char *)str.data, str.length);
    lexbor_str_destroy(&str, node->owner_document->text, false);
    return ret;
  }

  return Qnil;
}

static VALUE
nl_node_has_key(VALUE self, VALUE rb_attr)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return Qfalse;
  }

  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  size_t attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_dom_interface_element(node);

  return lxb_dom_element_has_attribute(element, (const lxb_char_t *)attr_c, attr_len) ? Qtrue : Qfalse;
}

static VALUE
nl_node_keys(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE ary_keys = rb_ary_new();

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return ary_keys;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));

  while (attr != NULL) {
    size_t tmp_len;
    const lxb_char_t *tmp = lxb_dom_attr_qualified_name(attr, &tmp_len);
    rb_ary_push(ary_keys, rb_utf8_str_new((const char *)tmp, tmp_len));

    attr = lxb_dom_element_next_attribute(attr);
  }

  return ary_keys;
}

static VALUE
nl_node_values(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  VALUE ary_values = rb_ary_new();

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return ary_values;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));

  while (attr != NULL) {
    size_t tmp_len;
    const lxb_char_t *tmp = lxb_dom_attr_value(attr, &tmp_len);
    if (tmp != NULL) {
      rb_ary_push(ary_values, rb_utf8_str_new((const char *)tmp, tmp_len));
    } else {
      rb_ary_push(ary_values, rb_str_new("", 0));
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

  if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
    return rb_hash;
  }

  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));

  while (attr != NULL) {
    size_t tmp_len;
    const lxb_char_t *tmp = lxb_dom_attr_qualified_name(attr, &tmp_len);
    VALUE rb_key = rb_utf8_str_new((const char *)tmp, tmp_len);

    tmp = lxb_dom_attr_value(attr, &tmp_len);
    VALUE rb_value = tmp != NULL ? rb_utf8_str_new((const char *)tmp, tmp_len) : rb_str_new("", 0);

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
  while (node->prev != NULL) {
    node = node->prev;
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
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
  while (node->next != NULL) {
    node = node->next;
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
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

  while (child != NULL) {
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
  if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
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
  const lxb_char_t *name = lxb_dom_node_name_qualified(node, &len);
  return rb_utf8_str_new((const char *)name, len);
}

static lxb_dom_node_t *
nl_node_parse_fragment(lxb_dom_document_t *doc, lxb_dom_element_t *element, lxb_char_t *html, size_t size)
{
  size_t tag_name_len;
  lxb_html_document_t *html_doc = lxb_html_interface_document(doc);
  if (element == NULL) {
    const lxb_char_t *tag_name = lxb_tag_name_by_id(lxb_html_document_tags(html_doc), LXB_TAG__UNDEF, &tag_name_len);
    if (tag_name == NULL) {
      rb_raise(rb_eRuntimeError, "Error getting tag name");
    }
    element = lxb_dom_document_create_element(doc, tag_name, tag_name_len, NULL);
    if (element == NULL) {
      rb_raise(rb_eRuntimeError, "Error creating element");
    }
  }
  lxb_dom_node_t *frag_root = lxb_html_document_parse_fragment(html_doc, element, html, size);
  if (frag_root == NULL) {
    rb_raise(rb_eArgError, "Error parsing HTML");
  }
  return frag_root;
}

static VALUE
nl_node_parse(VALUE self, VALUE html)
{
  Check_Type(html, T_STRING);
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_document_t *doc = node->owner_document;

  lxb_dom_node_t *frag_root = nl_node_parse_fragment(doc, lxb_dom_interface_element(node), (lxb_char_t *)RSTRING_PTR(html), RSTRING_LEN(html));
  lexbor_array_t *array = lexbor_array_create();

  while (frag_root->first_child != NULL) {
    lxb_dom_node_t *child = frag_root->first_child;
    lxb_dom_node_remove(child);
    lexbor_array_push(array, child);
  }
  lxb_dom_node_destroy(frag_root);
  return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));
}

static VALUE
nl_node_add_sibling(VALUE self, VALUE next_or_previous, VALUE new)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_document_t *doc = node->owner_document;

  int insert_after;
  if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("next"))) {
    insert_after = 1;
  } else if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("previous"))) {
    insert_after = 0;
  } else {
    rb_raise(rb_eArgError, "Unsupported inserting position");
  }

  if (TYPE(new) == T_STRING) {
    lxb_dom_node_t *frag_root = nl_node_parse_fragment(doc, NULL, (lxb_char_t *)RSTRING_PTR(new), RSTRING_LEN(new));
    lexbor_array_t *array = lexbor_array_create();

    while (frag_root->first_child != NULL) {
      lxb_dom_node_t *child = frag_root->first_child;
      lxb_dom_node_remove(child);
      insert_after ? lxb_dom_node_insert_after(node, child) : lxb_dom_node_insert_before(node, child);
      lexbor_array_push(array, child);
    }
    lxb_dom_node_destroy(frag_root);
    return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));

  } else if (rb_obj_is_kind_of(new, cNokolexborNode)) {
    lxb_dom_node_t *node_new = nl_rb_node_unwrap(new);
    lxb_dom_node_remove(node_new);
    insert_after ? lxb_dom_node_insert_after(node, node_new) : lxb_dom_node_insert_before(node, node_new);
    return new;

  } else {
    rb_raise(rb_eArgError, "Unsupported node type");
  }
  return Qnil;
}

static VALUE
nl_node_add_child(VALUE self, VALUE new)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_document_t *doc = node->owner_document;

  if (TYPE(new) == T_STRING) {
    lxb_dom_node_t *frag_root = nl_node_parse_fragment(doc, NULL, (lxb_char_t *)RSTRING_PTR(new), RSTRING_LEN(new));
    lexbor_array_t *array = lexbor_array_create();

    while (frag_root->first_child != NULL) {
      lxb_dom_node_t *child = frag_root->first_child;
      lxb_dom_node_remove(child);
      lxb_dom_node_insert_child(node, child);
      lexbor_array_push(array, child);
    }
    lxb_dom_node_destroy(frag_root);
    return nl_rb_node_set_create_with_data(array, nl_rb_document_get(self));

  } else if (rb_obj_is_kind_of(new, cNokolexborNode)) {
    lxb_dom_node_t *node_new = nl_rb_node_unwrap(new);
    lxb_dom_node_remove(node_new);
    lxb_dom_node_insert_child(node, node_new);
    return new;

  } else {
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

  if (parent == NULL) {
    return Qnil;
  }
  switch (parent->type) {
  case LXB_DOM_NODE_TYPE_ELEMENT:
  case LXB_DOM_NODE_TYPE_ENTITY:
  case LXB_DOM_NODE_TYPE_DOCUMENT:
    cur = parent->first_child;
    break;
  default:
    return Qnil;
  }
  while (cur != NULL) {
    if (cur->type == LXB_DOM_NODE_TYPE_ELEMENT) {
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

  if (parent == NULL) {
    return Qnil;
  }
  switch (parent->type) {
  case LXB_DOM_NODE_TYPE_ELEMENT:
  case LXB_DOM_NODE_TYPE_ENTITY:
  case LXB_DOM_NODE_TYPE_DOCUMENT:
    cur = parent->last_child;
    break;
  default:
    return Qnil;
  }
  while (cur != NULL) {
    if (cur->type == LXB_DOM_NODE_TYPE_ELEMENT) {
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
  lxb_dom_node_t *clone;

  switch (node->type) {
  case LXB_DOM_NODE_TYPE_ATTRIBUTE:
    clone = lxb_dom_attr_interface_clone(node->owner_document, lxb_dom_interface_attr(node));
  case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    clone = lxb_dom_cdata_section_interface_clone(node->owner_document, lxb_dom_interface_cdata_section(node));
  default:
    clone = lxb_dom_node_clone(node, true);
    break;
  }
  return nl_rb_node_create(clone, nl_rb_document_get(self));
}

void Init_nl_node(void)
{
  cNokolexborNode = rb_define_class_under(mNokolexbor, "Node", rb_cObject);
  rb_undef_alloc_func(cNokolexborNode);

  cNokolexborElement = rb_define_class_under(mNokolexbor, "Element", cNokolexborNode);
  cNokolexborCharacterData = rb_define_class_under(mNokolexbor, "CharacterData", cNokolexborNode);

  rb_define_singleton_method(cNokolexborNode, "new", nl_node_new, -1);
  rb_define_method(cNokolexborNode, "attribute", nl_node_attribute, 1);
  rb_define_method(cNokolexborNode, "attribute_nodes", nl_node_attribute_nodes, 0);
  rb_define_method(cNokolexborNode, "content", nl_node_content, 0);
  rb_define_method(cNokolexborNode, "content=", nl_node_content_set, 1);
  rb_define_method(cNokolexborNode, "[]", nl_node_get_attr, 1);
  rb_define_method(cNokolexborNode, "[]=", nl_node_set_attr, 2);
  rb_define_method(cNokolexborNode, "remove_attr", nl_node_remove_attr, 1);
  rb_define_method(cNokolexborNode, "==", nl_node_equals, 1);
  rb_define_method(cNokolexborNode, "css_impl", nl_node_css, 1);
  rb_define_method(cNokolexborNode, "at_css_impl", nl_node_at_css, 1);
  rb_define_method(cNokolexborNode, "inner_html", nl_node_inner_html, -1);
  rb_define_method(cNokolexborNode, "outer_html", nl_node_outer_html, -1);
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
  rb_define_method(cNokolexborNode, "parse", nl_node_parse, 1);
  rb_define_method(cNokolexborNode, "add_sibling", nl_node_add_sibling, 2);
  rb_define_method(cNokolexborNode, "add_child", nl_node_add_child, 1);
  rb_define_method(cNokolexborNode, "node_type", nl_node_get_type, 0);
  rb_define_method(cNokolexborNode, "first_element_child", nl_node_first_element_child, 0);
  rb_define_method(cNokolexborNode, "last_element_child", nl_node_last_element_child, 0);
  rb_define_method(cNokolexborNode, "clone", nl_node_clone, 0);

  rb_define_alias(cNokolexborNode, "attr", "[]");
  rb_define_alias(cNokolexborNode, "get_attribute", "[]");
  rb_define_alias(cNokolexborNode, "set_attr", "[]=");
  rb_define_alias(cNokolexborNode, "set_attribute", "[]=");
  rb_define_alias(cNokolexborNode, "has_attribute?", "key?");
  rb_define_alias(cNokolexborNode, "delete", "remove_attr");
  rb_define_alias(cNokolexborNode, "remove_attribute", "remove_attr");
  rb_define_alias(cNokolexborNode, "text", "content");
  rb_define_alias(cNokolexborNode, "inner_text", "content");
  rb_define_alias(cNokolexborNode, "to_str", "content");
  rb_define_alias(cNokolexborNode, "to_html", "outer_html");
  rb_define_alias(cNokolexborNode, "serialize", "outer_html");
  rb_define_alias(cNokolexborNode, "to_s", "outer_html");
  rb_define_alias(cNokolexborNode, "unlink", "remove");
  rb_define_alias(cNokolexborNode, "type", "node_type");
  rb_define_alias(cNokolexborNode, "dup", "clone");
}
