#include "lexbor.h"

extern VALUE mLexbor;
extern VALUE cLexborNodeSet;
VALUE cLexborNode;

typedef struct
{
  VALUE rb_ary;
  VALUE rb_document;
} lexbor_node_css_callback_ctx;

static void
mark_lexbor_node(lexbor_node_t *node_ex)
{
  rb_gc_mark(node_ex->rb_document);
}

static void
free_lexbor_node(lexbor_node_t *node_ex)
{
  lexbor_free(node_ex);
}

const rb_data_type_t lexbor_node_type = {
    "Node",
    {
        mark_lexbor_node,
        free_lexbor_node,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

VALUE
lexbor_rb_node_create(lxb_dom_node_t *node, VALUE rb_document)
{
  lexbor_node_t *node_wrap = lexbor_malloc(sizeof(lexbor_node_t));
  node_wrap->node = node;
  node_wrap->rb_document = rb_document;
  return TypedData_Wrap_Struct(cLexborNode, &lexbor_node_type, node_wrap);
}

inline lexbor_node_t *
lexbor_rb_node_unwrap(VALUE rb_node)
{
  lexbor_node_t *node;
  TypedData_Get_Struct(rb_node, lxb_dom_node_t, &lexbor_node_type, node);
  return node;
}

static VALUE
lexbor_node_content(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);

  size_t str_len = 0;
  lxb_char_t *text = lxb_dom_node_text_content(node->node, &str_len);
  VALUE rb_str = rb_utf8_str_new(text, str_len);
  lxb_dom_document_destroy_text(node->node->owner_document, text);

  return rb_str;
}

static VALUE
lexbor_node_get_attr(VALUE self, VALUE rb_attr)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node->node);

  if (!lxb_dom_element_has_attribute(element, attr_c, attr_len))
  {
    return Qnil;
  }

  size_t attr_value_len;
  char *attr_value = lxb_dom_element_get_attribute(element, attr_c, attr_len, &attr_value_len);

  return rb_utf8_str_new(attr_value, attr_value_len);
}

static VALUE
lexbor_node_set_attr(VALUE self, VALUE rb_attr, VALUE rb_value)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  VALUE rb_attr_s = rb_String(rb_attr);
  VALUE rb_value_s = rb_String(rb_value);

  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);
  const char *value_c = RSTRING_PTR(rb_value_s);
  int value_len = RSTRING_LEN(rb_value_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node->node);

  lxb_dom_element_set_attribute(element, attr_c, attr_len, value_c, value_len);

  return rb_value;
}

static lxb_status_t
lexbor_node_at_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  *(lxb_dom_node_t **)ctx = node;
  // Stop at first result
  return LXB_STATUS_STOP;
}

static lxb_status_t
lexbor_node_css_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *_ctx)
{
  lexbor_node_css_callback_ctx *ctx = (lexbor_node_css_callback_ctx *)_ctx;
  VALUE rb_node = lexbor_rb_node_create(node, ctx->rb_document);
  rb_ary_push(ctx->rb_ary, rb_node);
  return LXB_STATUS_OK;
}

static void
lexbor_node_find(VALUE self, VALUE selector, lxb_selectors_cb_f cb, void *ctx)
{
  const char *selector_c = StringValuePtr(selector);
  int selector_len = RSTRING_LEN(selector);

  lexbor_node_t *node = lexbor_rb_node_unwrap(self);

  /* Create CSS parser. */
  lxb_css_parser_t *parser = lxb_css_parser_create();
  lxb_status_t status = lxb_css_parser_init(parser, NULL, NULL);
  if (status != LXB_STATUS_OK)
  {
    return;
  }

  /* Selectors. */
  lxb_selectors_t *selectors = lxb_selectors_create();
  status = lxb_selectors_init(selectors);
  if (status != LXB_STATUS_OK)
  {
    return;
  }

  /* Parse and get the log. */
  // TODO: Cache the list for reuse, improves performance
  lxb_css_selector_list_t *list = lxb_css_selectors_parse(parser, selector_c, selector_len);
  if (parser->status != LXB_STATUS_OK)
  {
    return;
  }

  /* Find HTML nodes by CSS Selectors. */
  status = lxb_selectors_find(selectors, node->node, list, cb, ctx);
  if (status != LXB_STATUS_OK)
  {
    return;
  }

  /* Destroy Selectors object. */
  (void)lxb_selectors_destroy(selectors, true);

  /* Destroy resources for CSS Parser. */
  (void)lxb_css_parser_destroy(parser, true);

  /* Destroy all object for all CSS Selector List. */
  lxb_css_selector_list_destroy_memory(list);
}

