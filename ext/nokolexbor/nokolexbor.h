#ifndef __NOKOLEXBOR_RUBY_H__
#define __NOKOLEXBOR_RUBY_H__

#include <ruby.h>

#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

typedef struct
{
    lxb_dom_node_t *node;
    VALUE rb_document;
} nl_node_t;

void Init_nl_document(void);
void Init_nl_node(void);
void Init_nl_node_set(void);
void Init_nl_xpath_context(void);

nl_node_t *nl_rb_node_unwrap(VALUE rb_node);
VALUE nl_rb_node_create(lxb_dom_node_t *node, VALUE rb_document);

const lxb_char_t *
lxb_dom_node_name_qualified(lxb_dom_node_t *node, size_t *len);

#endif