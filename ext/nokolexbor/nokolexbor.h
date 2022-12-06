#ifndef __NOKOLEXBOR_RUBY_H__
#define __NOKOLEXBOR_RUBY_H__

#include <ruby.h>

#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

typedef struct
{
    lxb_dom_node_t *node;
    VALUE rb_node;
    VALUE rb_document;
} nl_node_t;

typedef struct
{
    nl_node_t nl_node;
    lxb_html_document_t *document;
    VALUE rb_document;
} nl_document_t;

void Init_nl_document(void);
void Init_nl_node(void);
void Init_nl_node_set(void);
void Init_nl_xpath_context(void);

nl_node_t *nl_node_create(lxb_dom_node_t *node);
nl_node_t *nl_node_dup(nl_node_t *nl_node);
nl_node_t *nl_rb_node_unwrap(VALUE rb_node);
VALUE nl_rb_node_create(lxb_dom_node_t *node, VALUE rb_document);
VALUE nl_rb_node_set_create_with_data(lexbor_array_t *array, VALUE rb_document);

const lxb_char_t *
lxb_dom_node_name_qualified(lxb_dom_node_t *node, size_t *len);

#endif