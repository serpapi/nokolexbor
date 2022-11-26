#include "lexbor.h"

extern VALUE mLexbor;
VALUE cLexborNodeSet;

void Init_lexbor_node_set(void)
{
  cLexborNodeSet = rb_define_class_under(mLexbor, "NodeSet", rb_cArray);
}
