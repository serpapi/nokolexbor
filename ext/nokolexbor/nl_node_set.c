#include "nokolexbor.h"

extern VALUE mNokolexbor;
VALUE cNokolexborNodeSet;

void Init_nl_node_set(void)
{
  cNokolexborNodeSet = rb_define_class_under(mNokolexbor, "NodeSet", rb_cArray);
}
