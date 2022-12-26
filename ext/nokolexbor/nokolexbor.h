#ifndef __NOKOLEXBOR_RUBY_H__
#define __NOKOLEXBOR_RUBY_H__

#include <ruby.h>

#include <lexbor/css/css.h>
#include <lexbor/html/html.h>
#include <lexbor/selectors/selectors.h>

extern VALUE cNokolexborDocument;

void Init_nl_document(void);
void Init_nl_node(void);
void Init_nl_node_set(void);
void Init_nl_text(void);
void Init_nl_comment(void);
void Init_nl_cdata(void);
void Init_nl_xpath_context(void);

void nl_raise_lexbor_error(lxb_status_t error);
lxb_dom_node_t *nl_rb_node_unwrap(VALUE rb_node);
VALUE nl_rb_node_create(lxb_dom_node_t *node, VALUE rb_document);
VALUE nl_rb_node_set_create_with_data(lexbor_array_t *array, VALUE rb_document);

lxb_inline VALUE nl_rb_document_get(VALUE rb_node_or_doc)
{
  if (rb_obj_is_kind_of(rb_node_or_doc, cNokolexborDocument)) {
    return rb_node_or_doc;
  }
  return rb_iv_get(rb_node_or_doc, "@document");
}

lxb_dom_document_t *nl_rb_document_unwrap(VALUE rb_doc);

const lxb_char_t *
lxb_dom_node_name_qualified(lxb_dom_node_t *node, size_t *len);

lxb_status_t
lexbor_array_push_unique(lexbor_array_t *array, void *value);

#endif