VALUE
lexbor_node_at_css(VALUE self, VALUE selector)
{
  lxb_dom_node_t *result_node = NULL;
  lexbor_node_find(self, selector, lexbor_node_at_css_callback, &result_node);

  if (result_node == NULL)
  {
    return Qnil;
  }

  return lexbor_rb_node_create(result_node, self);
}

VALUE
lexbor_node_css(VALUE self, VALUE selector)
{
  VALUE rb_node_ary = rb_ary_new();
  lexbor_node_css_callback_ctx ctx = {rb_node_ary, self};
  lexbor_node_find(self, selector, lexbor_node_css_callback, &ctx);

  return rb_funcall(cLexborNodeSet, rb_intern("new"), 1, rb_node_ary);
}

VALUE
lexbor_node_inner_html(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  lxb_html_serialize_deep_str(node->node, &str);
  if (str.data != NULL)
  {
    VALUE ret = rb_utf8_str_new(str.data, str.length);
    lexbor_str_destroy(&str, node->node->owner_document->text, false);
    return ret;
  }
  return Qnil;
}

VALUE
lexbor_node_outer_html(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lexbor_str_t str = {0};
  lxb_html_serialize_tree_str(node->node, &str);
  if (str.data != NULL)
  {
    VALUE ret = rb_utf8_str_new(str.data, str.length);
    lexbor_str_destroy(&str, node->node->owner_document->text, false);
    return ret;
  }
  return Qnil;
}

static VALUE
lexbor_node_has_key(VALUE self, VALUE rb_attr)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  VALUE rb_attr_s = rb_String(rb_attr);
  const char *attr_c = RSTRING_PTR(rb_attr_s);
  int attr_len = RSTRING_LEN(rb_attr_s);

  lxb_dom_element_t *element = lxb_html_interface_element(node->node);

  return lxb_dom_element_has_attribute(element, attr_c, attr_len) ? Qtrue : Qfalse;
}

static VALUE
lexbor_node_keys(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node->node));
  VALUE ary_keys = rb_ary_new();

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
lexbor_node_values(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node->node));
  VALUE ary_values = rb_ary_new();

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
lexbor_node_attrs(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(lxb_html_interface_element(node->node));
  VALUE rb_hash = rb_hash_new();

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
lexbor_node_parent(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lexbor_node_t *parent = lxb_dom_node_parent(node->node);
  if (parent)
  {
    return lexbor_rb_node_create(parent, node->rb_document);
  }
  else
  {
    return Qnil;
  }
}

static VALUE
lexbor_node_previous(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lexbor_node_t *prev = lxb_dom_node_prev(node->node);
  if (prev)
  {
    return lexbor_rb_node_create(prev, node->rb_document);
  }
  else
  {
    return Qnil;
  }
}

static VALUE
lexbor_node_next(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lexbor_node_t *next = lxb_dom_node_next(node->node);
  if (next)
  {
    return lexbor_rb_node_create(next, node->rb_document);
  }
  else
  {
    return Qnil;
  }
}

static VALUE
lexbor_node_children(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_node_t *child = lxb_dom_node_first_child(node->node);
  VALUE ary_children = rb_ary_new();

  while (child != NULL)
  {
    rb_ary_push(ary_children, lexbor_rb_node_create(child, node->rb_document));

    child = lxb_dom_node_next(child);
  }

  return rb_funcall(cLexborNodeSet, rb_intern("new"), 1, ary_children);
}

static VALUE
lexbor_node_child(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_node_t *child = lxb_dom_node_first_child(node->node);
  if (child == NULL)
  {
    return Qnil;
  }
  return lexbor_rb_node_create(child, node->rb_document);
}

static VALUE
lexbor_node_remove(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  lxb_dom_node_destroy(node->node);
  return Qnil;
}

static VALUE
lexbor_node_equals(VALUE self, VALUE other)
{
  lexbor_node_t *node1 = lexbor_rb_node_unwrap(self);
  lexbor_node_t *node2 = lexbor_rb_node_unwrap(other);
  return node1->node == node2->node ? Qtrue : Qfalse;
}

static VALUE
lexbor_node_name(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  size_t len;
  lxb_char_t *name = lxb_dom_node_name(node->node, &len);
  return rb_utf8_str_new(name, len);
}

static VALUE
lexbor_node_is_comment(VALUE self)
{
  lexbor_node_t *node = lexbor_rb_node_unwrap(self);
  return node->node->type == LXB_DOM_NODE_TYPE_COMMENT ? Qtrue : Qfalse;
}

static VALUE
lexbor_node_add_sibling(VALUE self, VALUE next_or_previous, VALUE new)
{
  if (TYPE(new) != T_STRING) {
    rb_raise(rb_eArgError, "Only support inserting with String for now");
    return Qnil;
  }
  lexbor_node_t *node_self = lexbor_rb_node_unwrap(self);
  lxb_dom_node_t *node_new = lxb_dom_node_interface_create(node_self->node->owner_document);
  node_new->type = LXB_DOM_NODE_TYPE_TEXT;
  lxb_dom_node_text_content_set(node_new, RSTRING_PTR(new), RSTRING_LEN(new));

  if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("next"))) {
    lxb_dom_node_insert_after(node_self->node, node_new);
  } else if (rb_eql(rb_String(next_or_previous), rb_str_new_literal("previous"))) {
    lxb_dom_node_insert_before(node_self->node, node_new);
  } else {
    rb_raise(rb_eArgError, "Unsupported inserting position");
  }
  return Qnil;
}

