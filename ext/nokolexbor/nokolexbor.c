#include "nokolexbor.h"

VALUE mNokolexbor;

void Init_nokolexbor(void)
{
  mNokolexbor = rb_define_module("Nokolexbor");
  Init_nl_node();
  Init_nl_document();
  Init_nl_node_set();
  Init_nl_xpath_context();
}
