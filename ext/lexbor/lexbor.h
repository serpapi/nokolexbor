#ifndef __LEXBOR_RUBY_H__
#define __LEXBOR_RUBY_H__

#include <ruby.h>

#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

typedef struct
{
    lxb_dom_node_t *node;
    VALUE rb_document;
} lexbor_node_t;

void Init_lexbor_document(void);
void Init_lexbor_node(void);
void Init_lexbor_node_set(void);

inline lexbor_node_t *lexbor_rb_node_unwrap(VALUE rb_node);
VALUE lexbor_rb_node_create(lxb_dom_node_t *node, VALUE rb_document);

#endif