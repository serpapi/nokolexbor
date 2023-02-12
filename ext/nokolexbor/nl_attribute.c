#include "nokolexbor.h"

VALUE cNokolexborAttribute;
extern VALUE mNokolexbor;
extern VALUE cNokolexborNode;

/**
 * call-seq:
 *   new(document, name) -> Attribute
 *
 * Create a new Attribute on the +document+ with +name+.
 *
 * @param document [Document]
 * @param name [String]
 */
static VALUE
nl_attribute_new(int argc, VALUE *argv, VALUE klass)
{
  lxb_dom_document_t *document;
  VALUE rb_document;
  VALUE rb_name;
  VALUE rest;

  rb_scan_args(argc, argv, "2*", &rb_document, &rb_name, &rest);

  if (!rb_obj_is_kind_of(rb_document, cNokolexborDocument)) {
    rb_raise(rb_eArgError, "Document must be a Nokolexbor::Document");
  }

  document = nl_rb_document_unwrap(rb_document);

  const char *c_name = StringValuePtr(rb_name);
  size_t name_len = RSTRING_LEN(rb_name);
  lxb_dom_attr_t *attr = lxb_dom_attr_interface_create(document);
  if (attr == NULL) {
    rb_raise(rb_eRuntimeError, "Error creating attribute");
  }

  lxb_dom_attr_set_name(attr, (const lxb_char_t *)c_name, name_len, false);

  VALUE rb_node = nl_rb_node_create(&attr->node, rb_document);

  if (rb_block_given_p()) {
    rb_yield(rb_node);
  }

  return rb_node;
}

/**
 * Get the name of the Attribute.
 *
 * @return [String]
 */
static VALUE
nl_attribute_name(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  size_t len;
  lxb_char_t *name = lxb_dom_attr_qualified_name(attr, &len);

  return rb_utf8_str_new(name, len);
}

/**
 * call-seq:
 *   name=(name) -> String
 *
 * Set the name of the Attribute.
 */
static VALUE
nl_attribute_set_name(VALUE self, VALUE rb_name)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  const char *c_name = StringValuePtr(rb_name);
  size_t name_len = RSTRING_LEN(rb_name);

  lxb_status_t status = lxb_dom_attr_set_name(attr, (const lxb_char_t *)c_name, name_len, false);
  if (status != LXB_STATUS_OK) {
    nl_raise_lexbor_error(status);
  }

  return rb_name;
}

/**
 * Get the value of the Attribute.
 *
 * @return [String]
 */
static VALUE
nl_attribute_value(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  size_t len;
  lxb_char_t *value = lxb_dom_attr_value(attr, &len);

  return rb_utf8_str_new(value, len);
}

/**
 * call-seq:
 *   value=(value) -> String
 *
 * Set the value of the Attribute.
 */
static VALUE
nl_attribute_set_value(VALUE self, VALUE rb_content)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  const char *c_content = StringValuePtr(rb_content);
  size_t content_len = RSTRING_LEN(rb_content);

  lxb_status_t status = lxb_dom_attr_set_value(attr, (const lxb_char_t *)c_content, content_len);
  if (status != LXB_STATUS_OK) {
    nl_raise_lexbor_error(status);
  }

  return rb_content;
}

/**
 * Get the owner Node of the Attribute.
 *
 * @return [Node]
 */
static VALUE
nl_attribute_parent(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  if (attr->owner == NULL) {
    return Qnil;
  }
  return nl_rb_node_create(attr->owner, nl_rb_document_get(self));
}

/**
 * Get the previous Attribute.
 *
 * @return [Attribute]
 */
static VALUE
nl_attribute_previous(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  if (attr->prev == NULL) {
    return Qnil;
  }
  return nl_rb_node_create(attr->prev, nl_rb_document_get(self));
}

/**
 * Get the next Attribute.
 *
 * @return [Attribute]
 */
static VALUE
nl_attribute_next(VALUE self)
{
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);

  if (attr->next == NULL) {
    return Qnil;
  }
  return nl_rb_node_create(attr->next, nl_rb_document_get(self));
}

static VALUE
nl_attribute_inspect(VALUE self)
{
  VALUE c = rb_class_name(CLASS_OF(self));
  lxb_dom_node_t *node = nl_rb_node_unwrap(self);
  lxb_dom_attr_t *attr = lxb_dom_interface_attr(node);
  size_t len;
  lxb_char_t *attr_value = lxb_dom_attr_value(attr, &len);

  return rb_sprintf("#<%" PRIsVALUE " %s=\"%s\">", c,
                    lxb_dom_attr_qualified_name(attr, &len),
                    attr_value == NULL ? "" : attr_value);
}

void Init_nl_attribute(void)
{
  cNokolexborAttribute = rb_define_class_under(mNokolexbor, "Attribute", cNokolexborNode);

  rb_define_singleton_method(cNokolexborAttribute, "new", nl_attribute_new, -1);

  rb_define_method(cNokolexborAttribute, "name", nl_attribute_name, 0);
  rb_define_method(cNokolexborAttribute, "name=", nl_attribute_set_name, 1);
  rb_define_method(cNokolexborAttribute, "value", nl_attribute_value, 0);
  rb_define_method(cNokolexborAttribute, "value=", nl_attribute_set_value, 1);
  rb_define_method(cNokolexborAttribute, "parent", nl_attribute_parent, 0);
  rb_define_method(cNokolexborAttribute, "previous", nl_attribute_previous, 0);
  rb_define_method(cNokolexborAttribute, "next", nl_attribute_next, 0);
  rb_define_method(cNokolexborAttribute, "inspect", nl_attribute_inspect, 0);

  rb_define_alias(cNokolexborAttribute, "node_name", "name");
  rb_define_alias(cNokolexborAttribute, "node_name=", "name=");
  rb_define_alias(cNokolexborAttribute, "text", "value");
  rb_define_alias(cNokolexborAttribute, "content", "value");
  rb_define_alias(cNokolexborAttribute, "to_s", "value");
  rb_define_alias(cNokolexborAttribute, "to_str", "value");
}
