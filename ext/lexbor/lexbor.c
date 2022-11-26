#include "lexbor.h"

VALUE mLexbor;

void Init_lexbor(void)
{
  mLexbor = rb_define_module("Lexbor");
  Init_lexbor_document();
  Init_lexbor_node();
  Init_lexbor_node_set();
}