void Init_lexbor_node(void)
{
  mLexbor = rb_define_module("Lexbor");
  cLexborNode = rb_define_class_under(mLexbor, "Node", rb_cObject);
  rb_define_method(cLexborNode, "content", lexbor_node_content, 0);
  rb_define_method(cLexborNode, "[]", lexbor_node_get_attr, 1);
  rb_define_method(cLexborNode, "[]=", lexbor_node_set_attr, 2);
  rb_define_method(cLexborNode, "==", lexbor_node_equals, 1);
  rb_define_method(cLexborNode, "css", lexbor_node_css, 1);
  rb_define_method(cLexborNode, "at_css", lexbor_node_at_css, 1);
  rb_define_method(cLexborNode, "inner_html", lexbor_node_inner_html, 0);
  rb_define_method(cLexborNode, "outer_html", lexbor_node_outer_html, 0);
  rb_define_method(cLexborNode, "key?", lexbor_node_has_key, 1);
  rb_define_method(cLexborNode, "keys", lexbor_node_keys, 0);
  rb_define_method(cLexborNode, "values", lexbor_node_values, 0);
  rb_define_method(cLexborNode, "parent", lexbor_node_parent, 0);
  rb_define_method(cLexborNode, "previous", lexbor_node_previous, 0);
  rb_define_method(cLexborNode, "next", lexbor_node_next, 0);
  rb_define_method(cLexborNode, "children", lexbor_node_children, 0);
  rb_define_method(cLexborNode, "child", lexbor_node_child, 0);
  rb_define_method(cLexborNode, "remove", lexbor_node_remove, 0);
  rb_define_method(cLexborNode, "attrs", lexbor_node_attrs, 0);
  rb_define_method(cLexborNode, "name", lexbor_node_name, 0);
  rb_define_method(cLexborNode, "comment?", lexbor_node_is_comment, 0);
  rb_define_method(cLexborNode, "add_sibling", lexbor_node_add_sibling, 2);

  rb_define_alias(cLexborNode, "attr", "[]");
  rb_define_alias(cLexborNode, "text", "content");
  rb_define_alias(cLexborNode, "inner_text", "content");
  rb_define_alias(cLexborNode, "to_str", "content");
  rb_define_alias(cLexborNode, "to_html", "outer_html");
  rb_define_alias(cLexborNode, "previous_element", "previous");
  rb_define_alias(cLexborNode, "next_element", "next");
  rb_define_alias(cLexborNode, "destroy", "remove");
}
