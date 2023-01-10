#include "nokolexbor.h"

VALUE mNokolexbor;

void Init_nokolexbor(void)
{
#ifndef NOKOLEXBOR_ASAN
  lexbor_memory_setup(ruby_xmalloc, ruby_xrealloc, ruby_xcalloc, ruby_xfree);
#endif
  mNokolexbor = rb_define_module("Nokolexbor");
  Init_nl_error();
  Init_nl_node();
  Init_nl_document();
  Init_nl_text();
  Init_nl_comment();
  Init_nl_cdata();
  Init_nl_processing_instruction();
  Init_nl_node_set();
  Init_nl_document_fragment();
  Init_nl_attribute();
  Init_nl_xpath_context();
}
